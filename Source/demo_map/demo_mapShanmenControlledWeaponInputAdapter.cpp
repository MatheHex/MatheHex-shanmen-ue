#include "demo_mapShanmenControlledWeaponInputAdapter.h"

#include "demo_mapShanmenControlledWeaponActor.h"
#include "demo_mapShanmenControlledWeaponProductController.h"
#include "demo_mapShanmenControlledWeaponWorldLifecycle.h"

bool Fdemo_mapShanmenControlledWeaponInputReadModel::TryCapture(
	const FGuid& RequestedRunId,
	const FGuid& RequestedItemInstanceId,
	const EShanmenControlledWeaponState RequestedState,
	Fdemo_mapShanmenControlledWeaponInputReadModel& OutReadModel)
{
	OutReadModel = Fdemo_mapShanmenControlledWeaponInputReadModel();
	if (!RequestedRunId.IsValid()
		|| !RequestedItemInstanceId.IsValid()
		|| (RequestedState != EShanmenControlledWeaponState::Orbiting
			&& RequestedState != EShanmenControlledWeaponState::Directed))
	{
		return false;
	}
	OutReadModel.RunId = RequestedRunId;
	OutReadModel.ItemInstanceId = RequestedItemInstanceId;
	OutReadModel.State = RequestedState;
	return OutReadModel.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponInputReadModel::IsValid() const
{
	return RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& (State == EShanmenControlledWeaponState::Orbiting
			|| State == EShanmenControlledWeaponState::Directed);
}

bool Fdemo_mapShanmenControlledWeaponInputResult::IsAccepted() const
{
	if (Status != Edemo_mapShanmenControlledWeaponInputStatus::Applied
		|| !bCanonicalReadInvoked
		|| !bIntentIdCreated
		|| !bIntentCaptured
		|| !bProductRouteInvoked
		|| !ReadModel.IsValid()
		|| !IntentId.IsValid()
		|| !Intent.IsValid()
		|| !ProductRoute.IsAccepted())
	{
		return false;
	}
	switch (Intent.GetKind())
	{
	case EShanmenControlledWeaponCommandKind::Launch:
		return ReadModel.GetState() == EShanmenControlledWeaponState::Orbiting
			&& bDirectionSampled;
	case EShanmenControlledWeaponCommandKind::Redirect:
		return ReadModel.GetState() == EShanmenControlledWeaponState::Directed
			&& bDirectionSampled;
	case EShanmenControlledWeaponCommandKind::Recall:
		return ReadModel.GetState() == EShanmenControlledWeaponState::Directed
			&& !bDirectionSampled;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenControlledWeaponInputAdapter::TryReadCanonical(
	const Fdemo_mapShanmenControlledWeaponWorldLifecycle& Lifecycle,
	const Fdemo_mapShanmenControlledWeaponRunHost& Host,
	Fdemo_mapShanmenControlledWeaponInputReadModel& OutReadModel)
{
	OutReadModel = Fdemo_mapShanmenControlledWeaponInputReadModel();
	if (!Lifecycle.IsActive()
		|| !Host.IsValid()
		|| Lifecycle.GetRunId() != Host.GetRunId())
	{
		return false;
	}

	const FGuid& ItemInstanceId = Lifecycle.GetItemInstanceId();
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Host.FindController(ItemInstanceId);
	if (!Controller
		|| Controller->GetWeaponActor() != Lifecycle.GetWeaponActor())
	{
		return false;
	}

	const EShanmenControlledWeaponState State = Controller->IsOrbiting()
		? EShanmenControlledWeaponState::Orbiting
		: Controller->IsDirected()
			? EShanmenControlledWeaponState::Directed
			: EShanmenControlledWeaponState::Recalled;
	return Fdemo_mapShanmenControlledWeaponInputReadModel::TryCapture(
		Lifecycle.GetRunId(), ItemInstanceId, State, OutReadModel);
}

Fdemo_mapShanmenControlledWeaponInputResult
Fdemo_mapShanmenControlledWeaponInputAdapter::RouteToggleInput(
	const bool bGameplayInputAllowed,
	const bool bProductRouteAvailable,
	TFunctionRef<bool(
		Fdemo_mapShanmenControlledWeaponInputReadModel&)> ReadCanonicalWeapon,
	TFunctionRef<FGuid()> CreateIntentId,
	TFunctionRef<FVector()> SampleLaunchDirection,
	TFunctionRef<Fdemo_mapShanmenControlledWeaponRunCommandResult(
		const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)> RouteIntent)
{
	Fdemo_mapShanmenControlledWeaponInputResult Result;
	if (!bGameplayInputAllowed)
	{
		Result.Diagnostic =
			TEXT("Controlled-weapon input is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bProductRouteAvailable)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::ProductRouteUnavailable;
		Result.Diagnostic =
			TEXT("Controlled-weapon input requires the authoritative GameMode route.");
		return Result;
	}

	Result.bCanonicalReadInvoked = true;
	if (!ReadCanonicalWeapon(Result.ReadModel)
		|| !Result.ReadModel.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::
				CanonicalWeaponUnavailable;
		Result.Diagnostic =
			TEXT("No active canonical TrainingFlyingSword can consume this input.");
		return Result;
	}

	Result.bIntentIdCreated = true;
	Result.IntentId = CreateIntentId();
	if (!Result.IntentId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::IntentIdInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon input could not create a valid intent identity.");
		return Result;
	}

	const bool bLaunch = Result.ReadModel.GetState()
		== EShanmenControlledWeaponState::Orbiting;
	if (bLaunch)
	{
		Result.bDirectionSampled = true;
		Result.SampledDirection = SampleLaunchDirection();
	}
	const TArray<FGuid> TargetItemInstanceIds = {
		Result.ReadModel.GetItemInstanceId() };
	if (!Fdemo_mapShanmenControlledWeaponRunCommandIntent::TryCapture(
			Result.IntentId,
			Result.ReadModel.GetRunId(),
			bLaunch
				? EShanmenControlledWeaponCommandKind::Launch
				: EShanmenControlledWeaponCommandKind::Recall,
			TargetItemInstanceIds,
			Result.SampledDirection,
			Result.Intent))
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::IntentCaptureRejected;
		Result.Diagnostic = bLaunch
			? TEXT("Controlled-weapon launch requires one valid current aim direction.")
			: TEXT("Controlled-weapon recall intent capture was rejected.");
		return Result;
	}
	Result.bIntentCaptured = true;

	Result.bProductRouteInvoked = true;
	Result.ProductRoute = RouteIntent(Result.Intent);
	Result.Status = Result.ProductRoute.IsAccepted()
		? Edemo_mapShanmenControlledWeaponInputStatus::Applied
		: Edemo_mapShanmenControlledWeaponInputStatus::ProductRejected;
	Result.Diagnostic = Result.ProductRoute.Diagnostic;
	return Result;
}

Fdemo_mapShanmenControlledWeaponInputResult
Fdemo_mapShanmenControlledWeaponInputAdapter::RouteRedirectInput(
	const bool bGameplayInputAllowed,
	const bool bProductRouteAvailable,
	TFunctionRef<bool(
		Fdemo_mapShanmenControlledWeaponInputReadModel&)> ReadCanonicalWeapon,
	TFunctionRef<FGuid()> CreateIntentId,
	TFunctionRef<FVector()> SampleRedirectDirection,
	TFunctionRef<Fdemo_mapShanmenControlledWeaponRunCommandResult(
		const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)> RouteIntent)
{
	Fdemo_mapShanmenControlledWeaponInputResult Result;
	if (!bGameplayInputAllowed)
	{
		Result.Diagnostic =
			TEXT("Controlled-weapon redirect is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bProductRouteAvailable)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::ProductRouteUnavailable;
		Result.Diagnostic =
			TEXT("Controlled-weapon redirect requires the authoritative GameMode route.");
		return Result;
	}

	Result.bCanonicalReadInvoked = true;
	if (!ReadCanonicalWeapon(Result.ReadModel)
		|| !Result.ReadModel.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::
				CanonicalWeaponUnavailable;
		Result.Diagnostic =
			TEXT("No active canonical TrainingFlyingSword can consume this redirect.");
		return Result;
	}
	if (Result.ReadModel.GetState()
		!= EShanmenControlledWeaponState::Directed)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::CommandUnavailable;
		Result.Diagnostic =
			TEXT("Controlled-weapon redirect is available only during directed flight.");
		return Result;
	}

	Result.bIntentIdCreated = true;
	Result.IntentId = CreateIntentId();
	if (!Result.IntentId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::IntentIdInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon redirect could not create a valid intent identity.");
		return Result;
	}

	Result.bDirectionSampled = true;
	Result.SampledDirection = SampleRedirectDirection();
	const TArray<FGuid> TargetItemInstanceIds = {
		Result.ReadModel.GetItemInstanceId() };
	if (!Fdemo_mapShanmenControlledWeaponRunCommandIntent::TryCapture(
			Result.IntentId,
			Result.ReadModel.GetRunId(),
			EShanmenControlledWeaponCommandKind::Redirect,
			TargetItemInstanceIds,
			Result.SampledDirection,
			Result.Intent))
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponInputStatus::IntentCaptureRejected;
		Result.Diagnostic =
			TEXT("Controlled-weapon redirect requires one valid current aim direction.");
		return Result;
	}
	Result.bIntentCaptured = true;

	Result.bProductRouteInvoked = true;
	Result.ProductRoute = RouteIntent(Result.Intent);
	Result.Status = Result.ProductRoute.IsAccepted()
		? Edemo_mapShanmenControlledWeaponInputStatus::Applied
		: Edemo_mapShanmenControlledWeaponInputStatus::ProductRejected;
	Result.Diagnostic = Result.ProductRoute.Diagnostic;
	return Result;
}
