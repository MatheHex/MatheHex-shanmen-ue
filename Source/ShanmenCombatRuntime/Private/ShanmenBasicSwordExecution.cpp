#include "ShanmenBasicSwordExecution.h"

#include "ShanmenCombatTags.h"

FName FShanmenBasicSwordDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Sword.Basic01");
}

bool FShanmenBasicSwordDefinition::TryCapture(
	const FShanmenBasicSwordDefinitionCapture& Capture,
	FShanmenBasicSwordDefinition& OutDefinition)
{
	OutDefinition = FShanmenBasicSwordDefinition();
	OutDefinition.ActionDefinitionId = Capture.ActionDefinitionId;
	OutDefinition.DetectorId = Capture.DetectorId;
	OutDefinition.FormulaId = Capture.FormulaId;
	OutDefinition.BaseDamage = Capture.BaseDamage;
	OutDefinition.AttackPowerCoefficient = Capture.AttackPowerCoefficient;
	OutDefinition.DamageTags = Capture.DamageTags;
	OutDefinition.RequiredTargetTags = Capture.RequiredTargetTags;
	OutDefinition.bRejectSelf = Capture.bRejectSelf;
	if (!OutDefinition.IsValid())
	{
		OutDefinition = FShanmenBasicSwordDefinition();
		return false;
	}
	return true;
}

bool FShanmenBasicSwordDefinition::IsValid() const
{
	return ActionDefinitionId == CanonicalActionDefinitionId()
		&& !DetectorId.IsNone()
		&& !FormulaId.IsNone()
		&& FMath::IsFinite(BaseDamage)
		&& BaseDamage >= 0.0f
		&& FMath::IsFinite(AttackPowerCoefficient)
		&& AttackPowerCoefficient >= 0.0f
		&& DamageTags.HasTag(FShanmenCombatNativeTags::DamagePhysicalSlash())
		&& RequiredTargetTags.HasTag(FShanmenCombatNativeTags::TargetLiving())
		&& bRejectSelf;
}

bool FShanmenBasicSwordOffenseSnapshot::TryCapture(
	float AttackPower,
	FShanmenBasicSwordOffenseSnapshot& OutSnapshot)
{
	OutSnapshot = FShanmenBasicSwordOffenseSnapshot();
	if (!FMath::IsFinite(AttackPower) || AttackPower < 0.0f)
	{
		return false;
	}
	OutSnapshot.AttackPower = AttackPower;
	OutSnapshot.bCaptured = true;
	return true;
}

bool FShanmenBasicSwordOffenseSnapshot::IsValid() const
{
	return bCaptured && FMath::IsFinite(AttackPower) && AttackPower >= 0.0f;
}

bool FShanmenBasicSwordImpactReceipt::IsValid() const
{
	return Request.IsValid()
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool FShanmenBasicSwordExecution::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenBasicSwordDefinition& Definition,
	const FShanmenBasicSwordOffenseSnapshot& Offense,
	FShanmenBasicSwordExecution& OutExecution)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenBasicSwordDefinition FrozenDefinition = Definition;
	const FShanmenBasicSwordOffenseSnapshot FrozenOffense = Offense;
	OutExecution.Reset();
	if (!FrozenAction.IsValid()
		|| !FrozenDefinition.IsValid()
		|| !FrozenOffense.IsValid()
		|| FrozenAction.GetActionDefinitionId() != FrozenDefinition.GetActionDefinitionId())
	{
		return false;
	}

	FShanmenDetectorEmissionSession PreparedEmission;
	if (!FShanmenDetectorEmissionSession::TryStart(
			FrozenAction,
			FrozenDefinition.GetDetectorId(),
			EShanmenHitDetectorKind::WeaponTrajectory,
			PreparedEmission))
	{
		return false;
	}

	OutExecution.Action = FrozenAction;
	OutExecution.Definition = FrozenDefinition;
	OutExecution.Offense = FrozenOffense;
	OutExecution.EmissionSession = MoveTemp(PreparedEmission);
	return true;
}

bool FShanmenBasicSwordExecution::IsValid() const
{
	return Action.IsValid()
		&& Definition.IsValid()
		&& Offense.IsValid()
		&& Action.GetActionDefinitionId() == Definition.GetActionDefinitionId()
		&& EmissionSession.IsValid();
}

bool FShanmenBasicSwordExecution::TryBeginEmission(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& EmissionSession.TryBeginEmission(OutContext);
}

bool FShanmenBasicSwordExecution::TryResolveCandidate(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenHitCandidate& Candidate,
	const FShanmenTargetVitalitySnapshot& TargetVitality,
	const FShanmenDefenseSnapshot& Defense,
	FShanmenBasicSwordImpactReceipt& OutReceipt)
{
	OutReceipt = FShanmenBasicSwordImpactReceipt();
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
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

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	OutReceipt.Request = MoveTemp(Request);
	OutReceipt.Result = Result;
	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenBasicSwordImpactReceipt();
		return false;
	}
	return true;
}

bool FShanmenBasicSwordExecution::TryEndEmission(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& EmissionSession.TryEndEmission();
}

void FShanmenBasicSwordExecution::EndEmissionForTermination()
{
	if (EmissionSession.IsEmissionActive())
	{
		EmissionSession.TryEndEmission();
	}
}

void FShanmenBasicSwordExecution::Reset()
{
	*this = FShanmenBasicSwordExecution();
}

bool FShanmenBasicSwordExecution::MatchesActionRuntime(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	if (!IsValid() || !ActionRuntime.IsValid())
	{
		return false;
	}

	const FShanmenCombatActionSnapshot& RuntimeAction = ActionRuntime.GetAction();
	return RuntimeAction.GetRunId() == Action.GetRunId()
		&& RuntimeAction.GetOwnerId() == Action.GetOwnerId()
		&& RuntimeAction.GetActivationId() == Action.GetActivationId()
		&& RuntimeAction.GetSourceEntityId() == Action.GetSourceEntityId()
		&& RuntimeAction.GetSourceItemInstanceId() == Action.GetSourceItemInstanceId()
		&& RuntimeAction.GetActionDefinitionId() == Action.GetActionDefinitionId()
		&& RuntimeAction.GetContent().Version == Action.GetContent().Version
		&& RuntimeAction.GetContent().Digest == Action.GetContent().Digest
		&& RuntimeAction.GetSourceTags() == Action.GetSourceTags();
}

bool FShanmenBasicSwordExecution::IsTargetAllowed(
	const FShanmenHitCandidate& Candidate,
	const FShanmenDefenseSnapshot& Defense) const
{
	return Candidate.IsValid()
		&& Candidate.ActivationId == Action.GetActivationId()
		&& Candidate.SourceEntityId == Action.GetSourceEntityId()
		&& Candidate.DetectorId == Definition.GetDetectorId()
		&& Candidate.DetectorKind == EShanmenHitDetectorKind::WeaponTrajectory
		&& (!Definition.RejectsSelf() || Candidate.TargetEntityId != Action.GetSourceEntityId())
		&& Defense.TargetTags.HasAll(Definition.GetRequiredTargetTags());
}

bool FShanmenBasicSwordExecution::TryBuildDamagePacket(FShanmenDamagePacket& OutPacket) const
{
	OutPacket = FShanmenDamagePacket();
	if (!Definition.IsValid() || !Offense.IsValid())
	{
		return false;
	}

	const double RawDamage = static_cast<double>(Definition.GetBaseDamage())
		+ static_cast<double>(Offense.GetAttackPower())
			* static_cast<double>(Definition.GetAttackPowerCoefficient());
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
