#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using EFailure =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus;
	using FAdapterResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult;
	using FBaseSurface =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface;
	using FCheckpoint =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint;
	using FFailedHandoff =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult;
	using FOwner =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt;
	using FRecovery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using FSurface =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface;
	using FTicket =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket;

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	FState PhysicalCursorFor(const FState& HostCursor)
	{
		return HostCursor.IsVisible() ? HostCursor : FState();
	}

	bool IsRecoverableFailure(const EFailure Status)
	{
		return Status == EFailure::RetirementResponseInvalid
			|| Status == EFailure::RetirementInvariantViolation
			|| Status == EFailure::CommitInvariantViolation;
	}

	FString GuidKey(const FGuid& Value)
	{
		return Value.IsValid()
			? Value.ToString(EGuidFormats::Digits)
			: TEXT("INVALID_GUID");
	}

	FString NameKey(const FName Value)
	{
		return Value.IsNone() ? TEXT("NONE") : Value.ToString();
	}

	FString StateKey(const FState& State)
	{
		if (State.IsEmpty())
		{
			return TEXT("EMPTY");
		}
		return State.IsValid()
			? FString::Printf(
				TEXT("%d:%s"),
				static_cast<int32>(State.GetMode()),
				*State.GetPresentationStateId().ToString(
					EGuidFormats::Digits))
			: TEXT("INVALID_STATE");
	}

	FGuid MakeCheckpointId(
		const FGuid& TicketId,
		const EFailure SourceFailureStatus,
		const FGuid& SourceRetirementResponseId,
		const FGuid& PreviousSurfaceInstanceId,
		const FGuid& SurfaceInstanceId,
		const FState& PreviousSurfaceCursor,
		const FState& RetiredSurfaceCursor,
		const FState& SurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryCheckpoint.r1"),
			{
				GuidKey(TicketId),
				FString::FromInt(static_cast<int32>(SourceFailureStatus)),
				GuidKey(SourceRetirementResponseId),
				GuidKey(PreviousSurfaceInstanceId),
				GuidKey(SurfaceInstanceId),
				StateKey(PreviousSurfaceCursor),
				StateKey(RetiredSurfaceCursor),
				StateKey(SurfaceCursor)
			});
	}

	FGuid MakeRecoveryReceiptId(
		const FGuid& CheckpointId,
		const FGuid& TransitionTicketId,
		const FGuid& RunId,
		const FName ConsumerDefinitionId,
		const FGuid& PreviousSurfaceInstanceId,
		const FGuid& SurfaceInstanceId,
		const FState& RetiredSurfaceCursor,
		const FState& SurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryReceipt.r1"),
			{
				GuidKey(CheckpointId),
				GuidKey(TransitionTicketId),
				GuidKey(RunId),
				NameKey(ConsumerDefinitionId),
				GuidKey(PreviousSurfaceInstanceId),
				GuidKey(SurfaceInstanceId),
				StateKey(RetiredSurfaceCursor),
				StateKey(SurfaceCursor)
			});
	}
}

