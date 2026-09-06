#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using EExecutor =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus;
	using EReceipt =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome;
	using EResponse =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome;
	using FExecutor =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor;
	using FPermit =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt;
	using FResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	bool IsSurfaceCursor(const FState& State)
	{
		return State.IsEmpty() || (State.IsValid() && State.IsVisible());
	}

	FString StateKey(const FState& State)
	{
		return State.IsEmpty()
			? TEXT("EMPTY")
			: State.IsValid()
				? State.GetPresentationStateId().ToString(EGuidFormats::Digits)
				: TEXT("INVALID_STATE");
	}

	bool IsKnownResponseOutcome(const EResponse Outcome)
	{
		return Outcome == EResponse::Applied
			|| Outcome == EResponse::Rejected;
	}

	bool IsKnownReceiptOutcome(const EReceipt Outcome)
	{
		return Outcome == EReceipt::BindingReady
			|| Outcome == EReceipt::CleanupApplied
			|| Outcome == EReceipt::CleanupRejected;
	}

	FGuid MakeResponseId(
		const FPermit& Permit,
		const EResponse Outcome,
		const FName OutcomeCode,
		const FState& PreviousSurfaceCursor,
		const FState& SurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceLifecycleResponse.r1"),
			{
				Permit.GetPermitId().ToString(EGuidFormats::Digits),
				FString::FromInt(static_cast<int32>(Outcome)),
				OutcomeCode.IsNone() ? TEXT("NONE") : OutcomeCode.ToString(),
				StateKey(PreviousSurfaceCursor),
				StateKey(SurfaceCursor)
			});
	}

	FGuid MakeReceiptId(
		const FPermit& Permit,
		const EReceipt Outcome,
		const int32 SurfaceCallCount,
		const FResponse& SurfaceResponse,
		const FState& PreviousSurfaceCursor,
		const FState& SurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceLifecycleReceipt.r1"),
			{
				Permit.GetPermitId().ToString(EGuidFormats::Digits),
				FString::FromInt(static_cast<int32>(Outcome)),
				FString::FromInt(SurfaceCallCount),
				SurfaceResponse.IsValid()
					? SurfaceResponse.GetResponseId().ToString(EGuidFormats::Digits)
					: TEXT("NO_RESPONSE"),
				StateKey(PreviousSurfaceCursor),
				StateKey(SurfaceCursor)
			});
	}

	bool ResponseMatchesObserved(
		const FResponse& Response,
		const FState& Previous,
		const FState& Current)
	{
		return Response.IsValid()
			&& StatesMatchOrAreEmpty(
				Response.GetPreviousSurfaceCursor(), Previous)
			&& StatesMatchOrAreEmpty(
				Response.GetSurfaceCursor(), Current);
	}
}

bool FResponse::TryCreate(
	const FPermit& Permit,
	const EResponse InOutcome,
	const FName InOutcomeCode,
	const FState& InPreviousSurfaceCursor,
	const FState& InSurfaceCursor,
	FResponse& OutResponse,
	FString& OutDiagnostic)
{
	OutResponse = FResponse();
	OutDiagnostic.Reset();
	if (!Permit.IsCleanupPermit())
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle response requires one valid cleanup permit.");
		return false;
	}
	if (!IsKnownResponseOutcome(InOutcome) || InOutcomeCode.IsNone()
		|| !IsSurfaceCursor(InPreviousSurfaceCursor)
		|| !IsSurfaceCursor(InSurfaceCursor)
		|| !StatesMatchOrAreEmpty(
			InPreviousSurfaceCursor,
			Permit.GetObservedSurfaceCursor()))
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle response has invalid outcome or cursor evidence.");
		return false;
	}
	if ((InOutcome == EResponse::Applied && !InSurfaceCursor.IsEmpty())
		|| (InOutcome == EResponse::Rejected
			&& !StatesMatchOrAreEmpty(
				InPreviousSurfaceCursor, InSurfaceCursor)))
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle response violates cleanup transition semantics.");
		return false;
	}

	FResponse Candidate;
	Candidate.PermitId = Permit.GetPermitId();
	Candidate.Outcome = InOutcome;
	Candidate.OutcomeCode = InOutcomeCode;
	Candidate.PreviousSurfaceCursor = InPreviousSurfaceCursor;
	Candidate.SurfaceCursor = InSurfaceCursor;
	Candidate.ResponseId = MakeResponseId(
		Permit,
		Candidate.Outcome,
		Candidate.OutcomeCode,
		Candidate.PreviousSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid() || !Candidate.MatchesPermit(Permit))
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle response failed deterministic validation.");
		return false;
	}
	OutResponse = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview lifecycle response sealed one exact cleanup outcome.");
	return true;
}

