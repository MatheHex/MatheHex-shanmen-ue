#include "demo_mapShanmenControlledWeaponProductController.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"

namespace
{
	constexpr float OrbitTwoPi = 2.0f * UE_PI;

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	float WrapOrbitPhase(float PhaseRadians)
	{
		float Wrapped = FMath::Fmod(PhaseRadians, OrbitTwoPi);
		if (Wrapped < 0.0f)
		{
			Wrapped += OrbitTwoPi;
		}
		return FMath::IsNearlyEqual(Wrapped, OrbitTwoPi)
			? 0.0f
			: Wrapped;
	}

	bool IsCanonicalOrbitPhase(float PhaseRadians)
	{
		return FMath::IsFinite(PhaseRadians)
			&& PhaseRadians >= 0.0f
			&& PhaseRadians < OrbitTwoPi;
	}

	FVector MakeOrbitLocation(
		const FVector& Center,
		const FVector& ReferenceAxis,
		const FVector& PlaneNormal,
		float Radius,
		float PhaseRadians)
	{
		const FVector Tangent =
			FVector::CrossProduct(PlaneNormal, ReferenceAxis);
		return Center + Radius * (
			ReferenceAxis * FMath::Cos(PhaseRadians)
			+ Tangent * FMath::Sin(PhaseRadians));
	}
}

bool Fdemo_mapShanmenControlledWeaponMotionCapture::IsValid() const
{
	return FMath::IsFinite(DirectedSpeed)
		&& DirectedSpeed > 0.0f
		&& IsFiniteVector(OrbitCenterOffset)
		&& IsFiniteVector(OrbitPlaneNormal)
		&& OrbitPlaneNormal.IsNormalized()
		&& IsFiniteVector(OrbitReferenceAxis)
		&& OrbitReferenceAxis.IsNormalized()
		&& FMath::IsNearlyZero(FVector::DotProduct(
			OrbitPlaneNormal, OrbitReferenceAxis))
		&& FVector::CrossProduct(
			OrbitPlaneNormal, OrbitReferenceAxis).IsNormalized()
		&& FMath::IsFinite(OrbitRadius)
		&& OrbitRadius > 0.0f
		&& FMath::IsFinite(OrbitAngularSpeedRadiansPerSecond)
		&& !FMath::IsNearlyZero(OrbitAngularSpeedRadiansPerSecond)
		&& FMath::IsFinite(InitialOrbitPhaseRadians)
		&& FMath::IsFinite(MaximumStepSeconds)
		&& MaximumStepSeconds > 0.0f;
}

bool Fdemo_mapShanmenControlledWeaponMovementReceipt::IsValid() const
{
	return ActivationId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& CommandSequence >= 0
		&& IsFiniteVector(Direction)
		&& Direction.IsNormalized()
		&& IsFiniteVector(StartLocation)
		&& IsFiniteVector(RequestedEndLocation)
		&& IsFiniteVector(ActualEndLocation)
		&& FMath::IsFinite(DeltaSeconds)
		&& DeltaSeconds > 0.0f
		&& (bMoved || bBlockingHit);
}

