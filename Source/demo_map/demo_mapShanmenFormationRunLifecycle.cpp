#include "demo_mapShanmenFormationRunLifecycle.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	using EScatterRunEvent =
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent;
	using EScatterRouteStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationRunRouteStatus;

	Fdemo_mapShanmenFormationControllerResult RejectSubmission(
		const Edemo_mapShanmenFormationControllerStatus Status,
		const FGuid& RunId,
		const Fdemo_mapShanmenFormationIntent& Intent,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationControllerResult Result;
		Result.Status = Status;
		Result.IntentId = Intent.GetIntentId();
		Result.RunId = RunId;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	Fdemo_mapShanmenFormationAnchorOperationResult RejectAnchorOperation(
		const Edemo_mapShanmenFormationAnchorOperationStatus Status,
		const Fdemo_mapShanmenFormationAnchorOperation& Operation,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationAnchorOperationResult Result;
		Result.Status = Status;
		Result.Operation = Operation;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
	RejectScatterPublication(
		const EScatterRouteStatus Status,
		const FGuid& RunId,
		const Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute*
			Route,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult Result;
		Result.Status = Status;
		Result.RunId = RunId;
		Result.RouteId = Route ? Route->GetRouteId() : FGuid();
		Result.Event = EScatterRunEvent::Publish;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}
}

bool Fdemo_mapShanmenFormationRunLifecycle::TryTakeover(
	Fdemo_mapShanmenFormationRunLifecycle& Previous,
	Fdemo_mapShanmenFormationRunLifecycle& OutLifecycle)
{
	if (&Previous == &OutLifecycle || !Previous.IsValid()
		|| !Previous.IsActive() || !OutLifecycle.IsEmpty())
	{
		return false;
	}

	Fdemo_mapShanmenFormationRunLifecycle Candidate;
	if (Previous.ScatterPublicationRoute.IsSet())
	{
		Candidate.ScatterPublicationRoute.Emplace();
		if (!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::
			TryTakeover(
				Previous.ScatterPublicationRoute.GetValue(),
				Candidate.ScatterPublicationRoute.GetValue()))
		{
			return false;
		}
		Previous.ScatterPublicationRoute.Reset();
	}

	Candidate.RunId = Previous.RunId;
	Candidate.Controller = MoveTemp(Previous.Controller);
	Candidate.ScatterPublicationTeardownCheckpoint =
		MoveTemp(Previous.ScatterPublicationTeardownCheckpoint);
	Candidate.ProductTeardownCheckpoint =
		MoveTemp(Previous.ProductTeardownCheckpoint);
	if (!Candidate.IsValid())
	{
		Previous.RunId = Candidate.RunId;
		Previous.Controller = MoveTemp(Candidate.Controller);
		Previous.ScatterPublicationTeardownCheckpoint =
			MoveTemp(Candidate.ScatterPublicationTeardownCheckpoint);
		Previous.ProductTeardownCheckpoint =
			MoveTemp(Candidate.ProductTeardownCheckpoint);
		if (Candidate.ScatterPublicationRoute.IsSet())
		{
			Previous.ScatterPublicationRoute.Emplace();
			Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::
				TryTakeover(
					Candidate.ScatterPublicationRoute.GetValue(),
					Previous.ScatterPublicationRoute.GetValue());
		}
		return false;
	}

	Previous.Clear();
	OutLifecycle = MoveTemp(Candidate);
	return Previous.IsEmpty() && OutLifecycle.IsValid();
}

bool Fdemo_mapShanmenFormationRunLifecycle::TryBegin(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsActive())
	{
		if (IsValid() && Coordinator.IsActive()
			&& Coordinator.GetRunId() == RunId)
		{
			OutDiagnostic = ProductTeardownCheckpoint.IsSet()
				? TEXT("Formation lifecycle is waiting to retry the exact Combat Run release.")
				: ScatterPublicationTeardownCheckpoint.IsSet()
					? TEXT("Formation lifecycle is waiting to retry product teardown after scatter publication ended.")
				: TEXT("Formation lifecycle is already bound to this exact Combat Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("An active formation lifecycle cannot change Combat Run identity.");
		return false;
	}
	if (!IsValid() || !IsEmpty() || !Coordinator.IsReady())
	{
		OutDiagnostic =
			TEXT("Formation lifecycle requires empty valid state and one ready Combat Run.");
		return false;
	}

	RunId = Coordinator.GetRunId();
	if (!Controller.TryBegin(RunId, OutDiagnostic) || !IsValid())
	{
		Clear();
		if (OutDiagnostic.IsEmpty())
		{
			OutDiagnostic =
				TEXT("Formation lifecycle failed closed during controller binding.");
		}
		return false;
	}
	OutDiagnostic =
		TEXT("Formation lifecycle bound its sole controller to the ready Combat Run.");
	return true;
}

bool Fdemo_mapShanmenFormationRunLifecycle::
TryOpenScatterPublicationRoute(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	const TSubclassOf<AActor> ActorClass,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic =
			TEXT("Scatter publication route requires one active formation lifecycle.");
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic =
			TEXT("Formation lifecycle invariants are invalid before route binding.");
		return false;
	}
	if (ScatterPublicationTeardownCheckpoint.IsSet()
		|| ProductTeardownCheckpoint.IsSet())
	{
		OutDiagnostic =
			TEXT("A teardown checkpoint forbids opening or replacing publication routes.");
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Candidate;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
			RunId, HandoffEvidence, ActorClass, Candidate))
	{
		OutDiagnostic =
			TEXT("Scatter publication route rejected the committed Handoff or Actor class.");
		return false;
	}
	if (ScatterPublicationRoute.IsSet())
	{
		if (ScatterPublicationRoute->IsValid()
			&& ScatterPublicationRoute->GetRunId() == RunId
			&& ScatterPublicationRoute->GetRouteId()
				== Candidate.GetRouteId())
		{
			OutDiagnostic =
				TEXT("The exact scatter publication route is already bound.");
			return true;
		}
		OutDiagnostic =
			TEXT("The sole scatter publication route slot cannot be rebound.");
		return false;
	}

	ScatterPublicationRoute = MoveTemp(Candidate);
	if (!IsValid())
	{
		ScatterPublicationRoute.Reset();
		OutDiagnostic =
			TEXT("Formation lifecycle failed closed after publication route binding.");
		return false;
	}
	OutDiagnostic =
		TEXT("Formation lifecycle bound its sole scatter publication route.");
	return true;
}

Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
Fdemo_mapShanmenFormationRunLifecycle::TryPublishScatterPublication(
	UWorld* World)
{
	const auto* Route = ScatterPublicationRoute.IsSet()
		? &ScatterPublicationRoute.GetValue()
		: nullptr;
	if (!IsActive())
	{
		return RejectScatterPublication(
			EScatterRouteStatus::RouteInvalid, RunId, Route,
			TEXT("Scatter publication requires one active formation lifecycle."));
	}
	if (!IsValid())
	{
		return RejectScatterPublication(
			EScatterRouteStatus::StateInvalid, RunId, Route,
			TEXT("Formation lifecycle invariants are invalid before publication."));
	}
	if (ScatterPublicationTeardownCheckpoint.IsSet()
		|| ProductTeardownCheckpoint.IsSet())
	{
		return RejectScatterPublication(
			EScatterRouteStatus::RouteInvalid, RunId, Route,
			TEXT("Scatter publication is closed after teardown begins."));
	}
	if (!Route)
	{
		return RejectScatterPublication(
			EScatterRouteStatus::RouteInvalid, RunId, nullptr,
			TEXT("Scatter publication requires the sole route slot."));
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult Result =
		ScatterPublicationRoute->TryPublish(RunId, World);
	if (!Result.IsValid() || !IsValid())
	{
		Result.Status = EScatterRouteStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Formation lifecycle detected inconsistent publication evidence.");
	}
	return Result;
}

Fdemo_mapShanmenFormationControllerResult
Fdemo_mapShanmenFormationRunLifecycle::TrySubmit(
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseProductController& SpiritEnergyController,
	const Fdemo_mapShanmenFormationIntent& Intent)
{
	if (!IsActive())
	{
		return RejectSubmission(
			Edemo_mapShanmenFormationControllerStatus::ControllerInactive,
			RunId,
			Intent,
			TEXT("Formation submission requires one active Run lifecycle."));
	}
	if (!IsValid())
	{
		return RejectSubmission(
			Edemo_mapShanmenFormationControllerStatus::ControllerInvalid,
			RunId,
			Intent,
			TEXT("Formation Run lifecycle invariants are invalid."));
	}
	if (ScatterPublicationTeardownCheckpoint.IsSet()
		|| ProductTeardownCheckpoint.IsSet())
	{
		return RejectSubmission(
			Edemo_mapShanmenFormationControllerStatus::ControllerInactive,
			RunId,
			Intent,
			TEXT("Formation teardown has begun; only the exact remaining teardown and Combat Run release may retry."));
	}
	if (!Coordinator.IsReady() || Coordinator.GetRunId() != RunId)
	{
		return RejectSubmission(
			Edemo_mapShanmenFormationControllerStatus::RunMismatch,
			RunId,
			Intent,
			TEXT("Formation lifecycle and Coordinator must name one ready Combat Run."));
	}

	Fdemo_mapShanmenFormationControllerResult Result =
		Controller.TrySubmit(
			Authority, Coordinator, SpiritEnergyController, Intent);
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationControllerStatus::ControllerInvalid;
		Result.Diagnostic =
			TEXT("Formation lifecycle failed invariants after controller submission.");
	}
	return Result;
}

