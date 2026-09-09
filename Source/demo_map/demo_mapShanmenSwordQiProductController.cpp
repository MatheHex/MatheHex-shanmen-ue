#include "demo_mapShanmenSwordQiProductController.h"

#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	Fdemo_mapShanmenSwordQiControllerResult Reject(
		Edemo_mapShanmenSwordQiControllerStatus Status,
		const Fdemo_mapShanmenSwordQiIntent& Intent,
		const FGuid& ControllerRunId,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordQiControllerResult Result;
		Result.Status = Status;
		Result.IntentId = Intent.GetIntentId();
		Result.RunId = ControllerRunId;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordQiIntent::TryCapture(
	const FGuid& RequestedIntentId,
	const FGuid& RequestedRunId,
	const FVector& RequestedOrigin,
	const FVector& RequestedAimDirection,
	Fdemo_mapShanmenSwordQiIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenSwordQiIntent();
	if (!RequestedIntentId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedAimDirection)
		|| RequestedAimDirection.IsNearlyZero())
	{
		return false;
	}
	OutIntent.IntentId = RequestedIntentId;
	OutIntent.RunId = RequestedRunId;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.AimDirection = RequestedAimDirection.GetSafeNormal();
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenSwordQiIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordQiIntent::IsValid() const
{
	return IntentId.IsValid()
		&& RunId.IsValid()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(AimDirection)
		&& AimDirection.IsNormalized();
}

bool Fdemo_mapShanmenSwordQiIntent::Matches(
	const Fdemo_mapShanmenSwordQiIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& IntentId == Other.IntentId
		&& RunId == Other.RunId
		&& Origin == Other.Origin
		&& AimDirection == Other.AimDirection;
}

bool Fdemo_mapShanmenSwordQiControllerResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenSwordQiControllerStatus::Applied
		&& IntentId.IsValid()
		&& RunId.IsValid()
		&& Item.IsAuthorized()
		&& FMath::IsFinite(AttackPower)
		&& AttackPower >= 0.0f
		&& Start.IsReady()
		&& Route.IsAccepted()
		&& Route.CommandId == Start.Command.GetCommandId()
		&& Route.RunId == RunId
		&& Route.SourceItemInstanceId
			== Item.Authorization.GetSourceItemInstanceId();
}

bool Fdemo_mapShanmenSwordQiControllerEndSummary::IsValid() const
{
	return RunId.IsValid()
		&& CapturedIntentCount >= 0
		&& ProcessedCommandCount >= 0
		&& ProcessedCommandCount <= CapturedIntentCount
		&& (!bInterruptedFlight || TerminalReceipt.IsValid());
}

