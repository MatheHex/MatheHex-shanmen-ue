#include "demo_mapShanmenFormationProductHost.h"

#include "demo_mapShanmenItemAuthoritySubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool IntentsMatch(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Left,
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.PlacementId == Right.PlacementId
			&& Left.RunId == Right.RunId
			&& Left.OwnerId == Right.OwnerId
			&& Left.DeploymentId == Right.DeploymentId
			&& Left.AnchorDefinitionId == Right.AnchorDefinitionId
			&& Left.AnchorInstanceId == Right.AnchorInstanceId
			&& Left.WorldLocation == Right.WorldLocation
			&& Left.AttemptId == Right.AttemptId
			&& Left.FulfillmentId == Right.FulfillmentId
			&& Left.DeploymentReceiptId == Right.DeploymentReceiptId
			&& Left.AuthorityRevision == Right.AuthorityRevision
			&& SameContent(Left.Content, Right.Content);
	}

	Fdemo_mapShanmenFormationHostResult Reject(
		const Edemo_mapShanmenFormationHostStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationHostResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	Fdemo_mapShanmenFormationHostCoverageResult RejectCoverage(
		const Edemo_mapShanmenFormationHostCoverageStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenFormationHostCoverageResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	bool IsConcreteActorClass(
		const TSubclassOf<AActor> ActorClass,
		FString& OutClassPath)
	{
		OutClassPath.Reset();
		UClass* RawClass = ActorClass.Get();
		if (!RawClass
			|| RawClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			return false;
		}
		OutClassPath = RawClass->GetPathName();
		return !OutClassPath.IsEmpty();
	}
}

bool Fdemo_mapShanmenFormationHostResult::IsSuccess() const
{
	return Status == Edemo_mapShanmenFormationHostStatus::Prepared
		|| Status
			== Edemo_mapShanmenFormationHostStatus::CommittedPendingPlacement
		|| Status == Edemo_mapShanmenFormationHostStatus::Placed
		|| Status == Edemo_mapShanmenFormationHostStatus::Replayed
		|| Status == Edemo_mapShanmenFormationHostStatus::Cancelled
		|| Status == Edemo_mapShanmenFormationHostStatus::Ended
		|| Status == Edemo_mapShanmenFormationHostStatus::TeardownReplayed;
}

bool Fdemo_mapShanmenFormationHostCoverageResult::IsSuccess() const
{
	switch (Status)
	{
	case Edemo_mapShanmenFormationHostCoverageStatus::Coordinated:
		return Area.IsSuccess() && Coordination.IsSuccess()
			&& !TrackerReset.IsSuccess();
	case Edemo_mapShanmenFormationHostCoverageStatus::Reset:
	case Edemo_mapShanmenFormationHostCoverageStatus::ResetReplayed:
		return !Area.IsSuccess() && !Coordination.IsSuccess()
			&& TrackerReset.IsSuccess();
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationProductHost::TryStart(
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenFormationDiagramDefinition& Diagram,
	const FVector& Origin,
	const FVector& Forward,
	Fdemo_mapShanmenFormationProductHost& OutHost,
	FShanmenActionTransitionReceipt& OutStartup,
	FShanmenActionTransitionReceipt& OutActive,
	FShanmenFormationDeploymentReceipt& OutBegin)
{
	OutHost = Fdemo_mapShanmenFormationProductHost();
	Fdemo_mapShanmenFormationProductHost Candidate;
	if (!Fdemo_mapShanmenFormationProductSession::TryStart(
			Correlation, Action, Diagram, Origin, Forward,
			Candidate.Session, OutStartup, OutActive, OutBegin)
		|| !Candidate.IsValid())
	{
		OutStartup = FShanmenActionTransitionReceipt();
		OutActive = FShanmenActionTransitionReceipt();
		OutBegin = FShanmenFormationDeploymentReceipt();
		return false;
	}
	OutHost = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenFormationHostResult
Fdemo_mapShanmenFormationProductHost::TryPrepareAnchor(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::HostInvalid,
			TEXT("Formation preparation requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::CorrelationMismatch,
			TEXT("Formation preparation rejected a stale or foreign Run correlation."));
	}
	if (AnchorDefinitionId.IsNone() || !AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::RequestInvalid,
			TEXT("Formation preparation requires an anchor and explicit AttemptId."));
	}
	if (bHasPendingPlacement)
	{
		Fdemo_mapShanmenFormationHostResult Result = Reject(
			Edemo_mapShanmenFormationHostStatus::PlacementPending,
			TEXT("The exact committed placement must resolve before another material operation."));
		Result.PlacementIntent = PendingPlacement;
		return Result;
	}

	Fdemo_mapShanmenFormationHostResult Result;
	Result.Session = Session.TryPrepareAnchor(
		Authority, RequestedCorrelation, AnchorDefinitionId, AttemptId);
	if (!Result.Session.IsSuccess())
	{
		Result.Status = Edemo_mapShanmenFormationHostStatus::SessionRejected;
		Result.Diagnostic = Result.Session.Diagnostic;
		return Result;
	}
	Result.Status = Result.Session.Status
		== Edemo_mapShanmenFormationSessionStatus::Replayed
		? Edemo_mapShanmenFormationHostStatus::Replayed
		: Edemo_mapShanmenFormationHostStatus::Prepared;
	Result.Diagnostic = Result.Session.Diagnostic;
	return Result;
}

Fdemo_mapShanmenFormationHostResult
Fdemo_mapShanmenFormationProductHost::TryCommitPreparedAnchor(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::HostInvalid,
			TEXT("Formation commit requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::CorrelationMismatch,
			TEXT("Formation commit rejected a stale or foreign Run correlation."));
	}
	if (AnchorDefinitionId.IsNone() || !AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::RequestInvalid,
			TEXT("Formation commit requires an anchor and explicit AttemptId."));
	}
	if (bHasPendingPlacement)
	{
		Fdemo_mapShanmenFormationHostResult Result;
		const bool bExactReplay =
			PendingMatches(AnchorDefinitionId, AttemptId);
		Result.Status = bExactReplay
			? Edemo_mapShanmenFormationHostStatus::CommittedPendingPlacement
			: Edemo_mapShanmenFormationHostStatus::PlacementPending;
		Result.Diagnostic = bExactReplay
			? TEXT("The exact durable commit replayed its pending placement intent.")
			: TEXT("Another committed placement must resolve before a different anchor commit.");
		Result.PlacementIntent = PendingPlacement;
		if (bExactReplay)
		{
			const Fdemo_mapShanmenFormationAnchorAudit* Audit =
				Session.GetAnchorAudits().FindByPredicate(
					[AnchorDefinitionId](
						const Fdemo_mapShanmenFormationAnchorAudit& Candidate)
					{
						return Candidate.AnchorDefinitionId
							== AnchorDefinitionId;
					});
			if (!Audit || Audit->AttemptId != AttemptId)
			{
				return Reject(
					Edemo_mapShanmenFormationHostStatus::HostInvalid,
					TEXT("Pending placement lost its immutable anchor audit."));
			}
			Result.Session.Status =
				Edemo_mapShanmenFormationSessionStatus::Replayed;
			Result.Session.Diagnostic = Result.Diagnostic;
			Result.Session.AnchorDefinitionId = AnchorDefinitionId;
			Result.Session.AttemptId = AttemptId;
			Result.Session.Material = Audit->Material;
			Result.Session.DeploymentReceipt = Audit->DeploymentReceipt;
		}
		return Result;
	}

	Fdemo_mapShanmenFormationHostResult Result;
	Result.Session = Session.TryCommitPreparedAnchor(
		Authority, RequestedCorrelation, AnchorDefinitionId, AttemptId);
	if (!Result.Session.IsSuccess())
	{
		Result.Status = Edemo_mapShanmenFormationHostStatus::SessionRejected;
		Result.Diagnostic = Result.Session.Diagnostic;
		return Result;
	}
	if (!Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
			Session, AnchorDefinitionId, Result.PlacementIntent)
		|| Result.PlacementIntent.AttemptId != AttemptId)
	{
		Result.Status = Edemo_mapShanmenFormationHostStatus::HostInvalid;
		Result.Diagnostic =
			TEXT("Durable anchor audit failed to publish its canonical placement intent.");
		return Result;
	}
	if (WorldAdapter.FindReceipt(Result.PlacementIntent.PlacementId))
	{
		Result.Status = Edemo_mapShanmenFormationHostStatus::Replayed;
		Result.Diagnostic =
			TEXT("The exact durable commit and published placement already exist.");
		return Result;
	}

	PendingPlacement = Result.PlacementIntent;
	bHasPendingPlacement = true;
	Result.Status =
		Edemo_mapShanmenFormationHostStatus::CommittedPendingPlacement;
	Result.Diagnostic =
		TEXT("Materials and deployment committed; this exact placement is now the only forward operation.");
	return Result;
}

