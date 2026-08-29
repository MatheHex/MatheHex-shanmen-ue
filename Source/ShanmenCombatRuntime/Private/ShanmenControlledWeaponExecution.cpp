#include "ShanmenControlledWeaponExecution.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool TryCanonicalDirection(
		EShanmenControlledWeaponCommandKind Kind,
		const FVector& DesiredDirection,
		FVector& OutDirection)
	{
		OutDirection = FVector::ZeroVector;
		if (DesiredDirection.ContainsNaN())
		{
			return false;
		}

		if (Kind == EShanmenControlledWeaponCommandKind::Recall)
		{
			return DesiredDirection.IsNearlyZero();
		}

		if (DesiredDirection.IsNearlyZero())
		{
			return false;
		}
		OutDirection = DesiredDirection.GetSafeNormal();
		if (OutDirection.IsNearlyZero() || OutDirection.ContainsNaN())
		{
			OutDirection = FVector::ZeroVector;
			return false;
		}

		if (OutDirection.X == 0.0) OutDirection.X = 0.0;
		if (OutDirection.Y == 0.0) OutDirection.Y = 0.0;
		if (OutDirection.Z == 0.0) OutDirection.Z = 0.0;
		return FMath::IsNearlyEqual(OutDirection.SizeSquared(), 1.0);
	}

	FGuid MakeCommandId(
		const FShanmenCombatActionSnapshot& Action,
		int64 Sequence,
		EShanmenControlledWeaponCommandKind Kind,
		const FVector& Direction)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ControlledWeapon.Command.r1"),
			{
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceItemInstanceId()),
				FString::Printf(TEXT("%lld"), Sequence),
				FString::FromInt(static_cast<int32>(Kind)),
				DoubleBits(Direction.X),
				DoubleBits(Direction.Y),
				DoubleBits(Direction.Z)
			});
	}

	FGuid MakeThreatPresenceIntentId(
		const FGuid& RunId,
		const FGuid& SourceItemInstanceId,
		const FShanmenHitCandidate& Candidate)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ControlledWeapon.ThreatPresence.r1"),
			{
				GuidDigits(RunId),
				GuidDigits(Candidate.ActivationId),
				GuidDigits(Candidate.SourceEntityId),
				GuidDigits(SourceItemInstanceId),
				Candidate.DetectorId.ToString(),
				FString::FromInt(
					static_cast<int32>(Candidate.DetectorKind)),
				FString::FromInt(Candidate.HitOrdinal),
				GuidDigits(Candidate.TargetEntityId)
			});
	}

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

	bool CandidatesMatch(
		const FShanmenHitCandidate& Left,
		const FShanmenHitCandidate& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.ActivationId == Right.ActivationId
			&& Left.SourceEntityId == Right.SourceEntityId
			&& Left.TargetEntityId == Right.TargetEntityId
			&& Left.DetectorId == Right.DetectorId
			&& Left.DetectorKind == Right.DetectorKind
			&& Left.HitOrdinal == Right.HitOrdinal
			&& Left.HitLocation.Equals(Right.HitLocation)
			&& Left.HitNormal.Equals(Right.HitNormal);
	}

	EShanmenControlledWeaponThreatTargetDecision EvaluateThreatTarget(
		const FGuid& SourceEntityId,
		bool bRejectSelf,
		const FGameplayTagContainer& RequiredTargetTags,
		const FGuid& TargetEntityId,
		const FGameplayTagContainer& TargetTags)
	{
		if (bRejectSelf && TargetEntityId == SourceEntityId)
		{
			return EShanmenControlledWeaponThreatTargetDecision::RejectedSelf;
		}
		return TargetTags.HasAll(RequiredTargetTags)
			? EShanmenControlledWeaponThreatTargetDecision::Accepted
			: EShanmenControlledWeaponThreatTargetDecision::RejectedMissingRequiredTags;
	}

}

FName FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.ControlledWeapon.FlyingSword01");
}

