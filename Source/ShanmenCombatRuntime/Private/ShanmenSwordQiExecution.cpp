#include "ShanmenSwordQiExecution.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	double CanonicalZero(double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	float CanonicalZero(float Value)
	{
		return Value == 0.0f ? 0.0f : Value;
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	FString FloatBits(float Value)
	{
		Value = CanonicalZero(Value);
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool TryCanonicalDirection(
		const FVector& AimDirection,
		FVector& OutDirection)
	{
		OutDirection = FVector::ZeroVector;
		if (!IsFiniteVector(AimDirection) || AimDirection.IsNearlyZero())
		{
			return false;
		}

		OutDirection = AimDirection.GetSafeNormal();
		if (!IsFiniteVector(OutDirection)
			|| OutDirection.IsNearlyZero()
			|| !FMath::IsNearlyEqual(OutDirection.SizeSquared(), 1.0))
		{
			OutDirection = FVector::ZeroVector;
			return false;
		}

		OutDirection.X = CanonicalZero(OutDirection.X);
		OutDirection.Y = CanonicalZero(OutDirection.Y);
		OutDirection.Z = CanonicalZero(OutDirection.Z);
		return true;
	}

	FGuid MakeLaunchId(
		const FShanmenCombatActionSnapshot& Action,
		const FVector& Origin,
		const FVector& Direction,
		float Speed,
		float MaximumRange)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.SwordQi.Launch.r1"),
			{
				GuidDigits(Action.GetRunId()),
				GuidDigits(Action.GetOwnerId()),
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				GuidDigits(Action.GetSourceItemInstanceId()),
				Action.GetActionDefinitionId().ToString(),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest,
				DoubleBits(Origin.X),
				DoubleBits(Origin.Y),
				DoubleBits(Origin.Z),
				DoubleBits(Direction.X),
				DoubleBits(Direction.Y),
				DoubleBits(Direction.Z),
				FloatBits(Speed),
				FloatBits(MaximumRange)
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
}

FName FShanmenSwordQiDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Sword.Qi.Basic01");
}

bool FShanmenSwordQiDefinition::TryCapture(
	const FShanmenSwordQiDefinitionCapture& Capture,
	FShanmenSwordQiDefinition& OutDefinition)
{
	OutDefinition = FShanmenSwordQiDefinition();
	OutDefinition.ActionDefinitionId = Capture.ActionDefinitionId;
	OutDefinition.DetectorId = Capture.DetectorId;
	OutDefinition.FormulaId = Capture.FormulaId;
	OutDefinition.BaseDamage = CanonicalZero(Capture.BaseDamage);
	OutDefinition.AttackPowerCoefficient =
		CanonicalZero(Capture.AttackPowerCoefficient);
	OutDefinition.FlightSpeed = CanonicalZero(Capture.FlightSpeed);
	OutDefinition.MaximumRange = CanonicalZero(Capture.MaximumRange);
	OutDefinition.DamageTags = Capture.DamageTags;
	OutDefinition.RequiredTargetTags = Capture.RequiredTargetTags;
	OutDefinition.bRejectSelf = Capture.bRejectSelf;
	if (!OutDefinition.IsValid())
	{
		OutDefinition = FShanmenSwordQiDefinition();
		return false;
	}
	return true;
}

bool FShanmenSwordQiDefinition::IsValid() const
{
	return ActionDefinitionId == CanonicalActionDefinitionId()
		&& !DetectorId.IsNone()
		&& !FormulaId.IsNone()
		&& FMath::IsFinite(BaseDamage)
		&& BaseDamage >= 0.0f
		&& FMath::IsFinite(AttackPowerCoefficient)
		&& AttackPowerCoefficient >= 0.0f
		&& FMath::IsFinite(FlightSpeed)
		&& FlightSpeed > 0.0f
		&& FMath::IsFinite(MaximumRange)
		&& MaximumRange > 0.0f
		&& DamageTags.HasTag(FShanmenCombatNativeTags::Damage())
		&& RequiredTargetTags.HasTag(
			FShanmenCombatNativeTags::TargetLiving())
		&& bRejectSelf;
}

bool FShanmenSwordQiOffenseSnapshot::TryCapture(
	float AttackPower,
	FShanmenSwordQiOffenseSnapshot& OutSnapshot)
{
	OutSnapshot = FShanmenSwordQiOffenseSnapshot();
	if (!FMath::IsFinite(AttackPower) || AttackPower < 0.0f)
	{
		return false;
	}
	OutSnapshot.AttackPower = CanonicalZero(AttackPower);
	OutSnapshot.bCaptured = true;
	return true;
}

bool FShanmenSwordQiOffenseSnapshot::IsValid() const
{
	return bCaptured
		&& FMath::IsFinite(AttackPower)
		&& AttackPower >= 0.0f;
}

bool FShanmenSwordQiLaunchReceipt::IsValid() const
{
	return LaunchId.IsValid()
		&& Action.IsValid()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenSwordQiDefinition::CanonicalActionDefinitionId()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(Direction)
		&& FMath::IsNearlyEqual(Direction.SizeSquared(), 1.0)
		&& FMath::IsFinite(Speed)
		&& Speed > 0.0f
		&& FMath::IsFinite(MaximumRange)
		&& MaximumRange > 0.0f
		&& LaunchId == MakeLaunchId(
			Action, Origin, Direction, Speed, MaximumRange);
}

bool FShanmenSwordQiImpactReceipt::IsValid() const
{
	return Request.IsValid()
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool FShanmenSwordQiExecution::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSwordQiDefinition& Definition,
	const FShanmenSwordQiOffenseSnapshot& Offense,
	FShanmenSwordQiExecution& OutExecution)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenSwordQiDefinition FrozenDefinition = Definition;
	const FShanmenSwordQiOffenseSnapshot FrozenOffense = Offense;
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
			EShanmenHitDetectorKind::Projectile,
			PreparedEmission))
	{
		return false;
	}

	OutExecution.Action = FrozenAction;
	OutExecution.Definition = FrozenDefinition;
	OutExecution.Offense = FrozenOffense;
	OutExecution.EmissionSession = MoveTemp(PreparedEmission);
	OutExecution.State = EShanmenSwordQiState::Ready;
	OutExecution.bInitialized = true;
	return OutExecution.IsValid();
}

