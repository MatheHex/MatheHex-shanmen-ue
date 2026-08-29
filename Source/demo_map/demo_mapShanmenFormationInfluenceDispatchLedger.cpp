#include "demo_mapShanmenFormationInfluenceDispatchLedger.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownOrigin(
		const Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin)
	{
		return Origin == Edemo_mapShanmenFormationInfluenceDispatchOrigin::Transition
			|| Origin
				== Edemo_mapShanmenFormationInfluenceDispatchOrigin::Reconciliation;
	}

	bool IsKnownOutcome(
		const Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome)
	{
		return Outcome
				== Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure
			|| Outcome
				== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded;
	}

	bool ScopeMatches(
		const Fdemo_mapShanmenFormationInfluenceDispatchScope& Left,
		const Fdemo_mapShanmenFormationInfluenceDispatchScope& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.RunId == Right.RunId && Left.OwnerId == Right.OwnerId
			&& Left.SourceEntityId == Right.SourceEntityId
			&& Left.DeploymentId == Right.DeploymentId;
	}

	bool IntentMatchesScope(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		const Fdemo_mapShanmenFormationInfluenceDispatchScope& Scope)
	{
		return Intent.IsValid() && Scope.IsValid()
			&& Intent.RunId == Scope.RunId && Intent.OwnerId == Scope.OwnerId
			&& Intent.SourceEntityId == Scope.SourceEntityId
			&& Intent.DeploymentId == Scope.DeploymentId;
	}

	FGuid MakeLedgerId(
		const Fdemo_mapShanmenFormationInfluenceDispatchScope& Scope)
	{
		if (!Scope.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceDispatchLedger.r1"),
			{
				GuidDigits(Scope.RunId), GuidDigits(Scope.OwnerId),
				GuidDigits(Scope.SourceEntityId), GuidDigits(Scope.DeploymentId)
			});
	}

	FGuid MakeBatchRecordId(
		const FGuid& LedgerId,
		const Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin,
		const FGuid& SourceBatchId,
		const TArray<FGuid>& IntentIds)
	{
		if (!LedgerId.IsValid() || !IsKnownOrigin(Origin)
			|| !SourceBatchId.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(LedgerId), FString::FromInt(static_cast<int32>(Origin)),
			GuidDigits(SourceBatchId), FString::FromInt(IntentIds.Num())
		};
		for (const FGuid& IntentId : IntentIds)
		{
			if (!IntentId.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(IntentId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceDispatchBatchRecord.r1"), Parts);
	}

	FGuid MakeAttemptReceiptId(
		const Fdemo_mapShanmenFormationInfluenceAttemptReceipt& Receipt)
	{
		if (!Receipt.LedgerId.IsValid() || !Receipt.IntentId.IsValid()
			|| !Receipt.AttemptId.IsValid()
			|| !Receipt.ExecutorReceiptId.IsValid()
			|| !IsKnownOutcome(Receipt.Outcome))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceDispatchAttempt.r1"),
			{
				GuidDigits(Receipt.LedgerId), GuidDigits(Receipt.IntentId),
				GuidDigits(Receipt.AttemptId),
				GuidDigits(Receipt.ExecutorReceiptId),
				FString::FromInt(static_cast<int32>(Receipt.Outcome))
			});
	}

	bool ReceiptMatchesCommand(
		const Fdemo_mapShanmenFormationInfluenceAttemptReceipt& Receipt,
		const Fdemo_mapShanmenFormationInfluenceAttemptCommand& Command)
	{
		return Receipt.IsValid() && Command.IsValid()
			&& Receipt.IntentId == Command.IntentId
			&& Receipt.AttemptId == Command.AttemptId
			&& Receipt.ExecutorReceiptId == Command.ExecutorReceiptId
			&& Receipt.Outcome == Command.Outcome;
	}

	FGuid MakeSealId(
		const FGuid& LedgerId,
		const TArray<FGuid>& BatchRecordIds,
		const TArray<FGuid>& AttemptReceiptIds)
	{
		if (!LedgerId.IsValid() || BatchRecordIds.IsEmpty())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(LedgerId), FString::FromInt(BatchRecordIds.Num())
		};
		for (const FGuid& Id : BatchRecordIds)
		{
			if (!Id.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Id));
		}
		Parts.Add(FString::FromInt(AttemptReceiptIds.Num()));
		for (const FGuid& Id : AttemptReceiptIds)
		{
			if (!Id.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Id));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceDispatchSeal.r1"), Parts);
	}

	Fdemo_mapShanmenFormationInfluenceSubmitResult SubmitResult(
		const Edemo_mapShanmenFormationInfluenceSubmitStatus Status,
		const TCHAR* Diagnostic,
		const FGuid& LedgerId = FGuid(),
		const FGuid& BatchRecordId = FGuid(),
		const int32 AcceptedBatchCount = 0,
		const int32 PendingIntentCount = 0)
	{
		Fdemo_mapShanmenFormationInfluenceSubmitResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.LedgerId = LedgerId;
		Result.BatchRecordId = BatchRecordId;
		Result.AcceptedBatchCount = AcceptedBatchCount;
		Result.PendingIntentCount = PendingIntentCount;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceAcknowledgeResult AckResult(
		const Edemo_mapShanmenFormationInfluenceAcknowledgeStatus Status,
		const TCHAR* Diagnostic,
		const int32 PendingIntentCount,
		const Fdemo_mapShanmenFormationInfluenceAttemptReceipt& Receipt = {})
	{
		Fdemo_mapShanmenFormationInfluenceAcknowledgeResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Receipt = Receipt;
		Result.PendingIntentCount = PendingIntentCount;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceDispatchScope::IsValid() const
{
	return RunId.IsValid() && OwnerId.IsValid() && SourceEntityId.IsValid()
		&& DeploymentId.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceAttemptCommand::IsValid() const
{
	return IntentId.IsValid() && AttemptId.IsValid()
		&& ExecutorReceiptId.IsValid() && IsKnownOutcome(Outcome);
}

bool Fdemo_mapShanmenFormationInfluenceAttemptReceipt::IsValid() const
{
	return ReceiptId.IsValid() && ReceiptId == MakeAttemptReceiptId(*this);
}

bool Fdemo_mapShanmenFormationInfluenceSubmitResult::IsSuccess() const
{
	return (Status == Edemo_mapShanmenFormationInfluenceSubmitStatus::Accepted
			|| Status == Edemo_mapShanmenFormationInfluenceSubmitStatus::Replayed)
		&& LedgerId.IsValid() && BatchRecordId.IsValid()
		&& AcceptedBatchCount > 0 && PendingIntentCount >= 0;
}

bool Fdemo_mapShanmenFormationInfluenceAcknowledgeResult::IsSuccess() const
{
	return (Status
				== Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::RetryRecorded
			|| Status
				== Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::RetryReplayed
			|| Status
				== Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::Acknowledged
			|| Status
				== Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AcknowledgementReplayed)
		&& Receipt.IsValid() && PendingIntentCount >= 0;
}

bool Fdemo_mapShanmenFormationInfluenceSealResult::IsSuccess() const
{
	return (Status == Edemo_mapShanmenFormationInfluenceSealStatus::Sealed
			|| Status == Edemo_mapShanmenFormationInfluenceSealStatus::SealReplayed)
		&& SealId.IsValid();
}

int32 Fdemo_mapShanmenFormationInfluenceDispatchLedger::GetPendingIntentCount()
	const
{
	int32 Count = 0;
	for (const FIntentEntry& Entry : Entries)
	{
		Count += Entry.bAcknowledged ? 0 : 1;
	}
	return Count;
}

const Fdemo_mapShanmenFormationInfluenceDispatchLedger::FAcceptedBatchRecord*
Fdemo_mapShanmenFormationInfluenceDispatchLedger::FindBatch(
	const Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin,
	const FGuid& SourceBatchId) const
{
	return AcceptedBatches.FindByPredicate(
		[Origin, SourceBatchId](const FAcceptedBatchRecord& Record)
		{
			return Record.Origin == Origin
				&& Record.SourceBatchId == SourceBatchId;
		});
}

Fdemo_mapShanmenFormationInfluenceDispatchLedger::FIntentEntry*
Fdemo_mapShanmenFormationInfluenceDispatchLedger::FindEntry(
	const FGuid& IntentId)
{
	return Entries.FindByPredicate(
		[IntentId](const FIntentEntry& Entry)
		{
			return Entry.Intent.IntentId == IntentId;
		});
}

const Fdemo_mapShanmenFormationInfluenceDispatchLedger::FIntentEntry*
Fdemo_mapShanmenFormationInfluenceDispatchLedger::FindEntry(
	const FGuid& IntentId) const
{
	return Entries.FindByPredicate(
		[IntentId](const FIntentEntry& Entry)
		{
			return Entry.Intent.IntentId == IntentId;
		});
}

bool Fdemo_mapShanmenFormationInfluenceDispatchLedger::IsConsistent() const
{
	if (!LedgerId.IsValid())
	{
		return !Scope.IsValid() && AcceptedBatches.IsEmpty()
			&& Entries.IsEmpty() && !bSealed && !SealId.IsValid();
	}
	if (!Scope.IsValid() || LedgerId != MakeLedgerId(Scope)
		|| AcceptedBatches.IsEmpty())
	{
		return false;
	}

	TSet<FGuid> BatchIds;
	TSet<FGuid> SourceKeys;
	TSet<FGuid> ReferencedIntents;
	TArray<FGuid> BatchRecordIds;
	for (const FAcceptedBatchRecord& Record : AcceptedBatches)
	{
		const FGuid SourceKey = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceDispatchSourceKey.r1"),
			{
				FString::FromInt(static_cast<int32>(Record.Origin)),
				GuidDigits(Record.SourceBatchId)
			});
		if (!IsKnownOrigin(Record.Origin) || !Record.SourceBatchId.IsValid()
			|| Record.RecordId != MakeBatchRecordId(
				LedgerId, Record.Origin, Record.SourceBatchId, Record.IntentIds)
			|| BatchIds.Contains(Record.RecordId) || SourceKeys.Contains(SourceKey))
		{
			return false;
		}
		BatchIds.Add(Record.RecordId);
		SourceKeys.Add(SourceKey);
		BatchRecordIds.Add(Record.RecordId);
		for (const FGuid& IntentId : Record.IntentIds)
		{
			if (!IntentId.IsValid() || ReferencedIntents.Contains(IntentId))
			{
				return false;
			}
			ReferencedIntents.Add(IntentId);
		}
	}

	TSet<FGuid> EntryIds;
	TArray<FGuid> AttemptReceiptIds;
	for (const FIntentEntry& Entry : Entries)
	{
		if (!IntentMatchesScope(Entry.Intent, Scope)
			|| !ReferencedIntents.Contains(Entry.Intent.IntentId)
			|| EntryIds.Contains(Entry.Intent.IntentId))
		{
			return false;
		}
		EntryIds.Add(Entry.Intent.IntentId);
		TSet<FGuid> AttemptIds;
		int32 SuccessCount = 0;
		for (const auto& Attempt : Entry.Attempts)
		{
			if (!Attempt.IsValid() || Attempt.LedgerId != LedgerId
				|| Attempt.IntentId != Entry.Intent.IntentId
				|| AttemptIds.Contains(Attempt.AttemptId))
			{
				return false;
			}
			AttemptIds.Add(Attempt.AttemptId);
			AttemptReceiptIds.Add(Attempt.ReceiptId);
			SuccessCount += Attempt.Outcome
				== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded;
		}
		if (SuccessCount > 1 || Entry.bAcknowledged != (SuccessCount == 1))
		{
			return false;
		}
	}
	if (EntryIds.Num() != ReferencedIntents.Num())
	{
		return false;
	}
	if (bSealed)
	{
		return GetPendingIntentCount() == 0 && SealId.IsValid()
			&& SealId == MakeSealId(
				LedgerId, BatchRecordIds, AttemptReceiptIds);
	}
	return !SealId.IsValid();
}

Fdemo_mapShanmenFormationInfluenceSubmitResult
Fdemo_mapShanmenFormationInfluenceDispatchLedger::Accept(
	const Fdemo_mapShanmenFormationInfluenceTransitionBatch& Batch)
{
	if (!Batch.IsValid())
	{
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::BatchInvalid,
			TEXT("Dispatch requires one valid P8.11 transition batch."));
	}
	return AcceptInternal(
		Edemo_mapShanmenFormationInfluenceDispatchOrigin::Transition,
		Batch.BatchId,
		{ Batch.Area.RunId, Batch.Area.OwnerId, Batch.SourceEntityId,
			Batch.Area.DeploymentId },
		Batch.Intents);
}

Fdemo_mapShanmenFormationInfluenceSubmitResult
Fdemo_mapShanmenFormationInfluenceDispatchLedger::Accept(
	const Fdemo_mapShanmenFormationInfluenceReconciliationBatch& Batch)
{
	if (!Batch.IsValid())
	{
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::BatchInvalid,
			TEXT("Dispatch requires one valid P8.12 reconciliation batch."));
	}
	const auto& SourceScope = Batch.Current.IsSet()
		? Batch.Current.GetValue() : Batch.Previous.GetValue();
	return AcceptInternal(
		Edemo_mapShanmenFormationInfluenceDispatchOrigin::Reconciliation,
		Batch.BatchId,
		{ SourceScope.Area.RunId, SourceScope.Area.OwnerId,
			Batch.SourceEntityId, SourceScope.Area.DeploymentId },
		Batch.Intents);
}

