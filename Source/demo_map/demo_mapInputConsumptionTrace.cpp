#include "demo_mapInputConsumptionTrace.h"

#if !UE_BUILD_SHIPPING

#include "demo_map.h"
#include "demo_mapPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	Fdemo_mapInputConsumptionTrace* Gdemo_mapInputConsumptionTrace = nullptr;

	uint8 StageBit(Edemo_mapInputConsumptionStage Stage)
	{
		switch (Stage)
		{
		case Edemo_mapInputConsumptionStage::AfterAddMovementInput:
			return 1 << 0;
		case Edemo_mapInputConsumptionStage::PreCharacterMovement:
			return 1 << 1;
		case Edemo_mapInputConsumptionStage::PostCharacterMovement:
			return 1 << 2;
		case Edemo_mapInputConsumptionStage::PostPhysics:
			return 1 << 3;
		default:
			return 0;
		}
	}

	bool IsMovementFrameStage(Edemo_mapInputConsumptionStage Stage)
	{
		return StageBit(Stage) != 0;
	}

	float PlanarDistance(const FVector& A, const FVector& B)
	{
		return FVector::Dist2D(A, B);
	}
}

Fdemo_mapInputConsumptionTrace::Fdemo_mapInputConsumptionTrace(
	bool bInEnabled,
	bool bInGateEnabled)
	: bGateEnabled(bInEnabled && bInGateEnabled),
	  State(
		bInEnabled
			? Edemo_mapInputConsumptionTraceState::Idle
			: Edemo_mapInputConsumptionTraceState::Off)
{
	if (bInEnabled)
	{
		const IConsoleVariable* AsyncMovement =
			IConsoleManager::Get().FindConsoleVariable(
				TEXT("p.CharacterMovement.AsyncCharacterMovement"));
		MovementPath =
			AsyncMovement && AsyncMovement->GetInt() != 0 ? 1 : 0;
	}
}

void Fdemo_mapInputConsumptionTrace::BeginBoundary(
	APawn* Pawn,
	AActor* FocusedChest)
{
	if (!IsEnabled())
	{
		return;
	}

	ObservedPawn = Pawn;
	ObservedChest = FocusedChest;
	BoundaryLocation = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	Count = 0;
	DroppedCount = 0;
	MovementFrameCount = 0;
	LatestPostMovementEntryIndex = INDEX_NONE;
	VerdictFrame = 0;
	bHasLatestPostMovementSnapshot = false;
	bFrozenProbePassed = false;
	ControllerTickSerial = 0;
	LatestControllerTickFrame = 0;
	LatestControllerDeltaSeconds = -1.0f;
	DerivedSecondarySerial = 0;
	DerivedSecondaryFrame = 0;
	DerivedSecondaryDeltaSeconds = -1.0f;
	bReflectedFunctionPresent = false;
	bHandlerIsAlreadyBound = false;
	PreBoundaryActualCallbackCount = 0;
	PreBoundaryLastActualCallbackDeltaSeconds = -1.0f;
	ActualCallbackSerial = 0;
	ActualCallbackFrame = 0;
	LatestActualCallbackWorldTime = -1.0;
	LatestMovementUpdateFrame = 0;
	LatestMovementDeltaSeconds = -1.0f;
	LatestMovementOldLocation = FVector::ZeroVector;
	LatestMovementOldVelocity = FVector::ZeroVector;
	LatestActualCallbackActorLocation = FVector::ZeroVector;
	LatestActualCallbackUpdatedComponentLocation = FVector::ZeroVector;
	LatestActualCallbackVelocity = FVector::ZeroVector;
	LatestActualCallbackMovementMode = INDEX_NONE;
	LatestActualCallbackCustomMovementMode = INDEX_NONE;
	if (Ademo_mapPlayerController* DemoController =
		Pawn
			? Cast<Ademo_mapPlayerController>(Pawn->GetController())
			: nullptr)
	{
		bReflectedFunctionPresent =
			DemoController->
				IsInputConsumptionReflectedFunctionPresentForAutomation();
		bHandlerIsAlreadyBound =
			DemoController->
				IsInputConsumptionHandlerAlreadyBoundForAutomation();
		PreBoundaryActualCallbackCount =
			DemoController->
				GetInputConsumptionActualCallbackCountForAutomation();
		PreBoundaryLastActualCallbackDeltaSeconds =
			DemoController->
				GetInputConsumptionLastActualCallbackDeltaSecondsForAutomation();
		ActualCallbackSerial = PreBoundaryActualCallbackCount;
	}
	MovementUpdateSerial = ActualCallbackSerial;
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (const UCharacterMovementComponent* Movement =
			Character->GetCharacterMovement())
		{
			LastObservedUpdateLocation =
				Movement->GetLastUpdateLocation();
			LastObservedUpdateVelocity =
				Movement->GetLastUpdateVelocity();
			bHasObservedUpdateBaseline = true;
		}
	}
	else
	{
		LastObservedUpdateLocation = FVector::ZeroVector;
		LastObservedUpdateVelocity = FVector::ZeroVector;
		bHasObservedUpdateBaseline = false;
	}
	FMemory::Memzero(MovementFrames, sizeof(MovementFrames));
	FMemory::Memzero(MovementFrameMasks, sizeof(MovementFrameMasks));
	LatestPostMovementSnapshot = {};
	Enrichment = {};
	State = Edemo_mapInputConsumptionTraceState::Active;
}

int32 Fdemo_mapInputConsumptionTrace::FindOrCreateMovementFrame(
	uint64 Frame,
	Edemo_mapInputConsumptionStage Stage)
{
	for (int32 Index = 0; Index < MovementFrameCount; ++Index)
	{
		if (MovementFrames[Index] == Frame)
		{
			return Index;
		}
	}
	if (Stage != Edemo_mapInputConsumptionStage::AfterAddMovementInput
		|| MovementFrameCount >= RequiredMovementFrames)
	{
		return INDEX_NONE;
	}
	const int32 NewIndex = MovementFrameCount++;
	MovementFrames[NewIndex] = Frame;
	return NewIndex;
}

