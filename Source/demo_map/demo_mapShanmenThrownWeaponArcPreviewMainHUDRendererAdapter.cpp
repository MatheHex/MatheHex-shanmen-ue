#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.h"

namespace
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	using ESurfaceOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome;
	using ELifecycleOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome;
	using ERetirementOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome;
	using ERecreationAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using FAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	using FCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using FSurfaceResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse;
	using FLifecycleResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse;
	using FRetirementResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse;

	bool StatesMatchOrAreEmpty(const FState& Left, const FState& Right)
	{
		return (Left.IsEmpty() && Right.IsEmpty()) || Left.Matches(Right);
	}

	FState SurfaceCursorFor(const FState& PresentationState)
	{
		return PresentationState.IsVisible() ? PresentationState : FState();
	}

	FName OutcomeCode(const ECommand Kind, const TCHAR* Suffix)
	{
		const TCHAR* KindName = Kind == ECommand::Show
			? TEXT("Show")
			: Kind == ECommand::Replace
				? TEXT("Replace")
				: Kind == ECommand::Hide ? TEXT("Hide") : TEXT("Invalid");
		return FName(*FString::Printf(
			TEXT("Renderer.ArcPreview.MainHUD.%s%s"), KindName, Suffix));
	}

	FSurfaceResponse TryRejectedResponse(
		const FCommand& Command,
		const FState& Cursor,
		const TCHAR* Suffix)
	{
		FSurfaceResponse Response;
		FString Diagnostic;
		if (!Command.IsValid() || !Command.RequiresRenderMutation()
			|| !StatesMatchOrAreEmpty(
				Cursor, SurfaceCursorFor(Command.GetPreviousState()))
			|| !FSurfaceResponse::TryCreate(
				Command,
				ESurfaceOutcome::Rejected,
				OutcomeCode(Command.GetKind(), Suffix),
				Cursor,
				Cursor,
				Response,
				Diagnostic))
		{
			return FSurfaceResponse();
		}
		return Response;
	}
}

FName FAdapter::StableConsumerDefinitionId()
{
	return FName(TEXT("Renderer.ArcPreview.MainHUD.r1"));
}

bool FAdapter::TryInitialize(
	const FGuid& RequestedSurfaceInstanceId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!RequestedSurfaceInstanceId.IsValid())
	{
		OutDiagnostic = TEXT(
			"MainHUD Arc preview renderer requires a physical surface identity.");
		return false;
	}
	if (IsInitialized())
	{
		if (IsValid() && SurfaceInstanceId == RequestedSurfaceInstanceId)
		{
			OutDiagnostic = TEXT(
				"MainHUD Arc preview renderer was already initialized.");
			return true;
		}
		OutDiagnostic = TEXT(
			"MainHUD Arc preview renderer rejects surface identity rotation.");
		return false;
	}
	if (SurfaceInstanceId.IsValid() || !ConsumerDefinitionId.IsNone()
		|| !SurfaceCursor.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"MainHUD Arc preview renderer rejected a dirty initial state.");
		return false;
	}

	SurfaceInstanceId = RequestedSurfaceInstanceId;
	ConsumerDefinitionId = StableConsumerDefinitionId();
	if (!IsValid())
	{
		SurfaceInstanceId.Invalidate();
		ConsumerDefinitionId = NAME_None;
		OutDiagnostic = TEXT(
			"MainHUD Arc preview renderer failed post-initialize validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"MainHUD Arc preview renderer initialized one empty physical surface.");
	return true;
}

bool FAdapter::IsInitialized() const
{
	return SurfaceInstanceId.IsValid() && !ConsumerDefinitionId.IsNone();
}

bool FAdapter::IsValid() const
{
	return IsInitialized()
		&& ConsumerDefinitionId == StableConsumerDefinitionId()
		&& (SurfaceCursor.IsEmpty() || SurfaceCursor.IsVisible());
}

bool FAdapter::TryRehydrateVisibleForHandoff(
	const FGuid& ExpectedRunId,
	const FName ExpectedConsumerDefinitionId,
	const FState& AuthoritativeVisibleState,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid()
		|| ExpectedConsumerDefinitionId != ConsumerDefinitionId
		|| !AuthoritativeVisibleState.IsValid()
		|| !AuthoritativeVisibleState.IsVisible()
		|| AuthoritativeVisibleState.GetRunId() != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"MainHUD Arc preview rehydrate requires one exact visible Run/consumer snapshot.");
		return false;
	}
	if (SurfaceCursor.IsVisible())
	{
		if (SurfaceCursor.Matches(AuthoritativeVisibleState))
		{
			OutDiagnostic = TEXT(
				"MainHUD Arc preview surface already holds the exact rehydrated cursor.");
			return true;
		}
		OutDiagnostic = TEXT(
			"MainHUD Arc preview rehydrate rejects a conflicting visible cursor.");
		return false;
	}
	if (!SurfaceCursor.IsEmpty())
	{
		OutDiagnostic = TEXT(
			"MainHUD Arc preview rehydrate requires an empty physical surface.");
		return false;
	}

	SurfaceCursor = AuthoritativeVisibleState;
	if (!IsValid())
	{
		SurfaceCursor = FState();
		OutDiagnostic = TEXT(
			"MainHUD Arc preview rehydrate failed post-mutation validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"MainHUD Arc preview surface rehydrated the exact authoritative cursor.");
	return true;
}

bool FAdapter::TryDiscardRehydratedVisibleForHandoff(
	const FState& ExpectedVisibleState,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedVisibleState.IsValid()
		|| !ExpectedVisibleState.IsVisible()
		|| !SurfaceCursor.IsVisible()
		|| !SurfaceCursor.Matches(ExpectedVisibleState))
	{
		OutDiagnostic = TEXT(
			"MainHUD Arc preview rollback requires the exact unadopted visible cursor.");
		return false;
	}
	SurfaceCursor = FState();
	OutDiagnostic = TEXT(
		"MainHUD Arc preview discarded one unadopted rehydrated cursor.");
	return true;
}