bool FResponse::IsValid() const
{
	if (!ResponseId.IsValid() || !PermitId.IsValid()
		|| !IsKnownResponseOutcome(Outcome) || OutcomeCode.IsNone()
		|| !IsSurfaceCursor(PreviousSurfaceCursor)
		|| !PreviousSurfaceCursor.IsVisible()
		|| !IsSurfaceCursor(SurfaceCursor))
	{
		return false;
	}
	if ((Outcome == EResponse::Applied && !SurfaceCursor.IsEmpty())
		|| (Outcome == EResponse::Rejected
			&& !StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor)))
	{
		return false;
	}
	return true;
}

bool FResponse::MatchesPermit(const FPermit& Permit) const
{
	return IsValid() && Permit.IsCleanupPermit()
		&& PermitId == Permit.GetPermitId()
		&& StatesMatchOrAreEmpty(
			PreviousSurfaceCursor, Permit.GetObservedSurfaceCursor())
		&& ResponseId == MakeResponseId(
			Permit,
			Outcome,
			OutcomeCode,
			PreviousSurfaceCursor,
			SurfaceCursor);
}

bool FResponse::IsApplied() const
{
	return IsValid() && Outcome == EResponse::Applied;
}

bool FResponse::IsRejected() const
{
	return IsValid() && Outcome == EResponse::Rejected;
}

bool FReceipt::TryCreate(
	const FPermit& Permit,
	const EReceipt InOutcome,
	const int32 InSurfaceCallCount,
	const FResponse& InSurfaceResponse,
	const FState& InPreviousSurfaceCursor,
	const FState& InSurfaceCursor,
	FReceipt& OutReceipt,
	FString& OutDiagnostic)
{
	OutReceipt = FReceipt();
	OutDiagnostic.Reset();
	if (!Permit.IsValid() || !IsKnownReceiptOutcome(InOutcome)
		|| !IsSurfaceCursor(InPreviousSurfaceCursor)
		|| !IsSurfaceCursor(InSurfaceCursor)
		|| !Permit.MatchesSnapshot(
			Permit.GetRunId(),
			Permit.GetConsumerDefinitionId(),
			Permit.GetExpectedSurfaceCursor(),
			InPreviousSurfaceCursor))
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle receipt requires an exact permit snapshot.");
		return false;
	}

	const bool bBinding = InOutcome == EReceipt::BindingReady;
	const bool bCleanupApplied = InOutcome == EReceipt::CleanupApplied;
	const bool bCleanupRejected = InOutcome == EReceipt::CleanupRejected;
	if ((bBinding
			&& (!Permit.IsBindingPermit() || InSurfaceCallCount != 0
				|| InSurfaceResponse.IsValid()
				|| !StatesMatchOrAreEmpty(
					InPreviousSurfaceCursor, InSurfaceCursor)))
		|| ((bCleanupApplied || bCleanupRejected)
			&& (!Permit.IsCleanupPermit() || InSurfaceCallCount != 1
				|| !InSurfaceResponse.IsValid()
				|| !InSurfaceResponse.MatchesPermit(Permit)
				|| !StatesMatchOrAreEmpty(
					InSurfaceResponse.GetPreviousSurfaceCursor(),
					InPreviousSurfaceCursor)
				|| !StatesMatchOrAreEmpty(
					InSurfaceResponse.GetSurfaceCursor(),
					InSurfaceCursor)
				|| (bCleanupApplied != InSurfaceResponse.IsApplied())
				|| (bCleanupRejected != InSurfaceResponse.IsRejected()))))
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle receipt outcome does not match its execution evidence.");
		return false;
	}

	FReceipt Candidate;
	Candidate.PermitId = Permit.GetPermitId();
	Candidate.Outcome = InOutcome;
	Candidate.SurfaceCallCount = InSurfaceCallCount;
	Candidate.SurfaceResponse = InSurfaceResponse;
	Candidate.PreviousSurfaceCursor = InPreviousSurfaceCursor;
	Candidate.SurfaceCursor = InSurfaceCursor;
	Candidate.ReceiptId = MakeReceiptId(
		Permit,
		Candidate.Outcome,
		Candidate.SurfaceCallCount,
		Candidate.SurfaceResponse,
		Candidate.PreviousSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid() || !Candidate.MatchesPermit(Permit))
	{
		OutDiagnostic = TEXT(
			"Arc preview lifecycle receipt failed deterministic validation.");
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Arc preview lifecycle receipt sealed one bounded execution outcome.");
	return true;
}

