#include "demo_mapShanmenWeaponGuardProductAuthority.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	FString FloatBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"),
			static_cast<unsigned long long>(Bits));
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
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool DefinitionsMatch(
		const FShanmenWeaponGuardDefinition& Left,
		const FShanmenWeaponGuardDefinition& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetRuleId() == Right.GetRuleId()
			&& Left.GetGuardFraction() == Right.GetGuardFraction()
			&& Left.GetRequiredDamageTags() == Right.GetRequiredDamageTags()
			&& Left.GetBlockedDamageTags() == Right.GetBlockedDamageTags()
			&& Left.GetRequiredSourceTags() == Right.GetRequiredSourceTags()
			&& Left.GetBlockedSourceTags() == Right.GetBlockedSourceTags()
			&& Left.GetRequiredTargetTags() == Right.GetRequiredTargetTags()
			&& Left.GetBlockedTargetTags() == Right.GetBlockedTargetTags();
	}
}

FName Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalContentVersion()
{
	return TEXT("0.0.10.P11.6");
}

FString Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalContentDigest()
{
	return TEXT("Shanmen.WeaponGuard.ProductConfig.r1");
}

FName Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalDefenseRuleId()
{
	return TEXT("Defense.Sword.WeaponGuard01");
}

FName Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalPerfectRuleId()
{
	return TEXT("Defense.Sword.PerfectGuard01");
}

FName Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalArcRuleId()
{
	return TEXT("Defense.Sword.WeaponGuardArc01");
}

float Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalGuardFraction()
{
	return 0.25f;
}

int64 Fdemo_mapShanmenWeaponGuardProductConfig::
CanonicalPerfectWindowTickCount()
{
	return 5;
}

double Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalMinimumFacingDot()
{
	return 0.5;
}

FGuid Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalConfigId()
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Sword.WeaponGuard.ProductConfig.r1"),
		{
			CanonicalContentVersion().ToString(),
			CanonicalContentDigest(),
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId()
				.ToString(),
			CanonicalDefenseRuleId().ToString(),
			FloatBits(CanonicalGuardFraction()),
			FShanmenCombatNativeTags::DamagePhysical().ToString(),
			FShanmenCombatNativeTags::DamageMental().ToString(),
			FShanmenCombatNativeTags::TargetLiving().ToString(),
			CanonicalPerfectRuleId().ToString(),
			CanonicalArcRuleId().ToString(),
			LexToString(CanonicalPerfectWindowTickCount()),
			DoubleBits(CanonicalMinimumFacingDot())
		});
}

bool Fdemo_mapShanmenWeaponGuardProductConfig::TryCreateCanonical(
	Fdemo_mapShanmenWeaponGuardProductConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenWeaponGuardProductConfig();
	Fdemo_mapShanmenWeaponGuardProductConfig Candidate;
	Candidate.Content.Version = CanonicalContentVersion();
	Candidate.Content.Digest = CanonicalContentDigest();

	FShanmenWeaponGuardDefinitionCapture Capture;
	Capture.ActionDefinitionId =
		FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
	Capture.RuleId = CanonicalDefenseRuleId();
	Capture.GuardFraction = CanonicalGuardFraction();
	Capture.RequiredDamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysical());
	Capture.BlockedDamageTags.AddTag(
		FShanmenCombatNativeTags::DamageMental());
	Capture.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	if (!FShanmenWeaponGuardDefinition::TryCapture(
			Capture, Candidate.Definition))
	{
		return false;
	}

	Candidate.PerfectRuleId = CanonicalPerfectRuleId();
	Candidate.ArcRuleId = CanonicalArcRuleId();
	Candidate.PerfectWindowTickCount = CanonicalPerfectWindowTickCount();
	Candidate.MinimumFacingDot = CanonicalMinimumFacingDot();
	Candidate.ConfigId = CanonicalConfigId();
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutConfig = Candidate;
	return true;
}

bool Fdemo_mapShanmenWeaponGuardProductConfig::IsValid() const
{
	FGameplayTagContainer ExpectedRequiredDamage;
	ExpectedRequiredDamage.AddTag(
		FShanmenCombatNativeTags::DamagePhysical());
	FGameplayTagContainer ExpectedBlockedDamage;
	ExpectedBlockedDamage.AddTag(FShanmenCombatNativeTags::DamageMental());
	FGameplayTagContainer ExpectedTarget;
	ExpectedTarget.AddTag(FShanmenCombatNativeTags::TargetLiving());
	return ConfigId.IsValid()
		&& ConfigId == CanonicalConfigId()
		&& Content.IsValid()
		&& Content.Version == CanonicalContentVersion()
		&& Content.Digest == CanonicalContentDigest()
		&& Definition.IsValid()
		&& Definition.GetActionDefinitionId()
			== FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId()
		&& Definition.GetRuleId() == CanonicalDefenseRuleId()
		&& Definition.GetGuardFraction() == CanonicalGuardFraction()
		&& Definition.GetRequiredDamageTags() == ExpectedRequiredDamage
		&& Definition.GetBlockedDamageTags() == ExpectedBlockedDamage
		&& Definition.GetRequiredSourceTags().IsEmpty()
		&& Definition.GetBlockedSourceTags().IsEmpty()
		&& Definition.GetRequiredTargetTags() == ExpectedTarget
		&& Definition.GetBlockedTargetTags().IsEmpty()
		&& PerfectRuleId == CanonicalPerfectRuleId()
		&& ArcRuleId == CanonicalArcRuleId()
		&& PerfectWindowTickCount == CanonicalPerfectWindowTickCount()
		&& MinimumFacingDot == CanonicalMinimumFacingDot();
}