Fdemo_mapInputConsumptionSnapshot
Fdemo_mapInputConsumptionTrace::CaptureRaw(
	APawn* Pawn,
	APlayerController* Controller,
	float ProbeDistanceUU,
	double LatencySeconds,
	const FVector& IntendedInput) const
{
	Fdemo_mapInputConsumptionSnapshot Snapshot;
	Snapshot.Frame = static_cast<uint64>(GFrameCounter);
	Snapshot.WorldSeconds =
		Pawn && Pawn->GetWorld() ? Pawn->GetWorld()->GetTimeSeconds() : -1.0;
	Snapshot.LatencySeconds = LatencySeconds;
	Snapshot.ProbeDistanceUU = ProbeDistanceUU;
	Snapshot.IntendedInput = IntendedInput;
	Snapshot.PawnIdentity = reinterpret_cast<uint64>(Pawn);
	Snapshot.ControllerIdentity = reinterpret_cast<uint64>(Controller);
	Snapshot.MovementPath = MovementPath;
	Snapshot.ControllerTickSerial = ControllerTickSerial;
	Snapshot.MovementUpdateSerial = MovementUpdateSerial;
	Snapshot.ControllerTickFrame = LatestControllerTickFrame;
	Snapshot.MovementUpdateFrame = LatestMovementUpdateFrame;
	Snapshot.ActualCallbackSerial = ActualCallbackSerial;
	Snapshot.ActualCallbackFrame = ActualCallbackFrame;
	Snapshot.ActualCallbackWorldTime = LatestActualCallbackWorldTime;
	Snapshot.ActualCallbackEffectiveDeltaSeconds =
		LatestMovementDeltaSeconds;
	Snapshot.ActualCallbackOldLocation = LatestMovementOldLocation;
	Snapshot.ActualCallbackOldVelocity = LatestMovementOldVelocity;
	Snapshot.ActualCallbackCurrentActorLocation =
		LatestActualCallbackActorLocation;
	Snapshot.ActualCallbackCurrentUpdatedComponentLocation =
		LatestActualCallbackUpdatedComponentLocation;
	Snapshot.ActualCallbackCurrentVelocity =
		LatestActualCallbackVelocity;
	Snapshot.ActualCallbackMovementMode =
		LatestActualCallbackMovementMode;
	Snapshot.ActualCallbackCustomMovementMode =
		LatestActualCallbackCustomMovementMode;
	Snapshot.PreBoundaryActualCallbackCount =
		PreBoundaryActualCallbackCount;
	Snapshot.PreBoundaryLastActualCallbackDeltaSeconds =
		PreBoundaryLastActualCallbackDeltaSeconds;
	Snapshot.DerivedSecondarySerial = DerivedSecondarySerial;
	Snapshot.DerivedSecondaryFrame = DerivedSecondaryFrame;
	Snapshot.DerivedSecondaryDeltaSeconds =
		DerivedSecondaryDeltaSeconds;
	Snapshot.bReflectedFunctionPresent =
		bReflectedFunctionPresent;
	Snapshot.bHandlerIsAlreadyBound = bHandlerIsAlreadyBound;
	Snapshot.bHookQualifiedBeforeBoundary =
		IsHookQualifiedBeforeBoundary();
	Snapshot.ControllerDeltaSeconds = LatestControllerDeltaSeconds;
	Snapshot.EffectiveDeltaSeconds = LatestMovementDeltaSeconds;
	Snapshot.MovementOldLocation = LatestMovementOldLocation;
	Snapshot.MovementOldVelocity = LatestMovementOldVelocity;
	Snapshot.ProbeOrigin = BoundaryLocation;
	Snapshot.bControllerTickEnabled =
		Controller && Controller->IsActorTickEnabled();
	Snapshot.ControllerTickInterval =
		Controller ? Controller->PrimaryActorTick.TickInterval : -1.0f;
	Snapshot.bControllerPossessesPawn =
		Controller && Controller->GetPawn() == Pawn;
	if (!Pawn)
	{
		return Snapshot;
	}

	Snapshot.LocalRole = static_cast<int32>(Pawn->GetLocalRole());
	Snapshot.NetMode = static_cast<int32>(Pawn->GetNetMode());
	Snapshot.PendingInput = Pawn->GetPendingMovementInputVector();
	Snapshot.ConsumedInput = Pawn->GetLastMovementInputVector();
	Snapshot.Velocity = Pawn->GetVelocity();
	Snapshot.Location = Pawn->GetActorLocation();

	const ACharacter* Character = Cast<ACharacter>(Pawn);
	Snapshot.bCharacterMovementUpdateDelegateBound =
		Character && Character->OnCharacterMovementUpdated.IsBound();
	const UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr;
	if (Movement)
	{
		Snapshot.MovementIdentity = reinterpret_cast<uint64>(Movement);
		Snapshot.CharacterOwnerIdentity =
			reinterpret_cast<uint64>(Movement->GetCharacterOwner());
		Snapshot.Acceleration = Movement->GetCurrentAcceleration();
		Snapshot.MaxAcceleration = Movement->GetMaxAcceleration();
		Snapshot.MaxSpeed = Movement->GetMaxSpeed();
		Snapshot.MaxInputSpeed = Movement->GetMaxSpeed()
			* Movement->GetAnalogInputModifier();
		Snapshot.AnalogInputModifier = Movement->GetAnalogInputModifier();
		Snapshot.LastRequestedVelocity =
			Movement->GetLastUpdateRequestedVelocity();
		Snapshot.BrakingDeceleration =
			Movement->GetMaxBrakingDeceleration();
		Snapshot.MovementMode = static_cast<int32>(Movement->MovementMode);
		Snapshot.CustomMovementMode =
			static_cast<int32>(Movement->CustomMovementMode);
		Snapshot.bMovementTickRegistered =
			Movement->PrimaryComponentTick.IsTickFunctionRegistered();
		Snapshot.bMovementTickEnabled = Movement->IsComponentTickEnabled();
		Snapshot.MovementTickInterval =
			Movement->PrimaryComponentTick.TickInterval;
		Snapshot.MovementPrerequisiteCount =
			Movement->PrimaryComponentTick.GetPrerequisites().Num();
		Snapshot.bMovementComponentRegistered = Movement->IsRegistered();
		Snapshot.bMovementComponentActive = Movement->IsActive();
		Snapshot.bHasValidData = Movement->HasValidData();
		Snapshot.bUpdateOnlyIfRendered = Movement->bUpdateOnlyIfRendered;
		Snapshot.bMovingOnGround = Movement->IsMovingOnGround();
		Snapshot.bFalling = Movement->IsFalling();
		Snapshot.bCurrentFloorWalkable =
			Movement->CurrentFloor.IsWalkableFloor();
		Snapshot.FloorDistance = Movement->CurrentFloor.FloorDist;
		Snapshot.bHasAnimRootMotion = Movement->HasAnimRootMotion();
		Snapshot.bHasRootMotionSources = Movement->HasRootMotionSources();
		Snapshot.bMovementInProgress = Movement->IsMovementInProgress();
		Snapshot.bScopedMovementUpdates =
			Movement->bEnableScopedMovementUpdates;
		Snapshot.bActualMovementUpdateFired =
			ActualCallbackSerial > PreBoundaryActualCallbackCount;

		const USceneComponent* UpdatedComponent = Movement->UpdatedComponent;
		Snapshot.UpdatedComponentIdentity =
			reinterpret_cast<uint64>(UpdatedComponent);
		Snapshot.bUpdatedComponentPresent = UpdatedComponent != nullptr;
		if (UpdatedComponent)
		{
			Snapshot.UpdatedComponentLocation =
				UpdatedComponent->GetComponentLocation();
			Snapshot.bUpdatedComponentRegistered =
				UpdatedComponent->IsRegistered();
			Snapshot.bUpdatedComponentActive =
				UpdatedComponent->IsActive();
			Snapshot.UpdatedComponentMobility =
				static_cast<int32>(UpdatedComponent->Mobility);
		}

		const UPrimitiveComponent* UpdatedPrimitive =
			Cast<UPrimitiveComponent>(UpdatedComponent);
		if (UpdatedPrimitive)
		{
			Snapshot.bUpdatedComponentSimulatingPhysics =
				UpdatedPrimitive->IsSimulatingPhysics();
			Snapshot.bUpdatedComponentQueryCollision =
				UpdatedPrimitive->IsQueryCollisionEnabled();
			Snapshot.UpdatedComponentLastRenderTime =
				UpdatedPrimitive->GetLastRenderTime();
			const UWorld* World = Pawn->GetWorld();
			Snapshot.bUpdatedComponentRenderedRecently =
				World
				&& World->TimeSince(
					UpdatedPrimitive->GetLastRenderTime()) <= 0.41f;
		}

		const UObject* MovementBase = Character->GetMovementBaseObject();
		Snapshot.MovementBaseIdentity =
			reinterpret_cast<uint64>(MovementBase);
		Snapshot.bHasMovementBase = MovementBase != nullptr;
	}

	const USkeletalMeshComponent* CharacterMesh =
		Character ? Character->GetMesh() : nullptr;
	if (CharacterMesh)
	{
		Snapshot.CharacterMeshIdentity =
			reinterpret_cast<uint64>(CharacterMesh);
		Snapshot.CharacterMeshLastRenderTime =
			CharacterMesh->GetLastRenderTime();
		const UWorld* World = Pawn->GetWorld();
		Snapshot.bCharacterMeshRenderedRecently =
			World
			&& World->TimeSince(CharacterMesh->GetLastRenderTime())
				<= 0.41f;
	}
	Snapshot.bCharacterPlayingRootMotion =
		Character && Character->IsPlayingRootMotion();

	const UCapsuleComponent* Capsule =
		Character ? Character->GetCapsuleComponent() : nullptr;
	if (Capsule)
	{
		Snapshot.CapsuleIdentity = reinterpret_cast<uint64>(Capsule);
		Capsule->GetScaledCapsuleSize(
			Snapshot.CapsuleRadius,
			Snapshot.CapsuleHalfHeight);
	}
	return Snapshot;
}