bool FShanmenSwordQiExecution::IsValid() const
{
	if (!bInitialized
		|| !Action.IsValid()
		|| !Action.GetSourceItemInstanceId().IsValid()
		|| !Definition.IsValid()
		|| !Offense.IsValid()
		|| Action.GetActionDefinitionId()
			!= Definition.GetActionDefinitionId()
		|| !EmissionSession.IsValid())
	{
		return false;
	}

	if (State == EShanmenSwordQiState::Ready)
	{
		return !LaunchReceipt.IsValid()
			&& !EmissionSession.IsEmissionActive();
	}
	if (!LaunchReceipt.IsValid()
		|| !ActionsMatch(LaunchReceipt.GetAction(), Action)
		|| LaunchReceipt.GetSpeed() != Definition.GetFlightSpeed()
		|| LaunchReceipt.GetMaximumRange()
			!= Definition.GetMaximumRange())
	{
		return false;
	}
	return State == EShanmenSwordQiState::InFlight
		|| (State == EShanmenSwordQiState::Dissipated
			&& !EmissionSession.IsEmissionActive());
}

bool FShanmenSwordQiExecution::TryLaunch(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FVector& Origin,
	const FVector& AimDirection,
	FShanmenSwordQiLaunchReceipt& OutReceipt)
{
	OutReceipt = FShanmenSwordQiLaunchReceipt();
	FVector Direction;
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| !IsFiniteVector(Origin)
		|| !TryCanonicalDirection(AimDirection, Direction))
	{
		return false;
	}

	FVector CanonicalOrigin = Origin;
	CanonicalOrigin.X = CanonicalZero(CanonicalOrigin.X);
	CanonicalOrigin.Y = CanonicalZero(CanonicalOrigin.Y);
	CanonicalOrigin.Z = CanonicalZero(CanonicalOrigin.Z);
	const FGuid LaunchId = MakeLaunchId(
		Action,
		CanonicalOrigin,
		Direction,
		Definition.GetFlightSpeed(),
		Definition.GetMaximumRange());
	if (LaunchReceipt.IsValid())
	{
		if (LaunchReceipt.GetLaunchId() != LaunchId)
		{
			return false;
		}
		OutReceipt = LaunchReceipt;
		return true;
	}
	if (State != EShanmenSwordQiState::Ready
		|| EmissionSession.IsEmissionActive())
	{
		return false;
	}

	OutReceipt.LaunchId = LaunchId;
	OutReceipt.Action = Action;
	OutReceipt.Origin = CanonicalOrigin;
	OutReceipt.Direction = Direction;
	OutReceipt.Speed = Definition.GetFlightSpeed();
	OutReceipt.MaximumRange = Definition.GetMaximumRange();
	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenSwordQiLaunchReceipt();
		return false;
	}

	LaunchReceipt = OutReceipt;
	State = EShanmenSwordQiState::InFlight;
	if (!IsValid())
	{
		LaunchReceipt = FShanmenSwordQiLaunchReceipt();
		State = EShanmenSwordQiState::Ready;
		OutReceipt = FShanmenSwordQiLaunchReceipt();
		return false;
	}
	return true;
}

