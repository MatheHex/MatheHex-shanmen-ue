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
		&& !WeaponActor.IsValid();
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
}