Fdemo_mapShanmenFormationInfluenceSubmitResult
Fdemo_mapShanmenFormationInfluenceDispatchLedger::AcceptInternal(
	const Edemo_mapShanmenFormationInfluenceDispatchOrigin Origin,
	const FGuid& SourceBatchId,
	const Fdemo_mapShanmenFormationInfluenceDispatchScope& IncomingScope,
	const TArray<Fdemo_mapShanmenFormationInfluenceIntent>& Intents)
{
	if (!IsConsistent())
	{
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::StateInvalid,
			TEXT("Dispatch ledger is internally inconsistent."));
	}
	if (!IsKnownOrigin(Origin) || !SourceBatchId.IsValid()
		|| !IncomingScope.IsValid())
	{
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::ScopeInvalid,
			TEXT("Dispatch batch scope is invalid."));
	}
	for (const auto& Intent : Intents)
	{
		if (!IntentMatchesScope(Intent, IncomingScope))
		{
			return SubmitResult(
				Edemo_mapShanmenFormationInfluenceSubmitStatus::ScopeMismatch,
				TEXT("One dispatch intent belongs to another ledger scope."));
		}
	}
	if (LedgerId.IsValid() && !ScopeMatches(Scope, IncomingScope))
	{
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::ScopeMismatch,
			TEXT("One ledger cannot accept another deployment scope."),
			LedgerId, FGuid(), AcceptedBatches.Num(), GetPendingIntentCount());
	}

	const FGuid CandidateLedgerId = LedgerId.IsValid()
		? LedgerId : MakeLedgerId(IncomingScope);
	TArray<FGuid> IntentIds;
	TSet<FGuid> IncomingIds;
	for (const auto& Intent : Intents)
	{
		if (IncomingIds.Contains(Intent.IntentId))
		{
			return SubmitResult(
				Edemo_mapShanmenFormationInfluenceSubmitStatus::IntentConflict,
				TEXT("A source batch contains a duplicate intent identity."));
		}
		IncomingIds.Add(Intent.IntentId);
		IntentIds.Add(Intent.IntentId);
	}
	const FGuid RecordId = MakeBatchRecordId(
		CandidateLedgerId, Origin, SourceBatchId, IntentIds);
	if (const FAcceptedBatchRecord* Existing = FindBatch(Origin, SourceBatchId))
	{
		if (Existing->RecordId != RecordId)
		{
			return SubmitResult(
				Edemo_mapShanmenFormationInfluenceSubmitStatus::BatchConflict,
				TEXT("A source batch identity replayed with different intents."),
				LedgerId, Existing->RecordId, AcceptedBatches.Num(),
				GetPendingIntentCount());
		}
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::Replayed,
			TEXT("The exact accepted source batch replayed without duplication."),
			LedgerId, Existing->RecordId, AcceptedBatches.Num(),
			GetPendingIntentCount());
	}
	if (bSealed)
	{
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::LedgerClosed,
			TEXT("A sealed dispatch ledger cannot accept another source batch."),
			LedgerId, FGuid(), AcceptedBatches.Num(), 0);
	}
	for (const FGuid& IntentId : IntentIds)
	{
		if (FindEntry(IntentId))
		{
			return SubmitResult(
				Edemo_mapShanmenFormationInfluenceSubmitStatus::IntentConflict,
				TEXT("A different source batch already owns one intent identity."),
				CandidateLedgerId, FGuid(), AcceptedBatches.Num(),
				GetPendingIntentCount());
		}
	}

	const auto Before = *this;
	if (!LedgerId.IsValid())
	{
		Scope = IncomingScope;
		LedgerId = CandidateLedgerId;
	}
	FAcceptedBatchRecord& Record = AcceptedBatches.AddDefaulted_GetRef();
	Record.RecordId = RecordId;
	Record.Origin = Origin;
	Record.SourceBatchId = SourceBatchId;
	Record.IntentIds = IntentIds;
	for (const auto& Intent : Intents)
	{
		FIntentEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Intent = Intent;
	}
	if (!IsConsistent())
	{
		*this = Before;
		return SubmitResult(
			Edemo_mapShanmenFormationInfluenceSubmitStatus::StateInvalid,
			TEXT("Accepted source batch failed ledger self-validation."));
	}
	return SubmitResult(
		Edemo_mapShanmenFormationInfluenceSubmitStatus::Accepted,
		TEXT("Source batch intents entered canonical pending order."),
		LedgerId, RecordId, AcceptedBatches.Num(), GetPendingIntentCount());
}

