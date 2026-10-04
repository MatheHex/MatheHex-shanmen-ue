#pragma once
#include "CoreMinimal.h"
#include "ShanmenDemo20Session.h"

/** Product-world state, never an inventory/quantity authority. Fixed layout revision 1. */
struct FShanmenDemo20WorldCheckpoint
{
	int32 Generation = 0;
	FGuid ContentId;
	uint64 RunSeed = 0;
	FShanmenDemo20CombatCheckpoint Combat;
	FVector PlayerPosition = FVector(-760.f, 0.f, 90.f);
	float PlayerYaw = 0.f;
	FVector EnemyPositions[3] = {FVector(1000,-350,65), FVector(3100,200,65), FVector(5200,-300,80)};
	FVector WarningTargets[3] = {FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector};
	float EnemyClocks[3] = {0.f,0.f,0.f};
	bool IsValid() const;
	static FGuid CurrentContentId();
	static uint64 SeedForRun(const FGuid& Run);
};

/** Same-volume verified-temp/atomic-replace protocol used by existing native stores.
 * A stale backup is NOT automatically substituted: doing so could resurrect a confirmed enemy.
 * Each file is one Run, with CAS generation. No item schema or quantity is duplicated. */
class FShanmenDemo20WorldCheckpointStore
{
public:
	static FString Path(const FString& ProfileRoot, const FGuid& Run);
	static bool Load(const FString& ProfileRoot, const FGuid& Run, FShanmenDemo20WorldCheckpoint& Out, FString& Reason);
	static bool Save(const FString& ProfileRoot, FShanmenDemo20WorldCheckpoint& InOutConfirmed,
		const FShanmenDemo20WorldCheckpoint& Candidate, FString& Reason);
#if WITH_DEV_AUTOMATION_TESTS
	static bool bFailBeforeReplace;
	static bool bFailFirstReadAfterReplace;
#endif
};
