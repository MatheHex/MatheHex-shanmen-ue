#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner.h"

namespace
{
	using EOwnerUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	using EOwnerRecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus;
	using EHostUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	using EHostRecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus;
	using EAdapter =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus;
	using EReceipt =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using FAdapterResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FHostResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult;
	using FHostRecovery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult;
	using FOwner =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;
	using FOwnerUpdate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;
	using FOwnerRecovery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryResult;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
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

	bool CommandsMatchOrAreEmpty(const FCommand& Left, const FCommand& Right)
	{
		return (!Left.IsValid() && !Right.IsValid()) || Left.Matches(Right);
	}

	FState PhysicalCursorFor(const FState& HostCursor)
	{
		return HostCursor.IsVisible() ? HostCursor : FState();
	}

	bool AdapterEvidenceMatchesHost(
		const FHostResult& HostResult,
		const bool bHostResultValid,
		const int32 AdapterApplyCallCount,
		const FAdapterResult& AdapterResult)
	{
		if (!bHostResultValid)
		{
			return AdapterApplyCallCount == 0 && !AdapterResult.IsValid();
		}
		const bool bHostCalledAdapter = HostResult.GetDeliveryCallCount() == 1
			&& HostResult.GetDelivery().IsValid()
			&& HostResult.GetDelivery().DidCallPort();
		if (!bHostCalledAdapter)
		{
			return AdapterApplyCallCount == 0 && !AdapterResult.IsValid();
		}
		if (AdapterApplyCallCount != 1 || !AdapterResult.IsValid()
			|| !HostResult.GetProjection().IsValid())
		{
			return false;
		}
		const auto& Delivery = HostResult.GetDelivery().GetDelivery();
		return Delivery.IsValid()
			&& AdapterResult.GetCommand().Matches(
				HostResult.GetProjection().GetCommand())
			&& AdapterResult.GetPortResponse().Matches(
				Delivery.GetPortResponse());
	}

	bool AdapterEvidenceShapeMatchesHost(
		const FHostResult& HostResult,
		const bool bHostResultPresent,
		const int32 AdapterApplyCallCount,
		const FAdapterResult& AdapterResult)
	{
		const bool bAdapterResultPresent =
			AdapterResult.GetStatus() != EAdapter::Invalid
			&& !AdapterResult.GetDiagnostic().IsEmpty();
		if (!bHostResultPresent)
		{
			return AdapterApplyCallCount == 0 && !bAdapterResultPresent;
		}
		const auto& DeliverySession = HostResult.GetDelivery();
		const auto& Delivery = DeliverySession.GetDelivery();
		const bool bHostCalledAdapter = HostResult.GetDeliveryCallCount() == 1
			&& DeliverySession.GetCoordinatorCallCount() == 1
			&& Delivery.GetPortCallCount() == 1;
		if (!bHostCalledAdapter)
		{
			return AdapterApplyCallCount == 0 && !bAdapterResultPresent;
		}
		return AdapterApplyCallCount == 1 && bAdapterResultPresent
			&& AdapterResult.GetCommand().Matches(
				HostResult.GetProjection().GetCommand())
			&& AdapterResult.GetPortResponse().Matches(
				Delivery.GetPortResponse());
	}
}

bool FOwnerUpdate::IsValid() const
{
	return bValidated;
}

