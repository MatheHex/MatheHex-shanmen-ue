#include "ShanmenThrownWeaponExecution.h"

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

	FGuid MakeStraightLaunchId(
		const FShanmenCombatActionSnapshot& Action,
		const FVector& Origin,
		const FVector& Direction,
		float Speed)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ThrownWeapon.StraightLaunch.r1"),
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
				FloatBits(Speed)
			});
	}

	FGuid MakeArcLaunchId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenThrownWeaponArcPlan& Plan)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ThrownWeapon.ArcLaunch.r1"),
			{
				GuidDigits(Action.GetRunId()),
				GuidDigits(Action.GetOwnerId()),
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				GuidDigits(Action.GetSourceItemInstanceId()),
				Action.GetActionDefinitionId().ToString(),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest,
				GuidDigits(Plan.GetPlanId())
			});
	}

	bool NearlyEqualFloatAndDouble(float Left, double Right)
	{
		const double Tolerance = FMath::Max(
			1.0e-4,
			FMath::Abs(Right) * 1.0e-6);
		return FMath::IsNearlyEqual(
			static_cast<double>(Left), Right, Tolerance);
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

FName FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId()
{
	return StraightActionDefinitionId();
}

FName FShanmenThrownWeaponDefinition::StraightActionDefinitionId()
{
	return TEXT("Combat.Action.ThrownWeapon.Straight01");
}

FName FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
{
	return FShanmenThrownWeaponArcRequest::CanonicalActionDefinitionId();
}

bool FShanmenThrownWeaponDefinition::TryCapture(
	const FShanmenThrownWeaponDefinitionCapture& Capture,
	FShanmenThrownWeaponDefinition& OutDefinition)
{
	OutDefinition = FShanmenThrownWeaponDefinition();
	OutDefinition.ActionDefinitionId = Capture.ActionDefinitionId;
	OutDefinition.DetectorId = Capture.DetectorId;
	OutDefinition.FormulaId = Capture.FormulaId;
	OutDefinition.BaseDamage = Capture.BaseDamage;
	OutDefinition.TechniquePowerCoefficient =
		Capture.TechniquePowerCoefficient;
	OutDefinition.LaunchSpeed = CanonicalZero(Capture.LaunchSpeed);
	OutDefinition.DamageTags = Capture.DamageTags;
	OutDefinition.RequiredTargetTags = Capture.RequiredTargetTags;
	OutDefinition.bRejectSelf = Capture.bRejectSelf;
	if (!OutDefinition.IsValid())
	{
		OutDefinition = FShanmenThrownWeaponDefinition();
		return false;
	}
	return true;
}

bool FShanmenThrownWeaponDefinition::IsValid() const
{
	return (ActionDefinitionId == StraightActionDefinitionId()
			|| ActionDefinitionId == ArcActionDefinitionId())
		&& !DetectorId.IsNone()
		&& !FormulaId.IsNone()
		&& FMath::IsFinite(BaseDamage)
		&& BaseDamage >= 0.0f
		&& FMath::IsFinite(TechniquePowerCoefficient)
		&& TechniquePowerCoefficient >= 0.0f
		&& FMath::IsFinite(LaunchSpeed)
		&& LaunchSpeed > 0.0f
		&& DamageTags.HasTag(FShanmenCombatNativeTags::DamagePhysical())
		&& RequiredTargetTags.HasTag(
			FShanmenCombatNativeTags::TargetLiving())
		&& bRejectSelf;
}

bool FShanmenThrownWeaponOffenseSnapshot::TryCapture(
	float TechniquePower,
	FShanmenThrownWeaponOffenseSnapshot& OutSnapshot)
{
	OutSnapshot = FShanmenThrownWeaponOffenseSnapshot();
	if (!FMath::IsFinite(TechniquePower) || TechniquePower < 0.0f)
	{
		return false;
	}
	OutSnapshot.TechniquePower = CanonicalZero(TechniquePower);
	OutSnapshot.bCaptured = true;
	return true;
}

bool FShanmenThrownWeaponOffenseSnapshot::IsValid() const
{
	return bCaptured
		&& FMath::IsFinite(TechniquePower)
		&& TechniquePower >= 0.0f;
}

bool FShanmenThrownWeaponLaunchReceipt::IsValid() const
{
	if (!LaunchId.IsValid()
		|| !Action.IsValid()
		|| !Action.GetSourceItemInstanceId().IsValid()
		|| !IsFiniteVector(Origin)
		|| !IsFiniteVector(Direction)
		|| !FMath::IsNearlyEqual(Direction.SizeSquared(), 1.0)
		|| !FMath::IsFinite(Speed)
		|| Speed <= 0.0f)
	{
		return false;
	}

	if (TrajectoryKind == EShanmenThrownWeaponTrajectoryKind::Straight)
	{
		return Action.GetActionDefinitionId()
				== FShanmenThrownWeaponDefinition::StraightActionDefinitionId()
			&& !ArcPlan.IsValid()
			&& LaunchId == MakeStraightLaunchId(
				Action, Origin, Direction, Speed);
	}
	if (TrajectoryKind
		!= EShanmenThrownWeaponTrajectoryKind::BallisticArc
		|| Action.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		|| !ArcPlan.IsValid()
		|| !ActionsMatch(ArcPlan.GetRequest().GetAction(), Action)
		|| Origin != ArcPlan.GetRequest().GetOrigin()
		|| !NearlyEqualFloatAndDouble(Speed, ArcPlan.GetLaunchSpeed()))
	{
		return false;
	}

	FVector ExpectedDirection;
	return TryCanonicalDirection(
			ArcPlan.GetInitialVelocity(), ExpectedDirection)
		&& Direction.Equals(ExpectedDirection, UE_DOUBLE_SMALL_NUMBER)
		&& LaunchId == MakeArcLaunchId(Action, ArcPlan);
}

bool FShanmenThrownWeaponImpactReceipt::IsValid() const
{
	return Request.IsValid()
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool FShanmenThrownWeaponExecution::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenThrownWeaponDefinition& Definition,
	const FShanmenThrownWeaponOffenseSnapshot& Offense,
	FShanmenThrownWeaponExecution& OutExecution)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenThrownWeaponDefinition FrozenDefinition = Definition;
	const FShanmenThrownWeaponOffenseSnapshot FrozenOffense = Offense;
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
	OutExecution.State = EShanmenThrownWeaponState::Ready;
	OutExecution.bInitialized = true;
	return OutExecution.IsValid();
}

