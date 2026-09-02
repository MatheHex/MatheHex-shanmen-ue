#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"

/**
 * Portable, versioned write-ahead decision for one Meridian Shock treatment.
 *
 * This value is captured only after the item prepare and condition revision
 * are known, but before the condition mutation. It is not proof that treatment
 * committed. A future durable store may use it to choose the same treatment
 * outcome after process loss without copying inventory or condition truth.
 */
struct Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent
{
public:
	static constexpr int32 CurrentSchemaVersion = 1;

	static bool TryCapture(
		const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& OutIntent);
	static bool TryDecode(
		const FString& Encoded,
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& OutIntent);

	bool TryEncode(FString& OutEncoded) const;
	bool TryRestore(
		Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent,
		Fdemo_mapShanmenCombatRunTimelineSample& OutTimelineSample) const;
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Other)
		const;

	int32 GetSchemaVersion() const { return SchemaVersion; }
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetTreatmentId() const { return TreatmentId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	const FGuid& GetTimelineSampleId() const { return TimelineSampleId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	FName GetItemDefinitionId() const { return ItemDefinitionId; }
	FName GetConditionDefinitionId() const { return ConditionDefinitionId; }
	int64 GetTreatmentTick() const { return TreatmentTick; }
	int64 GetExpectedConditionRevision() const
	{
		return ExpectedConditionRevision;
	}

private:
	static FGuid MakeIntentId(
		int32 SchemaVersion,
		const FGuid& TreatmentId,
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		const FGuid& TimelineSampleId,
		const FGuid& ItemInstanceId,
		FName ItemDefinitionId,
		FName ConditionDefinitionId,
		int64 TreatmentTick,
		int64 ExpectedConditionRevision);

	int32 SchemaVersion = 0;
	FGuid IntentId;
	FGuid TreatmentId;
	FGuid RunId;
	FGuid TargetEntityId;
	FGuid TimelineId;
	FGuid TimelineSampleId;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FName ConditionDefinitionId = NAME_None;
	int64 TreatmentTick = INDEX_NONE;
	int64 ExpectedConditionRevision = INDEX_NONE;
};
