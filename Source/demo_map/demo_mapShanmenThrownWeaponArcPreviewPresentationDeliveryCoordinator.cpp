#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EDelivery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using ELedger =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using EPortOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome;
	using EReceiptOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FLedger =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger;
	using FLedgerResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult;
	using FPortResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	FName InvalidPortResponseCode()
	{
		return FName(TEXT("Renderer.ArcPreview.InvalidPortResponse"));
	}

	FName LedgerCommitRejectedCode()
	{
		return FName(TEXT("Renderer.ArcPreview.LedgerCommitRejected"));
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownPortOutcome(const EPortOutcome Outcome)
	{
		return Outcome == EPortOutcome::Applied
			|| Outcome == EPortOutcome::Rejected;
	}

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	FGuid MakePortResponseId(
		const FGuid& CommandId,
		const EPortOutcome Outcome,
		const FName OutcomeCode)
	{
		if (!CommandId.IsValid() || !IsKnownPortOutcome(Outcome)
			|| OutcomeCode.IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationPortResponse.r1"),
			{
				GuidDigits(CommandId),
				FString::FromInt(static_cast<int32>(Outcome)),
				OutcomeCode.ToString()
			});
	}

	bool ReceiptMatchesCommandAndConsumer(
		const FReceipt& Receipt,
		const FCommand& Command,
		const FName ConsumerDefinitionId)
	{
		return Receipt.IsValid() && Command.IsValid()
			&& !ConsumerDefinitionId.IsNone()
			&& Receipt.GetCommand().Matches(Command)
			&& Receipt.GetConsumerDefinitionId() == ConsumerDefinitionId;
	}

	bool LedgerResultMatchesReceipt(
		const FLedgerResult& LedgerResult,
		const FReceipt& Receipt)
	{
		return LedgerResult.IsValid() && Receipt.IsValid()
			&& LedgerResult.GetReceipt().Matches(Receipt);
	}

	bool TrySealReceipt(
		const FCommand& Command,
		const FName ConsumerDefinitionId,
		const EReceiptOutcome Outcome,
		const FName OutcomeCode,
		FReceipt& OutReceipt)
	{
		FString Diagnostic;
		return FReceipt::TryCreate(
			Command,
			ConsumerDefinitionId,
			Outcome,
			OutcomeCode,
			OutReceipt,
			Diagnostic);
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse::TryCreate(
	const FGuid& InCommandId,
	const EPortOutcome InOutcome,
	const FName InOutcomeCode,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse& OutResponse,
	FString& OutDiagnostic)
{
	OutResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse();
	OutDiagnostic.Reset();
	if (!InCommandId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation port response requires one command identity.");
		return false;
	}
	if (!IsKnownPortOutcome(InOutcome) || InOutcomeCode.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation port response requires Applied or Rejected and one stable outcome code.");
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse Candidate;
	Candidate.CommandId = InCommandId;
	Candidate.Outcome = InOutcome;
	Candidate.OutcomeCode = InOutcomeCode;
	Candidate.ResponseId = MakePortResponseId(
		Candidate.CommandId, Candidate.Outcome, Candidate.OutcomeCode);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview presentation port response failed deterministic validation.");
		return false;
	}
	OutResponse = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Captured one immutable Arc preview presentation port response.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse::IsValid()
	const
{
	return ResponseId.IsValid() && CommandId.IsValid()
		&& IsKnownPortOutcome(Outcome) && !OutcomeCode.IsNone()
		&& ResponseId == MakePortResponseId(CommandId, Outcome, OutcomeCode);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& ResponseId == Other.ResponseId
		&& CommandId == Other.CommandId
		&& Outcome == Other.Outcome
		&& OutcomeCode == Other.OutcomeCode;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse::IsApplied()
	const
{
	return IsValid() && Outcome == EPortOutcome::Applied;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse::IsRejected()
	const
{
	return IsValid() && Outcome == EPortOutcome::Rejected;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult::IsValid()
	const
{
	if (Status == EDelivery::Invalid || Diagnostic.IsEmpty()
		|| PortCallCount < 0 || PortCallCount > 1)
	{
		return false;
	}

	const bool bHasNoDeliveryEvidence =
		PortCallCount == 0 && !PortResponse.IsValid()
		&& !Receipt.IsValid() && !LedgerResult.IsValid();
	switch (Status)
	{
	case EDelivery::CommandInvalid:
		return !Command.IsValid() && ConsumerDefinitionId.IsNone()
			&& bHasNoDeliveryEvidence;

	case EDelivery::LedgerInvalid:
	case EDelivery::LedgerInactive:
	case EDelivery::RunMismatch:
		return Command.IsValid() && ConsumerDefinitionId.IsNone()
			&& bHasNoDeliveryEvidence;

	case EDelivery::PortConsumerInvalid:
		return Command.IsValid() && ConsumerDefinitionId.IsNone()
			&& bHasNoDeliveryEvidence;

	case EDelivery::ConsumerMismatch:
	case EDelivery::CursorMismatch:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& bHasNoDeliveryEvidence;

	case EDelivery::ApplicationReplayed:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 0 && !PortResponse.IsValid()
			&& ReceiptMatchesCommandAndConsumer(
				Receipt, Command, ConsumerDefinitionId)
			&& Receipt.IsApplied()
			&& LedgerResultMatchesReceipt(LedgerResult, Receipt)
			&& LedgerResult.IsReplay()
			&& !LedgerResult.DidAdvanceCursor()
			&& LedgerResult.GetStatus() == ELedger::ApplicationReplayed;

	case EDelivery::RejectionReplayed:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 0 && !PortResponse.IsValid()
			&& ReceiptMatchesCommandAndConsumer(
				Receipt, Command, ConsumerDefinitionId)
			&& Receipt.IsRejected()
			&& LedgerResultMatchesReceipt(LedgerResult, Receipt)
			&& LedgerResult.IsReplay()
			&& !LedgerResult.DidAdvanceCursor()
			&& LedgerResult.GetStatus() == ELedger::RejectionReplayed;

	case EDelivery::PortApplied:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 1 && PortResponse.IsApplied()
			&& PortResponse.GetCommandId() == Command.GetCommandId()
			&& ReceiptMatchesCommandAndConsumer(
				Receipt, Command, ConsumerDefinitionId)
			&& Receipt.IsApplied()
			&& Receipt.GetOutcomeCode() == PortResponse.GetOutcomeCode()
			&& LedgerResultMatchesReceipt(LedgerResult, Receipt)
			&& LedgerResult.GetStatus() == ELedger::ApplicationRecorded
			&& LedgerResult.DidAdvanceCursor();

	case EDelivery::PortRejected:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 1 && PortResponse.IsRejected()
			&& PortResponse.GetCommandId() == Command.GetCommandId()
			&& ReceiptMatchesCommandAndConsumer(
				Receipt, Command, ConsumerDefinitionId)
			&& Receipt.IsRejected()
			&& Receipt.GetOutcomeCode() == PortResponse.GetOutcomeCode()
			&& LedgerResultMatchesReceipt(LedgerResult, Receipt)
			&& LedgerResult.GetStatus() == ELedger::RejectionRecorded
			&& !LedgerResult.DidAdvanceCursor();

	case EDelivery::PortResponseRejected:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 1
			&& (!PortResponse.IsValid()
				|| PortResponse.GetCommandId() != Command.GetCommandId())
			&& ReceiptMatchesCommandAndConsumer(
				Receipt, Command, ConsumerDefinitionId)
			&& Receipt.IsRejected()
			&& Receipt.GetOutcomeCode() == InvalidPortResponseCode()
			&& LedgerResultMatchesReceipt(LedgerResult, Receipt)
			&& LedgerResult.GetStatus() == ELedger::RejectionRecorded
			&& !LedgerResult.DidAdvanceCursor();

	case EDelivery::LedgerCommitRejected:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 1
			&& ReceiptMatchesCommandAndConsumer(
				Receipt, Command, ConsumerDefinitionId)
			&& Receipt.IsRejected()
			&& Receipt.GetOutcomeCode() == LedgerCommitRejectedCode()
			&& LedgerResultMatchesReceipt(LedgerResult, Receipt)
			&& LedgerResult.GetStatus() == ELedger::RejectionRecorded
			&& !LedgerResult.DidAdvanceCursor();

	case EDelivery::InvariantViolation:
		return Command.IsValid() && !ConsumerDefinitionId.IsNone()
			&& PortCallCount == 1
			&& (!Receipt.IsValid() || !LedgerResult.IsValid()
				|| !LedgerResult.IsAccepted());

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult::
	IsAccepted() const
{
	return IsValid()
		&& (Status == EDelivery::ApplicationReplayed
			|| Status == EDelivery::RejectionReplayed
			|| Status == EDelivery::PortApplied
			|| Status == EDelivery::PortRejected
			|| Status == EDelivery::PortResponseRejected
			|| Status == EDelivery::LedgerCommitRejected);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult::
	WasApplied() const
{
	return IsValid()
		&& (Status == EDelivery::ApplicationReplayed
			|| Status == EDelivery::PortApplied);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult::
	WasRejected() const
{
	return IsValid()
		&& (Status == EDelivery::RejectionReplayed
			|| Status == EDelivery::PortRejected
			|| Status == EDelivery::PortResponseRejected
			|| Status == EDelivery::LedgerCommitRejected);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult::
	DidCallPort() const
{
	return IsValid() && PortCallCount == 1;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult::IsReplay()
	const
{
	return IsValid()
		&& (Status == EDelivery::ApplicationReplayed
			|| Status == EDelivery::RejectionReplayed);
}

FResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator::
	MakeResult(
		const EDelivery Status,
		const TCHAR* Diagnostic,
		const FCommand& Command,
		const FName ConsumerDefinitionId,
		const int32 PortCallCount,
		const FPortResponse& PortResponse,
		const FReceipt& Receipt,
		const FLedgerResult& LedgerResult)
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Command = Command;
	Result.ConsumerDefinitionId = ConsumerDefinitionId;
	Result.PortCallCount = PortCallCount;
	Result.PortResponse = PortResponse;
	Result.Receipt = Receipt;
	Result.LedgerResult = LedgerResult;
	return Result;
}

FResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator::Deliver(
	const FCommand& Command,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port,
	FLedger& Ledger)
{
	if (!Command.IsValid())
	{
		return MakeResult(
			EDelivery::CommandInvalid,
			TEXT("Arc preview delivery requires one valid command."),
			Command);
	}
	if (!Ledger.IsValid())
	{
		return MakeResult(
			EDelivery::LedgerInvalid,
			TEXT("Arc preview delivery requires a valid consumer ledger."),
			Command);
	}
	if (!Ledger.IsActive())
	{
		return MakeResult(
			EDelivery::LedgerInactive,
			TEXT("Arc preview delivery requires an active consumer ledger."),
			Command);
	}
	if (Command.GetRunId() != Ledger.GetRunId())
	{
		return MakeResult(
			EDelivery::RunMismatch,
			TEXT("Arc preview command belongs to another Run."),
			Command);
	}

	const FName ConsumerDefinitionId = Port.GetConsumerDefinitionId();
	if (ConsumerDefinitionId.IsNone())
	{
		return MakeResult(
			EDelivery::PortConsumerInvalid,
			TEXT("Arc preview presentation port has no stable consumer identity."),
			Command);
	}
	if (ConsumerDefinitionId != Ledger.GetConsumerDefinitionId())
	{
		return MakeResult(
			EDelivery::ConsumerMismatch,
			TEXT("Arc preview presentation port belongs to another consumer."),
			Command,
			ConsumerDefinitionId);
	}

	FReceipt ExistingReceipt;
	if (Ledger.TryGetAppliedReceipt(Command.GetCommandId(), ExistingReceipt))
	{
		FLedger Candidate = Ledger;
		const FLedgerResult LedgerResult = Candidate.Record(ExistingReceipt);
		return MakeResult(
			EDelivery::ApplicationReplayed,
			TEXT("Arc preview command was already applied; replayed without a port call."),
			Command,
			ConsumerDefinitionId,
			0,
			FPortResponse(),
			ExistingReceipt,
			LedgerResult);
	}
	if (Ledger.TryGetRejectedReceipt(Command.GetCommandId(), ExistingReceipt))
	{
		FLedger Candidate = Ledger;
		const FLedgerResult LedgerResult = Candidate.Record(ExistingReceipt);
		return MakeResult(
			EDelivery::RejectionReplayed,
			TEXT("Arc preview command was already rejected; replayed without a port call."),
			Command,
			ConsumerDefinitionId,
			0,
			FPortResponse(),
			ExistingReceipt,
			LedgerResult);
	}
	if (!StatesMatchOrAreEmpty(
			Ledger.GetCursorState(), Command.GetPreviousState()))
	{
		return MakeResult(
			EDelivery::CursorMismatch,
			TEXT("Arc preview command does not continue the consumer cursor."),
			Command,
			ConsumerDefinitionId);
	}

	FLedger Candidate = Ledger;
	const FPortResponse PortResponse = Port.Apply(Command);
	const bool bResponseMatches = PortResponse.IsValid()
		&& PortResponse.GetCommandId() == Command.GetCommandId();
	const EReceiptOutcome ReceiptOutcome =
		bResponseMatches && PortResponse.IsApplied()
			? EReceiptOutcome::Applied
			: EReceiptOutcome::Rejected;
	const FName ReceiptCode = bResponseMatches
		? PortResponse.GetOutcomeCode()
		: InvalidPortResponseCode();
	const EDelivery DeliveryStatus = !bResponseMatches
		? EDelivery::PortResponseRejected
		: PortResponse.IsApplied()
			? EDelivery::PortApplied
			: EDelivery::PortRejected;

	FReceipt Receipt;
	if (!TrySealReceipt(
			Command,
			ConsumerDefinitionId,
			ReceiptOutcome,
			ReceiptCode,
			Receipt))
	{
		return MakeResult(
			EDelivery::InvariantViolation,
			TEXT("Arc preview delivery could not seal its terminal receipt."),
			Command,
			ConsumerDefinitionId,
			1,
			PortResponse);
	}

	FLedgerResult LedgerResult = Candidate.Record(Receipt);
	if (!LedgerResult.IsAccepted())
	{
		Candidate = Ledger;
		FReceipt FailureReceipt;
		if (!TrySealReceipt(
				Command,
				ConsumerDefinitionId,
				EReceiptOutcome::Rejected,
				LedgerCommitRejectedCode(),
				FailureReceipt))
		{
			return MakeResult(
				EDelivery::InvariantViolation,
				TEXT("Arc preview delivery could not seal ledger failure evidence."),
				Command,
				ConsumerDefinitionId,
				1,
				PortResponse);
		}
		LedgerResult = Candidate.Record(FailureReceipt);
		if (!LedgerResult.IsAccepted())
		{
			return MakeResult(
				EDelivery::InvariantViolation,
				TEXT("Arc preview delivery ledger rejected terminal failure evidence."),
				Command,
				ConsumerDefinitionId,
				1,
				PortResponse,
				FailureReceipt,
				LedgerResult);
		}
		Ledger = MoveTemp(Candidate);
		return MakeResult(
			EDelivery::LedgerCommitRejected,
			TEXT("Arc preview delivery recorded a terminal ledger commit rejection."),
			Command,
			ConsumerDefinitionId,
			1,
			PortResponse,
			FailureReceipt,
			LedgerResult);
	}

	Ledger = MoveTemp(Candidate);
	const TCHAR* Diagnostic = DeliveryStatus == EDelivery::PortApplied
		? TEXT("Arc preview presentation port applied and recorded one command.")
		: DeliveryStatus == EDelivery::PortRejected
			? TEXT("Arc preview presentation port rejected and recorded one command.")
			: TEXT("Invalid Arc preview port response was recorded as a terminal rejection.");
	return MakeResult(
		DeliveryStatus,
		Diagnostic,
		Command,
		ConsumerDefinitionId,
		1,
		PortResponse,
		Receipt,
		LedgerResult);
}
