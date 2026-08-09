#include "demo_mapM01EnemyTypes.h"

#include "demo_mapEnemySkillTypes.h"

namespace
{
	Fdemo_mapEnemyCombatTuning Melee(
		int32 Health,
		float Speed,
		float Damage,
		float Cooldown)
	{
		Fdemo_mapEnemyCombatTuning Result;
		Result.MaxHealth = Health;
		Result.MovementSpeed = Speed;
		Result.AttackDamage = Damage;
		Result.AttackCooldown = Cooldown;
		return Result;
	}

	Fdemo_mapEnemyCombatTuning Heavy(
		int32 Health,
		float Speed,
		float Damage,
		float Windup,
		float Cooldown)
	{
		Fdemo_mapEnemyCombatTuning Result =
			Melee(Health, Speed, Damage, Cooldown);
		Result.AttackWindup = Windup;
		return Result;
	}

	Fdemo_mapEnemyCombatTuning Ranged(
		int32 Health,
		float Speed,
		float Damage,
		float Windup,
		float Cooldown)
	{
		Fdemo_mapEnemyCombatTuning Result =
			Melee(Health, Speed, Damage, Cooldown);
		Result.AttackWindup = Windup;
		Result.ProjectileWidth = 52.0f;
		Result.ProjectileCollisionRadius = 24.0f;
		Result.ProjectileSpeed = 880.0f;
		Result.ProjectileMaxDistance = 1900.0f;
		return Result;
	}

	Fdemo_mapM01EnemyDefinition Definition(
		Edemo_mapM01EnemyArchetype Archetype,
		const TCHAR* Encounter,
		const TCHAR* ParentMarker,
		const TCHAR* SpawnMarker,
		const TCHAR* Route,
		const TCHAR* ArchetypeId,
		const TCHAR* SourceRole,
		const TCHAR* Corpse,
		const TCHAR* Risk,
		FName SkillProfile,
		const FVector& Offset,
		const Fdemo_mapEnemyCombatTuning& Tuning)
	{
		Fdemo_mapM01EnemyDefinition Result;
		Result.Archetype = Archetype;
		Result.EncounterId = Encounter;
		Result.ParentMarkerId = ParentMarker;
		Result.SpawnMarkerId = SpawnMarker;
		Result.RouteId = Route;
		Result.EnemyArchetypeId = ArchetypeId;
		Result.RewardSourceRoleId = SourceRole;
		Result.CorpseIdentity = Corpse;
		Result.RiskTierId = Risk;
		Result.SkillProfileId = SkillProfile;
		Result.LocalOffset = Offset;
		Result.Tuning = Tuning;
		return Result;
	}
}

bool Fdemo_mapM01EnemyDefinition::IsElite() const
{
	return Archetype == Edemo_mapM01EnemyArchetype::EliteStalker
		|| Archetype == Edemo_mapM01EnemyArchetype::EliteBulwark;
}

bool Fdemo_mapM01EnemyDefinition::IsBoss() const
{
	return Archetype == Edemo_mapM01EnemyArchetype::BossMain;
}

bool Fdemo_mapM01EnemyDefinition::IsValid() const
{
	return !EncounterId.IsNone()
		&& !ParentMarkerId.IsNone()
		&& !SpawnMarkerId.IsNone()
		&& !RouteId.IsNone()
		&& !EnemyArchetypeId.IsNone()
		&& !RewardSourceRoleId.IsNone()
		&& !CorpseIdentity.IsNone()
		&& (RiskTierId == TEXT("M01.Risk.LOW")
			|| RiskTierId == TEXT("M01.Risk.MID")
			|| RiskTierId == TEXT("M01.Risk.HIGH"))
		&& Tuning.IsValid()
		&& ((Archetype == Edemo_mapM01EnemyArchetype::StandardBruiser
				|| Archetype == Edemo_mapM01EnemyArchetype::EliteBulwark
				|| Archetype == Edemo_mapM01EnemyArchetype::BossMain)
			? SkillProfileId.IsNone()
			: !SkillProfileId.IsNone());
}

