#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus;
	using FRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest;
	using FTicket =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionResult;
	using FTransition =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition;
	using FPolicy =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy;
	using FPolicyResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult;
	using FLifecycleResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool IsPhysicalCursor(const FState& State)
	{
		return State.IsEmpty() || (State.IsValid() && State.IsVisible());
	}

	bool IsTransitionAction(const EAction Action)
	{
		return Action == EAction::BindFresh || Action == EAction::AdoptExact
			|| Action == EAction::ClearToEmpty;
	}

	bool IsBindingAction(const EAction Action)
	{
		return Action == EAction::BindFresh || Action == EAction::AdoptExact;
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
				*State.GetPresentationStateId().ToString(EGuidFormats::Digits))
			: TEXT("INVALID_STATE");
	}

	FGuid MakeRequestId(
		const FGuid& RunId,
		const FName ConsumerDefinitionId,
		const FState& AuthoritativeCursor,
		const FGuid& SurfaceInstanceId,
		const EAction Action)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceOwnershipTransitionRequest.r1"),
			{
				GuidKey(RunId),
				NameKey(ConsumerDefinitionId),
				StateKey(AuthoritativeCursor),
				GuidKey(SurfaceInstanceId),
				FString::FromInt(static_cast<int32>(Action))
			});
	}

	FGuid MakeTicketId(
		const FGuid& RequestId,
		const FGuid& PolicyDecisionId,
		const FGuid& PermitId,
		const FGuid& LifecycleReceiptId,
		const FGuid& RunId,
		const FName ConsumerDefinitionId,
		const FGuid& SurfaceInstanceId,
		const EAction Action,
		const FState& ExpectedSurfaceCursor,
		const FState& ObservedSurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceOwnershipTransitionTicket.r1"),
			{
				GuidKey(RequestId),
				GuidKey(PolicyDecisionId),
				GuidKey(PermitId),
				GuidKey(LifecycleReceiptId),
				GuidKey(RunId),
				NameKey(ConsumerDefinitionId),
				GuidKey(SurfaceInstanceId),
				FString::FromInt(static_cast<int32>(Action)),
				StateKey(ExpectedSurfaceCursor),
				StateKey(ObservedSurfaceCursor)
			});
	}

	bool PolicyMatchesRequest(
		const FPolicyResult& PolicyResult,
		const FRequest& Request)
	{
		return PolicyResult.IsValid()
			&& PolicyResult.GetRunId() == Request.GetRunId()
			&& PolicyResult.GetExpectedConsumerDefinitionId()
				== Request.GetConsumerDefinitionId()
			&& PolicyResult.GetRequestedAction() == Request.GetAction()
			&& StatesMatchOrAreEmpty(
				PolicyResult.GetAuthoritativeCursor(),
				Request.GetAuthoritativeCursor());
	}

}

bool FRequest::TryCreate(
	const FGuid& InRunId,
	const FName InConsumerDefinitionId,
	const FState& InAuthoritativeCursor,
	const FGuid& InSurfaceInstanceId,
	const EAction InAction,
	FRequest& OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = FRequest();
	OutDiagnostic.Reset();
	if (!InRunId.IsValid() || InConsumerDefinitionId.IsNone()
		|| !InSurfaceInstanceId.IsValid() || !IsTransitionAction(InAction)
		|| (!InAuthoritativeCursor.IsEmpty()
			&& (!InAuthoritativeCursor.IsValid()
				|| InAuthoritativeCursor.GetRunId() != InRunId)))
	{
		OutDiagnostic = TEXT(
			"Arc preview surface ownership transition requires one Run, consumer, immutable surface identity, valid authority and supported action.");
		return false;
	}

	FRequest Candidate;
	Candidate.RunId = InRunId;
	Candidate.ConsumerDefinitionId = InConsumerDefinitionId;
	Candidate.AuthoritativeCursor = InAuthoritativeCursor;
	Candidate.SurfaceInstanceId = InSurfaceInstanceId;
	Candidate.Action = InAction;
	Candidate.RequestId = MakeRequestId(
		Candidate.RunId,
		Candidate.ConsumerDefinitionId,
		Candidate.AuthoritativeCursor,
		Candidate.SurfaceInstanceId,
		Candidate.Action);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc preview surface ownership transition request failed deterministic validation.");
		return false;
	}
	OutRequest = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview surface ownership transition request sealed one exact candidate scope.");
	return true;
}

