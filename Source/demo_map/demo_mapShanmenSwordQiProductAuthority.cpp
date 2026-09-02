#include "demo_mapShanmenSwordQiProductAuthority.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	constexpr float CanonicalBaseDamage = 0.5f;
	constexpr float CanonicalAttackPowerCoefficient = 0.01f;
	constexpr float CanonicalFlightSpeed = 900.0f;
	constexpr float CanonicalMaximumRange = 1400.0f;

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FString FloatBits(float Value)
	{
		Value = Value == 0.0f ? 0.0f : Value;
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& SameContent(Left.GetContent(), Right.GetContent())
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool DefinitionsMatch(
		const FShanmenSwordQiDefinition& Left,
		const FShanmenSwordQiDefinition& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetDetectorId() == Right.GetDetectorId()
			&& Left.GetFormulaId() == Right.GetFormulaId()
			&& Left.GetBaseDamage() == Right.GetBaseDamage()
			&& Left.GetAttackPowerCoefficient()
				== Right.GetAttackPowerCoefficient()
			&& Left.GetFlightSpeed() == Right.GetFlightSpeed()
			&& Left.GetMaximumRange() == Right.GetMaximumRange()
			&& Left.GetDamageTags() == Right.GetDamageTags()
			&& Left.GetRequiredTargetTags()
				== Right.GetRequiredTargetTags()
			&& Left.RejectsSelf() == Right.RejectsSelf();
	}
}

FName Fdemo_mapShanmenSwordQiProductConfig::CanonicalContentVersion()
{
	return TEXT("0.0.10.P18.3");
}

FString Fdemo_mapShanmenSwordQiProductConfig::CanonicalContentDigest()
{
	return TEXT("Shanmen.SwordQi.ProductConfig.r1");
}

FName Fdemo_mapShanmenSwordQiProductConfig::CanonicalDetectorId()
{
	return TEXT("Detector.SwordQi.Basic01.Straight");
}

FName Fdemo_mapShanmenSwordQiProductConfig::CanonicalFormulaId()
{
	return TEXT("Combat.Formula.SwordQi.Basic01.r1");
}

FGuid Fdemo_mapShanmenSwordQiProductConfig::CanonicalConfigId()
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.SwordQi.ProductConfig.r1"),
		{
			CanonicalContentVersion().ToString(),
			CanonicalContentDigest(),
			FShanmenSwordQiDefinition::CanonicalActionDefinitionId()
				.ToString(),
			CanonicalDetectorId().ToString(),
			CanonicalFormulaId().ToString(),
			FloatBits(CanonicalBaseDamage),
			FloatBits(CanonicalAttackPowerCoefficient),
			FloatBits(CanonicalFlightSpeed),
			FloatBits(CanonicalMaximumRange),
			FShanmenCombatNativeTags::DamageSpirit().ToString(),
			FShanmenCombatNativeTags::TargetLiving().ToString()
		});
}

