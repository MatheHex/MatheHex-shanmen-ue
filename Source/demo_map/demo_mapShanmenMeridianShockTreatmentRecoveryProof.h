#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenCombatConditionComponent.h"

/**
 * Portable, versioned proof that one exact Meridian Shock treatment committed.
 *
 * This value owns no inventory or active-condition truth. It is the canonical
 * condition-domain payload a future storage owner may persist atomically. The
 * strict codec rejects unknown schemas, non-canonical values and integrity-ID
 * mismatches before a product recovery path can consume the receipt.
 */
struct Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof
{
public:
	static constexpr int32 CurrentSchemaVersion = 1;

	static bool TryCapture(
		const Fdemo_mapShanmenCombatConditionTreatmentReceipt& Receipt,
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& OutProof);
	static bool TryDecode(
		const FString& Encoded,
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& OutProof);

	bool TryEncode(FString& OutEncoded) const;
	bool TryRestore(
		Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent,
		Fdemo_mapShanmenCombatConditionTreatmentReceipt& OutReceipt) const;
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Other) const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	const FGuid& GetProofId() const { return ProofId; }
	const FGuid& GetTreatmentId() const { return TreatmentId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	FName GetItemDefinitionId() const { return ItemDefinitionId; }
	FName GetConditionDefinitionId() const { return ConditionDefinitionId; }
	int64 GetTreatedAtTick() const { return TreatedAtTick; }
	int64 GetConditionRevisionBefore() const
	{
		return ConditionRevisionBefore;
	}
	int64 GetConditionRevisionAfter() const
	{
		return ConditionRevisionAfter;
	}

private:
	static FGuid MakeProofId(
		int32 SchemaVersion,
		const FGuid& TreatmentId,
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		const FGuid& ItemInstanceId,
		FName ItemDefinitionId,
		FName ConditionDefinitionId,
		int64 TreatedAtTick,
		int64 ConditionRevisionBefore,
		int64 ConditionRevisionAfter);

	int32 SchemaVersion = 0;
	FGuid ProofId;
	FGuid TreatmentId;
	FGuid RunId;
	FGuid TargetEntityId;
	FGuid TimelineId;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FName ConditionDefinitionId = NAME_None;
	int64 TreatedAtTick = INDEX_NONE;
	int64 ConditionRevisionBefore = INDEX_NONE;
	int64 ConditionRevisionAfter = INDEX_NONE;
};