bool FRequest::IsValid() const
{
	return RequestId.IsValid() && RunId.IsValid()
		&& !ConsumerDefinitionId.IsNone() && SurfaceInstanceId.IsValid()
		&& IsTransitionAction(Action)
		&& (AuthoritativeCursor.IsEmpty()
			|| (AuthoritativeCursor.IsValid()
				&& AuthoritativeCursor.GetRunId() == RunId))
		&& RequestId == MakeRequestId(
			RunId,
			ConsumerDefinitionId,
			AuthoritativeCursor,
			SurfaceInstanceId,
			Action);
}

bool FTicket::TryRehydrate(
	const FGuid& ExpectedTicketId,
	const FGuid& InRequestId,
	const FGuid& InPolicyDecisionId,
	const FGuid& InPermitId,
	const FGuid& InLifecycleReceiptId,
	const FGuid& InRunId,
	const FName InConsumerDefinitionId,
	const FGuid& InSurfaceInstanceId,
	const EAction InAction,
	const FState& InExpectedSurfaceCursor,
	const FState& InObservedSurfaceCursor,
	FTicket& OutTicket)
{
	OutTicket = FTicket();
	FTicket Candidate;
	Candidate.RequestId = InRequestId;
	Candidate.PolicyDecisionId = InPolicyDecisionId;
	Candidate.PermitId = InPermitId;
	Candidate.LifecycleReceiptId = InLifecycleReceiptId;
	Candidate.RunId = InRunId;
	Candidate.ConsumerDefinitionId = InConsumerDefinitionId;
	Candidate.SurfaceInstanceId = InSurfaceInstanceId;
	Candidate.Action = InAction;
	Candidate.ExpectedSurfaceCursor = InExpectedSurfaceCursor;
	Candidate.ObservedSurfaceCursor = InObservedSurfaceCursor;
	Candidate.TicketId = MakeTicketId(
		Candidate.RequestId,
		Candidate.PolicyDecisionId,
		Candidate.PermitId,
		Candidate.LifecycleReceiptId,
		Candidate.RunId,
		Candidate.ConsumerDefinitionId,
		Candidate.SurfaceInstanceId,
		Candidate.Action,
		Candidate.ExpectedSurfaceCursor,
		Candidate.ObservedSurfaceCursor);
	if (!ExpectedTicketId.IsValid()
		|| Candidate.TicketId != ExpectedTicketId
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutTicket = MoveTemp(Candidate);
	return true;
}

bool FTicket::IsValid() const
{
	if (!TicketId.IsValid() || !RequestId.IsValid()
		|| !PolicyDecisionId.IsValid() || !PermitId.IsValid()
		|| !LifecycleReceiptId.IsValid() || !RunId.IsValid()
		|| ConsumerDefinitionId.IsNone() || !SurfaceInstanceId.IsValid()
		|| !IsBindingAction(Action)
		|| !IsPhysicalCursor(ExpectedSurfaceCursor)
		|| !IsPhysicalCursor(ObservedSurfaceCursor))
	{
		return false;
	}
	if (Action == EAction::BindFresh
		&& (!ExpectedSurfaceCursor.IsEmpty()
			|| !ObservedSurfaceCursor.IsEmpty()))
	{
		return false;
	}
	if (Action == EAction::AdoptExact
		&& (!ExpectedSurfaceCursor.IsVisible()
			|| !ObservedSurfaceCursor.IsVisible()
			|| !ExpectedSurfaceCursor.Matches(ObservedSurfaceCursor)
			|| ExpectedSurfaceCursor.GetRunId() != RunId))
	{
		return false;
	}
	return TicketId == MakeTicketId(
		RequestId,
		PolicyDecisionId,
		PermitId,
		LifecycleReceiptId,
		RunId,
		ConsumerDefinitionId,
		SurfaceInstanceId,
		Action,
		ExpectedSurfaceCursor,
		ObservedSurfaceCursor);
}

bool FTicket::MatchesCandidateSnapshot(
	const FGuid& InSurfaceInstanceId,
	const FName InConsumerDefinitionId,
	const FState& InSurfaceCursor) const
{
	return IsValid() && SurfaceInstanceId == InSurfaceInstanceId
		&& ConsumerDefinitionId == InConsumerDefinitionId
		&& StatesMatchOrAreEmpty(ObservedSurfaceCursor, InSurfaceCursor);
}

bool FResult::Validate() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| IdentityQueryCount < 0 || IdentityQueryCount > 2
		|| PolicySnapshotReadCount < 0 || PolicySnapshotReadCount > 1)
	{
		return false;
	}
	const bool bNoPolicy = !PolicyResult.IsValid();
	const bool bNoLifecycle = !LifecycleResult.IsValid();
	const bool bNoTicket = !TransitionTicket.IsValid();
	if (Status == EStatus::OperationInProgress)
	{
		return IdentityQueryCount == 0 && PolicySnapshotReadCount == 0
			&& !InitialSurfaceInstanceId.IsValid()
			&& !FinalSurfaceInstanceId.IsValid() && bNoPolicy
			&& bNoLifecycle && bNoTicket;
	}
	if (Status == EStatus::RequestInvalid)
	{
		return !Request.IsValid() && IdentityQueryCount == 0
			&& PolicySnapshotReadCount == 0
			&& !InitialSurfaceInstanceId.IsValid()
			&& !FinalSurfaceInstanceId.IsValid() && bNoPolicy
			&& bNoLifecycle && bNoTicket;
	}
	if (!Request.IsValid())
	{
		return false;
	}
	if (Status == EStatus::SurfaceIdentityMismatch)
	{
		return IdentityQueryCount == 1 && PolicySnapshotReadCount == 0
			&& InitialSurfaceInstanceId != Request.GetSurfaceInstanceId()
			&& FinalSurfaceInstanceId == InitialSurfaceInstanceId
			&& bNoPolicy && bNoLifecycle && bNoTicket;
	}
	if (!PolicyMatchesRequest(PolicyResult, Request))
	{
		return Status == EStatus::EvidenceInvariantViolation && bNoTicket;
	}
	if (Status == EStatus::PolicyRejected)
	{
		return IdentityQueryCount == 1 && PolicySnapshotReadCount == 1
			&& InitialSurfaceInstanceId == Request.GetSurfaceInstanceId()
			&& FinalSurfaceInstanceId == InitialSurfaceInstanceId
			&& PolicyResult.IsRejected() && bNoLifecycle && bNoTicket;
	}
	if (IdentityQueryCount != 2 || PolicySnapshotReadCount != 1
		|| InitialSurfaceInstanceId != Request.GetSurfaceInstanceId()
		|| !LifecycleResult.IsValid())
	{
		return Status == EStatus::EvidenceInvariantViolation && bNoTicket;
	}
	if (Status == EStatus::SurfaceIdentityDrift)
	{
		return FinalSurfaceInstanceId != InitialSurfaceInstanceId
			&& bNoTicket;
	}
	if (FinalSurfaceInstanceId != InitialSurfaceInstanceId
		|| !PolicyResult.IsAuthorized())
	{
		return false;
	}
	if (Status == EStatus::LifecycleRejected)
	{
		return !LifecycleResult.IsBindingReady()
			&& !LifecycleResult.DidClear()
			&& !LifecycleResult.WasSurfaceRejected() && bNoTicket;
	}
	if (Status == EStatus::BindingAuthorized)
	{
		return IsBindingAction(Request.GetAction())
			&& LifecycleResult.IsBindingReady()
			&& LifecycleResult.HasReceipt()
			&& TransitionTicket.IsValid()
			&& TransitionTicket.GetRequestId() == Request.GetRequestId()
			&& TransitionTicket.GetPolicyDecisionId()
				== PolicyResult.GetDecisionId()
			&& TransitionTicket.GetPermitId()
				== PolicyResult.GetPermit().GetPermitId()
			&& TransitionTicket.GetLifecycleReceiptId()
				== LifecycleResult.GetReceipt().GetReceiptId()
			&& TransitionTicket.GetSurfaceInstanceId()
				== Request.GetSurfaceInstanceId();
	}
	if (Status == EStatus::CleanupApplied)
	{
		return Request.GetAction() == EAction::ClearToEmpty
			&& LifecycleResult.DidClear() && bNoTicket;
	}
	if (Status == EStatus::CleanupRejected)
	{
		return Request.GetAction() == EAction::ClearToEmpty
			&& LifecycleResult.WasSurfaceRejected() && bNoTicket;
	}
	return Status == EStatus::EvidenceInvariantViolation && bNoTicket;
}

