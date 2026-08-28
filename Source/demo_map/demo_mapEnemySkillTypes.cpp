#include "demo_mapEnemySkillTypes.h"

const FName Fdemo_mapEnemySkillProfileIds::StandardMeleeDash(
	TEXT("P6.Standard.MeleeDash"));
const FName Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep(
	TEXT("P6.Standard.RangedBackstep"));
const FName Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash(
	TEXT("P7.Enhanced.MeleeDash"));
const FName Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep(
	TEXT("P7.Enhanced.RangedBackstep"));

bool Fdemo_mapEnemySkillDefinition::IsValid() const
{
	return Kind != Edemo_mapEnemySkillKind::None
		&& FMath::IsFinite(TriggerMinDistance)
		&& FMath::IsFinite(TriggerMaxDistance)
		&& FMath::IsFinite(WindupDuration)
		&& FMath::IsFinite(DisplacementDistance)
		&& FMath::IsFinite(DisplacementDuration)
		&& FMath::IsFinite(MinimumResolvedDistance)
		&& FMath::IsFinite(RecoveryDuration)
		&& FMath::IsFinite(CooldownDuration)
		&& TriggerMinDistance >= 0.0f
		&& TriggerMaxDistance >= TriggerMinDistance
		&& WindupDuration >= 0.0f
		&& DisplacementDistance > 0.0f
		&& DisplacementDuration > 0.0f
		&& MinimumResolvedDistance >= 0.0f
		&& MinimumResolvedDistance <= DisplacementDistance
		&& RecoveryDuration >= 0.0f
		&& CooldownDuration >= 0.0f;
}

bool Fdemo_mapEnemySkillDefinition::CanResolveRangedFire(
	float ActualResolvedDistance,
	bool bTargetValid,
	bool bHasWorldStaticLineOfSight) const
{
	return Kind == Edemo_mapEnemySkillKind::RangedBackstepShot
		&& FMath::IsFinite(ActualResolvedDistance)
		&& ActualResolvedDistance + KINDA_SMALL_NUMBER
			>= MinimumResolvedDistance
		&& bTargetValid
		&& bHasWorldStaticLineOfSight;
}

bool Fdemo_mapEnemySkillDefinition::IsInsideTriggerRange(float Distance) const
{
	return Distance >= TriggerMinDistance && Distance <= TriggerMaxDistance;
}

