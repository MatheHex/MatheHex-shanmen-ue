#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using ERetirement =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome;
	using EReceipt =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus;
	using FAdapterResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult;
	using FBaseSurface =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface;
	using FHandoff =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff;
	using FHandoffSurface =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface;
	using FOwner =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt;
	using FResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using FTicket =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket;

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool IsBindingAction(const EAction Action)
	{
		return Action == EAction::BindFresh || Action == EAction::AdoptExact;
	}

	FState PhysicalCursorFor(const FState& HostCursor)
	{
		return HostCursor.IsVisible() ? HostCursor : FState();
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

	FGuid MakeRetirementResponseId(
		const FGuid& TicketId,
		const FGuid& RetiredSurfaceInstanceId,
		const FGuid& ReplacementSurfaceInstanceId,
		const ERetirement Outcome,
		const FName OutcomeCode,
		const FState& PreviousSurfaceCursor,
		const FState& SurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceRetirementResponse.r1"),
			{
				GuidKey(TicketId),
				GuidKey(RetiredSurfaceInstanceId),
				GuidKey(ReplacementSurfaceInstanceId),
				FString::FromInt(static_cast<int32>(Outcome)),
				NameKey(OutcomeCode),
				StateKey(PreviousSurfaceCursor),
				StateKey(SurfaceCursor)
			});
	}

	FGuid MakeHandoffReceiptId(
		const FGuid& TicketId,
		const FGuid& RunId,
		const FName ConsumerDefinitionId,
		const FGuid& PreviousSurfaceInstanceId,
		const FGuid& SurfaceInstanceId,
		const EAction Action,
		const EReceipt Outcome,
		const int32 RetirementCallCount,
		const FGuid& RetirementResponseId,
		const FState& PreviousSurfaceCursor,
		const FState& RetiredSurfaceCursor,
		const FState& SurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffReceipt.r1"),
			{
				GuidKey(TicketId),
				GuidKey(RunId),
				NameKey(ConsumerDefinitionId),
				GuidKey(PreviousSurfaceInstanceId),
				GuidKey(SurfaceInstanceId),
				FString::FromInt(static_cast<int32>(Action)),
				FString::FromInt(static_cast<int32>(Outcome)),
				FString::FromInt(RetirementCallCount),
				GuidKey(RetirementResponseId),
				StateKey(PreviousSurfaceCursor),
				StateKey(RetiredSurfaceCursor),
				StateKey(SurfaceCursor)
			});
	}

}

bool FResponse::TryCreate(
	const FTicket& TransitionTicket,
	const FGuid& InRetiredSurfaceInstanceId,
	const ERetirement InOutcome,
	const FName InOutcomeCode,
	const FState& InPreviousSurfaceCursor,
	const FState& InSurfaceCursor,
	FResponse& OutResponse,
	FString& OutDiagnostic)
{
	OutResponse = FResponse();
	OutDiagnostic.Reset();
	const bool bApplied = InOutcome == ERetirement::Applied;
	const bool bRejected = InOutcome == ERetirement::Rejected;
	if (!TransitionTicket.IsValid()
		|| TransitionTicket.GetAction() != EAction::AdoptExact
		|| !InRetiredSurfaceInstanceId.IsValid()
		|| InRetiredSurfaceInstanceId
			== TransitionTicket.GetSurfaceInstanceId()
		|| (!bApplied && !bRejected) || InOutcomeCode.IsNone()
		|| !InPreviousSurfaceCursor.IsVisible()
		|| !InPreviousSurfaceCursor.Matches(
			TransitionTicket.GetObservedSurfaceCursor())
		|| (bApplied && !InSurfaceCursor.IsEmpty())
		|| (bRejected
			&& !InSurfaceCursor.Matches(InPreviousSurfaceCursor)))
	{
		OutDiagnostic = TEXT(
			"Arc preview surface retirement response requires one exact AdoptExact ticket and either visible-to-empty application or unchanged rejection.");
		return false;
	}

	FResponse Candidate;
	Candidate.TransitionTicketId = TransitionTicket.GetTicketId();
	Candidate.RetiredSurfaceInstanceId = InRetiredSurfaceInstanceId;
	Candidate.ReplacementSurfaceInstanceId =
		TransitionTicket.GetSurfaceInstanceId();
	Candidate.Outcome = InOutcome;
	Candidate.OutcomeCode = InOutcomeCode;
	Candidate.PreviousSurfaceCursor = InPreviousSurfaceCursor;
	Candidate.SurfaceCursor = InSurfaceCursor;
	Candidate.ResponseId = MakeRetirementResponseId(
		Candidate.TransitionTicketId,
		Candidate.RetiredSurfaceInstanceId,
		Candidate.ReplacementSurfaceInstanceId,
		Candidate.Outcome,
		Candidate.OutcomeCode,
		Candidate.PreviousSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview surface retirement response failed deterministic validation.");
		return false;
	}
	OutResponse = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview surface retirement response sealed one bounded outcome.");
	return true;
}