bool FShanmenControlledWeaponDefinition::TryCapture(
	const FShanmenControlledWeaponDefinitionCapture& Capture,
	FShanmenControlledWeaponDefinition& OutDefinition)
{
	OutDefinition = FShanmenControlledWeaponDefinition();
	OutDefinition.ActionDefinitionId = Capture.ActionDefinitionId;
	OutDefinition.DetectorId = Capture.DetectorId;
	OutDefinition.FormulaId = Capture.FormulaId;
	OutDefinition.BaseDamage = Capture.BaseDamage;
	OutDefinition.ControlPowerCoefficient = Capture.ControlPowerCoefficient;
	OutDefinition.DamageTags = Capture.DamageTags;
	OutDefinition.RequiredTargetTags = Capture.RequiredTargetTags;
	OutDefinition.bRejectSelf = Capture.bRejectSelf;
	if (!OutDefinition.IsValid())
	{
		OutDefinition = FShanmenControlledWeaponDefinition();
		return false;
	}
	return true;
}

bool FShanmenControlledWeaponDefinition::IsValid() const
{
	return ActionDefinitionId == CanonicalActionDefinitionId()
		&& !DetectorId.IsNone()
		&& !FormulaId.IsNone()
		&& FMath::IsFinite(BaseDamage)
		&& BaseDamage >= 0.0f
		&& FMath::IsFinite(ControlPowerCoefficient)
		&& ControlPowerCoefficient >= 0.0f
		&& DamageTags.HasTag(FShanmenCombatNativeTags::DamagePhysicalSlash())
		&& RequiredTargetTags.HasTag(FShanmenCombatNativeTags::TargetLiving())
		&& bRejectSelf;
}

bool FShanmenControlledWeaponOffenseSnapshot::TryCapture(
	float ControlPower,
	FShanmenControlledWeaponOffenseSnapshot& OutSnapshot)
{
	OutSnapshot = FShanmenControlledWeaponOffenseSnapshot();
	if (!FMath::IsFinite(ControlPower) || ControlPower < 0.0f)
	{
		return false;
	}
	OutSnapshot.ControlPower = ControlPower;
	OutSnapshot.bCaptured = true;
	return true;
}

bool FShanmenControlledWeaponOffenseSnapshot::IsValid() const
{
	return bCaptured
		&& FMath::IsFinite(ControlPower)
		&& ControlPower >= 0.0f;
}

bool FShanmenControlledWeaponCommandReceipt::IsValid() const
{
	if (!CommandId.IsValid()
		|| !ActivationId.IsValid()
		|| !SourceItemInstanceId.IsValid()
		|| Sequence < 0
		|| DirectionAfter.ContainsNaN())
	{
		return false;
	}

	switch (Kind)
	{
	case EShanmenControlledWeaponCommandKind::Launch:
		return StateBefore == EShanmenControlledWeaponState::Orbiting
			&& StateAfter == EShanmenControlledWeaponState::Directed
			&& FMath::IsNearlyEqual(DirectionAfter.SizeSquared(), 1.0);
	case EShanmenControlledWeaponCommandKind::Redirect:
		return StateBefore == EShanmenControlledWeaponState::Directed
			&& StateAfter == EShanmenControlledWeaponState::Directed
			&& FMath::IsNearlyEqual(DirectionAfter.SizeSquared(), 1.0);
	case EShanmenControlledWeaponCommandKind::Recall:
		return StateBefore == EShanmenControlledWeaponState::Directed
			&& StateAfter == EShanmenControlledWeaponState::Recalled
			&& DirectionAfter.IsNearlyZero();
	}
	return false;
}

bool FShanmenControlledWeaponImpactReceipt::IsValid() const
{
	return Request.IsValid()
		&& Request.Action.GetSourceItemInstanceId().IsValid()
		&& Request.Candidate.DetectorKind
			== EShanmenHitDetectorKind::ControlledObject
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool FShanmenControlledWeaponThreatTargetEvidence::TryCapture(
	const FGuid& TargetEntityId,
	const FGameplayTagContainer& TargetTags,
	FShanmenControlledWeaponThreatTargetEvidence& OutEvidence)
{
	OutEvidence = FShanmenControlledWeaponThreatTargetEvidence();
	if (!TargetEntityId.IsValid())
	{
		return false;
	}
	OutEvidence.TargetEntityId = TargetEntityId;
	OutEvidence.TargetTags = TargetTags;
	return true;
}

bool FShanmenControlledWeaponThreatTargetReceipt::IsValid() const
{
	if (!Candidate.IsValid())
	{
		return false;
	}
	return Decision
		== EShanmenControlledWeaponThreatTargetDecision::Accepted
		|| Decision
			== EShanmenControlledWeaponThreatTargetDecision::RejectedSelf
		|| Decision
			== EShanmenControlledWeaponThreatTargetDecision::RejectedMissingRequiredTags;
}

bool FShanmenControlledWeaponThreatPolicyReceipt::IsValid() const
{
	if (!Emission.IsValid()
		|| RequiredTargetTags.IsEmpty()
		|| !bRejectSelf
		|| Targets.Num() != Emission.GetCandidates().Num())
	{
		return false;
	}

	const FGuid& SourceEntityId =
		Emission.GetContext().GetAction().GetSourceEntityId();
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const FShanmenControlledWeaponThreatTargetReceipt& Target =
			Targets[Index];
		if (!Target.IsValid()
			|| !CandidatesMatch(
				Target.Candidate,
				Emission.GetCandidates()[Index])
			|| Target.Decision != EvaluateThreatTarget(
				SourceEntityId,
				bRejectSelf,
				RequiredTargetTags,
				Target.Candidate.TargetEntityId,
				Target.TargetTags))
		{
			return false;
		}
	}
	return true;
}

