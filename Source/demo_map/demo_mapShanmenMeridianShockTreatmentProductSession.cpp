#include "demo_mapShanmenMeridianShockTreatmentProductSession.h"

#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

bool Fdemo_mapShanmenMeridianShockTreatmentProductSession::TryBegin(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	Udemo_mapShanmenCombatConditionComponent* ConditionComponent,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsInGameThread() || !IsValid())
	{
		OutDiagnostic = TEXT("Treatment session begin requires valid empty Game-Thread state.");
		return false;
	}
	if (!IsEmpty())
	{
		return Route.TryBegin(
			Correlation, ConditionComponent, OutDiagnostic);
	}
	if (!Route.TryBegin(Correlation, ConditionComponent, OutDiagnostic))
	{
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Treatment session failed closed after route binding.");
		return false;
	}
	OutDiagnostic = TEXT("Meridian Shock treatment session bound to the active Run.");
	return true;
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductSession::TrySubmitHotbar(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FGuid& RequestId,
	const FGuid& ItemInstanceId,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	if (!IsInGameThread() || !IsValid() || IsEmpty())
	{
		return Reject(
			Route.IsEmpty()
				? Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInactive
				: Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInvalid,
			RequestId,
			TEXT("Treatment hotbar request requires one valid active session."));
	}
	if (!RequestId.IsValid() || !ItemInstanceId.IsValid()
		|| !TimelineSample.IsValid())
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RequestInvalid,
			RequestId,
			TEXT("Treatment hotbar request identity, item or timeline is invalid."));
	}
	FString RecoveryDiagnostic;
	if (!TryRecoverPending(Authority, RecoveryDiagnostic))
	{
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result = Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::CommitRejected,
			RequestId,
			*RecoveryDiagnostic);
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentRouteStatus::RecoveryRequired;
		return Result;
	}

	if (FCapturedRequest* Existing = Requests.Find(RequestId))
	{
		if (Existing->Command.GetItemInstanceId() != ItemInstanceId
			|| Existing->Command.GetTimelineSample().GetSampleId()
				!= TimelineSample.GetSampleId())
		{
			return Reject(
				Edemo_mapShanmenMeridianShockTreatmentRouteError::RequestIdConflict,
				RequestId,
				TEXT("Treatment RequestId was reused with another hotbar payload."));
		}
		Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result =
			Route.TryExecute(Authority, Existing->Command);
		Existing->bRequiresRecovery = Result.RequiresRecovery();
		return Result;
	}

	Fdemo_mapShanmenMeridianShockTreatmentCommand Command;
	FString CaptureDiagnostic;
	if (!Route.TryCaptureCommand(
			RequestId,
			ItemInstanceId,
			TimelineSample,
			Command,
			CaptureDiagnostic))
	{
		return Reject(
			Edemo_mapShanmenMeridianShockTreatmentRouteError::RequestInvalid,
			RequestId,
			*CaptureDiagnostic);
	}
	FCapturedRequest& Stored = Requests.Add(RequestId);
	Stored.Command = Command;
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result =
		Route.TryExecute(Authority, Stored.Command);
	Stored.bRequiresRecovery = Result.RequiresRecovery();
	return Result;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductSession::TryRecoverPending(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	FString& OutDiagnostic)
{
	return TryRecoverPending(
		Authority,
		TConstArrayView<
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof>(),
		OutDiagnostic);
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductSession::TryRecoverPending(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const TConstArrayView<
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof> Proofs,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsInGameThread() || !IsValid() || IsEmpty())
	{
		OutDiagnostic = TEXT("Treatment recovery requires one valid active session.");
		return false;
	}
	int32 DurableRecoveredCount = 0;
	if (!Route.TryRecoverDurablePreparation(
			Authority,
			Proofs,
			DurableRecoveredCount,
			OutDiagnostic))
	{
		return false;
	}

	TArray<FGuid> PendingIds;
	for (const TPair<FGuid, FCapturedRequest>& Pair : Requests)
	{
		if (Pair.Value.bRequiresRecovery)
		{
			PendingIds.Add(Pair.Key);
		}
	}
	PendingIds.Sort([](const FGuid& A, const FGuid& B)
	{
		return A.ToString(EGuidFormats::Digits)
			< B.ToString(EGuidFormats::Digits);
	});
	for (const FGuid& RequestId : PendingIds)
	{
		FCapturedRequest* Pending = Requests.Find(RequestId);
		if (!Pending)
		{
			OutDiagnostic = TEXT("Treatment recovery journal changed during deterministic traversal.");
			return false;
		}
		const Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result =
			Route.TryExecute(Authority, Pending->Command);
		Pending->bRequiresRecovery = Result.RequiresRecovery();
		if (Pending->bRequiresRecovery)
		{
			OutDiagnostic = Result.Diagnostic;
			return false;
		}
	}
	if (Route.HasUnresolvedRecovery() || NumPendingRecovery() != 0)
	{
		OutDiagnostic = TEXT("Treatment route still owns unresolved commit work.");
		return false;
	}
	if (DurableRecoveredCount > 0)
	{
		OutDiagnostic = PendingIds.IsEmpty()
			? TEXT("Treatment session reconstructed and resolved one durable ledger transaction.")
			: TEXT("Treatment session resolved durable and runtime recovery in stable order.");
	}
	else
	{
		OutDiagnostic = PendingIds.IsEmpty()
			? TEXT("Treatment session has no pending recovery.")
			: TEXT("Treatment session recovered all pending commits in stable order.");
	}
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductSession::TryEnd(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Treatment session is already empty.");
		return IsValid();
	}
	if (!IsValid() || ExpectedRunId != GetRunId())
	{
		OutDiagnostic = TEXT("Treatment session end requires its exact valid active Run.");
		return false;
	}
	if (!TryRecoverPending(Authority, OutDiagnostic)
		|| !Route.TryEnd(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	Requests.Reset();
	if (!IsValid() || !IsEmpty())
	{
		OutDiagnostic = TEXT("Treatment session failed empty-state validation after end.");
		return false;
	}
	OutDiagnostic = TEXT("Treatment session ended after resolving all runtime recovery work.");
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentProductSession::IsValid() const
{
	if (!Route.IsValid())
	{
		return false;
	}
	if (Route.IsEmpty())
	{
		return Requests.IsEmpty();
	}
	bool bAnyPending = false;
	for (const TPair<FGuid, FCapturedRequest>& Pair : Requests)
	{
		if (Pair.Key != Pair.Value.Command.GetRequestId()
			|| !Pair.Value.Command.IsValid()
			|| Pair.Value.Command.GetCorrelation().ActiveRunId
				!= Route.GetRunId())
		{
			return false;
		}
		bAnyPending |= Pair.Value.bRequiresRecovery;
	}
	return bAnyPending == Route.HasUnresolvedRecovery();
}

int32 Fdemo_mapShanmenMeridianShockTreatmentProductSession::
	NumPendingRecovery() const
{
	int32 Count = 0;
	for (const TPair<FGuid, FCapturedRequest>& Pair : Requests)
	{
		Count += Pair.Value.bRequiresRecovery ? 1 : 0;
	}
	return Count;
}

Fdemo_mapShanmenMeridianShockTreatmentRouteResult
Fdemo_mapShanmenMeridianShockTreatmentProductSession::Reject(
	const Edemo_mapShanmenMeridianShockTreatmentRouteError Error,
	const FGuid& RequestId,
	const TCHAR* Diagnostic) const
{
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Result;
	Result.Error = Error;
	Result.RequestId = RequestId;
	Result.Diagnostic = Diagnostic ? Diagnostic : TEXT("Treatment session rejected the request.");
	return Result;
}
