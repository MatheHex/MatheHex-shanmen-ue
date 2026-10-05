#pragma once
#include "CoreMinimal.h"
#include "ShanmenDemo20Session.h"

enum class EShanmenDemo20MedicineOrigin : uint8 { PreparedCarry, Secure, StoredCarry };
/** Exact operation input, not a second quantity balance. The item ledger owns consumption. */
struct FShanmenDemo20MedicineIntent
{
	FGuid ItemId;
	int32 ExpectedQuantity = 0;
	int64 ExpectedItemRevision = 0;
	EShanmenDemo20MedicineOrigin Origin = EShanmenDemo20MedicineOrigin::PreparedCarry;
	bool IsSet() const { return ItemId.IsValid(); }
};

/** Product-world state, never an inventory/quantity authority. Fixed layout, versioned encounter content. */
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
	FShanmenDemo20MedicineIntent Medicine;
	bool IsValid() const;
	static FGuid CurrentContentId();
	static FGuid LegacyContentId();
	static uint64 SeedForRun(const FGuid& Run);
};

/** Same-volume verified-temp/atomic-replace protocol used by existing native stores.
 * A bounded high-water witness admits an exact next candidate before replacement;
 * an identical replica permits explicit repair without selecting an older backup.
 * Legacy files remain readable, but corrupt legacy data has no invented recovery.
 * Each file is one Run, with CAS generation. No item schema or quantity is duplicated. */
class FShanmenDemo20WorldCheckpointStore
{
public:
	static FString Path(const FString& ProfileRoot, const FGuid& Run);
	static bool Load(const FString& ProfileRoot, const FGuid& Run, FShanmenDemo20WorldCheckpoint& Out, FString& Reason);
	/** Explicitly finish/repair ONLY the candidate named by the durable witness.
	 * Damaged primary bytes are preserved; failure leaves Out unchanged. */
	static bool Recover(const FString& ProfileRoot, const FGuid& Run, FShanmenDemo20WorldCheckpoint& Out, FString& Reason);
	static bool Save(const FString& ProfileRoot, FShanmenDemo20WorldCheckpoint& InOutConfirmed,
		const FShanmenDemo20WorldCheckpoint& Candidate, FString& Reason);
#if WITH_DEV_AUTOMATION_TESTS
	static bool bFailBeforeReplace;
	static bool bFailFirstReadAfterReplace;
	static bool bFailAfterWitnessBeforeReplace;
	static bool bFailReplicaWrite;
	static bool bFailBeforeRepairReplace;
#endif
};
