#include "demo_mapShanmenDivineSenseProductRoute.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	using ERouteStatus =
		Edemo_mapShanmenDivineSenseProductRouteStatus;

	Fdemo_mapShanmenDivineSenseProductRouteResult Reject(
		ERouteStatus Status,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenDivineSenseProductRouteResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic.IsEmpty()
			? TEXT("Divine Sense product route rejected the request.")
			: Diagnostic;
		return Result;
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool ValidateControllerBinding(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductRouteResult& OutResult)
	{
		if (!Controller.IsActive())
		{
			OutResult.Status = ERouteStatus::ControllerUnavailable;
			OutResult.Diagnostic =
				TEXT("Divine Sense product route requires one active Controller.");
			return false;
		}
		if (!Controller.IsValid()
			|| !Fdemo_mapShanmenDivineSenseProductAuthority::
				IsCanonicalConfig(Controller.GetConfig()))
		{
			OutResult.Status = ERouteStatus::StateDesynchronized;
			OutResult.Diagnostic =
				TEXT("Active Divine Sense Controller is not bound to canonical product state.");
			return false;
		}
		if (!Coordinator.IsReady())
		{
			OutResult.Status = ERouteStatus::CoordinatorUnavailable;
			OutResult.Diagnostic =
				TEXT("Divine Sense product route requires one ready Combat Run.");
			return false;
		}
		if (Coordinator.GetRunId() != Controller.GetRunId()
			|| Coordinator.GetPlayerEntityId()
				!= Controller.GetSourceEntityId())
		{
			OutResult.Status = ERouteStatus::RunMismatch;
			OutResult.Diagnostic =
				TEXT("Divine Sense Controller and Combat Run do not share one player binding.");
			return false;
		}
		return true;
	}

	bool ValidateLiveInputs(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const TArray<AActor*>& SubjectActors,
		FString& OutDiagnostic)
	{
		if (!::IsValid(World))
		{
			OutDiagnostic =
				TEXT("Divine Sense use requires one live caller-owned World.");
			return false;
		}
		if (!::IsValid(SourceActor)
			|| SourceActor->IsActorBeingDestroyed())
		{
			OutDiagnostic =
				TEXT("Divine Sense use requires one live source Actor.");
			return false;
		}
		if (SourceActor->GetWorld() != World)
		{
			OutDiagnostic =
				TEXT("Divine Sense source Actor must belong to the supplied World.");
			return false;
		}
		if (!IsFiniteVector(SourceActor->GetActorLocation()))
		{
			OutDiagnostic =
				TEXT("Divine Sense source Actor location must be finite.");
			return false;
		}

		const FShanmenWorldEntityRegistry& Registry =
			Coordinator.GetEntityRegistry();
		FGuid ResolvedSourceEntityId;
		if (Registry.GetRunId() != Controller.GetRunId()
			|| !Registry.TryResolveObject(
				Controller.GetRunId(),
				SourceActor,
				INDEX_NONE,
				ResolvedSourceEntityId)
			|| ResolvedSourceEntityId != Controller.GetSourceEntityId())
		{
			OutDiagnostic =
				TEXT("Divine Sense source Actor is not the active Run player entity.");
			return false;
		}

		const int32 SubjectBudget =
			Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalSubjectActorBudget();
		if (SubjectActors.Num() > SubjectBudget)
		{
			OutDiagnostic =
				TEXT("Divine Sense explicit subject batch exceeds canonical product budget.");
			return false;
		}

		TSet<FGuid> SubjectEntityIds;
		SubjectEntityIds.Reserve(SubjectActors.Num());
		for (AActor* SubjectActor : SubjectActors)
		{
			if (!::IsValid(SubjectActor)
				|| SubjectActor->IsActorBeingDestroyed())
			{
				OutDiagnostic =
					TEXT("Every Divine Sense subject Actor must remain live.");
				return false;
			}
			if (SubjectActor->GetWorld() != World)
			{
				OutDiagnostic =
					TEXT("One Divine Sense use cannot mix Actors from another World.");
				return false;
			}
			if (!IsFiniteVector(SubjectActor->GetActorLocation()))
			{
				OutDiagnostic =
					TEXT("Every Divine Sense subject Actor location must be finite.");
				return false;
			}

			FGuid SubjectEntityId;
			if (!Registry.TryResolveObject(
					Controller.GetRunId(),
					SubjectActor,
					INDEX_NONE,
					SubjectEntityId))
			{
				OutDiagnostic =
					TEXT("Every Divine Sense subject Actor requires a current Run registry binding.");
				return false;
			}
			if (Controller.GetConfig().GetDefinition().RejectsSelf()
				&& SubjectEntityId == ResolvedSourceEntityId)
			{
				OutDiagnostic =
					TEXT("Canonical Divine Sense use rejects the source entity from its subject batch.");
				return false;
			}
			if (SubjectEntityIds.Contains(SubjectEntityId))
			{
				OutDiagnostic =
					TEXT("Divine Sense subject batch contains a duplicate stable entity.");
				return false;
			}
			SubjectEntityIds.Add(SubjectEntityId);
		}
		return true;
	}

	bool AttemptMatchesController(
		const Fdemo_mapShanmenDivineSenseProductUseAttempt& Attempt,
		const Fdemo_mapShanmenDivineSenseProductController& Controller)
	{
		return Attempt.IsValid()
			&& Controller.IsActive()
			&& Controller.IsValid()
			&& Attempt.GetControllerId() == Controller.GetControllerId()
			&& Attempt.GetRunId() == Controller.GetRunId()
			&& Attempt.GetSourceEntityId() == Controller.GetSourceEntityId()
			&& Attempt.GetConfigId()
				== Controller.GetConfig().GetConfigId()
			&& Attempt.GetPrepared().Config.Matches(
				Controller.GetConfig());
	}

	void ClassifyControllerResult(
		Fdemo_mapShanmenDivineSenseProductRouteResult& Result)
	{
		if (!Result.ControllerResult.IsValid())
		{
			Result.Status = ERouteStatus::StateDesynchronized;
			Result.Diagnostic =
				TEXT("Divine Sense Controller returned invalid route evidence.");
			return;
		}
		Result.Diagnostic = Result.ControllerResult.Diagnostic;
		if (!Result.ControllerResult.IsAccepted())
		{
			Result.Status = ERouteStatus::ControllerRejected;
			return;
		}
		Result.Status = Result.ControllerResult.IsReplay()
			? ERouteStatus::Replayed
			: ERouteStatus::Applied;
	}
}

