#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost.h"

namespace
{
	using EDeliverySession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus;
	using EHost =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FDeliveryResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult;
	using FHost =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost;
	using FHostResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult;
	using FProjectResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectResult;
	using FRecoveryResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryResult;
	using FSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using EStateSession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	using EProject =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus;
	using EDeliveryRecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus;

	bool IsUsableState(const FState& State)
	{
		return State.IsEmpty() || State.IsValid();
	}

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool CommandsMatchOrAreEmpty(
		const FCommand& Left,
		const FCommand& Right)
	{
		return (!Left.IsValid() && !Right.IsValid()) || Left.Matches(Right);
	}

	bool IsAcceptedStateStatus(const EStateSession Status)
	{
		return Status == EStateSession::Applied
			|| Status == EStateSession::NoChange;
	}

	bool IsAcceptedDeliveryStatus(const EDeliverySession Status)
	{
		return Status == EDeliverySession::Applied
			|| Status == EDeliverySession::Rejected
			|| Status == EDeliverySession::ApplicationReplayed
			|| Status == EDeliverySession::RejectionReplayed;
	}

	bool IsAppliedDeliveryStatus(const EDeliverySession Status)
	{
		return Status == EDeliverySession::Applied
			|| Status == EDeliverySession::ApplicationReplayed;
	}

	bool IsRejectedDeliveryStatus(const EDeliverySession Status)
	{
		return Status == EDeliverySession::Rejected
			|| Status == EDeliverySession::RejectionReplayed;
	}

	bool IsDeliveryReplayStatus(const EDeliverySession Status)
	{
		return Status == EDeliverySession::ApplicationReplayed
			|| Status == EDeliverySession::RejectionReplayed;
	}

	bool IsAcceptedRecoveryStatus(const EDeliveryRecovery Status)
	{
		return Status == EDeliveryRecovery::Recovered
			|| Status == EDeliveryRecovery::RecoveryReplayed;
	}
}

bool FHostResult::IsValid() const
{
	if (Status == EHost::Invalid || Diagnostic.IsEmpty()
		|| StateUpdateCallCount < 0 || StateUpdateCallCount > 1
		|| ProjectionCallCount < 0 || ProjectionCallCount > 1
		|| DeliveryCallCount < 0 || DeliveryCallCount > 1
		|| ProjectionCallCount > StateUpdateCallCount
		|| DeliveryCallCount > ProjectionCallCount
		|| !IsUsableState(PreviousState) || !IsUsableState(State)
		|| !IsUsableState(PreviousCursorState)
		|| !IsUsableState(CursorState))
	{
		return false;
	}

	const bool bHasScope = RunId.IsValid() && !ConsumerDefinitionId.IsNone();
	const bool bNoCalls = StateUpdateCallCount == 0
		&& ProjectionCallCount == 0 && DeliveryCallCount == 0
		&& !StateUpdate.IsValid() && !Projection.IsValid()
		&& !Delivery.IsValid();
	const bool bUnchanged = StatesMatchOrAreEmpty(PreviousState, State)
		&& StatesMatchOrAreEmpty(PreviousCursorState, CursorState)
		&& CommandsMatchOrAreEmpty(
			PreviousPendingCommand, PendingCommand);
	const bool bOneStateCall = StateUpdateCallCount == 1
		&& StateUpdate.IsValid();
	const bool bOneProjectionCall = ProjectionCallCount == 1
		&& Projection.IsValid();
	const bool bOneDeliveryCall = DeliveryCallCount == 1
		&& Delivery.IsValid();
	const bool bStateAccepted = bOneStateCall
		&& IsAcceptedStateStatus(StateUpdate.GetStatus());
	const bool bProjected = bStateAccepted && bOneProjectionCall
		&& Projection.GetStatus() == EProject::Projected
		&& StatesMatchOrAreEmpty(
			Projection.GetSessionResult().GetState(),
			StateUpdate.GetState());
	const EDeliverySession DeliveryStatus = Delivery.GetStatus();

	switch (Status)
	{
	case EHost::HostInactive:
		return !RunId.IsValid() && ConsumerDefinitionId.IsNone()
			&& PreviousState.IsEmpty() && State.IsEmpty()
			&& PreviousCursorState.IsEmpty() && CursorState.IsEmpty()
			&& !PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid() && bNoCalls;

	case EHost::HostInvalid:
		return bNoCalls && bUnchanged;

	case EHost::OperationInProgress:
		return bHasScope && bNoCalls && bUnchanged;

	case EHost::RecoveryRequired:
		return bHasScope && PendingCommand.IsValid()
			&& bNoCalls && bUnchanged;

	case EHost::StateUpdateRejected:
		return bHasScope && bOneStateCall && !bStateAccepted
			&& ProjectionCallCount == 0 && !Projection.IsValid()
			&& DeliveryCallCount == 0 && !Delivery.IsValid()
			&& bUnchanged;

	case EHost::ProjectionRejected:
		return bHasScope && bStateAccepted
			&& bOneProjectionCall
			&& Projection.GetStatus() != EProject::Projected
			&& DeliveryCallCount == 0 && !Delivery.IsValid()
			&& bUnchanged;

	case EHost::DeliveryRejected:
		return bHasScope && bProjected && bOneDeliveryCall
			&& !IsAcceptedDeliveryStatus(DeliveryStatus) && bUnchanged;

	case EHost::RejectedPendingRecovery:
		return bHasScope && bProjected && bOneDeliveryCall
			&& IsRejectedDeliveryStatus(DeliveryStatus)
			&& !PreviousPendingCommand.IsValid()
			&& PendingCommand.IsValid()
			&& PendingCommand.Matches(Projection.GetCommand())
			&& StatesMatchOrAreEmpty(
				State, PendingCommand.GetState())
			&& StatesMatchOrAreEmpty(
				CursorState, PendingCommand.GetPreviousState());

	case EHost::Applied:
	case EHost::ApplicationReplayed:
		return bHasScope && bProjected && bOneDeliveryCall
			&& IsAppliedDeliveryStatus(DeliveryStatus)
			&& IsDeliveryReplayStatus(DeliveryStatus)
				== (Status == EHost::ApplicationReplayed)
			&& !PendingCommand.IsValid()
			&& StatesMatchOrAreEmpty(State, CursorState)
			&& StatesMatchOrAreEmpty(
				State, Projection.GetCommand().GetState());

	case EHost::InvariantViolation:
		return bHasScope && bUnchanged
			&& StateUpdateCallCount >= ProjectionCallCount
			&& ProjectionCallCount >= DeliveryCallCount;

	default:
		return false;
	}
}