Fdemo_mapShanmenFormationInfluenceAcknowledgeResult
Fdemo_mapShanmenFormationInfluenceDispatchLedger::Acknowledge(
	const Fdemo_mapShanmenFormationInfluenceAttemptCommand& Command)
{
	if (!IsConsistent())
	{
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::StateInvalid,
			TEXT("Dispatch ledger is internally inconsistent."), 0);
	}
	if (!Command.IsValid())
	{
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::CommandInvalid,
			TEXT("Acknowledgement requires valid intent, attempt, and executor receipts."),
			GetPendingIntentCount());
	}
	if (!LedgerId.IsValid())
	{
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::LedgerEmpty,
			TEXT("No source batch has established this ledger."), 0);
	}
	FIntentEntry* Entry = FindEntry(Command.IntentId);
	if (!Entry)
	{
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::IntentUnknown,
			TEXT("Acknowledgement intent is not owned by this ledger."),
			GetPendingIntentCount());
	}
	for (const auto& Existing : Entry->Attempts)
	{
		if (Existing.AttemptId != Command.AttemptId)
		{
			continue;
		}
		if (!ReceiptMatchesCommand(Existing, Command))
		{
			return AckResult(
				Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AttemptConflict,
				TEXT("AttemptId replayed with different executor evidence."),
				GetPendingIntentCount());
		}
		const auto Status = Existing.Outcome
			== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded
			? Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AcknowledgementReplayed
			: Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::RetryReplayed;
		return AckResult(
			Status, TEXT("The exact execution attempt replayed."),
			GetPendingIntentCount(), Existing);
	}
	if (bSealed)
	{
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::LedgerClosed,
			TEXT("A sealed ledger accepts only exact acknowledgement replay."), 0);
	}
	if (Entry->bAcknowledged)
	{
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AlreadyAcknowledged,
			TEXT("A successful intent cannot accept a different execution attempt."),
			GetPendingIntentCount());
	}

	const auto Before = *this;
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt Receipt;
	Receipt.LedgerId = LedgerId;
	Receipt.IntentId = Command.IntentId;
	Receipt.AttemptId = Command.AttemptId;
	Receipt.ExecutorReceiptId = Command.ExecutorReceiptId;
	Receipt.Outcome = Command.Outcome;
	Receipt.ReceiptId = MakeAttemptReceiptId(Receipt);
	Entry->Attempts.Add(Receipt);
	Entry->bAcknowledged = Command.Outcome
		== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded;
	if (!IsConsistent())
	{
		*this = Before;
		return AckResult(
			Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::StateInvalid,
			TEXT("Execution attempt failed ledger self-validation."),
			GetPendingIntentCount());
	}
	const auto Status = Entry->bAcknowledged
		? Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::Acknowledged
		: Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::RetryRecorded;
	return AckResult(
		Status, Entry->bAcknowledged
			? TEXT("Intent execution succeeded and left the pending set.")
			: TEXT("Retryable failure was recorded; intent remains pending."),
		GetPendingIntentCount(), Receipt);
}

