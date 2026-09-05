#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommand.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome
	: uint8
{
	Invalid,
	Applied,
	Rejected
};

/**
 * Immutable caller attestation for one exact renderer-neutral command.
 *
 * Applied means the named consumer reports that it consumed the command;
 * Rejected preserves one stable failure code. This value deliberately does
 * not claim that a widget, renderer, component or visible frame existed.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		FName ConsumerDefinitionId,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome
			Outcome,
		FName OutcomeCode,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			OutReceipt,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			Other) const;
	bool IsApplied() const;
	bool IsRejected() const;

	const FGuid& GetReceiptId() const { return ReceiptId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& GetCommand()
		const
	{
		return Command;
	}
	const FGuid& GetCommandId() const { return Command.GetCommandId(); }
	const FGuid& GetRunId() const { return Command.GetRunId(); }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	FName GetOutcomeCode() const { return OutcomeCode; }

private:
	FGuid ReceiptId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand Command;
	FName ConsumerDefinitionId = NAME_None;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome::
				Invalid;
	FName OutcomeCode = NAME_None;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus
	: uint8
{
	Invalid,
	LedgerInactive,
	LedgerInvalid,
	ReceiptInvalid,
	RunMismatch,
	ConsumerMismatch,
	CursorMismatch,
	ReceiptConflict,
	RejectionRecorded,
	RejectionReplayed,
	ApplicationRecorded,
	ApplicationRecovered,
	ApplicationReplayed
};

/** Self-validating audit result for one bounded ledger mutation attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool DidAdvanceCursor() const;
	bool IsReplay() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousCursorState() const
	{
		return PreviousCursorState;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetCursorState() const
	{
		return CursorState;
	}
	int32 GetPreviousAppliedCount() const { return PreviousAppliedCount; }
	int32 GetAppliedCount() const { return AppliedCount; }
	int32 GetPreviousRejectedCount() const { return PreviousRejectedCount; }
	int32 GetRejectedCount() const { return RejectedCount; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt Receipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousCursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState CursorState;
	int32 PreviousAppliedCount = 0;
	int32 AppliedCount = 0;
	int32 PreviousRejectedCount = 0;
	int32 RejectedCount = 0;
};

/**
 * Consumer-owned in-memory acknowledgement and cursor ledger.
 *
 * Rejections are auditable but do not advance the cursor. A later Applied
 * receipt for that exact command may recover and advance once. Exact receipt
 * replay is idempotent; conflicting evidence fails closed. Applied commands
 * must form one contiguous previous-state to current-state chain.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger
{
public:
	bool TryBegin(
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult Record(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			Receipt);

	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const { return IsValid() && !IsActive(); }
	const FGuid& GetLedgerId() const { return LedgerId; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetCursorState() const
	{
		return CursorState;
	}
	FGuid GetLastAppliedCommandId() const
	{
		return AppliedCommandIds.IsEmpty()
			? FGuid()
			: AppliedCommandIds.Last();
	}
	int32 NumAppliedCommands() const { return AppliedCommandIds.Num(); }
	int32 NumRejectedCommands() const;
	bool HasAppliedCommand(const FGuid& CommandId) const;
	bool HasRejectedCommand(const FGuid& CommandId) const;
	bool TryGetAppliedReceipt(
		const FGuid& CommandId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			OutReceipt) const;
	bool TryGetRejectedReceipt(
		const FGuid& CommandId,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			OutReceipt) const;

private:
	struct FEntry
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt
			RejectedReceipt;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt
			AppliedReceipt;
	};

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus
			Status,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			Receipt,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousCursorState,
		int32 PreviousAppliedCount,
		int32 PreviousRejectedCount,
		const TCHAR* Diagnostic) const;

	FGuid LedgerId;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState CursorState;
	TMap<FGuid, FEntry> Entries;
	TArray<FGuid> AppliedCommandIds;
};
