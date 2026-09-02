#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShanmenVitalityAuthority.h"
#include "demo_mapAttributeTypes.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"
#include "demo_mapShanmenCombatConditionStatus.h"

#include "demo_mapShanmenCombatConditionComponent.generated.h"

class Udemo_mapAttributeComponent;

/** Outcome of projecting one committed vitality receipt into a Run condition. */
enum class Edemo_mapShanmenCombatConditionApplicationStatus : uint8
{
	Applied,
	Refreshed,
	AlreadyApplied,
	Rejected
};

/** Fail-closed reasons at the committed-impact -> condition boundary. */
enum class Edemo_mapShanmenCombatConditionError : uint8
{
	None,
	ComponentNotReady,
	InvalidReceipt,
	TargetMismatch,
	TimelineMismatch,
	StaleTimeline,
	NoCommittedDamage,
	ImpactConflict,
	RevisionExhausted,
	ModifierRejected
};

/** Immutable proof that one exact committed Impact authored Meridian Shock. */
struct Fdemo_mapShanmenCombatConditionApplicationReceipt
{
public:
	bool IsValid() const;
	const FGuid& GetApplicationId() const { return ApplicationId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	const FGuid& GetImpactId() const { return ImpactId; }
	const FGuid& GetResolutionId() const { return ResolutionId; }
	FName GetDefinitionId() const { return DefinitionId; }
	int64 GetAppliedAtTick() const { return AppliedAtTick; }
	int64 GetExpiryTick() const { return ExpiryTick; }
	int64 GetConditionRevision() const { return ConditionRevision; }

private:
	friend class Udemo_mapShanmenCombatConditionComponent;
	FGuid ApplicationId;
	FGuid RunId;
	FGuid TargetEntityId;
	FGuid TimelineId;
	FGuid ImpactId;
	FGuid ResolutionId;
	FName DefinitionId = NAME_None;
	int64 AppliedAtTick = INDEX_NONE;
	int64 ExpiryTick = INDEX_NONE;
	int64 ConditionRevision = INDEX_NONE;
};

struct Fdemo_mapShanmenCombatConditionApplicationResult
{
	Edemo_mapShanmenCombatConditionApplicationStatus Status =
		Edemo_mapShanmenCombatConditionApplicationStatus::Rejected;
	Edemo_mapShanmenCombatConditionError Error =
		Edemo_mapShanmenCombatConditionError::ComponentNotReady;
	Fdemo_mapShanmenCombatConditionApplicationReceipt Receipt;

	bool IsSuccess() const
	{
		return Error == Edemo_mapShanmenCombatConditionError::None
			&& Status
				!= Edemo_mapShanmenCombatConditionApplicationStatus::Rejected
			&& Receipt.IsValid();
	}
};

enum class Edemo_mapShanmenCombatConditionAdvanceStatus : uint8
{
	Observed,
	Expired,
	Rejected
};

struct Fdemo_mapShanmenCombatConditionAdvanceResult
{
	Edemo_mapShanmenCombatConditionAdvanceStatus Status =
		Edemo_mapShanmenCombatConditionAdvanceStatus::Rejected;
	Edemo_mapShanmenCombatConditionError Error =
		Edemo_mapShanmenCombatConditionError::ComponentNotReady;
	int64 ObservedTick = INDEX_NONE;
	int64 ConditionRevision = INDEX_NONE;

	bool IsSuccess() const
	{
		return Error == Edemo_mapShanmenCombatConditionError::None
			&& Status != Edemo_mapShanmenCombatConditionAdvanceStatus::Rejected;
	}
};

/** Immutable item-authored request to clear one exact active condition revision. */
struct Fdemo_mapShanmenCombatConditionTreatmentIntent
{
public:
	static bool TryCapture(
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		const FGuid& ItemInstanceId,
		FName ItemDefinitionId,
		int64 ExpectedConditionRevision,
		Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent);

	bool IsValid() const;
	const FGuid& GetTreatmentId() const { return TreatmentId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	FName GetItemDefinitionId() const { return ItemDefinitionId; }
	FName GetConditionDefinitionId() const { return ConditionDefinitionId; }
	int64 GetExpectedConditionRevision() const
	{
		return ExpectedConditionRevision;
	}

private:
	FGuid TreatmentId;
	FGuid RunId;
	FGuid TargetEntityId;
	FGuid TimelineId;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FName ConditionDefinitionId = NAME_None;
	int64 ExpectedConditionRevision = INDEX_NONE;
};

enum class Edemo_mapShanmenCombatConditionTreatmentStatus : uint8
{
	Treated,
	AlreadyTreated,
	Rejected
};

enum class Edemo_mapShanmenCombatConditionTreatmentError : uint8
{
	None,
	ComponentNotReady,
	InvalidIntent,
	RunMismatch,
	TargetMismatch,
	TimelineMismatch,
	StaleTimeline,
	ConditionInactive,
	ConditionRevisionMismatch,
	TreatmentConflict,
	RevisionExhausted,
	ModifierRejected
};

/** Immutable proof that one exact item intent cleared Meridian Shock. */
struct Fdemo_mapShanmenCombatConditionTreatmentReceipt
{
public:
	bool IsValid() const;
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
	bool Matches(
		const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent) const;

private:
	friend class Udemo_mapShanmenCombatConditionComponent;
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

struct Fdemo_mapShanmenCombatConditionTreatmentResult
{
	Edemo_mapShanmenCombatConditionTreatmentStatus Status =
		Edemo_mapShanmenCombatConditionTreatmentStatus::Rejected;
	Edemo_mapShanmenCombatConditionTreatmentError Error =
		Edemo_mapShanmenCombatConditionTreatmentError::ComponentNotReady;
	Fdemo_mapShanmenCombatConditionTreatmentReceipt Receipt;