bool Fdemo_mapShanmenSwordQiProductController::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsActive())
	{
		if (IsValid() && RunId == RequestedRunId)
		{
			OutDiagnostic =
				TEXT("Sword Qi controller is already bound to this exact Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("An active Sword Qi controller cannot change Run identity.");
		return false;
	}
	if (!RequestedRunId.IsValid() || !IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Sword Qi controller requires empty valid state and one Run identity.");
		return false;
	}
	RunId = RequestedRunId;
	if (!IsValid())
	{
		Reset();
		OutDiagnostic =
			TEXT("Sword Qi controller failed closed during Run binding.");
		return false;
	}
	OutDiagnostic = TEXT("Sword Qi controller bound to the active combat Run.");
	return true;
}

Fdemo_mapShanmenSwordQiControllerResult
Fdemo_mapShanmenSwordQiProductController::TrySubmit(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
	const Fdemo_mapItemAuthority& ItemAuthority,
	const Udemo_mapAttributeComponent& Attributes,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* SourceActor,
	const Fdemo_mapShanmenSwordQiIntent& Intent,
	TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()> AuthorizeAction)
{
	if (!IsActive())
	{
		return Reject(
			Edemo_mapShanmenSwordQiControllerStatus::ControllerInactive,
			Intent,
			RunId,
			TEXT("Sword Qi submission requires one active Run controller."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiControllerStatus::ControllerInvalid,
			Intent,
			RunId,
			TEXT("Sword Qi controller invariants are invalid."));
	}
	if (!Intent.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiControllerStatus::IntentInvalid,
			Intent,
			RunId,
			TEXT("Sword Qi requires a valid device-independent intent."));
	}
	if (Intent.GetRunId() != RunId
		|| !Coordinator.IsReady()
		|| Coordinator.GetRunId() != RunId)
	{
		return Reject(
			Edemo_mapShanmenSwordQiControllerStatus::RunMismatch,
			Intent,
			RunId,
			TEXT("Sword Qi intent, controller and coordinator must name one Run."));
	}

	if (FCapturedIntent* Existing = CapturedIntents.Find(Intent.GetIntentId()))
	{
		if (!Existing->Intent.Matches(Intent))
		{
			return Reject(
				Edemo_mapShanmenSwordQiControllerStatus::IntentIdConflict,
				Intent,
				RunId,
				TEXT("Sword Qi IntentId was reused with another payload."));
		}
		return RouteCaptured(
			World,
			ProjectileClass,
			Coordinator,
			SourceActor,
			*Existing,
			true,
			AuthorizeAction);
	}

	FCapturedIntent Captured;
	Captured.Intent = Intent;
	Captured.Item =
		Fdemo_mapShanmenSwordQiItemAdapter::AuthorizeEquippedSword(
			ItemAuthority);
	if (!Captured.Item.IsAuthorized())
	{
		Fdemo_mapShanmenSwordQiControllerResult Result = Reject(
			Edemo_mapShanmenSwordQiControllerStatus::
				ItemAuthorizationRejected,
			Intent,
			RunId,
			*Captured.Item.Diagnostic);
		Result.Item = Captured.Item;
		return Result;
	}
	if (!Attributes.GetFinalValue(
			Fdemo_mapAttributeIds::AttackPower,
			Captured.AttackPower)
		|| !FMath::IsFinite(Captured.AttackPower)
		|| Captured.AttackPower < 0.0f)
	{
		Fdemo_mapShanmenSwordQiControllerResult Result = Reject(
			Edemo_mapShanmenSwordQiControllerStatus::
				AttackPowerUnavailable,
			Intent,
			RunId,
			TEXT("Sword Qi could not sample one finite non-negative final AttackPower."));
		Result.Item = Captured.Item;
		return Result;
	}

	Captured.Start = Fdemo_mapShanmenSwordQiProductAuthority::PrepareLaunch(
		Coordinator,
		Captured.Item.Authorization.GetSourceItemInstanceId(),
		Captured.AttackPower,
		Intent.GetOrigin(),
		Intent.GetAimDirection());
	if (!Captured.Start.IsReady())
	{
		Fdemo_mapShanmenSwordQiControllerResult Result = Reject(
			Edemo_mapShanmenSwordQiControllerStatus::ProductCaptureRejected,
			Intent,
			RunId,
			*Captured.Start.Diagnostic);
		Result.Item = Captured.Item;
		Result.AttackPower = Captured.AttackPower;
		Result.Start = Captured.Start;
		return Result;
	}

	// Keep the new intent private until its synchronous route has produced a
	// receipt. Action authorization may project this controller's occupancy;
	// publishing a half-routed intent would make that projection fail closed.
	Fdemo_mapShanmenSwordQiControllerResult Result = RouteCaptured(
		World,
		ProjectileClass,
		Coordinator,
		SourceActor,
		Captured,
		false,
		AuthorizeAction);
	CapturedIntents.Add(Intent.GetIntentId(), MoveTemp(Captured));
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiControllerStatus::ControllerInvalid;
		Result.Diagnostic =
			TEXT("Sword Qi controller failed committed-intent invariants.");
	}
	return Result;
}

Fdemo_mapShanmenSwordQiControllerResult
Fdemo_mapShanmenSwordQiProductController::RouteCaptured(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* SourceActor,
	FCapturedIntent& Captured,
	bool bReusedIntent,
	TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()> AuthorizeAction)
{
	Captured.LastRoute = Session.TryRoute(
		World,
		ProjectileClass,
		Coordinator,
		SourceActor,
		Captured.Start.Command,
		AuthorizeAction);

	Fdemo_mapShanmenSwordQiControllerResult Result;
	Result.Status = Captured.LastRoute.IsAccepted()
		? Edemo_mapShanmenSwordQiControllerStatus::Applied
		: Edemo_mapShanmenSwordQiControllerStatus::RouteRejected;
	Result.bReusedIntent = bReusedIntent;
	Result.IntentId = Captured.Intent.GetIntentId();
	Result.RunId = RunId;
	Result.Item = Captured.Item;
	Result.AttackPower = Captured.AttackPower;
	Result.Start = Captured.Start;
	Result.Route = Captured.LastRoute;
	Result.Diagnostic = Captured.LastRoute.Diagnostic;
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordQiControllerStatus::ControllerInvalid;
		Result.Diagnostic =
			TEXT("Sword Qi controller failed post-route invariants.");
	}
	return Result;
}

bool Fdemo_mapShanmenSwordQiProductController::TryInterrupt()
{
	return IsActive() && IsValid() && Session.TryInterrupt();
}

bool Fdemo_mapShanmenSwordQiProductController::TryExpireRange()
{
	return IsActive() && IsValid() && Session.TryExpireRange();
}

bool Fdemo_mapShanmenSwordQiProductController::TryRetireTerminal(
	Fdemo_mapShanmenSwordQiTerminalReceipt& OutReceipt)
{
	return IsActive()
		&& IsValid()
		&& Session.TryRetireTerminal(OutReceipt);
}

