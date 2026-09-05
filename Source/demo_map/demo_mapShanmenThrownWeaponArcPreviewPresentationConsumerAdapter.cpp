#include "demo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	using EPortOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus;
	using ESurfaceOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FPortResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using FSurfaceResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse;

	bool IsMutatingCommand(const ECommand Kind)
	{
		return Kind == ECommand::Show || Kind == ECommand::Replace
			|| Kind == ECommand::Hide;
	}

	bool IsKnownSurfaceOutcome(const ESurfaceOutcome Outcome)
	{
		return Outcome == ESurfaceOutcome::Applied
			|| Outcome == ESurfaceOutcome::Rejected;
	}

	bool IsSurfaceCursor(const FState& State)
	{
		return State.IsEmpty() || (State.IsValid() && State.IsVisible());
	}

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	FState SurfaceCursorFor(const FState& PresentationState)
	{
		return PresentationState.IsVisible() ? PresentationState : FState();
	}

	bool SurfaceCursorBelongsToRun(const FState& State, const FGuid& RunId)
	{
		return State.IsEmpty()
			|| (State.IsValid() && State.IsVisible()
				&& State.GetRunId() == RunId);
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString SurfaceStateKey(const FState& State)
	{
		return State.IsEmpty()
			? TEXT("EMPTY")
			: GuidDigits(State.GetPresentationStateId());
	}

	FGuid MakeSurfaceResponseId(
		const FGuid& CommandId,
		const ECommand Kind,
		const ESurfaceOutcome Outcome,
		const FName OutcomeCode,
		const FState& PreviousSurfaceCursor,
		const FState& SurfaceCursor)
	{
		if (!CommandId.IsValid() || !IsMutatingCommand(Kind)
			|| !IsKnownSurfaceOutcome(Outcome) || OutcomeCode.IsNone()
			|| !IsSurfaceCursor(PreviousSurfaceCursor)
			|| !IsSurfaceCursor(SurfaceCursor))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationSurfaceResponse.r1"),
			{
				GuidDigits(CommandId),
				FString::FromInt(static_cast<int32>(Kind)),
				FString::FromInt(static_cast<int32>(Outcome)),
				OutcomeCode.ToString(),
				SurfaceStateKey(PreviousSurfaceCursor),
				SurfaceStateKey(SurfaceCursor)
			});
	}

	bool SurfaceTransitionIsValid(
		const ECommand Kind,
		const ESurfaceOutcome Outcome,
		const FState& Previous,
		const FState& Current)
	{
		if (!IsMutatingCommand(Kind) || !IsKnownSurfaceOutcome(Outcome)
			|| !IsSurfaceCursor(Previous) || !IsSurfaceCursor(Current))
		{
			return false;
		}
		if (Outcome == ESurfaceOutcome::Rejected)
		{
			return StatesMatchOrAreEmpty(Previous, Current);
		}
		switch (Kind)
		{
		case ECommand::Show:
			return Previous.IsEmpty() && Current.IsVisible();
		case ECommand::Replace:
			return Previous.IsVisible() && Current.IsVisible()
				&& !Previous.Matches(Current);
		case ECommand::Hide:
			return Previous.IsVisible() && Current.IsEmpty();
		default:
			return false;
		}
	}

	bool SurfaceResponseMatchesCommand(
		const FSurfaceResponse& Response,
		const FCommand& Command)
	{
		if (!Response.IsValid() || !Command.IsValid()
			|| !Command.RequiresRenderMutation()
			|| Response.GetCommandId() != Command.GetCommandId()
			|| Response.GetCommandKind() != Command.GetKind())
		{
			return false;
		}
		const FState ExpectedPrevious =
			SurfaceCursorFor(Command.GetPreviousState());
		if (!StatesMatchOrAreEmpty(
				Response.GetPreviousSurfaceCursor(), ExpectedPrevious))
		{
			return false;
		}
		const FState ExpectedCurrent = Response.IsApplied()
			? SurfaceCursorFor(Command.GetState())
			: ExpectedPrevious;
		return StatesMatchOrAreEmpty(
			Response.GetSurfaceCursor(), ExpectedCurrent);
	}

	FName AdapterCode(const TCHAR* Suffix)
	{
		return FName(
			*FString::Printf(
				TEXT("Renderer.ArcPreview.ConsumerAdapter.%s"), Suffix));
	}

	FPortResponse MakePortResponse(
		const FCommand& Command,
		const EPortOutcome Outcome,
		const FName OutcomeCode)
	{
		FPortResponse Response;
		FString Diagnostic;
		if (!Command.IsValid()
			|| !FPortResponse::TryCreate(
				Command.GetCommandId(),
				Outcome,
				OutcomeCode,
				Response,
				Diagnostic))
		{
			return FPortResponse();
		}
		return Response;
	}

	bool PortResponseMatches(
		const FPortResponse& Response,
		const FCommand& Command,
		const bool bApplied,
		const FName OutcomeCode)
	{
		return Response.IsValid() && Command.IsValid()
			&& Response.GetCommandId() == Command.GetCommandId()
			&& Response.GetOutcomeCode() == OutcomeCode
			&& (bApplied ? Response.IsApplied() : Response.IsRejected());
	}

	bool IsPreflightRejection(const EStatus Status)
	{
		return Status == EStatus::AdapterInactive
			|| Status == EStatus::AdapterInvalid
			|| Status == EStatus::OperationInProgress
			|| Status == EStatus::RunMismatch
			|| Status == EStatus::CursorMismatch;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse::
	TryCreate(
		const FCommand& Command,
		const ESurfaceOutcome InOutcome,
		const FName InOutcomeCode,
		const FState& InPreviousSurfaceCursor,
		const FState& InSurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse&
			OutResponse,
		FString& OutDiagnostic)
{
	OutResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse();
	OutDiagnostic.Reset();
	if (!Command.IsValid() || !Command.RequiresRenderMutation())
	{
		OutDiagnostic = TEXT(
			"Arc preview surface response requires one mutating command.");
		return false;
	}
	if (!IsKnownSurfaceOutcome(InOutcome) || InOutcomeCode.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview surface response requires Applied or Rejected and one stable outcome code.");
		return false;
	}
	if (!SurfaceTransitionIsValid(
			Command.GetKind(),
			InOutcome,
			InPreviousSurfaceCursor,
			InSurfaceCursor))
	{
		OutDiagnostic = TEXT(
			"Arc preview surface response does not describe a valid visible-cursor transition.");
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse Candidate;
	Candidate.CommandId = Command.GetCommandId();
	Candidate.CommandKind = Command.GetKind();
	Candidate.Outcome = InOutcome;
	Candidate.OutcomeCode = InOutcomeCode;
	Candidate.PreviousSurfaceCursor = InPreviousSurfaceCursor;
	Candidate.SurfaceCursor = InSurfaceCursor;
	Candidate.ResponseId = MakeSurfaceResponseId(
		Candidate.CommandId,
		Candidate.CommandKind,
		Candidate.Outcome,
		Candidate.OutcomeCode,
		Candidate.PreviousSurfaceCursor,
		Candidate.SurfaceCursor);
	if (!Candidate.IsValid() || !Candidate.MatchesCommand(Command))
	{
		OutDiagnostic = TEXT(
			"Arc preview surface response failed command-bound deterministic validation.");
		return false;
	}
	OutResponse = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Captured one immutable Arc preview surface response.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse::
	IsValid() const
{
	return ResponseId.IsValid() && CommandId.IsValid()
		&& IsMutatingCommand(CommandKind)
		&& IsKnownSurfaceOutcome(Outcome) && !OutcomeCode.IsNone()
		&& SurfaceTransitionIsValid(
			CommandKind, Outcome, PreviousSurfaceCursor, SurfaceCursor)
		&& ResponseId == MakeSurfaceResponseId(
			CommandId,
			CommandKind,
			Outcome,
			OutcomeCode,
			PreviousSurfaceCursor,
			SurfaceCursor);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse::
	MatchesCommand(const FCommand& Command) const
{
	return SurfaceResponseMatchesCommand(*this, Command);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse::
	IsApplied() const
{
	return IsValid() && Outcome == ESurfaceOutcome::Applied;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse::
	IsRejected() const
{
	return IsValid() && Outcome == ESurfaceOutcome::Rejected;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult::
	IsValid() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| SurfaceCallCount < 0 || SurfaceCallCount > 1
		|| !IsSurfaceCursor(PreviousSurfaceCursor)
		|| !IsSurfaceCursor(SurfaceCursor))
	{
		return false;
	}
	if (Status == EStatus::CommandInvalid)
	{
		return !Command.IsValid() && SurfaceCallCount == 0
			&& !SurfaceResponse.IsValid() && !PortResponse.IsValid();
	}
	if (!Command.IsValid())
	{
		return false;
	}
	if (Status == EStatus::AdapterInactive)
	{
		return !RunId.IsValid() && ConsumerDefinitionId.IsNone()
			&& SurfaceCallCount == 0 && !SurfaceResponse.IsValid()
			&& PortResponseMatches(
				PortResponse,
				Command,
				false,
				AdapterCode(TEXT("Inactive")));
	}
	if (!RunId.IsValid() || ConsumerDefinitionId.IsNone())
	{
		return false;
	}
	if (IsPreflightRejection(Status))
	{
		const FName Code = Status == EStatus::AdapterInvalid
			? AdapterCode(TEXT("Invalid"))
			: Status == EStatus::OperationInProgress
				? AdapterCode(TEXT("OperationInProgress"))
				: Status == EStatus::RunMismatch
					? AdapterCode(TEXT("RunMismatch"))
					: AdapterCode(TEXT("CursorMismatch"));
		return SurfaceCallCount == 0 && !SurfaceResponse.IsValid()
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor)
			&& PortResponseMatches(
				PortResponse, Command, false, Code);
	}
	if (Status == EStatus::NoOpApplied)
	{
		return Command.IsNoOp() && SurfaceCallCount == 0
			&& !SurfaceResponse.IsValid()
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor)
			&& StatesMatchOrAreEmpty(
				SurfaceCursor, SurfaceCursorFor(Command.GetState()))
			&& PortResponseMatches(
				PortResponse,
				Command,
				true,
				AdapterCode(TEXT("NoOpApplied")));
	}
	if (SurfaceCallCount != 1)
	{
		return false;
	}
	if (Status == EStatus::SurfaceRejected)
	{
		return SurfaceResponse.IsRejected()
			&& SurfaceResponse.MatchesCommand(Command)
			&& StatesMatchOrAreEmpty(
				PreviousSurfaceCursor, SurfaceCursor)
			&& PortResponseMatches(
				PortResponse,
				Command,
				false,
				SurfaceResponse.GetOutcomeCode());
	}
	if (Status == EStatus::Applied)
	{
		return SurfaceResponse.IsApplied()
			&& SurfaceResponse.MatchesCommand(Command)
			&& StatesMatchOrAreEmpty(
				SurfaceResponse.GetPreviousSurfaceCursor(),
				PreviousSurfaceCursor)
			&& StatesMatchOrAreEmpty(
				SurfaceResponse.GetSurfaceCursor(), SurfaceCursor)
			&& StatesMatchOrAreEmpty(
				SurfaceCursor, SurfaceCursorFor(Command.GetState()))
			&& PortResponseMatches(
				PortResponse,
				Command,
				true,
				SurfaceResponse.GetOutcomeCode());
	}
	if (Status == EStatus::SurfaceResponseInvalid)
	{
		return PortResponseMatches(
			PortResponse,
			Command,
			false,
			AdapterCode(TEXT("InvalidSurfaceResponse")));
	}
	if (Status == EStatus::SurfaceInvariantViolation)
	{
		return PortResponseMatches(
			PortResponse,
			Command,
			false,
			AdapterCode(TEXT("SurfaceInvariantViolation")));
	}
	return false;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult::
	IsAccepted() const
{
	return IsValid()
		&& (Status == EStatus::Applied
			|| Status == EStatus::NoOpApplied
			|| Status == EStatus::SurfaceRejected);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult::
	WasApplied() const
{
	return IsValid()
		&& (Status == EStatus::Applied || Status == EStatus::NoOpApplied);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult::
	WasRejected() const
{
	return IsValid() && Status != EStatus::Applied
		&& Status != EStatus::NoOpApplied;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult::
	DidCallSurface() const
{
	return IsValid() && SurfaceCallCount == 1;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult::
	IsNoOp() const
{
	return IsValid() && Status == EStatus::NoOpApplied;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::
	TryBegin(
		const FGuid& RequestedRunId,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface&
			RequestedSurface,
		FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	const FName RequestedConsumer =
		RequestedSurface.GetConsumerDefinitionId();
	if (!RequestedRunId.IsValid() || RequestedConsumer.IsNone())
	{
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter requires a Run and stable surface consumer identity.");
		return false;
	}
	if (bOperationInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter cannot begin during a surface callback.");
		return false;
	}
	if (IsActive())
	{
		if (IsValid() && RunId == RequestedRunId
			&& ConsumerDefinitionId == RequestedConsumer
			&& Surface == &RequestedSurface)
		{
			OutDiagnostic = TEXT(
				"Arc preview consumer adapter scope was already active.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter rejects active Run or surface rotation.");
		return false;
	}
	if (!IsValid() || !RequestedSurface.GetSurfaceCursor().IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter can bind only an empty valid surface.");
		return false;
	}

	RunId = RequestedRunId;
	ConsumerDefinitionId = RequestedConsumer;
	Surface = &RequestedSurface;
	LastResult = FResult();
	if (!IsValid())
	{
		RunId.Invalidate();
		ConsumerDefinitionId = NAME_None;
		Surface = nullptr;
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter failed post-bind validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Arc preview consumer adapter bound one empty surface for the Run.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::
	TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic = TEXT("Arc preview consumer adapter is inactive.");
		return false;
	}
	if (bOperationInProgress)
	{
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter cannot end during a surface callback.");
		return false;
	}
	if (!IsValid() || !ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter end requires its exact valid Run.");
		return false;
	}
	if (!CanEnd())
	{
		OutDiagnostic = TEXT(
			"Arc preview consumer adapter cannot end while the surface is visible.");
		return false;
	}

	RunId.Invalidate();
	ConsumerDefinitionId = NAME_None;
	Surface = nullptr;
	bOperationInProgress = false;
	LastResult = FResult();
	OutDiagnostic = TEXT(
		"Arc preview consumer adapter released its empty surface.");
	return IsEmpty();
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::Apply(
	const FCommand& Command)
{
	const FState Previous = GetSurfaceCursor();
	const auto Finish = [this](const FResult& Result)
	{
		LastResult = Result;
		return Result.GetPortResponse();
	};
	if (!Command.IsValid())
	{
		return Finish(MakeResult(
			EStatus::CommandInvalid,
			TEXT("Arc preview consumer adapter requires one valid command."),
			Command,
			0,
			FSurfaceResponse(),
			FPortResponse(),
			Previous,
			Previous));
	}
	if (!IsActive())
	{
		return Finish(MakeResult(
			EStatus::AdapterInactive,
			TEXT("Arc preview consumer adapter is inactive."),
			Command,
			0,
			FSurfaceResponse(),
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("Inactive"))),
			Previous,
			Previous));
	}
	if (bOperationInProgress)
	{
		return Finish(MakeResult(
			EStatus::OperationInProgress,
			TEXT("Arc preview surface callback cannot re-enter its adapter."),
			Command,
			0,
			FSurfaceResponse(),
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("OperationInProgress"))),
			Previous,
			Previous));
	}
	if (!IsValid())
	{
		return Finish(MakeResult(
			EStatus::AdapterInvalid,
			TEXT("Arc preview consumer adapter failed active validation."),
			Command,
			0,
			FSurfaceResponse(),
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("Invalid"))),
			Previous,
			Previous));
	}
	if (Command.GetRunId() != RunId)
	{
		return Finish(MakeResult(
			EStatus::RunMismatch,
			TEXT("Arc preview command belongs to another adapter Run."),
			Command,
			0,
			FSurfaceResponse(),
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("RunMismatch"))),
			Previous,
			Previous));
	}
	const FState ExpectedPrevious =
		SurfaceCursorFor(Command.GetPreviousState());
	if (!StatesMatchOrAreEmpty(Previous, ExpectedPrevious))
	{
		return Finish(MakeResult(
			EStatus::CursorMismatch,
			TEXT("Arc preview command does not continue the surface cursor."),
			Command,
			0,
			FSurfaceResponse(),
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("CursorMismatch"))),
			Previous,
			Previous));
	}
	if (Command.IsNoOp())
	{
		return Finish(MakeResult(
			EStatus::NoOpApplied,
			TEXT("Arc preview NoOp matched the surface cursor without mutation."),
			Command,
			0,
			FSurfaceResponse(),
			MakePortResponse(
				Command,
				EPortOutcome::Applied,
				AdapterCode(TEXT("NoOpApplied"))),
			Previous,
			Previous));
	}

	bOperationInProgress = true;
	FSurfaceResponse SurfaceResponse;
	switch (Command.GetKind())
	{
	case ECommand::Show:
		SurfaceResponse = Surface->Show(Command);
		break;
	case ECommand::Replace:
		SurfaceResponse = Surface->Replace(Command);
		break;
	case ECommand::Hide:
		SurfaceResponse = Surface->Hide(Command);
		break;
	default:
		break;
	}
	bOperationInProgress = false;
	const FState Current = GetSurfaceCursor();
	if (!SurfaceResponse.IsValid()
		|| !SurfaceResponse.MatchesCommand(Command))
	{
		return Finish(MakeResult(
			EStatus::SurfaceResponseInvalid,
			TEXT("Arc preview surface returned invalid or foreign response evidence."),
			Command,
			1,
			SurfaceResponse,
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("InvalidSurfaceResponse"))),
			Previous,
			Current));
	}
	if (!StatesMatchOrAreEmpty(
			SurfaceResponse.GetPreviousSurfaceCursor(), Previous)
		|| !StatesMatchOrAreEmpty(
			SurfaceResponse.GetSurfaceCursor(), Current))
	{
		return Finish(MakeResult(
			EStatus::SurfaceInvariantViolation,
			TEXT("Arc preview surface response does not match its observed cursor."),
			Command,
			1,
			SurfaceResponse,
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("SurfaceInvariantViolation"))),
			Previous,
			Current));
	}
	if (SurfaceResponse.IsRejected())
	{
		return Finish(MakeResult(
			EStatus::SurfaceRejected,
			TEXT("Arc preview surface rejected the command without mutation."),
			Command,
			1,
			SurfaceResponse,
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				SurfaceResponse.GetOutcomeCode()),
			Previous,
			Current));
	}
	const FState ExpectedCurrent = SurfaceCursorFor(Command.GetState());
	if (!StatesMatchOrAreEmpty(Current, ExpectedCurrent))
	{
		return Finish(MakeResult(
			EStatus::SurfaceInvariantViolation,
			TEXT("Applied Arc preview surface command produced the wrong cursor."),
			Command,
			1,
			SurfaceResponse,
			MakePortResponse(
				Command,
				EPortOutcome::Rejected,
				AdapterCode(TEXT("SurfaceInvariantViolation"))),
			Previous,
			Current));
	}
	return Finish(MakeResult(
		EStatus::Applied,
		TEXT("Arc preview consumer adapter applied one typed surface mutation."),
		Command,
		1,
		SurfaceResponse,
		MakePortResponse(
			Command,
			EPortOutcome::Applied,
			SurfaceResponse.GetOutcomeCode()),
		Previous,
		Current));
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::
	IsValid() const
{
	if (!IsActive())
	{
		return !RunId.IsValid() && ConsumerDefinitionId.IsNone()
			&& Surface == nullptr && !bOperationInProgress;
	}
	if (!RunId.IsValid() || ConsumerDefinitionId.IsNone()
		|| Surface == nullptr
		|| Surface->GetConsumerDefinitionId() != ConsumerDefinitionId)
	{
		return false;
	}
	return SurfaceCursorBelongsToRun(
		Surface->GetSurfaceCursor(), RunId);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::
	CanEnd() const
{
	return IsValid() && IsActive() && !bOperationInProgress
		&& Surface->GetSurfaceCursor().IsEmpty();
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::
	GetSurfaceCursor() const
{
	return Surface != nullptr ? Surface->GetSurfaceCursor() : FState();
}

FResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter::MakeResult(
	const EStatus Status,
	const TCHAR* Diagnostic,
	const FCommand& Command,
	const int32 SurfaceCallCount,
	const FSurfaceResponse& SurfaceResponse,
	const FPortResponse& PortResponse,
	const FState& PreviousSurfaceCursor,
	const FState& SurfaceCursor) const
{
	FResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.RunId = RunId;
	Result.ConsumerDefinitionId = ConsumerDefinitionId;
	Result.Command = Command;
	Result.SurfaceCallCount = SurfaceCallCount;
	Result.SurfaceResponse = SurfaceResponse;
	Result.PortResponse = PortResponse;
	Result.PreviousSurfaceCursor = PreviousSurfaceCursor;
	Result.SurfaceCursor = SurfaceCursor;
	return Result;
}