int32 FShanmenControlledWeaponThreatPolicyReceipt::NumAcceptedTargets() const
{
	int32 Count = 0;
	for (const FShanmenControlledWeaponThreatTargetReceipt& Target : Targets)
	{
		Count += Target.IsAccepted() ? 1 : 0;
	}
	return Count;
}

bool FShanmenControlledWeaponThreatPresenceIntent::IsValid() const
{
	return IntentId.IsValid()
		&& RunId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& Candidate.IsValid()
		&& Candidate.DetectorKind
			== EShanmenHitDetectorKind::ControlledObject
		&& IntentId == MakeThreatPresenceIntentId(
			RunId, SourceItemInstanceId, Candidate);
}

bool FShanmenControlledWeaponThreatPresenceReceipt::IsValid() const
{
	if (!Policy.IsValid()
		|| Intents.Num() != Policy.NumAcceptedTargets())
	{
		return false;
	}

	const FShanmenCombatActionSnapshot& PolicyAction =
		Policy.GetEmission().GetContext().GetAction();
	TSet<FGuid> IntentIds;
	int32 IntentIndex = 0;
	for (const FShanmenControlledWeaponThreatTargetReceipt& Target :
		Policy.GetTargets())
	{
		if (!Target.IsAccepted())
		{
			continue;
		}

		if (!Intents.IsValidIndex(IntentIndex))
		{
			return false;
		}
		const FShanmenControlledWeaponThreatPresenceIntent& Intent =
			Intents[IntentIndex++];
		if (!Intent.IsValid()
			|| Intent.RunId != PolicyAction.GetRunId()
			|| Intent.SourceItemInstanceId
				!= PolicyAction.GetSourceItemInstanceId()
			|| !CandidatesMatch(Intent.Candidate, Target.GetCandidate())
			|| IntentIds.Contains(Intent.IntentId))
		{
			return false;
		}
		IntentIds.Add(Intent.IntentId);
	}
	return IntentIndex == Intents.Num();
}

bool FShanmenControlledWeaponExecution::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenControlledWeaponDefinition& Definition,
	const FShanmenControlledWeaponOffenseSnapshot& Offense,
	FShanmenControlledWeaponExecution& OutExecution)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenControlledWeaponDefinition FrozenDefinition = Definition;
	const FShanmenControlledWeaponOffenseSnapshot FrozenOffense = Offense;
	OutExecution.Reset();
	if (!FrozenAction.IsValid()
		|| !FrozenAction.GetSourceItemInstanceId().IsValid()
		|| !FrozenDefinition.IsValid()
		|| !FrozenOffense.IsValid()
		|| FrozenAction.GetActionDefinitionId()
			!= FrozenDefinition.GetActionDefinitionId())
	{
		return false;
	}

	FShanmenDetectorEmissionSession PreparedEmission;
	if (!FShanmenDetectorEmissionSession::TryStart(
			FrozenAction,
			FrozenDefinition.GetDetectorId(),
			EShanmenHitDetectorKind::ControlledObject,
			PreparedEmission))
	{
		return false;
	}

	OutExecution.Action = FrozenAction;
	OutExecution.Definition = FrozenDefinition;
	OutExecution.Offense = FrozenOffense;
	OutExecution.EmissionSession = MoveTemp(PreparedEmission);
	OutExecution.bInitialized = true;
	return OutExecution.IsValid();
}