bool Fdemo_mapShanmenDivineSenseProductUseAttempt::IsValid() const
{
	return ControllerId.IsValid() && RunId.IsValid()
		&& SourceEntityId.IsValid() && ConfigId.IsValid()
		&& Prepared.IsReady()
		&& Fdemo_mapShanmenDivineSenseProductAuthority::IsCanonicalConfig(
			Prepared.Config)
		&& Prepared.Config.GetConfigId() == ConfigId
		&& Prepared.Reservation.GetConfigId() == ConfigId
		&& Prepared.Reservation.GetAction().GetRunId() == RunId
		&& Prepared.Reservation.GetAction().GetOwnerId() == SourceEntityId
		&& Prepared.Reservation.GetAction().GetSourceEntityId()
			== SourceEntityId
		&& Prepared.Intent.GetRunId() == RunId
		&& Prepared.Intent.GetAction().GetOwnerId() == SourceEntityId
		&& Prepared.Intent.GetAction().GetSourceEntityId()
			== SourceEntityId;
}

bool Fdemo_mapShanmenDivineSenseProductUseAttempt::Matches(
	const Fdemo_mapShanmenDivineSenseProductUseAttempt& Other) const
{
	return IsValid() && Other.IsValid()
		&& ControllerId == Other.ControllerId
		&& RunId == Other.RunId
		&& SourceEntityId == Other.SourceEntityId
		&& ConfigId == Other.ConfigId
		&& Prepared.Config.Matches(Other.Prepared.Config)
		&& Prepared.Reservation.GetActivationSequence()
			== Other.Prepared.Reservation.GetActivationSequence()
		&& Prepared.Reservation.GetActivationId()
			== Other.Prepared.Reservation.GetActivationId()
		&& Prepared.Intent.Matches(Other.Prepared.Intent);
}

bool Fdemo_mapShanmenDivineSenseProductRouteResult::IsValid() const
{
	if (Status == ERouteStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}

	if (Status == ERouteStatus::Applied || Status == ERouteStatus::Replayed)
	{
		return Attempt.IsValid()
			&& ControllerResult.IsAccepted()
			&& ControllerResult.ControllerId == Attempt.GetControllerId()
			&& ControllerResult.RunId == Attempt.GetRunId()
			&& ControllerResult.Intent.Matches(Attempt.GetIntent())
			&& ControllerResult.IsReplay()
				== (Status == ERouteStatus::Replayed);
	}
	if (Status == ERouteStatus::ControllerRejected)
	{
		return Attempt.IsValid()
			&& ControllerResult.IsValid()
			&& !ControllerResult.IsAccepted()
			&& ControllerResult.ControllerId == Attempt.GetControllerId()
			&& ControllerResult.RunId == Attempt.GetRunId();
	}
	if (Status == ERouteStatus::AvailabilityRejected)
	{
		return AvailabilityBefore.IsValid()
			&& !AvailabilityBefore.CanCaptureNewIntent()
			&& !Attempt.IsValid();
	}
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductRouteResult::IsAccepted() const
{
	return IsValid()
		&& (Status == ERouteStatus::Applied
			|| Status == ERouteStatus::Replayed);
}

bool Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
	Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenActionResourceSnapshot& OpeningSpiritEnergy,
	FString& OutDiagnostic)
{
	Fdemo_mapShanmenDivineSenseProductConfig Config;
	if (!Fdemo_mapShanmenDivineSenseProductAuthority::
		TryCreateCanonicalConfig(Config))
	{
		OutDiagnostic =
			TEXT("Divine Sense product route could not create canonical config.");
		return false;
	}
	return Controller.TryBegin(
		Coordinator, OpeningSpiritEnergy, Config, OutDiagnostic);
}

