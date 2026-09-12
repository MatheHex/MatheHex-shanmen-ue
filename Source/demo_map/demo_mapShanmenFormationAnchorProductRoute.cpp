#include "demo_mapShanmenFormationAnchorProductRoute.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenFormationRunLifecycle.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	using EProductStatus =
		Edemo_mapShanmenFormationAnchorProductRouteStatus;
	using EInputStatus = Edemo_mapShanmenFormationAnchorInputStatus;

	bool IsDependencyRejection(const EProductStatus Status)
	{
		switch (Status)
		{
		case EProductStatus::CoordinatorUnavailable:
		case EProductStatus::LifecycleUnavailable:
		case EProductStatus::LifecycleRunMismatch:
		case EProductStatus::WorldUnavailable:
		case EProductStatus::ActorClassUnavailable:
			return true;
		default:
			return false;
		}
	}

	bool HasNoSampleLifecycleUnavailableProof(
		const Fdemo_mapShanmenFormationAnchorInputResult& Input)
	{
		return Input.IsValid()
			&& Input.Status == EInputStatus::LifecycleUnavailable
			&& Input.SampleCount == 0
			&& Input.LifecycleInvocationCount == 0;
	}

	bool IsWorldAvailable(UWorld* World)
	{
		return ::IsValid(World) && !World->bIsTearingDown;
	}

	bool TryGetConcreteActorClassPath(
		const TSubclassOf<AActor> ActorClass,
		FString& OutActorClassPath)
	{
		OutActorClassPath.Reset();
		UClass* RawClass = ActorClass.Get();
		if (!RawClass
			|| !RawClass->IsChildOf(AActor::StaticClass())
			|| RawClass->HasAnyClassFlags(
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			return false;
		}
		OutActorClassPath = RawClass->GetPathName();
		return !OutActorClassPath.IsEmpty();
	}

	bool HasExactActiveBinding(
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		const FString& ActorClassPath,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenFormationRunLifecycle& Lifecycle,
		UWorld* World,
		const TSubclassOf<AActor> ActorClass)
	{
		FString CurrentActorClassPath;
		return Coordinator.IsReady()
			&& Coordinator.GetRunId() == RunId
			&& Coordinator.GetPlayerEntityId() == PlayerEntityId
			&& Lifecycle.IsActive()
			&& Lifecycle.IsValid()
			&& Lifecycle.GetRunId() == RunId
			&& IsWorldAvailable(World)
			&& TryGetConcreteActorClassPath(
				ActorClass, CurrentActorClassPath)
			&& CurrentActorClassPath == ActorClassPath;
	}
}

bool Fdemo_mapShanmenFormationAnchorProductRouteResult::IsValid() const
{
	if (Status == EProductStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| DependencyValidationCount < 0
		|| DependencyValidationCount > 1)
	{
		return false;
	}

	if (Status == EProductStatus::GameplayBlocked)
	{
		return DependencyValidationCount == 0
			&& !bStateDesynchronized
			&& !RunId.IsValid()
			&& !PlayerEntityId.IsValid()
			&& !LifecycleRunId.IsValid()
			&& ActorClassPath.IsEmpty()
			&& Input.IsValid()
			&& Input.Status == EInputStatus::GameplayBlocked
			&& Input.SampleCount == 0
			&& Input.LifecycleInvocationCount == 0;
	}

	if (IsDependencyRejection(Status))
	{
		if (DependencyValidationCount != 1
			|| bStateDesynchronized
			|| !HasNoSampleLifecycleUnavailableProof(Input)
			|| !ActorClassPath.IsEmpty())
		{
			return false;
		}

		switch (Status)
		{
		case EProductStatus::CoordinatorUnavailable:
			return true;
		case EProductStatus::LifecycleUnavailable:
			return RunId.IsValid() && PlayerEntityId.IsValid();
		case EProductStatus::LifecycleRunMismatch:
			return RunId.IsValid() && PlayerEntityId.IsValid()
				&& LifecycleRunId.IsValid()
				&& LifecycleRunId != RunId;
		case EProductStatus::WorldUnavailable:
		case EProductStatus::ActorClassUnavailable:
			return RunId.IsValid() && PlayerEntityId.IsValid()
				&& LifecycleRunId == RunId;
		default:
			return false;
		}
	}

	if (Status == EProductStatus::StateDesynchronized)
	{
		return bStateDesynchronized;
	}

	return Status == EProductStatus::Routed
		&& DependencyValidationCount == 1
		&& !bStateDesynchronized
		&& RunId.IsValid()
		&& PlayerEntityId.IsValid()
		&& LifecycleRunId == RunId
		&& !ActorClassPath.IsEmpty()
		&& Input.IsValid()
		&& Input.RunId == RunId;
}

bool Fdemo_mapShanmenFormationAnchorProductRouteResult::IsAccepted() const
{
	return IsValid()
		&& Status == EProductStatus::Routed
		&& Input.IsAccepted();
}