void Fdemo_mapInputConsumptionTrace::RecordControllerTick(
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds)
{
	if (!IsActive() || !bGateEnabled)
	{
		return;
	}
	++ControllerTickSerial;
	LatestControllerTickFrame = static_cast<uint64>(GFrameCounter);
	LatestControllerDeltaSeconds = DeltaSeconds;
}

void Fdemo_mapInputConsumptionTrace::RecordMovementUpdated(
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds,
	const FVector& OldLocation,
	const FVector& OldVelocity)
{
	if (!IsActive() || !bGateEnabled)
	{
		return;
	}
	const Ademo_mapPlayerController* DemoController =
		Cast<Ademo_mapPlayerController>(Controller);
	const uint64 ControllerCallbackCount =
		DemoController
			? DemoController->
				GetInputConsumptionActualCallbackCountForAutomation()
			: 0;
	ActualCallbackSerial =
		ControllerCallbackCount > ActualCallbackSerial
			? ControllerCallbackCount
			: ActualCallbackSerial + 1;
	MovementUpdateSerial = ActualCallbackSerial;
	ActualCallbackFrame = static_cast<uint64>(GFrameCounter);
	LatestMovementUpdateFrame = static_cast<uint64>(GFrameCounter);
	LatestMovementDeltaSeconds = DeltaSeconds;
	LatestMovementOldLocation = OldLocation;
	LatestMovementOldVelocity = OldVelocity;
	LatestActualCallbackWorldTime =
		Pawn && Pawn->GetWorld()
			? Pawn->GetWorld()->GetTimeSeconds()
			: -1.0;
	LatestActualCallbackActorLocation =
		Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	LatestActualCallbackVelocity =
		Pawn ? Pawn->GetVelocity() : FVector::ZeroVector;
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCharacterMovementComponent* Movement =
		Character ? Character->GetCharacterMovement() : nullptr;
	if (Movement)
	{
		LatestActualCallbackUpdatedComponentLocation =
			Movement->UpdatedComponent
				? Movement->UpdatedComponent->GetComponentLocation()
				: FVector::ZeroVector;
		LatestActualCallbackVelocity = Movement->Velocity;
		LatestActualCallbackMovementMode =
			static_cast<int32>(Movement->MovementMode);
		LatestActualCallbackCustomMovementMode =
			static_cast<int32>(Movement->CustomMovementMode);
	}
}

void Fdemo_mapInputConsumptionTrace::RecordDerivedMovementUpdated(
	UCharacterMovementComponent* Movement,
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds)
{
	if (!IsActive() || !bGateEnabled || !Movement)
	{
		return;
	}
	const FVector CurrentLocation = Movement->GetLastUpdateLocation();
	const FVector CurrentVelocity = Movement->GetLastUpdateVelocity();
	if (!bHasObservedUpdateBaseline)
	{
		LastObservedUpdateLocation = CurrentLocation;
		LastObservedUpdateVelocity = CurrentVelocity;
		bHasObservedUpdateBaseline = true;
		return;
	}
	if (CurrentLocation.Equals(
			LastObservedUpdateLocation,
			KINDA_SMALL_NUMBER)
		&& CurrentVelocity.Equals(
			LastObservedUpdateVelocity,
			KINDA_SMALL_NUMBER))
	{
		return;
	}
	++DerivedSecondarySerial;
	DerivedSecondaryFrame = static_cast<uint64>(GFrameCounter);
	DerivedSecondaryDeltaSeconds = DeltaSeconds;
	LastObservedUpdateLocation = CurrentLocation;
	LastObservedUpdateVelocity = CurrentVelocity;
}

