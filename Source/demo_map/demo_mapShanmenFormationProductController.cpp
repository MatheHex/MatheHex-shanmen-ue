#include "demo_mapShanmenFormationProductController.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	FString FloatBits(const float Value)
	{
		uint32 Bits = 0;
		FPlatformMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	bool FloatsMatchExactly(const float Left, const float Right)
	{
		return FloatBits(Left) == FloatBits(Right);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool RequirementsMatch(
		const FShanmenFormationMaterialRequirement& Left,
		const FShanmenFormationMaterialRequirement& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetOrder() == Right.GetOrder()
			&& Left.GetMaterialDefinitionId()
				== Right.GetMaterialDefinitionId()
			&& Left.GetQuantity() == Right.GetQuantity();
	}

	bool AnchorsMatch(
		const FShanmenFormationAnchorDefinition& Left,
		const FShanmenFormationAnchorDefinition& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.GetOrder() != Right.GetOrder()
			|| Left.GetAnchorDefinitionId() != Right.GetAnchorDefinitionId()
			|| Left.GetRelativeOffset() != Right.GetRelativeOffset()
			|| Left.GetRequirements().Num() != Right.GetRequirements().Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.GetRequirements().Num(); ++Index)
		{
			if (!RequirementsMatch(
					Left.GetRequirements()[Index],
					Right.GetRequirements()[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool DiagramsMatch(
		const FShanmenFormationDiagramDefinition& Left,
		const FShanmenFormationDiagramDefinition& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.GetActionDefinitionId() != Right.GetActionDefinitionId()
			|| Left.GetDiagramDefinitionId() != Right.GetDiagramDefinitionId()
			|| Left.GetActivationEnergyCost().GetCostId()
				!= Right.GetActivationEnergyCost().GetCostId()
			|| Left.GetAnchors().Num() != Right.GetAnchors().Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.GetAnchors().Num(); ++Index)
		{
			if (!AnchorsMatch(Left.GetAnchors()[Index], Right.GetAnchors()[Index]))
			{
				return false;
			}
		}
		return true;
	}

	Fdemo_mapShanmenFormationControllerResult Reject(
		const Edemo_mapShanmenFormationControllerStatus Status,
		const Fdemo_mapShanmenFormationIntent& Intent,
		const FGuid& ControllerRunId,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationControllerResult Result;
		Result.Status = Status;
		Result.IntentId = Intent.GetIntentId();
		Result.RunId = ControllerRunId;
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

	bool IsConcreteWorldBinding(
		UWorld* World,
		const TSubclassOf<AActor> ActorClass,
		FString& OutActorClassPath)
	{
		OutActorClassPath.Reset();
		UClass* RawClass = ActorClass.Get();
		if (!::IsValid(World)
			|| World->bIsTearingDown
			|| !RawClass
			|| RawClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			return false;
		}
		OutActorClassPath = RawClass->GetPathName();
		return !OutActorClassPath.IsEmpty();
	}
}

bool Fdemo_mapShanmenFormationIntent::TryCapture(
	const FGuid& RequestedIntentId,
	const FGuid& RequestedRunId,
	const FShanmenFormationDiagramDefinition& RequestedDiagram,
	const FVector& RequestedOrigin,
	const FVector& RequestedForward,
	Fdemo_mapShanmenFormationIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenFormationIntent();
	const FVector PlanarForward(
		RequestedForward.X, RequestedForward.Y, 0.0);
	if (!RequestedIntentId.IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedDiagram.IsValid()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedForward)
		|| PlanarForward.IsNearlyZero())
	{
		return false;
	}

	OutIntent.IntentId = RequestedIntentId;
	OutIntent.RunId = RequestedRunId;
	OutIntent.Diagram = RequestedDiagram;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.Forward = PlanarForward.GetSafeNormal();
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenFormationIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationIntent::IsValid() const
{
	return IntentId.IsValid()
		&& RunId.IsValid()
		&& Diagram.IsValid()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(Forward)
		&& Forward.Z == 0.0
		&& Forward.IsNormalized();
}

bool Fdemo_mapShanmenFormationIntent::Matches(
	const Fdemo_mapShanmenFormationIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& IntentId == Other.IntentId
		&& RunId == Other.RunId
		&& DiagramsMatch(Diagram, Other.Diagram)
		&& Origin == Other.Origin
		&& Forward == Other.Forward;
}

bool Fdemo_mapShanmenFormationControllerResult::IsAccepted() const
{
	if (Status != Edemo_mapShanmenFormationControllerStatus::Started
		|| !IntentId.IsValid()
		|| !RunId.IsValid()
		|| !Preparation.IsReady()
		|| !Startup.IsValid()
		|| !Active.IsValid()
		|| !Begin.IsValid()
		|| !ResourceReserve.IsValid()
		|| ResourceReserve.Status
			!= EShanmenActionResourceTransactionStatus::Reserved
		|| !ResourceCommit.IsValid()
		|| ResourceCommit.Status
			!= EShanmenActionResourceTransactionStatus::Committed
		|| !SharedResource.IsSuccess())
	{
		return false;
	}
	const FGuid& CommandId = Preparation.Command.GetCommandId();
	const FShanmenActionResourceCost& Cost =
		Preparation.Command.GetDiagram().GetActivationEnergyCost();
	const FShanmenActionResourceReservationReceipt& Reservation =
		ResourceReserve.Reservation;
	const FShanmenActionResourceFinalizationReceipt& Commit =
		ResourceCommit.Finalization;
	const Fdemo_mapShanmenSharedSpiritEnergyTransactionReceipt& Shared =
		SharedResource.Receipt;
	const FShanmenActionResourceSnapshot& Before =
		Shared.GetResourceBefore();
	const FShanmenActionResourceSnapshot& After =
		Shared.GetResourceAfter();
	return Preparation.Command.GetCorrelation().ActiveRunId == RunId
		&& Startup.GetActivationId() == CommandId
		&& Active.GetActivationId() == CommandId
		&& Startup.GetFromPhase() == EShanmenCombatActionPhase::Idle
		&& Startup.GetToPhase() == EShanmenCombatActionPhase::Startup
		&& Active.GetFromPhase() == EShanmenCombatActionPhase::Startup
		&& Active.GetToPhase() == EShanmenCombatActionPhase::Active
		&& Active.CrossedCommitPointNow()
		&& Begin.GetDeploymentId().IsValid()
		&& Reservation.GetRequest().GetAction().GetActivationId()
			== CommandId
		&& Reservation.GetRequest().GetStartupReceipt().GetActivationId()
			== Startup.GetActivationId()
		&& Reservation.GetRequest().GetStartupReceipt().GetSequence()
			== Startup.GetSequence()
		&& Reservation.GetRequest().GetCost().GetCostId()
			== Cost.GetCostId()
		&& Commit.GetRequest().GetReservation().GetReceiptId()
			== Reservation.GetReceiptId()
		&& Commit.GetRequest().GetTransition().GetActivationId()
			== Active.GetActivationId()
		&& Commit.GetRequest().GetTransition().GetSequence()
			== Active.GetSequence()
		&& Commit.GetRequest().GetDisposition()
			== EShanmenActionResourceDisposition::Commit
		&& Shared.GetTransactionId()
			== Fdemo_mapShanmenFormationProductAuthority::
				MakeActivationEnergyTransactionId(Preparation.Command)
		&& Shared.GetCommandId()
			== Fdemo_mapShanmenFormationProductAuthority::
				MakeActivationEnergyCommandId(Preparation.Command)
		&& Before.GetSnapshotId()
			== Reservation.GetRequest().GetResourceSnapshot().GetSnapshotId()
		&& Before.GetOwnerEntityId()
			== Preparation.Command.GetAction().GetSourceEntityId()
		&& Before.GetResourceChannel() == Cost.GetResourceChannel()
		&& After.GetResourceChannel() == Cost.GetResourceChannel()
		&& Commit.GetAuthorityRevisionAfter()
			== After.GetAuthorityRevision()
		&& FloatsMatchExactly(
			Before.GetCurrentAmount() - Cost.GetAmount(),
			After.GetCurrentAmount())
		&& FloatsMatchExactly(
			Commit.GetCurrentAfter(), After.GetCurrentAmount())
		&& FloatsMatchExactly(
			Commit.GetReservedAfter(), After.GetReservedAmount())
		&& FloatsMatchExactly(After.GetReservedAmount(), 0.0f);
}

bool Fdemo_mapShanmenFormationAnchorOperation::TryCapture(
	const FGuid& RequestedRunId,
	const FName RequestedAnchorDefinitionId,
	const FGuid& RequestedAttemptId,
	Fdemo_mapShanmenFormationAnchorOperation& OutOperation)
{
	OutOperation = Fdemo_mapShanmenFormationAnchorOperation();
	if (!RequestedRunId.IsValid()
		|| RequestedAnchorDefinitionId.IsNone()
		|| !RequestedAttemptId.IsValid())
	{
		return false;
	}
	OutOperation.RunId = RequestedRunId;
	OutOperation.AnchorDefinitionId = RequestedAnchorDefinitionId;
	OutOperation.AttemptId = RequestedAttemptId;
	return OutOperation.IsValid();
}

bool Fdemo_mapShanmenFormationAnchorOperation::IsValid() const
{
	return RunId.IsValid()
		&& !AnchorDefinitionId.IsNone()
		&& AttemptId.IsValid();
}

bool Fdemo_mapShanmenFormationAnchorOperationResult::IsSuccess() const
{
	if ((Status != Edemo_mapShanmenFormationAnchorOperationStatus::Placed
			&& Status
				!= Edemo_mapShanmenFormationAnchorOperationStatus::Replayed)
		|| !Operation.IsValid()
		|| ActorClassPath.IsEmpty()
		|| (!bResumedCommittedPlacement && !Prepared.IsSuccess())
		|| !Committed.IsSuccess()
		|| !Placement.IsSuccess()
		|| !Placement.PlacementIntent.IsValid()
		|| !Placement.World.IsPlacementSuccess()
		|| !Placement.World.PlacementReceipt.IsValid())
	{
		return false;
	}
	return Placement.PlacementIntent.RunId == Operation.GetRunId()
		&& Placement.PlacementIntent.AnchorDefinitionId
			== Operation.GetAnchorDefinitionId()
		&& Placement.PlacementIntent.AttemptId == Operation.GetAttemptId()
		&& Placement.World.PlacementReceipt.ActorClassPath == ActorClassPath;
}

bool Fdemo_mapShanmenFormationControllerEndSummary::IsValid() const
{
	if (!RunId.IsValid()
		|| CapturedIntentCount < 0
		|| CapturedIntentCount > 1
		|| (bHadProductHost && bDiscardedUnstartedCommand)
		|| (bEndedCompletedFormation && !bHadProductHost))
	{
		return false;
	}
	if (CapturedIntentCount == 0)
	{
		return !bHadProductHost
			&& !bEndedCompletedFormation
			&& !bDiscardedUnstartedCommand;
	}
	if (bHadProductHost)
	{
		return Terminal.IsSuccess()
			&& Terminal.World.IsTeardownSuccess()
			&& Terminal.World.TeardownReceipt.IsValid()
			&& Terminal.Session.Status
				== (bEndedCompletedFormation
					? Edemo_mapShanmenFormationSessionStatus::Ended
					: Edemo_mapShanmenFormationSessionStatus::Cancelled);
	}
	return !bEndedCompletedFormation
		&& bDiscardedUnstartedCommand
		&& !Terminal.IsSuccess();
}

bool Fdemo_mapShanmenFormationProductController::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsActive())
	{
		if (IsValid() && RunId == RequestedRunId)
		{
			OutDiagnostic =
				TEXT("Formation controller is already bound to this exact Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("An active formation controller cannot change Run identity.");
		return false;
	}
	if (!RequestedRunId.IsValid() || !IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Formation controller requires empty valid state and one Run identity.");
		return false;
	}

	RunId = RequestedRunId;
	if (!IsValid())
	{
		Clear();
		OutDiagnostic =
			TEXT("Formation controller failed closed during Run binding.");
		return false;
	}
	OutDiagnostic = TEXT("Formation controller bound to the active combat Run.");
	return true;
}

Fdemo_mapShanmenFormationControllerResult
Fdemo_mapShanmenFormationProductController::TrySubmit(
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseProductController& SpiritEnergyController,
	const Fdemo_mapShanmenFormationIntent& Intent)
{
	if (!IsActive())
	{
		return Reject(
			Edemo_mapShanmenFormationControllerStatus::ControllerInactive,
			Intent,
			RunId,
			TEXT("Formation submission requires one active Run controller."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationControllerStatus::ControllerInvalid,
			Intent,
			RunId,
			TEXT("Formation controller invariants are invalid."));
	}
	if (!Intent.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationControllerStatus::IntentInvalid,
			Intent,
			RunId,
			TEXT("Formation requires a valid device-independent intent."));
	}
	if (Intent.GetRunId() != RunId
		|| !Coordinator.IsReady()
		|| Coordinator.GetRunId() != RunId)
	{
		return Reject(
			Edemo_mapShanmenFormationControllerStatus::RunMismatch,
			Intent,
			RunId,
			TEXT("Formation intent, controller and coordinator must name one Run."));
	}

	if (CapturedIntent.IsSet())
	{
		if (CapturedIntent->Intent.GetIntentId() == Intent.GetIntentId())
		{
			if (!CapturedIntent->Intent.Matches(Intent))
			{
				return Reject(
					Edemo_mapShanmenFormationControllerStatus::IntentIdConflict,
					Intent,
					RunId,
					TEXT("Formation IntentId was reused with another payload."));
			}
			return StartCaptured(
				*CapturedIntent,
				Coordinator,
				SpiritEnergyController,
				true);
		}
		return Reject(
			Edemo_mapShanmenFormationControllerStatus::HostBusy,
			Intent,
			RunId,
			TEXT("The Run already owns one frozen formation intent."));
	}

	FCapturedIntent Captured;
	Captured.Intent = Intent;
	Captured.Preparation =
		Fdemo_mapShanmenFormationProductAuthority::PrepareDeployment(
			Coordinator,
			Authority,
			Intent.GetDiagram(),
			Intent.GetOrigin(),
			Intent.GetForward());
	if (!Captured.Preparation.IsReady())
	{
		Fdemo_mapShanmenFormationControllerResult Result = Reject(
			Edemo_mapShanmenFormationControllerStatus::PreparationRejected,
			Intent,
			RunId,
			Captured.Preparation.Diagnostic);
		Result.Preparation = Captured.Preparation;
		return Result;
	}

	CapturedIntent = MoveTemp(Captured);
	Fdemo_mapShanmenFormationControllerResult Result =
		StartCaptured(
			*CapturedIntent,
			Coordinator,
			SpiritEnergyController,
			false);
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationControllerStatus::ControllerInvalid;
		Result.Diagnostic =
			TEXT("Formation controller failed committed-intent invariants.");
	}
	return Result;
}

Fdemo_mapShanmenFormationControllerResult
Fdemo_mapShanmenFormationProductController::StartCaptured(
	FCapturedIntent& Captured,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseProductController& SpiritEnergyController,
	const bool bReusedIntent)
{
	if (bHasProductHost)
	{
		Fdemo_mapShanmenFormationControllerResult Result = Captured.LastResult;
		Result.bReusedIntent = bReusedIntent;
		Result.Diagnostic = bReusedIntent
			? TEXT("The exact formation intent replayed its frozen ProductHost start proof.")
			: Result.Diagnostic;
		return Result;
	}

	const Fdemo_mapShanmenFormationDeploymentCommand& Command =
		Captured.Preparation.Command;
	Fdemo_mapShanmenFormationProductHost CandidateHost;
	Fdemo_mapShanmenFormationControllerResult Result;
	Result.bReusedIntent = bReusedIntent;
	Result.IntentId = Captured.Intent.GetIntentId();
	Result.RunId = RunId;
	Result.Preparation = Captured.Preparation;
	if (!Fdemo_mapShanmenFormationProductHost::TryStart(
			Command.GetCorrelation(),
			Command.GetAction(),
			Command.GetDiagram(),
			Command.GetOrigin(),
			Command.GetForward(),
			CandidateHost,
			Result.Startup,
			Result.Active,
			Result.Begin))
	{
		Result.Status =
			Edemo_mapShanmenFormationControllerStatus::HostStartRejected;
		Result.Diagnostic =
			TEXT("The frozen formation command could not start ProductHost; exact retry remains available without another sequence.");
		Captured.LastResult = Result;
		return Result;
	}

	Fdemo_mapShanmenDivineSenseProductController EnergyCandidate =
		SpiritEnergyController;
	const FGuid TransactionId =
		Fdemo_mapShanmenFormationProductAuthority::
			MakeActivationEnergyTransactionId(Command);
	const FGuid EnergyCommandId =
		Fdemo_mapShanmenFormationProductAuthority::
			MakeActivationEnergyCommandId(Command);
	Result.SharedResource = EnergyCandidate.ApplySharedSpiritEnergyTransaction(
		Coordinator,
		TransactionId,
		EnergyCommandId,
		[&](FShanmenActionResourceAuthority& ResourceAuthority)
		{
			FShanmenActionResourceSnapshot ResourceBefore;
			FShanmenActionResourceReservationRequest ReservationRequest;
			if (!ResourceAuthority.TryCaptureSnapshot(ResourceBefore)
				|| !FShanmenActionResourceReservationRequest::TryCreate(
					Command.GetAction(),
					Result.Startup,
					Command.GetDiagram().GetActivationEnergyCost(),
					ResourceBefore,
					ReservationRequest))
			{
				return false;
			}

			Result.ResourceReserve =
				ResourceAuthority.Reserve(ReservationRequest);
			if (!Result.ResourceReserve.IsSuccess()
				|| Result.ResourceReserve.Status
					!= EShanmenActionResourceTransactionStatus::Reserved)
			{
				return false;
			}

			FShanmenActionResourceFinalizationRequest CommitRequest;
			if (!FShanmenActionResourceFinalizationRequest::TryCreate(
					Result.ResourceReserve.Reservation,
					Result.Active,
					CommitRequest))
			{
				return false;
			}
			Result.ResourceCommit = ResourceAuthority.Finalize(CommitRequest);
			return Result.ResourceCommit.IsSuccess()
				&& Result.ResourceCommit.Status
					== EShanmenActionResourceTransactionStatus::Committed
				&& ResourceAuthority.NumPendingReservations() == 0;
		});
	if (!Result.SharedResource.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationControllerStatus::SharedResourceRejected;
		Result.Diagnostic = Result.SharedResource.Diagnostic.IsEmpty()
			? TEXT("Formation activation energy transaction rejected; exact retry remains available without another sequence.")
			: Result.SharedResource.Diagnostic;
		Captured.LastResult = Result;
		return Result;
	}

	Result.Status = Edemo_mapShanmenFormationControllerStatus::Started;
	Result.Diagnostic = bReusedIntent
		? TEXT("The exact formation intent recovered ProductHost and committed its authored activation cost once.")
		: TEXT("Formation authority atomically started ProductHost and committed its authored shared SpiritEnergy cost.");
	if (!Result.IsAccepted()
		|| !CandidateHost.IsValid()
		|| !EnergyCandidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationControllerStatus::StateDesynchronized;
		Result.Diagnostic =
			TEXT("Formation ProductHost and shared SpiritEnergy proofs did not agree; no candidate state was published.");
		Captured.LastResult = Result;
		return Result;
	}

	SpiritEnergyController = MoveTemp(EnergyCandidate);
	ProductHost = MoveTemp(CandidateHost);
	bHasProductHost = true;
	Captured.LastResult = Result;
	return Result;
}

Fdemo_mapShanmenFormationAnchorOperationResult
Fdemo_mapShanmenFormationProductController::TryExecuteAnchorOperation(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	const TSubclassOf<AActor> ActorClass,
	const Fdemo_mapShanmenFormationAnchorOperation& Operation)
{
	if (!IsActive())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::ControllerInactive,
			Operation,
			TEXT("Formation anchor operation requires one active controller."));
	}
	if (!IsValid())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::ControllerInvalid,
			Operation,
			TEXT("Formation controller invariants are invalid."));
	}
	if (!Operation.IsValid())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::OperationInvalid,
			Operation,
			TEXT("Formation anchor operation requires immutable Run, anchor and attempt identities."));
	}
	if (Operation.GetRunId() != RunId || Coordinator.GetRunId() != RunId)
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::RunMismatch,
			Operation,
			TEXT("Anchor operation, controller and Coordinator must name one Run."));
	}
	if (!Coordinator.IsReady())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::CoordinatorNotReady,
			Operation,
			TEXT("Formation anchor operation requires its ready Combat Run."));
	}
	if (!bHasProductHost || !CapturedIntent.IsSet())
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::ProductHostMissing,
			Operation,
			TEXT("Formation anchor operation requires the controller's sole ProductHost."));
	}

	FString ActorClassPath;
	if (!IsConcreteWorldBinding(World, ActorClass, ActorClassPath))
	{
		return RejectAnchorOperation(
			Edemo_mapShanmenFormationAnchorOperationStatus::WorldBindingInvalid,
			Operation,
			TEXT("World and concrete Actor class must pass before durable anchor authority is accessed."));
	}

	Fdemo_mapShanmenFormationAnchorOperationResult Result;
	Result.Operation = Operation;
	Result.ActorClassPath = MoveTemp(ActorClassPath);
	const FName AnchorDefinitionId = Operation.GetAnchorDefinitionId();
	const FGuid& AttemptId = Operation.GetAttemptId();
	const Fdemo_mapShanmenRunCorrelation& Correlation =
		CapturedIntent->Preparation.Command.GetCorrelation();
	if (const Fdemo_mapShanmenFormationAnchorPlacementIntent* Pending =
		ProductHost.GetPendingPlacement())
	{
		if (Pending->RunId != RunId
			|| Pending->AnchorDefinitionId != AnchorDefinitionId
			|| Pending->AttemptId != AttemptId)
		{
			Result.Status =
				Edemo_mapShanmenFormationAnchorOperationStatus::
					PendingPlacementConflict;
			Result.Committed.PlacementIntent = *Pending;
			Result.Diagnostic =
				TEXT("A different committed placement must recover before this anchor operation.");
			return Result;
		}
		Result.bResumedCommittedPlacement = true;
	}
	else
	{
		Result.Prepared = ProductHost.TryPrepareAnchor(
			Authority, Correlation, AnchorDefinitionId, AttemptId);
		if (!Result.Prepared.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenFormationAnchorOperationStatus::
					PreparationRejected;
			Result.Diagnostic = Result.Prepared.Diagnostic;
			return Result;
		}
	}

	Result.Committed = ProductHost.TryCommitPreparedAnchor(
		Authority, Correlation, AnchorDefinitionId, AttemptId);
	if (!Result.Committed.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationAnchorOperationStatus::CommitRejected;
		Result.Diagnostic = Result.Committed.Diagnostic;
		return Result;
	}
	Result.Placement = ProductHost.TryPlaceCommittedAnchor(
		World, ActorClass, Correlation, AnchorDefinitionId, AttemptId);
	if (!Result.Placement.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationAnchorOperationStatus::PlacementRejected;
		Result.Diagnostic = Result.Placement.Diagnostic;
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationAnchorOperationStatus::ControllerInvalid;
		Result.Diagnostic =
			TEXT("Formation controller failed invariants after anchor placement.");
		return Result;
	}

	Result.Status = Result.Placement.Status
		== Edemo_mapShanmenFormationHostStatus::Placed
		? Edemo_mapShanmenFormationAnchorOperationStatus::Placed
		: Edemo_mapShanmenFormationAnchorOperationStatus::Replayed;
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenFormationAnchorOperationStatus::Placed
		? TEXT("The explicit anchor operation committed once and entered the World.")
		: TEXT("The exact anchor operation replayed existing durable and World evidence.");
	if (!Result.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationAnchorOperationStatus::ControllerInvalid;
		Result.Diagnostic =
			TEXT("Formation anchor gateway produced invalid composite evidence.");
	}
	return Result;
}