bool FShanmenControlledWeaponExecution::IsValid() const
{
	return bInitialized
		&& Action.IsValid()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& Definition.IsValid()
		&& Offense.IsValid()
		&& Action.GetActionDefinitionId() == Definition.GetActionDefinitionId()
		&& EmissionSession.IsValid()
		&& NextCommandSequence >= 0
		&& CommandLedger.Num() <= NextCommandSequence
		&& ((State == EShanmenControlledWeaponState::Directed)
			? FMath::IsNearlyEqual(CurrentDirection.SizeSquared(), 1.0)
			: CurrentDirection.IsNearlyZero());
}

bool FShanmenControlledWeaponExecution::TryIssueCommand(
	const FShanmenActionOrchestrator& ActionRuntime,
	int64 ExpectedSequence,
	EShanmenControlledWeaponCommandKind Kind,
	const FVector& DesiredDirection,
	FShanmenControlledWeaponCommandReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponCommandReceipt();
	FVector Direction;
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| ExpectedSequence < 0
		|| !TryCanonicalDirection(Kind, DesiredDirection, Direction))
	{
		return false;
	}

	const FGuid CommandId = MakeCommandId(
		Action, ExpectedSequence, Kind, Direction);
	if (ExpectedSequence < NextCommandSequence)
	{
		const FShanmenControlledWeaponCommandReceipt* Existing =
			CommandLedger.Find(ExpectedSequence);
		if (!Existing || Existing->GetCommandId() != CommandId)
		{
			return false;
		}
		OutReceipt = *Existing;
		return OutReceipt.IsValid();
	}
	if (ExpectedSequence != NextCommandSequence)
	{
		return false;
	}

	const EShanmenControlledWeaponState StateBefore = State;
	const FVector DirectionBefore = CurrentDirection;
	switch (Kind)
	{
	case EShanmenControlledWeaponCommandKind::Launch:
		if (State != EShanmenControlledWeaponState::Orbiting
			|| EmissionSession.IsEmissionActive())
		{
			return false;
		}
		State = EShanmenControlledWeaponState::Directed;
		CurrentDirection = Direction;
		break;

	case EShanmenControlledWeaponCommandKind::Redirect:
		if (State != EShanmenControlledWeaponState::Directed)
		{
			return false;
		}
		CurrentDirection = Direction;
		break;

	case EShanmenControlledWeaponCommandKind::Recall:
		if (State != EShanmenControlledWeaponState::Directed
			|| EmissionSession.IsEmissionActive())
		{
			return false;
		}
		State = EShanmenControlledWeaponState::Recalled;
		CurrentDirection = FVector::ZeroVector;
		break;
	}

	OutReceipt.CommandId = CommandId;
	OutReceipt.ActivationId = Action.GetActivationId();
	OutReceipt.SourceItemInstanceId = Action.GetSourceItemInstanceId();
	OutReceipt.Sequence = NextCommandSequence;
	OutReceipt.Kind = Kind;
	OutReceipt.StateBefore = StateBefore;
	OutReceipt.StateAfter = State;
	OutReceipt.DirectionAfter = CurrentDirection;
	if (!OutReceipt.IsValid())
	{
		State = StateBefore;
		CurrentDirection = DirectionBefore;
		OutReceipt = FShanmenControlledWeaponCommandReceipt();
		return false;
	}

	CommandLedger.Add(NextCommandSequence, OutReceipt);
	++NextCommandSequence;
	return IsValid();
}

bool FShanmenControlledWeaponExecution::TryBeginEmission(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenControlledWeaponState::Directed
		&& FMath::IsNearlyEqual(CurrentDirection.SizeSquared(), 1.0)
		&& EmissionSession.TryBeginEmission(OutContext);
}

bool FShanmenControlledWeaponExecution::TryBeginOrbitThreatEmission(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenControlledWeaponState::Orbiting
		&& CurrentDirection.IsNearlyZero()
		&& EmissionSession.TryBeginEmission(OutContext);
}

bool FShanmenControlledWeaponExecution::TryAcceptOrbitThreatCandidate(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenHitCandidate& Candidate)
{
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenControlledWeaponState::Orbiting
		&& EmissionSession.IsEmissionActive()
		&& EmissionSession.TryAcceptCandidate(Candidate);
}

bool FShanmenControlledWeaponExecution::TryEndOrbitThreatEmission(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenDetectorEmissionReceipt& OutReceipt)
{
	OutReceipt = FShanmenDetectorEmissionReceipt();
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenControlledWeaponState::Orbiting
		&& EmissionSession.TryEndEmission(OutReceipt);
}

