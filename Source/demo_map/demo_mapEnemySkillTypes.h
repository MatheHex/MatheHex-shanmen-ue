#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapEnemySkillKind : uint8
{
	None,
	MeleeDash,
	RangedBackstepShot
};

enum class Edemo_mapEnemySkillPhase : uint8
{
	Idle,
	Windup,
	Displacing,
	Resolving,
	Recovery
};

enum class Edemo_mapEnemySkillStartStatus : uint8
{
	Accepted,
	InvalidDefinition,
	Busy,
	Cooldown,
	InvalidTarget,
	OutsideTriggerRange,
	InvalidDirection,
	InsufficientResolvedDistance
};

/** The sole source of truth for the P6 prototype numbers. */
struct Fdemo_mapEnemySkillPrototypeValues
{
	static constexpr float RangedTriggerMax = 500.0f;
	static constexpr float RangedWindup = 0.18f;
	static constexpr float RangedDistance = 320.0f;
	static constexpr float RangedDisplacementDuration = 0.28f;
	static constexpr float RangedMinimumResolvedDistance = 120.0f;
	static constexpr float RangedRecovery = 0.22f;
	static constexpr float RangedCooldown = 2.40f;

	static constexpr float MeleeTriggerMin = 220.0f;
	static constexpr float MeleeTriggerMax = 650.0f;
	static constexpr float MeleeWindup = 0.22f;
	static constexpr float MeleeDistance = 440.0f;
	static constexpr float MeleeDisplacementDuration = 0.32f;
	static constexpr float MeleeMinimumResolvedDistance = 120.0f;
	static constexpr float MeleeRecovery = 0.30f;
	static constexpr float MeleeCooldown = 2.60f;

	static constexpr float KnockbackDistance = 140.0f;
	static constexpr float KnockbackDuration = 0.18f;
	static constexpr float WorldStaticSkin = 2.0f;
};

struct Fdemo_mapEnemySkillProfileIds
{
	static const FName StandardMeleeDash;
	static const FName StandardRangedBackstep;
	static const FName EnhancedMeleeDash;
	static const FName EnhancedRangedBackstep;
};

struct Fdemo_mapEnemySkillDefinition
{
	Edemo_mapEnemySkillKind Kind = Edemo_mapEnemySkillKind::None;
	float TriggerMinDistance = 0.0f;
	float TriggerMaxDistance = 0.0f;
	float WindupDuration = 0.0f;
	float DisplacementDistance = 0.0f;
	float DisplacementDuration = 0.0f;
	float MinimumResolvedDistance = 0.0f;
	float RecoveryDuration = 0.0f;
	float CooldownDuration = 0.0f;
	bool bLockDirectionAtEndOfWindup = false;
	bool bDirectionAwayFromTarget = false;
	bool bAppliesContactDamage = false;
	bool bStopsOnFirstLegalHit = false;

	bool IsValid() const;
	bool IsInsideTriggerRange(float Distance) const;
	bool CanResolveRangedFire(
		float ActualResolvedDistance,
		bool bTargetValid,
		bool bHasWorldStaticLineOfSight) const;
};

struct Fdemo_mapEnemySkillPrototypeConfig
{
	Fdemo_mapEnemySkillDefinition MeleeDash;
	Fdemo_mapEnemySkillDefinition RangedBackstepShot;
	Fdemo_mapEnemySkillDefinition EnhancedMeleeDash;
	Fdemo_mapEnemySkillDefinition EnhancedRangedBackstepShot;
	float KnockbackDistance = 0.0f;
	float KnockbackDuration = 0.0f;
	float WorldStaticSkin = 0.0f;

	static const Fdemo_mapEnemySkillPrototypeConfig& Get();
	static const Fdemo_mapEnemySkillDefinition* FindProfile(FName ProfileId);
};

struct Fdemo_mapEnemySkillActivationIntent
{
	Fdemo_mapEnemySkillDefinition Definition;
	TWeakObjectPtr<AActor> Target;
	FVector InitialPlanarDirection = FVector::ZeroVector;
	float CurrentTargetDistance = 0.0f;
};

struct Fdemo_mapEnemySkillStartResult
{
	Edemo_mapEnemySkillStartStatus Status =
		Edemo_mapEnemySkillStartStatus::InvalidDefinition;
	float ResolvedPreflightDistance = 0.0f;

	bool IsAccepted() const
	{
		return Status == Edemo_mapEnemySkillStartStatus::Accepted;
	}
};

struct Fdemo_mapEnemySkillRuntimeSnapshot
{
	Edemo_mapEnemySkillKind Kind = Edemo_mapEnemySkillKind::None;
	Edemo_mapEnemySkillPhase Phase = Edemo_mapEnemySkillPhase::Idle;
	FVector LockedPlanarDirection = FVector::ZeroVector;
	float ResolvedPreflightDistance = 0.0f;
	float ResolvedDisplacementDistance = 0.0f;
	float CooldownRemaining = 0.0f;
	uint32 ActivationSerial = 0;
	bool bFirstLegalHitConsumed = false;
};

struct Fdemo_mapEnemySkillDisplacementSegment
{
	Edemo_mapEnemySkillKind Kind = Edemo_mapEnemySkillKind::None;
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FHitResult BlockingHit;
};

struct Fdemo_mapKnockbackIntent
{
	FVector PlanarDirection = FVector::ZeroVector;
	float Distance = 0.0f;
	float Duration = 0.0f;
};

bool IsLegalEnemySkillPhaseTransition(
	Edemo_mapEnemySkillPhase From,
	Edemo_mapEnemySkillPhase To);
bool ShouldRequestEnemySkillKnockback(
	int32 AppliedDamage,
	bool bTargetDefeated);
