#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenThrownWeaponExecution.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenThrownWeaponItemStatus : uint8
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
	ActionInvalid,
	ActionMismatch,
	SourceItemMismatch,
	ItemNotFound,
	DefinitionNotThrownWeapon,
	QuantityReservationInvalid,
	QuantityUnavailable,
	RequestInvalid,
	PrepareRejected,
	PreparationInvalid,
	LaunchReceiptInvalid,
	LaunchMismatch,
	FinalizeRejected
};

/** One exact action-to-item transaction attempt and its durable evidence. */
struct Fdemo_mapShanmenThrownWeaponItemResult
{
	Edemo_mapShanmenThrownWeaponItemStatus Status =
		Edemo_mapShanmenThrownWeaponItemStatus::RequestInvalid;
	FString Diagnostic;
	FShanmenCombatActionSnapshot Action;
	FShanmenItemRunQuantityIntentRequest PrepareRequest;
	FShanmenItemRunQuantityIntentFinalizeRequest FinalizeRequest;
	FShanmenItemDurableCommandResult PrepareCommand;
	FShanmenItemDurableCommandResult FinalizeCommand;

	bool HasPrepareRequest() const;
	bool IsPrepared() const;
	bool IsFinalized() const;
};

/**
 * Product bridge from one P7.0 physical thrown-item action to ShanmenItems.
 *
 * Prepare freezes one exact active-Run Quantity balance. Commit is legal only
 * after the matching immutable straight-launch receipt exists; cancellation is
 * the only pre-launch terminal path. No Runtime inventory or legacy authority
 * is mutated by this adapter.
 */
struct Fdemo_mapShanmenThrownWeaponItemAdapter
{
	/** Pure read-only authority gate and deterministic prepare request builder. */
	static Fdemo_mapShanmenThrownWeaponItemResult BuildPrepareRequest(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenCombatActionSnapshot& Action,
		int32 Quantity = 1);

	/** Captures the ready authority, validates evidence, and durably prepares. */
	static Fdemo_mapShanmenThrownWeaponItemResult PrepareActiveRun(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenCombatActionSnapshot& Action,
		int32 Quantity = 1);

	/** Pure commit builder; accepts only the launch receipt for this exact action. */
	static Fdemo_mapShanmenThrownWeaponItemResult BuildCommitRequest(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenThrownWeaponLaunchReceipt& LaunchReceipt);

	/** Pure pre-launch cancellation request builder. */
	static Fdemo_mapShanmenThrownWeaponItemResult BuildCancelRequest(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation);

	/** Durably consumes only after P7.0 produced the exact launch proof. */
	static Fdemo_mapShanmenThrownWeaponItemResult CommitLaunched(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation,
		const FShanmenThrownWeaponLaunchReceipt& LaunchReceipt);

	/** Durably releases a prepared intent before any launch was accepted. */
	static Fdemo_mapShanmenThrownWeaponItemResult CancelBeforeLaunch(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponItemResult& Preparation);
};