bool FShanmenControlledWeaponExecution::TryEndOrbitThreatEmission(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	FShanmenDetectorEmissionReceipt Ignored;
	return TryEndOrbitThreatEmission(ActionRuntime, Ignored);
}

bool FShanmenControlledWeaponExecution::
IsLatestCompletedOrbitThreatEmission(
	const FShanmenDetectorEmissionReceipt& Emission) const
{
	if (!IsValid()
		|| State != EShanmenControlledWeaponState::Orbiting
		|| EmissionSession.IsEmissionActive()
		|| !Emission.IsValid())
	{
		return false;
	}

	const FShanmenWorldHitContext& Context = Emission.GetContext();
	return Context.GetDetectorKind()
			== EShanmenHitDetectorKind::ControlledObject
		&& Context.GetDetectorId() == Definition.GetDetectorId()
		&& ActionsMatch(Context.GetAction(), Action)
		&& static_cast<int64>(Context.GetHitOrdinal()) + 1
			== EmissionSession.GetNextEmissionOrdinal();
}

bool FShanmenControlledWeaponExecution::TryEvaluateOrbitThreatReceipt(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
	FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| !IsLatestCompletedOrbitThreatEmission(Emission)
		|| TargetEvidence.Num() != Emission.GetCandidates().Num())
	{
		return false;
	}

	TMap<FGuid, FShanmenControlledWeaponThreatTargetEvidence> EvidenceByTarget;
	for (const FShanmenControlledWeaponThreatTargetEvidence& Evidence :
		TargetEvidence)
	{
		if (!Evidence.IsValid()
			|| EvidenceByTarget.Contains(Evidence.GetTargetEntityId()))
		{
			return false;
		}
		EvidenceByTarget.Add(Evidence.GetTargetEntityId(), Evidence);
	}

	OutReceipt.Emission = Emission;
	OutReceipt.RequiredTargetTags = Definition.GetRequiredTargetTags();
	OutReceipt.bRejectSelf = Definition.RejectsSelf();
	for (const FShanmenHitCandidate& Candidate : Emission.GetCandidates())
	{
		const FShanmenControlledWeaponThreatTargetEvidence* Evidence =
			EvidenceByTarget.Find(Candidate.TargetEntityId);
		if (!Evidence)
		{
			OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
			return false;
		}

		FShanmenControlledWeaponThreatTargetReceipt TargetReceipt;
		TargetReceipt.Candidate = Candidate;
		TargetReceipt.TargetTags = Evidence->GetTargetTags();
		TargetReceipt.Decision = EvaluateThreatTarget(
			Action.GetSourceEntityId(),
			Definition.RejectsSelf(),
			Definition.GetRequiredTargetTags(),
			Candidate.TargetEntityId,
			Evidence->GetTargetTags());
		OutReceipt.Targets.Add(MoveTemp(TargetReceipt));
	}

	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
		return false;
	}
	return true;
}

bool FShanmenControlledWeaponExecution::TryBuildOrbitThreatPresenceIntents(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
	FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPresenceReceipt();
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| !Policy.IsValid()
		|| !IsLatestCompletedOrbitThreatEmission(Policy.GetEmission())
		|| Policy.GetRequiredTargetTags()
			!= Definition.GetRequiredTargetTags()
		|| Policy.RejectsSelf() != Definition.RejectsSelf())
	{
		return false;
	}

	OutReceipt.Policy = Policy;
	OutReceipt.Intents.Reserve(Policy.NumAcceptedTargets());
	for (const FShanmenControlledWeaponThreatTargetReceipt& Target :
		Policy.GetTargets())
	{
		if (!Target.IsAccepted())
		{
			continue;
		}

		FShanmenControlledWeaponThreatPresenceIntent Intent;
		Intent.RunId = Action.GetRunId();
		Intent.SourceItemInstanceId = Action.GetSourceItemInstanceId();
		Intent.Candidate = Target.GetCandidate();
		Intent.IntentId = MakeThreatPresenceIntentId(
			Intent.RunId,
			Intent.SourceItemInstanceId,
			Intent.Candidate);
		OutReceipt.Intents.Add(MoveTemp(Intent));
	}

	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponThreatPresenceReceipt();
		return false;
	}
	return true;
}

