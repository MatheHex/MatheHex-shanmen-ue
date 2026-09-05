#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus
	: uint8
{
	Invalid,
	OwnerInactive,
	OwnerInvalid,
	OperationInProgress,
	HostRejected,
	RejectedPendingRecovery,
	Applied,
	ApplicationReplayed,
	InvariantViolation
};

/** Self-validating evidence for one bounded owner update attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasApplied() const;
	bool WasRejected() const;
	bool IsReplay() const;
	bool NeedsRecovery() const;
	bool DidCallHost() const;
	bool DidCallAdapter() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	int32 GetHostUpdateCallCount() const { return HostUpdateCallCount; }
	int32 GetAdapterApplyCallCount() const { return AdapterApplyCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult&
	GetHostResult() const
	{
		return HostResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult&
	GetAdapterResult() const
	{
		return AdapterResult;
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
	GetPreviousHostCursor() const
	{
		return PreviousHostCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetHostCursor() const
	{
		return HostCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousSurfaceCursor() const
	{
		return PreviousSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
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
	bool IsOwnerValidAfter() const { return bOwnerValidAfter; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus::
				Invalid;
	FString Diagnostic;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	int32 HostUpdateCallCount = 0;
	int32 AdapterApplyCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult
		HostResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult
		AdapterResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousHostCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState HostCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand
		PreviousPendingCommand;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand PendingCommand;
	bool bOwnerValidAfter = false;
	bool bValidated = false;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus
	: uint8
{
	Invalid,
	OwnerInactive,
	OwnerInvalid,
	OperationInProgress,
	NoRecoveryPending,
	AdapterRejected,
	HostRecoveryRejected,
	Recovered,
	InvariantViolation
};

/** Self-validating evidence for one bounded surface-to-Host recovery attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool DidRecover() const;
	bool WasRejected() const;
	bool DidCallAdapter() const;
	bool DidCallHostRecovery() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	int32 GetAdapterApplyCallCount() const { return AdapterApplyCallCount; }
	int32 GetHostRecoveryCallCount() const { return HostRecoveryCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult&
	GetAdapterResult() const
	{
		return AdapterResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
	GetAppliedReceipt() const
	{
		return AppliedReceipt;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult&
	GetHostRecoveryResult() const
	{
		return HostRecoveryResult;
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
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousHostCursor() const
	{
		return PreviousHostCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetHostCursor() const
	{
		return HostCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousSurfaceCursor() const
	{
		return PreviousSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}
	bool IsOwnerValidAfter() const { return bOwnerValidAfter; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus::
				Invalid;
	FString Diagnostic;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	int32 AdapterApplyCallCount = 0;
	int32 HostRecoveryCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult
		AdapterResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt
		AppliedReceipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult
		HostRecoveryResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand
		PreviousPendingCommand;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand PendingCommand;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousHostCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState HostCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
	bool bOwnerValidAfter = false;
	bool bValidated = false;
};

/**
 * Run-scoped owner of one delivery Host and its renderer-facing Adapter.
 *
 * Begin and end bind both children atomically. Update always delegates through
 * the owned Adapter. Recovery retries exactly the Host's pending command once
 * on the surface and forwards the resulting Applied receipt once to the Host.
 * The bound surface is non-owning and must outlive the active owner scope.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner
{
public:
	bool TryBegin(
		const FGuid& RunId,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface& Surface,
		FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult
	TryUpdate(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryResult
	TryRecoverRejected();

	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);

	bool IsValid() const;
	bool IsActive() const { return Surface != nullptr; }
	bool IsEmpty() const { return IsValid() && !IsActive(); }
	bool IsOperationInProgress() const { return bOperationInProgress; }
	bool NeedsRecovery() const { return Host.NeedsRecovery(); }
	bool IsSynchronized() const;
	bool CanEnd() const;
	const FGuid& GetRunId() const { return Host.GetRunId(); }
	FName GetConsumerDefinitionId() const
	{
		return Host.GetConsumerDefinitionId();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return Host.GetState();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetHostCursor() const
	{
		return Host.GetCursorState();
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState GetSurfaceCursor()
		const
	{
		return Adapter.GetSurfaceCursor();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetPendingRejectedCommand() const
	{
		return Host.GetPendingRejectedCommand();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost&
	GetHost() const
	{
		return Host;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter&
	GetAdapter() const
	{
		return Adapter;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult
	MakeUpdateResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus
			Status,
		const TCHAR* Diagnostic,
		int32 HostUpdateCallCount,
		int32 AdapterApplyCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult&
			HostResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult&
			AdapterResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousState,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousHostCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
			PreviousPendingCommand) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryResult
	MakeRecoveryResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus
			Status,
		const TCHAR* Diagnostic,
		int32 AdapterApplyCallCount,
		int32 HostRecoveryCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult&
			AdapterResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			AppliedReceipt,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult&
			HostRecoveryResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
			PreviousPendingCommand,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousHostCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter Adapter;
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface* Surface = nullptr;
	bool bOperationInProgress = false;
};
