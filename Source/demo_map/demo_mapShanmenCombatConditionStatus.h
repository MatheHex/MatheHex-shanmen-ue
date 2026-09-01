#pragma once

#include "CoreMinimal.h"

#include "demo_mapShanmenCombatConditionStatus.generated.h"

class Udemo_mapShanmenCombatConditionComponent;

/**
 * Immutable Blueprint-readable view of the Run-scoped Meridian Shock state.
 *
 * It owns no condition lifecycle or modifier authority. Repeated capture of
 * identical authority state produces the same StatusId; fixed-timeline
 * progress produces a new exact snapshot without mutating prior copies.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenCombatConditionStatusSnapshot
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& Other) const;

	const FGuid& GetStatusId() const { return StatusId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	FName GetDefinitionId() const { return DefinitionId; }
	int64 GetObservedTick() const { return ObservedTick; }
	int64 GetExpiryTick() const { return ExpiryTick; }
	int64 GetRemainingTicks() const { return RemainingTicks; }
	int64 GetConditionRevision() const { return ConditionRevision; }
	int64 GetDurationTicks() const { return DurationTicks; }
	int64 GetTimelineTicksPerSecond() const
	{
		return TimelineTicksPerSecond;
	}
	float GetMoveSpeedMultiplier() const { return MoveSpeedMultiplier; }
	bool IsActive() const { return bActive; }

private:
	friend class Udemo_mapShanmenCombatConditionComponent;

	static bool TryCapture(
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		int64 ObservedTick,
		int64 ExpiryTick,
		int64 ConditionRevision,
		bool bActive,
		Fdemo_mapShanmenCombatConditionStatusSnapshot& OutStatus);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	FGuid StatusId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	FGuid TargetEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	FName DefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	int64 ObservedTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	int64 ExpiryTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	int64 RemainingTicks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	int64 ConditionRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	int64 DurationTicks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	int64 TimelineTicksPerSecond = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	float MoveSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Conditions|Status", meta = (AllowPrivateAccess = "true"))
	bool bActive = false;
};
