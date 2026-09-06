#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using EDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition;
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationOutcome;
	using FPermit =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult;
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

	FState PhysicalCursorFor(const FState& State)
	{
		return State.IsVisible() ? State : FState();
	}

	bool IsKnownAction(const EAction Action)
	{
		return Action == EAction::Inspect || Action == EAction::BindFresh
			|| Action == EAction::AdoptExact
			|| Action == EAction::ClearToEmpty;
	}

	bool IsKnownDisposition(const EDisposition Disposition)
	{
		return Disposition != EDisposition::Invalid;
	}

	bool IsCleanupDisposition(const EDisposition Disposition)
	{
		return Disposition == EDisposition::ResidualVisible
			|| Disposition == EDisposition::ForeignVisible
			|| Disposition == EDisposition::ConflictingVisible;
	}

	bool ActionIsAllowed(
		const EAction Action,
		const EDisposition Disposition)
	{
		switch (Action)
		{
		case EAction::BindFresh:
			return Disposition == EDisposition::FreshEmpty;
		case EAction::AdoptExact:
			return Disposition == EDisposition::ExactVisible;
		case EAction::ClearToEmpty:
			return IsCleanupDisposition(Disposition);
		default:
			return false;
		}
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
		if (!State.IsValid())
		{
			return TEXT("INVALID_STATE");
		}
		return FString::Printf(
			TEXT("%d:%s"),
			static_cast<int32>(State.GetMode()),
			*State.GetPresentationStateId().ToString(EGuidFormats::Digits));
	}

	EDisposition Classify(
		const FGuid& RunId,
		const FName ExpectedConsumerDefinitionId,
		const FState& AuthoritativeCursor,
		const FName ObservedConsumerDefinitionId,
		const FState& ObservedSurfaceCursor,
		FState& OutExpectedSurfaceCursor)
	{
		OutExpectedSurfaceCursor = PhysicalCursorFor(AuthoritativeCursor);
		if (!RunId.IsValid() || ExpectedConsumerDefinitionId.IsNone()
			|| ObservedConsumerDefinitionId.IsNone()
			|| (!AuthoritativeCursor.IsEmpty()
				&& (!AuthoritativeCursor.IsValid()
					|| AuthoritativeCursor.GetRunId() != RunId))
			|| !IsSurfaceCursor(ObservedSurfaceCursor))
		{
			return EDisposition::InputRejected;
		}
		if (ExpectedConsumerDefinitionId != ObservedConsumerDefinitionId)
		{
			return EDisposition::ConsumerMismatch;
		}
		if (OutExpectedSurfaceCursor.IsEmpty()
			&& ObservedSurfaceCursor.IsEmpty())
		{
			return EDisposition::FreshEmpty;
		}
		if (OutExpectedSurfaceCursor.IsVisible()
			&& ObservedSurfaceCursor.IsVisible()
			&& OutExpectedSurfaceCursor.Matches(ObservedSurfaceCursor))
		{
			return EDisposition::ExactVisible;
		}
		if (OutExpectedSurfaceCursor.IsVisible()
			&& ObservedSurfaceCursor.IsEmpty())
		{
			return EDisposition::EmptyNeedsRehydrate;
		}
		if (ObservedSurfaceCursor.IsVisible()
			&& ObservedSurfaceCursor.GetRunId() != RunId)
		{
			return EDisposition::ForeignVisible;
		}
		if (OutExpectedSurfaceCursor.IsEmpty()
			&& ObservedSurfaceCursor.IsVisible())
		{
			return EDisposition::ResidualVisible;
		}
		if (OutExpectedSurfaceCursor.IsVisible()
			&& ObservedSurfaceCursor.IsVisible())
		{
			return EDisposition::ConflictingVisible;
		}
		return EDisposition::InputRejected;
	}

	EOutcome ResolveOutcome(
		const EDisposition Disposition,
		const EAction Action)
	{
		if (!IsKnownDisposition(Disposition)
			|| Disposition == EDisposition::InputRejected)
		{
			return EOutcome::Rejected;
		}
		if (Action == EAction::Inspect)
		{
			return EOutcome::Inspected;
		}
		if (!IsKnownAction(Action)
			|| Disposition == EDisposition::ConsumerMismatch)
		{
			return EOutcome::Rejected;
		}
		return ActionIsAllowed(Action, Disposition)
			? EOutcome::Authorized
			: EOutcome::Rejected;
	}

	FGuid MakePermitId(
		const EAction Action,
		const EDisposition Disposition,
		const FGuid& RunId,
		const FName ConsumerDefinitionId,
		const FState& ExpectedSurfaceCursor,
		const FState& ObservedSurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceRecreationPermit.r1"),
			{
				FString::FromInt(static_cast<int32>(Action)),
				FString::FromInt(static_cast<int32>(Disposition)),
				GuidKey(RunId),
				NameKey(ConsumerDefinitionId),
				StateKey(ExpectedSurfaceCursor),
				StateKey(ObservedSurfaceCursor)
			});
	}

	FGuid MakeDecisionId(
		const EOutcome Outcome,
		const EAction Action,
		const EDisposition Disposition,
		const FGuid& RunId,
		const FName ExpectedConsumerDefinitionId,
		const FName ObservedConsumerDefinitionId,
		const FState& AuthoritativeCursor,
		const FState& ExpectedSurfaceCursor,
		const FState& ObservedSurfaceCursor)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewSurfaceRecreationDecision.r1"),
			{
				FString::FromInt(static_cast<int32>(Outcome)),
				FString::FromInt(static_cast<int32>(Action)),
				FString::FromInt(static_cast<int32>(Disposition)),
				GuidKey(RunId),
				NameKey(ExpectedConsumerDefinitionId),
				NameKey(ObservedConsumerDefinitionId),
				StateKey(AuthoritativeCursor),
				StateKey(ExpectedSurfaceCursor),
				StateKey(ObservedSurfaceCursor)
			});
	}

	const TCHAR* DiagnosticFor(
		const EOutcome Outcome,
		const EDisposition Disposition)
	{
		if (Outcome == EOutcome::Authorized)
		{
			return TEXT(
				"Arc preview surface recreation action was authorized for this exact snapshot.");
		}
		if (Outcome == EOutcome::Inspected)
		{
			return TEXT(
				"Arc preview surface recreation snapshot was classified without mutation authority.");
		}
		switch (Disposition)
		{
		case EDisposition::InputRejected:
			return TEXT(
				"Arc preview surface recreation inputs are invalid or not a physical cursor.");
		case EDisposition::ConsumerMismatch:
			return TEXT(
				"Arc preview surface recreation consumer identity does not match the expected scope.");
		case EDisposition::EmptyNeedsRehydrate:
			return TEXT(
				"An empty recreated surface requires a separate attested rehydrate operation.");
		default:
			return TEXT(
				"The requested Arc preview surface recreation action is incompatible with this snapshot.");
		}
	}
}