const TArray<Fdemo_mapM01EnemyDefinition>&
Fdemo_mapM01EnemyConfig::GetDefinitions()
{
	static const TCHAR* LowParent = TEXT("M01.Encounter.Normal.01");
	static const TCHAR* MidParent = TEXT("M01.Encounter.Normal.02");
	static const TCHAR* HighParent = TEXT("M01.Encounter.Elite.01");
	static const TCHAR* BossParent = TEXT("M01.Boss.Main");
	static const TArray<Fdemo_mapM01EnemyDefinition> Definitions = {
		Definition(Edemo_mapM01EnemyArchetype::StandardSkirmisher,
			TEXT("M01.Encounter.LOW.Skirmisher.01"), LowParent,
			TEXT("M01.Spawn.LOW.Skirmisher.01"), TEXT("M01.Route.Low.North"),
			TEXT("M01.Enemy.Standard.Skirmisher"),
			TEXT("M01.SourceRole.Enemy.Standard.Skirmisher"),
			TEXT("M01.Corpse.Standard.Skirmisher"), TEXT("M01.Risk.LOW"),
			Fdemo_mapEnemySkillProfileIds::StandardMeleeDash,
			FVector(-260.0f, -220.0f, 0.0f), Melee(4, 315.0f, 1.0f, 1.10f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardSkirmisher,
			TEXT("M01.Encounter.LOW.Skirmisher.02"), LowParent,
			TEXT("M01.Spawn.LOW.Skirmisher.02"), TEXT("M01.Route.Low.North"),
			TEXT("M01.Enemy.Standard.Skirmisher"),
			TEXT("M01.SourceRole.Enemy.Standard.Skirmisher"),
			TEXT("M01.Corpse.Standard.Skirmisher"), TEXT("M01.Risk.LOW"),
			Fdemo_mapEnemySkillProfileIds::StandardMeleeDash,
			FVector(180.0f, 190.0f, 0.0f), Melee(4, 315.0f, 1.0f, 1.10f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardSkirmisher,
			TEXT("M01.Encounter.LOW.Skirmisher.03"), LowParent,
			TEXT("M01.Spawn.LOW.Skirmisher.03"), TEXT("M01.Route.Low.North"),
			TEXT("M01.Enemy.Standard.Skirmisher"),
			TEXT("M01.SourceRole.Enemy.Standard.Skirmisher"),
			TEXT("M01.Corpse.Standard.Skirmisher"), TEXT("M01.Risk.LOW"),
			Fdemo_mapEnemySkillProfileIds::StandardMeleeDash,
			FVector(300.0f, -170.0f, 0.0f), Melee(4, 315.0f, 1.0f, 1.10f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardRanged,
			TEXT("M01.Encounter.LOW.Ranged.01"), LowParent,
			TEXT("M01.Spawn.LOW.Ranged.01"), TEXT("M01.Route.Low.North"),
			TEXT("M01.Enemy.Standard.Ranged"),
			TEXT("M01.SourceRole.Enemy.Standard.Ranged"),
			TEXT("M01.Corpse.Standard.Ranged"), TEXT("M01.Risk.LOW"),
			Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep,
			FVector(-80.0f, 410.0f, 0.0f), Ranged(4, 250.0f, 1.0f, 0.50f, 1.75f)),

		Definition(Edemo_mapM01EnemyArchetype::StandardSkirmisher,
			TEXT("M01.Encounter.MID.Skirmisher.01"), MidParent,
			TEXT("M01.Spawn.MID.Skirmisher.01"), TEXT("M01.Route.Mid.Loop.South"),
			TEXT("M01.Enemy.Standard.Skirmisher"),
			TEXT("M01.SourceRole.Enemy.Standard.Skirmisher"),
			TEXT("M01.Corpse.Standard.Skirmisher"), TEXT("M01.Risk.MID"),
			Fdemo_mapEnemySkillProfileIds::StandardMeleeDash,
			FVector(-420.0f, 170.0f, 0.0f), Melee(5, 325.0f, 1.0f, 1.05f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardSkirmisher,
			TEXT("M01.Encounter.MID.Skirmisher.02"), MidParent,
			TEXT("M01.Spawn.MID.Skirmisher.02"), TEXT("M01.Route.Mid.Loop.South"),
			TEXT("M01.Enemy.Standard.Skirmisher"),
			TEXT("M01.SourceRole.Enemy.Standard.Skirmisher"),
			TEXT("M01.Corpse.Standard.Skirmisher"), TEXT("M01.Risk.MID"),
			Fdemo_mapEnemySkillProfileIds::StandardMeleeDash,
			FVector(-230.0f, -310.0f, 0.0f), Melee(5, 325.0f, 1.0f, 1.05f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardRanged,
			TEXT("M01.Encounter.MID.Ranged.01"), MidParent,
			TEXT("M01.Spawn.MID.Ranged.01"), TEXT("M01.Route.Mid.Loop.North"),
			TEXT("M01.Enemy.Standard.Ranged"),
			TEXT("M01.SourceRole.Enemy.Standard.Ranged"),
			TEXT("M01.Corpse.Standard.Ranged"), TEXT("M01.Risk.MID"),
			Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep,
			FVector(0.0f, 430.0f, 0.0f), Ranged(5, 260.0f, 1.0f, 0.45f, 1.60f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardRanged,
			TEXT("M01.Encounter.MID.Ranged.02"), MidParent,
			TEXT("M01.Spawn.MID.Ranged.02"), TEXT("M01.Route.Mid.Loop.South"),
			TEXT("M01.Enemy.Standard.Ranged"),
			TEXT("M01.SourceRole.Enemy.Standard.Ranged"),
			TEXT("M01.Corpse.Standard.Ranged"), TEXT("M01.Risk.MID"),
			Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep,
			FVector(190.0f, -390.0f, 0.0f), Ranged(5, 260.0f, 1.0f, 0.45f, 1.60f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardBruiser,
			TEXT("M01.Encounter.MID.Bruiser.01"), MidParent,
			TEXT("M01.Spawn.MID.Bruiser.01"), TEXT("M01.Route.Mid.Loop.North"),
			TEXT("M01.Enemy.Standard.Bruiser"),
			TEXT("M01.SourceRole.Enemy.Standard.Bruiser"),
			TEXT("M01.Corpse.Standard.Bruiser"), TEXT("M01.Risk.MID"), NAME_None,
			FVector(390.0f, 160.0f, 0.0f), Heavy(8, 195.0f, 2.0f, 0.85f, 2.25f)),
		Definition(Edemo_mapM01EnemyArchetype::StandardBruiser,
			TEXT("M01.Encounter.MID.Bruiser.02"), MidParent,
			TEXT("M01.Spawn.MID.Bruiser.02"), TEXT("M01.Route.Mid.Loop.South"),
			TEXT("M01.Enemy.Standard.Bruiser"),
			TEXT("M01.SourceRole.Enemy.Standard.Bruiser"),
			TEXT("M01.Corpse.Standard.Bruiser"), TEXT("M01.Risk.MID"), NAME_None,
			FVector(560.0f, -190.0f, 0.0f), Heavy(8, 195.0f, 2.0f, 0.85f, 2.25f)),

		Definition(Edemo_mapM01EnemyArchetype::EliteStalker,
			TEXT("M01.Encounter.HIGH.Stalker.01"), HighParent,
			TEXT("M01.Spawn.HIGH.Stalker.01"), TEXT("M01.Route.High.Core"),
			TEXT("M01.Enemy.Elite.Stalker"),
			TEXT("M01.SourceRole.Enemy.Elite.Stalker"),
			TEXT("M01.Corpse.Elite.Stalker"), TEXT("M01.Risk.HIGH"),
			Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash,
			FVector(-310.0f, 150.0f, 0.0f), Melee(11, 365.0f, 2.0f, 0.90f)),
		Definition(Edemo_mapM01EnemyArchetype::EliteStalker,
			TEXT("M01.Encounter.HIGH.Stalker.02"), HighParent,
			TEXT("M01.Spawn.HIGH.Stalker.02"), TEXT("M01.Route.High.Core"),
			TEXT("M01.Enemy.Elite.Stalker"),
			TEXT("M01.SourceRole.Enemy.Elite.Stalker"),
			TEXT("M01.Corpse.Elite.Stalker"), TEXT("M01.Risk.HIGH"),
			Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash,
			FVector(170.0f, -280.0f, 0.0f), Melee(11, 365.0f, 2.0f, 0.90f)),
		Definition(Edemo_mapM01EnemyArchetype::EliteBulwark,
			TEXT("M01.Encounter.HIGH.Bulwark.01"), HighParent,
			TEXT("M01.Spawn.HIGH.Bulwark.01"), TEXT("M01.Route.High.Core"),
			TEXT("M01.Enemy.Elite.Bulwark"),
			TEXT("M01.SourceRole.Enemy.Elite.Bulwark"),
			TEXT("M01.Corpse.Elite.Bulwark"), TEXT("M01.Risk.HIGH"), NAME_None,
			FVector(390.0f, 250.0f, 0.0f), Heavy(16, 220.0f, 3.0f, 1.0f, 2.45f)),

		Definition(Edemo_mapM01EnemyArchetype::BossMain,
			TEXT("M01.Encounter.Boss.Main"), BossParent,
			TEXT("M01.Spawn.Boss.Main"), TEXT("M01.Route.High.Core"),
			TEXT("M01.Boss.Main"), TEXT("M01.SourceRole.Enemy.Boss.Main"),
			TEXT("M01.Corpse.Boss.Main"), TEXT("M01.Risk.HIGH"), NAME_None,
			FVector::ZeroVector, Heavy(34, 235.0f, 3.0f, 0.75f, 2.20f))
	};
	return Definitions;
}

bool Fdemo_mapM01EnemyConfig::Validate(FString* OutError)
{
	const TArray<Fdemo_mapM01EnemyDefinition>& Definitions = GetDefinitions();
	TSet<FName> Encounters;
	TSet<FName> SpawnMarkers;
	TSet<FName> Archetypes;
	int32 StandardCount = 0;
	int32 EliteCount = 0;
	int32 BossCount = 0;
	for (const Fdemo_mapM01EnemyDefinition& DefinitionValue : Definitions)
	{
		if (!DefinitionValue.IsValid()
			|| Encounters.Contains(DefinitionValue.EncounterId)
			|| SpawnMarkers.Contains(DefinitionValue.SpawnMarkerId))
		{
			if (OutError) *OutError = TEXT("M01 enemy definition or stable identity is invalid or duplicated.");
			return false;
		}
		Encounters.Add(DefinitionValue.EncounterId);
		SpawnMarkers.Add(DefinitionValue.SpawnMarkerId);
		Archetypes.Add(DefinitionValue.EnemyArchetypeId);
		if (DefinitionValue.IsBoss()) ++BossCount;
		else if (DefinitionValue.IsElite()) ++EliteCount;
		else ++StandardCount;
	}
	if (Definitions.Num() != 14 || StandardCount != 10 || EliteCount != 3
		|| BossCount != 1 || Archetypes.Num() != 6)
	{
		if (OutError) *OutError = TEXT("M01 composition must contain 10 standard, 3 elite, 1 Boss, and six archetypes.");
		return false;
	}
	return true;
}

void Fdemo_mapM01RunEnemyLedger::ResetForNewRun(FGuid InRunId)
{
	RunId = InRunId.IsValid() ? InRunId : FGuid::NewGuid();
	ClaimedEncounterIds.Reset();
	bBossDeathCommitted = false;
	bActive = true;
}

bool Fdemo_mapM01RunEnemyLedger::TryClaimEncounter(FName EncounterId)
{
	if (!bActive || EncounterId.IsNone() || ClaimedEncounterIds.Contains(EncounterId))
	{
		return false;
	}
	ClaimedEncounterIds.Add(EncounterId);
	return true;
}

bool Fdemo_mapM01RunEnemyLedger::TryCommitBossDeath(FName BossId)
{
	if (!bActive || bBossDeathCommitted || BossId != TEXT("M01.Boss.Main"))
	{
		return false;
	}
	bBossDeathCommitted = true;
	return true;
}

void Fdemo_mapM01RunEnemyLedger::MarkTerminal()
{
	bActive = false;
}

Edemo_mapM01BossAttack Fdemo_mapM01BossCombatPlanner::SelectAttack(float Distance)
{
	if (Distance <= 290.0f) return Edemo_mapM01BossAttack::Sweep;
	if (Distance <= 780.0f) return Edemo_mapM01BossAttack::Charge;
	return Edemo_mapM01BossAttack::Volley;
}