Fdemo_mapShanmenFormationAnchorProductRouteResult
Fdemo_mapShanmenFormationAnchorProductRoute::RouteAnchor(
	const bool bGameplayInputAllowed,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenFormationRunLifecycle& Lifecycle,
	UWorld* World,
	const TSubclassOf<AActor> ActorClass,
	const FGuid& InputEventId,
	const FName RequestedAnchorDefinitionId)
{
	Fdemo_mapShanmenFormationAnchorProductRouteResult Result;
	auto SampleAnchor = [RequestedAnchorDefinitionId]()
	{
		Fdemo_mapShanmenFormationAnchorInputSample Sample;
		Fdemo_mapShanmenFormationAnchorInputSample::TryCapture(
			RequestedAnchorDefinitionId, Sample);
		return Sample;
	};
	auto RouteLifecycle = [&Authority, &Coordinator, &Lifecycle, World,
		ActorClass](const Fdemo_mapShanmenFormationAnchorOperation& Operation)
	{
		return Lifecycle.TryExecuteAnchorOperation(
			Authority,
			Coordinator,
			World,
			ActorClass,
			Operation);
	};

	if (!bGameplayInputAllowed)
	{
		Result.Input = Fdemo_mapShanmenFormationInputAdapter::RouteAnchorInput(
			false,
			false,
			FGuid(),
			InputEventId,
			SampleAnchor,
			RouteLifecycle);
		if (Result.Input.IsValid()
			&& Result.Input.Status == EInputStatus::GameplayBlocked)
		{
			Result.Status = EProductStatus::GameplayBlocked;
			Result.Diagnostic = Result.Input.Diagnostic;
		}
		else
		{
			Result.Status = EProductStatus::StateDesynchronized;
			Result.bStateDesynchronized = true;
			Result.Diagnostic =
				TEXT("Formation anchor product route could not preserve the gameplay-first input proof.");
		}
		return Result;
	}

	Result.DependencyValidationCount = 1;
	Result.RunId = Coordinator.GetRunId();
	Result.PlayerEntityId = Coordinator.GetPlayerEntityId();
	Result.LifecycleRunId = Lifecycle.GetRunId();
	if (!Coordinator.IsReady())
	{
		Result.Status = EProductStatus::CoordinatorUnavailable;
		Result.Diagnostic =
			TEXT("Formation anchor product route requires one ready Combat Run coordinator.");
	}
	else if (!Lifecycle.IsActive() || !Lifecycle.IsValid())
	{
		Result.Status = EProductStatus::LifecycleUnavailable;
		Result.Diagnostic =
			TEXT("Formation anchor product route requires one active valid formation Run lifecycle.");
	}
	else if (Result.LifecycleRunId != Result.RunId)
	{
		Result.Status = EProductStatus::LifecycleRunMismatch;
		Result.Diagnostic =
			TEXT("Formation lifecycle and Combat Run do not share one Run identity.");
	}
	else if (!IsWorldAvailable(World))
	{
		Result.Status = EProductStatus::WorldUnavailable;
		Result.Diagnostic =
			TEXT("Formation anchor product route requires one live explicit World.");
	}
	else if (!TryGetConcreteActorClassPath(
		ActorClass, Result.ActorClassPath))
	{
		Result.Status = EProductStatus::ActorClassUnavailable;
		Result.Diagnostic =
			TEXT("Formation anchor product route requires one concrete explicit Actor class.");
	}

	if (IsDependencyRejection(Result.Status))
	{
		Result.ActorClassPath.Reset();
		Result.Input = Fdemo_mapShanmenFormationInputAdapter::RouteAnchorInput(
			true,
			false,
			Result.RunId,
			InputEventId,
			SampleAnchor,
			RouteLifecycle);
		if (!HasNoSampleLifecycleUnavailableProof(Result.Input))
		{
			Result.Status = EProductStatus::StateDesynchronized;
			Result.bStateDesynchronized = true;
			Result.Diagnostic =
				TEXT("Formation anchor product dependency rejection produced inconsistent input evidence.");
		}
		return Result;
	}

	Result.Input = Fdemo_mapShanmenFormationInputAdapter::RouteAnchorInput(
		true,
		true,
		Result.RunId,
		InputEventId,
		SampleAnchor,
		RouteLifecycle);
	if (!Result.Input.IsValid()
		|| !HasExactActiveBinding(
			Result.RunId,
			Result.PlayerEntityId,
			Result.ActorClassPath,
			Coordinator,
			Lifecycle,
			World,
			ActorClass))
	{
		Result.Status = EProductStatus::StateDesynchronized;
		Result.bStateDesynchronized = true;
		Result.Diagnostic =
			TEXT("Formation anchor product dependencies changed or returned invalid evidence during routing.");
		return Result;
	}

	Result.Status = EProductStatus::Routed;
	Result.Diagnostic = Result.Input.Diagnostic;
	return Result;
}