Fdemo_mapShanmenFormationAnchorOperationResult
Fdemo_mapShanmenFormationRunLifecycle::TryExecuteAnchorOperation(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	const TSubclassOf<AActor> ActorClass,
	const Fdemo_mapShanmenFormationAnchorOperation& Operation)
{
	if (!IsActive())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::LifecycleInactive,
			Operation,
			TEXT("Anchor operation requires one active formation lifecycle."));
	}
	if (!IsValid())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::LifecycleInvalid,
			Operation,
			TEXT("Formation Run lifecycle invariants are invalid."));
	}
	if (ScatterPublicationTeardownCheckpoint.IsSet()
		|| ProductTeardownCheckpoint.IsSet())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::TeardownPending,
			Operation,
			TEXT("Formation teardown has begun; only the exact remaining teardown and Combat Run release may retry."));
	}
	if (!Operation.IsValid())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::OperationInvalid,
			Operation,
			TEXT("Anchor operation requires immutable Run, anchor and attempt identities."));
	}
	if (Operation.GetRunId() != RunId || Coordinator.GetRunId() != RunId)
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::RunMismatch,
			Operation,
			TEXT("Anchor operation, lifecycle and Coordinator must name one Run."));
	}
	if (!Coordinator.IsReady())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::CoordinatorNotReady,
			Operation,
			TEXT("Anchor operation requires its ready Combat Run Coordinator."));
	}

	Fdemo_mapShanmenFormationAnchorOperationResult Result =
		Controller.TryExecuteAnchorOperation(
			Authority, Coordinator, World, ActorClass, Operation);
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationAnchorOperationStatus::LifecycleInvalid;
		Result.Diagnostic =
			TEXT("Formation lifecycle failed invariants after anchor operation.");
	}
	return Result;
}