bool FReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !PermitId.IsValid()
		|| !IsKnownReceiptOutcome(Outcome)
		|| !IsSurfaceCursor(PreviousSurfaceCursor)
		|| !IsSurfaceCursor(SurfaceCursor))
	{
		return false;
	}
	if (Outcome == EReceipt::BindingReady)
	{
		return SurfaceCallCount == 0 && !SurfaceResponse.IsValid()
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor);
	}
	return SurfaceCallCount == 1 && SurfaceResponse.IsValid()
		&& StatesMatchOrAreEmpty(
			SurfaceResponse.GetPreviousSurfaceCursor(),
			PreviousSurfaceCursor)
		&& StatesMatchOrAreEmpty(
			SurfaceResponse.GetSurfaceCursor(), SurfaceCursor)
		&& ((Outcome == EReceipt::CleanupApplied
				&& SurfaceResponse.IsApplied() && SurfaceCursor.IsEmpty())
			|| (Outcome == EReceipt::CleanupRejected
				&& SurfaceResponse.IsRejected()
				&& StatesMatchOrAreEmpty(
					PreviousSurfaceCursor, SurfaceCursor)));
}

bool FReceipt::MatchesPermit(const FPermit& Permit) const
{
	return IsValid() && Permit.IsValid()
		&& PermitId == Permit.GetPermitId()
		&& Permit.MatchesSnapshot(
			Permit.GetRunId(),
			Permit.GetConsumerDefinitionId(),
			Permit.GetExpectedSurfaceCursor(),
			PreviousSurfaceCursor)
		&& ((Outcome == EReceipt::BindingReady
				&& Permit.IsBindingPermit())
			|| (Outcome != EReceipt::BindingReady
				&& Permit.IsCleanupPermit()
				&& SurfaceResponse.MatchesPermit(Permit)))
		&& ReceiptId == MakeReceiptId(
			Permit,
			Outcome,
			SurfaceCallCount,
			SurfaceResponse,
			PreviousSurfaceCursor,
			SurfaceCursor);
}

bool FReceipt::IsBindingReady() const
{
	return IsValid() && Outcome == EReceipt::BindingReady;
}

bool FReceipt::WasCleanupApplied() const
{
	return IsValid() && Outcome == EReceipt::CleanupApplied;
}

bool FReceipt::WasCleanupRejected() const
{
	return IsValid() && Outcome == EReceipt::CleanupRejected;
}