bool Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt::IsValid() const
{
	if (!ActivationId.IsValid()
		|| !SourceItemInstanceId.IsValid()
		|| !IsFiniteVector(Center)
		|| !IsFiniteVector(PlaneNormal)
		|| !PlaneNormal.IsNormalized()
		|| !IsFiniteVector(ReferenceAxis)
		|| !ReferenceAxis.IsNormalized()
		|| !FMath::IsNearlyZero(FVector::DotProduct(
			PlaneNormal, ReferenceAxis))
		|| !FMath::IsFinite(Radius)
		|| Radius <= 0.0f
		|| !FMath::IsFinite(AngularSpeedRadiansPerSecond)
		|| FMath::IsNearlyZero(AngularSpeedRadiansPerSecond)
		|| !IsCanonicalOrbitPhase(StartPhaseRadians)
		|| !IsCanonicalOrbitPhase(EndPhaseRadians)
		|| !IsFiniteVector(StartLocation)
		|| !IsFiniteVector(RequestedEndLocation)
		|| !IsFiniteVector(ActualEndLocation)
		|| !FMath::IsFinite(DeltaSeconds)
		|| DeltaSeconds <= 0.0f
		|| !bPlaced)
	{
		return false;
	}

	const float ExpectedEndPhase = WrapOrbitPhase(
		StartPhaseRadians
			+ AngularSpeedRadiansPerSecond * DeltaSeconds);
	const FVector ExpectedEndLocation = MakeOrbitLocation(
		Center,
		ReferenceAxis,
		PlaneNormal,
		Radius,
		ExpectedEndPhase);
	return FMath::IsNearlyEqual(
			EndPhaseRadians, ExpectedEndPhase, KINDA_SMALL_NUMBER)
		&& RequestedEndLocation.Equals(
			ExpectedEndLocation, KINDA_SMALL_NUMBER)
		&& ActualEndLocation.Equals(
			RequestedEndLocation, KINDA_SMALL_NUMBER)
		&& bMoved == !ActualEndLocation.Equals(
			StartLocation, KINDA_SMALL_NUMBER);
}