bool FOwnerUpdate::Validate() const
{
	if (Status == EOwnerUpdate::Invalid || Diagnostic.IsEmpty()
		|| HostUpdateCallCount < 0 || HostUpdateCallCount > 1
		|| AdapterApplyCallCount < 0 || AdapterApplyCallCount > 1
		|| AdapterApplyCallCount > HostUpdateCallCount
		|| !IsUsableState(PreviousState) || !IsUsableState(State)
		|| !IsUsableState(PreviousHostCursor)
		|| !IsUsableState(HostCursor)
		|| !IsUsableState(PreviousSurfaceCursor)
		|| !IsUsableState(SurfaceCursor))
	{
		return false;
	}

	const bool bHasScope = RunId.IsValid() && !ConsumerDefinitionId.IsNone();
	const bool bHostResultPresent =
		HostResult.GetStatus() != EHostUpdate::Invalid
		&& !HostResult.GetDiagnostic().IsEmpty();
	const bool bAdapterResultPresent =
		AdapterResult.GetStatus() != EAdapter::Invalid
		&& !AdapterResult.GetDiagnostic().IsEmpty();
	const bool bNoCalls = HostUpdateCallCount == 0
		&& AdapterApplyCallCount == 0
		&& !bHostResultPresent && !bAdapterResultPresent;
	const bool bOneHostCall = HostUpdateCallCount == 1;
	const bool bHostResultValid = bHostResultPresent;
	const bool bNestedMatch = bOneHostCall
		&& AdapterEvidenceShapeMatchesHost(
			HostResult,
			bHostResultValid,
			AdapterApplyCallCount,
			AdapterResult);
	const bool bUnchanged = StatesMatchOrAreEmpty(PreviousState, State)
		&& StatesMatchOrAreEmpty(PreviousHostCursor, HostCursor)
		&& StatesMatchOrAreEmpty(
			PreviousSurfaceCursor, SurfaceCursor)
		&& CommandsMatchOrAreEmpty(
			PreviousPendingCommand, PendingCommand);
	const bool bCursorsSynchronized = StatesMatchOrAreEmpty(
		PhysicalCursorFor(HostCursor), SurfaceCursor);

	switch (Status)
	{
	case EOwnerUpdate::OwnerInactive:
		return !bHasScope && bOwnerValidAfter && bNoCalls
			&& PreviousState.IsEmpty() && State.IsEmpty()
			&& PreviousHostCursor.IsEmpty() && HostCursor.IsEmpty()
			&& PreviousSurfaceCursor.IsEmpty() && SurfaceCursor.IsEmpty()
			&& !PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid();

	case EOwnerUpdate::OwnerInvalid:
		return !bOwnerValidAfter && bNoCalls && bUnchanged;

	case EOwnerUpdate::OperationInProgress:
		return bHasScope && bOwnerValidAfter && bNoCalls && bUnchanged;

	case EOwnerUpdate::HostRejected:
		return bHasScope && bOwnerValidAfter && bNestedMatch
			&& bHostResultValid
			&& HostResult.GetStatus() != EHostUpdate::RejectedPendingRecovery
			&& HostResult.GetStatus() != EHostUpdate::Applied
			&& HostResult.GetStatus() != EHostUpdate::ApplicationReplayed;

	case EOwnerUpdate::RejectedPendingRecovery:
		return bHasScope && bOwnerValidAfter && bNestedMatch
			&& HostResult.GetStatus() == EHostUpdate::RejectedPendingRecovery
			&& PendingCommand.IsValid()
			&& bCursorsSynchronized;

	case EOwnerUpdate::Applied:
		return bHasScope && bOwnerValidAfter && bNestedMatch
			&& HostResult.GetStatus() == EHostUpdate::Applied
			&& !PendingCommand.IsValid() && bCursorsSynchronized;

	case EOwnerUpdate::ApplicationReplayed:
		return bHasScope && bOwnerValidAfter && bNestedMatch
			&& HostResult.GetStatus() == EHostUpdate::ApplicationReplayed
			&& AdapterApplyCallCount == 0
			&& !PendingCommand.IsValid() && bCursorsSynchronized;

	case EOwnerUpdate::InvariantViolation:
		return bHasScope && bOneHostCall
			&& (!bHostResultValid || !bOwnerValidAfter
				|| !bNestedMatch);

	default:
		return false;
	}
}

bool FOwnerUpdate::IsAccepted() const
{
	return IsValid()
		&& (Status == EOwnerUpdate::RejectedPendingRecovery
			|| Status == EOwnerUpdate::Applied
			|| Status == EOwnerUpdate::ApplicationReplayed);
}

