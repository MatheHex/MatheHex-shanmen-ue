#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class AActor;
class APawn;
class APlayerController;
class UCharacterMovementComponent;

enum class Edemo_mapInputConsumptionStage : uint8
{
	ExistingProbeBoundaryEmitted,
	MoveKeyDispatch,
	MoveActionHandlerEntered,
	AfterAddMovementInput,
	PreCharacterMovement,
	PostCharacterMovement,
	PostPhysics,
	ProbeSample,
	ProbePass,
	ProbeFail
};

enum class Edemo_mapInputConsumptionTraceState : uint8
{
	Off,
	Idle,
	Active,
	VerdictFrozen,
	Enriched,
	Flushed
};

/**
 * Fixed POD copied by the live callbacks. It deliberately contains no
 * FString, container, UObject smart pointer, collision result, or formatted
 * identity.
 */
struct Fdemo_mapInputConsumptionSnapshot
{
	uint64 Frame = 0;
	double WorldSeconds = -1.0;
	double LatencySeconds = -1.0;
	float ProbeDistanceUU = -1.0f;
	FVector IntendedInput = FVector::ZeroVector;
	FVector PendingInput = FVector::ZeroVector;
	FVector ConsumedInput = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector Location = FVector::ZeroVector;
	FVector UpdatedComponentLocation = FVector::ZeroVector;
	FVector MovementOldLocation = FVector::ZeroVector;
	FVector MovementOldVelocity = FVector::ZeroVector;
	FVector LastRequestedVelocity = FVector::ZeroVector;
	FVector ProbeOrigin = FVector::ZeroVector;
	float MaxAcceleration = 0.0f;
	float MaxSpeed = 0.0f;
	float MaxInputSpeed = 0.0f;
	float AnalogInputModifier = 0.0f;
	float BrakingDeceleration = 0.0f;
	float EffectiveDeltaSeconds = -1.0f;
	float ControllerDeltaSeconds = -1.0f;
	float MovementTickInterval = -1.0f;
	float ControllerTickInterval = -1.0f;
	float FloorDistance = 0.0f;
	float UpdatedComponentLastRenderTime = -1.0f;
	float CharacterMeshLastRenderTime = -1.0f;
	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	uint64 ControllerTickSerial = 0;
	uint64 MovementUpdateSerial = 0;
	uint64 ControllerTickFrame = 0;
	uint64 MovementUpdateFrame = 0;
	uint64 ActualCallbackSerial = 0;
	uint64 ActualCallbackFrame = 0;
	uint64 PreBoundaryActualCallbackCount = 0;
	uint64 DerivedSecondarySerial = 0;
	uint64 DerivedSecondaryFrame = 0;
	double ActualCallbackWorldTime = -1.0;
	float ActualCallbackEffectiveDeltaSeconds = -1.0f;
	float PreBoundaryLastActualCallbackDeltaSeconds = -1.0f;
	float DerivedSecondaryDeltaSeconds = -1.0f;
	FVector ActualCallbackOldLocation = FVector::ZeroVector;
	FVector ActualCallbackOldVelocity = FVector::ZeroVector;
	FVector ActualCallbackCurrentActorLocation = FVector::ZeroVector;
	FVector ActualCallbackCurrentUpdatedComponentLocation =
		FVector::ZeroVector;
	FVector ActualCallbackCurrentVelocity = FVector::ZeroVector;
	int32 ActualCallbackMovementMode = INDEX_NONE;
	int32 ActualCallbackCustomMovementMode = INDEX_NONE;
	uint64 PawnIdentity = 0;
	uint64 ControllerIdentity = 0;
	uint64 MovementIdentity = 0;
	uint64 UpdatedComponentIdentity = 0;
	uint64 CharacterOwnerIdentity = 0;
	uint64 MovementBaseIdentity = 0;
	uint64 CharacterMeshIdentity = 0;
	uint64 CapsuleIdentity = 0;
	int32 MovementMode = INDEX_NONE;
	int32 CustomMovementMode = INDEX_NONE;
	int32 MovementPath = INDEX_NONE;
	int32 LocalRole = INDEX_NONE;
	int32 NetMode = INDEX_NONE;
	int32 UpdatedComponentMobility = INDEX_NONE;
	int32 MovementPrerequisiteCount = 0;
	bool bMovementTickRegistered = false;
	bool bMovementTickEnabled = false;
	bool bMovementComponentRegistered = false;
	bool bMovementComponentActive = false;
	bool bControllerTickEnabled = false;
	bool bControllerPossessesPawn = false;
	bool bCharacterMovementUpdateDelegateBound = false;
	bool bActualMovementUpdateFired = false;
	bool bReflectedFunctionPresent = false;
	bool bHandlerIsAlreadyBound = false;
	bool bHookQualifiedBeforeBoundary = false;
	bool bHasValidData = false;
	bool bUpdatedComponentPresent = false;
	bool bUpdatedComponentRegistered = false;
	bool bUpdatedComponentActive = false;
	bool bUpdatedComponentSimulatingPhysics = false;
	bool bUpdatedComponentQueryCollision = false;
	bool bUpdateOnlyIfRendered = false;
	bool bUpdatedComponentRenderedRecently = false;
	bool bCharacterMeshRenderedRecently = false;
	bool bMovingOnGround = false;
	bool bFalling = false;
	bool bCurrentFloorWalkable = false;
	bool bHasMovementBase = false;
	bool bHasAnimRootMotion = false;
	bool bHasRootMotionSources = false;
	bool bCharacterPlayingRootMotion = false;
	bool bMovementInProgress = false;
	bool bScopedMovementUpdates = false;
};