bool Fdemo_mapShanmenFormationProductController::TryTerminateAndEnd(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	UWorld* World,
	const FGuid& ExpectedRunId,
	Fdemo_mapShanmenFormationControllerEndSummary& OutSummary,
	FString& OutDiagnostic)
{
	OutSummary = Fdemo_mapShanmenFormationControllerEndSummary();
	OutDiagnostic.Reset();
	if (!IsActive() || !IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic =
			TEXT("Formation controller end requires its exact active Run identity.");
		return false;
	}

	OutSummary.RunId = RunId;
	OutSummary.CapturedIntentCount = NumCapturedIntents();
	OutSummary.bHadProductHost = bHasProductHost;
	OutSummary.bDiscardedUnstartedCommand =
		CapturedIntent.IsSet() && !bHasProductHost;
	if (bHasProductHost)
	{
		const Edemo_mapShanmenFormationSessionState SessionState =
			ProductHost.GetSession().GetState();
		OutSummary.bEndedCompletedFormation =
			SessionState == Edemo_mapShanmenFormationSessionState::Active
			|| SessionState == Edemo_mapShanmenFormationSessionState::Ended;
		const Fdemo_mapShanmenRunCorrelation& Correlation =
			CapturedIntent->Preparation.Command.GetCorrelation();
		OutSummary.Terminal = OutSummary.bEndedCompletedFormation
			? ProductHost.TryEndAndTeardown(World, Correlation)
			: ProductHost.TryCancelAndTeardown(
				Authority, World, Correlation);
		if (!OutSummary.Terminal.IsSuccess())
		{
			OutDiagnostic = OutSummary.Terminal.Diagnostic;
			return false;
		}
	}

	if (!OutSummary.IsValid())
	{
		OutDiagnostic =
			TEXT("Formation controller produced an invalid Run-end summary.");
		return false;
	}
	Clear();
	if (!IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Formation controller failed empty-state validation after teardown.");
		return false;
	}
	OutDiagnostic =
		TEXT("Formation controller ended without hidden intent or ProductHost work.");
	return true;
}