const Fdemo_mapEnemySkillPrototypeConfig&
Fdemo_mapEnemySkillPrototypeConfig::Get()
{
	static const Fdemo_mapEnemySkillPrototypeConfig Config = []()
	{
		Fdemo_mapEnemySkillPrototypeConfig Result;
		Result.MeleeDash.Kind = Edemo_mapEnemySkillKind::MeleeDash;
		Result.MeleeDash.TriggerMinDistance =
			Fdemo_mapEnemySkillPrototypeValues::MeleeTriggerMin;
		Result.MeleeDash.TriggerMaxDistance =
			Fdemo_mapEnemySkillPrototypeValues::MeleeTriggerMax;
		Result.MeleeDash.WindupDuration =
			Fdemo_mapEnemySkillPrototypeValues::MeleeWindup;
		Result.MeleeDash.DisplacementDistance =
			Fdemo_mapEnemySkillPrototypeValues::MeleeDistance;
		Result.MeleeDash.DisplacementDuration =
			Fdemo_mapEnemySkillPrototypeValues::MeleeDisplacementDuration;
		Result.MeleeDash.MinimumResolvedDistance =
			Fdemo_mapEnemySkillPrototypeValues::MeleeMinimumResolvedDistance;
		Result.MeleeDash.RecoveryDuration =
			Fdemo_mapEnemySkillPrototypeValues::MeleeRecovery;
		Result.MeleeDash.CooldownDuration =
			Fdemo_mapEnemySkillPrototypeValues::MeleeCooldown;
		Result.MeleeDash.bLockDirectionAtEndOfWindup = true;
		Result.MeleeDash.bDirectionAwayFromTarget = false;
		Result.MeleeDash.bAppliesContactDamage = true;
		Result.MeleeDash.bStopsOnFirstLegalHit = true;

		Result.RangedBackstepShot.Kind =
			Edemo_mapEnemySkillKind::RangedBackstepShot;
		Result.RangedBackstepShot.TriggerMinDistance = 0.0f;
		Result.RangedBackstepShot.TriggerMaxDistance =
			Fdemo_mapEnemySkillPrototypeValues::RangedTriggerMax;
		Result.RangedBackstepShot.WindupDuration =
			Fdemo_mapEnemySkillPrototypeValues::RangedWindup;
		Result.RangedBackstepShot.DisplacementDistance =
			Fdemo_mapEnemySkillPrototypeValues::RangedDistance;
		Result.RangedBackstepShot.DisplacementDuration =
			Fdemo_mapEnemySkillPrototypeValues::RangedDisplacementDuration;
		Result.RangedBackstepShot.MinimumResolvedDistance =
			Fdemo_mapEnemySkillPrototypeValues::RangedMinimumResolvedDistance;
		Result.RangedBackstepShot.RecoveryDuration =
			Fdemo_mapEnemySkillPrototypeValues::RangedRecovery;
		Result.RangedBackstepShot.CooldownDuration =
			Fdemo_mapEnemySkillPrototypeValues::RangedCooldown;
		Result.RangedBackstepShot.bLockDirectionAtEndOfWindup = false;
		Result.RangedBackstepShot.bDirectionAwayFromTarget = true;
		Result.RangedBackstepShot.bAppliesContactDamage = false;
		Result.RangedBackstepShot.bStopsOnFirstLegalHit = false;

		Result.EnhancedMeleeDash.Kind = Edemo_mapEnemySkillKind::MeleeDash;
		Result.EnhancedMeleeDash.TriggerMinDistance = 240.0f;
		Result.EnhancedMeleeDash.TriggerMaxDistance = 720.0f;
		Result.EnhancedMeleeDash.WindupDuration = 0.18f;
		Result.EnhancedMeleeDash.DisplacementDistance = 520.0f;
		Result.EnhancedMeleeDash.DisplacementDuration = 0.30f;
		Result.EnhancedMeleeDash.MinimumResolvedDistance = 140.0f;
		Result.EnhancedMeleeDash.RecoveryDuration = 0.24f;
		Result.EnhancedMeleeDash.CooldownDuration = 2.20f;
		Result.EnhancedMeleeDash.bLockDirectionAtEndOfWindup = true;
		Result.EnhancedMeleeDash.bDirectionAwayFromTarget = false;
		Result.EnhancedMeleeDash.bAppliesContactDamage = true;
		Result.EnhancedMeleeDash.bStopsOnFirstLegalHit = true;

		Result.EnhancedRangedBackstepShot.Kind =
			Edemo_mapEnemySkillKind::RangedBackstepShot;
		Result.EnhancedRangedBackstepShot.TriggerMinDistance = 0.0f;
		Result.EnhancedRangedBackstepShot.TriggerMaxDistance = 560.0f;
		Result.EnhancedRangedBackstepShot.WindupDuration = 0.14f;
		Result.EnhancedRangedBackstepShot.DisplacementDistance = 380.0f;
		Result.EnhancedRangedBackstepShot.DisplacementDuration = 0.26f;
		Result.EnhancedRangedBackstepShot.MinimumResolvedDistance = 140.0f;
		Result.EnhancedRangedBackstepShot.RecoveryDuration = 0.18f;
		Result.EnhancedRangedBackstepShot.CooldownDuration = 2.00f;
		Result.EnhancedRangedBackstepShot.bLockDirectionAtEndOfWindup = false;
		Result.EnhancedRangedBackstepShot.bDirectionAwayFromTarget = true;
		Result.EnhancedRangedBackstepShot.bAppliesContactDamage = false;
		Result.EnhancedRangedBackstepShot.bStopsOnFirstLegalHit = false;

		Result.KnockbackDistance =
			Fdemo_mapEnemySkillPrototypeValues::KnockbackDistance;
		Result.KnockbackDuration =
			Fdemo_mapEnemySkillPrototypeValues::KnockbackDuration;
		Result.WorldStaticSkin =
			Fdemo_mapEnemySkillPrototypeValues::WorldStaticSkin;
		return Result;
	}();
	return Config;
}

const Fdemo_mapEnemySkillDefinition*
Fdemo_mapEnemySkillPrototypeConfig::FindProfile(FName ProfileId)
{
	const Fdemo_mapEnemySkillPrototypeConfig& Config = Get();
	if (ProfileId == Fdemo_mapEnemySkillProfileIds::StandardMeleeDash)
	{
		return &Config.MeleeDash;
	}
	if (ProfileId == Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep)
	{
		return &Config.RangedBackstepShot;
	}
	if (ProfileId == Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash)
	{
		return &Config.EnhancedMeleeDash;
	}
	if (ProfileId == Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep)
	{
		return &Config.EnhancedRangedBackstepShot;
	}
	return nullptr;
}

bool IsLegalEnemySkillPhaseTransition(
	Edemo_mapEnemySkillPhase From,
	Edemo_mapEnemySkillPhase To)
{
	if (To == Edemo_mapEnemySkillPhase::Idle)
	{
		return From != Edemo_mapEnemySkillPhase::Idle;
	}
	switch (From)
	{
	case Edemo_mapEnemySkillPhase::Idle:
		return To == Edemo_mapEnemySkillPhase::Windup;
	case Edemo_mapEnemySkillPhase::Windup:
		return To == Edemo_mapEnemySkillPhase::Displacing
			|| To == Edemo_mapEnemySkillPhase::Recovery;
	case Edemo_mapEnemySkillPhase::Displacing:
		return To == Edemo_mapEnemySkillPhase::Resolving
			|| To == Edemo_mapEnemySkillPhase::Recovery;
	case Edemo_mapEnemySkillPhase::Resolving:
		return To == Edemo_mapEnemySkillPhase::Recovery;
	case Edemo_mapEnemySkillPhase::Recovery:
	default:
		return false;
	}
}

bool ShouldRequestEnemySkillKnockback(
	float AppliedDamage,
	bool bTargetDefeated)
{
	return FMath::IsFinite(AppliedDamage)
		&& AppliedDamage > 0.0f
		&& !bTargetDefeated;
}
