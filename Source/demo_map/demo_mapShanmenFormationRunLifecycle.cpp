#include "demo_mapShanmenFormationRunLifecycle.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
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
	if (ProductTeardownCheckpoint.IsSet())
	{
		return RejectSubmission(
			Edemo_mapShanmenFormationControllerStatus::ControllerInactive,
			RunId,
			Intent,
			TEXT("Formation product teardown is complete; only exact Combat Run release may retry."));
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
	if (ProductTeardownCheckpoint.IsSet())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::TeardownPending,
			Operation,
			TEXT("Product teardown is complete; only exact Combat Run release may retry."));
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
		? TEXT("Formation product teardown reused its durable checkpoint; shared Combat Run release remains external.")
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
	if (!IsValid() || !ProductTeardownCheckpoint.IsSet())
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
		? TEXT("Combat Run release reused the durable formation teardown checkpoint.")
		: TEXT("Formation product ended before shared Combat Run identities were released.");
	return Result;
}

bool Fdemo_mapShanmenFormationRunLifecycle::IsValid() const
{
	if (!RunId.IsValid())
	{
		return Controller.IsValid()
			&& Controller.IsEmpty()
			&& !ProductTeardownCheckpoint.IsSet();
	}
	if (!Controller.IsValid())
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
		&& !ProductTeardownCheckpoint.IsSet();
}

void Fdemo_mapShanmenFormationRunLifecycle::Clear()
{
	RunId.Invalidate();
	Controller = Fdemo_mapShanmenFormationProductController();
	ProductTeardownCheckpoint.Reset();
}
