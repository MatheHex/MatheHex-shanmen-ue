#include "demo_mapEnemyEncounterTypes.h"

bool Fdemo_mapEnemyCombatTuning::IsValid() const
{
	return MaxHealth > 0
		&& FMath::IsFinite(MovementSpeed)
		&& MovementSpeed > 0.0f
		&& FMath::IsFinite(AttackDamage)
		&& AttackDamage > 0.0f
		&& FMath::IsFinite(AttackWindup)
		&& AttackWindup >= 0.0f
		&& FMath::IsFinite(AttackCooldown)
		&& AttackCooldown > 0.0f;
}

bool Fdemo_mapEnemyEncounterSpawnRecord::IsValid() const
{
	return Identity.IsValid()
		&& Tuning.IsValid()
		&& !SourceMarkerType.IsNone()
		&& (Archetype != Edemo_mapEnemyEncounterArchetype::MeleeHeavy
			|| Identity.SkillProfileId.IsNone())
		&& (Archetype == Edemo_mapEnemyEncounterArchetype::MeleeHeavy
			|| !Identity.SkillProfileId.IsNone());
}