Fdemo_mapShanmenFormationRunLifecycleEndResult
Fdemo_mapShanmenFormationRunLifecycle::TryTeardownProduct(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	UWorld* World,
	Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenFormationRunLifecycleEndResult Result;
	if (!IsActive())
	{
		Result.Diagnostic =
			TEXT("Formation Run end requires one active lifecycle.");
		return Result;
	}
	Result.RunId = RunId;
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationRunLifecycleEndStatus::LifecycleInvalid;
		Result.Diagnostic =
			TEXT("Formation Run lifecycle invariants are invalid.");
		return Result;
	}
	if (!Coordinator.IsActive())
	{
		Result.Status =
			Edemo_mapShanmenFormationRunLifecycleEndStatus::CoordinatorNotActive;
		Result.Diagnostic =
			TEXT("Formation Run end requires its active Combat Run Coordinator.");
		return Result;
	}
	if (Coordinator.GetRunId() != RunId)
	{
		Result.Status =
			Edemo_mapShanmenFormationRunLifecycleEndStatus::RunMismatch;
		Result.Diagnostic =
			TEXT("Formation lifecycle cannot end a foreign Combat Run.");
		return Result;
	}

	Result.bHadScatterPublicationRoute =
		ScatterPublicationRoute.IsSet();
	Result.bReusedScatterPublicationTeardown =
		ScatterPublicationTeardownCheckpoint.IsSet();
	if (ScatterPublicationRoute.IsSet()
		&& !ScatterPublicationTeardownCheckpoint.IsSet())
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
			ScatterTeardown =
				ScatterPublicationRoute->TryEnd(RunId, World);
		if (!ScatterTeardown.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenFormationRunLifecycleEndStatus::
					ScatterPublicationTeardownRejected;
			Result.ScatterPublicationTeardown =
				MoveTemp(ScatterTeardown);
			Result.Diagnostic =
				Result.ScatterPublicationTeardown.Diagnostic.IsEmpty()
					? TEXT("Scatter publication route rejected teardown.")
					: Result.ScatterPublicationTeardown.Diagnostic;
			return Result;
		}
		ScatterPublicationTeardownCheckpoint =
			MoveTemp(ScatterTeardown);
	}
	if (ScatterPublicationTeardownCheckpoint.IsSet())
	{
		Result.ScatterPublicationTeardown =
			ScatterPublicationTeardownCheckpoint.GetValue();
	}

	Result.bReusedProductTeardown = ProductTeardownCheckpoint.IsSet();
	if (!ProductTeardownCheckpoint.IsSet())
	{
		Fdemo_mapShanmenFormationControllerEndSummary ProductTeardown;
		FString ProductDiagnostic;
		if (!Controller.TryTerminateAndEnd(
				Authority,
				World,
				RunId,
				ProductTeardown,
				ProductDiagnostic))
		{
			Result.Status =
				Edemo_mapShanmenFormationRunLifecycleEndStatus::
					ProductTeardownRejected;
			Result.ProductTeardown = MoveTemp(ProductTeardown);
			Result.Diagnostic = ProductDiagnostic.IsEmpty()
				? TEXT("Formation controller rejected product teardown.")
				: MoveTemp(ProductDiagnostic);
			return Result;
		}
		if (!ProductTeardown.IsValid()
			|| ProductTeardown.RunId != RunId
			|| !Controller.IsValid()
			|| !Controller.IsEmpty())
		{
			Result.Status =
				Edemo_mapShanmenFormationRunLifecycleEndStatus::
					LifecycleInvalid;
			Result.ProductTeardown = MoveTemp(ProductTeardown);
			Result.Diagnostic =
				TEXT("Formation controller produced an invalid teardown checkpoint.");
			return Result;
		}
		ProductTeardownCheckpoint = MoveTemp(ProductTeardown);
	}
	Result.ProductTeardown = ProductTeardownCheckpoint.GetValue();
	Result.Status =
		Edemo_mapShanmenFormationRunLifecycleEndStatus::
			ProductTeardownComplete;
	Result.Diagnostic = Result.bReusedProductTeardown
		? Result.bHadScatterPublicationRoute
			? TEXT("Formation teardown reused durable publication and product checkpoints; shared Combat Run release remains external.")
			: TEXT("Formation teardown reused its durable product checkpoint; shared Combat Run release remains external.")
		: Result.bHadScatterPublicationRoute
			? TEXT("Scatter publication and formation product teardown completed before shared Combat Run release.")
			: TEXT("Formation product teardown completed before shared Combat Run release.");
	return Result;
}

