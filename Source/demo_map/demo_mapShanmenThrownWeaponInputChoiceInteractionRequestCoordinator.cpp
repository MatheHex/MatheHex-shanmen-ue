#include "demo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus;

	FGuid MakeRequestId(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Request)
	{
		if (!Request.GetExpectedReadModelId().IsValid()
			|| !Request.GetIntent().IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT(
				"demo_map.ShanmenThrownWeapon.InputChoiceInteractionRequest.r1")),
			{
				Request.GetExpectedReadModelId().ToString(
					EGuidFormats::Digits),
				Request.GetIntent().GetIntentId().ToString(
					EGuidFormats::Digits)
			});
	}

	bool IntentResultMatchesRequest(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult& Result,
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Request)
	{
		return Result.IsValid()
			&& Result.GetIntent().Matches(Request.GetIntent());
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
TryCaptureTrajectorySelection(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
		TryEmitTrajectorySelection(ReadModel, TrajectoryKind, Intent))
	{
		OutRequest =
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
		return false;
	}
	return TryCapture(ReadModel, Intent, OutRequest);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
TryCaptureArcTargetIntent(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const FVector2D& RawTargetIntent,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
		TryEmitArcTargetIntent(ReadModel, RawTargetIntent, Intent))
	{
		OutRequest =
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
		return false;
	}
	return TryCapture(ReadModel, Intent, OutRequest);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
TryCaptureArcApexAdjustment(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const double RawNormalizedDelta,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
		TryEmitArcApexAdjustment(ReadModel, RawNormalizedDelta, Intent))
	{
		OutRequest =
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
		return false;
	}
	return TryCapture(ReadModel, Intent, OutRequest);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
TryCaptureArcTargetClear(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
		TryEmitArcTargetClear(ReadModel, Intent))
	{
		OutRequest =
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
		return false;
	}
	return TryCapture(ReadModel, Intent, OutRequest);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::TryCapture(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& ReadModel,
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& InIntent,
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest)
{
	OutRequest = Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
	if (!ReadModel.IsValid() || !InIntent.IsValid())
	{
		return false;
	}
	OutRequest.ExpectedReadModelId = ReadModel.GetReadModelId();
	OutRequest.Intent = InIntent;
	OutRequest.RequestId = MakeRequestId(OutRequest);
	return OutRequest.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::IsValid() const
{
	return RequestId.IsValid() && ExpectedReadModelId.IsValid()
		&& Intent.IsValid() && RequestId == MakeRequestId(*this);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::Matches(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& RequestId == Other.RequestId
		&& ExpectedReadModelId == Other.ExpectedReadModelId
		&& Intent.Matches(Other.Intent);
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult::IsValid()
	const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| InteractionReadCount < 0 || InteractionReadCount > 1
		|| IntentRouteCount < 0 || IntentRouteCount > 1)
	{
		return false;
	}
	if (Status == EStatus::RequestInvalid)
	{
		return !Request.IsValid() && InteractionReadCount == 0
			&& IntentRouteCount == 0
			&& !InteractionReadResult.IsValid()
			&& !IntentResult.IsValid();
	}
	if (!Request.IsValid() || InteractionReadCount != 1)
	{
		return false;
	}

	switch (Status)
	{
	case EStatus::InteractionReadRejected:
		return InteractionReadResult.IsValid()
			&& !InteractionReadResult.IsProjected()
			&& IntentRouteCount == 0 && !IntentResult.IsValid();
	case EStatus::InteractionReadProtocolRejected:
		return !InteractionReadResult.IsValid()
			&& IntentRouteCount == 0 && !IntentResult.IsValid();
	case EStatus::StaleReadModel:
		return InteractionReadResult.IsProjected()
			&& InteractionReadResult.GetReadModel().GetReadModelId()
				!= Request.GetExpectedReadModelId()
			&& IntentRouteCount == 0 && !IntentResult.IsValid();
	case EStatus::Routed:
		return InteractionReadResult.IsProjected()
			&& InteractionReadResult.GetReadModel().GetReadModelId()
				== Request.GetExpectedReadModelId()
			&& IntentRouteCount == 1
			&& IntentResultMatchesRequest(IntentResult, Request)
			&& Diagnostic == IntentResult.GetDiagnostic();
	case EStatus::IntentRouteProtocolRejected:
		return InteractionReadResult.IsProjected()
			&& InteractionReadResult.GetReadModel().GetReadModelId()
				== Request.GetExpectedReadModelId()
			&& IntentRouteCount == 1
			&& !IntentResultMatchesRequest(IntentResult, Request);
	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult::
IsAccepted() const
{
	return IsValid() && Status == EStatus::Routed
		&& IntentResult.IsAccepted();
}

bool Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult::
WasRejectedByIntentRoute() const
{
	return IsValid() && Status == EStatus::Routed
		&& !IntentResult.IsAccepted();
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::Execute(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Request,
	FReadCurrentInteraction ReadCurrentInteraction,
	FRouteIntent RouteIntent)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult Result;
	Result.Request = Request;
	if (!Request.IsValid())
	{
		Result.Status = EStatus::RequestInvalid;
		Result.Diagnostic =
			TEXT("Thrown-weapon choice interaction request is invalid.");
		return Result;
	}

	Result.InteractionReadCount = 1;
	Result.InteractionReadResult = ReadCurrentInteraction();
	if (!Result.InteractionReadResult.IsValid())
	{
		Result.Status = EStatus::InteractionReadProtocolRejected;
		Result.Diagnostic = TEXT(
			"Thrown-weapon choice interaction returned invalid read evidence.");
		return Result;
	}
	if (!Result.InteractionReadResult.IsProjected())
	{
		Result.Status = EStatus::InteractionReadRejected;
		Result.Diagnostic = Result.InteractionReadResult.GetDiagnostic();
		return Result;
	}
	if (Result.InteractionReadResult.GetReadModel().GetReadModelId()
		!= Request.GetExpectedReadModelId())
	{
		Result.Status = EStatus::StaleReadModel;
		Result.Diagnostic = TEXT(
			"Thrown-weapon choice interaction request targets a stale read model.");
		return Result;
	}

	Result.IntentRouteCount = 1;
	Result.IntentResult = RouteIntent(Request.GetIntent());
	if (!IntentResultMatchesRequest(Result.IntentResult, Request))
	{
		Result.Status = EStatus::IntentRouteProtocolRejected;
		Result.Diagnostic = TEXT(
			"Thrown-weapon choice interaction returned invalid intent-route evidence.");
		return Result;
	}
	Result.Status = EStatus::Routed;
	Result.Diagnostic = Result.IntentResult.GetDiagnostic();
	return Result;
}