Fdemo_mapShanmenFormationInfluenceSealResult
Fdemo_mapShanmenFormationInfluenceDispatchLedger::Seal()
{
	Fdemo_mapShanmenFormationInfluenceSealResult Result;
	if (!IsConsistent())
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceSealStatus::StateInvalid;
		Result.Diagnostic = TEXT("Dispatch ledger is internally inconsistent.");
		return Result;
	}
	if (bSealed)
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceSealStatus::SealReplayed;
		Result.Diagnostic = TEXT("The exact sealed ledger replayed.");
		Result.SealId = SealId;
		return Result;
	}
	if (!LedgerId.IsValid() || AcceptedBatches.IsEmpty())
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceSealStatus::LedgerEmpty;
		Result.Diagnostic = TEXT("At least one accepted source batch is required.");
		return Result;
	}
	if (GetPendingIntentCount() != 0)
	{
		Result.Status = Edemo_mapShanmenFormationInfluenceSealStatus::PendingIntents;
		Result.Diagnostic = TEXT("Every accepted intent must succeed before sealing.");
		return Result;
	}

	const auto Before = *this;
	TArray<FGuid> BatchRecordIds;
	TArray<FGuid> AttemptReceiptIds;
	for (const auto& Record : AcceptedBatches)
	{
		BatchRecordIds.Add(Record.RecordId);
	}
	for (const auto& Entry : Entries)
	{
		for (const auto& Attempt : Entry.Attempts)
		{
			AttemptReceiptIds.Add(Attempt.ReceiptId);
		}
	}
	SealId = MakeSealId(LedgerId, BatchRecordIds, AttemptReceiptIds);
	bSealed = true;
	if (!IsConsistent())
	{
		*this = Before;
		Result.Status = Edemo_mapShanmenFormationInfluenceSealStatus::StateInvalid;
		Result.Diagnostic = TEXT("Dispatch seal failed ledger self-validation.");
		return Result;
	}
	Result.Status = Edemo_mapShanmenFormationInfluenceSealStatus::Sealed;
	Result.Diagnostic = TEXT("All accepted influence intents are acknowledged.");
	Result.SealId = SealId;
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceDispatchLedger::TryPeekNextPending(
	Fdemo_mapShanmenFormationInfluenceIntent& OutIntent) const
{
	OutIntent = Fdemo_mapShanmenFormationInfluenceIntent();
	if (!IsConsistent())
	{
		return false;
	}
	for (const FIntentEntry& Entry : Entries)
	{
		if (!Entry.bAcknowledged)
		{
			OutIntent = Entry.Intent;
			return true;
		}
	}
	return false;
}

