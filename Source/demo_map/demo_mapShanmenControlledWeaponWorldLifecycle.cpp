#include "demo_mapShanmenControlledWeaponWorldLifecycle.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapShanmenControlledWeaponActor.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	Fdemo_mapShanmenControlledWeaponWorldStartResult RejectWorldStart(
		const Edemo_mapShanmenControlledWeaponWorldStartStatus Status,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenControlledWeaponWorldStartResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponActiveRunIntent MakeTrainingSwordIntent(
		const FGuid& ItemInstanceId,
		const uint64 ActivationSequence)
	{
		Fdemo_mapShanmenControlledWeaponActiveRunIntent Intent;
		Intent.SourceItemInstanceId = ItemInstanceId;
		Intent.ActivationSequence = ActivationSequence;
		Intent.Definition.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		Intent.Definition.DetectorId =
			TEXT("Detector.ControlledWeapon.TrainingFlyingSword");
		Intent.Definition.FormulaId =
			TEXT("Combat.Formula.ControlledWeapon.TrainingFlyingSword.r1");
		Intent.Definition.BaseDamage = 10.0f;
		Intent.Definition.ControlPowerCoefficient = 0.25f;
		Intent.Definition.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Intent.Definition.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Intent.Definition.bRejectSelf = true;
		Intent.ControlPower = 40.0f;
		Intent.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Intent.Motion.DirectedSpeed = 400.0f;
		Intent.Motion.OrbitCenterOffset = FVector(0.0f, 0.0f, 50.0f);
		Intent.Motion.OrbitPlaneNormal = FVector::UpVector;
		Intent.Motion.OrbitReferenceAxis = FVector::ForwardVector;
		Intent.Motion.OrbitRadius = 100.0f;
		Intent.Motion.OrbitAngularSpeedRadiansPerSecond = UE_PI * 0.5f;
		Intent.Motion.InitialOrbitPhaseRadians = 0.0f;
		Intent.Motion.MaximumStepSeconds = 0.5f;
		return Intent;
	}
}

bool Fdemo_mapShanmenControlledWeaponWorldStartResult::IsStarted() const
{
	Ademo_mapShanmenControlledWeaponActor* Actor = WeaponActor.Get();
	return Status
			== Edemo_mapShanmenControlledWeaponWorldStartStatus::Started
		&& RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ::IsValid(Actor)
		&& Actor->IsProductBoundTo(
			RunId, ItemInstanceId, Actor->GetSourceActor())
		&& Actor->GetCollisionComponent()
		&& Actor->GetCollisionComponent()->GetCollisionEnabled()
			== ECollisionEnabled::QueryOnly
		&& Route.IsStarted()
		&& Route.Preparation.Action.GetRunId() == RunId
		&& Route.Preparation.Evidence.ItemInstanceId == ItemInstanceId;
}

bool Fdemo_mapShanmenControlledWeaponWorldRedeployResult::IsRedeployed() const
{
	Ademo_mapShanmenControlledWeaponActor* Actor = WeaponActor.Get();
	return Status
			== Edemo_mapShanmenControlledWeaponWorldRedeployStatus::Redeployed
		&& RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& PreviousActivationId.IsValid()
		&& NewActivationId.IsValid()
		&& PreviousActivationId != NewActivationId
		&& ActivationSequence > 0
		&& ::IsValid(Actor)
		&& Actor->IsProductBoundTo(
			RunId, ItemInstanceId, Actor->GetSourceActor())
		&& Route.IsStarted()
		&& Route.Preparation.Action.GetRunId() == RunId
		&& Route.Preparation.Evidence.ItemInstanceId == ItemInstanceId
		&& Route.Attachment.ActivationId == NewActivationId
		&& !Diagnostic.IsEmpty();
}

