#include "demo_mapShanmenMeridianShockTreatmentProductLifecycle.h"

#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

bool Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle::TryBegin(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Udemo_mapShanmenCombatConditionComponent* ConditionComponent,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		OutDiagnostic = TEXT("Treatment lifecycle requires the ready item authority on the Game Thread.");
		return false;
	}
	if (Session.IsActive() && BoundAuthority.Get() != &Authority)
	{
		OutDiagnostic = TEXT("An active treatment lifecycle cannot switch item authority.");
		return false;
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority,
			Correlation,
			&OutDiagnostic))
	{
		return false;
	}
	if (!::IsValid(ConditionComponent)
		|| !ConditionComponent->IsValid()
		|| ConditionComponent->IsEmpty()
		|| ConditionComponent->GetRunId() != Correlation.ActiveRunId
		|| Authority.GetBoundOwnerId() != Correlation.OwnerId)
	{
		OutDiagnostic = TEXT("Treatment lifecycle requires matching item owner, active Run and condition authority.");
		return false;
	}
	const Fdemo_mapShanmenTreatmentRecoveryStorageContext RecoveryStorage =
		Fdemo_mapShanmenTreatmentRecoveryStorageContext::ForRoot(
			Authority.GetBoundStorageRoot(),
			Correlation.OwnerId,
			Correlation.ActiveRunId);
	if (!RecoveryStorage.IsValid()
		|| !Session.TryBegin(
			Correlation,
			ConditionComponent,
			RecoveryStorage,
			OutDiagnostic))
	{
		return false;
	}
	BoundAuthority = &Authority;
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Treatment lifecycle failed closed after product binding.");
		return false;
	}
	OutDiagnostic = TEXT("Treatment product lifecycle bound to durable Run and condition authority.");
	return true;
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle::TrySubmitHotbar(
	const FGuid& RequestId,
	const FGuid& ItemInstanceId,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	if (!IsValid() || !Session.IsActive() || !BoundAuthority.IsValid())
	{
		return RejectUnavailable(
			RequestId,
			TEXT("Treatment hotbar routing requires one valid product lifecycle."));
	}
	return Session.TrySubmitHotbar(
		*BoundAuthority.Get(), RequestId, ItemInstanceId, TimelineSample);
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle::
	TryRecoverPending(FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !Session.IsActive() || !BoundAuthority.IsValid())
	{
		OutDiagnostic = TEXT("Treatment recovery requires one valid product lifecycle.");
		return false;
	}
	return Session.TryRecoverPending(*BoundAuthority.Get(), OutDiagnostic);
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle::TryEnd(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Session.IsActive())
	{
		if (!IsValid())
		{
			OutDiagnostic = TEXT("Inactive treatment lifecycle retains inconsistent authority state.");
			return false;
		}
		OutDiagnostic = TEXT("Treatment product lifecycle is already empty.");
		return true;
	}
	if (!IsValid() || !BoundAuthority.IsValid())
	{
		OutDiagnostic = TEXT("Invalid treatment lifecycle cannot discard captured recovery state.");
		return false;
	}
	const FGuid ExpectedRunId = Session.GetRunId();
	if (!Session.TryEnd(
			*BoundAuthority.Get(), ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	BoundAuthority.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Treatment lifecycle failed empty-state validation after end.");
		return false;
	}
	OutDiagnostic = TEXT("Treatment lifecycle ended after deterministic recovery completed.");
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle::IsValid() const
{
	return Session.IsValid()
		&& (Session.IsActive() == BoundAuthority.IsValid());
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle::RejectUnavailable(
	const FGuid& RequestId,
	const TCHAR* Diagnostic) const
{
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
	Result.Error = Session.IsActive()
		? Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInvalid
		: Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInactive;
	Result.RequestId = RequestId;
	Result.Diagnostic = Diagnostic ? Diagnostic : TEXT("Treatment lifecycle rejected the request.");
	return Result;
}
