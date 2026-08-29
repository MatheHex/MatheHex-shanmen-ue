#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceReconciliationPlanner.h"

enum class Edemo_mapShanmenFormationInfluenceDispatchOrigin : uint8
{
	Transition,
	Reconciliation
};

struct Fdemo_mapShanmenFormationInfluenceDispatchScope
{
	FGuid RunId;
	FGuid OwnerId;
	FGuid SourceEntityId;
	FGuid DeploymentId;

	bool IsValid() const;
};

enum class Edemo_mapShanmenFormationInfluenceAttemptOutcome : uint8
{
	RetryableFailure,
	Succeeded
};

/** Opaque executor evidence; the ledger owns retry/idempotency, not execution. */
struct Fdemo_mapShanmenFormationInfluenceAttemptCommand
{
	FGuid IntentId;
	FGuid AttemptId;
	FGuid ExecutorReceiptId;
	Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome =
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure;

	bool IsValid() const;
};

struct Fdemo_mapShanmenFormationInfluenceAttemptReceipt
{
	FGuid ReceiptId;
	FGuid LedgerId;
	FGuid IntentId;
	FGuid AttemptId;
	FGuid ExecutorReceiptId;
	Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome =
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure;

	bool IsValid() const;
};

enum class Edemo_mapShanmenFormationInfluenceSubmitStatus : uint8
{
	Accepted,
	Replayed,
	BatchInvalid,
	ScopeInvalid,
	ScopeMismatch,
	LedgerClosed,
	BatchConflict,
	IntentConflict,
	StateInvalid
};

struct Fdemo_mapShanmenFormationInfluenceSubmitResult
{
	Edemo_mapShanmenFormationInfluenceSubmitStatus Status =
		Edemo_mapShanmenFormationInfluenceSubmitStatus::BatchInvalid;
	FString Diagnostic;
	FGuid LedgerId;
	FGuid BatchRecordId;
	int32 AcceptedBatchCount = 0;
	int32 PendingIntentCount = 0;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationInfluenceAcknowledgeStatus : uint8
{
	RetryRecorded,
	RetryReplayed,
	Acknowledged,
	AcknowledgementReplayed,
	CommandInvalid,
	LedgerEmpty,
	LedgerClosed,
	IntentUnknown,
	AttemptConflict,
	AlreadyAcknowledged,
	StateInvalid
};

struct Fdemo_mapShanmenFormationInfluenceAcknowledgeResult
{
	Edemo_mapShanmenFormationInfluenceAcknowledgeStatus Status =
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::CommandInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt Receipt;
	int32 PendingIntentCount = 0;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationInfluenceSealStatus : uint8
{
	Sealed,
	SealReplayed,
	LedgerEmpty,
	PendingIntents,
	StateInvalid
};

struct Fdemo_mapShanmenFormationInfluenceSealResult
{
	Edemo_mapShanmenFormationInfluenceSealStatus Status =
		Edemo_mapShanmenFormationInfluenceSealStatus::LedgerEmpty;
	FString Diagnostic;
	FGuid SealId;

	bool IsSuccess() const;
};

/**
 * One deployment-scoped, explicitly driven delivery ledger.
 *
 * It validates P8.11/P8.12 batches, owns canonical pending order and attempt
 * replay, and can be sealed only after every accepted intent succeeds. It owns
 * no World, Actor, effect executor, cadence, timer, persistence, or GAS state.
 */
class Fdemo_mapShanmenFormationInfluenceDispatchLedger
{
public:
	Fdemo_mapShanmenFormationInfluenceSubmitResult Accept(
		const Fdemo_mapShanmenFormationInfluenceTransitionBatch& Batch);
	Fdemo_mapShanmenFormationInfluenceSubmitResult Accept(
		const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch);
	Fdemo_mapShanmenFormationInfluenceAcknowledgeResult Acknowledge(
		const Fdemo_mapShanmenFormationInfluenceAttemptCommand& Command);
	Fdemo_mapShanmenFormationInfluenceSealResult Seal();

	bool IsConsistent() const;
	bool IsSealed() const { return bSealed; }
	FGuid GetLedgerId() const { return LedgerId; }
	FGuid GetSealId() const { return SealId; }
	int32 GetAcceptedBatchCount() const { return AcceptedBatches.Num(); }
	int32 GetIntentCount() const { return Entries.Num(); }
	int32 GetPendingIntentCount() const;
	bool TryPeekNextPending(
		Fdemo_mapShanmenFormationInfluenceIntent& OutIntent) const;
	bool TryGetSuccessfulReceipt(
		const FGuid& IntentId,
		Fdemo_mapShanmenFormationInfluenceAttemptReceipt& OutReceipt) const;

private:
	struct FAcceptedBatchRecord
	{
		FGuid RecordId;
		Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin =
			Edemo_mapShanmenFormationInfluenceDispatchOrigin::Transition;
		FGuid SourceBatchId;
		TArray<FGuid> IntentIds;
	};

	struct FIntentEntry
	{
		Fdemo_mapShanmenFormationInfluenceIntent Intent;
		TArray<Fdemo_mapShanmenFormationInfluenceAttemptReceipt> Attempts;
		bool bAcknowledged = false;
	};

	Fdemo_mapShanmenFormationInfluenceSubmitResult AcceptInternal(
		Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin,
		const FGuid& SourceBatchId,
		const Fdemo_mapShanmenFormationInfluenceDispatchScope& Scope,
		const TArray<Fdemo_mapShanmenFormationInfluenceIntent>& Intents);
	const FAcceptedBatchRecord* FindBatch(
		Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin,
		const FGuid& SourceBatchId) const;
	FIntentEntry* FindEntry(const FGuid& IntentId);
	const FIntentEntry* FindEntry(const FGuid& IntentId) const;

	Fdemo_mapShanmenFormationInfluenceDispatchScope Scope;
	FGuid LedgerId;
	TArray<FAcceptedBatchRecord> AcceptedBatches;
	TArray<FIntentEntry> Entries;
	bool bSealed = false;
	FGuid SealId;
};