struct Fdemo_mapInputConsumptionEntry
{
	uint64 Sequence = 0;
	Edemo_mapInputConsumptionStage Stage =
		Edemo_mapInputConsumptionStage::ExistingProbeBoundaryEmitted;
	Fdemo_mapInputConsumptionSnapshot Snapshot;
};

/** Heavy facts collected only after the immutable Probe verdict. */
struct Fdemo_mapInputConsumptionEnrichment
{
	FVector ChestBoundsOrigin = FVector::ZeroVector;
	FVector ChestBoundsExtent = FVector::ZeroVector;
	uint64 ChestIdentity = 0;
	uint64 BlockingActorIdentity = 0;
	int32 CapsuleCollisionEnabled = INDEX_NONE;
	int32 CapsuleObjectType = INDEX_NONE;
	int32 ChestCollisionEnabled = INDEX_NONE;
	bool bCapsuleOverlapsChest = false;
	bool bStartPenetrating = false;
	bool bBlockingHit = false;
	bool bProjectOwnedMovementZeroWriter = false;
	bool bCompleted = false;
};

struct Fdemo_mapInputConsumptionClassificationEvidence
{
	bool bComplete = false;
	bool bIntendedNonZero = false;
	bool bAfterAddNonZero = false;
	bool bPreMovementNonZero = false;
	bool bConsumedNonZero = false;
	bool bAccelerationNonZero = false;
	bool bPostMovementDisplaced = false;
	bool bPostPhysicsDisplaced = false;
	bool bCollisionOrPenetration = false;
	bool bProbeAgreesWithPostPhysics = true;
	bool bConcreteResidual = false;
};

/**
 * P8.9 MicroTrace: one in-place observer, fixed storage, and no active-window
 * sweep, bounds query, enumeration, formatting, logging, I/O, allocation,
 * lock, sleep, timer, or product write.
 */
class Fdemo_mapInputConsumptionTrace final
{
public:
	static constexpr int32 Capacity = 96;
	// MicroTrace retains one complete natural-frame chain. Keeping additional
	// frames would add active-window copies without improving first-divergence
	// attribution.
	static constexpr int32 RequiredMovementFrames = 1;

	explicit Fdemo_mapInputConsumptionTrace(
		bool bInEnabled = true,
		bool bInGateEnabled = false);

