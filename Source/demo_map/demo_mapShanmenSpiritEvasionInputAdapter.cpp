#include "demo_mapShanmenSpiritEvasionInputAdapter.h"

bool Fdemo_mapShanmenSpiritEvasionInputResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSpiritEvasionInputStatus::Applied
		&& bDirectionSampled
		&& bProductRouteInvoked
		&& ProductRoute.IsAccepted();
}

Fdemo_mapShanmenSpiritEvasionInputResult
Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
	const bool bGameplayInputAllowed,
	const bool bProductRouteAvailable,
	TFunctionRef<FVector()> SampleDirection,
	TFunctionRef<Fdemo_mapShanmenSpiritEvasionProductRouteResult(
		const FVector&)> RouteStartIntent)
{
	Fdemo_mapShanmenSpiritEvasionInputResult Result;
	if (!bGameplayInputAllowed)
	{
		Result.Diagnostic =
			TEXT("Spirit Evasion input is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bProductRouteAvailable)
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionInputStatus::ProductRouteUnavailable;
		Result.Diagnostic =
			TEXT("Spirit Evasion input requires the authoritative GameMode product route.");
		return Result;
	}

	Result.bDirectionSampled = true;
	Result.SampledDirection = SampleDirection();
	Result.bProductRouteInvoked = true;
	Result.ProductRoute = RouteStartIntent(Result.SampledDirection);
	Result.Status = Result.ProductRoute.IsAccepted()
		? Edemo_mapShanmenSpiritEvasionInputStatus::Applied
		: Edemo_mapShanmenSpiritEvasionInputStatus::ProductRejected;
	Result.Diagnostic = Result.ProductRoute.Diagnostic;
	return Result;
}