bool FPermit::IsValid() const
{
	if (!PermitId.IsValid() || !RunId.IsValid()
		|| ConsumerDefinitionId.IsNone()
		|| !IsSurfaceCursor(ExpectedSurfaceCursor)
		|| !IsSurfaceCursor(ObservedSurfaceCursor)
		|| !ActionIsAllowed(Action, Disposition))
	{
		return false;
	}
	if (Disposition == EDisposition::FreshEmpty
		&& (!ExpectedSurfaceCursor.IsEmpty()
			|| !ObservedSurfaceCursor.IsEmpty()))
	{
		return false;
	}
	if (Disposition == EDisposition::ExactVisible
		&& (!ExpectedSurfaceCursor.IsVisible()
			|| !ObservedSurfaceCursor.IsVisible()
			|| !ExpectedSurfaceCursor.Matches(ObservedSurfaceCursor)
			|| ExpectedSurfaceCursor.GetRunId() != RunId))
	{
		return false;
	}
	if (IsCleanupDisposition(Disposition)
		&& !ObservedSurfaceCursor.IsVisible())
	{
		return false;
	}
	return PermitId == MakePermitId(
		Action,
		Disposition,
		RunId,
		ConsumerDefinitionId,
		ExpectedSurfaceCursor,
		ObservedSurfaceCursor);
}

bool FPermit::IsBindingPermit() const
{
	return IsValid()
		&& (Action == EAction::BindFresh
			|| Action == EAction::AdoptExact);
}

bool FPermit::IsCleanupPermit() const
{
	return IsValid() && Action == EAction::ClearToEmpty;
}

bool FPermit::MatchesSnapshot(
	const FGuid& ExpectedRunId,
	const FName InConsumerDefinitionId,
	const FState& InExpectedSurfaceCursor,
	const FState& InObservedSurfaceCursor) const
{
	return IsValid() && RunId == ExpectedRunId
		&& ConsumerDefinitionId == InConsumerDefinitionId
		&& StatesMatchOrAreEmpty(
			ExpectedSurfaceCursor, InExpectedSurfaceCursor)
		&& StatesMatchOrAreEmpty(
			ObservedSurfaceCursor, InObservedSurfaceCursor);
}