Fdemo_mapShanmenControlledWeaponWorldStartResult
Fdemo_mapShanmenControlledWeaponWorldLifecycle::TryBegin(
	UWorld* World,
	const Udemo_mapShanmenItemAuthoritySubsystem* Authority,
	const Udemo_mapItemSubsystem* Runtime,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenControlledWeaponRunHost& Host,
	AActor* SourceActor,
	const uint64 ActivationSequence)
{
	if (!IsValid())
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::LifecycleInvalid,
			TEXT("Controlled-weapon World lifecycle contains invalid retained state."));
	}
	if (!IsEmpty())
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::LifecycleBusy,
			TEXT("Controlled-weapon World lifecycle already owns one Actor."));
	}
	if (!World || !Authority || !Runtime || !::IsValid(SourceActor)
		|| SourceActor->GetWorld() != World)
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				DependenciesUnavailable,
			TEXT("Controlled-weapon World start requires co-located World, item authorities, and source Actor."));
	}
	if (!Coordinator.IsReady())
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				CoordinatorNotReady,
			TEXT("Controlled-weapon World start requires the canonical combat Run."));
	}
	if (ActivationSequence == 0 || ActivationSequence == MAX_uint64)
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				ActivationSequenceUnavailable,
			TEXT("Controlled-weapon World start requires a usable non-terminal activation sequence."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FString CorrelationDiagnostic;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			*Authority, Correlation, &CorrelationDiagnostic))
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				CorrelationUnavailable,
			CorrelationDiagnostic.IsEmpty()
				? TEXT("Durable active-Run correlation is unavailable.")
				: CorrelationDiagnostic);
	}
	if (!Correlation.IsValid()
		|| Correlation.ActiveRunId != Coordinator.GetRunId()
		|| Runtime->GetRunState() != Edemo_mapRunState::Active
		|| Runtime->GetActiveRunId() != Correlation.ActiveRunId)
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				CorrelationMismatch,
			TEXT("Durable, Runtime, and combat Run identities do not match."));
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority->TryCaptureSnapshot(Snapshot))
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				EvidenceUnavailable,
			TEXT("Ready item authority did not expose its immutable evidence."));
	}
	const FShanmenItemInstance* WeaponItem =
		Snapshot.Items.FindByPredicate([&Correlation](
			const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId
				== Correlation.WeaponItemInstanceId;
		});
	if (!WeaponItem
		|| !WeaponItem->ItemInstanceId.IsValid()
		|| WeaponItem->State != EShanmenItemInstanceState::Deployed)
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				WeaponEvidenceInvalid,
			TEXT("Prepared weapon identity is absent or no longer deployed."));
	}
	if (WeaponItem->DefinitionId != Fdemo_mapItemIds::TrainingFlyingSword)
	{
		Fdemo_mapShanmenControlledWeaponWorldStartResult Result;
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldStartStatus::NotApplicable;
		Result.RunId = Correlation.ActiveRunId;
		Result.ItemInstanceId = WeaponItem->ItemInstanceId;
		Result.Diagnostic =
			TEXT("Prepared weapon is not the canonical training flying sword; no World carrier was created.");
		return Result;
	}

	const Fdemo_mapShanmenControlledWeaponActiveRunIntent Intent =
		MakeTrainingSwordIntent(
			WeaponItem->ItemInstanceId, ActivationSequence);
	const FVector InitialLocation =
		SourceActor->GetActorLocation()
		+ Intent.Motion.OrbitCenterOffset
		+ Intent.Motion.OrbitReferenceAxis.GetSafeNormal()
			* Intent.Motion.OrbitRadius;
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.Owner = SourceActor;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapShanmenControlledWeaponActor* Spawned =
		World->SpawnActor<Ademo_mapShanmenControlledWeaponActor>(
			Ademo_mapShanmenControlledWeaponActor::StaticClass(),
			FTransform(SourceActor->GetActorRotation(), InitialLocation),
			SpawnParameters);
	if (!Spawned)
	{
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::SpawnRejected,
			TEXT("World rejected the canonical training flying-sword Actor."));
	}
	if (!Spawned->TryBindProductIdentity(
			Correlation.ActiveRunId,
			WeaponItem->ItemInstanceId,
			SourceActor))
	{
		Spawned->Destroy();
		return RejectWorldStart(
			Edemo_mapShanmenControlledWeaponWorldStartStatus::
				ActorBindingRejected,
			TEXT("Spawned flying-sword Actor rejected exact Run/item identity."));
	}

	Fdemo_mapShanmenControlledWeaponWorldStartResult Result;
	Result.RunId = Correlation.ActiveRunId;
	Result.ItemInstanceId = WeaponItem->ItemInstanceId;
	Result.WeaponActor = Spawned;
	Result.Route =
		Fdemo_mapShanmenControlledWeaponActiveRunRoute::TryStart(
			Authority,
			Runtime,
			Coordinator,
			Host,
			SourceActor,
			Spawned,
			Spawned->GetCollisionComponent(),
			Intent);
	if (!Result.Route.IsStarted())
	{
		Spawned->Destroy();
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldStartStatus::RouteRejected;
		Result.Diagnostic = Result.Route.Diagnostic.IsEmpty()
			? TEXT("Canonical P21.1 active-Run route rejected the World Actor.")
			: Result.Route.Diagnostic;
		return Result;
	}

	bOwnsSpawnedActor = true;
	RunId = Correlation.ActiveRunId;
	ItemInstanceId = WeaponItem->ItemInstanceId;
	WeaponActor = Spawned;
	NextActivationSequence = ActivationSequence + 1;
	Spawned->ActivateProductCollision();
	Result.Status =
		Edemo_mapShanmenControlledWeaponWorldStartStatus::Started;
	Result.Diagnostic =
		TEXT("Canonical training flying sword entered the World and P6 Host for this combat Run.");
	if (!IsValid() || !Result.IsStarted())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldStartStatus::ResultInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon World publication failed final validation.");
	}
	return Result;
}