bool FOwnerUpdate::WasApplied() const
{
	return IsValid()
		&& (Status == EOwnerUpdate::Applied
			|| Status == EOwnerUpdate::ApplicationReplayed);
}

bool FOwnerUpdate::WasRejected() const
{
	return IsValid() && Status == EOwnerUpdate::RejectedPendingRecovery;
}

bool FOwnerUpdate::IsReplay() const
{
	return IsValid() && Status == EOwnerUpdate::ApplicationReplayed;
}

bool FOwnerUpdate::NeedsRecovery() const
{
	return IsValid()
		&& (Status == EOwnerUpdate::RejectedPendingRecovery
			|| (HostResult.IsValid() && HostResult.NeedsRecovery()));
}

bool FOwnerUpdate::DidCallHost() const
{
	return IsValid() && HostUpdateCallCount == 1;
}

bool FOwnerUpdate::DidCallAdapter() const
{
	return IsValid() && AdapterApplyCallCount == 1;
}

bool FOwnerRecovery::IsValid() const
{
	return bValidated;
}

bool FOwnerRecovery::Validate() const
{
	if (Status == EOwnerRecovery::Invalid || Diagnostic.IsEmpty()
		|| AdapterApplyCallCount < 0 || AdapterApplyCallCount > 1
		|| HostRecoveryCallCount < 0 || HostRecoveryCallCount > 1
		|| HostRecoveryCallCount > AdapterApplyCallCount
		|| !IsUsableState(PreviousHostCursor)
		|| !IsUsableState(HostCursor)
		|| !IsUsableState(PreviousSurfaceCursor)
		|| !IsUsableState(SurfaceCursor))
	{
		return false;
	}

	const bool bHasScope = RunId.IsValid() && !ConsumerDefinitionId.IsNone();
	const bool bAdapterResultPresent =
		AdapterResult.GetStatus() != EAdapter::Invalid
		&& !AdapterResult.GetDiagnostic().IsEmpty();
	const bool bHostRecoveryResultPresent =
		HostRecoveryResult.GetStatus() != EHostRecovery::Invalid
		&& !HostRecoveryResult.GetDiagnostic().IsEmpty();
	const bool bNoCalls = AdapterApplyCallCount == 0
		&& HostRecoveryCallCount == 0 && !bAdapterResultPresent
		&& !AppliedReceipt.IsValid() && !bHostRecoveryResultPresent;
	const bool bAdapterResultValid = bAdapterResultPresent;
	const bool bHostRecoveryResultValid = bHostRecoveryResultPresent;
	const bool bAdapterCalled = AdapterApplyCallCount == 1
		&& bAdapterResultValid
		&& AdapterResult.GetCommand().Matches(PreviousPendingCommand);
	const bool bHostCalled = HostRecoveryCallCount == 1
		&& bHostRecoveryResultValid;
	const bool bPendingUnchanged = CommandsMatchOrAreEmpty(
		PreviousPendingCommand, PendingCommand);
	const bool bHostCursorUnchanged = StatesMatchOrAreEmpty(
		PreviousHostCursor, HostCursor);
	const bool bSurfaceCursorUnchanged = StatesMatchOrAreEmpty(
		PreviousSurfaceCursor, SurfaceCursor);
	const bool bCursorsSynchronized = StatesMatchOrAreEmpty(
		PhysicalCursorFor(HostCursor), SurfaceCursor);

	switch (Status)
	{
	case EOwnerRecovery::OwnerInactive:
		return !bHasScope && bOwnerValidAfter && bNoCalls
			&& !PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid()
			&& PreviousHostCursor.IsEmpty() && HostCursor.IsEmpty()
			&& PreviousSurfaceCursor.IsEmpty() && SurfaceCursor.IsEmpty();

	case EOwnerRecovery::OwnerInvalid:
		return !bOwnerValidAfter && bNoCalls && bPendingUnchanged
			&& bHostCursorUnchanged && bSurfaceCursorUnchanged;

	case EOwnerRecovery::OperationInProgress:
	case EOwnerRecovery::NoRecoveryPending:
		return bHasScope && bOwnerValidAfter && bNoCalls
			&& bPendingUnchanged && bHostCursorUnchanged
			&& bSurfaceCursorUnchanged;

	case EOwnerRecovery::AdapterRejected:
		return bHasScope && bOwnerValidAfter && bAdapterCalled
			&& AdapterResult.GetPortResponse().IsRejected()
			&& !AppliedReceipt.IsValid() && !HostRecoveryResult.IsValid()
			&& PreviousPendingCommand.IsValid() && bPendingUnchanged
			&& bHostCursorUnchanged && bSurfaceCursorUnchanged
			&& bCursorsSynchronized;

	case EOwnerRecovery::HostRecoveryRejected:
		return bHasScope && bAdapterCalled
			&& AdapterResult.GetPortResponse().IsApplied()
			&& AppliedReceipt.IsValid() && AppliedReceipt.IsApplied()
			&& bHostCalled
			&& HostRecoveryResult.GetStatus() != EHostRecovery::Recovered
			&& HostRecoveryResult.GetStatus()
				!= EHostRecovery::RecoveryReplayed;

	case EOwnerRecovery::Recovered:
		return bHasScope && bOwnerValidAfter && bAdapterCalled
			&& AdapterResult.GetPortResponse().IsApplied()
			&& AppliedReceipt.IsValid() && AppliedReceipt.IsApplied()
			&& AppliedReceipt.GetCommand().Matches(
				PreviousPendingCommand)
			&& bHostCalled
			&& HostRecoveryResult.GetStatus() == EHostRecovery::Recovered
			&& PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid() && bCursorsSynchronized;

	case EOwnerRecovery::InvariantViolation:
		return bHasScope && AdapterApplyCallCount == 1
			&& (!bOwnerValidAfter || !bAdapterCalled
				|| (HostRecoveryCallCount == 1
					&& !bHostRecoveryResultValid));

	default:
		return false;
	}
}