bool FCheckpoint::TryCreate(
	const FFailedHandoff& FailedHandoff,
	FCheckpoint& OutCheckpoint,
	FString& OutDiagnostic)
{
	OutCheckpoint = FCheckpoint();
	OutDiagnostic.Reset();
	if (!FailedHandoff.IsValid() || !FailedHandoff.NeedsManualRecovery()
		|| !IsRecoverableFailure(FailedHandoff.GetStatus())
		|| FailedHandoff.GetRetirementCallCount() != 1
		|| FailedHandoff.HasReceipt()
		|| FailedHandoff.IsOwnerValidAfter())
	{
		OutDiagnostic = TEXT(
			"Arc preview handoff recovery checkpoint requires one valid manual-recovery failure.");
		return false;
	}

	const FTicket& Ticket = FailedHandoff.GetTransitionTicket();
	if (!Ticket.IsValid() || Ticket.GetAction() != EAction::AdoptExact
		|| !FailedHandoff.GetPreviousSurfaceInstanceId().IsValid()
		|| FailedHandoff.GetPreviousSurfaceInstanceId()
			== FailedHandoff.GetSurfaceInstanceId()
		|| FailedHandoff.GetSurfaceInstanceId()
			!= Ticket.GetSurfaceInstanceId()
		|| !FailedHandoff.GetPreviousSurfaceCursor().IsVisible()
		|| !FailedHandoff.GetPreviousSurfaceCursor().Matches(
			Ticket.GetExpectedSurfaceCursor())
		|| !FailedHandoff.GetRetiredSurfaceCursor().IsEmpty()
		|| !Ticket.MatchesCandidateSnapshot(
			FailedHandoff.GetSurfaceInstanceId(),
			Ticket.GetConsumerDefinitionId(),
			FailedHandoff.GetSurfaceCursor()))
	{
		OutDiagnostic = TEXT(
			"Arc preview handoff recovery checkpoint requires exact visible-to-empty retirement and an unchanged replacement snapshot.");
		return false;
	}

	FCheckpoint Candidate;
	Candidate.TransitionTicket = Ticket;
	Candidate.SourceFailureStatus = FailedHandoff.GetStatus();
	Candidate.SourceRetirementResponseId =
		FailedHandoff.GetRetirementResponse().GetResponseId();
	Candidate.PreviousSurfaceInstanceId =
		FailedHandoff.GetPreviousSurfaceInstanceId();
	Candidate.SurfaceInstanceId = FailedHandoff.GetSurfaceInstanceId();
	Candidate.PreviousSurfaceCursor =
		FailedHandoff.GetPreviousSurfaceCursor();
	Candidate.RetiredSurfaceCursor = FailedHandoff.GetRetiredSurfaceCursor();
	Candidate.SurfaceCursor = FailedHandoff.GetSurfaceCursor();
	Candidate.CheckpointId = MakeCheckpointId(
		Candidate.TransitionTicket.GetTicketId(),
		Candidate.SourceFailureStatus,
		Candidate.SourceRetirementResponseId,
		Candidate.PreviousSurfaceInstanceId,
		Candidate.SurfaceInstanceId,
		Candidate.PreviousSurfaceCursor,
		Candidate.RetiredSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview handoff recovery checkpoint failed deterministic validation.");
		return false;
	}

	OutCheckpoint = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview handoff recovery checkpoint froze one exact half-commit boundary.");
	return true;
}

bool FCheckpoint::IsValid() const
{
	return CheckpointId.IsValid() && TransitionTicket.IsValid()
		&& TransitionTicket.GetAction() == EAction::AdoptExact
		&& IsRecoverableFailure(SourceFailureStatus)
		&& PreviousSurfaceInstanceId.IsValid() && SurfaceInstanceId.IsValid()
		&& PreviousSurfaceInstanceId != SurfaceInstanceId
		&& SurfaceInstanceId == TransitionTicket.GetSurfaceInstanceId()
		&& PreviousSurfaceCursor.IsVisible()
		&& PreviousSurfaceCursor.Matches(
			TransitionTicket.GetExpectedSurfaceCursor())
		&& RetiredSurfaceCursor.IsEmpty()
		&& TransitionTicket.MatchesCandidateSnapshot(
			SurfaceInstanceId,
			TransitionTicket.GetConsumerDefinitionId(),
			SurfaceCursor)
		&& CheckpointId == MakeCheckpointId(
			TransitionTicket.GetTicketId(),
			SourceFailureStatus,
			SourceRetirementResponseId,
			PreviousSurfaceInstanceId,
			SurfaceInstanceId,
			PreviousSurfaceCursor,
			RetiredSurfaceCursor,
			SurfaceCursor);
}