void Fdemo_mapInputConsumptionTrace::AppendSnapshot(
	Edemo_mapInputConsumptionStage Stage,
	const Fdemo_mapInputConsumptionSnapshot& Snapshot)
{
	if (Count >= Capacity)
	{
		++DroppedCount;
		return;
	}
	Fdemo_mapInputConsumptionEntry& Entry = Entries[Count++];
	Entry.Sequence = static_cast<uint64>(Count);
	Entry.Stage = Stage;
	Entry.Snapshot = Snapshot;
}

void Fdemo_mapInputConsumptionTrace::Record(
	Edemo_mapInputConsumptionStage Stage,
	APawn* Pawn,
	APlayerController* Controller,
	float ProbeDistanceUU,
	double LatencySeconds,
	const FVector& IntendedInput)
{
	const uint64 Frame = static_cast<uint64>(GFrameCounter);
	if (State == Edemo_mapInputConsumptionTraceState::VerdictFrozen)
	{
		if (Stage == Edemo_mapInputConsumptionStage::PostPhysics
			&& Frame == VerdictFrame)
		{
			AppendSnapshot(
				Stage,
				CaptureRaw(
					Pawn,
					Controller,
					ProbeDistanceUU,
					LatencySeconds,
					IntendedInput));
			for (int32 Index = 0; Index < MovementFrameCount; ++Index)
			{
				if (MovementFrames[Index] == Frame)
				{
					MovementFrameMasks[Index] |= PostPhysicsBit;
					break;
				}
			}
		}
		return;
	}
	if (State != Edemo_mapInputConsumptionTraceState::Active)
	{
		return;
	}

	if (IsMovementFrameStage(Stage))
	{
		const int32 FrameIndex = FindOrCreateMovementFrame(Frame, Stage);
		if (FrameIndex == INDEX_NONE)
		{
			if (Stage == Edemo_mapInputConsumptionStage::PostCharacterMovement)
			{
				LatestPostMovementSnapshot =
					CaptureRaw(
						Pawn,
						Controller,
						ProbeDistanceUU,
						LatencySeconds,
						IntendedInput);
				bHasLatestPostMovementSnapshot = true;
				LatestPostMovementEntryIndex = INDEX_NONE;
			}
			return;
		}
		if (Count >= Capacity)
		{
			++DroppedCount;
			return;
		}
		const Fdemo_mapInputConsumptionSnapshot Snapshot =
			CaptureRaw(
				Pawn,
				Controller,
				ProbeDistanceUU,
				LatencySeconds,
				IntendedInput);
		const int32 EntryIndex = Count;
		AppendSnapshot(Stage, Snapshot);
		MovementFrameMasks[FrameIndex] |= StageBit(Stage);
		if (Stage == Edemo_mapInputConsumptionStage::PostCharacterMovement)
		{
			LatestPostMovementEntryIndex = EntryIndex;
			bHasLatestPostMovementSnapshot = false;
		}
		return;
	}

	AppendSnapshot(
		Stage,
		CaptureRaw(
			Pawn,
			Controller,
			ProbeDistanceUU,
			LatencySeconds,
			IntendedInput));
}

void Fdemo_mapInputConsumptionTrace::FreezeVerdict(
	bool bProbePassed,
	APawn* Pawn,
	APlayerController* Controller,
	float ProbeDistanceUU,
	double LatencySeconds)
{
	if (State != Edemo_mapInputConsumptionTraceState::Active)
	{
		return;
	}

	const Edemo_mapInputConsumptionStage TerminalStage =
		bProbePassed
			? Edemo_mapInputConsumptionStage::ProbePass
			: Edemo_mapInputConsumptionStage::ProbeFail;
	Record(
		TerminalStage,
		Pawn,
		Controller,
		ProbeDistanceUU,
		LatencySeconds);
	VerdictFrame = static_cast<uint64>(GFrameCounter);
	bFrozenProbePassed = bProbePassed;
	State = Edemo_mapInputConsumptionTraceState::VerdictFrozen;

	const bool bEntryMatchesVerdict =
		LatestPostMovementEntryIndex >= 0
		&& LatestPostMovementEntryIndex < Count
		&& Entries[LatestPostMovementEntryIndex].Snapshot.Frame == VerdictFrame;
	if (!bEntryMatchesVerdict
		&& bHasLatestPostMovementSnapshot
		&& LatestPostMovementSnapshot.Frame == VerdictFrame)
	{
		AppendSnapshot(
			Edemo_mapInputConsumptionStage::PostCharacterMovement,
			LatestPostMovementSnapshot);
	}
}

bool Fdemo_mapInputConsumptionTrace::HasVerdictFramePostHooks() const
{
	bool bPostMovement = false;
	bool bPostPhysics = false;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Entries[Index].Snapshot.Frame != VerdictFrame)
		{
			continue;
		}
		bPostMovement |=
			Entries[Index].Stage
				== Edemo_mapInputConsumptionStage::PostCharacterMovement;
		bPostPhysics |=
			Entries[Index].Stage
				== Edemo_mapInputConsumptionStage::PostPhysics;
	}
	return VerdictFrame != 0 && bPostMovement && bPostPhysics;
}

bool Fdemo_mapInputConsumptionTrace::IsReadyForPostTerminalEnrichment() const
{
	if (State != Edemo_mapInputConsumptionTraceState::VerdictFrozen)
	{
		return false;
	}
	const uint64 CurrentFrame = static_cast<uint64>(GFrameCounter);
	return HasVerdictFramePostHooks()
		|| CurrentFrame > VerdictFrame + 2;
}

