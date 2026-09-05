#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus
	: uint8
{
	Invalid,
	SessionInactive,
	SessionInvalid,
	DeliveryInProgress,
	CommandInvalid,
	RunMismatch,
	DeliveryRejected,
	Applied,
	Rejected,
	ApplicationReplayed,
	RejectionReplayed,
	InvariantViolation
};

/** Self-validating audit result for one serialized Session delivery attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasApplied() const;
	bool WasRejected() const;
	bool IsReplay() const;
	bool DidCallCoordinator() const;
	bool DidCallPort() const;
	bool DidAdvanceCursor() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetSessionRunId() const { return SessionRunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& GetCommand()
		const
	{
		return Command;
	}
	int32 GetCoordinatorCallCount() const { return CoordinatorCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult&
	GetDelivery() const
	{
		return Delivery;
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
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus::
				Invalid;
	FString Diagnostic;
	FGuid SessionRunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand Command;
	int32 CoordinatorCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult Delivery;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousCursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState CursorState;
	int32 PreviousAppliedCount = 0;
	int32 AppliedCount = 0;
	int32 PreviousRejectedCount = 0;
	int32 RejectedCount = 0;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus
	: uint8
{
	Invalid,
	SessionInactive,
	SessionInvalid,
	SessionBusy,
	ReceiptInvalid,
	AppliedReceiptRequired,
	RunMismatch,
	ConsumerMismatch,
	RejectionNotFound,
	RejectedCommandMismatch,
	LedgerRejected,
	Recovered,
	RecoveryReplayed,
	InvariantViolation
};

/** Self-validating audit result for one externally attested recovery attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool DidRecover() const;
	bool IsReplay() const;
	bool DidRecordLedger() const;
	bool DidAdvanceCursor() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetSessionRunId() const { return SessionRunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	int32 GetLedgerRecordCallCount() const { return LedgerRecordCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult&
	GetLedgerResult() const
	{
		return LedgerResult;
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
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus::
				Invalid;
	FString Diagnostic;
	FGuid SessionRunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt Receipt;
	int32 LedgerRecordCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult
		LedgerResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousCursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState CursorState;
	int32 PreviousAppliedCount = 0;
	int32 AppliedCount = 0;
	int32 PreviousRejectedCount = 0;
	int32 RejectedCount = 0;
};

/**
 * Consumer-owned, Run-scoped owner for one serialized command stream.
 *
 * The Session privately owns the P20.37 ledger and delegates each accepted
 * attempt to the P20.38 coordinator at most once. Re-entrant delivery is
 * rejected before another coordinator or port call. Graceful end is allowed
 * only while the consumer cursor is Hidden or Empty, so dropping C++ state
 * cannot be mistaken for proving that a visible renderer was cleared.
 * A narrow recovery seam accepts caller-provided Applied evidence only for an
 * exact command already carrying Rejected evidence in this private ledger.
 *
 * This in-memory object is deliberately not a cross-thread or crash-durable
 * queue. It owns no World, Actor, widget, component, input or product state.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession
{
public:
	bool TryBegin(
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult
	TryDeliver(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult
	TryRecoverRejected(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			AppliedReceipt);

	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const { return IsValid() && !IsActive(); }
	bool IsDeliveryInProgress() const { return bDeliveryInProgress; }
	bool CanEnd() const;
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const FGuid& GetLedgerId() const { return Ledger.GetLedgerId(); }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetCursorState() const
	{
		return Ledger.GetCursorState();
	}
	int32 NumAppliedCommands() const { return Ledger.NumAppliedCommands(); }
	int32 NumRejectedCommands() const { return Ledger.NumRejectedCommands(); }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger&
	GetLedger() const
	{
		return Ledger;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		int32 CoordinatorCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult&
			Delivery,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousCursorState,
		int32 PreviousAppliedCount,
		int32 PreviousRejectedCount) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult
	MakeRecoveryResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			Receipt,
		int32 LedgerRecordCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult&
			LedgerResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousCursorState,
		int32 PreviousAppliedCount,
		int32 PreviousRejectedCount) const;

	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	bool bDeliveryInProgress = false;
};
