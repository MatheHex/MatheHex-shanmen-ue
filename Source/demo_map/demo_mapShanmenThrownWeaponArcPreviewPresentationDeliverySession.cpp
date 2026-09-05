#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession.h"

namespace
{
	using EDelivery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using ELedger =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus;
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FDelivery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult;
	using FLedger =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger;
	using FLedgerResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	using FRecoveryResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult;
	using FSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	bool IsUsableState(const FState& State)
	{
		return State.IsEmpty() || State.IsValid();
	}

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool DeliveryMatchesCommand(
		const FDelivery& Delivery,
		const FCommand& Command)
	{
		return Delivery.IsValid() && Command.IsValid()
			&& Delivery.GetCommand().Matches(Command);
	}

	bool DeliveryLedgerMatchesResult(
		const FDelivery& Delivery,
		const FResult& Result)
	{
		if (!Delivery.IsValid() || !Delivery.GetLedgerResult().IsValid())
		{
			return false;
		}
		const auto& LedgerResult = Delivery.GetLedgerResult();
		return StatesMatchOrAreEmpty(
				LedgerResult.GetPreviousCursorState(),
				Result.GetPreviousCursorState())
			&& StatesMatchOrAreEmpty(
				LedgerResult.GetCursorState(), Result.GetCursorState())
			&& LedgerResult.GetPreviousAppliedCount()
				== Result.GetPreviousAppliedCount()
			&& LedgerResult.GetAppliedCount() == Result.GetAppliedCount()
			&& LedgerResult.GetPreviousRejectedCount()
				== Result.GetPreviousRejectedCount()
			&& LedgerResult.GetRejectedCount() == Result.GetRejectedCount();
	}

	bool RecoveryLedgerMatchesResult(
		const FLedgerResult& LedgerResult,
		const FRecoveryResult& Result)
	{
		return LedgerResult.IsValid() && Result.GetReceipt().IsValid()
			&& LedgerResult.GetReceipt().Matches(Result.GetReceipt())
			&& StatesMatchOrAreEmpty(
				LedgerResult.GetPreviousCursorState(),
				Result.GetPreviousCursorState())
			&& StatesMatchOrAreEmpty(
				LedgerResult.GetCursorState(), Result.GetCursorState())
			&& LedgerResult.GetPreviousAppliedCount()
				== Result.GetPreviousAppliedCount()
			&& LedgerResult.GetAppliedCount() == Result.GetAppliedCount()
			&& LedgerResult.GetPreviousRejectedCount()
				== Result.GetPreviousRejectedCount()
			&& LedgerResult.GetRejectedCount() == Result.GetRejectedCount();
	}
}