Fdemo_mapShanmenFormationHostResult
Fdemo_mapShanmenFormationProductHost::TryPlaceCommittedAnchor(
	UWorld* World,
	const TSubclassOf<AActor> ActorClass,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const FName AnchorDefinitionId,
	const FGuid& AttemptId)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::HostInvalid,
			TEXT("Formation placement requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::CorrelationMismatch,
			TEXT("Formation placement rejected a stale or foreign Run correlation."));
	}
	if (AnchorDefinitionId.IsNone() || !AttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::RequestInvalid,
			TEXT("Formation placement requires an anchor and explicit AttemptId."));
	}
	if (Session.IsTerminal())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::TerminalConflict,
			TEXT("A terminal formation accepts teardown replay only."));
	}

	Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
	if (bHasPendingPlacement)
	{
		if (!PendingMatches(AnchorDefinitionId, AttemptId))
		{
			Fdemo_mapShanmenFormationHostResult Result = Reject(
				Edemo_mapShanmenFormationHostStatus::PlacementPending,
				TEXT("Placement retry must match the sole committed anchor intent."));
			Result.PlacementIntent = PendingPlacement;
			return Result;
		}
		Intent = PendingPlacement;
	}
	else
	{
		if (!Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
				Session, AnchorDefinitionId, Intent)
			|| Intent.AttemptId != AttemptId)
		{
			return Reject(
				Edemo_mapShanmenFormationHostStatus::PlacementMissing,
				TEXT("No exact committed audit exists for this placement request."));
		}
	}

	FString ActorClassPath;
	if (!::IsValid(World) || !IsConcreteActorClass(ActorClass, ActorClassPath))
	{
		Fdemo_mapShanmenFormationHostResult Result = Reject(
			Edemo_mapShanmenFormationHostStatus::RequestInvalid,
			TEXT("Placement preflight requires one live World and concrete Actor class."));
		Result.PlacementIntent = Intent;
		return Result;
	}
	const Fdemo_mapShanmenFormationAnchorPlacementReceipt* ExistingReceipt =
		WorldAdapter.FindReceipt(Intent.PlacementId);
	if (ExistingReceipt && ExistingReceipt->ActorClassPath != ActorClassPath)
	{
		Fdemo_mapShanmenFormationHostResult Result = Reject(
			Edemo_mapShanmenFormationHostStatus::PlacementBindingConflict,
			TEXT("The published placement cannot be rebound to another Actor class."));
		Result.PlacementIntent = Intent;
		return Result;
	}
	if (bHasPendingPlacement)
	{
		if (bPlacementBindingFrozen
			&& (BoundPlacementWorld.Get() != World
				|| BoundPlacementClassPath != ActorClassPath))
		{
			Fdemo_mapShanmenFormationHostResult Result = Reject(
				Edemo_mapShanmenFormationHostStatus::PlacementBindingConflict,
				TEXT("Forward placement retry must retain its first valid World and Actor class."));
			Result.PlacementIntent = Intent;
			return Result;
		}
		if (!bPlacementBindingFrozen)
		{
			BoundPlacementWorld = World;
			BoundPlacementClass = ActorClass;
			BoundPlacementClassPath = ActorClassPath;
			bPlacementBindingFrozen = true;
		}
	}

	Fdemo_mapShanmenFormationHostResult Result;
	Result.PlacementIntent = Intent;
	Result.World = WorldAdapter.TryPlaceCommittedAnchor(
		World, ActorClass, Session, AnchorDefinitionId);
	if (!Result.World.IsPlacementSuccess())
	{
		Result.Status = Edemo_mapShanmenFormationHostStatus::WorldRejected;
		Result.Diagnostic = Result.World.Diagnostic;
		return Result;
	}
	if (bHasPendingPlacement)
	{
		ClearPendingPlacement();
	}
	Result.Status = Result.World.Status
		== Edemo_mapShanmenFormationWorldStatus::Placed
		? Edemo_mapShanmenFormationHostStatus::Placed
		: Edemo_mapShanmenFormationHostStatus::Replayed;
	Result.Diagnostic = Result.World.Diagnostic;
	return Result;
}