bool FResponse::IsValid() const
{
	const bool bApplied = Outcome == ERetirement::Applied;
	const bool bRejected = Outcome == ERetirement::Rejected;
	return ResponseId.IsValid() && TransitionTicketId.IsValid()
		&& RetiredSurfaceInstanceId.IsValid()
		&& ReplacementSurfaceInstanceId.IsValid()
		&& RetiredSurfaceInstanceId != ReplacementSurfaceInstanceId
		&& (!OutcomeCode.IsNone()) && PreviousSurfaceCursor.IsVisible()
		&& ((bApplied && SurfaceCursor.IsEmpty())
			|| (bRejected
				&& SurfaceCursor.Matches(PreviousSurfaceCursor)))
		&& ResponseId == MakeRetirementResponseId(
			TransitionTicketId,
			RetiredSurfaceInstanceId,
			ReplacementSurfaceInstanceId,
			Outcome,
			OutcomeCode,
			PreviousSurfaceCursor,
			SurfaceCursor);
}

bool FResponse::MatchesTransitionTicket(const FTicket& TransitionTicket) const
{
	return IsValid() && TransitionTicket.IsValid()
		&& TransitionTicket.GetAction() == EAction::AdoptExact
		&& TransitionTicketId == TransitionTicket.GetTicketId()
		&& ReplacementSurfaceInstanceId
			== TransitionTicket.GetSurfaceInstanceId()
		&& PreviousSurfaceCursor.Matches(
			TransitionTicket.GetObservedSurfaceCursor());
}

bool FResponse::IsApplied() const
{
	return IsValid() && Outcome == ERetirement::Applied;
}

bool FResponse::IsRejected() const
{
	return IsValid() && Outcome == ERetirement::Rejected;
}

bool FReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !TransitionTicketId.IsValid()
		|| !RunId.IsValid() || ConsumerDefinitionId.IsNone()
		|| !PreviousSurfaceInstanceId.IsValid()
		|| !SurfaceInstanceId.IsValid()
		|| PreviousSurfaceInstanceId == SurfaceInstanceId
		|| !IsBindingAction(Action))
	{
		return false;
	}
	const bool bFresh = Action == EAction::BindFresh
		&& Outcome == EReceipt::FreshBound && RetirementCallCount == 0
		&& !RetirementResponse.IsValid()
		&& PreviousSurfaceCursor.IsEmpty()
		&& RetiredSurfaceCursor.IsEmpty() && SurfaceCursor.IsEmpty();
	const bool bExact = Action == EAction::AdoptExact
		&& Outcome == EReceipt::ExactAdopted && RetirementCallCount == 1
		&& RetirementResponse.IsApplied()
		&& RetirementResponse.GetTransitionTicketId()
			== TransitionTicketId
		&& RetirementResponse.GetRetiredSurfaceInstanceId()
			== PreviousSurfaceInstanceId
		&& RetirementResponse.GetReplacementSurfaceInstanceId()
			== SurfaceInstanceId
		&& PreviousSurfaceCursor.IsVisible()
		&& RetirementResponse.GetPreviousSurfaceCursor().Matches(
			PreviousSurfaceCursor)
		&& RetiredSurfaceCursor.IsEmpty()
		&& RetirementResponse.GetSurfaceCursor().IsEmpty()
		&& SurfaceCursor.IsVisible()
		&& SurfaceCursor.Matches(PreviousSurfaceCursor);
	if (!bFresh && !bExact)
	{
		return false;
	}
	return ReceiptId == MakeHandoffReceiptId(
		TransitionTicketId,
		RunId,
		ConsumerDefinitionId,
		PreviousSurfaceInstanceId,
		SurfaceInstanceId,
		Action,
		Outcome,
		RetirementCallCount,
		RetirementResponse.GetResponseId(),
		PreviousSurfaceCursor,
		RetiredSurfaceCursor,
		SurfaceCursor);
}