bool Fdemo_mapShanmenFormationRunLifecycle::
TryAcknowledgeCoordinatorEnded(
	const FGuid& ExpectedRunId,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic =
			TEXT("Formation Run release acknowledgement requires one active lifecycle.");
		return false;
	}
	if (!IsValid() || !ProductTeardownCheckpoint.IsSet()
		|| (ScatterPublicationRoute.IsSet()
			&& !ScatterPublicationTeardownCheckpoint.IsSet()))
	{
		OutDiagnostic =
			TEXT("Formation Run release acknowledgement requires one valid product teardown checkpoint.");
		return false;
	}
	if (!ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic =
			TEXT("Formation Run release acknowledgement rejected a mismatched Run identity.");
		return false;
	}
	if (Coordinator.IsActive())
	{
		OutDiagnostic =
			TEXT("Formation Run release acknowledgement requires shared Combat Run identities to be released first.");
		return false;
	}

	Clear();
	OutDiagnostic =
		TEXT("Formation lifecycle acknowledged the externally released Combat Run.");
	return IsValid() && IsEmpty();
}

Fdemo_mapShanmenFormationRunLifecycleEndResult
Fdemo_mapShanmenFormationRunLifecycle::TryEndRun(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	UWorld* World,
	Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenFormationRunLifecycleEndResult Result =
		TryTeardownProduct(Authority, World, Coordinator);
	if (!Result.IsProductTeardownComplete())
	{
		return Result;
	}

	FString CoordinatorDiagnostic;
	if (!Coordinator.TryEndRun(RunId, CoordinatorDiagnostic))
	{
		Result.Status =
			Edemo_mapShanmenFormationRunLifecycleEndStatus::
				CoordinatorEndRejected;
		Result.Diagnostic = CoordinatorDiagnostic.IsEmpty()
			? TEXT("Combat Run Coordinator rejected release after formation teardown.")
			: MoveTemp(CoordinatorDiagnostic);
		return Result;
	}

	FString AcknowledgeDiagnostic;
	if (!TryAcknowledgeCoordinatorEnded(
			Result.RunId, Coordinator, AcknowledgeDiagnostic))
	{
		Result.Status =
			Edemo_mapShanmenFormationRunLifecycleEndStatus::LifecycleInvalid;
		Result.Diagnostic = AcknowledgeDiagnostic.IsEmpty()
			? TEXT("Formation lifecycle rejected the completed Combat Run release acknowledgement.")
			: MoveTemp(AcknowledgeDiagnostic);
		return Result;
	}

	Result.Status = Edemo_mapShanmenFormationRunLifecycleEndStatus::Ended;
	Result.Diagnostic = Result.bReusedProductTeardown
		|| Result.bReusedScatterPublicationTeardown
		? TEXT("Combat Run release reused durable formation teardown checkpoints.")
		: Result.bHadScatterPublicationRoute
			? TEXT("Scatter publication and formation product ended before shared Combat Run identities were released.")
			: TEXT("Formation product ended before shared Combat Run identities were released.");
	return Result;
}

