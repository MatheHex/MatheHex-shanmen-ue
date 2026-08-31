#include "demo_mapShanmenWeaponGuardInputAdapter.h"

bool Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
	const FGuid& RequestedTimelineId,
	int64 RequestedActiveStartTick,
	Fdemo_mapShanmenWeaponGuardInputTimelineSample& OutSample)
{
	OutSample = Fdemo_mapShanmenWeaponGuardInputTimelineSample();
	const int64 WindowTicks =
		Fdemo_mapShanmenWeaponGuardProductConfig::
			CanonicalPerfectWindowTickCount();
	if (!RequestedTimelineId.IsValid()
		|| RequestedActiveStartTick < 0
		|| WindowTicks <= 0
		|| RequestedActiveStartTick > MAX_int64 - WindowTicks)
	{
		return false;
	}

	OutSample.TimelineId = RequestedTimelineId;
	OutSample.ActiveStartTick = RequestedActiveStartTick;
	return OutSample.IsValid();
}

bool Fdemo_mapShanmenWeaponGuardInputTimelineSample::IsValid() const
{
	const int64 WindowTicks =
		Fdemo_mapShanmenWeaponGuardProductConfig::
			CanonicalPerfectWindowTickCount();
	return TimelineId.IsValid()
		&& ActiveStartTick >= 0
		&& WindowTicks > 0
		&& ActiveStartTick <= MAX_int64 - WindowTicks;
}

bool Fdemo_mapShanmenWeaponGuardInputResult::IsAccepted() const
{
	if (Status != Edemo_mapShanmenWeaponGuardInputStatus::Applied
		|| !bTimelineSampled
		|| !bProductRouteInvoked
		|| !TimelineSample.IsValid()
		|| !ProductRoute.IsReady())
	{
		return false;
	}

	const FShanmenWeaponPerfectGuardPolicy& Timing =
		ProductRoute.ProductStart.Host.GetTimingPolicy();
	return Timing.GetTimelineId() == TimelineSample.GetTimelineId()
		&& Timing.GetActiveStartTick()
			== TimelineSample.GetActiveStartTick();
}

Fdemo_mapShanmenWeaponGuardInputResult
Fdemo_mapShanmenWeaponGuardInputAdapter::RouteStartInput(
	const bool bGameplayInputAllowed,
	const bool bProductRouteAvailable,
	TFunctionRef<Fdemo_mapShanmenWeaponGuardInputTimelineSample()>
		SampleTimeline,
	TFunctionRef<Fdemo_mapShanmenWeaponGuardProductRouteResult(
		const FGuid& TimelineId,
		int64 ActiveStartTick)> RouteStartIntent)
{
	Fdemo_mapShanmenWeaponGuardInputResult Result;
	if (!bGameplayInputAllowed)
	{
		Result.Diagnostic =
			TEXT("Weapon guard input is blocked by the current gameplay surface.");
		return Result;
	}
	if (!bProductRouteAvailable)
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardInputStatus::ProductRouteUnavailable;
		Result.Diagnostic =
			TEXT("Weapon guard input requires the authoritative product route.");
		return Result;
	}

	Result.bTimelineSampled = true;
	Result.TimelineSample = SampleTimeline();
	if (!Result.TimelineSample.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardInputStatus::TimelineRejected;
		Result.Diagnostic =
			TEXT("Weapon guard input received an invalid monotonic timeline sample.");
		return Result;
	}

	Result.bProductRouteInvoked = true;
	Result.ProductRoute = RouteStartIntent(
		Result.TimelineSample.GetTimelineId(),
		Result.TimelineSample.GetActiveStartTick());
	Result.Status = Result.ProductRoute.IsReady()
		? Edemo_mapShanmenWeaponGuardInputStatus::Applied
		: Edemo_mapShanmenWeaponGuardInputStatus::ProductRejected;
	Result.Diagnostic = Result.ProductRoute.Diagnostic;
	return Result;
}