bool FReceipt::MatchesTransitionTicket(const FTicket& TransitionTicket) const
{
	return IsValid() && TransitionTicket.IsValid()
		&& TransitionTicketId == TransitionTicket.GetTicketId()
		&& RunId == TransitionTicket.GetRunId()
		&& ConsumerDefinitionId
			== TransitionTicket.GetConsumerDefinitionId()
		&& SurfaceInstanceId == TransitionTicket.GetSurfaceInstanceId()
		&& Action == TransitionTicket.GetAction()
		&& StatesMatchOrAreEmpty(
			SurfaceCursor, TransitionTicket.GetObservedSurfaceCursor());
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

bool FReceipt::IsFreshBound() const
{
	return IsValid() && Outcome == EReceipt::FreshBound;
}

bool FReceipt::IsExactAdopted() const
{
	return IsValid() && Outcome == EReceipt::ExactAdopted;
}

bool FResult::Validate() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| RetirementCallCount < 0 || RetirementCallCount > 1)
	{
		return false;
	}
	if (Status == EStatus::TicketInvalid)
	{
		return !TransitionTicket.IsValid() && RetirementCallCount == 0
			&& !RetirementResponse.IsValid() && !Receipt.IsValid();
	}
	if (!TransitionTicket.IsValid())
	{
		return false;
	}
	if (Status == EStatus::Committed || Status == EStatus::Replayed)
	{
		return Receipt.IsValid()
			&& Receipt.MatchesTransitionTicket(TransitionTicket)
			&& SurfaceInstanceId
				== TransitionTicket.GetSurfaceInstanceId()
			&& bOwnerValidAfter
			&& ((Status == EStatus::Committed
					&& RetirementCallCount
						== Receipt.GetRetirementCallCount())
				|| (Status == EStatus::Replayed
					&& RetirementCallCount == 0));
	}
	if (Receipt.IsValid())
	{
		return false;
	}
	if (Status == EStatus::RetirementRejected)
	{
		return TransitionTicket.GetAction() == EAction::AdoptExact
			&& RetirementCallCount == 1
			&& RetirementResponse.IsRejected()
			&& RetirementResponse.MatchesTransitionTicket(
				TransitionTicket)
			&& bOwnerValidAfter;
	}
	if (Status == EStatus::RetirementResponseInvalid
		|| Status == EStatus::RetirementInvariantViolation)
	{
		return TransitionTicket.GetAction() == EAction::AdoptExact
			&& RetirementCallCount == 1;
	}
	if (Status == EStatus::CommitInvariantViolation)
	{
		return (TransitionTicket.GetAction() == EAction::BindFresh
				&& RetirementCallCount == 0
				&& !RetirementResponse.IsValid())
			|| (TransitionTicket.GetAction() == EAction::AdoptExact
				&& RetirementCallCount == 1);
	}
	return RetirementCallCount == 0 && !RetirementResponse.IsValid();
}

bool FResult::IsAccepted() const
{
	return IsValid()
		&& (Status == EStatus::Committed || Status == EStatus::Replayed);
}

bool FResult::WasCommitted() const
{
	return IsValid() && Status == EStatus::Committed;
}

bool FResult::IsReplay() const
{
	return IsValid() && Status == EStatus::Replayed;
}

bool FResult::WasRetirementRejected() const
{
	return IsValid() && Status == EStatus::RetirementRejected;
}

bool FResult::DidCallRetirement() const
{
	return IsValid() && RetirementCallCount == 1;
}

bool FResult::HasReceipt() const
{
	return IsValid() && Receipt.IsValid();
}

bool FResult::CanRetryExactTicket() const
{
	return IsValid() && Status == EStatus::RetirementRejected
		&& bOwnerValidAfter;
}

bool FResult::NeedsManualRecovery() const
{
	return IsValid() && !bOwnerValidAfter
		&& (Status == EStatus::RetirementResponseInvalid
			|| Status == EStatus::RetirementInvariantViolation
			|| Status == EStatus::CommitInvariantViolation);
}

