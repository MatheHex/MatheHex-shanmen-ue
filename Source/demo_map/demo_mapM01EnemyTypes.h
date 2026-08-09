#pragma once

#include "CoreMinimal.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapM01EnemyTypes.generated.h"

UENUM(BlueprintType)
enum class Edemo_mapM01EnemyArchetype : uint8
{
	StandardSkirmisher,
	StandardRanged,
	StandardBruiser,
	EliteStalker,
	EliteBulwark,
	BossMain
};

UENUM(BlueprintType)
enum class Edemo_mapM01BossAttack : uint8
{
	Sweep,
	Charge,
	Volley
};

/** Immutable product identity and combat projection for one M01 spawn. */
struct Fdemo_mapM01EnemyDefinition
{
	Edemo_mapM01EnemyArchetype Archetype =
		Edemo_mapM01EnemyArchetype::StandardSkirmisher;
	FName EncounterId = NAME_None;
	FName ParentMarkerId = NAME_None;
	FName SpawnMarkerId = NAME_None;
	FName RouteId = NAME_None;
	FName EnemyArchetypeId = NAME_None;
	FName RewardSourceRoleId = NAME_None;
	FName CorpseIdentity = NAME_None;
	FName RiskTierId = NAME_None;
	FName SkillProfileId = NAME_None;
	FVector LocalOffset = FVector::ZeroVector;
	Fdemo_mapEnemyCombatTuning Tuning;

	bool IsElite() const;
	bool IsBoss() const;
	bool IsValid() const;
};

struct Fdemo_mapM01EnemyConfig
{
	static const TArray<Fdemo_mapM01EnemyDefinition>& GetDefinitions();
	static bool Validate(FString* OutError = nullptr);
};

/** Small per-Run authority preventing duplicate encounter and Boss commits. */
struct Fdemo_mapM01RunEnemyLedger
{
	void ResetForNewRun(FGuid InRunId);
	bool TryClaimEncounter(FName EncounterId);
	bool TryCommitBossDeath(FName BossId);
	void MarkTerminal();
	bool IsActive() const { return bActive; }
	int32 GetClaimedEncounterCount() const { return ClaimedEncounterIds.Num(); }
	FGuid GetRunId() const { return RunId; }

private:
	FGuid RunId;
	TSet<FName> ClaimedEncounterIds;
	bool bBossDeathCommitted = false;
	bool bActive = false;
};

struct Fdemo_mapM01BossCombatPlanner
{
	static Edemo_mapM01BossAttack SelectAttack(float Distance);
};