bool Fdemo_mapShanmenSwordQiProductConfig::TryCreateCanonical(
	Fdemo_mapShanmenSwordQiProductConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenSwordQiProductConfig();
	Fdemo_mapShanmenSwordQiProductConfig Candidate;
	Candidate.Content.Version = CanonicalContentVersion();
	Candidate.Content.Digest = CanonicalContentDigest();

	FShanmenSwordQiDefinitionCapture Capture;
	Capture.ActionDefinitionId =
		FShanmenSwordQiDefinition::CanonicalActionDefinitionId();
	Capture.DetectorId = CanonicalDetectorId();
	Capture.FormulaId = CanonicalFormulaId();
	Capture.BaseDamage = CanonicalBaseDamage;
	Capture.AttackPowerCoefficient = CanonicalAttackPowerCoefficient;
	Capture.FlightSpeed = CanonicalFlightSpeed;
	Capture.MaximumRange = CanonicalMaximumRange;
	Capture.DamageTags.AddTag(FShanmenCombatNativeTags::DamageSpirit());
	Capture.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	Capture.bRejectSelf = true;
	if (!FShanmenSwordQiDefinition::TryCapture(
			Capture, Candidate.Definition))
	{
		return false;
	}

	Candidate.ConfigId = CanonicalConfigId();
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutConfig = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordQiProductConfig::IsValid() const
{
	FGameplayTagContainer ExpectedDamageTags;
	ExpectedDamageTags.AddTag(FShanmenCombatNativeTags::DamageSpirit());
	FGameplayTagContainer ExpectedTargetTags;
	ExpectedTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	return ConfigId.IsValid()
		&& ConfigId == CanonicalConfigId()
		&& Content.IsValid()
		&& Content.Version == CanonicalContentVersion()
		&& Content.Digest == CanonicalContentDigest()
		&& Definition.IsValid()
		&& Definition.GetActionDefinitionId()
			== FShanmenSwordQiDefinition::CanonicalActionDefinitionId()
		&& Definition.GetDetectorId() == CanonicalDetectorId()
		&& Definition.GetFormulaId() == CanonicalFormulaId()
		&& Definition.GetBaseDamage() == CanonicalBaseDamage
		&& Definition.GetAttackPowerCoefficient()
			== CanonicalAttackPowerCoefficient
		&& Definition.GetFlightSpeed() == CanonicalFlightSpeed
		&& Definition.GetMaximumRange() == CanonicalMaximumRange
		&& Definition.GetDamageTags() == ExpectedDamageTags
		&& Definition.GetRequiredTargetTags() == ExpectedTargetTags
		&& Definition.RejectsSelf();
}

bool Fdemo_mapPlayerSwordQiActionReservation::IsValid() const
{
	FGameplayTagContainer ExpectedSourceTags;
	ExpectedSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	return ActivationSequence > 0
		&& ConfigId == Fdemo_mapShanmenSwordQiProductConfig::CanonicalConfigId()
		&& Action.IsValid()
		&& Action.GetOwnerId() == Action.GetSourceEntityId()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenSwordQiDefinition::CanonicalActionDefinitionId()
		&& Action.GetContent().Version
			== Fdemo_mapShanmenSwordQiProductConfig::CanonicalContentVersion()
		&& Action.GetContent().Digest
			== Fdemo_mapShanmenSwordQiProductConfig::CanonicalContentDigest()
		&& Action.GetSourceTags() == ExpectedSourceTags
		&& Action.GetActivationId() == FShanmenCombatIdFactory::MakeActivationId(
			Action.GetRunId(),
			Action.GetSourceEntityId(),
			Action.GetActionDefinitionId(),
			ActivationSequence);
}

bool Fdemo_mapShanmenSwordQiLaunchCommand::TryCapture(
	const Fdemo_mapPlayerSwordQiActionReservation& Reservation,
	const Fdemo_mapShanmenSwordQiProductConfig& Config,
	const FShanmenSwordQiOffenseSnapshot& RequestedOffense,
	const FVector& RequestedOrigin,
	const FVector& RequestedAimDirection,
	Fdemo_mapShanmenSwordQiLaunchCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordQiLaunchCommand();
	if (!Reservation.IsValid()
		|| !Config.IsValid()
		|| Reservation.GetConfigId() != Config.GetConfigId()
		|| !RequestedOffense.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedAimDirection)
		|| RequestedAimDirection.IsNearlyZero())
	{
		return false;
	}

	OutCommand.ConfigId = Config.GetConfigId();
	OutCommand.Action = Reservation.GetAction();
	OutCommand.Definition = Config.GetDefinition();
	OutCommand.Offense = RequestedOffense;
	OutCommand.Origin = RequestedOrigin;
	OutCommand.AimDirection = RequestedAimDirection.GetSafeNormal();
	if (!OutCommand.IsValid())
	{
		OutCommand = Fdemo_mapShanmenSwordQiLaunchCommand();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordQiLaunchCommand::IsValid() const
{
	Fdemo_mapShanmenSwordQiProductConfig Canonical;
	return Fdemo_mapShanmenSwordQiProductConfig::TryCreateCanonical(Canonical)
		&& ConfigId == Canonical.GetConfigId()
		&& Action.IsValid()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenSwordQiDefinition::CanonicalActionDefinitionId()
		&& SameContent(Action.GetContent(), Canonical.GetContent())
		&& DefinitionsMatch(Definition, Canonical.GetDefinition())
		&& Offense.IsValid()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(AimDirection)
		&& AimDirection.IsNormalized();
}

bool Fdemo_mapShanmenSwordQiLaunchCommand::Matches(
	const Fdemo_mapShanmenSwordQiLaunchCommand& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& ConfigId == Other.ConfigId
		&& ActionsMatch(Action, Other.Action)
		&& DefinitionsMatch(Definition, Other.Definition)
		&& Offense.GetAttackPower() == Other.Offense.GetAttackPower()
		&& Origin == Other.Origin
		&& AimDirection == Other.AimDirection;
}

bool Fdemo_mapShanmenSwordQiProductStartResult::IsReady() const
{
	return Status == Edemo_mapShanmenSwordQiProductStartStatus::Ready
		&& Config.IsValid()
		&& Reservation.IsValid()
		&& Reservation.GetConfigId() == Config.GetConfigId()
		&& Command.IsValid()
		&& Command.GetCommandId() == Reservation.GetActivationId()
		&& Command.GetConfigId() == Config.GetConfigId()
		&& ActionsMatch(Command.GetAction(), Reservation.GetAction())
		&& DefinitionsMatch(Command.GetDefinition(), Config.GetDefinition());
}

Fdemo_mapShanmenSwordQiProductStartResult
Fdemo_mapShanmenSwordQiProductAuthority::PrepareLaunch(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FGuid& SourceItemInstanceId,
	float AttackPower,
	const FVector& Origin,
	const FVector& AimDirection)
{
	Fdemo_mapShanmenSwordQiProductStartResult Result;
	if (!SourceItemInstanceId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductStartStatus::InvalidSourceItem;
		Result.Diagnostic =
			TEXT("Sword Qi requires one exact sword item identity.");
		return Result;
	}
	FShanmenSwordQiOffenseSnapshot Offense;
	if (!FShanmenSwordQiOffenseSnapshot::TryCapture(AttackPower, Offense))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductStartStatus::InvalidOffense;
		Result.Diagnostic =
			TEXT("Sword Qi requires one finite non-negative attack snapshot.");
		return Result;
	}
	if (!IsFiniteVector(Origin))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductStartStatus::InvalidOrigin;
		Result.Diagnostic = TEXT("Sword Qi requires one finite launch origin.");
		return Result;
	}
	if (!IsFiniteVector(AimDirection) || AimDirection.IsNearlyZero())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductStartStatus::InvalidDirection;
		Result.Diagnostic =
			TEXT("Sword Qi requires one finite non-zero aim direction.");
		return Result;
	}
	if (!Fdemo_mapShanmenSwordQiProductConfig::TryCreateCanonical(
			Result.Config))
	{
		Result.Diagnostic = TEXT("Canonical Sword Qi config failed closed.");
		return Result;
	}
	if (!Coordinator.TryReservePlayerSwordQiAction(
			Result.Config,
			SourceItemInstanceId,
			Result.Reservation,
			Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductStartStatus::ReservationRejected;
		return Result;
	}
	if (!Fdemo_mapShanmenSwordQiLaunchCommand::TryCapture(
			Result.Reservation,
			Result.Config,
			Offense,
			Origin,
			AimDirection,
			Result.Command))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiProductStartStatus::CommandRejected;
		Result.Diagnostic =
			TEXT("Reserved Sword Qi identity could not freeze a launch command; the sequence remains consumed.");
		return Result;
	}

	Result.Status = Edemo_mapShanmenSwordQiProductStartStatus::Ready;
	Result.Diagnostic =
		TEXT("Canonical Sword Qi policy and Run identity prepared one launch command.");
	return Result;
}