Fdemo_mapShanmenFormationHostResult
Fdemo_mapShanmenFormationProductHost::TryCancelAndTeardown(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	UWorld* World,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::HostInvalid,
			TEXT("Formation cancellation requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::CorrelationMismatch,
			TEXT("Formation cancellation rejected a stale or foreign Run correlation."));
	}
	if (Session.GetState() == Edemo_mapShanmenFormationSessionState::Ended)
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::TerminalConflict,
			TEXT("An ended formation cannot replay the cancellation path."));
	}
	if (bPlacementBindingFrozen && BoundPlacementWorld.Get() != World)
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::PlacementBindingConflict,
			TEXT("Terminal cleanup must use the World frozen by placement recovery."));
	}

	Fdemo_mapShanmenFormationHostResult Result;
	if (Session.GetState()
		!= Edemo_mapShanmenFormationSessionState::Cancelled)
	{
		Result.Session = Session.TryCancel(Authority, RequestedCorrelation);
		if (!Result.Session.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenFormationHostStatus::SessionRejected;
			Result.Diagnostic = Result.Session.Diagnostic;
			return Result;
		}
	}
	else
	{
		Result.Session.Status =
			Edemo_mapShanmenFormationSessionStatus::Cancelled;
		Result.Session.Diagnostic =
			TEXT("The exact cancelled session replayed terminal cleanup.");
	}
	Result.World = WorldAdapter.TryTeardownTerminal(World, Session);
	if (!Result.World.IsTeardownSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationHostStatus::TerminalRecoveryRequired;
		Result.Diagnostic = Result.World.Diagnostic;
		return Result;
	}
	ClearPendingPlacement();
	CoverageTracker.Reset();
	Result.Status = Result.World.Status
		== Edemo_mapShanmenFormationWorldStatus::TeardownReplayed
		? Edemo_mapShanmenFormationHostStatus::TeardownReplayed
		: Edemo_mapShanmenFormationHostStatus::Cancelled;
	Result.Diagnostic = Result.World.Diagnostic;
	return Result;
}