bool FResult::IsValid() const
{
	if (Status == ESession::Invalid || Diagnostic.IsEmpty()
		|| CoordinatorCallCount < 0 || CoordinatorCallCount > 1
		|| !IsUsableState(PreviousCursorState)
		|| !IsUsableState(CursorState)
		|| PreviousAppliedCount < 0 || AppliedCount < 0
		|| PreviousRejectedCount < 0 || RejectedCount < 0)
	{
		return false;
	}

	const bool bHasScope =
		SessionRunId.IsValid() && !ConsumerDefinitionId.IsNone();
	const bool bUnchanged =
		StatesMatchOrAreEmpty(PreviousCursorState, CursorState)
		&& PreviousAppliedCount == AppliedCount
		&& PreviousRejectedCount == RejectedCount;
	const bool bNoCoordinatorEvidence =
		CoordinatorCallCount == 0 && !Delivery.IsValid();

	switch (Status)
	{
	case ESession::SessionInactive:
		return !SessionRunId.IsValid() && ConsumerDefinitionId.IsNone()
			&& PreviousCursorState.IsEmpty() && CursorState.IsEmpty()
			&& AppliedCount == 0 && RejectedCount == 0
			&& bNoCoordinatorEvidence;

	case ESession::SessionInvalid:
		return bUnchanged && bNoCoordinatorEvidence;

	case ESession::DeliveryInProgress:
		return bHasScope && Command.IsValid()
			&& Command.GetRunId() == SessionRunId
			&& bUnchanged && bNoCoordinatorEvidence;

	case ESession::CommandInvalid:
		return bHasScope && !Command.IsValid()
			&& bUnchanged && bNoCoordinatorEvidence;

	case ESession::RunMismatch:
		return bHasScope && Command.IsValid()
			&& Command.GetRunId() != SessionRunId
			&& bUnchanged && bNoCoordinatorEvidence;

	case ESession::DeliveryRejected:
		return bHasScope && CoordinatorCallCount == 1
			&& DeliveryMatchesCommand(Delivery, Command)
			&& !Delivery.IsAccepted()
			&& Delivery.GetStatus() != EDelivery::InvariantViolation
			&& bUnchanged;

	case ESession::Applied:
		return bHasScope && CoordinatorCallCount == 1
			&& DeliveryMatchesCommand(Delivery, Command)
			&& Delivery.GetConsumerDefinitionId() == ConsumerDefinitionId
			&& Delivery.IsAccepted() && Delivery.WasApplied()
			&& !Delivery.IsReplay() && Delivery.DidCallPort()
			&& Delivery.GetStatus() == EDelivery::PortApplied
			&& DeliveryLedgerMatchesResult(Delivery, *this)
			&& AppliedCount == PreviousAppliedCount + 1
			&& RejectedCount == PreviousRejectedCount;

	case ESession::Rejected:
		return bHasScope && CoordinatorCallCount == 1
			&& DeliveryMatchesCommand(Delivery, Command)
			&& Delivery.GetConsumerDefinitionId() == ConsumerDefinitionId
			&& Delivery.IsAccepted() && Delivery.WasRejected()
			&& !Delivery.IsReplay() && Delivery.DidCallPort()
			&& DeliveryLedgerMatchesResult(Delivery, *this)
			&& StatesMatchOrAreEmpty(PreviousCursorState, CursorState)
			&& AppliedCount == PreviousAppliedCount
			&& RejectedCount == PreviousRejectedCount + 1;

	case ESession::ApplicationReplayed:
		return bHasScope && CoordinatorCallCount == 1
			&& DeliveryMatchesCommand(Delivery, Command)
			&& Delivery.GetConsumerDefinitionId() == ConsumerDefinitionId
			&& Delivery.IsAccepted() && Delivery.WasApplied()
			&& Delivery.IsReplay() && !Delivery.DidCallPort()
			&& Delivery.GetStatus() == EDelivery::ApplicationReplayed
			&& DeliveryLedgerMatchesResult(Delivery, *this)
			&& bUnchanged;

	case ESession::RejectionReplayed:
		return bHasScope && CoordinatorCallCount == 1
			&& DeliveryMatchesCommand(Delivery, Command)
			&& Delivery.GetConsumerDefinitionId() == ConsumerDefinitionId
			&& Delivery.IsAccepted() && Delivery.WasRejected()
			&& Delivery.IsReplay() && !Delivery.DidCallPort()
			&& Delivery.GetStatus() == EDelivery::RejectionReplayed
			&& DeliveryLedgerMatchesResult(Delivery, *this)
			&& bUnchanged;

	case ESession::InvariantViolation:
		return bHasScope && CoordinatorCallCount == 1
			&& Command.IsValid() && Command.GetRunId() == SessionRunId
			&& bUnchanged
			&& (!Delivery.IsValid()
				|| Delivery.GetStatus() == EDelivery::InvariantViolation
				|| Delivery.IsAccepted());

	default:
		return false;
	}
}

bool FResult::IsAccepted() const
{
	return IsValid()
		&& (Status == ESession::Applied
			|| Status == ESession::Rejected
			|| Status == ESession::ApplicationReplayed
			|| Status == ESession::RejectionReplayed);
}

bool FResult::WasApplied() const
{
	return IsValid()
		&& (Status == ESession::Applied
			|| Status == ESession::ApplicationReplayed);
}