bool FHandoff::TryMakeReceipt(
	const FTicket& TransitionTicket,
	const FGuid& PreviousSurfaceInstanceId,
	const FResponse& RetirementResponse,
	const FState& PreviousSurfaceCursor,
	const FState& RetiredSurfaceCursor,
	const FState& SurfaceCursor,
	FReceipt& OutReceipt) const
{
	OutReceipt = FReceipt();
	if (!TransitionTicket.IsValid()
		|| !PreviousSurfaceInstanceId.IsValid()
		|| PreviousSurfaceInstanceId
			== TransitionTicket.GetSurfaceInstanceId())
	{
		return false;
	}

	FReceipt Candidate;
	Candidate.TransitionTicketId = TransitionTicket.GetTicketId();
	Candidate.RunId = TransitionTicket.GetRunId();
	Candidate.ConsumerDefinitionId =
		TransitionTicket.GetConsumerDefinitionId();
	Candidate.PreviousSurfaceInstanceId = PreviousSurfaceInstanceId;
	Candidate.SurfaceInstanceId = TransitionTicket.GetSurfaceInstanceId();
	Candidate.Action = TransitionTicket.GetAction();
	Candidate.RetirementResponse = RetirementResponse;
	Candidate.PreviousSurfaceCursor = PreviousSurfaceCursor;
	Candidate.RetiredSurfaceCursor = RetiredSurfaceCursor;
	Candidate.SurfaceCursor = SurfaceCursor;
	if (Candidate.Action == EAction::BindFresh)
	{
		Candidate.Outcome = EReceipt::FreshBound;
		Candidate.RetirementCallCount = 0;
	}
	else if (Candidate.Action == EAction::AdoptExact)
	{
		Candidate.Outcome = EReceipt::ExactAdopted;
		Candidate.RetirementCallCount = 1;
	}
	else
	{
		return false;
	}
	Candidate.ReceiptId = MakeHandoffReceiptId(
		Candidate.TransitionTicketId,
		Candidate.RunId,
		Candidate.ConsumerDefinitionId,
		Candidate.PreviousSurfaceInstanceId,
		Candidate.SurfaceInstanceId,
		Candidate.Action,
		Candidate.Outcome,
		Candidate.RetirementCallCount,
		Candidate.RetirementResponse.GetResponseId(),
		Candidate.PreviousSurfaceCursor,
		Candidate.RetiredSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid()
		|| !Candidate.MatchesTransitionTicket(TransitionTicket))
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

FResult FHandoff::Execute(
	FOwner& Owner,
	const FTicket& TransitionTicket,
	FHandoffSurface& ExpectedOldSurface,
	FHandoffSurface& NewSurface)
{
	const auto Reject = [this, &Owner, &TransitionTicket](
		const EStatus Status,
		const TCHAR* Diagnostic,
		const int32 RetirementCallCount = 0,
		const FGuid& PreviousSurfaceInstanceId = FGuid(),
		const FGuid& SurfaceInstanceId = FGuid(),
		const FResponse& RetirementResponse = FResponse(),
		const FState& PreviousSurfaceCursor = FState(),
		const FState& RetiredSurfaceCursor = FState(),
		const FState& SurfaceCursor = FState(),
		const bool bOwnerValidAfter = false)
	{
		return MakeResult(
			Status,
			Diagnostic,
			TransitionTicket,
			RetirementCallCount,
			PreviousSurfaceInstanceId,
			SurfaceInstanceId,
			RetirementResponse,
			FReceipt(),
			PreviousSurfaceCursor,
			RetiredSurfaceCursor,
			SurfaceCursor,
			bOwnerValidAfter);
	};

	if (!TransitionTicket.IsValid()
		|| !IsBindingAction(TransitionTicket.GetAction()))
	{
		return Reject(
			EStatus::TicketInvalid,
			TEXT("Arc preview Owner handoff requires one valid binding ticket."));
	}
	if (Owner.bOperationInProgress)
	{
		return Reject(
			EStatus::OperationInProgress,
			TEXT("Arc preview Owner handoff rejects re-entrant operations."));
	}
	if (Owner.Surface == nullptr)
	{
		return Reject(
			EStatus::OwnerInactive,
			TEXT("Arc preview Owner handoff requires one active binding."));
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
			EStatus::OwnerInvalid,
			TEXT("Arc preview Owner handoff requires stable joint Host and Adapter structure."));
	}

	FBaseSurface* const NewBase = static_cast<FBaseSurface*>(&NewSurface);
	if (Owner.LastSurfaceHandoffReceipt.IsValid()
		&& Owner.LastSurfaceHandoffReceipt.MatchesTransitionTicket(
			TransitionTicket))
	{
		if (Owner.Surface != NewBase || Owner.IdentitySurface != &NewSurface
			|| Owner.BoundSurfaceInstanceId
				!= TransitionTicket.GetSurfaceInstanceId())
		{
			return Reject(
				EStatus::TicketReplayConflict,
				TEXT("Arc preview Owner handoff ticket was already consumed by another current binding."));
		}
		TGuardValue<bool> OperationGuard(Owner.bOperationInProgress, true);
		const FGuid NewSurfaceId = NewSurface.GetSurfaceInstanceId();
		const FName NewConsumer = NewSurface.GetConsumerDefinitionId();
		const FState NewCursor = NewSurface.GetSurfaceCursor();
		const bool bOwnerValidAfter =
			TransitionTicket.MatchesCandidateSnapshot(
				NewSurfaceId, NewConsumer, NewCursor)
			&& Owner.IsValid();
		if (!bOwnerValidAfter)
		{
			return Reject(
				EStatus::TicketReplayConflict,
				TEXT("Arc preview Owner handoff replay no longer matches its committed surface snapshot."),
				0,
				Owner.LastSurfaceHandoffReceipt
					.GetPreviousSurfaceInstanceId(),
				NewSurfaceId,
				FResponse(),
				Owner.LastSurfaceHandoffReceipt.GetPreviousSurfaceCursor(),
				Owner.LastSurfaceHandoffReceipt.GetRetiredSurfaceCursor(),
				NewCursor,
				false);
		}
		return MakeResult(
			EStatus::Replayed,
			TEXT("Arc preview Owner handoff replay matched the already committed binding without mutation."),
			TransitionTicket,
			0,
			Owner.LastSurfaceHandoffReceipt
				.GetPreviousSurfaceInstanceId(),
			NewSurfaceId,
			FResponse(),
			Owner.LastSurfaceHandoffReceipt,
			Owner.LastSurfaceHandoffReceipt.GetPreviousSurfaceCursor(),
			Owner.LastSurfaceHandoffReceipt.GetRetiredSurfaceCursor(),
			NewCursor,
			true);
	}

	FBaseSurface* const OldBase =
		static_cast<FBaseSurface*>(&ExpectedOldSurface);
	if (Owner.Surface != OldBase || OldBase == NewBase)
	{
		return Reject(
			EStatus::OldSurfaceMismatch,
			TEXT("Arc preview Owner handoff requires the exact live current surface and a distinct replacement."));
	}

	// From here onward every external surface read and callback is guarded.
	TGuardValue<bool> OperationGuard(Owner.bOperationInProgress, true);
	const FGuid PreviousSurfaceId =
		ExpectedOldSurface.GetSurfaceInstanceId();
	const FName PreviousConsumer =
		ExpectedOldSurface.GetConsumerDefinitionId();
	const FState PreviousSurfaceCursor =
		ExpectedOldSurface.GetSurfaceCursor();
	const FGuid NewSurfaceId = NewSurface.GetSurfaceInstanceId();
	const FName NewConsumer = NewSurface.GetConsumerDefinitionId();
	const FState NewCursor = NewSurface.GetSurfaceCursor();
	if (!NewSurfaceId.IsValid() || NewSurfaceId == PreviousSurfaceId
		|| !TransitionTicket.MatchesCandidateSnapshot(
			NewSurfaceId, NewConsumer, NewCursor))
	{
		return Reject(
			EStatus::NewSurfaceMismatch,
			TEXT("Arc preview replacement surface does not match the exact transition ticket snapshot."),
			0,
			PreviousSurfaceId,
			NewSurfaceId,
			FResponse(),
			PreviousSurfaceCursor,
			PreviousSurfaceCursor,
			NewCursor,
			Owner.IsValid());
	}
	const FState ExpectedPhysicalCursor =
		PhysicalCursorFor(Owner.Host.GetCursorState());
	if (!Owner.IsValid() || Owner.Host.NeedsRecovery()
		|| Owner.Host.GetRunId() != TransitionTicket.GetRunId()
		|| Owner.Host.GetConsumerDefinitionId()
			!= TransitionTicket.GetConsumerDefinitionId()
		|| PreviousConsumer != TransitionTicket.GetConsumerDefinitionId()
		|| !StatesMatchOrAreEmpty(
			ExpectedPhysicalCursor,
			TransitionTicket.GetExpectedSurfaceCursor())
		|| !StatesMatchOrAreEmpty(
			ExpectedPhysicalCursor, PreviousSurfaceCursor))
	{
		return Reject(
			EStatus::SnapshotMismatch,
			TEXT("Arc preview Owner, old surface and ticket do not share one recoverable snapshot."),
			0,
			PreviousSurfaceId,
			NewSurfaceId,
			FResponse(),
			PreviousSurfaceCursor,
			PreviousSurfaceCursor,
			NewCursor,
			Owner.IsValid());
	}

	FOwner Candidate = Owner;
	Candidate.Surface = NewBase;
	Candidate.Adapter.Surface = NewBase;
	Candidate.Adapter.LastResult = FAdapterResult();
	Candidate.IdentitySurface = &NewSurface;
	Candidate.BoundSurfaceInstanceId = NewSurfaceId;

	int32 RetirementCallCount = 0;
	FResponse RetirementResponse;
	FState RetiredCursor = PreviousSurfaceCursor;
	if (TransitionTicket.GetAction() == EAction::AdoptExact)
	{
		RetirementCallCount = 1;
		RetirementResponse =
			ExpectedOldSurface.RetireForHandoff(TransitionTicket);
		const FGuid FinalOldSurfaceId =
			ExpectedOldSurface.GetSurfaceInstanceId();
		const FGuid FinalNewSurfaceId = NewSurface.GetSurfaceInstanceId();
		const FName FinalNewConsumer =
			NewSurface.GetConsumerDefinitionId();
		RetiredCursor = ExpectedOldSurface.GetSurfaceCursor();
		const FState FinalNewCursor = NewSurface.GetSurfaceCursor();
		const bool bOwnerValidAfter = Owner.IsValid();
		if (!RetirementResponse.IsValid()
			|| !RetirementResponse.MatchesTransitionTicket(
				TransitionTicket)
			|| RetirementResponse.GetRetiredSurfaceInstanceId()
				!= PreviousSurfaceId)
		{
			return Reject(
				EStatus::RetirementResponseInvalid,
				TEXT("Arc preview old surface returned invalid retirement evidence."),
				RetirementCallCount,
				PreviousSurfaceId,
				FinalNewSurfaceId,
				RetirementResponse,
				PreviousSurfaceCursor,
				RetiredCursor,
				FinalNewCursor,
				bOwnerValidAfter);
		}
		if (FinalOldSurfaceId != PreviousSurfaceId
			|| FinalNewSurfaceId != NewSurfaceId
			|| !TransitionTicket.MatchesCandidateSnapshot(
				FinalNewSurfaceId, FinalNewConsumer, FinalNewCursor)
			|| !StatesMatchOrAreEmpty(
				RetirementResponse.GetSurfaceCursor(), RetiredCursor))
		{
			return Reject(
				EStatus::RetirementInvariantViolation,
				TEXT("Arc preview surface identity or cursor drifted during old-surface retirement."),
				RetirementCallCount,
				PreviousSurfaceId,
				FinalNewSurfaceId,
				RetirementResponse,
				PreviousSurfaceCursor,
				RetiredCursor,
				FinalNewCursor,
				bOwnerValidAfter);
		}
		if (RetirementResponse.IsRejected())
		{
			return Reject(
				EStatus::RetirementRejected,
				TEXT("Arc preview old surface rejected retirement without mutation; the exact ticket remains retryable."),
				RetirementCallCount,
				PreviousSurfaceId,
				NewSurfaceId,
				RetirementResponse,
				PreviousSurfaceCursor,
				RetiredCursor,
				FinalNewCursor,
				bOwnerValidAfter);
		}
	}
	else
	{
		const FGuid FinalOldSurfaceId =
			ExpectedOldSurface.GetSurfaceInstanceId();
		const FGuid FinalNewSurfaceId = NewSurface.GetSurfaceInstanceId();
		const FName FinalNewConsumer =
			NewSurface.GetConsumerDefinitionId();
		RetiredCursor = ExpectedOldSurface.GetSurfaceCursor();
		const FState FinalNewCursor = NewSurface.GetSurfaceCursor();
		if (FinalOldSurfaceId != PreviousSurfaceId
			|| FinalNewSurfaceId != NewSurfaceId
			|| !RetiredCursor.IsEmpty()
			|| !TransitionTicket.MatchesCandidateSnapshot(
				FinalNewSurfaceId, FinalNewConsumer, FinalNewCursor))
		{
			return Reject(
				EStatus::CommitInvariantViolation,
				TEXT("Arc preview fresh binding changed before its zero-mutation pointer commit."),
				0,
				PreviousSurfaceId,
				FinalNewSurfaceId,
				FResponse(),
				PreviousSurfaceCursor,
				RetiredCursor,
				FinalNewCursor,
				Owner.IsValid());
		}
	}

	const FState FinalNewCursor = NewSurface.GetSurfaceCursor();
	FReceipt Receipt;
	if (!TryMakeReceipt(
			TransitionTicket,
			PreviousSurfaceId,
			RetirementResponse,
			PreviousSurfaceCursor,
			RetiredCursor,
			FinalNewCursor,
			Receipt))
	{
		return Reject(
			EStatus::CommitInvariantViolation,
			TEXT("Arc preview Owner handoff could not seal its immutable receipt."),
			RetirementCallCount,
			PreviousSurfaceId,
			NewSurfaceId,
			RetirementResponse,
			PreviousSurfaceCursor,
			RetiredCursor,
			FinalNewCursor,
			Owner.IsValid());
	}
	Candidate.LastSurfaceHandoffReceipt = Receipt;
	Candidate.LastSurfaceHandoffRecoveryReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt();
	if (!Candidate.IsValid() || !Candidate.IsSynchronized())
	{
		return Reject(
			EStatus::CommitInvariantViolation,
			TEXT("Arc preview Owner handoff replacement failed joint commit validation."),
			RetirementCallCount,
			PreviousSurfaceId,
			NewSurfaceId,
			RetirementResponse,
			PreviousSurfaceCursor,
			RetiredCursor,
			FinalNewCursor,
			Owner.IsValid());
	}

	Owner = MoveTemp(Candidate);
	const bool bOwnerValidAfter = Owner.IsValid() && Owner.IsSynchronized();
	return MakeResult(
		EStatus::Committed,
		TEXT("Arc preview Owner and Adapter atomically committed the ticket-authorized surface handoff."),
		TransitionTicket,
		RetirementCallCount,
		PreviousSurfaceId,
		NewSurfaceId,
		RetirementResponse,
		Receipt,
		PreviousSurfaceCursor,
		RetiredCursor,
		FinalNewCursor,
		bOwnerValidAfter);
}

FResult FHandoff::MakeResult(
	const EStatus Status,
	const TCHAR* Diagnostic,
	const FTicket& TransitionTicket,
	const int32 RetirementCallCount,
	const FGuid& PreviousSurfaceInstanceId,
	const FGuid& SurfaceInstanceId,
	const FResponse& RetirementResponse,
	const FReceipt& Receipt,
	const FState& PreviousSurfaceCursor,
	const FState& RetiredSurfaceCursor,
	const FState& SurfaceCursor,
	const bool bOwnerValidAfter) const
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.TransitionTicket = TransitionTicket;
	Result.RetirementCallCount = RetirementCallCount;
	Result.PreviousSurfaceInstanceId = PreviousSurfaceInstanceId;
	Result.SurfaceInstanceId = SurfaceInstanceId;
	Result.RetirementResponse = RetirementResponse;
	Result.Receipt = Receipt;
	Result.PreviousSurfaceCursor = PreviousSurfaceCursor;
	Result.RetiredSurfaceCursor = RetiredSurfaceCursor;
	Result.SurfaceCursor = SurfaceCursor;
	Result.bOwnerValidAfter = bOwnerValidAfter;
	Result.bValidated = Result.Validate();
	return Result;
}