Fdemo_mapShanmenControlledWeaponProductStartResult
Fdemo_mapShanmenControlledWeaponProductController::TryStart(
	const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* RequestedSourceActor,
	AActor* RequestedWeaponActor,
	UPrimitiveComponent* RequestedWeaponCollisionRoot,
	const Fdemo_mapShanmenControlledWeaponMotionCapture& RequestedMotion,
	Fdemo_mapShanmenControlledWeaponProductController& OutController)
{
	OutController.Reset();
	Fdemo_mapShanmenControlledWeaponProductStartResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!Prepared.IsPrepared()
		|| Prepared.Action.GetRunId() != Coordinator.GetRunId())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponProductStartError::PreparedInvalid;
		return Result;
	}
	if (!RequestedMotion.IsValid())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponProductStartError::MotionInvalid;
		return Result;
	}
	if (!RequestedSourceActor
		|| !RequestedWeaponActor
		|| RequestedSourceActor == RequestedWeaponActor
		|| !RequestedWeaponCollisionRoot
		|| RequestedWeaponCollisionRoot->GetOwner() != RequestedWeaponActor
		|| RequestedWeaponActor->GetRootComponent()
			!= RequestedWeaponCollisionRoot)
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponProductStartError::ActorBindingInvalid;
		return Result;
	}

	FGuid ResolvedSourceEntityId;
	if (!Coordinator.GetEntityRegistry().TryResolveObject(
			Coordinator.GetRunId(),
			RequestedSourceActor,
			INDEX_NONE,
			ResolvedSourceEntityId))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponProductStartError::SourceNotRegistered;
		return Result;
	}
	if (ResolvedSourceEntityId != Coordinator.GetPlayerEntityId()
		|| Prepared.Action.GetSourceEntityId() != ResolvedSourceEntityId)
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponProductStartError::SourceMismatch;
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate;
	Candidate.RunId = Coordinator.GetRunId();
	Candidate.SourceEntityId = ResolvedSourceEntityId;
	Candidate.SourceActor = RequestedSourceActor;
	Candidate.WeaponActor = RequestedWeaponActor;
	Candidate.WeaponCollisionRoot = RequestedWeaponCollisionRoot;
	Candidate.Motion = RequestedMotion;
	Candidate.Motion.InitialOrbitPhaseRadians =
		WrapOrbitPhase(RequestedMotion.InitialOrbitPhaseRadians);
	Candidate.CurrentOrbitPhaseRadians =
		Candidate.Motion.InitialOrbitPhaseRadians;
	if (!Fdemo_mapShanmenControlledWeaponSession::TryStart(
			Prepared,
			Candidate.Session,
			Result.Startup,
			Result.Active)
		|| !Candidate.IsValid())
	{
		Result = Fdemo_mapShanmenControlledWeaponProductStartResult();
		Result.Error =
			Edemo_mapShanmenControlledWeaponProductStartError::SessionStartRejected;
		return Result;
	}

	OutController = MoveTemp(Candidate);
	Result.Error =
		Edemo_mapShanmenControlledWeaponProductStartError::None;
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponProductController::IsValid() const
{
	AActor* BoundSourceActor = SourceActor.Get();
	AActor* BoundWeaponActor = WeaponActor.Get();
	UPrimitiveComponent* BoundCollisionRoot = WeaponCollisionRoot.Get();
	if (!RunId.IsValid()
		|| !SourceEntityId.IsValid()
		|| !BoundSourceActor
		|| !BoundWeaponActor
		|| BoundSourceActor == BoundWeaponActor
		|| !BoundCollisionRoot
		|| BoundCollisionRoot->GetOwner() != BoundWeaponActor
		|| BoundWeaponActor->GetRootComponent() != BoundCollisionRoot
		|| !Motion.IsValid()
		|| !IsCanonicalOrbitPhase(Motion.InitialOrbitPhaseRadians)
		|| !IsCanonicalOrbitPhase(CurrentOrbitPhaseRadians)
		|| !Session.IsValid()
		|| Session.GetActionRuntime().GetAction().GetRunId() != RunId
		|| Session.GetActionRuntime().GetAction().GetSourceEntityId()
			!= SourceEntityId)
	{
		return false;
	}

	return Session.GetExecution().IsEmissionActive()
		? ContactContextMatchesSession()
		: !ActiveContactContext.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponProductController::IsActive() const
{
	return IsValid() && Session.IsActive();
}

bool Fdemo_mapShanmenControlledWeaponProductController::IsOrbiting() const
{
	return IsActive()
		&& Session.GetExecution().GetState()
			== EShanmenControlledWeaponState::Orbiting;
}

bool Fdemo_mapShanmenControlledWeaponProductController::IsDirected() const
{
	return IsActive()
		&& Session.GetExecution().GetState()
			== EShanmenControlledWeaponState::Directed;
}

bool Fdemo_mapShanmenControlledWeaponProductController::HasActiveContactWindow() const
{
	return IsValid()
		&& Session.GetExecution().IsEmissionActive()
		&& ActiveContactContext.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponProductController::
HasActiveOrbitThreatWindow() const
{
	return HasActiveContactWindow() && IsOrbiting();
}

bool Fdemo_mapShanmenControlledWeaponProductController::
HasActiveDirectedContactWindow() const
{
	return HasActiveContactWindow() && IsDirected();
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryLaunch(
	int64 ExpectedSequence,
	const FVector& DesiredDirection,
	FShanmenControlledWeaponCommandReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponCommandReceipt();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryIssueControl(
			ExpectedSequence,
			EShanmenControlledWeaponCommandKind::Launch,
			DesiredDirection,
			OutReceipt)
		|| !Candidate.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponCommandReceipt();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryRedirect(
	int64 ExpectedSequence,
	const FVector& DesiredDirection,
	FShanmenControlledWeaponCommandReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponCommandReceipt();
	if (!IsDirected())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryIssueControl(
			ExpectedSequence,
			EShanmenControlledWeaponCommandKind::Redirect,
			DesiredDirection,
			OutReceipt)
		|| !Candidate.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponCommandReceipt();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryAdvanceOrbiting(
	float DeltaSeconds,
	Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt& OutReceipt)
{
	OutReceipt =
		Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt();
	if (!IsOrbiting()
		|| HasActiveContactWindow()
		|| !FMath::IsFinite(DeltaSeconds)
		|| DeltaSeconds <= 0.0f
		|| DeltaSeconds > Motion.MaximumStepSeconds)
	{
		return false;
	}

	AActor* BoundSourceActor = SourceActor.Get();
	AActor* BoundWeaponActor = WeaponActor.Get();
	const FVector Center =
		BoundSourceActor->GetActorLocation() + Motion.OrbitCenterOffset;
	const FVector StartLocation = BoundWeaponActor->GetActorLocation();
	const float EndPhase = WrapOrbitPhase(
		CurrentOrbitPhaseRadians
			+ Motion.OrbitAngularSpeedRadiansPerSecond * DeltaSeconds);
	const FVector RequestedEndLocation = MakeOrbitLocation(
		Center,
		Motion.OrbitReferenceAxis,
		Motion.OrbitPlaneNormal,
		Motion.OrbitRadius,
		EndPhase);
	if (!IsFiniteVector(Center)
		|| !IsFiniteVector(StartLocation)
		|| !IsFiniteVector(RequestedEndLocation))
	{
		return false;
	}
	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	Candidate.CurrentOrbitPhaseRadians = EndPhase;
	if (!Candidate.IsValid())
	{
		return false;
	}

	const bool bPlaced = BoundWeaponActor->SetActorLocation(
		RequestedEndLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	const FVector ActualEndLocation =
		BoundWeaponActor->GetActorLocation();
	OutReceipt.ActivationId =
		Session.GetActionRuntime().GetAction().GetActivationId();
	OutReceipt.SourceItemInstanceId =
		Session.GetEvidence().ItemInstanceId;
	OutReceipt.Center = Center;
	OutReceipt.PlaneNormal = Motion.OrbitPlaneNormal;
	OutReceipt.ReferenceAxis = Motion.OrbitReferenceAxis;
	OutReceipt.Radius = Motion.OrbitRadius;
	OutReceipt.AngularSpeedRadiansPerSecond =
		Motion.OrbitAngularSpeedRadiansPerSecond;
	OutReceipt.StartPhaseRadians = CurrentOrbitPhaseRadians;
	OutReceipt.EndPhaseRadians = EndPhase;
	OutReceipt.StartLocation = StartLocation;
	OutReceipt.RequestedEndLocation = RequestedEndLocation;
	OutReceipt.ActualEndLocation = ActualEndLocation;
	OutReceipt.DeltaSeconds = DeltaSeconds;
	OutReceipt.bPlaced = bPlaced;
	OutReceipt.bMoved = !ActualEndLocation.Equals(
		StartLocation, KINDA_SMALL_NUMBER);
	if (!OutReceipt.IsValid())
	{
		OutReceipt =
			Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt();
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::
TryBeginOrbitThreatWindow(FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	if (!IsOrbiting() || HasActiveContactWindow())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryBeginOrbitThreatWindow(
			Candidate.ActiveContactContext)
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutContext = Candidate.ActiveContactContext;
	*this = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenControlledWeaponOrbitThreatResult
Fdemo_mapShanmenControlledWeaponProductController::ProjectOrbitThreatOverlap(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal)
{
	if (!HasActiveOrbitThreatWindow() || !CoordinatorMatches(Coordinator))
	{
		return Fdemo_mapShanmenControlledWeaponOrbitThreatResult();
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	Fdemo_mapShanmenControlledWeaponOrbitThreatResult Result =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ProjectOrbitThreatOverlap(
			Candidate.Session,
			Coordinator,
			Candidate.ActiveContactContext,
			Overlap,
			ContactLocation,
			ContactNormal);
	if (Result.IsProjected())
	{
		*this = MoveTemp(Candidate);
	}
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponProductController::
TryEndOrbitThreatWindow(FShanmenDetectorEmissionReceipt& OutReceipt)
{
	OutReceipt = FShanmenDetectorEmissionReceipt();
	if (!HasActiveOrbitThreatWindow())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	FShanmenDetectorEmissionReceipt Receipt;
	if (!Candidate.Session.TryEndOrbitThreatWindow(Receipt)
		|| !Receipt.IsValid())
	{
		return false;
	}
	Candidate.ActiveContactContext = FShanmenWorldHitContext();
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Receipt);
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::
TryEndOrbitThreatWindow()
{
	FShanmenDetectorEmissionReceipt Ignored;
	return TryEndOrbitThreatWindow(Ignored);
}

bool Fdemo_mapShanmenControlledWeaponProductController::
TryEvaluateOrbitThreatReceipt(
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
	FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
	return IsOrbiting()
		&& !HasActiveContactWindow()
		&& Session.TryEvaluateOrbitThreatReceipt(
			Emission, TargetEvidence, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponProductController::
TryEvaluateOrbitThreatActors(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<AActor*>& TargetActors,
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult& OutEvidence,
	FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const
{
	OutEvidence =
		Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult();
	OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
	if (!IsOrbiting()
		|| HasActiveContactWindow()
		|| !CoordinatorMatches(Coordinator))
	{
		return false;
	}

	OutEvidence = Fdemo_mapShanmenControlledWeaponWorldAdapter::
		CaptureOrbitThreatTargetEvidence(
			Coordinator, Emission, TargetActors);
	return OutEvidence.IsCaptured()
		&& TryEvaluateOrbitThreatReceipt(
			Emission, OutEvidence.TargetEvidence, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponProductController::
TryBuildOrbitThreatPresenceIntents(
	const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
	FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPresenceReceipt();
	return IsOrbiting()
		&& !HasActiveContactWindow()
		&& Session.TryBuildOrbitThreatPresenceIntents(
			Policy, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryAdvanceDirected(
	float DeltaSeconds,
	Fdemo_mapShanmenControlledWeaponMovementReceipt& OutReceipt,
	FHitResult& OutBlockingHit)
{
	OutReceipt = Fdemo_mapShanmenControlledWeaponMovementReceipt();
	OutBlockingHit = FHitResult();
	if (!IsDirected()
		|| !FMath::IsFinite(DeltaSeconds)
		|| DeltaSeconds <= 0.0f
		|| DeltaSeconds > Motion.MaximumStepSeconds)
	{
		return false;
	}

	AActor* BoundWeaponActor = WeaponActor.Get();
	const FVector Direction =
		Session.GetExecution().GetCurrentDirection();
	const FVector StartLocation = BoundWeaponActor->GetActorLocation();
	const FVector RequestedEndLocation =
		StartLocation + Direction * Motion.DirectedSpeed * DeltaSeconds;
	if (!IsFiniteVector(Direction)
		|| !Direction.IsNormalized()
		|| !IsFiniteVector(StartLocation)
		|| !IsFiniteVector(RequestedEndLocation))
	{
		return false;
	}

	const bool bMoved = BoundWeaponActor->SetActorLocation(
		RequestedEndLocation,
		true,
		&OutBlockingHit,
		ETeleportType::None);
	const FVector ActualEndLocation = BoundWeaponActor->GetActorLocation();
	OutReceipt.ActivationId =
		Session.GetActionRuntime().GetAction().GetActivationId();
	OutReceipt.SourceItemInstanceId =
		Session.GetEvidence().ItemInstanceId;
	OutReceipt.CommandSequence =
		Session.GetExecution().GetNextCommandSequence() - 1;
	OutReceipt.Direction = Direction;
	OutReceipt.StartLocation = StartLocation;
	OutReceipt.RequestedEndLocation = RequestedEndLocation;
	OutReceipt.ActualEndLocation = ActualEndLocation;
	OutReceipt.DeltaSeconds = DeltaSeconds;
	OutReceipt.bMoved = bMoved
		|| !ActualEndLocation.Equals(StartLocation, KINDA_SMALL_NUMBER);
	OutReceipt.bBlockingHit = OutBlockingHit.bBlockingHit;
	if (!OutReceipt.IsValid())
	{
		OutReceipt = Fdemo_mapShanmenControlledWeaponMovementReceipt();
		OutBlockingHit = FHitResult();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryBeginContactWindow(
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	if (!IsDirected() || HasActiveContactWindow())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryBeginContactWindow(
			Candidate.ActiveContactContext)
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutContext = Candidate.ActiveContactContext;
	*this = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponProductController::ResolveSweepContact(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FHitResult& Hit)
{
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result;
	if (!HasActiveDirectedContactWindow() || !CoordinatorMatches(Coordinator))
	{
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	Result = Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveSweepContact(
		Candidate.Session,
		Coordinator,
		Candidate.ActiveContactContext,
		Hit);
	if (Result.IsDelivered())
	{
		*this = MoveTemp(Candidate);
	}
	return Result;
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponProductController::ResolveOverlapContact(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal)
{
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result;
	if (!HasActiveDirectedContactWindow() || !CoordinatorMatches(Coordinator))
	{
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	Result = Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveOverlapContact(
		Candidate.Session,
		Coordinator,
		Candidate.ActiveContactContext,
		Overlap,
		ContactLocation,
		ContactNormal);
	if (Result.IsDelivered())
	{
		*this = MoveTemp(Candidate);
	}
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryEndContactWindow()
{
	if (!HasActiveDirectedContactWindow())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryEndContactWindow())
	{
		return false;
	}
	Candidate.ActiveContactContext = FShanmenWorldHitContext();
	if (!Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryRecallAndComplete(
	int64 ExpectedSequence,
	FShanmenControlledWeaponCommandReceipt& OutRecall,
	FShanmenActionTransitionReceipt& OutRecovery,
	FShanmenActionTransitionReceipt& OutCompleted)
{
	OutRecall = FShanmenControlledWeaponCommandReceipt();
	OutRecovery = FShanmenActionTransitionReceipt();
	OutCompleted = FShanmenActionTransitionReceipt();
	if (!IsActive() || HasActiveContactWindow())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryRecallAndComplete(
			ExpectedSequence,
			OutRecall,
			OutRecovery,
			OutCompleted)
		|| !Candidate.IsValid())
	{
		OutRecall = FShanmenControlledWeaponCommandReceipt();
		OutRecovery = FShanmenActionTransitionReceipt();
		OutCompleted = FShanmenActionTransitionReceipt();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponProductController::TryInterrupt(
	FShanmenActionTransitionReceipt& OutInterrupted)
{
	OutInterrupted = FShanmenActionTransitionReceipt();
	if (!IsActive())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Candidate = *this;
	if (!Candidate.Session.TryInterrupt(OutInterrupted))
	{
		OutInterrupted = FShanmenActionTransitionReceipt();
		return false;
	}
	Candidate.ActiveContactContext = FShanmenWorldHitContext();
	if (!Candidate.IsValid())
	{
		OutInterrupted = FShanmenActionTransitionReceipt();
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

void Fdemo_mapShanmenControlledWeaponProductController::Reset()
{
	*this = Fdemo_mapShanmenControlledWeaponProductController();
}

bool Fdemo_mapShanmenControlledWeaponProductController::CoordinatorMatches(
	const Fdemo_mapCombatRunCoordinator& Coordinator) const
{
	if (!IsActive()
		|| !Coordinator.IsReady()
		|| Coordinator.GetRunId() != RunId
		|| Coordinator.GetPlayerEntityId() != SourceEntityId)
	{
		return false;
	}

	FGuid ResolvedSourceEntityId;
	return Coordinator.GetEntityRegistry().TryResolveObject(
			RunId,
			SourceActor.Get(),
			INDEX_NONE,
			ResolvedSourceEntityId)
		&& ResolvedSourceEntityId == SourceEntityId;
}

bool Fdemo_mapShanmenControlledWeaponProductController::
ContactContextMatchesSession() const
{
	const FShanmenCombatActionSnapshot& Action =
		Session.GetActionRuntime().GetAction();
	return ActiveContactContext.IsValid()
		&& ActiveContactContext.GetDetectorKind()
			== EShanmenHitDetectorKind::ControlledObject
		&& ActiveContactContext.GetDetectorId()
			== Session.GetExecution().GetDefinition().GetDetectorId()
		&& ActiveContactContext.GetAction().GetRunId() == Action.GetRunId()
		&& ActiveContactContext.GetAction().GetActivationId()
			== Action.GetActivationId()
		&& ActiveContactContext.GetAction().GetSourceEntityId()
			== Action.GetSourceEntityId()
		&& ActiveContactContext.GetAction().GetSourceItemInstanceId()
			== Action.GetSourceItemInstanceId();
}