bool FResult::WasRejected() const
{
	return IsValid()
		&& (Status == ESession::Rejected
			|| Status == ESession::RejectionReplayed);
}

bool FResult::IsReplay() const
{
	return IsValid()
		&& (Status == ESession::ApplicationReplayed
			|| Status == ESession::RejectionReplayed);
}

bool FResult::DidCallCoordinator() const
{
	return IsValid() && CoordinatorCallCount == 1;
}

bool FResult::DidCallPort() const
{
	return IsValid() && Delivery.IsValid() && Delivery.DidCallPort();
}

bool FResult::DidAdvanceCursor() const
{
	return IsValid() && Delivery.IsValid()
		&& Delivery.GetLedgerResult().IsValid()
		&& Delivery.GetLedgerResult().DidAdvanceCursor();
}

bool FRecoveryResult::IsValid() const
{
	if (Status == ERecovery::Invalid || Diagnostic.IsEmpty()
		|| LedgerRecordCallCount < 0 || LedgerRecordCallCount > 1
		|| !IsUsableState(PreviousCursorState)
		|| !IsUsableState(CursorState)
		|| PreviousAppliedCount < 0 || AppliedCount < 0
		|| PreviousRejectedCount < 0 || RejectedCount < 0)
	{
		return false;
	}

	const bool bHasScope =
		SessionRunId.IsValid() && !ConsumerDefinitionId.IsNone();
	const bool bReceiptMatchesScope =
		bHasScope && Receipt.IsValid() && Receipt.IsApplied()
		&& Receipt.GetRunId() == SessionRunId
		&& Receipt.GetConsumerDefinitionId() == ConsumerDefinitionId;
	const bool bUnchanged =
		StatesMatchOrAreEmpty(PreviousCursorState, CursorState)
		&& PreviousAppliedCount == AppliedCount
		&& PreviousRejectedCount == RejectedCount;
	const bool bNoLedgerEvidence =
		LedgerRecordCallCount == 0 && !LedgerResult.IsValid();
	const bool bLedgerMatches =
		LedgerRecordCallCount == 1
		&& RecoveryLedgerMatchesResult(LedgerResult, *this);

	switch (Status)
	{
	case ERecovery::SessionInactive:
		return !SessionRunId.IsValid() && ConsumerDefinitionId.IsNone()
			&& PreviousCursorState.IsEmpty() && CursorState.IsEmpty()
			&& AppliedCount == 0 && RejectedCount == 0
			&& bNoLedgerEvidence;

	case ERecovery::SessionInvalid:
		return bUnchanged && bNoLedgerEvidence;

	case ERecovery::SessionBusy:
		return bHasScope && bUnchanged && bNoLedgerEvidence;

	case ERecovery::ReceiptInvalid:
		return bHasScope && !Receipt.IsValid()
			&& bUnchanged && bNoLedgerEvidence;

	case ERecovery::AppliedReceiptRequired:
		return bHasScope && Receipt.IsValid() && !Receipt.IsApplied()
			&& bUnchanged && bNoLedgerEvidence;

	case ERecovery::RunMismatch:
		return bHasScope && Receipt.IsApplied()
			&& Receipt.GetRunId() != SessionRunId
			&& bUnchanged && bNoLedgerEvidence;

	case ERecovery::ConsumerMismatch:
		return bHasScope && Receipt.IsApplied()
			&& Receipt.GetRunId() == SessionRunId
			&& Receipt.GetConsumerDefinitionId() != ConsumerDefinitionId
			&& bUnchanged && bNoLedgerEvidence;

	case ERecovery::RejectionNotFound:
	case ERecovery::RejectedCommandMismatch:
		return bReceiptMatchesScope && bUnchanged && bNoLedgerEvidence;

	case ERecovery::LedgerRejected:
		return bReceiptMatchesScope && bLedgerMatches
			&& !LedgerResult.IsAccepted() && bUnchanged;

	case ERecovery::Recovered:
		return bReceiptMatchesScope && bLedgerMatches
			&& LedgerResult.IsAccepted()
			&& LedgerResult.GetStatus() == ELedger::ApplicationRecovered
			&& LedgerResult.DidAdvanceCursor()
			&& !LedgerResult.IsReplay()
			&& AppliedCount == PreviousAppliedCount + 1
			&& RejectedCount == PreviousRejectedCount;

	case ERecovery::RecoveryReplayed:
		return bReceiptMatchesScope && bLedgerMatches
			&& LedgerResult.IsAccepted()
			&& LedgerResult.GetStatus() == ELedger::ApplicationReplayed
			&& !LedgerResult.DidAdvanceCursor()
			&& LedgerResult.IsReplay() && bUnchanged;

	case ERecovery::InvariantViolation:
		return bReceiptMatchesScope && LedgerRecordCallCount == 1
			&& bUnchanged
			&& (!LedgerResult.IsValid() || !bLedgerMatches
				|| LedgerResult.IsAccepted());

	default:
		return false;
	}
}