bool FHostResult::IsAccepted() const
{
	return IsValid()
		&& (Status == EHost::RejectedPendingRecovery
			|| Status == EHost::Applied
			|| Status == EHost::ApplicationReplayed);
}

bool FHostResult::WasApplied() const
{
	return IsValid()
		&& (Status == EHost::Applied
			|| Status == EHost::ApplicationReplayed);
}

bool FHostResult::WasRejected() const
{
	return IsValid() && Status == EHost::RejectedPendingRecovery;
}

bool FHostResult::IsReplay() const
{
	return IsValid() && Status == EHost::ApplicationReplayed;
}

bool FHostResult::NeedsRecovery() const
{
	return IsValid()
		&& (Status == EHost::RecoveryRequired
			|| Status == EHost::RejectedPendingRecovery);
}

bool FRecoveryResult::IsValid() const
{
	if (Status == ERecovery::Invalid || Diagnostic.IsEmpty()
		|| DeliveryRecoveryCallCount < 0
		|| DeliveryRecoveryCallCount > 1
		|| !IsUsableState(State)
		|| !IsUsableState(PreviousCursorState)
		|| !IsUsableState(CursorState))
	{
		return false;
	}

	const bool bHasScope = RunId.IsValid() && !ConsumerDefinitionId.IsNone();
	const bool bNoCall = DeliveryRecoveryCallCount == 0
		&& !DeliveryRecovery.IsValid();
	const bool bOneCall = DeliveryRecoveryCallCount == 1
		&& DeliveryRecovery.IsValid();
	const EDeliveryRecovery DeliveryRecoveryStatus =
		DeliveryRecovery.GetStatus();
	const bool bCursorUnchanged = StatesMatchOrAreEmpty(
		PreviousCursorState, CursorState);
	const bool bPendingUnchanged = CommandsMatchOrAreEmpty(
		PreviousPendingCommand, PendingCommand);

	switch (Status)
	{
	case ERecovery::HostInactive:
		return !RunId.IsValid() && ConsumerDefinitionId.IsNone()
			&& State.IsEmpty() && PreviousCursorState.IsEmpty()
			&& CursorState.IsEmpty() && !PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid() && bNoCall;

	case ERecovery::HostInvalid:
		return bNoCall && bCursorUnchanged && bPendingUnchanged;

	case ERecovery::OperationInProgress:
	case ERecovery::NoRecoveryPending:
	case ERecovery::ReceiptMismatch:
		return bHasScope && bNoCall && bCursorUnchanged
			&& bPendingUnchanged;

	case ERecovery::DeliveryRecoveryRejected:
		return bHasScope && bOneCall
			&& !IsAcceptedRecoveryStatus(DeliveryRecoveryStatus)
			&& bCursorUnchanged && bPendingUnchanged;

	case ERecovery::Recovered:
		return bHasScope && bOneCall
			&& DeliveryRecoveryStatus == EDeliveryRecovery::Recovered
			&& PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid()
			&& StatesMatchOrAreEmpty(State, CursorState);

	case ERecovery::RecoveryReplayed:
		return bHasScope && bOneCall
			&& DeliveryRecoveryStatus == EDeliveryRecovery::RecoveryReplayed
			&& !PreviousPendingCommand.IsValid()
			&& !PendingCommand.IsValid() && bCursorUnchanged
			&& StatesMatchOrAreEmpty(State, CursorState);

	case ERecovery::InvariantViolation:
		return bHasScope && DeliveryRecoveryCallCount == 1
			&& bCursorUnchanged && bPendingUnchanged;

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

FHostResult FHost::MakeResult(
	const EHost Status,
	const TCHAR* Diagnostic,
	const int32 StateUpdateCallCount,
	const FSessionResult& StateUpdate,
	const int32 ProjectionCallCount,
	const FProjectResult& Projection,
	const int32 DeliveryCallCount,
	const FDeliveryResult& Delivery,
	const FState& PreviousState,
	const FState& PreviousCursorState,
	const FCommand& PreviousPendingCommand) const
{
	FHostResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.RunId = GetRunId();
	Result.ConsumerDefinitionId = GetConsumerDefinitionId();
	Result.StateUpdateCallCount = StateUpdateCallCount;
	Result.ProjectionCallCount = ProjectionCallCount;
	Result.DeliveryCallCount = DeliveryCallCount;
	Result.StateUpdate = StateUpdate;
	Result.Projection = Projection;
	Result.Delivery = Delivery;
	Result.PreviousState = PreviousState;
	Result.State = GetState();
	Result.PreviousCursorState = PreviousCursorState;
	Result.CursorState = GetCursorState();
	Result.PreviousPendingCommand = PreviousPendingCommand;
	Result.PendingCommand = PendingRejectedCommand;
	return Result;
}

FRecoveryResult FHost::MakeRecoveryResult(
	const ERecovery Status,
	const TCHAR* Diagnostic,
	const int32 DeliveryRecoveryCallCount,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult&
		DeliveryRecovery,
	const FState& PreviousCursorState,
	const FCommand& PreviousPendingCommand) const
{
	FRecoveryResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.RunId = GetRunId();
	Result.ConsumerDefinitionId = GetConsumerDefinitionId();
	Result.DeliveryRecoveryCallCount = DeliveryRecoveryCallCount;
	Result.DeliveryRecovery = DeliveryRecovery;
	Result.State = GetState();
	Result.PreviousCursorState = PreviousCursorState;
	Result.CursorState = GetCursorState();
	Result.PreviousPendingCommand = PreviousPendingCommand;
	Result.PendingCommand = PendingRejectedCommand;
	return Result;
}

bool FHost::TryBegin(
	const FGuid& RequestedRunId,
	const FName RequestedConsumerDefinitionId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || bOperationInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host requires stable valid state before begin.");
		return false;
	}
	if (!RequestedRunId.IsValid() || RequestedConsumerDefinitionId.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host requires one Run and consumer identity.");
		return false;
	}
	if (IsActive())
	{
		if (GetRunId() == RequestedRunId
			&& GetConsumerDefinitionId() == RequestedConsumerDefinitionId)
		{
			OutDiagnostic = TEXT(
				"Arc preview delivery Host already owns this exact scope.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Active Arc preview delivery Host rejects scope rotation.");
		return false;
	}

	FHost Candidate;
	FString NestedDiagnostic;
	if (!Candidate.PresentationSession.TryBegin(
			RequestedRunId, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	if (!Candidate.DeliverySession.TryBegin(
			RequestedRunId,
			RequestedConsumerDefinitionId,
			NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host failed closed during scope binding.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview delivery Host bound one state Session and consumer Session.");
	return true;
}

FHostResult FHost::TryUpdate(
	const int32 HotbarSlotNumber,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
	const int32 SegmentCount,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
	const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port)
{
	const FState PreviousState = GetState();
	const FState PreviousCursorState = GetCursorState();
	const FCommand PreviousPendingCommand = PendingRejectedCommand;
	auto RejectBeforeCalls = [this, &PreviousState, &PreviousCursorState,
		&PreviousPendingCommand](const EHost Status, const TCHAR* Diagnostic)
	{
		return MakeResult(
			Status,
			Diagnostic,
			0,
			FSessionResult(),
			0,
			FProjectResult(),
			0,
			FDeliveryResult(),
			PreviousState,
			PreviousCursorState,
			PreviousPendingCommand);
	};

	if (!IsValid())
	{
		return RejectBeforeCalls(
			EHost::HostInvalid,
			TEXT("Arc preview delivery Host invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeCalls(
			EHost::HostInactive,
			TEXT("Arc preview delivery Host update requires an active Run."));
	}
	if (bOperationInProgress)
	{
		return RejectBeforeCalls(
			EHost::OperationInProgress,
			TEXT("Arc preview delivery Host rejects re-entrant operations."));
	}
	if (NeedsRecovery())
	{
		return RejectBeforeCalls(
			EHost::RecoveryRequired,
			TEXT("Arc preview delivery Host requires rejected-command recovery before another update."));
	}
	FHost Candidate = *this;
	FSessionResult StateUpdate;
	FProjectResult Projection;
	FDeliveryResult Delivery;
	int32 StateUpdateCallCount = 0;
	int32 ProjectionCallCount = 0;
	int32 DeliveryCallCount = 0;
	{
		TGuardValue<bool> OperationGuard(bOperationInProgress, true);
		StateUpdateCallCount = 1;
		StateUpdate = Candidate.PresentationSession.TryUpdate(
			HotbarSlotNumber,
			CurrentChoice,
			ChoicePolicy,
			SegmentCount,
			SourceBasis,
			Lifecycle,
			Coordinator);
		if (StateUpdate.IsValid()
			&& IsAcceptedStateStatus(StateUpdate.GetStatus()))
		{
			ProjectionCallCount = 1;
			Projection =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
					Project(StateUpdate);
		}
		if (Projection.IsValid()
			&& Projection.GetStatus() == EProject::Projected)
		{
			DeliveryCallCount = 1;
			Delivery = Candidate.DeliverySession.TryDeliver(
				Projection.GetCommand(), Port);
		}
	}

	if (!StateUpdate.IsValid())
	{
		return MakeResult(
			EHost::InvariantViolation,
			TEXT("Arc preview delivery Host received invalid state-update evidence."),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}
	if (!IsAcceptedStateStatus(StateUpdate.GetStatus()))
	{
		return MakeResult(
			EHost::StateUpdateRejected,
			StateUpdate.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview delivery Host state update was rejected.")
				: *StateUpdate.GetDiagnostic(),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}
	if (!Projection.IsValid()
		|| Projection.GetStatus() != EProject::Projected)
	{
		return MakeResult(
			Projection.IsValid()
				? EHost::ProjectionRejected
				: EHost::InvariantViolation,
			Projection.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview delivery Host command projection failed closed.")
				: *Projection.GetDiagnostic(),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}
	if (!Delivery.IsValid()
		|| Delivery.GetStatus() == EDeliverySession::InvariantViolation)
	{
		return MakeResult(
			EHost::InvariantViolation,
			TEXT("Arc preview delivery Host received invalid delivery evidence."),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}
	if (!IsAcceptedDeliveryStatus(Delivery.GetStatus()))
	{
		return MakeResult(
			EHost::DeliveryRejected,
			Delivery.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview delivery Host delivery was rejected before terminal receipt commit.")
				: *Delivery.GetDiagnostic(),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}

	Candidate.PendingRejectedCommand =
		IsRejectedDeliveryStatus(Delivery.GetStatus())
		? Projection.GetCommand()
		: FCommand();
	if (!Candidate.IsValid())
	{
		return MakeResult(
			EHost::InvariantViolation,
			TEXT("Arc preview delivery Host candidate Sessions failed joint validation."),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}

	const EHost CommittedStatus =
		IsRejectedDeliveryStatus(Delivery.GetStatus())
		? EHost::RejectedPendingRecovery
		: (IsDeliveryReplayStatus(Delivery.GetStatus())
			? EHost::ApplicationReplayed
			: EHost::Applied);
	const FHostResult Result = Candidate.MakeResult(
		CommittedStatus,
		Delivery.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview delivery Host completed one bounded update.")
			: *Delivery.GetDiagnostic(),
		StateUpdateCallCount, StateUpdate,
		ProjectionCallCount, Projection,
		DeliveryCallCount, Delivery,
		PreviousState, PreviousCursorState, PreviousPendingCommand);
	if (!Result.IsValid())
	{
		return MakeResult(
			EHost::InvariantViolation,
			TEXT("Arc preview delivery Host rejected an invalid post-commit result."),
			StateUpdateCallCount, StateUpdate,
			ProjectionCallCount, Projection,
			DeliveryCallCount, Delivery,
			PreviousState, PreviousCursorState, PreviousPendingCommand);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

FRecoveryResult FHost::TryRecoverRejected(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
		AppliedReceipt)
{
	const FState PreviousCursorState = GetCursorState();
	const FCommand PreviousPendingCommand = PendingRejectedCommand;
	auto RejectBeforeCall = [this, &PreviousCursorState,
		&PreviousPendingCommand](
		const ERecovery Status,
		const TCHAR* Diagnostic)
	{
		return MakeRecoveryResult(
			Status,
			Diagnostic,
			0,
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult(),
			PreviousCursorState,
			PreviousPendingCommand);
	};

	if (!IsValid())
	{
		return RejectBeforeCall(
			ERecovery::HostInvalid,
			TEXT("Arc preview delivery Host recovery invariants are invalid."));
	}
	if (!IsActive())
	{
		return RejectBeforeCall(
			ERecovery::HostInactive,
			TEXT("Arc preview delivery Host recovery requires an active Run."));
	}
	if (bOperationInProgress)
	{
		return RejectBeforeCall(
			ERecovery::OperationInProgress,
			TEXT("Arc preview delivery Host rejects re-entrant recovery."));
	}

	const bool bReceiptInScope = AppliedReceipt.IsValid()
		&& AppliedReceipt.IsApplied()
		&& AppliedReceipt.GetRunId() == GetRunId()
		&& AppliedReceipt.GetConsumerDefinitionId()
			== GetConsumerDefinitionId();
	if (NeedsRecovery())
	{
		if (!bReceiptInScope
			|| !AppliedReceipt.GetCommand().Matches(PendingRejectedCommand))
		{
			return RejectBeforeCall(
				ERecovery::ReceiptMismatch,
				TEXT("Arc preview delivery Host recovery receipt does not match its pending rejected command."));
		}
	}
	else
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt
			RecordedApplied;
		const auto& Ledger = DeliverySession.GetLedger();
		const bool bExactRecoveryReplay = bReceiptInScope
			&& Ledger.HasRejectedCommand(AppliedReceipt.GetCommandId())
			&& Ledger.TryGetAppliedReceipt(
				AppliedReceipt.GetCommandId(), RecordedApplied)
			&& RecordedApplied.Matches(AppliedReceipt)
			&& StatesMatchOrAreEmpty(
				GetState(), AppliedReceipt.GetCommand().GetState());
		if (!bExactRecoveryReplay)
		{
			return RejectBeforeCall(
				ERecovery::NoRecoveryPending,
				TEXT("Arc preview delivery Host has no matching recovery work."));
		}
	}

	FHost Candidate = *this;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult
		DeliveryRecovery;
	{
		TGuardValue<bool> OperationGuard(bOperationInProgress, true);
		DeliveryRecovery =
			Candidate.DeliverySession.TryRecoverRejected(AppliedReceipt);
	}
	if (!DeliveryRecovery.IsValid())
	{
		return MakeRecoveryResult(
			ERecovery::InvariantViolation,
			TEXT("Arc preview delivery Host received invalid recovery evidence."),
			1,
			DeliveryRecovery,
			PreviousCursorState,
			PreviousPendingCommand);
	}
	if (!IsAcceptedRecoveryStatus(DeliveryRecovery.GetStatus()))
	{
		return MakeRecoveryResult(
			ERecovery::DeliveryRecoveryRejected,
			DeliveryRecovery.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview delivery Host recovery failed closed.")
				: *DeliveryRecovery.GetDiagnostic(),
			1,
			DeliveryRecovery,
			PreviousCursorState,
			PreviousPendingCommand);
	}

	Candidate.PendingRejectedCommand = FCommand();
	if (!Candidate.IsValid())
	{
		return MakeRecoveryResult(
			ERecovery::InvariantViolation,
			TEXT("Arc preview delivery Host recovery did not reconcile state and cursor."),
			1,
			DeliveryRecovery,
			PreviousCursorState,
			PreviousPendingCommand);
	}

	const ERecovery CommittedStatus =
		DeliveryRecovery.GetStatus() == EDeliveryRecovery::RecoveryReplayed
		? ERecovery::RecoveryReplayed
		: ERecovery::Recovered;
	const FRecoveryResult Result = Candidate.MakeRecoveryResult(
		CommittedStatus,
		DeliveryRecovery.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview delivery Host reconciled its consumer cursor.")
			: *DeliveryRecovery.GetDiagnostic(),
		1,
		DeliveryRecovery,
		PreviousCursorState,
		PreviousPendingCommand);
	if (!Result.IsValid())
	{
		return MakeRecoveryResult(
			ERecovery::InvariantViolation,
			TEXT("Arc preview delivery Host rejected an invalid recovery commit."),
			1,
			DeliveryRecovery,
			PreviousCursorState,
			PreviousPendingCommand);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

bool FHost::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host end requires valid state and Run identity.");
		return false;
	}
	if (bOperationInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host cannot end during an operation.");
		return false;
	}
	if (!IsActive())
	{
		OutDiagnostic = TEXT("Arc preview delivery Host is already empty.");
		return true;
	}
	if (GetRunId() != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host rejects mismatched Run teardown.");
		return false;
	}
	if (!CanEnd())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host requires synchronized hidden state before end.");
		return false;
	}

	FHost Candidate = *this;
	FString NestedDiagnostic;
	if (!Candidate.DeliverySession.TryEnd(
			ExpectedRunId, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	if (!Candidate.PresentationSession.TryEnd(
			ExpectedRunId, NestedDiagnostic))
	{
		OutDiagnostic = NestedDiagnostic;
		return false;
	}
	Candidate.PendingRejectedCommand = FCommand();
	if (!Candidate.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Arc preview delivery Host failed atomic empty-state validation.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview delivery Host ended both synchronized Sessions.");
	return true;
}

bool FHost::IsSynchronized() const
{
	return IsUsableState(PresentationSession.GetState())
		&& IsUsableState(DeliverySession.GetCursorState())
		&& StatesMatchOrAreEmpty(
			PresentationSession.GetState(), DeliverySession.GetCursorState());
}

bool FHost::CanEnd() const
{
	if (!IsValid() || bOperationInProgress)
	{
		return false;
	}
	if (!IsActive())
	{
		return true;
	}
	const FState& State = GetState();
	return !NeedsRecovery() && IsSynchronized()
		&& (State.IsEmpty() || State.IsHidden())
		&& DeliverySession.CanEnd();
}

bool FHost::IsValid() const
{
	if (!PresentationSession.IsValid() || !DeliverySession.IsValid())
	{
		return false;
	}
	if (!PresentationSession.IsActive())
	{
		return DeliverySession.IsEmpty()
			&& !PendingRejectedCommand.IsValid()
			&& !bOperationInProgress;
	}
	if (!DeliverySession.IsActive()
		|| DeliverySession.GetRunId() != PresentationSession.GetRunId()
		|| DeliverySession.GetConsumerDefinitionId().IsNone())
	{
		return false;
	}

	if (!PendingRejectedCommand.IsValid())
	{
		return StatesMatchOrAreEmpty(
			PresentationSession.GetState(), DeliverySession.GetCursorState());
	}

	const auto& Ledger = DeliverySession.GetLedger();
	return PendingRejectedCommand.GetRunId() == GetRunId()
		&& Ledger.HasRejectedCommand(
			PendingRejectedCommand.GetCommandId())
		&& !Ledger.HasAppliedCommand(
			PendingRejectedCommand.GetCommandId())
		&& StatesMatchOrAreEmpty(
			PendingRejectedCommand.GetPreviousState(), GetCursorState())
		&& StatesMatchOrAreEmpty(
			PendingRejectedCommand.GetState(), GetState());
}