bool FOwnerRecovery::IsAccepted() const
{
	return IsValid() && Status == EOwnerRecovery::Recovered;
}

bool FOwnerRecovery::DidRecover() const
{
	return IsAccepted();
}

bool FOwnerRecovery::WasRejected() const
{
	return IsValid()
		&& (Status == EOwnerRecovery::AdapterRejected
			|| Status == EOwnerRecovery::HostRecoveryRejected);
}

bool FOwnerRecovery::DidCallAdapter() const
{
	return IsValid() && AdapterApplyCallCount == 1;
}

bool FOwnerRecovery::DidCallHostRecovery() const
{
	return IsValid() && HostRecoveryCallCount == 1;
}

bool FOwner::TryBegin(
	const FGuid& RequestedRunId,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface& RequestedSurface,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	const FName RequestedConsumer =
		RequestedSurface.GetConsumerDefinitionId();
	if (!IsValid() || bOperationInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner requires stable valid state before begin.");
		return false;
	}
	if (!RequestedRunId.IsValid() || RequestedConsumer.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner requires one Run and surface consumer identity.");
		return false;
	}
	if (IsActive())
	{
		if (GetRunId() == RequestedRunId
			&& GetConsumerDefinitionId() == RequestedConsumer
			&& Surface == &RequestedSurface)
		{
			OutDiagnostic = TEXT(
				"Arc preview composition owner already owns this exact scope.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Active Arc preview composition owner rejects scope or surface rotation.");
		return false;
	}

	FOwner Candidate;
	FString NestedDiagnostic;
	if (!Candidate.Adapter.TryBegin(
			RequestedRunId, RequestedSurface, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	if (!Candidate.Host.TryBegin(
			RequestedRunId, RequestedConsumer, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	Candidate.Surface = &RequestedSurface;
	if (!Candidate.IsValid() || !Candidate.IsSynchronized())
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner failed joint scope validation.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview composition owner atomically bound its Host and surface Adapter.");
	return true;
}

FOwnerUpdate FOwner::TryUpdate(
	const int32 HotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const int32 SegmentCount,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator)
{
	const FState PreviousState = GetState();
	const FState PreviousHostCursor = GetHostCursor();
	const FState PreviousSurfaceCursor = GetSurfaceCursor();
	const FCommand PreviousPendingCommand = GetPendingRejectedCommand();
	const auto RejectBeforeHost = [this, &PreviousState,
		&PreviousHostCursor, &PreviousSurfaceCursor,
		&PreviousPendingCommand](
		const EOwnerUpdate Status, const TCHAR* Diagnostic)
	{
		return MakeUpdateResult(
			Status,
			Diagnostic,
			0,
			0,
			FHostResult(),
			FAdapterResult(),
			PreviousState,
			PreviousHostCursor,
			PreviousSurfaceCursor,
			PreviousPendingCommand);
	};

	if (!IsValid())
	{
		return RejectBeforeHost(
			EOwnerUpdate::OwnerInvalid,
			TEXT("Arc preview composition owner invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeHost(
			EOwnerUpdate::OwnerInactive,
			TEXT("Arc preview composition owner update requires an active Run."));
	}
	if (bOperationInProgress)
	{
		return RejectBeforeHost(
			EOwnerUpdate::OperationInProgress,
			TEXT("Arc preview composition owner rejects re-entrant operations."));
	}

	FHostResult HostResult;
	{
		TGuardValue<bool> OperationGuard(bOperationInProgress, true);
		HostResult = Host.TryUpdate(
			HotbarSlotNumber,
			CurrentChoice,
			ChoicePolicy,
			SegmentCount,
			SourceBasis,
			Lifecycle,
			Coordinator,
			Adapter);
	}

	const bool bHostResultValid = HostResult.IsValid();
	const bool bAdapterCalled = bHostResultValid
		&& HostResult.GetDeliveryCallCount() == 1
		&& HostResult.GetDelivery().IsValid()
		&& HostResult.GetDelivery().DidCallPort();
	const int32 AdapterApplyCallCount = bAdapterCalled ? 1 : 0;
	const FAdapterResult AdapterResult = bAdapterCalled
		? Adapter.GetLastResult()
		: FAdapterResult();
	const bool bNestedEvidenceValid = AdapterEvidenceMatchesHost(
		HostResult,
		bHostResultValid,
		AdapterApplyCallCount,
		AdapterResult);
	if (!bHostResultValid || !IsValid()
		|| !bNestedEvidenceValid)
	{
		return MakeUpdateResult(
			EOwnerUpdate::InvariantViolation,
			TEXT("Arc preview composition owner detected divergent Host and surface evidence."),
			1,
			AdapterApplyCallCount,
			HostResult,
			AdapterResult,
			PreviousState,
			PreviousHostCursor,
			PreviousSurfaceCursor,
			PreviousPendingCommand);
	}

	const EHostUpdate HostStatus = HostResult.GetStatus();
	const EOwnerUpdate Status =
		HostStatus == EHostUpdate::RejectedPendingRecovery
		? EOwnerUpdate::RejectedPendingRecovery
		: HostStatus == EHostUpdate::ApplicationReplayed
			? EOwnerUpdate::ApplicationReplayed
			: HostStatus == EHostUpdate::Applied
				? EOwnerUpdate::Applied
			: EOwnerUpdate::HostRejected;
	return MakeUpdateResult(
		Status,
		HostResult.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview composition owner completed one bounded update.")
			: *HostResult.GetDiagnostic(),
		1,
		AdapterApplyCallCount,
		HostResult,
		AdapterResult,
		PreviousState,
		PreviousHostCursor,
		PreviousSurfaceCursor,
		PreviousPendingCommand);
}

FOwnerRecovery FOwner::TryRecoverRejected()
{
	const FCommand PreviousPendingCommand = GetPendingRejectedCommand();
	const FState PreviousHostCursor = GetHostCursor();
	const FState PreviousSurfaceCursor = GetSurfaceCursor();
	const auto RejectBeforeAdapter = [this, &PreviousPendingCommand,
		&PreviousHostCursor, &PreviousSurfaceCursor](
		const EOwnerRecovery Status, const TCHAR* Diagnostic)
	{
		return MakeRecoveryResult(
			Status,
			Diagnostic,
			0,
			0,
			FAdapterResult(),
			FReceipt(),
			FHostRecovery(),
			PreviousPendingCommand,
			PreviousHostCursor,
			PreviousSurfaceCursor);
	};

	if (!IsValid())
	{
		return RejectBeforeAdapter(
			EOwnerRecovery::OwnerInvalid,
			TEXT("Arc preview composition owner recovery invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeAdapter(
			EOwnerRecovery::OwnerInactive,
			TEXT("Arc preview composition owner recovery requires an active Run."));
	}
	if (bOperationInProgress)
	{
		return RejectBeforeAdapter(
			EOwnerRecovery::OperationInProgress,
			TEXT("Arc preview composition owner rejects re-entrant recovery."));
	}
	if (!PreviousPendingCommand.IsValid())
	{
		return RejectBeforeAdapter(
			EOwnerRecovery::NoRecoveryPending,
			TEXT("Arc preview composition owner has no rejected command to recover."));
	}

	FAdapterResult AdapterResult;
	FReceipt AppliedReceipt;
	FHostRecovery HostRecovery;
	int32 HostRecoveryCallCount = 0;
	{
		TGuardValue<bool> OperationGuard(bOperationInProgress, true);
		const auto PortResponse = Adapter.Apply(PreviousPendingCommand);
		AdapterResult = Adapter.GetLastResult();
		if (!AdapterResult.IsValid()
			|| !AdapterResult.GetCommand().Matches(PreviousPendingCommand)
			|| !AdapterResult.GetPortResponse().Matches(PortResponse))
		{
			return MakeRecoveryResult(
				EOwnerRecovery::InvariantViolation,
				TEXT("Arc preview composition owner received invalid Adapter recovery evidence."),
				1,
				0,
				AdapterResult,
				AppliedReceipt,
				HostRecovery,
				PreviousPendingCommand,
				PreviousHostCursor,
				PreviousSurfaceCursor);
		}
		if (PortResponse.IsRejected())
		{
			const EOwnerRecovery Status = IsValid()
				? EOwnerRecovery::AdapterRejected
				: EOwnerRecovery::InvariantViolation;
			return MakeRecoveryResult(
				Status,
				IsValid()
					? TEXT("Arc preview surface rejected one bounded recovery retry.")
					: TEXT("Arc preview surface rejection produced divergent physical state."),
				1,
				0,
				AdapterResult,
				AppliedReceipt,
				HostRecovery,
				PreviousPendingCommand,
				PreviousHostCursor,
				PreviousSurfaceCursor);
		}

		FString ReceiptDiagnostic;
		if (!PortResponse.IsApplied()
			|| !FReceipt::TryCreate(
				PreviousPendingCommand,
				GetConsumerDefinitionId(),
				EReceipt::Applied,
				PortResponse.GetOutcomeCode(),
				AppliedReceipt,
				ReceiptDiagnostic))
		{
			return MakeRecoveryResult(
				EOwnerRecovery::InvariantViolation,
				TEXT("Arc preview composition owner could not seal an Applied recovery receipt."),
				1,
				0,
				AdapterResult,
				AppliedReceipt,
				HostRecovery,
				PreviousPendingCommand,
				PreviousHostCursor,
				PreviousSurfaceCursor);
		}
		HostRecoveryCallCount = 1;
		HostRecovery = Host.TryRecoverRejected(AppliedReceipt);
	}

	if (!HostRecovery.IsValid())
	{
		return MakeRecoveryResult(
			EOwnerRecovery::InvariantViolation,
			TEXT("Arc preview composition owner received invalid Host recovery evidence."),
			1,
			HostRecoveryCallCount,
			AdapterResult,
			AppliedReceipt,
			HostRecovery,
			PreviousPendingCommand,
			PreviousHostCursor,
			PreviousSurfaceCursor);
	}
	if (!HostRecovery.IsAccepted())
	{
		return MakeRecoveryResult(
			EOwnerRecovery::HostRecoveryRejected,
			HostRecovery.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview delivery Host rejected physical recovery evidence.")
				: *HostRecovery.GetDiagnostic(),
			1,
			HostRecoveryCallCount,
			AdapterResult,
			AppliedReceipt,
			HostRecovery,
			PreviousPendingCommand,
			PreviousHostCursor,
			PreviousSurfaceCursor);
	}
	if (!IsValid() || NeedsRecovery() || !IsSynchronized())
	{
		return MakeRecoveryResult(
			EOwnerRecovery::InvariantViolation,
			TEXT("Arc preview composition owner recovery did not reconcile Host and surface cursors."),
			1,
			HostRecoveryCallCount,
			AdapterResult,
			AppliedReceipt,
			HostRecovery,
			PreviousPendingCommand,
			PreviousHostCursor,
			PreviousSurfaceCursor);
	}
	return MakeRecoveryResult(
		EOwnerRecovery::Recovered,
		TEXT("Arc preview composition owner applied and recorded one exact rejected command."),
		1,
		HostRecoveryCallCount,
		AdapterResult,
		AppliedReceipt,
		HostRecovery,
		PreviousPendingCommand,
		PreviousHostCursor,
		PreviousSurfaceCursor);
}

bool FOwner::TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner end requires valid state and Run identity.");
		return false;
	}
	if (bOperationInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner cannot end during an operation.");
		return false;
	}
	if (!IsActive())
	{
		OutDiagnostic = TEXT("Arc preview composition owner is already empty.");
		return true;
	}
	if (GetRunId() != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner rejects mismatched Run teardown.");
		return false;
	}
	if (!CanEnd())
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner requires synchronized hidden and empty cursors before end.");
		return false;
	}

	FOwner Candidate = *this;
	FString NestedDiagnostic;
	if (!Candidate.Host.TryEnd(ExpectedRunId, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	if (!Candidate.Adapter.TryEnd(ExpectedRunId, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	Candidate.Surface = nullptr;
	Candidate.IdentitySurface = nullptr;
	Candidate.BoundSurfaceInstanceId.Invalidate();
	Candidate.LastSurfaceHandoffReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt();
	Candidate.bOperationInProgress = false;
	if (!Candidate.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Arc preview composition owner failed atomic empty-state validation.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview composition owner atomically ended Host and Adapter scope.");
	return true;
}

bool FOwner::IsSynchronized() const
{
	return IsActive() && Host.IsSynchronized()
		&& StatesMatchOrAreEmpty(
			PhysicalCursorFor(Host.GetCursorState()),
			Adapter.GetSurfaceCursor());
}

bool FOwner::CanEnd() const
{
	return IsValid() && IsActive() && !bOperationInProgress
		&& Host.CanEnd() && Adapter.CanEnd() && IsSynchronized();
}

bool FOwner::IsValid() const
{
	if (!Host.IsValid() || !Adapter.IsValid())
	{
		return false;
	}
	if (!IsActive())
	{
		return Host.IsEmpty() && Adapter.IsEmpty()
			&& Surface == nullptr && IdentitySurface == nullptr
			&& !BoundSurfaceInstanceId.IsValid()
			&& !LastSurfaceHandoffReceipt.IsValid()
			&& !bOperationInProgress;
	}
	if (!Host.IsActive() || !Adapter.IsActive()
		|| Host.GetRunId() != Adapter.GetRunId()
		|| Host.GetConsumerDefinitionId()
			!= Adapter.GetConsumerDefinitionId())
	{
		return false;
	}
	if ((IdentitySurface == nullptr) != !BoundSurfaceInstanceId.IsValid())
	{
		return false;
	}
	if (IdentitySurface != nullptr)
	{
		if (static_cast<
				Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface*>(
				IdentitySurface)
				!= Surface
			|| IdentitySurface->GetSurfaceInstanceId()
				!= BoundSurfaceInstanceId
			|| !LastSurfaceHandoffReceipt.IsValid()
			|| !LastSurfaceHandoffReceipt.MatchesCurrentBinding(
				Host.GetRunId(),
				Host.GetConsumerDefinitionId(),
				BoundSurfaceInstanceId))
		{
			return false;
		}
	}
	else if (LastSurfaceHandoffReceipt.IsValid())
	{
		return false;
	}
	return StatesMatchOrAreEmpty(
		PhysicalCursorFor(Host.GetCursorState()),
		Adapter.GetSurfaceCursor());
}

FOwnerUpdate FOwner::MakeUpdateResult(
	const EOwnerUpdate Status,
	const TCHAR* Diagnostic,
	const int32 HostUpdateCallCount,
	const int32 AdapterApplyCallCount,
	const FHostResult& HostResult,
	const FAdapterResult& AdapterResult,
	const FState& PreviousState,
	const FState& PreviousHostCursor,
	const FState& PreviousSurfaceCursor,
	const FCommand& PreviousPendingCommand) const
{
	FOwnerUpdate Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.RunId = Host.GetRunId();
	Result.ConsumerDefinitionId = Host.GetConsumerDefinitionId();
	Result.HostUpdateCallCount = HostUpdateCallCount;
	Result.AdapterApplyCallCount = AdapterApplyCallCount;
	Result.HostResult = HostResult;
	Result.AdapterResult = AdapterResult;
	Result.PreviousState = PreviousState;
	Result.State = Host.GetState();
	Result.PreviousHostCursor = PreviousHostCursor;
	Result.HostCursor = Host.GetCursorState();
	Result.PreviousSurfaceCursor = PreviousSurfaceCursor;
	Result.SurfaceCursor = Adapter.GetSurfaceCursor();
	Result.PreviousPendingCommand = PreviousPendingCommand;
	Result.PendingCommand = Host.GetPendingRejectedCommand();
	Result.bOwnerValidAfter = IsValid();
	Result.bValidated = Result.Validate();
	return Result;
}

FOwnerRecovery FOwner::MakeRecoveryResult(
	const EOwnerRecovery Status,
	const TCHAR* Diagnostic,
	const int32 AdapterApplyCallCount,
	const int32 HostRecoveryCallCount,
	const FAdapterResult& AdapterResult,
	const FReceipt& AppliedReceipt,
	const FHostRecovery& HostRecoveryResult,
	const FCommand& PreviousPendingCommand,
	const FState& PreviousHostCursor,
	const FState& PreviousSurfaceCursor) const
{
	FOwnerRecovery Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.RunId = Host.GetRunId();
	Result.ConsumerDefinitionId = Host.GetConsumerDefinitionId();
	Result.AdapterApplyCallCount = AdapterApplyCallCount;
	Result.HostRecoveryCallCount = HostRecoveryCallCount;
	Result.AdapterResult = AdapterResult;
	Result.AppliedReceipt = AppliedReceipt;
	Result.HostRecoveryResult = HostRecoveryResult;
	Result.PreviousPendingCommand = PreviousPendingCommand;
	Result.PendingCommand = Host.GetPendingRejectedCommand();
	Result.PreviousHostCursor = PreviousHostCursor;
	Result.HostCursor = Host.GetCursorState();
	Result.PreviousSurfaceCursor = PreviousSurfaceCursor;
	Result.SurfaceCursor = Adapter.GetSurfaceCursor();
	Result.bOwnerValidAfter = IsValid();
	Result.bValidated = Result.Validate();
	return Result;
}