bool FRecoveryResult::IsAccepted() const
{
	return IsValid()
		&& (Status == ERecovery::Recovered
			|| Status == ERecovery::RecoveryReplayed);
}

bool FRecoveryResult::DidRecover() const
{
	return IsValid() && Status == ERecovery::Recovered;
}

bool FRecoveryResult::IsReplay() const
{
	return IsValid() && Status == ERecovery::RecoveryReplayed;
}

bool FRecoveryResult::DidRecordLedger() const
{
	return IsValid() && LedgerRecordCallCount == 1;
}

bool FRecoveryResult::DidAdvanceCursor() const
{
	return IsValid() && LedgerResult.IsValid()
		&& LedgerResult.DidAdvanceCursor();
}

FResult FSession::MakeResult(
	const ESession Status,
	const TCHAR* Diagnostic,
	const FCommand& Command,
	const int32 CoordinatorCallCount,
	const FDelivery& Delivery,
	const FState& PreviousCursorState,
	const int32 PreviousAppliedCount,
	const int32 PreviousRejectedCount) const
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SessionRunId = RunId;
	Result.ConsumerDefinitionId = ConsumerDefinitionId;
	Result.Command = Command;
	Result.CoordinatorCallCount = CoordinatorCallCount;
	Result.Delivery = Delivery;
	Result.PreviousCursorState = PreviousCursorState;
	Result.CursorState = Ledger.GetCursorState();
	Result.PreviousAppliedCount = PreviousAppliedCount;
	Result.AppliedCount = Ledger.NumAppliedCommands();
	Result.PreviousRejectedCount = PreviousRejectedCount;
	Result.RejectedCount = Ledger.NumRejectedCommands();
	return Result;
}

FRecoveryResult FSession::MakeRecoveryResult(
	const ERecovery Status,
	const TCHAR* Diagnostic,
	const FReceipt& Receipt,
	const int32 LedgerRecordCallCount,
	const FLedgerResult& LedgerResult,
	const FState& PreviousCursorState,
	const int32 PreviousAppliedCount,
	const int32 PreviousRejectedCount) const
{
	FRecoveryResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SessionRunId = RunId;
	Result.ConsumerDefinitionId = ConsumerDefinitionId;
	Result.Receipt = Receipt;
	Result.LedgerRecordCallCount = LedgerRecordCallCount;
	Result.LedgerResult = LedgerResult;
	Result.PreviousCursorState = PreviousCursorState;
	Result.CursorState = Ledger.GetCursorState();
	Result.PreviousAppliedCount = PreviousAppliedCount;
	Result.AppliedCount = Ledger.NumAppliedCommands();
	Result.PreviousRejectedCount = PreviousRejectedCount;
	Result.RejectedCount = Ledger.NumRejectedCommands();
	return Result;
}