FSurfaceResponse FAdapter::Show(const FCommand& Command)
{
	return ApplyMutation(Command, ECommand::Show);
}

FSurfaceResponse FAdapter::Replace(const FCommand& Command)
{
	return ApplyMutation(Command, ECommand::Replace);
}

FSurfaceResponse FAdapter::Hide(const FCommand& Command)
{
	return ApplyMutation(Command, ECommand::Hide);
}

FSurfaceResponse FAdapter::ApplyMutation(
	const FCommand& Command,
	const ECommand ExpectedKind)
{
	const FState Previous = SurfaceCursor;
	if (!Command.IsValid() || !Command.RequiresRenderMutation())
	{
		return FSurfaceResponse();
	}
	if (!IsValid())
	{
		return TryRejectedResponse(Command, Previous, TEXT("NotReady"));
	}
	if (Command.GetKind() != ExpectedKind)
	{
		return TryRejectedResponse(
			Command, Previous, TEXT("EntrypointRejected"));
	}
	if (!StatesMatchOrAreEmpty(
			Previous, SurfaceCursorFor(Command.GetPreviousState())))
	{
		return FSurfaceResponse();
	}

	const FState Candidate = SurfaceCursorFor(Command.GetState());
	FSurfaceResponse Response;
	FString Diagnostic;
	if (!FSurfaceResponse::TryCreate(
			Command,
			ESurfaceOutcome::Applied,
			OutcomeCode(ExpectedKind, TEXT("Applied")),
			Previous,
			Candidate,
			Response,
			Diagnostic))
	{
		return FSurfaceResponse();
	}
	SurfaceCursor = Candidate;
	return Response;
}

FLifecycleResponse FAdapter::ClearToEmpty(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
		Permit)
{
	const FState Previous = SurfaceCursor;
	if (!IsValid() || !Permit.IsCleanupPermit()
		|| Permit.GetConsumerDefinitionId() != ConsumerDefinitionId
		|| !Previous.IsVisible()
		|| !Previous.Matches(Permit.GetObservedSurfaceCursor()))
	{
		return FLifecycleResponse();
	}

	FLifecycleResponse Response;
	FString Diagnostic;
	if (!FLifecycleResponse::TryCreate(
			Permit,
			ELifecycleOutcome::Applied,
			FName(TEXT("Renderer.ArcPreview.MainHUD.CleanupApplied")),
			Previous,
			FState(),
			Response,
			Diagnostic))
	{
		return FLifecycleResponse();
	}
	SurfaceCursor = FState();
	return Response;
}

FRetirementResponse FAdapter::RetireForHandoff(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
		TransitionTicket)
{
	const FState Previous = SurfaceCursor;
	if (!IsValid() || !TransitionTicket.IsValid()
		|| TransitionTicket.GetAction() != ERecreationAction::AdoptExact
		|| TransitionTicket.GetConsumerDefinitionId() != ConsumerDefinitionId
		|| TransitionTicket.GetSurfaceInstanceId() == SurfaceInstanceId
		|| !Previous.IsVisible()
		|| !Previous.Matches(TransitionTicket.GetObservedSurfaceCursor()))
	{
		return FRetirementResponse();
	}

	FRetirementResponse Response;
	FString Diagnostic;
	if (!FRetirementResponse::TryCreate(
			TransitionTicket,
			SurfaceInstanceId,
			ERetirementOutcome::Applied,
			FName(TEXT("Renderer.ArcPreview.MainHUD.RetirementApplied")),
			Previous,
			FState(),
			Response,
			Diagnostic))
	{
		return FRetirementResponse();
	}
	SurfaceCursor = FState();
	return Response;
}