bool FCheckpoint::TryRehydrate(
	const FGuid& ExpectedCheckpointId,
	const FTicket& InTransitionTicket,
	const EFailure InSourceFailureStatus,
	const FGuid& InSourceRetirementResponseId,
	const FGuid& InPreviousSurfaceInstanceId,
	const FGuid& InSurfaceInstanceId,
	const FState& InPreviousSurfaceCursor,
	const FState& InRetiredSurfaceCursor,
	const FState& InSurfaceCursor,
	FCheckpoint& OutCheckpoint)
{
	OutCheckpoint = FCheckpoint();
	FCheckpoint Candidate;
	Candidate.TransitionTicket = InTransitionTicket;
	Candidate.SourceFailureStatus = InSourceFailureStatus;
	Candidate.SourceRetirementResponseId = InSourceRetirementResponseId;
	Candidate.PreviousSurfaceInstanceId = InPreviousSurfaceInstanceId;
	Candidate.SurfaceInstanceId = InSurfaceInstanceId;
	Candidate.PreviousSurfaceCursor = InPreviousSurfaceCursor;
	Candidate.RetiredSurfaceCursor = InRetiredSurfaceCursor;
	Candidate.SurfaceCursor = InSurfaceCursor;
	Candidate.CheckpointId = MakeCheckpointId(
		Candidate.TransitionTicket.GetTicketId(),
		Candidate.SourceFailureStatus,
		Candidate.SourceRetirementResponseId,
		Candidate.PreviousSurfaceInstanceId,
		Candidate.SurfaceInstanceId,
		Candidate.PreviousSurfaceCursor,
		Candidate.RetiredSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!ExpectedCheckpointId.IsValid()
		|| Candidate.CheckpointId != ExpectedCheckpointId
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutCheckpoint = MoveTemp(Candidate);
	return true;
}

bool FCheckpoint::MatchesFailedHandoff(
	const FFailedHandoff& FailedHandoff) const
{
	return IsValid() && FailedHandoff.IsValid()
		&& FailedHandoff.NeedsManualRecovery()
		&& FailedHandoff.GetTransitionTicket().GetTicketId()
			== TransitionTicket.GetTicketId()
		&& FailedHandoff.GetStatus() == SourceFailureStatus
		&& FailedHandoff.GetRetirementResponse().GetResponseId()
			== SourceRetirementResponseId
		&& FailedHandoff.GetPreviousSurfaceInstanceId()
			== PreviousSurfaceInstanceId
		&& FailedHandoff.GetSurfaceInstanceId() == SurfaceInstanceId
		&& FailedHandoff.GetPreviousSurfaceCursor().Matches(
			PreviousSurfaceCursor)
		&& StatesMatchOrAreEmpty(
			FailedHandoff.GetRetiredSurfaceCursor(), RetiredSurfaceCursor)
		&& FailedHandoff.GetSurfaceCursor().Matches(SurfaceCursor);
}

bool FReceipt::IsValid() const
{
	return ReceiptId.IsValid() && CheckpointId.IsValid()
		&& TransitionTicketId.IsValid() && RunId.IsValid()
		&& !ConsumerDefinitionId.IsNone()
		&& PreviousSurfaceInstanceId.IsValid() && SurfaceInstanceId.IsValid()
		&& PreviousSurfaceInstanceId != SurfaceInstanceId
		&& RetiredSurfaceCursor.IsEmpty() && SurfaceCursor.IsVisible()
		&& SurfaceCursor.GetRunId() == RunId
		&& ReceiptId == MakeRecoveryReceiptId(
			CheckpointId,
			TransitionTicketId,
			RunId,
			ConsumerDefinitionId,
			PreviousSurfaceInstanceId,
			SurfaceInstanceId,
			RetiredSurfaceCursor,
			SurfaceCursor);
}

bool FReceipt::MatchesCheckpoint(const FCheckpoint& Checkpoint) const
{
	return IsValid() && Checkpoint.IsValid()
		&& CheckpointId == Checkpoint.GetCheckpointId()
		&& TransitionTicketId
			== Checkpoint.GetTransitionTicket().GetTicketId()
		&& RunId == Checkpoint.GetTransitionTicket().GetRunId()
		&& ConsumerDefinitionId
			== Checkpoint.GetTransitionTicket().GetConsumerDefinitionId()
		&& PreviousSurfaceInstanceId
			== Checkpoint.GetPreviousSurfaceInstanceId()
		&& SurfaceInstanceId == Checkpoint.GetSurfaceInstanceId()
		&& StatesMatchOrAreEmpty(
			RetiredSurfaceCursor, Checkpoint.GetRetiredSurfaceCursor())
		&& SurfaceCursor.Matches(Checkpoint.GetSurfaceCursor());
}

bool FReceipt::MatchesCurrentBinding(
	const FGuid& InRunId,
	const FName InConsumerDefinitionId,
	const FGuid& InSurfaceInstanceId) const
{
	return IsValid() && RunId == InRunId
		&& ConsumerDefinitionId == InConsumerDefinitionId
		&& SurfaceInstanceId == InSurfaceInstanceId;
}

bool FResult::Validate() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| SurfaceSnapshotReadCount < 0 || SurfaceSnapshotReadCount > 2)
	{
		return false;
	}
	if (Status == EStatus::CheckpointInvalid)
	{
		return !Checkpoint.IsValid() && SurfaceSnapshotReadCount == 0
			&& !Receipt.IsValid();
	}
	if (!Checkpoint.IsValid())
	{
		return false;
	}
	if (Status == EStatus::Recovered)
	{
		return SurfaceSnapshotReadCount == 2 && Receipt.IsValid()
			&& Receipt.MatchesCheckpoint(Checkpoint)
			&& ObservedPreviousSurfaceInstanceId
				== Checkpoint.GetPreviousSurfaceInstanceId()
			&& ObservedSurfaceInstanceId
				== Checkpoint.GetSurfaceInstanceId()
			&& ObservedRetiredSurfaceCursor.IsEmpty()
			&& ObservedSurfaceCursor.Matches(Checkpoint.GetSurfaceCursor())
			&& bOwnerValidAfter;
	}
	if (Status == EStatus::Replayed)
	{
		return SurfaceSnapshotReadCount == 1 && Receipt.IsValid()
			&& Receipt.MatchesCheckpoint(Checkpoint)
			&& !ObservedPreviousSurfaceInstanceId.IsValid()
			&& ObservedSurfaceInstanceId
				== Checkpoint.GetSurfaceInstanceId()
			&& ObservedSurfaceCursor.Matches(Checkpoint.GetSurfaceCursor())
			&& bOwnerValidAfter;
	}
	if (Receipt.IsValid())
	{
		return false;
	}
	if (Status == EStatus::OperationInProgress
		|| Status == EStatus::OwnerInactive
		|| Status == EStatus::OwnerStructureMismatch
		|| Status == EStatus::OldSurfaceMismatch)
	{
		return SurfaceSnapshotReadCount == 0;
	}
	if (Status == EStatus::CheckpointReplayConflict)
	{
		return SurfaceSnapshotReadCount == 0
			|| SurfaceSnapshotReadCount == 1;
	}
	if (Status == EStatus::OldSnapshotMismatch)
	{
		return SurfaceSnapshotReadCount == 1;
	}
	return SurfaceSnapshotReadCount == 2;
}