bool FResult::Validate() const
{
	if (!DecisionId.IsValid() || Diagnostic.IsEmpty())
	{
		return false;
	}
	FState RecomputedExpected;
	const EDisposition RecomputedDisposition = Classify(
		RunId,
		ExpectedConsumerDefinitionId,
		AuthoritativeCursor,
		ObservedConsumerDefinitionId,
		ObservedSurfaceCursor,
		RecomputedExpected);
	const EOutcome RecomputedOutcome = ResolveOutcome(
		RecomputedDisposition, RequestedAction);
	if (Disposition != RecomputedDisposition || Outcome != RecomputedOutcome
		|| !StatesMatchOrAreEmpty(
			ExpectedSurfaceCursor, RecomputedExpected)
		|| DecisionId != MakeDecisionId(
			Outcome,
			RequestedAction,
			Disposition,
			RunId,
			ExpectedConsumerDefinitionId,
			ObservedConsumerDefinitionId,
			AuthoritativeCursor,
			ExpectedSurfaceCursor,
			ObservedSurfaceCursor))
	{
		return false;
	}
	if (Outcome != EOutcome::Authorized)
	{
		return !Permit.IsValid();
	}
	return Permit.IsValid()
		&& Permit.GetAction() == RequestedAction
		&& Permit.GetDisposition() == Disposition
		&& Permit.GetRunId() == RunId
		&& Permit.GetConsumerDefinitionId()
			== ExpectedConsumerDefinitionId
		&& Permit.MatchesSnapshot(
			RunId,
			ExpectedConsumerDefinitionId,
			ExpectedSurfaceCursor,
			ObservedSurfaceCursor);
}

bool FResult::IsAccepted() const
{
	return IsValid()
		&& (Outcome == EOutcome::Inspected
			|| Outcome == EOutcome::Authorized);
}

bool FResult::IsInspected() const
{
	return IsValid() && Outcome == EOutcome::Inspected;
}

bool FResult::IsAuthorized() const
{
	return IsValid() && Outcome == EOutcome::Authorized;
}

bool FResult::IsRejected() const
{
	return IsValid() && Outcome == EOutcome::Rejected;
}

bool FResult::CanBindFresh() const
{
	return IsValid() && Disposition == EDisposition::FreshEmpty;
}

bool FResult::CanAdoptExact() const
{
	return IsValid() && Disposition == EDisposition::ExactVisible;
}

bool FResult::NeedsRehydrate() const
{
	return IsValid()
		&& Disposition == EDisposition::EmptyNeedsRehydrate;
}

bool FResult::RequiresExplicitCleanup() const
{
	return IsValid() && IsCleanupDisposition(Disposition);
}

bool FResult::HasPermit() const
{
	return IsValid() && Permit.IsValid();
}

FResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy::
	Evaluate(
		const FGuid& RequestedRunId,
		const FName RequestedConsumerDefinitionId,
		const FState& RequestedAuthoritativeCursor,
		const FName RequestedObservedConsumerDefinitionId,
		const FState& RequestedObservedSurfaceCursor,
		const EAction RequestedAction)
{
	FResult Result;
	Result.RunId = RequestedRunId;
	Result.ExpectedConsumerDefinitionId =
		RequestedConsumerDefinitionId;
	Result.ObservedConsumerDefinitionId =
		RequestedObservedConsumerDefinitionId;
	Result.AuthoritativeCursor = RequestedAuthoritativeCursor;
	Result.ObservedSurfaceCursor = RequestedObservedSurfaceCursor;
	Result.RequestedAction = RequestedAction;
	Result.Disposition = Classify(
		Result.RunId,
		Result.ExpectedConsumerDefinitionId,
		Result.AuthoritativeCursor,
		Result.ObservedConsumerDefinitionId,
		Result.ObservedSurfaceCursor,
		Result.ExpectedSurfaceCursor);
	Result.Outcome = ResolveOutcome(
		Result.Disposition, Result.RequestedAction);
	Result.Diagnostic = DiagnosticFor(
		Result.Outcome, Result.Disposition);

	if (Result.Outcome == EOutcome::Authorized)
	{
		Result.Permit.Action = Result.RequestedAction;
		Result.Permit.Disposition = Result.Disposition;
		Result.Permit.RunId = Result.RunId;
		Result.Permit.ConsumerDefinitionId =
			Result.ExpectedConsumerDefinitionId;
		Result.Permit.ExpectedSurfaceCursor =
			Result.ExpectedSurfaceCursor;
		Result.Permit.ObservedSurfaceCursor =
			Result.ObservedSurfaceCursor;
		Result.Permit.PermitId = MakePermitId(
			Result.Permit.Action,
			Result.Permit.Disposition,
			Result.Permit.RunId,
			Result.Permit.ConsumerDefinitionId,
			Result.Permit.ExpectedSurfaceCursor,
			Result.Permit.ObservedSurfaceCursor);
	}
	Result.DecisionId = MakeDecisionId(
		Result.Outcome,
		Result.RequestedAction,
		Result.Disposition,
		Result.RunId,
		Result.ExpectedConsumerDefinitionId,
		Result.ObservedConsumerDefinitionId,
		Result.AuthoritativeCursor,
		Result.ExpectedSurfaceCursor,
		Result.ObservedSurfaceCursor);
	Result.bValidated = Result.Validate();
	return Result;
}