bool FSession::TryBegin(
	const FGuid& RequestedRunId,
	const FName RequestedConsumerDefinitionId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || bDeliveryInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Session requires stable idle state before begin.");
		return false;
	}
	if (!RequestedRunId.IsValid() || RequestedConsumerDefinitionId.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Session requires one Run and consumer identity.");
		return false;
	}
	if (IsActive())
	{
		if (RunId == RequestedRunId
			&& ConsumerDefinitionId == RequestedConsumerDefinitionId)
		{
			OutDiagnostic = TEXT(
				"Arc preview delivery Session already owns this exact scope.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Active Arc preview delivery Session rejects scope rotation.");
		return false;
	}

	FLedger CandidateLedger;
	FString LedgerDiagnostic;
	if (!CandidateLedger.TryBegin(
			RequestedRunId,
			RequestedConsumerDefinitionId,
			LedgerDiagnostic))
	{
		OutDiagnostic = LedgerDiagnostic.IsEmpty()
			? TEXT("Arc preview delivery Session could not create its ledger.")
			: LedgerDiagnostic;
		return false;
	}

	RunId = RequestedRunId;
	ConsumerDefinitionId = RequestedConsumerDefinitionId;
	Ledger = MoveTemp(CandidateLedger);
	if (!IsValid())
	{
		*this = FSession();
		OutDiagnostic = TEXT(
			"Arc preview delivery Session failed closed during scope binding.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Arc preview delivery Session created one Run-scoped consumer ledger.");
	return true;
}

FResult FSession::TryDeliver(
	const FCommand& Command,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port)
{
	const FState PreviousCursorState = Ledger.GetCursorState();
	const int32 PreviousAppliedCount = Ledger.NumAppliedCommands();
	const int32 PreviousRejectedCount = Ledger.NumRejectedCommands();
	auto RejectBeforeCoordinator = [this, &Command, &PreviousCursorState,
		PreviousAppliedCount, PreviousRejectedCount](
		const ESession Status,
		const TCHAR* Diagnostic)
	{
		return MakeResult(
			Status,
			Diagnostic,
			Command,
			0,
			FDelivery(),
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	};

	if (!IsValid())
	{
		return RejectBeforeCoordinator(
			ESession::SessionInvalid,
			TEXT("Arc preview delivery Session invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeCoordinator(
			ESession::SessionInactive,
			TEXT("Arc preview delivery requires one active Session."));
	}
	if (bDeliveryInProgress)
	{
		return RejectBeforeCoordinator(
			ESession::DeliveryInProgress,
			TEXT("Arc preview delivery Session rejects re-entrant delivery."));
	}
	if (!Command.IsValid())
	{
		return RejectBeforeCoordinator(
			ESession::CommandInvalid,
			TEXT("Arc preview delivery Session requires one valid command."));
	}
	if (Command.GetRunId() != RunId)
	{
		return RejectBeforeCoordinator(
			ESession::RunMismatch,
			TEXT("Arc preview command belongs to another Session Run."));
	}

	const FLedger PreviousLedger = Ledger;
	FLedger CandidateLedger = Ledger;
	FDelivery Delivery;
	{
		TGuardValue<bool> DeliveryGuard(bDeliveryInProgress, true);
		Delivery =
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator::
				Deliver(Command, Port, CandidateLedger);
	}
	if (!Delivery.IsValid()
		|| Delivery.GetStatus() == EDelivery::InvariantViolation)
	{
		return MakeResult(
			ESession::InvariantViolation,
			TEXT("Arc preview delivery coordinator returned invalid terminal evidence."),
			Command,
			1,
			Delivery,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}
	if (!Delivery.IsAccepted())
	{
		return MakeResult(
			ESession::DeliveryRejected,
			Delivery.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview delivery coordinator rejected the command.")
				: *Delivery.GetDiagnostic(),
			Command,
			1,
			Delivery,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}
	if (!CandidateLedger.IsValid()
		|| !CandidateLedger.IsActive()
		|| CandidateLedger.GetRunId() != RunId
		|| CandidateLedger.GetConsumerDefinitionId()
			!= ConsumerDefinitionId)
	{
		return MakeResult(
			ESession::InvariantViolation,
			TEXT("Arc preview delivery candidate ledger failed Session validation."),
			Command,
			1,
			Delivery,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}

	Ledger = MoveTemp(CandidateLedger);
	ESession Status = ESession::InvariantViolation;
	if (Delivery.IsReplay())
	{
		Status = Delivery.WasApplied()
			? ESession::ApplicationReplayed
			: ESession::RejectionReplayed;
	}
	else if (Delivery.WasApplied())
	{
		Status = ESession::Applied;
	}
	else if (Delivery.WasRejected())
	{
		Status = ESession::Rejected;
	}

	const FResult Result = MakeResult(
		Status,
		Delivery.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview delivery Session completed one command attempt.")
			: *Delivery.GetDiagnostic(),
		Command,
		1,
		Delivery,
		PreviousCursorState,
		PreviousAppliedCount,
		PreviousRejectedCount);
	if (Status == ESession::InvariantViolation || !IsValid()
		|| !Result.IsValid())
	{
		Ledger = PreviousLedger;
		return MakeResult(
			ESession::InvariantViolation,
			TEXT("Arc preview delivery Session rejected an invalid post-commit result."),
			Command,
			1,
			Delivery,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}
	return Result;
}

FRecoveryResult FSession::TryRecoverRejected(
	const FReceipt& AppliedReceipt)
{
	const FState PreviousCursorState = Ledger.GetCursorState();
	const int32 PreviousAppliedCount = Ledger.NumAppliedCommands();
	const int32 PreviousRejectedCount = Ledger.NumRejectedCommands();
	auto RejectBeforeLedger = [this, &AppliedReceipt, &PreviousCursorState,
		PreviousAppliedCount, PreviousRejectedCount](
		const ERecovery Status,
		const TCHAR* Diagnostic)
	{
		return MakeRecoveryResult(
			Status,
			Diagnostic,
			AppliedReceipt,
			0,
			FLedgerResult(),
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	};

	if (!IsValid())
	{
		return RejectBeforeLedger(
			ERecovery::SessionInvalid,
			TEXT("Arc preview recovery Session invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeLedger(
			ERecovery::SessionInactive,
			TEXT("Arc preview recovery requires one active Session."));
	}
	if (bDeliveryInProgress)
	{
		return RejectBeforeLedger(
			ERecovery::SessionBusy,
			TEXT("Arc preview recovery cannot interleave with delivery."));
	}
	if (!AppliedReceipt.IsValid())
	{
		return RejectBeforeLedger(
			ERecovery::ReceiptInvalid,
			TEXT("Arc preview recovery requires one valid receipt."));
	}
	if (!AppliedReceipt.IsApplied())
	{
		return RejectBeforeLedger(
			ERecovery::AppliedReceiptRequired,
			TEXT("Arc preview recovery requires Applied evidence."));
	}
	if (AppliedReceipt.GetRunId() != RunId)
	{
		return RejectBeforeLedger(
			ERecovery::RunMismatch,
			TEXT("Arc preview recovery receipt belongs to another Run."));
	}
	if (AppliedReceipt.GetConsumerDefinitionId() != ConsumerDefinitionId)
	{
		return RejectBeforeLedger(
			ERecovery::ConsumerMismatch,
			TEXT("Arc preview recovery receipt belongs to another consumer."));
	}

	FReceipt RejectedReceipt;
	if (!Ledger.TryGetRejectedReceipt(
			AppliedReceipt.GetCommandId(), RejectedReceipt))
	{
		return RejectBeforeLedger(
			ERecovery::RejectionNotFound,
			TEXT("Arc preview recovery requires prior rejection evidence for the exact command."));
	}
	if (!RejectedReceipt.GetCommand().Matches(AppliedReceipt.GetCommand()))
	{
		return RejectBeforeLedger(
			ERecovery::RejectedCommandMismatch,
			TEXT("Arc preview recovery receipt does not match the rejected command."));
	}

	const FLedger PreviousLedger = Ledger;
	FLedger CandidateLedger = Ledger;
	const FLedgerResult LedgerResult = CandidateLedger.Record(AppliedReceipt);
	if (!LedgerResult.IsValid())
	{
		return MakeRecoveryResult(
			ERecovery::InvariantViolation,
			TEXT("Arc preview recovery ledger returned invalid evidence."),
			AppliedReceipt,
			1,
			LedgerResult,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}
	if (!LedgerResult.IsAccepted())
	{
		return MakeRecoveryResult(
			ERecovery::LedgerRejected,
			LedgerResult.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview recovery ledger rejected Applied evidence.")
				: *LedgerResult.GetDiagnostic(),
			AppliedReceipt,
			1,
			LedgerResult,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}

	ERecovery Status = ERecovery::InvariantViolation;
	if (LedgerResult.GetStatus() == ELedger::ApplicationRecovered)
	{
		Status = ERecovery::Recovered;
	}
	else if (LedgerResult.GetStatus() == ELedger::ApplicationReplayed)
	{
		Status = ERecovery::RecoveryReplayed;
	}
	if (Status == ERecovery::InvariantViolation
		|| !CandidateLedger.IsValid() || !CandidateLedger.IsActive()
		|| CandidateLedger.GetRunId() != RunId
		|| CandidateLedger.GetConsumerDefinitionId()
			!= ConsumerDefinitionId)
	{
		return MakeRecoveryResult(
			ERecovery::InvariantViolation,
			TEXT("Arc preview recovery candidate ledger failed Session validation."),
			AppliedReceipt,
			1,
			LedgerResult,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}

	Ledger = MoveTemp(CandidateLedger);
	const FRecoveryResult Result = MakeRecoveryResult(
		Status,
		LedgerResult.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview recovery accepted external Applied evidence.")
			: *LedgerResult.GetDiagnostic(),
		AppliedReceipt,
		1,
		LedgerResult,
		PreviousCursorState,
		PreviousAppliedCount,
		PreviousRejectedCount);
	if (!IsValid() || !Result.IsValid())
	{
		Ledger = PreviousLedger;
		return MakeRecoveryResult(
			ERecovery::InvariantViolation,
			TEXT("Arc preview recovery rejected an invalid post-commit result."),
			AppliedReceipt,
			1,
			LedgerResult,
			PreviousCursorState,
			PreviousAppliedCount,
			PreviousRejectedCount);
	}
	return Result;
}

bool FSession::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Session end requires valid state and Run identity.");
		return false;
	}
	if (bDeliveryInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Session cannot end during a delivery call.");
		return false;
	}
	if (!IsActive())
	{
		OutDiagnostic = TEXT("Arc preview delivery Session is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Session rejects mismatched Run teardown.");
		return false;
	}
	if (!CanEnd())
	{
		OutDiagnostic = TEXT(
			"Visible Arc preview cursor must be hidden before Session end.");
		return false;
	}

	FSession Candidate = *this;
	FString LedgerDiagnostic;
	if (!Candidate.Ledger.TryEnd(ExpectedRunId, LedgerDiagnostic))
	{
		OutDiagnostic = LedgerDiagnostic.IsEmpty()
			? TEXT("Arc preview delivery ledger rejected Session end.")
			: LedgerDiagnostic;
		return false;
	}
	Candidate.RunId.Invalidate();
	Candidate.ConsumerDefinitionId = NAME_None;
	if (!Candidate.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Session failed empty-state validation.");
		return false;
	}
	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview delivery Session ended from a hidden or empty cursor.");
	return true;
}

bool FSession::CanEnd() const
{
	if (!IsValid() || bDeliveryInProgress)
	{
		return false;
	}
	if (!IsActive())
	{
		return true;
	}
	const FState& Cursor = Ledger.GetCursorState();
	return Cursor.IsEmpty() || Cursor.IsHidden();
}

bool FSession::IsValid() const
{
	if (!RunId.IsValid())
	{
		return ConsumerDefinitionId.IsNone()
			&& !bDeliveryInProgress
			&& Ledger.IsValid() && Ledger.IsEmpty();
	}
	return !ConsumerDefinitionId.IsNone()
		&& Ledger.IsValid() && Ledger.IsActive()
		&& Ledger.GetRunId() == RunId
		&& Ledger.GetConsumerDefinitionId() == ConsumerDefinitionId;
}