Fdemo_mapShanmenFormationHostResult
Fdemo_mapShanmenFormationProductHost::TryEndAndTeardown(
	UWorld* World,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::HostInvalid,
			TEXT("Formation end requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::CorrelationMismatch,
			TEXT("Formation end rejected a stale or foreign Run correlation."));
	}
	if (bHasPendingPlacement)
	{
		Fdemo_mapShanmenFormationHostResult Result = Reject(
			Edemo_mapShanmenFormationHostStatus::PlacementPending,
			TEXT("The final committed anchor must enter the World before formation end."));
		Result.PlacementIntent = PendingPlacement;
		return Result;
	}
	if (Session.GetState()
		== Edemo_mapShanmenFormationSessionState::Cancelled)
	{
		return Reject(
			Edemo_mapShanmenFormationHostStatus::TerminalConflict,
			TEXT("A cancelled formation cannot replay the completion path."));
	}

	Fdemo_mapShanmenFormationHostResult Result;
	if (Session.GetState() != Edemo_mapShanmenFormationSessionState::Ended)
	{
		Result.Session = Session.TryEnd(RequestedCorrelation);
		if (!Result.Session.IsSuccess())
		{
			Result.Status =
				Edemo_mapShanmenFormationHostStatus::SessionRejected;
			Result.Diagnostic = Result.Session.Diagnostic;
			return Result;
		}
	}
	else
	{
		Result.Session.Status = Edemo_mapShanmenFormationSessionStatus::Ended;
		Result.Session.Diagnostic =
			TEXT("The exact ended session replayed terminal cleanup.");
	}
	Result.World = WorldAdapter.TryTeardownTerminal(World, Session);
	if (!Result.World.IsTeardownSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationHostStatus::TerminalRecoveryRequired;
		Result.Diagnostic = Result.World.Diagnostic;
		return Result;
	}
	CoverageTracker.Reset();
	Result.Status = Result.World.Status
		== Edemo_mapShanmenFormationWorldStatus::TeardownReplayed
		? Edemo_mapShanmenFormationHostStatus::TeardownReplayed
		: Edemo_mapShanmenFormationHostStatus::Ended;
	Result.Diagnostic = Result.World.Diagnostic;
	return Result;
}