bool Fdemo_mapShanmenFormationInfluenceDispatchLedger::TryGetIntent(
	const FGuid& IntentId,
	Fdemo_mapShanmenFormationInfluenceIntent& OutIntent) const
{
	OutIntent = Fdemo_mapShanmenFormationInfluenceIntent();
	if (!IsConsistent() || !IntentId.IsValid())
	{
		return false;
	}
	const FIntentEntry* Entry = FindEntry(IntentId);
	if (!Entry)
	{
		return false;
	}
	OutIntent = Entry->Intent;
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceDispatchLedger::TryGetAttemptReceipt(
	const FGuid& IntentId,
	const FGuid& AttemptId,
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt& OutReceipt) const
{
	OutReceipt = Fdemo_mapShanmenFormationInfluenceAttemptReceipt();
	if (!IsConsistent() || !IntentId.IsValid() || !AttemptId.IsValid())
	{
		return false;
	}
	const FIntentEntry* Entry = FindEntry(IntentId);
	if (!Entry)
	{
		return false;
	}
	for (const auto& Attempt : Entry->Attempts)
	{
		if (Attempt.AttemptId == AttemptId)
		{
			OutReceipt = Attempt;
			return true;
		}
	}
	return false;
}

bool Fdemo_mapShanmenFormationInfluenceDispatchLedger::TryGetSuccessfulReceipt(
	const FGuid& IntentId,
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt& OutReceipt) const
{
	OutReceipt = Fdemo_mapShanmenFormationInfluenceAttemptReceipt();
	if (!IsConsistent())
	{
		return false;
	}
	const FIntentEntry* Entry = FindEntry(IntentId);
	if (!Entry || !Entry->bAcknowledged)
	{
		return false;
	}
	for (const auto& Attempt : Entry->Attempts)
	{
		if (Attempt.Outcome
			== Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded)
		{
			OutReceipt = Attempt;
			return true;
		}
	}
	return false;
}
