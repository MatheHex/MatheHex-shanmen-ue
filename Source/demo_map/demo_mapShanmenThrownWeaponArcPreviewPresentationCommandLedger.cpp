#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	using FState = Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownOutcome(const EOutcome Outcome)
	{
		return Outcome == EOutcome::Applied
			|| Outcome == EOutcome::Rejected;
	}

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool IsUsableState(const FState& State)
	{
		return State.IsEmpty() || State.IsValid();
	}

	FGuid MakeReceiptId(
		const FCommand& Command,
		const FName ConsumerDefinitionId,
		const EOutcome Outcome,
		const FName OutcomeCode)
	{
		if (!Command.IsValid() || ConsumerDefinitionId.IsNone()
			|| !IsKnownOutcome(Outcome) || OutcomeCode.IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationCommandReceipt.r1"),
			{
				GuidDigits(Command.GetCommandId()),
				GuidDigits(Command.GetRunId()),
				ConsumerDefinitionId.ToString(),
				FString::FromInt(static_cast<int32>(Outcome)),
				OutcomeCode.ToString()
			});
	}

	FGuid MakeLedgerId(const FGuid& RunId, const FName ConsumerDefinitionId)
	{
		if (!RunId.IsValid() || ConsumerDefinitionId.IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationCommandLedger.r1"),
			{ GuidDigits(RunId), ConsumerDefinitionId.ToString() });
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt::
	TryCreate(
		const FCommand& InCommand,
		const FName InConsumerDefinitionId,
		const EOutcome InOutcome,
		const FName InOutcomeCode,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			OutReceipt,
		FString& OutDiagnostic)
{
	OutReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt();
	OutDiagnostic.Reset();
	if (!InCommand.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation receipt requires one valid command.");
		return false;
	}
	if (InConsumerDefinitionId.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation receipt requires one stable consumer identity.");
		return false;
	}
	if (!IsKnownOutcome(InOutcome) || InOutcomeCode.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation receipt requires Applied or Rejected and one stable outcome code.");
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt Candidate;
	Candidate.Command = InCommand;
	Candidate.ConsumerDefinitionId = InConsumerDefinitionId;
	Candidate.Outcome = InOutcome;
	Candidate.OutcomeCode = InOutcomeCode;
	Candidate.ReceiptId = MakeReceiptId(
		Candidate.Command,
		Candidate.ConsumerDefinitionId,
		Candidate.Outcome,
		Candidate.OutcomeCode);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation receipt failed deterministic self-validation.");
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Captured one immutable Arc preview presentation command receipt.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt::
	IsValid() const
{
	return ReceiptId.IsValid() && Command.IsValid()
		&& !ConsumerDefinitionId.IsNone() && IsKnownOutcome(Outcome)
		&& !OutcomeCode.IsNone()
		&& ReceiptId == MakeReceiptId(
			Command, ConsumerDefinitionId, Outcome, OutcomeCode);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
		Other) const
{
	return IsValid() && Other.IsValid()
		&& ReceiptId == Other.ReceiptId
		&& Command.Matches(Other.Command)
		&& ConsumerDefinitionId == Other.ConsumerDefinitionId
		&& Outcome == Other.Outcome && OutcomeCode == Other.OutcomeCode;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt::
	IsApplied() const
{
	return IsValid() && Outcome == EOutcome::Applied;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt::
	IsRejected() const
{
	return IsValid() && Outcome == EOutcome::Rejected;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult::
	IsValid() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| !IsUsableState(PreviousCursorState) || !IsUsableState(CursorState)
		|| PreviousAppliedCount < 0 || AppliedCount < 0
		|| PreviousRejectedCount < 0 || RejectedCount < 0)
	{
		return false;
	}

	const bool bUnchanged =
		StatesMatchOrAreEmpty(PreviousCursorState, CursorState)
		&& PreviousAppliedCount == AppliedCount
		&& PreviousRejectedCount == RejectedCount;
	switch (Status)
	{
	case EStatus::LedgerInactive:
	case EStatus::LedgerInvalid:
		return bUnchanged;

	case EStatus::ReceiptInvalid:
		return bUnchanged && !Receipt.IsValid();

	case EStatus::RunMismatch:
	case EStatus::ConsumerMismatch:
	case EStatus::CursorMismatch:
	case EStatus::ReceiptConflict:
		return bUnchanged && Receipt.IsValid();

	case EStatus::RejectionRecorded:
		return Receipt.IsRejected()
			&& StatesMatchOrAreEmpty(PreviousCursorState, CursorState)
			&& PreviousAppliedCount == AppliedCount
			&& RejectedCount == PreviousRejectedCount + 1;

	case EStatus::RejectionReplayed:
		return Receipt.IsRejected() && bUnchanged;

	case EStatus::ApplicationRecorded:
	case EStatus::ApplicationRecovered:
		return Receipt.IsApplied()
			&& StatesMatchOrAreEmpty(
				PreviousCursorState,
				Receipt.GetCommand().GetPreviousState())
			&& StatesMatchOrAreEmpty(
				CursorState, Receipt.GetCommand().GetState())
			&& AppliedCount == PreviousAppliedCount + 1
			&& RejectedCount == PreviousRejectedCount
			&& (Status != EStatus::ApplicationRecovered
				|| PreviousRejectedCount > 0);

	case EStatus::ApplicationReplayed:
		return Receipt.IsApplied() && bUnchanged;

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult::
	IsAccepted() const
{
	return IsValid()
		&& (Status == EStatus::RejectionRecorded
			|| Status == EStatus::RejectionReplayed
			|| Status == EStatus::ApplicationRecorded
			|| Status == EStatus::ApplicationRecovered
			|| Status == EStatus::ApplicationReplayed);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult::
	DidAdvanceCursor() const
{
	return IsValid()
		&& (Status == EStatus::ApplicationRecorded
			|| Status == EStatus::ApplicationRecovered);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult::
	IsReplay() const
{
	return IsValid()
		&& (Status == EStatus::RejectionReplayed
			|| Status == EStatus::ApplicationReplayed);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::TryBegin(
	const FGuid& InRunId,
	const FName InConsumerDefinitionId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation command ledger is internally invalid.");
		return false;
	}
	if (!InRunId.IsValid() || InConsumerDefinitionId.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation command ledger requires one Run and consumer identity.");
		return false;
	}
	if (IsActive())
	{
		if (RunId == InRunId
			&& ConsumerDefinitionId == InConsumerDefinitionId)
		{
			OutDiagnostic = TEXT(
				"Arc preview presentation command ledger already owns this scope.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Active Arc preview presentation command ledger rejects scope rotation.");
		return false;
	}

	RunId = InRunId;
	ConsumerDefinitionId = InConsumerDefinitionId;
	LedgerId = MakeLedgerId(RunId, ConsumerDefinitionId);
	if (!IsValid())
	{
		Reset();
		OutDiagnostic = TEXT(
			"Arc preview presentation command ledger failed scope validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Arc preview presentation command ledger bound one Run and consumer.");
	return true;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::MakeResult(
	const EStatus Status,
	const FReceipt& Receipt,
	const FState& PreviousCursorState,
	const int32 PreviousAppliedCount,
	const int32 PreviousRejectedCount,
	const TCHAR* Diagnostic) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Receipt = Receipt;
	Result.PreviousCursorState = PreviousCursorState;
	Result.CursorState = CursorState;
	Result.PreviousAppliedCount = PreviousAppliedCount;
	Result.AppliedCount = NumAppliedCommands();
	Result.PreviousRejectedCount = PreviousRejectedCount;
	Result.RejectedCount = NumRejectedCommands();
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::Record(
	const FReceipt& Receipt)
{
	const FState PreviousCursor = CursorState;
	const int32 PreviousAppliedCount = NumAppliedCommands();
	const int32 PreviousRejectedCount = NumRejectedCommands();
	auto Reject = [this, &Receipt, &PreviousCursor, PreviousAppliedCount,
		PreviousRejectedCount](const EStatus Status, const TCHAR* Diagnostic)
	{
		return MakeResult(
			Status,
			Receipt,
			PreviousCursor,
			PreviousAppliedCount,
			PreviousRejectedCount,
			Diagnostic);
	};

	if (!IsValid())
	{
		return Reject(
			EStatus::LedgerInvalid,
			TEXT("Arc preview presentation command ledger is invalid."));
	}
	if (!IsActive())
	{
		return Reject(
			EStatus::LedgerInactive,
			TEXT("Arc preview presentation command ledger is not active."));
	}
	if (!Receipt.IsValid())
	{
		return Reject(
			EStatus::ReceiptInvalid,
			TEXT("Arc preview presentation command receipt is invalid."));
	}
	if (Receipt.GetRunId() != RunId)
	{
		return Reject(
			EStatus::RunMismatch,
			TEXT("Arc preview presentation command receipt belongs to another Run."));
	}
	if (Receipt.GetConsumerDefinitionId() != ConsumerDefinitionId)
	{
		return Reject(
			EStatus::ConsumerMismatch,
			TEXT("Arc preview presentation command receipt belongs to another consumer."));
	}

	FEntry* Existing = Entries.Find(Receipt.GetCommandId());
	if (Receipt.IsRejected())
	{
		if (Existing && Existing->RejectedReceipt.IsValid())
		{
			return Existing->RejectedReceipt.Matches(Receipt)
				? Reject(
					EStatus::RejectionReplayed,
					TEXT("Exact Arc preview command rejection replayed."))
				: Reject(
					EStatus::ReceiptConflict,
					TEXT("Arc preview command already has different rejection evidence."));
		}
		if (Existing && Existing->AppliedReceipt.IsValid())
		{
			return Reject(
				EStatus::ReceiptConflict,
				TEXT("Applied Arc preview command cannot accept a later rejection."));
		}
		if (!StatesMatchOrAreEmpty(
				CursorState, Receipt.GetCommand().GetPreviousState()))
		{
			return Reject(
				EStatus::CursorMismatch,
				TEXT("Arc preview rejection is not at the current consumer cursor."));
		}

		const auto PreviousLedger = *this;
		FEntry& Entry = Entries.FindOrAdd(Receipt.GetCommandId());
		Entry.RejectedReceipt = Receipt;
		if (!IsValid())
		{
			*this = PreviousLedger;
			return Reject(
				EStatus::LedgerInvalid,
				TEXT("Arc preview rejection would invalidate the ledger."));
		}
		return MakeResult(
			EStatus::RejectionRecorded,
			Receipt,
			PreviousCursor,
			PreviousAppliedCount,
			PreviousRejectedCount,
			TEXT("Recorded Arc preview command rejection without advancing cursor."));
	}

	if (Existing && Existing->AppliedReceipt.IsValid())
	{
		return Existing->AppliedReceipt.Matches(Receipt)
			? Reject(
				EStatus::ApplicationReplayed,
				TEXT("Exact Arc preview command application replayed."))
			: Reject(
				EStatus::ReceiptConflict,
				TEXT("Arc preview command already has different application evidence."));
	}
	if (!StatesMatchOrAreEmpty(
			CursorState, Receipt.GetCommand().GetPreviousState()))
	{
		return Reject(
			EStatus::CursorMismatch,
			TEXT("Arc preview command does not continue the consumer cursor."));
	}

	const bool bRecovered =
		Existing && Existing->RejectedReceipt.IsValid();
	const auto PreviousLedger = *this;
	FEntry& Entry = Entries.FindOrAdd(Receipt.GetCommandId());
	Entry.AppliedReceipt = Receipt;
	AppliedCommandIds.Add(Receipt.GetCommandId());
	CursorState = Receipt.GetCommand().GetState();
	if (!IsValid())
	{
		*this = PreviousLedger;
		return Reject(
			EStatus::LedgerInvalid,
			TEXT("Arc preview application would invalidate the cursor ledger."));
	}
	return MakeResult(
		bRecovered
			? EStatus::ApplicationRecovered
			: EStatus::ApplicationRecorded,
		Receipt,
		PreviousCursor,
		PreviousAppliedCount,
		PreviousRejectedCount,
		bRecovered
			? TEXT("Applied previously rejected Arc preview command and advanced cursor once.")
			: TEXT("Applied Arc preview command and advanced cursor once."));
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation command ledger is internally invalid.");
		return false;
	}
	if (!IsActive() || !ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation command ledger end requires the active exact Run.");
		return false;
	}
	Reset();
	OutDiagnostic = TEXT(
		"Arc preview presentation command ledger ended and cleared all consumer state.");
	return true;
}

void Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::Reset()
{
	LedgerId.Invalidate();
	RunId.Invalidate();
	ConsumerDefinitionId = NAME_None;
	CursorState = FState();
	Entries.Reset();
	AppliedCommandIds.Reset();
}

int32 Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::
	NumRejectedCommands() const
{
	int32 Count = 0;
	for (const TPair<FGuid, FEntry>& Pair : Entries)
	{
		Count += Pair.Value.RejectedReceipt.IsValid() ? 1 : 0;
	}
	return Count;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::
	HasAppliedCommand(const FGuid& CommandId) const
{
	const FEntry* Entry = Entries.Find(CommandId);
	return Entry && Entry->AppliedReceipt.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::
	HasRejectedCommand(const FGuid& CommandId) const
{
	const FEntry* Entry = Entries.Find(CommandId);
	return Entry && Entry->RejectedReceipt.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::
	TryGetAppliedReceipt(
		const FGuid& CommandId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			OutReceipt) const
{
	OutReceipt = FReceipt();
	const FEntry* Entry = Entries.Find(CommandId);
	if (!Entry || !Entry->AppliedReceipt.IsValid())
	{
		return false;
	}
	OutReceipt = Entry->AppliedReceipt;
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::
	TryGetRejectedReceipt(
		const FGuid& CommandId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			OutReceipt) const
{
	OutReceipt = FReceipt();
	const FEntry* Entry = Entries.Find(CommandId);
	if (!Entry || !Entry->RejectedReceipt.IsValid())
	{
		return false;
	}
	OutReceipt = Entry->RejectedReceipt;
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger::IsValid()
	const
{
	if (!RunId.IsValid())
	{
		return !LedgerId.IsValid() && ConsumerDefinitionId.IsNone()
			&& CursorState.IsEmpty() && Entries.IsEmpty()
			&& AppliedCommandIds.IsEmpty();
	}
	if (!LedgerId.IsValid() || ConsumerDefinitionId.IsNone()
		|| LedgerId != MakeLedgerId(RunId, ConsumerDefinitionId)
		|| !IsUsableState(CursorState)
		|| (CursorState.IsValid() && CursorState.GetRunId() != RunId))
	{
		return false;
	}

	TSet<FGuid> SeenApplied;
	FState ReplayedCursor;
	for (const FGuid& CommandId : AppliedCommandIds)
	{
		if (!CommandId.IsValid() || SeenApplied.Contains(CommandId))
		{
			return false;
		}
		const FEntry* Entry = Entries.Find(CommandId);
		if (!Entry || !Entry->AppliedReceipt.IsApplied())
		{
			return false;
		}
		const FCommand& Command = Entry->AppliedReceipt.GetCommand();
		if (!StatesMatchOrAreEmpty(
				ReplayedCursor, Command.GetPreviousState()))
		{
			return false;
		}
		ReplayedCursor = Command.GetState();
		SeenApplied.Add(CommandId);
	}
	if (!StatesMatchOrAreEmpty(ReplayedCursor, CursorState))
	{
		return false;
	}

	int32 AppliedEntryCount = 0;
	for (const TPair<FGuid, FEntry>& Pair : Entries)
	{
		const FReceipt& Rejected = Pair.Value.RejectedReceipt;
		const FReceipt& Applied = Pair.Value.AppliedReceipt;
		if (!Pair.Key.IsValid()
			|| (!Rejected.IsValid() && !Applied.IsValid()))
		{
			return false;
		}
		if (Rejected.IsValid()
			&& (!Rejected.IsRejected()
				|| Rejected.GetCommandId() != Pair.Key
				|| Rejected.GetRunId() != RunId
				|| Rejected.GetConsumerDefinitionId()
					!= ConsumerDefinitionId))
		{
			return false;
		}
		if (Applied.IsValid()
			&& (!Applied.IsApplied()
				|| Applied.GetCommandId() != Pair.Key
				|| Applied.GetRunId() != RunId
				|| Applied.GetConsumerDefinitionId()
					!= ConsumerDefinitionId
				|| !SeenApplied.Contains(Pair.Key)))
		{
			return false;
		}
		if (Rejected.IsValid() && Applied.IsValid()
			&& !Rejected.GetCommand().Matches(Applied.GetCommand()))
		{
			return false;
		}
		AppliedEntryCount += Applied.IsValid() ? 1 : 0;
	}
	return AppliedEntryCount == AppliedCommandIds.Num();
}