void Fdemo_mapInputConsumptionTrace::EnrichAfterVerdict()
{
	if (!IsReadyForPostTerminalEnrichment())
	{
		return;
	}

	APawn* Pawn = ObservedPawn.Get();
	const ACharacter* Character = Cast<ACharacter>(Pawn);
	const UCapsuleComponent* Capsule =
		Character ? Character->GetCapsuleComponent() : nullptr;
	if (Capsule)
	{
		Enrichment.CapsuleCollisionEnabled =
			static_cast<int32>(Capsule->GetCollisionEnabled());
		Enrichment.CapsuleObjectType =
			static_cast<int32>(Capsule->GetCollisionObjectType());
	}

	AActor* Chest = ObservedChest.Get();
	if (Chest)
	{
		Enrichment.ChestIdentity = reinterpret_cast<uint64>(Chest);
		const FBox Bounds = Chest->GetComponentsBoundingBox(true);
		Enrichment.ChestBoundsOrigin = Bounds.GetCenter();
		Enrichment.ChestBoundsExtent = Bounds.GetExtent();
		const UPrimitiveComponent* ChestRoot =
			Chest->FindComponentByClass<UPrimitiveComponent>();
		Enrichment.ChestCollisionEnabled =
			ChestRoot
				? static_cast<int32>(ChestRoot->GetCollisionEnabled())
				: INDEX_NONE;
		Enrichment.bCapsuleOverlapsChest =
			Capsule && Capsule->IsOverlappingActor(Chest);
	}

	FVector IntendedInput = FVector::ZeroVector;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Entries[Index].Stage
				== Edemo_mapInputConsumptionStage::AfterAddMovementInput
			&& !Entries[Index].Snapshot.IntendedInput.IsNearlyZero())
		{
			IntendedInput = Entries[Index].Snapshot.IntendedInput;
			break;
		}
	}
	if (Pawn && Capsule && Pawn->GetWorld() && !IntendedInput.IsNearlyZero())
	{
		float Radius = 0.0f;
		float HalfHeight = 0.0f;
		Capsule->GetScaledCapsuleSize(Radius, HalfHeight);
		const FVector Start = Pawn->GetActorLocation();
		const FVector End = Start + IntendedInput.GetSafeNormal2D() * 10.0f;
		FCollisionQueryParams QueryParams(
			SCENE_QUERY_STAT(InputConsumptionMicroTraceEnrichment),
			false,
			Pawn);
		FHitResult Hit;
		Enrichment.bBlockingHit =
			Pawn->GetWorld()->SweepSingleByChannel(
				Hit,
				Start,
				End,
				FQuat::Identity,
				ECC_Pawn,
				FCollisionShape::MakeCapsule(Radius, HalfHeight),
				QueryParams);
		Enrichment.bStartPenetrating = Hit.bStartPenetrating;
		Enrichment.BlockingActorIdentity =
			reinterpret_cast<uint64>(Hit.GetActor());
	}

	// P8.9 source audit found no project-owned writer on this player path.
	// A concrete M6 can therefore only be emitted from the measured
	// PostMovement -> world-post-actor transform rollback itself.
	Enrichment.bProjectOwnedMovementZeroWriter = false;
	Enrichment.bCompleted = true;
	State = Edemo_mapInputConsumptionTraceState::Enriched;
}

bool Fdemo_mapInputConsumptionTrace::HasCompleteRequiredFrames() const
{
	if (MovementFrameCount != RequiredMovementFrames)
	{
		return false;
	}
	for (int32 Index = 0; Index < RequiredMovementFrames; ++Index)
	{
		if (MovementFrameMasks[Index] != CompleteFrameMask)
		{
			return false;
		}
	}
	return true;
}

const Fdemo_mapInputConsumptionEntry*
Fdemo_mapInputConsumptionTrace::GetEntry(int32 Index) const
{
	return Index >= 0 && Index < Count ? &Entries[Index] : nullptr;
}

Fdemo_mapInputConsumptionClassificationEvidence
Fdemo_mapInputConsumptionTrace::BuildClassificationEvidence() const
{
	Fdemo_mapInputConsumptionClassificationEvidence Evidence;
	Evidence.bComplete = HasCompleteTrace();
	if (MovementFrameCount == 0)
	{
		return Evidence;
	}

	const uint64 FirstFrame = MovementFrames[0];
	const Fdemo_mapInputConsumptionSnapshot* AfterAdd = nullptr;
	const Fdemo_mapInputConsumptionSnapshot* PreMovement = nullptr;
	const Fdemo_mapInputConsumptionSnapshot* PostMovement = nullptr;
	const Fdemo_mapInputConsumptionSnapshot* PostPhysics = nullptr;
	const Fdemo_mapInputConsumptionSnapshot* Terminal = nullptr;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const Fdemo_mapInputConsumptionEntry& Entry = Entries[Index];
		if (Entry.Snapshot.Frame == FirstFrame)
		{
			if (Entry.Stage
				== Edemo_mapInputConsumptionStage::AfterAddMovementInput)
			{
				AfterAdd = &Entry.Snapshot;
			}
			else if (Entry.Stage
				== Edemo_mapInputConsumptionStage::PreCharacterMovement)
			{
				PreMovement = &Entry.Snapshot;
			}
			else if (Entry.Stage
				== Edemo_mapInputConsumptionStage::PostCharacterMovement)
			{
				PostMovement = &Entry.Snapshot;
			}
			else if (Entry.Stage
				== Edemo_mapInputConsumptionStage::PostPhysics)
			{
				PostPhysics = &Entry.Snapshot;
			}
		}
		if (Entry.Stage == Edemo_mapInputConsumptionStage::ProbePass
			|| Entry.Stage == Edemo_mapInputConsumptionStage::ProbeFail)
		{
			Terminal = &Entry.Snapshot;
		}
	}

	Evidence.bIntendedNonZero =
		AfterAdd && !AfterAdd->IntendedInput.IsNearlyZero();
	Evidence.bAfterAddNonZero =
		AfterAdd && !AfterAdd->PendingInput.IsNearlyZero();
	Evidence.bPreMovementNonZero =
		PreMovement && !PreMovement->PendingInput.IsNearlyZero();
	Evidence.bConsumedNonZero =
		PostMovement && !PostMovement->ConsumedInput.IsNearlyZero();
	Evidence.bAccelerationNonZero =
		PostMovement && !PostMovement->Acceleration.IsNearlyZero();
	Evidence.bPostMovementDisplaced =
		PostMovement
		&& PlanarDistance(BoundaryLocation, PostMovement->Location)
			> KINDA_SMALL_NUMBER;
	Evidence.bPostPhysicsDisplaced =
		PostPhysics
		&& PlanarDistance(BoundaryLocation, PostPhysics->Location)
			> KINDA_SMALL_NUMBER;
	Evidence.bCollisionOrPenetration =
		Enrichment.bCapsuleOverlapsChest
		|| Enrichment.bStartPenetrating
		|| Enrichment.bBlockingHit;
	if (Terminal && PostPhysics)
	{
		const float Measured =
			PlanarDistance(BoundaryLocation, PostPhysics->Location);
		Evidence.bProbeAgreesWithPostPhysics =
			Terminal->PawnIdentity == PostPhysics->PawnIdentity
			&& (Terminal->ProbeDistanceUU < 0.0f
				|| FMath::IsNearlyEqual(
					Terminal->ProbeDistanceUU,
					Measured,
					0.25f));
	}
	Evidence.bConcreteResidual =
		Evidence.bComplete
		&& Evidence.bIntendedNonZero
		&& Evidence.bAfterAddNonZero
		&& Evidence.bPreMovementNonZero
		&& Evidence.bConsumedNonZero
		&& Evidence.bAccelerationNonZero;
	return Evidence;
}

