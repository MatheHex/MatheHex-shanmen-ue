#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenCombatConditionStatus.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenMeridianShockTreatmentStatus : uint8
{
	RequestReady,
	Prepared,
	Replayed,
	Committed,
	Cancelled,
	AuthorityNotReady,
	RunCorrelationInvalid,
	SnapshotUnavailable,
	SnapshotStale,
	ConditionStatusInvalid,
	ConditionMismatch,
	SourceItemMismatch,
	ItemNotFound,
	DefinitionNotTreatment,
	QuantityReservationInvalid,
	QuantityUnavailable,
	RequestInvalid,
	PrepareRejected,
	PreparationInvalid,
	TreatmentReceiptInvalid,
	TreatmentMismatch,
	FinalizeRejected
};

/** One exact condition-revision-to-item transaction and its durable evidence. */
struct Fdemo_mapShanmenMeridianShockTreatmentItemResult
{
	Edemo_mapShanmenMeridianShockTreatmentStatus Status =
		Edemo_mapShanmenMeridianShockTreatmentStatus::RequestInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenCombatConditionTreatmentIntent TreatmentIntent;
	FShanmenItemRunQuantityIntentRequest PrepareRequest;
	FShanmenItemRunQuantityIntentFinalizeRequest FinalizeRequest;
	FShanmenItemDurableCommandResult PrepareCommand;
	FShanmenItemDurableCommandResult FinalizeCommand;

	bool HasPrepareRequest() const;
	bool IsPrepared() const;
	bool IsFinalized() const;
};

/**
 * Product bridge from the canonical Meridian Shock treatment item to the
 * ShanmenItems active-Run Quantity authority.
 *
 * Prepare durably freezes one exact item balance before the condition changes.
 * Commit accepts only the matching immutable condition treatment receipt;
 * cancellation is legal only before treatment. No health, cooldown, legacy
 * inventory, UI, input, clock, or condition state is copied into this adapter.
 */
struct Fdemo_mapShanmenMeridianShockTreatmentAdapter
{
	/** Pure authority gate and deterministic prepare request builder. */
	static Fdemo_mapShanmenMeridianShockTreatmentItemResult BuildPrepareRequest(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& ConditionStatus,
		const FGuid& ItemInstanceId,
		int32 Quantity = 1);

	/** Captures the ready authority and durably prepares the treatment item. */
	static Fdemo_mapShanmenMeridianShockTreatmentItemResult PrepareActiveRun(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& ConditionStatus,
		const FGuid& ItemInstanceId,
		int32 Quantity = 1);

	/** Accepts only the receipt produced by the exact prepared treatment intent. */
	static Fdemo_mapShanmenMeridianShockTreatmentItemResult BuildCommitRequest(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
		const Fdemo_mapShanmenCombatConditionTreatmentReceipt& TreatmentReceipt);

	/** Builds the only pre-treatment terminal path without consuming Quantity. */
	static Fdemo_mapShanmenMeridianShockTreatmentItemResult BuildCancelRequest(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot&
			LiveConditionStatus);

	/** Durably consumes Quantity only after exact condition treatment proof. */
	static Fdemo_mapShanmenMeridianShockTreatmentItemResult CommitTreated(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
		const Fdemo_mapShanmenCombatConditionTreatmentReceipt& TreatmentReceipt);

	/** Durably releases a prepared intent before condition mutation. */
	static Fdemo_mapShanmenMeridianShockTreatmentItemResult CancelBeforeTreatment(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenMeridianShockTreatmentItemResult& Preparation,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot&
			LiveConditionStatus);
};
