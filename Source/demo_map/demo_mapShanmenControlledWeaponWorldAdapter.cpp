#include "demo_mapShanmenControlledWeaponWorldAdapter.h"

#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "demo_mapCombatVitalityHost.h"

namespace
{
	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}
}

Fdemo_mapShanmenControlledWeaponOrbitThreatResult
Fdemo_mapShanmenControlledWeaponWorldAdapter::ProjectOrbitThreatOverlap(
	Fdemo_mapShanmenControlledWeaponSession& Session,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenWorldHitContext& Context,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal)
{
	Fdemo_mapShanmenControlledWeaponOrbitThreatResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!Session.IsActive())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponOrbitThreatError::SessionNotActive;
		return Result;
	}
	if (!ContextMatchesSession(
			Session, Context, EShanmenControlledWeaponState::Orbiting))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponOrbitThreatError::ContextMismatch;
		return Result;
	}

	FShanmenHitCandidate Candidate;
	if (!FShanmenWorldHitAdapter::TryFromOverlap(
			Context,
			Overlap,
			ContactLocation,
			ContactNormal,
			Coordinator.GetEntityRegistry(),
			Candidate))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponOrbitThreatError::ContactNotResolved;
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponSession SessionCandidate = Session;
	if (!SessionCandidate.TryAcceptOrbitThreatCandidate(Candidate))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponOrbitThreatError::CandidateRejected;
		return Result;
	}

	Result.Context = Context;
	Result.Candidate = Candidate;
	Result.Error = Edemo_mapShanmenControlledWeaponOrbitThreatError::None;
	if (!Result.IsProjected())
	{
		return Fdemo_mapShanmenControlledWeaponOrbitThreatResult();
	}
	Session = MoveTemp(SessionCandidate);
	return Result;
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveSweepContact(
	Fdemo_mapShanmenControlledWeaponSession& Session,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenWorldHitContext& Context,
	const FHitResult& Hit)
{
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!Session.IsActive())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::SessionNotActive;
		return Result;
	}
	if (!ContextMatchesSession(
			Session, Context, EShanmenControlledWeaponState::Directed))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::ContextMismatch;
		return Result;
	}

	FShanmenHitCandidate Candidate;
	if (!FShanmenWorldHitAdapter::TryFromSweep(
			Context,
			Hit,
			Coordinator.GetEntityRegistry(),
			Candidate))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::ContactNotResolved;
		return Result;
	}
	return ResolveCandidateAndDeliver(
		Session, Coordinator, Candidate, Hit.GetActor());
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveOverlapContact(
	Fdemo_mapShanmenControlledWeaponSession& Session,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenWorldHitContext& Context,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal)
{
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!Session.IsActive())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::SessionNotActive;
		return Result;
	}
	if (!ContextMatchesSession(
			Session, Context, EShanmenControlledWeaponState::Directed))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::ContextMismatch;
		return Result;
	}

	FShanmenHitCandidate Candidate;
	if (!FShanmenWorldHitAdapter::TryFromOverlap(
			Context,
			Overlap,
			ContactLocation,
			ContactNormal,
			Coordinator.GetEntityRegistry(),
			Candidate))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::ContactNotResolved;
		return Result;
	}
	return ResolveCandidateAndDeliver(
		Session, Coordinator, Candidate, Overlap.GetActor());
}

bool Fdemo_mapShanmenControlledWeaponWorldAdapter::ContextMatchesSession(
	const Fdemo_mapShanmenControlledWeaponSession& Session,
	const FShanmenWorldHitContext& Context,
	EShanmenControlledWeaponState ExpectedState)
{
	return Session.IsActive()
		&& Session.GetExecution().IsEmissionActive()
		&& Session.GetExecution().GetState() == ExpectedState
		&& Context.IsValid()
		&& Context.GetDetectorKind()
			== EShanmenHitDetectorKind::ControlledObject
		&& Context.GetDetectorId()
			== Session.GetExecution().GetDefinition().GetDetectorId()
		&& ActionsMatch(
			Context.GetAction(), Session.GetActionRuntime().GetAction());
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveCandidateAndDeliver(
	Fdemo_mapShanmenControlledWeaponSession& Session,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenHitCandidate& Candidate,
	AActor* TargetActor)
{
	Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result;
	Result.TargetEntityId = Candidate.TargetEntityId;
	Idemo_mapCombatVitalityHost* VitalityHost = TargetActor
		? Cast<Idemo_mapCombatVitalityHost>(TargetActor)
		: nullptr;
	FShanmenTargetVitalitySnapshot Vitality;
	if (!VitalityHost
		|| !VitalityHost->IsCombatEntityBound()
		|| VitalityHost->GetCombatEntityId() != Candidate.TargetEntityId
		|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Vitality))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::TargetVitalityUnavailable;
		return Result;
	}

	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	Fdemo_mapShanmenControlledWeaponSession SessionCandidate = Session;
	if (!SessionCandidate.TryResolveCandidate(
			Candidate, Vitality, Defense, Result.Impact))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::CandidateRejected;
		return Result;
	}

	Result.Delivery = Coordinator.DeliverControlledWeaponImpactToM01Enemy(
		Result.Impact, TargetActor);
	if (!Result.Delivery.IsSuccess())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponWorldDeliveryError::DeliveryRejected;
		return Result;
	}

	Session = MoveTemp(SessionCandidate);
	Result.Error = Edemo_mapShanmenControlledWeaponWorldDeliveryError::None;
	return Result;
}