const TCHAR* Fdemo_mapInputConsumptionTrace::ClassifyEvidence(
	const Fdemo_mapInputConsumptionClassificationEvidence& Evidence,
	bool bProbePassed)
{
	if (bProbePassed)
	{
		return TEXT("PASS");
	}
	if (!Evidence.bComplete)
	{
		return TEXT("INCOMPLETE");
	}
	if (Evidence.bIntendedNonZero && !Evidence.bAfterAddNonZero)
	{
		return TEXT("M1");
	}
	if (Evidence.bAfterAddNonZero && !Evidence.bPreMovementNonZero)
	{
		return TEXT("M2");
	}
	if (Evidence.bPreMovementNonZero && !Evidence.bConsumedNonZero)
	{
		return TEXT("M3");
	}
	if (Evidence.bConsumedNonZero && !Evidence.bAccelerationNonZero)
	{
		return TEXT("M4");
	}
	if (Evidence.bAccelerationNonZero
		&& !Evidence.bPostMovementDisplaced
		&& Evidence.bCollisionOrPenetration)
	{
		return TEXT("M5");
	}
	if (Evidence.bPostMovementDisplaced
		&& !Evidence.bPostPhysicsDisplaced)
	{
		return TEXT("M6");
	}
	if (Evidence.bPostPhysicsDisplaced
		&& !Evidence.bProbeAgreesWithPostPhysics)
	{
		return TEXT("M7");
	}
	return Evidence.bConcreteResidual ? TEXT("M8") : TEXT("INCOMPLETE");
}

const TCHAR* Fdemo_mapInputConsumptionTrace::ClassifyRootCause() const
{
	return ClassifyEvidence(
		BuildClassificationEvidence(),
		bFrozenProbePassed);
}

const TCHAR* Fdemo_mapInputConsumptionTrace::ClassifyInternalGate() const
{
	if (bFrozenProbePassed)
	{
		return TEXT("PASS");
	}
	if (!bGateEnabled || !HasCompleteTrace())
	{
		return TEXT("INCOMPLETE");
	}

	const Fdemo_mapInputConsumptionSnapshot* PostMovement = nullptr;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Entries[Index].Stage
			== Edemo_mapInputConsumptionStage::PostCharacterMovement)
		{
			PostMovement = &Entries[Index].Snapshot;
			break;
		}
	}
	if (!PostMovement)
	{
		return TEXT("INCOMPLETE");
	}
	if (!PostMovement->bReflectedFunctionPresent
		|| !PostMovement->bHandlerIsAlreadyBound
		|| PostMovement->PreBoundaryActualCallbackCount < 1
		|| !FMath::IsFinite(
			PostMovement->
				PreBoundaryLastActualCallbackDeltaSeconds)
		|| PostMovement->
				PreBoundaryLastActualCallbackDeltaSeconds <= 0.0f)
	{
		return TEXT("INCOMPLETE");
	}

	// ShouldSkipUpdate is evaluated after ConsumeInputVector in UE 5.8.
	// Therefore consumed input and cached acceleration are not proof that the
	// current PerformMovement ran. Public predicate facts take precedence over
	// the absence of the actual movement-update delegate.
	if (!PostMovement->bHasValidData
		|| !PostMovement->bUpdatedComponentPresent
		|| !PostMovement->bUpdatedComponentRegistered
		|| PostMovement->UpdatedComponentMobility
			!= static_cast<int32>(EComponentMobility::Movable)
		|| PostMovement->bUpdatedComponentSimulatingPhysics
		|| (PostMovement->bUpdateOnlyIfRendered
			&& !PostMovement->bUpdatedComponentRenderedRecently
			&& !PostMovement->bCharacterMeshRenderedRecently))
	{
		return TEXT("I2");
	}
	if (!PostMovement->bActualMovementUpdateFired
		|| PostMovement->ActualCallbackSerial
			<= PostMovement->PreBoundaryActualCallbackCount)
	{
		return TEXT("I1");
	}
	if (PostMovement->EffectiveDeltaSeconds <= 1.e-6f)
	{
		return TEXT("I3");
	}
	if (PostMovement->bHasAnimRootMotion
		|| PostMovement->bHasRootMotionSources
		|| PostMovement->bCharacterPlayingRootMotion
		|| PostMovement->bMovementInProgress)
	{
		return TEXT("I4");
	}
	if (PostMovement->AnalogInputModifier <= KINDA_SMALL_NUMBER
		|| PostMovement->MaxInputSpeed <= KINDA_SMALL_NUMBER)
	{
		return TEXT("I5");
	}
	const bool bVelocityProduced =
		!PostMovement->Velocity.IsNearlyZero()
		|| !PostMovement->MovementOldVelocity.Equals(
			PostMovement->Velocity,
			KINDA_SMALL_NUMBER);
	const bool bUpdatedComponentMoved =
		PlanarDistance(
			PostMovement->MovementOldLocation,
			PostMovement->UpdatedComponentLocation)
			> KINDA_SMALL_NUMBER;
	if (!bVelocityProduced && !bUpdatedComponentMoved)
	{
		return TEXT("I6");
	}
	if (bVelocityProduced && !bUpdatedComponentMoved)
	{
		return TEXT("I7");
	}
	return TEXT("INCOMPLETE");
}