Fdemo_mapShanmenControlledWeaponWorldRedeployResult
Fdemo_mapShanmenControlledWeaponWorldLifecycle::TryRedeployReturned(
	const Udemo_mapShanmenItemAuthoritySubsystem* Authority,
	const Udemo_mapItemSubsystem* Runtime,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenControlledWeaponRunHost& Host)
{
	Fdemo_mapShanmenControlledWeaponWorldRedeployResult Result;
	Result.RunId = RunId;
	Result.ItemInstanceId = ItemInstanceId;
	Result.ActivationSequence = NextActivationSequence;
	Result.WeaponActor = WeaponActor;
	if (!IsValid() || IsEmpty())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::
				LifecycleInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon redeployment requires one valid retained World Actor.");
		return Result;
	}
	if (!Authority || !Runtime || !Coordinator.IsReady())
	{
		Result.Diagnostic =
			TEXT("Controlled-weapon redeployment requires current item authorities and Combat Run.");
		return Result;
	}
	if (NextActivationSequence == 0
		|| NextActivationSequence == MAX_uint64)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::
				ActivationSequenceUnavailable;
		Result.Diagnostic =
			TEXT("Controlled-weapon redeployment exhausted its activation sequence domain.");
		return Result;
	}
	if (!Host.IsValid()
		|| Host.GetRunId() != RunId
		|| Host.GetSourceEntityId() != Coordinator.GetPlayerEntityId())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::HostMismatch;
		Result.Diagnostic =
			TEXT("Controlled-weapon Host does not match the retained World lifecycle.");
		return Result;
	}

	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Host.FindController(ItemInstanceId);
	Ademo_mapShanmenControlledWeaponActor* Actor = WeaponActor.Get();
	if (!Controller
		|| Controller->GetWeaponActor() != Actor
		|| Controller->GetSourceActor() != Actor->GetSourceActor()
		|| !Controller->IsCompletedForReturn()
		|| !Controller->IsAtInitialOrbitLocation())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::
				ReturnNotReady;
		Result.Diagnostic =
			TEXT("Exact completed weapon has not reached its canonical return anchor.");
		return Result;
	}
	Result.PreviousActivationId = Controller->GetSession()
		.GetActionRuntime().GetAction().GetActivationId();

	Fdemo_mapShanmenControlledWeaponRunHost HostCandidate = Host;
	if (!HostCandidate.TryRemoveTerminal(ItemInstanceId))
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::
				TerminalRemovalRejected;
		Result.Diagnostic =
			TEXT("Returned terminal activation could not be retired from the candidate Host.");
		return Result;
	}

	const Fdemo_mapShanmenControlledWeaponActiveRunIntent Intent =
		MakeTrainingSwordIntent(ItemInstanceId, NextActivationSequence);
	Result.Route =
		Fdemo_mapShanmenControlledWeaponActiveRunRoute::TryStart(
			Authority,
			Runtime,
			Coordinator,
			HostCandidate,
			Actor->GetSourceActor(),
			Actor,
			Actor->GetCollisionComponent(),
			Intent);
	if (!Result.Route.IsStarted())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::RouteRejected;
		Result.Diagnostic = Result.Route.Diagnostic.IsEmpty()
			? TEXT("Authority-backed active-Run route rejected the returned Actor.")
			: Result.Route.Diagnostic;
		return Result;
	}
	Result.NewActivationId = Result.Route.Attachment.ActivationId;
	Result.Status =
		Edemo_mapShanmenControlledWeaponWorldRedeployStatus::Redeployed;
	Result.Diagnostic =
		TEXT("Returned flying sword reused its exact item and Actor under a new activation.");
	if (!HostCandidate.IsValid() || !Result.IsRedeployed())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::ResultInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon redeployment failed final publication validation.");
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponWorldLifecycle LifecycleCandidate = *this;
	++LifecycleCandidate.NextActivationSequence;
	if (!LifecycleCandidate.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldRedeployStatus::ResultInvalid;
		Result.Diagnostic =
			TEXT("Controlled-weapon lifecycle rejected the next activation checkpoint.");
		return Result;
	}
	Actor->ActivateProductCollision();
	Host = MoveTemp(HostCandidate);
	*this = MoveTemp(LifecycleCandidate);
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponWorldLifecycle::TryEndAfterRun(
	const FGuid& ExpectedRunId,
	const Fdemo_mapShanmenControlledWeaponRunHost& Host,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsEmpty())
	{
		OutDiagnostic =
			TEXT("Controlled-weapon World lifecycle was already empty.");
		return true;
	}
	if (!IsValid()
		|| !ExpectedRunId.IsValid()
		|| ExpectedRunId != RunId)
	{
		OutDiagnostic =
			TEXT("Controlled-weapon World retirement rejected invalid or mismatched lifecycle identity.");
		return false;
	}
	if (!Host.IsEmpty())
	{
		OutDiagnostic =
			TEXT("Physical flying-sword retirement requires completed logical Host teardown.");
		return false;
	}

	Ademo_mapShanmenControlledWeaponActor* Actor = WeaponActor.Get();
	if (!Actor || Actor->IsActorBeingDestroyed())
	{
		OutDiagnostic =
			TEXT("Owned flying-sword Actor disappeared before lifecycle retirement.");
		return false;
	}
	Actor->DeactivateProductCollision();
	if (!Actor->Destroy())
	{
		OutDiagnostic =
			TEXT("World rejected physical flying-sword Actor retirement.");
		return false;
	}
	Clear();
	OutDiagnostic =
		TEXT("Physical flying-sword carrier retired after logical Run teardown.");
	return true;
}