bool Fdemo_mapShanmenSwordQiProductController::TryAppendOccupancy(
	Fdemo_mapShanmenPlayerActionOccupancySnapshot& InOutSnapshot) const
{
	if (!IsValid())
	{
		InOutSnapshot.Invalidate();
		return false;
	}
	return Session.TryAppendOccupancy(InOutSnapshot);
}

bool Fdemo_mapShanmenSwordQiProductController::TryEnd(
	const FGuid& ExpectedRunId,
	Fdemo_mapShanmenSwordQiControllerEndSummary& OutSummary,
	FString& OutDiagnostic)
{
	OutSummary = Fdemo_mapShanmenSwordQiControllerEndSummary();
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		if (IsValid() && IsEmpty())
		{
			OutDiagnostic = TEXT("Sword Qi controller is already empty.");
			return true;
		}
		OutDiagnostic = TEXT("Inactive Sword Qi controller state is invalid.");
		return false;
	}
	if (!IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic =
			TEXT("Sword Qi controller end requires its exact active Run identity.");
		return false;
	}

	OutSummary.RunId = RunId;
	OutSummary.CapturedIntentCount = CapturedIntents.Num();
	OutSummary.ProcessedCommandCount = Session.NumProcessedCommands();
	if (Session.IsInFlight())
	{
		if (!Session.TryInterrupt())
		{
			OutDiagnostic =
				TEXT("Sword Qi flight could not be interrupted during Run teardown.");
			return false;
		}
		OutSummary.bInterruptedFlight = true;
	}
	if (Session.IsTerminal()
		&& !Session.TryRetireTerminal(OutSummary.TerminalReceipt))
	{
		OutDiagnostic =
			TEXT("Sword Qi terminal proof could not be retired during Run teardown.");
		return false;
	}
	if (!Session.Reset())
	{
		OutDiagnostic =
			TEXT("Sword Qi product Session could not reset during Run teardown.");
		return false;
	}
	CapturedIntents.Reset();
	RunId.Invalidate();
	if (!OutSummary.IsValid() || !IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Sword Qi controller failed empty-state validation after teardown.");
		return false;
	}
	OutDiagnostic =
		TEXT("Sword Qi controller ended without hidden flight or command work.");
	return true;
}

void Fdemo_mapShanmenSwordQiProductController::Reset()
{
	if (Session.IsInFlight())
	{
		Session.TryInterrupt();
	}
	if (Session.IsTerminal())
	{
		Fdemo_mapShanmenSwordQiTerminalReceipt Ignored;
		Session.TryRetireTerminal(Ignored);
	}
	Session.Reset();
	CapturedIntents.Reset();
	RunId.Invalidate();
}

bool Fdemo_mapShanmenSwordQiProductController::IsValid() const
{
	if (!Session.IsValid())
	{
		return false;
	}
	if (!RunId.IsValid())
	{
		return CapturedIntents.IsEmpty()
			&& Session.IsEmpty()
			&& !Session.GetRunId().IsValid();
	}
	if (Session.GetRunId().IsValid() && Session.GetRunId() != RunId)
	{
		return false;
	}
	for (const TPair<FGuid, FCapturedIntent>& Pair : CapturedIntents)
	{
		const FCapturedIntent& Captured = Pair.Value;
		if (!Pair.Key.IsValid()
			|| Pair.Key != Captured.Intent.GetIntentId()
			|| !Captured.Intent.IsValid()
			|| Captured.Intent.GetRunId() != RunId
			|| !Captured.Item.IsAuthorized()
			|| !FMath::IsFinite(Captured.AttackPower)
			|| Captured.AttackPower < 0.0f
			|| !Captured.Start.IsReady()
			|| Captured.Start.Command.GetRunId() != RunId
			|| Captured.Start.Command.GetAction().GetSourceItemInstanceId()
				!= Captured.Item.Authorization.GetSourceItemInstanceId()
			|| Captured.Start.Command.GetOffense().GetAttackPower()
				!= Captured.AttackPower
			|| Captured.Start.Command.GetOrigin()
				!= Captured.Intent.GetOrigin()
			|| Captured.Start.Command.GetAimDirection()
				!= Captured.Intent.GetAimDirection()
			|| Captured.LastRoute.CommandId
				!= Captured.Start.Command.GetCommandId()
			|| Captured.LastRoute.RunId != RunId)
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenSwordQiProductController::IsEmpty() const
{
	return !RunId.IsValid()
		&& CapturedIntents.IsEmpty()
		&& Session.IsEmpty();
}

const Fdemo_mapShanmenSwordQiLaunchCommand*
Fdemo_mapShanmenSwordQiProductController::FindCapturedCommand(
	const FGuid& IntentId) const
{
	const FCapturedIntent* Captured = CapturedIntents.Find(IntentId);
	return Captured && IsValid() ? &Captured->Start.Command : nullptr;
}