void Fdemo_mapInputConsumptionTrace::FlushToLog(
	const TCHAR* Terminal) const
{
	if (State != Edemo_mapInputConsumptionTraceState::Enriched)
	{
		return;
	}
	State = Edemo_mapInputConsumptionTraceState::Flushed;
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_MOVEMENT_HOOK_QUALIFICATION terminal=%s reflected_function_present=%d handler_is_already_bound=%d pre_boundary_actual_callback_count=%llu pre_boundary_last_delta_seconds=%.6f hook_qualified_before_boundary=%d actual_callback_serial=%llu callback_frame=%llu callback_world_time=%.6f callback_effective_delta_seconds=%.6f callback_old_location=(%.3f,%.3f,%.3f) callback_old_velocity=(%.3f,%.3f,%.3f) callback_current_actor_location=(%.3f,%.3f,%.3f) callback_current_updated_component_location=(%.3f,%.3f,%.3f) callback_current_velocity=(%.3f,%.3f,%.3f) movement_mode=%d custom_mode=%d derived_secondary_serial=%llu derived_secondary_frame=%llu derived_secondary_delta_seconds=%.6f"),
		Terminal ? Terminal : TEXT("Unknown"),
		bReflectedFunctionPresent,
		bHandlerIsAlreadyBound,
		PreBoundaryActualCallbackCount,
		PreBoundaryLastActualCallbackDeltaSeconds,
		IsHookQualifiedBeforeBoundary(),
		ActualCallbackSerial,
		ActualCallbackFrame,
		LatestActualCallbackWorldTime,
		LatestMovementDeltaSeconds,
		LatestMovementOldLocation.X,
		LatestMovementOldLocation.Y,
		LatestMovementOldLocation.Z,
		LatestMovementOldVelocity.X,
		LatestMovementOldVelocity.Y,
		LatestMovementOldVelocity.Z,
		LatestActualCallbackActorLocation.X,
		LatestActualCallbackActorLocation.Y,
		LatestActualCallbackActorLocation.Z,
		LatestActualCallbackUpdatedComponentLocation.X,
		LatestActualCallbackUpdatedComponentLocation.Y,
		LatestActualCallbackUpdatedComponentLocation.Z,
		LatestActualCallbackVelocity.X,
		LatestActualCallbackVelocity.Y,
		LatestActualCallbackVelocity.Z,
		LatestActualCallbackMovementMode,
		LatestActualCallbackCustomMovementMode,
		DerivedSecondarySerial,
		DerivedSecondaryFrame,
		DerivedSecondaryDeltaSeconds);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const Fdemo_mapInputConsumptionEntry& Entry = Entries[Index];
		const Fdemo_mapInputConsumptionSnapshot& Raw = Entry.Snapshot;
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("INPUT_CONSUMPTION_MICROTRACE terminal=%s seq=%llu frame=%llu stage=%s world_seconds=%.6f latency=%.6f probe_distance=%.3f intended=(%.4f,%.4f,%.4f) pending=(%.4f,%.4f,%.4f) consumed=(%.4f,%.4f,%.4f) acceleration=(%.3f,%.3f,%.3f) velocity=(%.3f,%.3f,%.3f) location=(%.3f,%.3f,%.3f) movement_path=%s movement_mode=%d movement_tick_registered=%d movement_tick_enabled=%d controller_tick_enabled=%d max_acceleration=%.3f max_speed=%.3f braking_deceleration=%.3f pawn=0x%llx controller=0x%llx movement=0x%llx capsule=0x%llx capsule_radius=%.3f capsule_half_height=%.3f"),
			Terminal ? Terminal : TEXT("Unknown"),
			Entry.Sequence,
			Raw.Frame,
			StageName(Entry.Stage),
			Raw.WorldSeconds,
			Raw.LatencySeconds,
			Raw.ProbeDistanceUU,
			Raw.IntendedInput.X,
			Raw.IntendedInput.Y,
			Raw.IntendedInput.Z,
			Raw.PendingInput.X,
			Raw.PendingInput.Y,
			Raw.PendingInput.Z,
			Raw.ConsumedInput.X,
			Raw.ConsumedInput.Y,
			Raw.ConsumedInput.Z,
			Raw.Acceleration.X,
			Raw.Acceleration.Y,
			Raw.Acceleration.Z,
			Raw.Velocity.X,
			Raw.Velocity.Y,
			Raw.Velocity.Z,
			Raw.Location.X,
			Raw.Location.Y,
			Raw.Location.Z,
			Raw.MovementPath == 1 ? TEXT("Async") : TEXT("Sync"),
			Raw.MovementMode,
			Raw.bMovementTickRegistered,
			Raw.bMovementTickEnabled,
			Raw.bControllerTickEnabled,
			Raw.MaxAcceleration,
			Raw.MaxSpeed,
			Raw.BrakingDeceleration,
			Raw.PawnIdentity,
			Raw.ControllerIdentity,
			Raw.MovementIdentity,
			Raw.CapsuleIdentity,
			Raw.CapsuleRadius,
			Raw.CapsuleHalfHeight);
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("INPUT_MOVEMENT_GATE_TRACE terminal=%s seq=%llu frame=%llu stage=%s gate_enabled=%d hook_bound=%d movement_update_source=actual_character_delegate_derived_secondary_separate controller_serial=%llu controller_frame=%llu controller_delta=%.6f movement_update_serial=%llu movement_update_frame=%llu effective_delta=%.6f actual_movement_update=%d valid_data=%d movement_registered=%d movement_active=%d updated_component=0x%llx updated_registered=%d updated_active=%d updated_mobility=%d updated_simulating=%d updated_query=%d update_only_if_rendered=%d updated_render_recent=%d mesh=0x%llx mesh_render_recent=%d old_location=(%.3f,%.3f,%.3f) updated_location=(%.3f,%.3f,%.3f) old_velocity=(%.3f,%.3f,%.3f) velocity=(%.3f,%.3f,%.3f) analog=%.6f max_input_speed=%.3f requested_velocity=(%.3f,%.3f,%.3f) root_anim=%d root_sources=%d playing_root_motion=%d movement_in_progress=%d scoped=%d floor_walkable=%d floor_distance=%.3f base=0x%llx controller_possesses=%d local_role=%d net_mode=%d movement_tick_interval=%.6f controller_tick_interval=%.6f prerequisites=%d"),
			Terminal ? Terminal : TEXT("Unknown"),
			Entry.Sequence,
			Raw.Frame,
			StageName(Entry.Stage),
			bGateEnabled,
			Raw.bCharacterMovementUpdateDelegateBound,
			Raw.ControllerTickSerial,
			Raw.ControllerTickFrame,
			Raw.ControllerDeltaSeconds,
			Raw.MovementUpdateSerial,
			Raw.MovementUpdateFrame,
			Raw.EffectiveDeltaSeconds,
			Raw.bActualMovementUpdateFired,
			Raw.bHasValidData,
			Raw.bMovementComponentRegistered,
			Raw.bMovementComponentActive,
			Raw.UpdatedComponentIdentity,
			Raw.bUpdatedComponentRegistered,
			Raw.bUpdatedComponentActive,
			Raw.UpdatedComponentMobility,
			Raw.bUpdatedComponentSimulatingPhysics,
			Raw.bUpdatedComponentQueryCollision,
			Raw.bUpdateOnlyIfRendered,
			Raw.bUpdatedComponentRenderedRecently,
			Raw.CharacterMeshIdentity,
			Raw.bCharacterMeshRenderedRecently,
			Raw.MovementOldLocation.X,
			Raw.MovementOldLocation.Y,
			Raw.MovementOldLocation.Z,
			Raw.UpdatedComponentLocation.X,
			Raw.UpdatedComponentLocation.Y,
			Raw.UpdatedComponentLocation.Z,
			Raw.MovementOldVelocity.X,
			Raw.MovementOldVelocity.Y,
			Raw.MovementOldVelocity.Z,
			Raw.Velocity.X,
			Raw.Velocity.Y,
			Raw.Velocity.Z,
			Raw.AnalogInputModifier,
			Raw.MaxInputSpeed,
			Raw.LastRequestedVelocity.X,
			Raw.LastRequestedVelocity.Y,
			Raw.LastRequestedVelocity.Z,
			Raw.bHasAnimRootMotion,
			Raw.bHasRootMotionSources,
			Raw.bCharacterPlayingRootMotion,
			Raw.bMovementInProgress,
			Raw.bScopedMovementUpdates,
			Raw.bCurrentFloorWalkable,
			Raw.FloorDistance,
			Raw.MovementBaseIdentity,
			Raw.bControllerPossessesPawn,
			Raw.LocalRole,
			Raw.NetMode,
			Raw.MovementTickInterval,
			Raw.ControllerTickInterval,
			Raw.MovementPrerequisiteCount);
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_CONSUMPTION_MICROTRACE terminal=%s stage=MicroTraceSummary summary=1 count=%d capacity=%d dropped=%d movement_frames=%d required_frames=%d movement_complete=%d verdict_frame=%llu verdict_post_hooks=%d complete=%d probe_passed=%d root_class=%s enrichment_complete=%d chest=0x%llx chest_bounds_origin=(%.3f,%.3f,%.3f) chest_bounds_extent=(%.3f,%.3f,%.3f) capsule_collision=%d capsule_object_type=%d chest_collision=%d overlaps_chest=%d start_penetrating=%d blocking_hit=%d blocking_actor=0x%llx writer_audit=NoProjectOwnedPlayerMovementZeroWriter"),
		Terminal ? Terminal : TEXT("Unknown"),
		Count,
		Capacity,
		DroppedCount,
		MovementFrameCount,
		RequiredMovementFrames,
		HasCompleteRequiredFrames(),
		VerdictFrame,
		HasVerdictFramePostHooks(),
		HasCompleteTrace(),
		bFrozenProbePassed,
		ClassifyRootCause(),
		Enrichment.bCompleted,
		Enrichment.ChestIdentity,
		Enrichment.ChestBoundsOrigin.X,
		Enrichment.ChestBoundsOrigin.Y,
		Enrichment.ChestBoundsOrigin.Z,
		Enrichment.ChestBoundsExtent.X,
		Enrichment.ChestBoundsExtent.Y,
		Enrichment.ChestBoundsExtent.Z,
		Enrichment.CapsuleCollisionEnabled,
		Enrichment.CapsuleObjectType,
		Enrichment.ChestCollisionEnabled,
		Enrichment.bCapsuleOverlapsChest,
		Enrichment.bStartPenetrating,
		Enrichment.bBlockingHit,
		Enrichment.BlockingActorIdentity);
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_MOVEMENT_GATE_TRACE terminal=%s stage=GateTraceSummary summary=1 gate_enabled=%d count=%d capacity=%d dropped=%d complete=%d probe_passed=%d internal_class=%s"),
		Terminal ? Terminal : TEXT("Unknown"),
		bGateEnabled,
		Count,
		Capacity,
		DroppedCount,
		HasCompleteTrace(),
		bFrozenProbePassed,
		ClassifyInternalGate());
}