bool FResult::IsAccepted() const
{
	return IsValid()
		&& (Status == EStatus::BindingAuthorized
			|| Status == EStatus::CleanupApplied);
}

bool FResult::DidAuthorizeBinding() const
{
	return IsValid() && Status == EStatus::BindingAuthorized;
}

bool FResult::DidClear() const
{
	return IsValid() && Status == EStatus::CleanupApplied;
}

bool FResult::WasCleanupRejected() const
{
	return IsValid() && Status == EStatus::CleanupRejected;
}

bool FResult::HasTransitionTicket() const
{
	return IsValid() && TransitionTicket.IsValid();
}

bool FResult::DidCallSurface() const
{
	return IsValid() && LifecycleResult.IsValid()
		&& LifecycleResult.DidCallSurface();
}

bool FResult::CanRetryExactRequest() const
{
	return IsValid() && Status == EStatus::CleanupRejected
		&& LifecycleResult.CanRetryExactPermit();
}

bool FResult::NeedsReevaluation() const
{
	return IsValid() && Status == EStatus::CleanupApplied;
}

bool FTransition::TryMakeTicket(
	const FRequest& Request,
	const FPolicyResult& PolicyResult,
	const FLifecycleResult& LifecycleResult,
	FTicket& OutTicket) const
{
	OutTicket = FTicket();
	if (!Request.IsValid() || !PolicyResult.IsAuthorized()
		|| !LifecycleResult.IsBindingReady()
		|| !LifecycleResult.HasReceipt()
		|| !IsBindingAction(Request.GetAction()))
	{
		return false;
	}

	const auto& Permit = PolicyResult.GetPermit();
	const auto& Receipt = LifecycleResult.GetReceipt();
	if (!Permit.IsBindingPermit() || !Receipt.IsBindingReady()
		|| !Receipt.MatchesPermit(Permit))
	{
		return false;
	}

	OutTicket.RequestId = Request.GetRequestId();
	OutTicket.PolicyDecisionId = PolicyResult.GetDecisionId();
	OutTicket.PermitId = Permit.GetPermitId();
	OutTicket.LifecycleReceiptId = Receipt.GetReceiptId();
	OutTicket.RunId = Request.GetRunId();
	OutTicket.ConsumerDefinitionId = Request.GetConsumerDefinitionId();
	OutTicket.SurfaceInstanceId = Request.GetSurfaceInstanceId();
	OutTicket.Action = Request.GetAction();
	OutTicket.ExpectedSurfaceCursor = Permit.GetExpectedSurfaceCursor();
	OutTicket.ObservedSurfaceCursor = Permit.GetObservedSurfaceCursor();
	OutTicket.TicketId = MakeTicketId(
		OutTicket.RequestId,
		OutTicket.PolicyDecisionId,
		OutTicket.PermitId,
		OutTicket.LifecycleReceiptId,
		OutTicket.RunId,
		OutTicket.ConsumerDefinitionId,
		OutTicket.SurfaceInstanceId,
		OutTicket.Action,
		OutTicket.ExpectedSurfaceCursor,
		OutTicket.ObservedSurfaceCursor);
	return OutTicket.IsValid();
}