Fdemo_mapShanmenDivineSenseProductRouteResult
Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
	Fdemo_mapShanmenDivineSenseProductController& Controller,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	Fdemo_mapShanmenDivineSenseProductRouteResult Result;
	if (!ValidateControllerBinding(Controller, Coordinator, Result))
	{
		return Result;
	}

	FString AvailabilityDiagnostic;
	if (!Controller.TryCaptureAvailability(
			Coordinator,
			Result.AvailabilityBefore,
			AvailabilityDiagnostic))
	{
		Result.Status = ERouteStatus::StateDesynchronized;
		Result.Diagnostic = AvailabilityDiagnostic.IsEmpty()
			? TEXT("Divine Sense route could not capture Controller availability.")
			: AvailabilityDiagnostic;
		return Result;
	}
	if (!Result.AvailabilityBefore.CanCaptureNewIntent())
	{
		Result.Status = ERouteStatus::AvailabilityRejected;
		Result.Diagnostic =
			TEXT("Divine Sense Controller lacks SpiritEnergy or new-Intent capacity.");
		return Result;
	}

	FString LiveDiagnostic;
	if (!ValidateLiveInputs(
			Controller,
			Coordinator,
			World,
			SourceActor,
			SubjectActors,
			LiveDiagnostic))
	{
		Result.Status = ERouteStatus::LiveInputRejected;
		Result.Diagnostic = MoveTemp(LiveDiagnostic);
		return Result;
	}

	const Fdemo_mapShanmenDivineSenseProductPrepareResult Prepared =
		Fdemo_mapShanmenDivineSenseProductAuthority::PrepareIntent(
			Coordinator);
	if (!Prepared.IsReady())
	{
		Result.Status = ERouteStatus::ProductPrepareRejected;
		Result.Diagnostic = Prepared.Diagnostic.IsEmpty()
			? TEXT("Divine Sense product authority rejected use preparation.")
			: Prepared.Diagnostic;
		return Result;
	}

	Result.Attempt.ControllerId = Controller.GetControllerId();
	Result.Attempt.RunId = Controller.GetRunId();
	Result.Attempt.SourceEntityId = Controller.GetSourceEntityId();
	Result.Attempt.ConfigId = Controller.GetConfig().GetConfigId();
	Result.Attempt.Prepared = Prepared;
	if (!AttemptMatchesController(Result.Attempt, Controller))
	{
		Result.Status = ERouteStatus::StateDesynchronized;
		Result.Diagnostic =
			TEXT("Prepared Divine Sense attempt does not match the active Controller.");
		return Result;
	}

	Result.ControllerResult = Controller.TrySubmit(
		Coordinator,
		World,
		SourceActor,
		Result.Attempt.GetIntent(),
		SubjectActors,
		EvidenceProvider);
	ClassifyControllerResult(Result);
	return Result;
}

Fdemo_mapShanmenDivineSenseProductRouteResult
Fdemo_mapShanmenDivineSenseProductRoute::TryRetry(
	Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const Fdemo_mapShanmenDivineSenseProductUseAttempt& Attempt,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	Fdemo_mapShanmenDivineSenseProductRouteResult Result;
	Result.bReusedAttempt = true;
	Result.Attempt = Attempt;
	if (!Attempt.IsValid())
	{
		Result.Status = ERouteStatus::AttemptRejected;
		Result.Diagnostic =
			TEXT("Divine Sense retry requires one valid route-issued attempt.");
		return Result;
	}
	if (!ValidateControllerBinding(Controller, Coordinator, Result))
	{
		return Result;
	}
	if (!AttemptMatchesController(Attempt, Controller))
	{
		Result.Status = ERouteStatus::AttemptRejected;
		Result.Diagnostic =
			TEXT("Divine Sense retry attempt belongs to another Controller or Run.");
		return Result;
	}

	Result.ControllerResult = Controller.TrySubmit(
		Coordinator,
		World,
		SourceActor,
		Attempt.GetIntent(),
		SubjectActors,
		EvidenceProvider);
	ClassifyControllerResult(Result);
	return Result;
}

Fdemo_mapShanmenDivineSenseProductControllerEndResult
Fdemo_mapShanmenDivineSenseProductRoute::TryEnd(
	Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator)
{
	return Controller.TryEnd(Coordinator, Controller.GetControllerId());
}