bool Fdemo_mapPlayerWeaponGuardActionReservation::IsValid() const
{
	FGameplayTagContainer ExpectedSourceTags;
	ExpectedSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	return ActivationSequence > 0
		&& ConfigId
			== Fdemo_mapShanmenWeaponGuardProductConfig::CanonicalConfigId()
		&& SourceItemInstanceId.IsValid()
		&& Action.IsValid()
		&& Action.GetOwnerId() == Action.GetSourceEntityId()
		&& Action.GetSourceItemInstanceId() == SourceItemInstanceId
		&& Action.GetActionDefinitionId()
			== FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId()
		&& Action.GetContent().Version
			== Fdemo_mapShanmenWeaponGuardProductConfig::
				CanonicalContentVersion()
		&& Action.GetContent().Digest
			== Fdemo_mapShanmenWeaponGuardProductConfig::
				CanonicalContentDigest()
		&& Action.GetSourceTags() == ExpectedSourceTags
		&& Action.GetActivationId() == FShanmenCombatIdFactory::MakeActivationId(
			Action.GetRunId(),
			Action.GetSourceEntityId(),
			Action.GetActionDefinitionId(),
			ActivationSequence);
}

bool Fdemo_mapShanmenWeaponGuardProductStartResult::IsReady() const
{
	if (Status != Edemo_mapShanmenWeaponGuardProductStartStatus::Ready
		|| !Config.IsValid()
		|| !Reservation.IsValid()
		|| Reservation.GetConfigId() != Config.GetConfigId()
		|| !HostStart.IsSuccess()
		|| !Host.IsValid()
		|| !Host.IsActive()
		|| HostStart.HostId != Host.GetHostId())
	{
		return false;
	}

	const FShanmenWeaponGuardWindowReceipt& Window =
		Host.GetWindow().GetOpenReceipt();
	const FShanmenWeaponPerfectGuardPolicy& Timing =
		Host.GetTimingPolicy();
	const FShanmenWeaponGuardArcPolicy& Arc = Host.GetArcPolicy();
	return ActionsMatch(
			Host.GetActionRuntime().GetAction(), Reservation.GetAction())
		&& ActionsMatch(Window.GetAction(), Reservation.GetAction())
		&& DefinitionsMatch(Window.GetDefinition(), Config.GetDefinition())
		&& HostStart.Window.GetReceiptId() == Window.GetReceiptId()
		&& HostStart.TimingPolicy.GetPolicyId() == Timing.GetPolicyId()
		&& HostStart.ArcPolicy.GetPolicyId() == Arc.GetPolicyId()
		&& Timing.GetPerfectRuleId() == Config.GetPerfectRuleId()
		&& Timing.GetPerfectEndTick() - Timing.GetActiveStartTick()
			== Config.GetPerfectWindowTickCount()
		&& Arc.GetArcRuleId() == Config.GetArcRuleId()
		&& Arc.GetMinimumFacingDot() == Config.GetMinimumFacingDot();
}

Fdemo_mapShanmenWeaponGuardProductStartResult
Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FGuid& SourceItemInstanceId,
	const FGuid& TimelineId,
	int64 ActiveStartTick)
{
	Fdemo_mapShanmenWeaponGuardProductStartResult Result;
	if (!SourceItemInstanceId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidSourceItem;
		Result.Diagnostic =
			TEXT("Weapon guard requires one exact source item before identity reservation.");
		return Result;
	}
	const int64 WindowTicks =
		Fdemo_mapShanmenWeaponGuardProductConfig::
			CanonicalPerfectWindowTickCount();
	if (!TimelineId.IsValid()
		|| ActiveStartTick < 0
		|| ActiveStartTick > MAX_int64 - WindowTicks)
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline;
		Result.Diagnostic =
			TEXT("Weapon guard requires one valid monotonic timeline sample whose perfect window cannot overflow.");
		return Result;
	}
	if (!Fdemo_mapShanmenWeaponGuardProductConfig::TryCreateCanonical(
			Result.Config))
	{
		Result.Diagnostic =
			TEXT("Canonical weapon-guard product config failed closed.");
		return Result;
	}
	if (!Coordinator.TryReservePlayerWeaponGuardAction(
			Result.Config,
			SourceItemInstanceId,
			Result.Reservation,
			Result.Diagnostic))
	{
		Result.Status = Edemo_mapShanmenWeaponGuardProductStartStatus::
			ReservationRejected;
		return Result;
	}

	Result.HostStart = Fdemo_mapShanmenWeaponGuardProductHost::TryStart(
		Result.Reservation.GetAction(),
		Result.Config.GetDefinition(),
		TimelineId,
		ActiveStartTick,
		ActiveStartTick + Result.Config.GetPerfectWindowTickCount(),
		Result.Config.GetPerfectRuleId(),
		Result.Config.GetArcRuleId(),
		Result.Config.GetMinimumFacingDot(),
		Result.Host);
	if (!Result.HostStart.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardProductStartStatus::HostRejected;
		Result.Diagnostic =
			TEXT("Reserved weapon-guard identity could not start its product host; the sequence remains consumed.");
		return Result;
	}

	Result.Status = Edemo_mapShanmenWeaponGuardProductStartStatus::Ready;
	Result.Diagnostic =
		TEXT("Canonical weapon-guard config and Run identity started one active product host.");
	return Result;
}