bool FResult::Validate() const
{
	if (Status == EExecutor::Invalid || Diagnostic.IsEmpty()
		|| SurfaceCallCount < 0 || SurfaceCallCount > 1
		|| !IsSurfaceCursor(PreviousSurfaceCursor)
		|| !IsSurfaceCursor(SurfaceCursor))
	{
		return false;
	}
	const bool bZeroCall = SurfaceCallCount == 0;
	const bool bNoEvidence = !SurfaceResponse.IsValid() && !Receipt.IsValid();
	if (Status == EExecutor::PermitInvalid)
	{
		return !Permit.IsValid() && bZeroCall && bNoEvidence
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor);
	}
	if (!Permit.IsValid())
	{
		return false;
	}
	if (Status == EExecutor::OperationInProgress
		|| Status == EExecutor::ConsumerMismatch
		|| Status == EExecutor::SnapshotMismatch)
	{
		return bZeroCall && bNoEvidence
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor);
	}
	if (Status == EExecutor::BindingReady)
	{
		return Permit.IsBindingPermit() && bZeroCall
			&& !SurfaceResponse.IsValid() && Receipt.IsBindingReady()
			&& Receipt.MatchesPermit(Permit)
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor);
	}
	if (!Permit.IsCleanupPermit() || SurfaceCallCount != 1)
	{
		return false;
	}
	if (Status == EExecutor::SurfaceResponseInvalid)
	{
		return !Receipt.IsValid()
			&& (!SurfaceResponse.IsValid()
				|| !SurfaceResponse.MatchesPermit(Permit));
	}
	if (Status == EExecutor::SurfaceInvariantViolation)
	{
		return SurfaceResponse.IsValid()
			&& SurfaceResponse.MatchesPermit(Permit)
			&& !Receipt.IsValid()
			&& !ResponseMatchesObserved(
				SurfaceResponse,
				PreviousSurfaceCursor,
				SurfaceCursor);
	}
	if (Status == EExecutor::SurfaceRejected)
	{
		return SurfaceResponse.IsRejected()
			&& ResponseMatchesObserved(
				SurfaceResponse,
				PreviousSurfaceCursor,
				SurfaceCursor)
			&& Receipt.WasCleanupRejected()
			&& Receipt.MatchesPermit(Permit);
	}
	if (Status == EExecutor::Cleared)
	{
		return SurfaceResponse.IsApplied() && SurfaceCursor.IsEmpty()
			&& ResponseMatchesObserved(
				SurfaceResponse,
				PreviousSurfaceCursor,
				SurfaceCursor)
			&& Receipt.WasCleanupApplied()
			&& Receipt.MatchesPermit(Permit);
	}
	return false;
}

bool FResult::IsAccepted() const
{
	return IsValid()
		&& (Status == EExecutor::BindingReady
			|| Status == EExecutor::Cleared);
}

bool FResult::IsBindingReady() const
{
	return IsValid() && Status == EExecutor::BindingReady;
}

bool FResult::DidClear() const
{
	return IsValid() && Status == EExecutor::Cleared;
}

bool FResult::WasSurfaceRejected() const
{
	return IsValid() && Status == EExecutor::SurfaceRejected;
}

bool FResult::DidCallSurface() const
{
	return IsValid() && SurfaceCallCount == 1;
}

bool FResult::HasReceipt() const
{
	return IsValid() && Receipt.IsValid();
}

bool FResult::CanRetryExactPermit() const
{
	return IsValid() && Status == EExecutor::SurfaceRejected;
}

bool FResult::NeedsReevaluation() const
{
	return IsValid() && Status == EExecutor::Cleared;
}

