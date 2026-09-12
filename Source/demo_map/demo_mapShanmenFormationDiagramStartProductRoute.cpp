#include "demo_mapShanmenFormationDiagramStartProductRoute.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenDivineSenseProductController.h"
#include "demo_mapShanmenFormationRunLifecycle.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	using EProductStatus =
		Edemo_mapShanmenFormationDiagramStartProductRouteStatus;
	using ECompositionStatus =
		Edemo_mapShanmenFormationDiagramStartInputCompositionStatus;
	using EInputStatus = Edemo_mapShanmenFormationStartInputStatus;

	bool IsDependencyRejection(const EProductStatus Status)
	{
		switch (Status)
		{
		case EProductStatus::CoordinatorUnavailable:
		case EProductStatus::LifecycleUnavailable:
		case EProductStatus::LifecycleRunMismatch:
		case EProductStatus::SpiritEnergyUnavailable:
		case EProductStatus::SpiritEnergyRunMismatch:
		case EProductStatus::SpiritEnergyOwnerMismatch:
			return true;
		default:
			return false;
		}
	}

	bool HasNoAccessLifecycleUnavailableProof(
		const Fdemo_mapShanmenFormationDiagramStartInputCompositionResult&
			Composition)
	{
		return Composition.IsValid()
			&& Composition.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& Composition.GetAccessInvocationCount() == 0
			&& Composition.GetSpatialSampleCaptureCount() == 0
			&& Composition.GetInput().Status
				== EInputStatus::LifecycleUnavailable
			&& Composition.GetInput().SampleCount == 0
			&& Composition.GetInput().LifecycleInvocationCount == 0;
	}

	bool HasExactActiveBinding(
		const FGuid& RunId,
		const FGuid& OwnerId,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenFormationRunLifecycle& Lifecycle,
		const Fdemo_mapShanmenDivineSenseProductController&
			SpiritEnergyController)
	{
		return Coordinator.IsReady()
			&& Coordinator.GetRunId() == RunId
			&& Coordinator.GetPlayerEntityId() == OwnerId
			&& Lifecycle.IsActive()
			&& Lifecycle.IsValid()
			&& Lifecycle.GetRunId() == RunId
			&& SpiritEnergyController.IsActive()
			&& SpiritEnergyController.IsValid()
			&& SpiritEnergyController.GetRunId() == RunId
			&& SpiritEnergyController.GetSourceEntityId() == OwnerId;
	}
}

bool Fdemo_mapShanmenFormationDiagramStartProductRouteResult::IsValid()
	const
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
			&& !RunId.IsValid() && !OwnerId.IsValid()
			&& !LifecycleRunId.IsValid()
			&& !SpiritEnergyRunId.IsValid()
			&& !SpiritEnergyOwnerId.IsValid()
			&& Composition.IsValid()
			&& Composition.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& Composition.GetInput().Status
				== EInputStatus::GameplayBlocked
			&& Composition.GetAccessInvocationCount() == 0
			&& Composition.GetInput().LifecycleInvocationCount == 0;
	}

	if (IsDependencyRejection(Status))
	{
		if (DependencyValidationCount != 1
			|| bStateDesynchronized
			|| !HasNoAccessLifecycleUnavailableProof(Composition))
		{
			return false;
		}

		switch (Status)
		{
		case EProductStatus::CoordinatorUnavailable:
			return true;
		case EProductStatus::LifecycleUnavailable:
			return RunId.IsValid() && OwnerId.IsValid();
		case EProductStatus::LifecycleRunMismatch:
			return RunId.IsValid() && OwnerId.IsValid()
				&& LifecycleRunId.IsValid()
				&& LifecycleRunId != RunId;
		case EProductStatus::SpiritEnergyUnavailable:
			return RunId.IsValid() && OwnerId.IsValid()
				&& LifecycleRunId == RunId;
		case EProductStatus::SpiritEnergyRunMismatch:
			return RunId.IsValid() && OwnerId.IsValid()
				&& LifecycleRunId == RunId
				&& SpiritEnergyRunId.IsValid()
				&& SpiritEnergyRunId != RunId;
		case EProductStatus::SpiritEnergyOwnerMismatch:
			return RunId.IsValid() && OwnerId.IsValid()
				&& LifecycleRunId == RunId
				&& SpiritEnergyRunId == RunId
				&& SpiritEnergyOwnerId.IsValid()
				&& SpiritEnergyOwnerId != OwnerId;
		default:
			return false;
		}
	}

	if (Status == EProductStatus::StateDesynchronized)
	{
		return DependencyValidationCount == 1 && bStateDesynchronized;
	}

	if (Status != EProductStatus::Routed
		|| DependencyValidationCount != 1
		|| bStateDesynchronized
		|| !RunId.IsValid() || !OwnerId.IsValid()
		|| LifecycleRunId != RunId
		|| SpiritEnergyRunId != RunId
		|| SpiritEnergyOwnerId != OwnerId
		|| !Composition.IsValid())
	{
		return false;
	}

	const Fdemo_mapShanmenFormationStartInputResult& Input =
		Composition.GetInput();
	return Input.RunId == RunId && Input.OwnerId == OwnerId;
}

bool Fdemo_mapShanmenFormationDiagramStartProductRouteResult::IsAccepted()
	const
{
	return IsValid()
		&& Status == EProductStatus::Routed
		&& Composition.IsAccepted();
}