bool Fdemo_mapShanmenFormationProductController::IsValid() const
{
	if (!RunId.IsValid())
	{
		return !CapturedIntent.IsSet() && !bHasProductHost;
	}
	if (!CapturedIntent.IsSet())
	{
		return !bHasProductHost;
	}

	const FCapturedIntent& Captured = *CapturedIntent;
	const Fdemo_mapShanmenFormationDeploymentCommand& Command =
		Captured.Preparation.Command;
	if (!Captured.Intent.IsValid()
		|| Captured.Intent.GetRunId() != RunId
		|| !Captured.Preparation.IsReady()
		|| Command.GetCorrelation().ActiveRunId != RunId
		|| !DiagramsMatch(
			Command.GetDiagram(), Captured.Intent.GetDiagram())
		|| Command.GetOrigin() != Captured.Intent.GetOrigin()
		|| Command.GetForward() != Captured.Intent.GetForward()
		|| Captured.LastResult.IntentId != Captured.Intent.GetIntentId()
		|| Captured.LastResult.RunId != RunId)
	{
		return false;
	}
	if (!bHasProductHost)
	{
		const bool bRetryableFailure =
			Captured.LastResult.Status
				== Edemo_mapShanmenFormationControllerStatus::HostStartRejected
			|| Captured.LastResult.Status
				== Edemo_mapShanmenFormationControllerStatus::
					SharedResourceRejected
			|| Captured.LastResult.Status
				== Edemo_mapShanmenFormationControllerStatus::
					StateDesynchronized;
		return bRetryableFailure
			&& !Captured.LastResult.IsAccepted();
	}
	if (!Captured.LastResult.IsAccepted() || !ProductHost.IsValid())
	{
		return false;
	}

	const Fdemo_mapShanmenFormationProductSession& Session =
		ProductHost.GetSession();
	return Session.GetCorrelation() == Command.GetCorrelation()
		&& Session.GetActionRuntime().GetAction().GetActivationId()
			== Command.GetCommandId()
		&& DiagramsMatch(
			Session.GetDeployment().GetDiagram(), Command.GetDiagram())
		&& Session.GetDeployment().GetOrigin() == Command.GetOrigin()
		&& Session.GetDeployment().GetForward() == Command.GetForward();
}

bool Fdemo_mapShanmenFormationProductController::IsEmpty() const
{
	return !RunId.IsValid() && !CapturedIntent.IsSet() && !bHasProductHost;
}

const Fdemo_mapShanmenFormationDeploymentCommand*
Fdemo_mapShanmenFormationProductController::FindCapturedCommand(
	const FGuid& IntentId) const
{
	return CapturedIntent.IsSet()
		&& CapturedIntent->Intent.GetIntentId() == IntentId
		&& CapturedIntent->Preparation.IsReady()
		? &CapturedIntent->Preparation.Command
		: nullptr;
}

void Fdemo_mapShanmenFormationProductController::Clear()
{
	RunId.Invalidate();
	CapturedIntent.Reset();
	ProductHost = Fdemo_mapShanmenFormationProductHost();
	bHasProductHost = false;
}