bool FResult::IsAccepted() const
{
	return IsValid()
		&& (Status == EStatus::Recovered || Status == EStatus::Replayed);
}

bool FResult::WasRecovered() const
{
	return IsValid() && Status == EStatus::Recovered;
}

bool FResult::IsReplay() const
{
	return IsValid() && Status == EStatus::Replayed;
}

bool FResult::HasReceipt() const
{
	return IsValid() && Receipt.IsValid();
}

bool FRecovery::TryMakeReceipt(
	const FCheckpoint& Checkpoint,
	const FState& RetiredSurfaceCursor,
	const FState& SurfaceCursor,
	FReceipt& OutReceipt) const
{
	OutReceipt = FReceipt();
	if (!Checkpoint.IsValid() || !RetiredSurfaceCursor.IsEmpty()
		|| !SurfaceCursor.Matches(Checkpoint.GetSurfaceCursor()))
	{
		return false;
	}

	FReceipt Candidate;
	Candidate.CheckpointId = Checkpoint.GetCheckpointId();
	Candidate.TransitionTicketId =
		Checkpoint.GetTransitionTicket().GetTicketId();
	Candidate.RunId = Checkpoint.GetTransitionTicket().GetRunId();
	Candidate.ConsumerDefinitionId =
		Checkpoint.GetTransitionTicket().GetConsumerDefinitionId();
	Candidate.PreviousSurfaceInstanceId =
		Checkpoint.GetPreviousSurfaceInstanceId();
	Candidate.SurfaceInstanceId = Checkpoint.GetSurfaceInstanceId();
	Candidate.RetiredSurfaceCursor = RetiredSurfaceCursor;
	Candidate.SurfaceCursor = SurfaceCursor;
	Candidate.ReceiptId = MakeRecoveryReceiptId(
		Candidate.CheckpointId,
		Candidate.TransitionTicketId,
		Candidate.RunId,
		Candidate.ConsumerDefinitionId,
		Candidate.PreviousSurfaceInstanceId,
		Candidate.SurfaceInstanceId,
		Candidate.RetiredSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid() || !Candidate.MatchesCheckpoint(Checkpoint))
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

FResult FRecovery::Execute(
	FOwner& Owner,
	const FCheckpoint& Checkpoint,
	FSurface& ExpectedRetiredSurface,
	FSurface& NewSurface)
{
	const auto Reject = [this, &Checkpoint](
		const EStatus Status,
		const TCHAR* Diagnostic,
		const int32 SnapshotReadCount = 0,
		const FGuid& ObservedPreviousSurfaceInstanceId = FGuid(),
		const FGuid& ObservedSurfaceInstanceId = FGuid(),
		const FState& ObservedRetiredSurfaceCursor = FState(),
		const FState& ObservedSurfaceCursor = FState(),
		const bool bOwnerValidAfter = false)
	{
		return MakeResult(
			Status,
			Diagnostic,
			Checkpoint,
			SnapshotReadCount,
			ObservedPreviousSurfaceInstanceId,
			ObservedSurfaceInstanceId,
			ObservedRetiredSurfaceCursor,
			ObservedSurfaceCursor,
			FReceipt(),
			bOwnerValidAfter);
	};

	if (!Checkpoint.IsValid())
	{
		return Reject(
			EStatus::CheckpointInvalid,
			TEXT("Arc preview Owner handoff recovery requires one valid checkpoint."));
	}
	if (Owner.bOperationInProgress)
	{
		return Reject(
			EStatus::OperationInProgress,
			TEXT("Arc preview Owner handoff recovery rejects re-entrant operations."));
	}
	if (Owner.Surface == nullptr)
	{
		return Reject(
			EStatus::OwnerInactive,
			TEXT("Arc preview Owner handoff recovery requires one active binding."));
	}
	if (!Owner.Host.IsActive() || !Owner.Adapter.IsActive()
		|| Owner.Adapter.Surface != Owner.Surface
		|| Owner.Host.GetRunId() != Owner.Adapter.RunId
		|| Owner.Host.GetConsumerDefinitionId()
			!= Owner.Adapter.ConsumerDefinitionId
		|| Owner.Host.IsOperationInProgress()
		|| Owner.Adapter.bOperationInProgress)
	{
		return Reject(
			EStatus::OwnerStructureMismatch,
			TEXT("Arc preview Owner handoff recovery requires stable joint Host and Adapter structure."));
	}

	FBaseSurface* const NewBase = static_cast<FBaseSurface*>(&NewSurface);
	if (Owner.LastSurfaceHandoffRecoveryReceipt.IsValid()
		&& Owner.LastSurfaceHandoffRecoveryReceipt.MatchesCheckpoint(
			Checkpoint))
	{
		if (Owner.Surface != NewBase || Owner.IdentitySurface != &NewSurface
			|| Owner.BoundSurfaceInstanceId
				!= Checkpoint.GetSurfaceInstanceId()
			|| Owner.LastSurfaceHandoffReceipt.IsValid())
		{
			return Reject(
				EStatus::CheckpointReplayConflict,
				TEXT("Arc preview Owner handoff recovery checkpoint was already consumed by another binding."));
		}

		TGuardValue<bool> OperationGuard(Owner.bOperationInProgress, true);
		const FGuid NewSurfaceId = NewSurface.GetSurfaceInstanceId();
		const FName NewConsumer = NewSurface.GetConsumerDefinitionId();
		const FState NewCursor = NewSurface.GetSurfaceCursor();
		const bool bOwnerValidAfter =
			Checkpoint.GetTransitionTicket().MatchesCandidateSnapshot(
				NewSurfaceId, NewConsumer, NewCursor)
			&& Owner.IsValid() && Owner.IsSynchronized();
		if (!bOwnerValidAfter)
		{
			return Reject(
				EStatus::CheckpointReplayConflict,
				TEXT("Arc preview Owner handoff recovery replay no longer matches its committed surface snapshot."),
				1,
				FGuid(),
				NewSurfaceId,
				FState(),
				NewCursor,
				false);
		}
		return MakeResult(
			EStatus::Replayed,
			TEXT("Arc preview Owner handoff recovery replay matched the committed binding without mutation."),
			Checkpoint,
			1,
			FGuid(),
			NewSurfaceId,
			FState(),
			NewCursor,
			Owner.LastSurfaceHandoffRecoveryReceipt,
			true);
	}

	FBaseSurface* const OldBase =
		static_cast<FBaseSurface*>(&ExpectedRetiredSurface);
	if (Owner.Surface != OldBase || OldBase == NewBase)
	{
		return Reject(
			EStatus::OldSurfaceMismatch,
			TEXT("Arc preview Owner handoff recovery requires the exact retired binding and a distinct replacement."));
	}

	// All surface observations are guarded, and recovery calls no mutator.
	TGuardValue<bool> OperationGuard(Owner.bOperationInProgress, true);
	const FGuid PreviousSurfaceId =
		ExpectedRetiredSurface.GetSurfaceInstanceId();
	const FName PreviousConsumer =
		ExpectedRetiredSurface.GetConsumerDefinitionId();
	const FState RetiredCursor = ExpectedRetiredSurface.GetSurfaceCursor();
	if (PreviousSurfaceId != Checkpoint.GetPreviousSurfaceInstanceId()
		|| PreviousConsumer
			!= Checkpoint.GetTransitionTicket().GetConsumerDefinitionId()
		|| !RetiredCursor.IsEmpty())
	{
		return Reject(
			EStatus::OldSnapshotMismatch,
			TEXT("Arc preview Owner handoff recovery retired surface no longer matches its checkpoint."),
			1,
			PreviousSurfaceId,
			FGuid(),
			RetiredCursor);
	}

	const FGuid NewSurfaceId = NewSurface.GetSurfaceInstanceId();
	const FName NewConsumer = NewSurface.GetConsumerDefinitionId();
	const FState NewCursor = NewSurface.GetSurfaceCursor();
	if (!Checkpoint.GetTransitionTicket().MatchesCandidateSnapshot(
			NewSurfaceId, NewConsumer, NewCursor))
	{
		return Reject(
			EStatus::NewSnapshotMismatch,
			TEXT("Arc preview Owner handoff recovery replacement no longer matches its transition ticket."),
			2,
			PreviousSurfaceId,
			NewSurfaceId,
			RetiredCursor,
			NewCursor);
	}

	const bool bLegacyBindingMetadata = Owner.IdentitySurface == nullptr
		&& !Owner.BoundSurfaceInstanceId.IsValid()
		&& !Owner.LastSurfaceHandoffReceipt.IsValid()
		&& !Owner.LastSurfaceHandoffRecoveryReceipt.IsValid();
	const bool bNormalReceiptMatchesOld =
		Owner.LastSurfaceHandoffReceipt.IsValid()
		&& Owner.LastSurfaceHandoffReceipt.MatchesCurrentBinding(
			Owner.Host.GetRunId(),
			Owner.Host.GetConsumerDefinitionId(),
			PreviousSurfaceId);
	const bool bRecoveryReceiptMatchesOld =
		Owner.LastSurfaceHandoffRecoveryReceipt.IsValid()
		&& Owner.LastSurfaceHandoffRecoveryReceipt.MatchesCurrentBinding(
			Owner.Host.GetRunId(),
			Owner.Host.GetConsumerDefinitionId(),
			PreviousSurfaceId);
	const bool bIdentityBindingMetadata = Owner.IdentitySurface
		== &ExpectedRetiredSurface
		&& Owner.BoundSurfaceInstanceId == PreviousSurfaceId
		&& Owner.LastSurfaceHandoffReceipt.IsValid()
			!= Owner.LastSurfaceHandoffRecoveryReceipt.IsValid()
		&& bNormalReceiptMatchesOld != bRecoveryReceiptMatchesOld;
	const FState ExpectedPhysicalCursor =
		PhysicalCursorFor(Owner.Host.GetCursorState());
	if (!Owner.Host.IsValid() || !Owner.Adapter.IsValid()
		|| Owner.Host.NeedsRecovery()
		|| Owner.Host.GetRunId()
			!= Checkpoint.GetTransitionTicket().GetRunId()
		|| Owner.Host.GetConsumerDefinitionId()
			!= Checkpoint.GetTransitionTicket().GetConsumerDefinitionId()
		|| Owner.Adapter.RunId
			!= Checkpoint.GetTransitionTicket().GetRunId()
		|| Owner.Adapter.ConsumerDefinitionId
			!= Checkpoint.GetTransitionTicket().GetConsumerDefinitionId()
		|| !ExpectedPhysicalCursor.Matches(
			Checkpoint.GetTransitionTicket().GetExpectedSurfaceCursor())
		|| Owner.IsValid() || Owner.IsSynchronized()
		|| (!bLegacyBindingMetadata && !bIdentityBindingMetadata))
	{
		return Reject(
			EStatus::AuthoritySnapshotMismatch,
			TEXT("Arc preview Owner handoff recovery authority scope is not the exact pre-commit half-state."),
			2,
			PreviousSurfaceId,
			NewSurfaceId,
			RetiredCursor,
			NewCursor,
			Owner.IsValid());
	}

	FReceipt Receipt;
	if (!TryMakeReceipt(Checkpoint, RetiredCursor, NewCursor, Receipt))
	{
		return Reject(
			EStatus::CommitInvariantViolation,
			TEXT("Arc preview Owner handoff recovery could not seal its immutable receipt."),
			2,
			PreviousSurfaceId,
			NewSurfaceId,
			RetiredCursor,
			NewCursor);
	}

	FOwner Candidate = Owner;
	Candidate.Surface = NewBase;
	Candidate.Adapter.Surface = NewBase;
	Candidate.Adapter.LastResult = FAdapterResult();
	Candidate.IdentitySurface = &NewSurface;
	Candidate.BoundSurfaceInstanceId = NewSurfaceId;
	Candidate.LastSurfaceHandoffReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt();
	Candidate.LastSurfaceHandoffRecoveryReceipt = Receipt;
	if (!Candidate.IsValid() || !Candidate.IsSynchronized())
	{
		return Reject(
			EStatus::CommitInvariantViolation,
			TEXT("Arc preview Owner handoff recovery replacement failed joint commit validation."),
			2,
			PreviousSurfaceId,
			NewSurfaceId,
			RetiredCursor,
			NewCursor);
	}

	Owner = MoveTemp(Candidate);
	const bool bOwnerValidAfter = Owner.IsValid() && Owner.IsSynchronized();
	return MakeResult(
		EStatus::Recovered,
		TEXT("Arc preview Owner and Adapter atomically recovered the checkpointed replacement without surface mutation."),
		Checkpoint,
		2,
		PreviousSurfaceId,
		NewSurfaceId,
		RetiredCursor,
		NewCursor,
		Receipt,
		bOwnerValidAfter);
}

FResult FRecovery::MakeResult(
	const EStatus Status,
	const TCHAR* Diagnostic,
	const FCheckpoint& Checkpoint,
	const int32 SurfaceSnapshotReadCount,
	const FGuid& ObservedPreviousSurfaceInstanceId,
	const FGuid& ObservedSurfaceInstanceId,
	const FState& ObservedRetiredSurfaceCursor,
	const FState& ObservedSurfaceCursor,
	const FReceipt& Receipt,
	const bool bOwnerValidAfter) const
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Checkpoint = Checkpoint;
	Result.SurfaceSnapshotReadCount = SurfaceSnapshotReadCount;
	Result.ObservedPreviousSurfaceInstanceId =
		ObservedPreviousSurfaceInstanceId;
	Result.ObservedSurfaceInstanceId = ObservedSurfaceInstanceId;
	Result.ObservedRetiredSurfaceCursor = ObservedRetiredSurfaceCursor;
	Result.ObservedSurfaceCursor = ObservedSurfaceCursor;
	Result.Receipt = Receipt;
	Result.bOwnerValidAfter = bOwnerValidAfter;
	Result.bValidated = Result.Validate();
	return Result;
}