Fdemo_mapShanmenFormationDiagramStartProductRouteResult
Fdemo_mapShanmenFormationDiagramStartProductRoute::RouteStart(
	const bool bGameplayInputAllowed,
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenFormationRunLifecycle& Lifecycle,
	Fdemo_mapShanmenDivineSenseProductController& SpiritEnergyController,
	const FGuid& InputEventId,
	const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
	const FName RequestedDiagramDefinitionId,
	const FVector& Origin,
	const FVector& Forward,
	FReadAuthority ReadAuthority)
{
	Fdemo_mapShanmenFormationDiagramStartProductRouteResult Result;
	auto RouteLifecycle = [&Authority, &Coordinator, &Lifecycle,
		&SpiritEnergyController](const Fdemo_mapShanmenFormationIntent& Intent)
	{
		return Lifecycle.TrySubmit(
			Authority,
			Coordinator,
			SpiritEnergyController,
			Intent);
	};

	if (!bGameplayInputAllowed)
	{
		Result.Composition =
			Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
				false,
				false,
				FGuid(),
				FGuid(),
				InputEventId,
				Catalog,
				RequestedDiagramDefinitionId,
				Origin,
				Forward,
				ReadAuthority,
				RouteLifecycle);
		if (Result.Composition.IsValid()
			&& Result.Composition.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& Result.Composition.GetInput().Status
				== EInputStatus::GameplayBlocked)
		{
			Result.Status = EProductStatus::GameplayBlocked;
			Result.Diagnostic = Result.Composition.GetDiagnostic();
		}
		else
		{
			Result.Status = EProductStatus::StateDesynchronized;
			Result.DependencyValidationCount = 1;
			Result.bStateDesynchronized = true;
			Result.Diagnostic =
				TEXT("Formation start product route could not preserve the gameplay-first input proof.");
		}
		return Result;
	}

	Result.DependencyValidationCount = 1;
	Result.RunId = Coordinator.GetRunId();
	Result.OwnerId = Coordinator.GetPlayerEntityId();
	Result.LifecycleRunId = Lifecycle.GetRunId();
	Result.SpiritEnergyRunId = SpiritEnergyController.GetRunId();
	Result.SpiritEnergyOwnerId =
		SpiritEnergyController.GetSourceEntityId();

	if (!Coordinator.IsReady())
	{
		Result.Status = EProductStatus::CoordinatorUnavailable;
		Result.Diagnostic =
			TEXT("Formation start product route requires one ready Combat Run coordinator.");
	}
	else if (!Lifecycle.IsActive() || !Lifecycle.IsValid())
	{
		Result.Status = EProductStatus::LifecycleUnavailable;
		Result.Diagnostic =
			TEXT("Formation start product route requires one active valid formation Run lifecycle.");
	}
	else if (Result.LifecycleRunId != Result.RunId)
	{
		Result.Status = EProductStatus::LifecycleRunMismatch;
		Result.Diagnostic =
			TEXT("Formation lifecycle and Combat Run do not share one Run identity.");
	}
	else if (!SpiritEnergyController.IsActive()
		|| !SpiritEnergyController.IsValid())
	{
		Result.Status = EProductStatus::SpiritEnergyUnavailable;
		Result.Diagnostic =
			TEXT("Formation start product route requires one active valid shared SpiritEnergy controller.");
	}
	else if (Result.SpiritEnergyRunId != Result.RunId)
	{
		Result.Status = EProductStatus::SpiritEnergyRunMismatch;
		Result.Diagnostic =
			TEXT("Shared SpiritEnergy controller and Combat Run do not share one Run identity.");
	}
	else if (Result.SpiritEnergyOwnerId != Result.OwnerId)
	{
		Result.Status = EProductStatus::SpiritEnergyOwnerMismatch;
		Result.Diagnostic =
			TEXT("Shared SpiritEnergy controller is not bound to the Combat Run player.");
	}

	if (IsDependencyRejection(Result.Status))
	{
		Result.Composition =
			Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
				true,
				false,
				Result.RunId,
				Result.OwnerId,
				InputEventId,
				Catalog,
				RequestedDiagramDefinitionId,
				Origin,
				Forward,
				ReadAuthority,
				RouteLifecycle);
		if (!HasNoAccessLifecycleUnavailableProof(Result.Composition))
		{
			Result.Status = EProductStatus::StateDesynchronized;
			Result.bStateDesynchronized = true;
			Result.Diagnostic =
				TEXT("Formation start product route dependency rejection produced inconsistent input evidence.");
		}
		return Result;
	}

	Result.Composition =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true,
			true,
			Result.RunId,
			Result.OwnerId,
			InputEventId,
			Catalog,
			RequestedDiagramDefinitionId,
			Origin,
			Forward,
			ReadAuthority,
			RouteLifecycle);

	if (!Result.Composition.IsValid()
		|| !HasExactActiveBinding(
			Result.RunId,
			Result.OwnerId,
			Coordinator,
			Lifecycle,
			SpiritEnergyController))
	{
		Result.Status = EProductStatus::StateDesynchronized;
		Result.bStateDesynchronized = true;
		Result.Diagnostic =
			TEXT("Formation start product dependencies changed or returned invalid evidence during routing.");
		return Result;
	}

	Result.Status = EProductStatus::Routed;
	Result.Diagnostic = Result.Composition.GetDiagnostic();
	return Result;
}