	bool IsSuccess() const
	{
		return Error == Edemo_mapShanmenCombatConditionTreatmentError::None
			&& Status
				!= Edemo_mapShanmenCombatConditionTreatmentStatus::Rejected
			&& Receipt.IsValid();
	}
};

/**
 * Run-scoped product condition authority for the first concrete 0.0.10 injury.
 *
 * P14.0 deliberately closes one real vertical slice: a successfully committed
 * Boss Charge applies Meridian Shock for 90 canonical 30 Hz ticks and projects
 * a single exact MoveSpeed x0.75 modifier through the existing attribute
 * authority. It owns no damage, vitality, wall clock, Actor tick, or inventory
 * truth. Exact Impact replay is idempotent and Run teardown removes the exact
 * modifier before state is discarded.
 */
UCLASS(ClassGroup=(Gameplay))
class Udemo_mapShanmenCombatConditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapShanmenCombatConditionComponent();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	static FName MeridianShockDefinitionId();
	static FName MeridianShockModifierSourceId();
	static int64 MeridianShockDurationTicks() { return 90; }
	static float MeridianShockMoveSpeedMultiplier() { return 0.75f; }
	static FGuid MakeApplicationId(
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		const FGuid& ImpactId,
		const FGuid& ResolutionId);
	static FGuid MakeTreatmentId(
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		const FGuid& ItemInstanceId,
		FName ItemDefinitionId,
		int64 ExpectedConditionRevision);
	static Fdemo_mapModifierHandle MakeMeridianShockModifierHandle(
		const FGuid& RunId,
		const FGuid& TargetEntityId);

	bool TryBegin(
		const FGuid& RunId,
		const FGuid& TargetEntityId,
		const FGuid& TimelineId,
		Udemo_mapAttributeComponent* AttributeComponent,
		FString& OutDiagnostic);
	Fdemo_mapShanmenCombatConditionApplicationResult TryApplyMeridianShock(
		const FShanmenVitalityCommitReceipt& VitalityReceipt,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample);
	Fdemo_mapShanmenCombatConditionAdvanceResult TryAdvance(
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample);
	Fdemo_mapShanmenCombatConditionTreatmentResult TryTreatMeridianShock(
		const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample);
	/** Copies the current immutable state for Blueprint/presentation polling. */
	bool TryCaptureMeridianShockStatus(
		Fdemo_mapShanmenCombatConditionStatusSnapshot& OutStatus) const;
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	/** Recovery-only cleanup. It best-effort removes the exact owned modifier. */
	void Reset();

	bool IsValid() const;
	bool IsEmpty() const;
	bool IsMeridianShockActive() const { return bMeridianShockActive; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetLastObservedTick() const { return LastObservedTick; }
	int64 GetMeridianShockExpiryTick() const { return MeridianShockExpiryTick; }
	int64 GetConditionRevision() const { return ConditionRevision; }
	int32 NumProcessedApplications() const
	{
		return ProcessedApplications.Num();
	}
	int32 NumProcessedTreatments() const
	{
		return ProcessedTreatments.Num();
	}

private:
	struct FProcessedApplication
	{
		FGuid ResolutionId;
		Fdemo_mapShanmenCombatConditionApplicationReceipt Receipt;
	};
	struct FProcessedTreatment
	{
		Fdemo_mapShanmenCombatConditionTreatmentIntent Intent;
		Fdemo_mapShanmenCombatConditionTreatmentReceipt Receipt;
	};

	static Fdemo_mapModifierSpec MakeMeridianShockModifierSpec();
	void ClearState();

	FGuid RunId;
	FGuid TargetEntityId;
	FGuid TimelineId;
	TWeakObjectPtr<Udemo_mapAttributeComponent> AttributeComponent;
	Fdemo_mapModifierHandle MeridianShockModifierHandle;
	int64 LastObservedTick = INDEX_NONE;
	int64 MeridianShockExpiryTick = INDEX_NONE;
	int64 ConditionRevision = 0;
	bool bMeridianShockActive = false;
	TMap<FGuid, FProcessedApplication> ProcessedApplications;
	TMap<FGuid, FProcessedTreatment> ProcessedTreatments;
};