Fdemo_mapShanmenFormationHostCoverageResult
Fdemo_mapShanmenFormationProductHost::TryCoordinateCoverage(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	const TArray<AActor*>& SourceActors,
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapShanmenFormationCoverageCommand& Command)
{
	if (!IsValid())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::HostInvalid,
			TEXT("Coverage coordination requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::CorrelationMismatch,
			TEXT("Coverage coordination rejected a stale or foreign Run correlation."));
	}
	if (Session.IsTerminal())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::SessionTerminal,
			TEXT("Terminal formation teardown owns coverage-baseline cleanup."));
	}
	if (Session.GetState() != Edemo_mapShanmenFormationSessionState::Active)
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::SessionNotActive,
			TEXT("Coverage begins only after every formation anchor commits."));
	}
	if (bHasPendingPlacement)
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::PlacementPending,
			TEXT("Coverage cannot sample while the final committed placement is pending."));
	}
	if (!WorldAdapter.IsBoundToWorld(World))
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::WorldMismatch,
			TEXT("Coverage must sample the World bound by this host's placements."));
	}

	const TArray<Fdemo_mapShanmenFormationAnchorAudit>& Audits =
		Session.GetAnchorAudits();
	if (Audits.IsEmpty() || WorldAdapter.GetPlacementCount() != Audits.Num())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::PlacementIncomplete,
			TEXT("Every committed anchor must have one canonical placement receipt."));
	}

	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Receipts;
	Receipts.Reserve(Audits.Num());
	for (const Fdemo_mapShanmenFormationAnchorAudit& Audit : Audits)
	{
		Fdemo_mapShanmenFormationAnchorPlacementIntent ExpectedIntent;
		if (!Audit.IsValid()
			|| !Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
				Session, Audit.AnchorDefinitionId, ExpectedIntent))
		{
			return RejectCoverage(
				Edemo_mapShanmenFormationHostCoverageStatus::PlacementIncomplete,
				TEXT("A committed anchor could not rebuild its canonical placement identity."));
		}
		const Fdemo_mapShanmenFormationAnchorPlacementReceipt* Receipt =
			WorldAdapter.FindReceipt(ExpectedIntent.PlacementId);
		if (!Receipt || !Receipt->IsValid()
			|| !IntentsMatch(Receipt->Intent, ExpectedIntent))
		{
			return RejectCoverage(
				Edemo_mapShanmenFormationHostCoverageStatus::PlacementIncomplete,
				TEXT("A committed anchor is missing its exact host-owned placement receipt."));
		}
		Receipts.Add(*Receipt);
	}

	Fdemo_mapShanmenFormationHostCoverageResult Result;
	Result.Area = Fdemo_mapShanmenFormationAreaProvider::BuildArea(Receipts);
	if (!Result.Area.IsSuccess()
		|| Result.Area.Area.RunId != RequestedCorrelation.ActiveRunId
		|| Result.Area.Area.OwnerId != RequestedCorrelation.OwnerId
		|| Result.Area.Area.DeploymentId
			!= Session.GetDeployment().GetDeploymentId())
	{
		Result.Status = Edemo_mapShanmenFormationHostCoverageStatus::AreaRejected;
		Result.Diagnostic = Result.Area.Diagnostic.IsEmpty()
			? TEXT("The rebuilt area does not match this host's Run and deployment scope.")
			: Result.Area.Diagnostic;
		return Result;
	}

	Result.Coordination =
		Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
			World, Result.Area.Area, EntityRegistry, SourceActors,
			Command, CoverageTracker);
	if (!Result.Coordination.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationHostCoverageStatus::CoordinatorRejected;
		Result.Diagnostic = Result.Coordination.Diagnostic;
		return Result;
	}
	Result.Status = Edemo_mapShanmenFormationHostCoverageStatus::Coordinated;
	Result.Diagnostic =
		TEXT("The host rebuilt its canonical area and committed one explicit coverage command.");
	return Result;
}