FResult FExecutor::Execute(
	const FPermit& Permit,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle& Surface)
{
	if (bOperationInProgress)
	{
		return MakeResult(
			EExecutor::OperationInProgress,
			TEXT("Arc preview lifecycle surface callback cannot re-enter its executor."),
			Permit,
			0,
			FResponse(),
			FReceipt(),
			FState(),
			FState());
	}

	// Guard every external surface callback, including the read-only preflight
	// queries. A hostile or accidental query callback therefore cannot recurse
	// into this executor before the mutation guard exists.
	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const FState Previous = Surface.GetSurfaceCursor();
	const auto RejectWithoutCall = [this, &Permit, &Previous](
		const EExecutor Status,
		const TCHAR* Diagnostic)
	{
		return MakeResult(
			Status,
			Diagnostic,
			Permit,
			0,
			FResponse(),
			FReceipt(),
			Previous,
			Previous);
	};

	if (!Permit.IsValid())
	{
		return RejectWithoutCall(
			EExecutor::PermitInvalid,
			TEXT("Arc preview lifecycle executor requires one valid exact permit."));
	}
	if (Surface.GetConsumerDefinitionId()
		!= Permit.GetConsumerDefinitionId())
	{
		return RejectWithoutCall(
			EExecutor::ConsumerMismatch,
			TEXT("Arc preview lifecycle surface consumer does not match the permit."));
	}
	if (!Permit.MatchesSnapshot(
			Permit.GetRunId(),
			Surface.GetConsumerDefinitionId(),
			Permit.GetExpectedSurfaceCursor(),
			Previous))
	{
		return RejectWithoutCall(
			EExecutor::SnapshotMismatch,
			TEXT("Arc preview lifecycle surface changed after policy evaluation."));
	}

	if (Permit.IsBindingPermit())
	{
		FReceipt Receipt;
		FString Diagnostic;
		if (!FReceipt::TryCreate(
				Permit,
				EReceipt::BindingReady,
				0,
				FResponse(),
				Previous,
				Previous,
				Receipt,
				Diagnostic))
		{
			return RejectWithoutCall(
				EExecutor::SurfaceInvariantViolation,
				TEXT("Arc preview lifecycle executor could not seal binding readiness."));
		}
		return MakeResult(
			EExecutor::BindingReady,
			TEXT("Arc preview lifecycle binding permit is ready without surface mutation."),
			Permit,
			0,
			FResponse(),
			Receipt,
			Previous,
			Previous);
	}

	const FResponse Response = Surface.ClearToEmpty(Permit);
	const FState Current = Surface.GetSurfaceCursor();
	if (!Response.IsValid() || !Response.MatchesPermit(Permit))
	{
		return MakeResult(
			EExecutor::SurfaceResponseInvalid,
			TEXT("Arc preview lifecycle surface returned invalid or foreign cleanup evidence."),
			Permit,
			1,
			Response,
			FReceipt(),
			Previous,
			Current);
	}
	if (!ResponseMatchesObserved(Response, Previous, Current))
	{
		return MakeResult(
			EExecutor::SurfaceInvariantViolation,
			TEXT("Arc preview lifecycle response does not match the observed surface mutation."),
			Permit,
			1,
			Response,
			FReceipt(),
			Previous,
			Current);
	}

	const EReceipt ReceiptOutcome = Response.IsApplied()
		? EReceipt::CleanupApplied
		: EReceipt::CleanupRejected;
	FReceipt Receipt;
	FString ReceiptDiagnostic;
	if (!FReceipt::TryCreate(
			Permit,
			ReceiptOutcome,
			1,
			Response,
			Previous,
			Current,
			Receipt,
			ReceiptDiagnostic))
	{
		return MakeResult(
			EExecutor::SurfaceInvariantViolation,
			TEXT("Arc preview lifecycle executor could not seal exact cleanup evidence."),
			Permit,
			1,
			Response,
			FReceipt(),
			Previous,
			Current);
	}
	return MakeResult(
		Response.IsApplied()
			? EExecutor::Cleared
			: EExecutor::SurfaceRejected,
		Response.IsApplied()
			? TEXT("Arc preview lifecycle surface applied one explicit cleanup.")
			: TEXT("Arc preview lifecycle surface rejected one bounded cleanup attempt."),
		Permit,
		1,
		Response,
		Receipt,
		Previous,
		Current);
}

FResult FExecutor::MakeResult(
	const EExecutor Status,
	const TCHAR* Diagnostic,
	const FPermit& Permit,
	const int32 SurfaceCallCount,
	const FResponse& SurfaceResponse,
	const FReceipt& Receipt,
	const FState& PreviousSurfaceCursor,
	const FState& SurfaceCursor) const
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Permit = Permit;
	Result.SurfaceCallCount = SurfaceCallCount;
	Result.SurfaceResponse = SurfaceResponse;
	Result.Receipt = Receipt;
	Result.PreviousSurfaceCursor = PreviousSurfaceCursor;
	Result.SurfaceCursor = SurfaceCursor;
	Result.bValidated = Result.Validate();
	return Result;
}
