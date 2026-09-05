#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession.h"

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus
	: uint8
{
	Invalid,
	HostInactive,
	HostInvalid,
	OperationInProgress,
	RecoveryRequired,
	StateUpdateRejected,
	ProjectionRejected,
	DeliveryRejected,
	RejectedPendingRecovery,
	Applied,
	ApplicationReplayed,
	InvariantViolation
};

/** Self-validating audit result for one bounded update -> command -> delivery. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasApplied() const;
	bool WasRejected() const;
	bool IsReplay() const;
	bool NeedsRecovery() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	int32 GetStateUpdateCallCount() const { return StateUpdateCallCount; }
	int32 GetProjectionCallCount() const { return ProjectionCallCount; }
	int32 GetDeliveryCallCount() const { return DeliveryCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
	GetStateUpdate() const
	{
		return StateUpdate;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult&
	GetProjection() const
	{
		return Projection;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult&
	GetDelivery() const
	{
		return Delivery;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousState() const
	{
		return PreviousState;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
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
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetPreviousPendingCommand() const
	{
		return PreviousPendingCommand;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetPendingCommand() const
	{
		return PendingCommand;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus::
				Invalid;
	FString Diagnostic;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	int32 StateUpdateCallCount = 0;
	int32 ProjectionCallCount = 0;
	int32 DeliveryCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
		StateUpdate;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult
		Projection;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult
		Delivery;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousCursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState CursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand
		PreviousPendingCommand;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand PendingCommand;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus
	: uint8
{
	Invalid,
	HostInactive,
	HostInvalid,
	OperationInProgress,
	NoRecoveryPending,
	ReceiptMismatch,
	DeliveryRecoveryRejected,
	Recovered,
	RecoveryReplayed,
	InvariantViolation
};

/** Self-validating audit result for one bounded Host recovery attempt. */
class
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool DidRecover() const;
	bool IsReplay() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	int32 GetDeliveryRecoveryCallCount() const
	{
		return DeliveryRecoveryCallCount;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult&
	GetDeliveryRecovery() const
	{
		return DeliveryRecovery;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
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
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetPreviousPendingCommand() const
	{
		return PreviousPendingCommand;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetPendingCommand() const
	{
		return PendingCommand;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus::
				Invalid;
	FString Diagnostic;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	int32 DeliveryRecoveryCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult
		DeliveryRecovery;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousCursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState CursorState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand
		PreviousPendingCommand;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand PendingCommand;
};

/**
 * Run-scoped owner for one Arc preview state and one consumer delivery cursor.
 *
 * Every TryUpdate uses candidate Sessions and performs at most one state
 * update, one pure command projection and one delivery call, in that order.
 * Applied delivery commits synchronized state/cursor. Rejected delivery
 * commits the rejection plus its exact desired state, then fences further
 * updates until the narrow recovery seam reconciles the cursor.
 *
 * The Host owns no World, Actor, widget, component, input or product state.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost
{
public:
	bool TryBegin(
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult
	TryUpdate(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult
	TryRecoverRejected(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			AppliedReceipt);

	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);

	bool IsValid() const;
	bool IsActive() const { return PresentationSession.IsActive(); }
	bool IsEmpty() const { return IsValid() && !IsActive(); }
	bool IsOperationInProgress() const { return bOperationInProgress; }
	bool NeedsRecovery() const { return PendingRejectedCommand.IsValid(); }
	bool IsSynchronized() const;
	bool CanEnd() const;
	const FGuid& GetRunId() const { return PresentationSession.GetRunId(); }
	FName GetConsumerDefinitionId() const
	{
		return DeliverySession.GetConsumerDefinitionId();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return PresentationSession.GetState();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetCursorState() const
	{
		return DeliverySession.GetCursorState();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetPendingRejectedCommand() const
	{
		return PendingRejectedCommand;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession&
	GetPresentationSession() const
	{
		return PresentationSession;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession&
	GetDeliverySession() const
	{
		return DeliverySession;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus
			Status,
		const TCHAR* Diagnostic,
		int32 StateUpdateCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult&
			StateUpdate,
		int32 ProjectionCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult&
			Projection,
		int32 DeliveryCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult&
			Delivery,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousState,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousCursorState,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
			PreviousPendingCommand) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult
	MakeRecoveryResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus
			Status,
		const TCHAR* Diagnostic,
		int32 DeliveryRecoveryCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult&
			DeliveryRecovery,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousCursorState,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
			PreviousPendingCommand) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession
		PresentationSession;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession
		DeliverySession;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand
		PendingRejectedCommand;
	bool bOperationInProgress = false;
};