FResult FTransition::Execute(
	const FRequest& Request,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate&
		Candidate)
{
	if (bOperationInProgress)
	{
		return MakeResult(
			EStatus::OperationInProgress,
			TEXT("Arc preview surface ownership candidate cannot re-enter its transition transaction."),
			Request,
			0,
			0,
			FGuid(),
			FGuid(),
			FPolicyResult(),
			FLifecycleResult(),
			FTicket());
	}
	if (!Request.IsValid())
	{
		return MakeResult(
			EStatus::RequestInvalid,
			TEXT("Arc preview surface ownership transaction requires one valid exact request."),
			Request,
			0,
			0,
			FGuid(),
			FGuid(),
			FPolicyResult(),
			FLifecycleResult(),
			FTicket());
	}

	// Guard every candidate callback, including the first identity read.
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const FGuid InitialSurfaceInstanceId = Candidate.GetSurfaceInstanceId();
	if (InitialSurfaceInstanceId != Request.GetSurfaceInstanceId())
	{
		return MakeResult(
			EStatus::SurfaceIdentityMismatch,
			TEXT("Arc preview surface instance does not match the requested ownership candidate."),
			Request,
			1,
			0,
			InitialSurfaceInstanceId,
			InitialSurfaceInstanceId,
			FPolicyResult(),
			FLifecycleResult(),
			FTicket());
	}

	const FName ObservedConsumerDefinitionId =
		Candidate.GetConsumerDefinitionId();
	const FState ObservedSurfaceCursor = Candidate.GetSurfaceCursor();
	const FPolicyResult PolicyResult = FPolicy::Evaluate(
		Request.GetRunId(),
		Request.GetConsumerDefinitionId(),
		Request.GetAuthoritativeCursor(),
		ObservedConsumerDefinitionId,
		ObservedSurfaceCursor,
		Request.GetAction());
	if (!PolicyResult.IsValid())
	{
		return MakeResult(
			EStatus::EvidenceInvariantViolation,
			TEXT("Arc preview surface ownership policy returned invalid evidence."),
			Request,
			1,
			1,
			InitialSurfaceInstanceId,
			InitialSurfaceInstanceId,
			PolicyResult,
			FLifecycleResult(),
			FTicket());
	}
	if (!PolicyResult.IsAuthorized())
	{
		return MakeResult(
			EStatus::PolicyRejected,
			PolicyResult.GetDiagnostic().IsEmpty()
				? TEXT("Arc preview surface ownership policy rejected this transition.")
				: *PolicyResult.GetDiagnostic(),
			Request,
			1,
			1,
			InitialSurfaceInstanceId,
			InitialSurfaceInstanceId,
			PolicyResult,
			FLifecycleResult(),
			FTicket());
	}

	const FLifecycleResult LifecycleResult = LifecycleExecutor.Execute(
		PolicyResult.GetPermit(), Candidate);
	const FGuid FinalSurfaceInstanceId = Candidate.GetSurfaceInstanceId();
	if (FinalSurfaceInstanceId != InitialSurfaceInstanceId)
	{
		return MakeResult(
			EStatus::SurfaceIdentityDrift,
			TEXT("Arc preview surface identity changed during the ownership transaction."),
			Request,
			2,
			1,
			InitialSurfaceInstanceId,
			FinalSurfaceInstanceId,
			PolicyResult,
			LifecycleResult,
			FTicket());
	}
	if (!LifecycleResult.IsValid())
	{
		return MakeResult(
			EStatus::EvidenceInvariantViolation,
			TEXT("Arc preview surface lifecycle executor returned invalid evidence."),
			Request,
			2,
			1,
			InitialSurfaceInstanceId,
			FinalSurfaceInstanceId,
			PolicyResult,
			LifecycleResult,
			FTicket());
	}
	if (LifecycleResult.IsBindingReady())
	{
		FTicket Ticket;
		if (!TryMakeTicket(Request, PolicyResult, LifecycleResult, Ticket))
		{
			return MakeResult(
				EStatus::EvidenceInvariantViolation,
				TEXT("Arc preview surface ownership transaction could not seal a binding ticket."),
				Request,
				2,
				1,
				InitialSurfaceInstanceId,
				FinalSurfaceInstanceId,
				PolicyResult,
				LifecycleResult,
				FTicket());
		}
		return MakeResult(
			EStatus::BindingAuthorized,
			TEXT("Arc preview surface ownership binding was authorized for one exact surface instance."),
			Request,
			2,
			1,
			InitialSurfaceInstanceId,
			FinalSurfaceInstanceId,
			PolicyResult,
			LifecycleResult,
			Ticket);
	}
	if (LifecycleResult.DidClear())
	{
		return MakeResult(
			EStatus::CleanupApplied,
			TEXT("Arc preview surface ownership cleanup applied; binding requires reevaluation."),
			Request,
			2,
			1,
			InitialSurfaceInstanceId,
			FinalSurfaceInstanceId,
			PolicyResult,
			LifecycleResult,
			FTicket());
	}
	if (LifecycleResult.WasSurfaceRejected())
	{
		return MakeResult(
			EStatus::CleanupRejected,
			TEXT("Arc preview surface rejected one bounded ownership cleanup attempt."),
			Request,
			2,
			1,
			InitialSurfaceInstanceId,
			FinalSurfaceInstanceId,
			PolicyResult,
			LifecycleResult,
			FTicket());
	}
	return MakeResult(
		EStatus::LifecycleRejected,
		LifecycleResult.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview surface lifecycle rejected this transition.")
			: *LifecycleResult.GetDiagnostic(),
		Request,
		2,
		1,
		InitialSurfaceInstanceId,
		FinalSurfaceInstanceId,
		PolicyResult,
		LifecycleResult,
		FTicket());
}

FResult FTransition::MakeResult(
	const EStatus Status,
	const TCHAR* Diagnostic,
	const FRequest& Request,
	const int32 IdentityQueryCount,
	const int32 PolicySnapshotReadCount,
	const FGuid& InitialSurfaceInstanceId,
	const FGuid& FinalSurfaceInstanceId,
	const FPolicyResult& PolicyResult,
	const FLifecycleResult& LifecycleResult,
	const FTicket& TransitionTicket) const
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Request = Request;
	Result.IdentityQueryCount = IdentityQueryCount;
	Result.PolicySnapshotReadCount = PolicySnapshotReadCount;
	Result.InitialSurfaceInstanceId = InitialSurfaceInstanceId;
	Result.FinalSurfaceInstanceId = FinalSurfaceInstanceId;
	Result.PolicyResult = PolicyResult;
	Result.LifecycleResult = LifecycleResult;
	Result.TransitionTicket = TransitionTicket;
	Result.bValidated = Result.Validate();
	return Result;
}