bool FShanmenThrownWeaponExecution::IsValid() const
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

	if (State == EShanmenThrownWeaponState::Ready)
	{
		return !LaunchReceipt.IsValid()
			&& !EmissionSession.IsEmissionActive();
	}
	if (!LaunchReceipt.IsValid()
		|| !ActionsMatch(LaunchReceipt.GetAction(), Action))
	{
		return false;
	}
	if (LaunchReceipt.GetTrajectoryKind()
		== EShanmenThrownWeaponTrajectoryKind::Straight)
	{
		if (Definition.GetActionDefinitionId()
				!= FShanmenThrownWeaponDefinition::StraightActionDefinitionId()
			|| LaunchReceipt.GetSpeed() != Definition.GetLaunchSpeed())
		{
			return false;
		}
	}
	else if (Definition.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		|| LaunchReceipt.GetArcPlan().GetRequest().GetMaximumLaunchSpeed()
			> static_cast<double>(Definition.GetLaunchSpeed())
		|| static_cast<double>(LaunchReceipt.GetSpeed())
			> static_cast<double>(Definition.GetLaunchSpeed())
		|| !NearlyEqualFloatAndDouble(
			LaunchReceipt.GetSpeed(),
			LaunchReceipt.GetArcPlan().GetLaunchSpeed()))
	{
		return false;
	}
	return State == EShanmenThrownWeaponState::InFlight
		|| (State == EShanmenThrownWeaponState::Spent
			&& !EmissionSession.IsEmissionActive());
}

bool FShanmenThrownWeaponExecution::TryLaunchStraight(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FVector& Origin,
	const FVector& AimDirection,
	FShanmenThrownWeaponLaunchReceipt& OutReceipt)
{
	OutReceipt = FShanmenThrownWeaponLaunchReceipt();
	FVector Direction;
	if (Definition.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::StraightActionDefinitionId()
		|| !MatchesActionRuntime(ActionRuntime)
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
	const FGuid LaunchId = MakeStraightLaunchId(
		Action,
		CanonicalOrigin,
		Direction,
		Definition.GetLaunchSpeed());
	if (LaunchReceipt.IsValid())
	{
		if (LaunchReceipt.GetLaunchId() != LaunchId)
		{
			return false;
		}
		OutReceipt = LaunchReceipt;
		return true;
	}
	if (State != EShanmenThrownWeaponState::Ready
		|| EmissionSession.IsEmissionActive())
	{
		return false;
	}

	OutReceipt.LaunchId = LaunchId;
	OutReceipt.Action = Action;
	OutReceipt.TrajectoryKind =
		EShanmenThrownWeaponTrajectoryKind::Straight;
	OutReceipt.Origin = CanonicalOrigin;
	OutReceipt.Direction = Direction;
	OutReceipt.Speed = Definition.GetLaunchSpeed();
	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenThrownWeaponLaunchReceipt();
		return false;
	}

	LaunchReceipt = OutReceipt;
	State = EShanmenThrownWeaponState::InFlight;
	if (!IsValid())
	{
		LaunchReceipt = FShanmenThrownWeaponLaunchReceipt();
		State = EShanmenThrownWeaponState::Ready;
		OutReceipt = FShanmenThrownWeaponLaunchReceipt();
		return false;
	}
	return true;
}