bool FShanmenControlledWeaponExecution::TryResolveCandidate(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenHitCandidate& Candidate,
	const FShanmenTargetVitalitySnapshot& TargetVitality,
	const FShanmenDefenseSnapshot& Defense,
	FShanmenControlledWeaponImpactReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponImpactReceipt();
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| State != EShanmenControlledWeaponState::Directed
		|| !EmissionSession.IsEmissionActive()
		|| !TargetVitality.IsValid()
		|| !Defense.IsValid()
		|| !IsTargetAllowed(Candidate, Defense))
	{
		return false;
	}

	FShanmenDamagePacket Damage;
	if (!TryBuildDamagePacket(Damage))
	{
		return false;
	}

	FShanmenImpactRequest Request;
	Request.Action = Action;
	Request.Candidate = Candidate;
	Request.Damage = Damage;
	Request.TargetVitality = TargetVitality;
	Request.Defense = Defense;
	Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
		Action.GetRunId(),
		Candidate.ActivationId,
		Candidate.DetectorId,
		Candidate.TargetEntityId,
		Candidate.HitOrdinal);
	if (!Request.IsValid()
		|| !EmissionSession.TryAcceptCandidate(Candidate)
		|| !ImpactLedger.TryAccept(Request))
	{
		return false;
	}

	OutReceipt.Request = Request;
	OutReceipt.Result = FShanmenDefenseResolver::Resolve(Request);
	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenControlledWeaponImpactReceipt();
		return false;
	}
	return true;
}

bool FShanmenControlledWeaponExecution::TryEndEmission(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenControlledWeaponState::Directed
		&& EmissionSession.TryEndEmission();
}

void FShanmenControlledWeaponExecution::EndEmissionForTermination()
{
	if (EmissionSession.IsEmissionActive())
	{
		EmissionSession.TryEndEmission();
	}
}

void FShanmenControlledWeaponExecution::Reset()
{
	*this = FShanmenControlledWeaponExecution();
}

bool FShanmenControlledWeaponExecution::MatchesActionRuntime(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	if (!IsValid() || !ActionRuntime.IsValid())
	{
		return false;
	}

	const FShanmenCombatActionSnapshot& RuntimeAction =
		ActionRuntime.GetAction();
	return RuntimeAction.GetRunId() == Action.GetRunId()
		&& RuntimeAction.GetOwnerId() == Action.GetOwnerId()
		&& RuntimeAction.GetActivationId() == Action.GetActivationId()
		&& RuntimeAction.GetSourceEntityId() == Action.GetSourceEntityId()
		&& RuntimeAction.GetSourceItemInstanceId()
			== Action.GetSourceItemInstanceId()
		&& RuntimeAction.GetActionDefinitionId()
			== Action.GetActionDefinitionId()
		&& RuntimeAction.GetContent().Version == Action.GetContent().Version
		&& RuntimeAction.GetContent().Digest == Action.GetContent().Digest
		&& RuntimeAction.GetSourceTags() == Action.GetSourceTags();
}

bool FShanmenControlledWeaponExecution::IsTargetAllowed(
	const FShanmenHitCandidate& Candidate,
	const FShanmenDefenseSnapshot& Defense) const
{
	return Candidate.IsValid()
		&& Candidate.ActivationId == Action.GetActivationId()
		&& Candidate.SourceEntityId == Action.GetSourceEntityId()
		&& Candidate.DetectorId == Definition.GetDetectorId()
		&& Candidate.DetectorKind
			== EShanmenHitDetectorKind::ControlledObject
		&& (!Definition.RejectsSelf()
			|| Candidate.TargetEntityId != Action.GetSourceEntityId())
		&& Defense.TargetTags.HasAll(Definition.GetRequiredTargetTags());
}

bool FShanmenControlledWeaponExecution::TryBuildDamagePacket(
	FShanmenDamagePacket& OutPacket) const
{
	OutPacket = FShanmenDamagePacket();
	if (!Definition.IsValid() || !Offense.IsValid())
	{
		return false;
	}

	const double RawDamage = static_cast<double>(Definition.GetBaseDamage())
		+ static_cast<double>(Offense.GetControlPower())
			* static_cast<double>(Definition.GetControlPowerCoefficient());
	if (!FMath::IsFinite(RawDamage)
		|| RawDamage < 0.0
		|| RawDamage > static_cast<double>(MAX_flt))
	{
		return false;
	}

	OutPacket.FormulaId = Definition.GetFormulaId();
	OutPacket.RawDamage = static_cast<float>(RawDamage);
	OutPacket.DamageTags = Definition.GetDamageTags();
	return OutPacket.IsValid();
}
