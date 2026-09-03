#include "demo_mapShanmenSwordQiInputAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}
}

bool Fdemo_mapShanmenSwordQiInputSample::TryCapture(
	const FVector& RequestedOrigin,
	const FVector& RequestedAimDirection,
	Fdemo_mapShanmenSwordQiInputSample& OutSample)
{
	OutSample = Fdemo_mapShanmenSwordQiInputSample();
	if (!IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedAimDirection)
		|| RequestedAimDirection.IsNearlyZero())
	{
		return false;
	}
	OutSample.Origin = RequestedOrigin;
	OutSample.AimDirection = RequestedAimDirection.GetSafeNormal();
	if (!OutSample.IsValid())
	{
		OutSample = Fdemo_mapShanmenSwordQiInputSample();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordQiInputSample::IsValid() const
{
	return IsFiniteVector(Origin)
		&& IsFiniteVector(AimDirection)
		&& AimDirection.IsNormalized();
}

bool Fdemo_mapShanmenSwordQiInputResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSwordQiInputStatus::Applied
		&& bSpatialSampled
		&& bProductRouteInvoked
		&& InputEventId.IsValid()
		&& RunId.IsValid()
		&& IntentId
			== Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
				RunId,
				InputEventId)
		&& Sample.IsValid()
		&& Intent.IsValid()
		&& Intent.GetIntentId() == IntentId
		&& Intent.GetRunId() == RunId
		&& Intent.GetOrigin() == Sample.GetOrigin()
		&& Intent.GetAimDirection().Equals(Sample.GetAimDirection())
		&& Product.IsAccepted()
		&& Product.IntentId == IntentId
		&& Product.RunId == RunId;
}

FGuid Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
	const FGuid& RunId,
	const FGuid& InputEventId)
{
	if (!RunId.IsValid() || !InputEventId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.SwordQi.InputIntent.r1")),
		{
			GuidDigits(RunId),
			GuidDigits(InputEventId)
		});
}

Fdemo_mapShanmenSwordQiInputResult
Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
	const bool bGameplayInputAllowed,
	const bool bProductRouteAvailable,
	const FGuid& RunId,
	const FGuid& InputEventId,
	TFunctionRef<Fdemo_mapShanmenSwordQiInputSample()> SampleSpatialInput,
	TFunctionRef<Fdemo_mapShanmenSwordQiControllerResult(
		const Fdemo_mapShanmenSwordQiIntent&)> RouteStartIntent)
{
	Fdemo_mapShanmenSwordQiInputResult Result;
	Result.InputEventId = InputEventId;
	Result.RunId = RunId;
	if (!bGameplayInputAllowed)
	{
		Result.Diagnostic =
			TEXT("Sword Qi input is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bProductRouteAvailable)
	{
		Result.Status =
			Edemo_mapShanmenSwordQiInputStatus::ProductRouteUnavailable;
		Result.Diagnostic =
			TEXT("Sword Qi input requires the authoritative GameMode product route.");
		return Result;
	}
	if (!RunId.IsValid())
	{
		Result.Status = Edemo_mapShanmenSwordQiInputStatus::RunUnavailable;
		Result.Diagnostic =
			TEXT("Sword Qi input requires one valid active Run identity.");
		return Result;
	}
	if (!InputEventId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiInputStatus::EventIdentityInvalid;
		Result.Diagnostic =
			TEXT("Sword Qi input requires one stable caller-owned event identity.");
		return Result;
	}

	Result.bSpatialSampled = true;
	Result.Sample = SampleSpatialInput();
	if (!Result.Sample.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiInputStatus::SpatialSampleRejected;
		Result.Diagnostic =
			TEXT("Sword Qi input received an invalid origin or aim sample.");
		return Result;
	}
	Result.IntentId = MakeIntentId(RunId, InputEventId);
	if (!Result.IntentId.IsValid()
		|| !Fdemo_mapShanmenSwordQiIntent::TryCapture(
			Result.IntentId,
			RunId,
			Result.Sample.GetOrigin(),
			Result.Sample.GetAimDirection(),
			Result.Intent))
	{
		Result.Status =
			Edemo_mapShanmenSwordQiInputStatus::IntentCaptureRejected;
		Result.Diagnostic =
			TEXT("Sword Qi input could not enter the immutable intent contract.");
		return Result;
	}

	Result.bProductRouteInvoked = true;
	Result.Product = RouteStartIntent(Result.Intent);
	Result.Status = Result.Product.IsAccepted()
		? Edemo_mapShanmenSwordQiInputStatus::Applied
		: Edemo_mapShanmenSwordQiInputStatus::ProductRejected;
	Result.Diagnostic = Result.Product.Diagnostic;
	return Result;
}