const TCHAR* Fdemo_mapInputConsumptionTrace::StageName(
	Edemo_mapInputConsumptionStage Stage)
{
	switch (Stage)
	{
	case Edemo_mapInputConsumptionStage::ExistingProbeBoundaryEmitted:
		return TEXT("ExistingProbeBoundaryEmitted");
	case Edemo_mapInputConsumptionStage::MoveKeyDispatch:
		return TEXT("MoveKeyDispatch");
	case Edemo_mapInputConsumptionStage::MoveActionHandlerEntered:
		return TEXT("MoveActionHandlerEntered");
	case Edemo_mapInputConsumptionStage::AfterAddMovementInput:
		return TEXT("AfterAddMovementInput");
	case Edemo_mapInputConsumptionStage::PreCharacterMovement:
		return TEXT("PreCharacterMovement");
	case Edemo_mapInputConsumptionStage::PostCharacterMovement:
		return TEXT("PostCharacterMovement");
	case Edemo_mapInputConsumptionStage::PostPhysics:
		return TEXT("PostPhysics");
	case Edemo_mapInputConsumptionStage::ProbeSample:
		return TEXT("ProbeSample");
	case Edemo_mapInputConsumptionStage::ProbePass:
		return TEXT("ProbePass");
	case Edemo_mapInputConsumptionStage::ProbeFail:
		return TEXT("ProbeFail");
	default:
		return TEXT("Unknown");
	}
}

bool Isdemo_mapInputConsumptionTraceRuntimeActive()
{
	return Gdemo_mapInputConsumptionTrace != nullptr
		&& Gdemo_mapInputConsumptionTrace->IsActive();
}

void Setdemo_mapInputConsumptionTraceRuntime(
	Fdemo_mapInputConsumptionTrace* Trace)
{
	Gdemo_mapInputConsumptionTrace = Trace;
}

void Recorddemo_mapInputConsumptionTraceStage(
	Edemo_mapInputConsumptionStage Stage,
	APawn* Pawn,
	APlayerController* Controller,
	float ProbeDistanceUU,
	double LatencySeconds,
	const FVector& IntendedInput)
{
	if (Gdemo_mapInputConsumptionTrace)
	{
		Gdemo_mapInputConsumptionTrace->Record(
			Stage,
			Pawn,
			Controller,
			ProbeDistanceUU,
			LatencySeconds,
			IntendedInput);
	}
}

void Recorddemo_mapInputConsumptionControllerTick(
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds)
{
	if (Gdemo_mapInputConsumptionTrace)
	{
		Gdemo_mapInputConsumptionTrace->RecordControllerTick(
			Pawn,
			Controller,
			DeltaSeconds);
	}
}

void Recorddemo_mapInputConsumptionMovementUpdated(
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds,
	const FVector& OldLocation,
	const FVector& OldVelocity)
{
	if (Gdemo_mapInputConsumptionTrace)
	{
		Gdemo_mapInputConsumptionTrace->RecordMovementUpdated(
			Pawn,
			Controller,
			DeltaSeconds,
			OldLocation,
			OldVelocity);
	}
}

void Recorddemo_mapInputConsumptionDerivedMovementUpdated(
	UCharacterMovementComponent* Movement,
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds)
{
	if (Gdemo_mapInputConsumptionTrace)
	{
		Gdemo_mapInputConsumptionTrace->RecordDerivedMovementUpdated(
			Movement,
			Pawn,
			Controller,
			DeltaSeconds);
	}
}

#endif