bool Fdemo_mapShanmenFormationRunLifecycle::IsValid() const
{
	if (!RunId.IsValid())
	{
		return Controller.IsValid()
			&& Controller.IsEmpty()
			&& !ScatterPublicationRoute.IsSet()
			&& !ScatterPublicationTeardownCheckpoint.IsSet()
			&& !ProductTeardownCheckpoint.IsSet();
	}
	if (!Controller.IsValid()
		|| (ScatterPublicationRoute.IsSet()
			&& (!ScatterPublicationRoute->IsValid()
				|| ScatterPublicationRoute->GetRunId() != RunId))
		|| (ScatterPublicationTeardownCheckpoint.IsSet()
			&& (!ScatterPublicationRoute.IsSet()
				|| !ScatterPublicationTeardownCheckpoint->IsSuccess()
				|| ScatterPublicationTeardownCheckpoint->RunId != RunId
				|| ScatterPublicationTeardownCheckpoint->RouteId
					!= ScatterPublicationRoute->GetRouteId()
				|| ScatterPublicationTeardownCheckpoint->Event
					!= EScatterRunEvent::End))
		|| (ProductTeardownCheckpoint.IsSet()
			&& ScatterPublicationRoute.IsSet()
			&& !ScatterPublicationTeardownCheckpoint.IsSet()))
	{
		return false;
	}
	if (!ProductTeardownCheckpoint.IsSet())
	{
		return Controller.IsActive()
			&& Controller.GetRunId() == RunId;
	}
	return Controller.IsEmpty()
		&& ProductTeardownCheckpoint->IsValid()
		&& ProductTeardownCheckpoint->RunId == RunId;
}

bool Fdemo_mapShanmenFormationRunLifecycle::IsEmpty() const
{
	return !RunId.IsValid()
		&& Controller.IsEmpty()
		&& !ScatterPublicationRoute.IsSet()
		&& !ScatterPublicationTeardownCheckpoint.IsSet()
		&& !ProductTeardownCheckpoint.IsSet();
}

void Fdemo_mapShanmenFormationRunLifecycle::Clear()
{
	RunId.Invalidate();
	Controller = Fdemo_mapShanmenFormationProductController();
	ScatterPublicationRoute.Reset();
	ScatterPublicationTeardownCheckpoint.Reset();
	ProductTeardownCheckpoint.Reset();
}