	bool IsEnabled() const { return State != Edemo_mapInputConsumptionTraceState::Off; }
	bool IsGateEnabled() const { return bGateEnabled; }
	bool IsActive() const { return State == Edemo_mapInputConsumptionTraceState::Active; }
	bool IsVerdictFrozen() const
	{
		return State == Edemo_mapInputConsumptionTraceState::VerdictFrozen
			|| State == Edemo_mapInputConsumptionTraceState::Enriched
			|| State == Edemo_mapInputConsumptionTraceState::Flushed;
	}
	bool IsPostTerminalEnriched() const
	{
		return State == Edemo_mapInputConsumptionTraceState::Enriched
			|| State == Edemo_mapInputConsumptionTraceState::Flushed;
	}
	Edemo_mapInputConsumptionTraceState GetState() const { return State; }

	void BeginBoundary(APawn* Pawn, AActor* FocusedChest);
	void Record(
		Edemo_mapInputConsumptionStage Stage,
		APawn* Pawn,
		APlayerController* Controller = nullptr,
		float ProbeDistanceUU = -1.0f,
		double LatencySeconds = -1.0,
		const FVector& IntendedInput = FVector::ZeroVector);
	void RecordControllerTick(
		APawn* Pawn,
		APlayerController* Controller,
		float DeltaSeconds);
	void RecordMovementUpdated(
		APawn* Pawn,
		APlayerController* Controller,
		float DeltaSeconds,
		const FVector& OldLocation,
		const FVector& OldVelocity);
	void RecordDerivedMovementUpdated(
		UCharacterMovementComponent* Movement,
		APawn* Pawn,
		APlayerController* Controller,
		float DeltaSeconds);
	void FreezeVerdict(
		bool bProbePassed,
		APawn* Pawn,
		APlayerController* Controller,
		float ProbeDistanceUU,
		double LatencySeconds);
	bool HasVerdictFramePostHooks() const;
	bool IsReadyForPostTerminalEnrichment() const;
	void EnrichAfterVerdict();
	void FlushToLog(const TCHAR* Terminal) const;

	int32 Num() const { return Count; }
	int32 GetDroppedCount() const { return DroppedCount; }
	int32 GetMovementFrameCount() const { return MovementFrameCount; }
	uint64 GetVerdictFrame() const { return VerdictFrame; }
	uint64 GetActualCallbackSerial() const { return ActualCallbackSerial; }
	uint64 GetPreBoundaryActualCallbackCount() const
	{
		return PreBoundaryActualCallbackCount;
	}
	float GetLastActualCallbackDeltaSeconds() const
	{
		return LatestMovementDeltaSeconds;
	}
	bool IsHookQualifiedBeforeBoundary() const
	{
		return bReflectedFunctionPresent
			&& bHandlerIsAlreadyBound
			&& PreBoundaryActualCallbackCount >= 1
			&& FMath::IsFinite(
				PreBoundaryLastActualCallbackDeltaSeconds)
			&& PreBoundaryLastActualCallbackDeltaSeconds > 0.0f;
	}
	bool GetFrozenProbeVerdict() const { return bFrozenProbePassed; }
	bool HasCompleteRequiredFrames() const;
	bool HasCompleteTrace() const
	{
		return HasCompleteRequiredFrames() && HasVerdictFramePostHooks();
	}
	const Fdemo_mapInputConsumptionEntry* GetEntry(int32 Index) const;
	const Fdemo_mapInputConsumptionEnrichment& GetEnrichment() const
	{
		return Enrichment;
	}
	const TCHAR* ClassifyRootCause() const;
	const TCHAR* ClassifyInternalGate() const;

	static const TCHAR* ClassifyEvidence(
		const Fdemo_mapInputConsumptionClassificationEvidence& Evidence,
		bool bProbePassed);
	static const TCHAR* StageName(Edemo_mapInputConsumptionStage Stage);

private:
	static constexpr uint8 AfterAddBit = 1 << 0;
	static constexpr uint8 PreMovementBit = 1 << 1;
	static constexpr uint8 PostMovementBit = 1 << 2;
	static constexpr uint8 PostPhysicsBit = 1 << 3;
	static constexpr uint8 CompleteFrameMask =
		AfterAddBit | PreMovementBit | PostMovementBit | PostPhysicsBit;