Fdemo_mapShanmenFormationHostCoverageResult
Fdemo_mapShanmenFormationProductHost::TryResetCoverage(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation)
{
	if (!IsValid())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::HostInvalid,
			TEXT("Coverage reset requires one valid product host."));
	}
	if (RequestedCorrelation != Session.GetCorrelation())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::CorrelationMismatch,
			TEXT("Coverage reset rejected a stale or foreign Run correlation."));
	}
	if (Session.IsTerminal())
	{
		return RejectCoverage(
			Edemo_mapShanmenFormationHostCoverageStatus::SessionTerminal,
			TEXT("Terminal formation teardown already owns coverage-baseline cleanup."));
	}

	Fdemo_mapShanmenFormationHostCoverageResult Result;
	Result.TrackerReset = CoverageTracker.Reset();
	Result.Status = Result.TrackerReset.PreviousBaselineReceiptId.IsValid()
		? Edemo_mapShanmenFormationHostCoverageStatus::Reset
		: Edemo_mapShanmenFormationHostCoverageStatus::ResetReplayed;
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenFormationHostCoverageStatus::Reset
		? TEXT("The host explicitly cleared its coverage baseline.")
		: TEXT("Coverage reset replayed with no retained baseline.");
	return Result;
}

bool Fdemo_mapShanmenFormationProductHost::IsValid() const
{
	if (!Session.IsValid() || !WorldAdapter.IsValid()
		|| !CoverageTracker.IsConsistent()
		|| (WorldAdapter.IsTeardownComplete() && CoverageTracker.IsPrimed()))
	{
		return false;
	}
	if (!bHasPendingPlacement)
	{
		return !PendingPlacement.IsValid() && !bPlacementBindingFrozen
			&& !BoundPlacementWorld.IsValid() && !BoundPlacementClass
			&& BoundPlacementClassPath.IsEmpty();
	}

	Fdemo_mapShanmenFormationAnchorPlacementIntent Expected;
	if (!PendingPlacement.IsValid()
		|| !Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
			Session, PendingPlacement.AnchorDefinitionId, Expected)
		|| !IntentsMatch(PendingPlacement, Expected)
		|| WorldAdapter.FindReceipt(PendingPlacement.PlacementId))
	{
		return false;
	}
	if (!bPlacementBindingFrozen)
	{
		return !BoundPlacementWorld.IsValid() && !BoundPlacementClass
			&& BoundPlacementClassPath.IsEmpty();
	}
	return BoundPlacementWorld.IsValid() && BoundPlacementClass
		&& !BoundPlacementClassPath.IsEmpty()
		&& BoundPlacementClass->GetPathName() == BoundPlacementClassPath;
}

void Fdemo_mapShanmenFormationProductHost::ClearPendingPlacement()
{
	bHasPendingPlacement = false;
	bPlacementBindingFrozen = false;
	PendingPlacement =
		Fdemo_mapShanmenFormationAnchorPlacementIntent();
	BoundPlacementWorld.Reset();
	BoundPlacementClass = nullptr;
	BoundPlacementClassPath.Reset();
}

bool Fdemo_mapShanmenFormationProductHost::PendingMatches(
	const FName AnchorDefinitionId,
	const FGuid& AttemptId) const
{
	return bHasPendingPlacement
		&& PendingPlacement.AnchorDefinitionId == AnchorDefinitionId
		&& PendingPlacement.AttemptId == AttemptId;
}