bool Fdemo_mapShanmenControlledWeaponWorldLifecycle::IsValid() const
{
	if (IsEmpty())
	{
		return true;
	}
	Ademo_mapShanmenControlledWeaponActor* Actor = WeaponActor.Get();
	return bOwnsSpawnedActor
		&& RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& NextActivationSequence > 0
		&& ::IsValid(Actor)
		&& Actor->IsProductBoundTo(
			RunId, ItemInstanceId, Actor->GetSourceActor())
		&& Actor->GetCollisionComponent()
		&& Actor->GetCollisionComponent()->GetCollisionEnabled()
			== ECollisionEnabled::QueryOnly;
}

bool Fdemo_mapShanmenControlledWeaponWorldLifecycle::IsEmpty() const
{
	return !bOwnsSpawnedActor
		&& !RunId.IsValid()
		&& !ItemInstanceId.IsValid()
		&& !WeaponActor.IsValid()
		&& NextActivationSequence == 0;
}

bool Fdemo_mapShanmenControlledWeaponWorldLifecycle::IsActive() const
{
	return !IsEmpty() && IsValid();
}

void Fdemo_mapShanmenControlledWeaponWorldLifecycle::Reset()
{
	if (Ademo_mapShanmenControlledWeaponActor* Actor = WeaponActor.Get())
	{
		if (!Actor->IsActorBeingDestroyed())
		{
			Actor->DeactivateProductCollision();
			Actor->Destroy();
		}
	}
	Clear();
}

void Fdemo_mapShanmenControlledWeaponWorldLifecycle::Clear()
{
	bOwnsSpawnedActor = false;
	RunId.Invalidate();
	ItemInstanceId.Invalidate();
	WeaponActor.Reset();
	NextActivationSequence = 0;
}