	int32 FindOrCreateMovementFrame(
		uint64 Frame,
		Edemo_mapInputConsumptionStage Stage);
	Fdemo_mapInputConsumptionSnapshot CaptureRaw(
		APawn* Pawn,
		APlayerController* Controller,
		float ProbeDistanceUU,
		double LatencySeconds,
		const FVector& IntendedInput) const;
	void AppendSnapshot(
		Edemo_mapInputConsumptionStage Stage,
		const Fdemo_mapInputConsumptionSnapshot& Snapshot);
	Fdemo_mapInputConsumptionClassificationEvidence
		BuildClassificationEvidence() const;

	Fdemo_mapInputConsumptionEntry Entries[Capacity] = {};
	uint64 MovementFrames[RequiredMovementFrames] = {};
	uint8 MovementFrameMasks[RequiredMovementFrames] = {};
	Fdemo_mapInputConsumptionSnapshot LatestPostMovementSnapshot = {};
	Fdemo_mapInputConsumptionEnrichment Enrichment = {};
	TWeakObjectPtr<APawn> ObservedPawn;
	TWeakObjectPtr<AActor> ObservedChest;
	FVector BoundaryLocation = FVector::ZeroVector;
	int32 Count = 0;
	int32 DroppedCount = 0;
	int32 MovementFrameCount = 0;
	int32 MovementPath = INDEX_NONE;
	int32 LatestPostMovementEntryIndex = INDEX_NONE;
	uint64 ControllerTickSerial = 0;
	uint64 MovementUpdateSerial = 0;
	uint64 LatestControllerTickFrame = 0;
	uint64 LatestMovementUpdateFrame = 0;
	uint64 ActualCallbackSerial = 0;
	uint64 ActualCallbackFrame = 0;
	uint64 PreBoundaryActualCallbackCount = 0;
	uint64 DerivedSecondarySerial = 0;
	uint64 DerivedSecondaryFrame = 0;
	uint64 VerdictFrame = 0;
	double LatestActualCallbackWorldTime = -1.0;
	float LatestControllerDeltaSeconds = -1.0f;
	float LatestMovementDeltaSeconds = -1.0f;
	float PreBoundaryLastActualCallbackDeltaSeconds = -1.0f;
	float DerivedSecondaryDeltaSeconds = -1.0f;
	FVector LatestMovementOldLocation = FVector::ZeroVector;
	FVector LatestMovementOldVelocity = FVector::ZeroVector;
	FVector LatestActualCallbackActorLocation = FVector::ZeroVector;
	FVector LatestActualCallbackUpdatedComponentLocation =
		FVector::ZeroVector;
	FVector LatestActualCallbackVelocity = FVector::ZeroVector;
	int32 LatestActualCallbackMovementMode = INDEX_NONE;
	int32 LatestActualCallbackCustomMovementMode = INDEX_NONE;
	FVector LastObservedUpdateLocation = FVector::ZeroVector;
	FVector LastObservedUpdateVelocity = FVector::ZeroVector;
	bool bHasObservedUpdateBaseline = false;
	bool bHasLatestPostMovementSnapshot = false;
	bool bFrozenProbePassed = false;
	bool bGateEnabled = false;
	bool bReflectedFunctionPresent = false;
	bool bHandlerIsAlreadyBound = false;
	mutable Edemo_mapInputConsumptionTraceState State =
		Edemo_mapInputConsumptionTraceState::Off;
};

bool Isdemo_mapInputConsumptionTraceRuntimeActive();
void Setdemo_mapInputConsumptionTraceRuntime(
	Fdemo_mapInputConsumptionTrace* Trace);
void Recorddemo_mapInputConsumptionTraceStage(
	Edemo_mapInputConsumptionStage Stage,
	APawn* Pawn,
	APlayerController* Controller = nullptr,
	float ProbeDistanceUU = -1.0f,
	double LatencySeconds = -1.0,
	const FVector& IntendedInput = FVector::ZeroVector);
void Recorddemo_mapInputConsumptionControllerTick(
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds);
void Recorddemo_mapInputConsumptionMovementUpdated(
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds,
	const FVector& OldLocation,
	const FVector& OldVelocity);
void Recorddemo_mapInputConsumptionDerivedMovementUpdated(
	UCharacterMovementComponent* Movement,
	APawn* Pawn,
	APlayerController* Controller,
	float DeltaSeconds);

#endif