bool FShanmenThrownWeaponExecution::TryLaunchArc(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenThrownWeaponArcPlan& ArcPlan,
	FShanmenThrownWeaponLaunchReceipt& OutReceipt)
{
	OutReceipt = FShanmenThrownWeaponLaunchReceipt();
	FVector Direction;
	if (Definition.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		|| !MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| !ArcPlan.IsValid()
		|| !ActionsMatch(ArcPlan.GetRequest().GetAction(), Action)
		|| ArcPlan.GetRequest().GetMaximumLaunchSpeed()
			> static_cast<double>(Definition.GetLaunchSpeed())
		|| ArcPlan.GetLaunchSpeed()
			> static_cast<double>(Definition.GetLaunchSpeed())
		|| ArcPlan.GetLaunchSpeed() > static_cast<double>(MAX_flt)
		|| !TryCanonicalDirection(
			ArcPlan.GetInitialVelocity(), Direction))
	{
		return false;
	}

	const FGuid LaunchId = MakeArcLaunchId(Action, ArcPlan);
	if (LaunchReceipt.IsValid())
	{
		if (LaunchReceipt.GetLaunchId() != LaunchId)
		{
			return false;
		}
		OutReceipt = LaunchReceipt;
		return true;
	}
	if (State != EShanmenThrownWeaponState::Ready
		|| EmissionSession.IsEmissionActive())
	{
		return false;
	}

	OutReceipt.LaunchId = LaunchId;
	OutReceipt.Action = Action;
	OutReceipt.TrajectoryKind =
		EShanmenThrownWeaponTrajectoryKind::BallisticArc;
	OutReceipt.Origin = ArcPlan.GetRequest().GetOrigin();
	OutReceipt.Direction = Direction;
	OutReceipt.Speed = static_cast<float>(ArcPlan.GetLaunchSpeed());
	OutReceipt.ArcPlan = ArcPlan;
	if (!OutReceipt.IsValid())
	{
		OutReceipt = FShanmenThrownWeaponLaunchReceipt();
		return false;
	}

	LaunchReceipt = OutReceipt;
	State = EShanmenThrownWeaponState::InFlight;
	if (!IsValid())
	{
		LaunchReceipt = FShanmenThrownWeaponLaunchReceipt();
		State = EShanmenThrownWeaponState::Ready;
		OutReceipt = FShanmenThrownWeaponLaunchReceipt();
		return false;
	}
	return true;
}

bool FShanmenThrownWeaponExecution::TryBeginEmission(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenThrownWeaponState::InFlight
		&& LaunchReceipt.IsValid()
		&& EmissionSession.TryBeginEmission(OutContext);
}

bool FShanmenThrownWeaponExecution::TryResolveCandidate(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenHitCandidate& Candidate,
	const FShanmenTargetVitalitySnapshot& TargetVitality,
	const FShanmenDefenseSnapshot& Defense,
	FShanmenThrownWeaponImpactReceipt& OutReceipt)
{
	OutReceipt = FShanmenThrownWeaponImpactReceipt();
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| State != EShanmenThrownWeaponState::InFlight
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
		OutReceipt = FShanmenThrownWeaponImpactReceipt();
		return false;
	}
	return true;
}

bool FShanmenThrownWeaponExecution::TryEndEmission(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	return MatchesActionRuntime(ActionRuntime)
		&& ActionRuntime.CanEmitCandidates()
		&& State == EShanmenThrownWeaponState::InFlight
		&& EmissionSession.TryEndEmission();
}

bool FShanmenThrownWeaponExecution::TryFinishFlight(
	const FShanmenActionOrchestrator& ActionRuntime)
{
	if (!MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| State != EShanmenThrownWeaponState::InFlight
		|| EmissionSession.IsEmissionActive())
	{
		return false;
	}
	State = EShanmenThrownWeaponState::Spent;
	return IsValid();
}

void FShanmenThrownWeaponExecution::EndEmissionForTermination()
{
	if (EmissionSession.IsEmissionActive())
	{
		EmissionSession.TryEndEmission();
	}
}

void FShanmenThrownWeaponExecution::Reset()
{
	*this = FShanmenThrownWeaponExecution();
}

bool FShanmenThrownWeaponExecution::MatchesActionRuntime(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	return IsValid()
		&& ActionRuntime.IsValid()
		&& ActionsMatch(ActionRuntime.GetAction(), Action);
}

bool FShanmenThrownWeaponExecution::IsTargetAllowed(
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

bool FShanmenThrownWeaponExecution::TryBuildDamagePacket(
	FShanmenDamagePacket& OutPacket) const
{
	OutPacket = FShanmenDamagePacket();
	if (!Definition.IsValid() || !Offense.IsValid())
	{
		return false;
	}

	const double RawDamage = static_cast<double>(Definition.GetBaseDamage())
		+ static_cast<double>(Offense.GetTechniquePower())
			* static_cast<double>(
				Definition.GetTechniquePowerCoefficient());
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