bool FShanmenSwordQiExecution::TryBeginEmission(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenSwordQiState::InFlight
		&& LaunchReceipt.IsValid()
		&& EmissionSession.TryBeginEmission(OutContext);
}

bool FShanmenSwordQiExecution::TryResolveCandidate(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenHitCandidate& Candidate,
	const FShanmenTargetVitalitySnapshot& TargetVitality,
	const FShanmenDefenseSnapshot& Defense,
	FShanmenSwordQiImpactReceipt& OutReceipt)
{
	OutReceipt = FShanmenSwordQiImpactReceipt();
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| State != EShanmenSwordQiState::InFlight
		|| !LaunchReceipt.IsValid()
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
		OutReceipt = FShanmenSwordQiImpactReceipt();
		return false;
	}
	return true;
}

bool FShanmenSwordQiExecution::TryEndEmission(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenSwordQiState::InFlight
		&& EmissionSession.TryEndEmission();
}

bool FShanmenSwordQiExecution::TryDissipate(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| State != EShanmenSwordQiState::InFlight
		|| EmissionSession.IsEmissionActive())
	{
		return false;
	}
	State = EShanmenSwordQiState::Dissipated;
	return IsValid();
}

void FShanmenSwordQiExecution::EndForActionTermination()
{
	if (EmissionSession.IsEmissionActive())
	{
		EmissionSession.TryEndEmission();
	}
	if (State == EShanmenSwordQiState::InFlight)
	{
		State = EShanmenSwordQiState::Dissipated;
	}
}

void FShanmenSwordQiExecution::Reset()
{
	*this = FShanmenSwordQiExecution();
}

bool FShanmenSwordQiExecution::MatchesActionRuntime(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	return IsValid()
		&& ActionRuntime.IsValid()
		&& ActionsMatch(ActionRuntime.GetAction(), Action);
}

bool FShanmenSwordQiExecution::IsTargetAllowed(
	const FShanmenHitCandidate& Candidate,
	const FShanmenDefenseSnapshot& Defense) const
{
	return Candidate.IsValid()
		&& Candidate.ActivationId == Action.GetActivationId()
		&& Candidate.SourceEntityId == Action.GetSourceEntityId()
		&& Candidate.DetectorId == Definition.GetDetectorId()
		&& Candidate.DetectorKind == EShanmenHitDetectorKind::Projectile
		&& (!Definition.RejectsSelf()
			|| Candidate.TargetEntityId != Action.GetSourceEntityId())
		&& Defense.TargetTags.HasAll(
			Definition.GetRequiredTargetTags());
}

bool FShanmenSwordQiExecution::TryBuildDamagePacket(
	FShanmenDamagePacket& OutPacket) const
{
	OutPacket = FShanmenDamagePacket();
	if (!Definition.IsValid() || !Offense.IsValid())
	{
		return false;
	}

	const double RawDamage = static_cast<double>(Definition.GetBaseDamage())
		+ static_cast<double>(Offense.GetAttackPower())
			* static_cast<double>(
				Definition.GetAttackPowerCoefficient());
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
