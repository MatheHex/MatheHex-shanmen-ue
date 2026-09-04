#include "demo_mapShanmenDivineSenseLogicalInputAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	using EAvailabilityState =
		Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState;
	using EInputStatus = Edemo_mapShanmenDivineSenseLogicalInputStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	Fdemo_mapShanmenDivineSenseLogicalInputResult Reject(
		const EInputStatus Status,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenDivineSenseLogicalInputResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic.IsEmpty()
			? TEXT("Divine Sense logical input rejected the request.")
			: Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAvailability::IsValid() const
{
	bool bShapeValid = false;
	switch (State)
	{
	case EAvailabilityState::Inactive:
		bShapeValid = !ControllerId.IsValid() && !RunId.IsValid()
			&& !ProductAvailability.IsValid() && !PendingAttempt.IsValid();
		break;
	case EAvailabilityState::UseReady:
		bShapeValid = ControllerId.IsValid() && RunId.IsValid()
			&& ProductAvailability.IsValid()
			&& ProductAvailability.GetControllerId() == ControllerId
			&& ProductAvailability.GetSessionAvailability().GetRunId() == RunId
			&& ProductAvailability.CanCaptureNewIntent()
			&& !PendingAttempt.IsValid();
		break;
	case EAvailabilityState::ProductUnavailable:
		bShapeValid = ControllerId.IsValid() && RunId.IsValid()
			&& ProductAvailability.IsValid()
			&& ProductAvailability.GetControllerId() == ControllerId
			&& ProductAvailability.GetSessionAvailability().GetRunId() == RunId
			&& !ProductAvailability.CanCaptureNewIntent()
			&& !PendingAttempt.IsValid();
		break;
	case EAvailabilityState::RetryRequired:
		bShapeValid = ControllerId.IsValid() && RunId.IsValid()
			&& ProductAvailability.IsValid()
			&& ProductAvailability.GetControllerId() == ControllerId
			&& ProductAvailability.GetSessionAvailability().GetRunId() == RunId
			&& PendingAttempt.IsValid()
			&& PendingAttempt.GetControllerId() == ControllerId
			&& PendingAttempt.GetRunId() == RunId;
		break;
	default:
		return false;
	}

	return bShapeValid
		&& ProjectionId
			== Fdemo_mapShanmenDivineSenseLogicalInputAdapter::
				MakeAvailabilityId(*this);
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAvailability::Matches(
	const Fdemo_mapShanmenDivineSenseLogicalInputAvailability& Other) const
{
	if (!IsValid() || !Other.IsValid()
		|| ProjectionId != Other.ProjectionId
		|| State != Other.State
		|| ControllerId != Other.ControllerId
		|| RunId != Other.RunId)
	{
		return false;
	}
	if (State == EAvailabilityState::Inactive)
	{
		return true;
	}
	if (!ProductAvailability.Matches(Other.ProductAvailability))
	{
		return false;
	}
	return State != EAvailabilityState::RetryRequired
		|| PendingAttempt.Matches(Other.PendingAttempt);
}

bool Fdemo_mapShanmenDivineSenseLogicalInputResult::IsValid() const
{
	if (Status == EInputStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}

	switch (Status)
	{
	case EInputStatus::Applied:
	case EInputStatus::Replayed:
		return bProductRouteInvoked && !bPendingRetryStored
			&& AvailabilityBefore.IsValid()
			&& (bRetryAttempt
				? AvailabilityBefore.CanRetry()
				: AvailabilityBefore.CanUse())
			&& AvailabilityAfter.IsValid()
			&& ProductRoute.IsAccepted()
			&& ProductRoute.IsReplay()
				== (Status == EInputStatus::Replayed);
	case EInputStatus::Busy:
		return !bProductRouteInvoked && !bRetryAttempt
			&& bPendingRetryStored
			&& AvailabilityBefore.CanRetry()
			&& AvailabilityAfter.Matches(AvailabilityBefore);
	case EInputStatus::ProductUnavailable:
		return !bProductRouteInvoked && !bRetryAttempt
			&& !bPendingRetryStored
			&& AvailabilityBefore.IsValid()
			&& !AvailabilityBefore.CanUse()
			&& !AvailabilityBefore.CanRetry()
			&& AvailabilityAfter.Matches(AvailabilityBefore);
	case EInputStatus::RetryUnavailable:
		return !bProductRouteInvoked && bRetryAttempt
			&& !bPendingRetryStored
			&& AvailabilityBefore.IsValid()
			&& !AvailabilityBefore.CanRetry()
			&& AvailabilityAfter.Matches(AvailabilityBefore);
	case EInputStatus::RetryRequired:
		return bProductRouteInvoked && bPendingRetryStored
			&& AvailabilityBefore.IsValid()
			&& AvailabilityAfter.CanRetry()
			&& ProductRoute.IsValid()
			&& !ProductRoute.IsAccepted()
			&& ProductRoute.Status
				== Edemo_mapShanmenDivineSenseProductRouteStatus::
					ControllerRejected
			&& ProductRoute.HasAttempt()
			&& AvailabilityAfter.GetPendingAttempt()
			&& AvailabilityAfter.GetPendingAttempt()->Matches(
				ProductRoute.Attempt);
	case EInputStatus::ProductRejected:
		return bProductRouteInvoked
			&& AvailabilityBefore.IsValid()
			&& AvailabilityAfter.IsValid()
			&& ProductRoute.IsValid()
			&& !ProductRoute.IsAccepted()
			&& (!bPendingRetryStored || AvailabilityAfter.CanRetry());
	case EInputStatus::AdapterInactive:
	case EInputStatus::AdapterInvalid:
	case EInputStatus::BindingMismatch:
		return !bProductRouteInvoked;
	case EInputStatus::StateDesynchronized:
		return true;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenDivineSenseLogicalInputResult::IsAccepted() const
{
	return IsValid()
		&& (Status == EInputStatus::Applied
			|| Status == EInputStatus::Replayed);
}

bool Fdemo_mapShanmenDivineSenseLogicalInputCancellation::IsValid() const
{
	return ControllerId.IsValid() && RunId.IsValid() && Attempt.IsValid()
		&& Attempt.GetControllerId() == ControllerId
		&& Attempt.GetRunId() == RunId;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputEndSummary::IsValid() const
{
	return ControllerId.IsValid() && RunId.IsValid()
		&& (!PendingAttempt.IsValid()
			|| (PendingAttempt.GetControllerId() == ControllerId
				&& PendingAttempt.GetRunId() == RunId));
}

FGuid Fdemo_mapShanmenDivineSenseLogicalInputAdapter::MakeAvailabilityId(
	const Fdemo_mapShanmenDivineSenseLogicalInputAvailability& Availability)
{
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.Spell.DivineSense.LogicalInputAvailability.r1")),
		{
			FString::FromInt(static_cast<int32>(Availability.State)),
			GuidDigits(Availability.ControllerId),
			GuidDigits(Availability.RunId),
			GuidDigits(
				Availability.ProductAvailability.GetAvailabilityId()),
			GuidDigits(
				Availability.PendingAttempt.GetIntent().GetIntentId()),
			GuidDigits(
				Availability.PendingAttempt.GetIntent().GetAction().
					GetActivationId())
		});
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::IsValid() const
{
	if (!bActive)
	{
		return IsEmpty();
	}
	return ControllerId.IsValid() && RunId.IsValid()
		&& (!PendingAttempt.IsValid()
			|| (PendingAttempt.GetControllerId() == ControllerId
				&& PendingAttempt.GetRunId() == RunId));
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::ValidateBinding(
	const Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FString& OutDiagnostic) const
{
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input adapter is inactive.");
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input adapter invariants are invalid.");
		return false;
	}
	if (!Controller.IsActive() || !Controller.IsValid()
		|| !Fdemo_mapShanmenDivineSenseProductAuthority::
			IsCanonicalConfig(Controller.GetConfig()))
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input requires one active canonical Controller.");
		return false;
	}
	if (!Coordinator.IsReady())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input requires one ready Combat Run.");
		return false;
	}
	if (Controller.GetControllerId() != ControllerId
		|| Controller.GetRunId() != RunId
		|| Coordinator.GetRunId() != RunId
		|| Coordinator.GetPlayerEntityId() != Controller.GetSourceEntityId())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input cannot cross its Controller or Run binding.");
		return false;
	}
	if (PendingAttempt.IsValid()
		&& (PendingAttempt.GetConfigId()
				!= Controller.GetConfig().GetConfigId()
			|| PendingAttempt.GetSourceEntityId()
				!= Controller.GetSourceEntityId()))
	{
		OutDiagnostic =
			TEXT("Retained Divine Sense attempt no longer matches product binding.");
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::TryBuildAvailability(
	const Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability& OutAvailability,
	FString& OutDiagnostic) const
{
	OutAvailability =
		Fdemo_mapShanmenDivineSenseLogicalInputAvailability();
	OutDiagnostic.Reset();

	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Candidate;
	if (!IsActive())
	{
		if (!IsValid() || !IsEmpty())
		{
			OutDiagnostic =
				TEXT("Inactive Divine Sense input state is not canonical.");
			return false;
		}
		Candidate.State = EAvailabilityState::Inactive;
		Candidate.ProjectionId = MakeAvailabilityId(Candidate);
		if (!Candidate.IsValid())
		{
			OutDiagnostic =
				TEXT("Inactive Divine Sense input projection failed closed.");
			return false;
		}
		OutAvailability = MoveTemp(Candidate);
		OutDiagnostic = TEXT("Divine Sense logical input adapter is inactive.");
		return true;
	}

	if (!ValidateBinding(Controller, Coordinator, OutDiagnostic))
	{
		return false;
	}
	if (!Controller.TryCaptureAvailability(
			Coordinator,
			Candidate.ProductAvailability,
			OutDiagnostic))
	{
		return false;
	}
	Candidate.ControllerId = ControllerId;
	Candidate.RunId = RunId;
	Candidate.PendingAttempt = PendingAttempt;
	Candidate.State = HasPendingRetry()
		? EAvailabilityState::RetryRequired
		: Candidate.ProductAvailability.CanCaptureNewIntent()
			? EAvailabilityState::UseReady
			: EAvailabilityState::ProductUnavailable;
	Candidate.ProjectionId = MakeAvailabilityId(Candidate);
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input projection failed closed.");
		return false;
	}

	OutDiagnostic = Candidate.CanRetry()
		? TEXT("Divine Sense logical input requires explicit retry or cancel.")
		: Candidate.CanUse()
			? TEXT("Divine Sense logical input can issue one new use.")
			: TEXT("Divine Sense product availability rejects a new use.");
	OutAvailability = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::TryBegin(
	const Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (IsActive())
	{
		if (Controller.GetControllerId() == ControllerId
			&& Coordinator.GetRunId() == RunId
			&& ValidateBinding(Controller, Coordinator, OutDiagnostic))
		{
			OutDiagnostic =
				TEXT("Divine Sense logical input is already bound to this product Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("Active Divine Sense logical input cannot change product binding.");
		return false;
	}
	if (!IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input requires canonical empty state.");
		return false;
	}
	if (!Controller.IsActive() || !Controller.IsValid()
		|| !Fdemo_mapShanmenDivineSenseProductAuthority::
			IsCanonicalConfig(Controller.GetConfig())
		|| !Coordinator.IsReady()
		|| Controller.GetRunId() != Coordinator.GetRunId()
		|| Controller.GetSourceEntityId()
			!= Coordinator.GetPlayerEntityId())
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input requires one matching canonical product Run.");
		return false;
	}

	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Candidate;
	Candidate.bActive = true;
	Candidate.ControllerId = Controller.GetControllerId();
	Candidate.RunId = Controller.GetRunId();
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Availability;
	if (!Candidate.IsValid()
		|| !Candidate.TryBuildAvailability(
			Controller, Coordinator, Availability, OutDiagnostic))
	{
		OutDiagnostic =
			TEXT("Divine Sense logical input failed closed during Run binding.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic =
		TEXT("Divine Sense logical input bound to the canonical product Run.");
	return true;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::
IsRetryableRejection(
	const Fdemo_mapShanmenDivineSenseProductRouteResult& Result)
{
	return Result.IsValid() && !Result.IsAccepted() && Result.HasAttempt()
		&& Result.Status
			== Edemo_mapShanmenDivineSenseProductRouteStatus::
				ControllerRejected
		&& Result.ControllerResult.Status
			== Edemo_mapShanmenDivineSenseProductControllerStatus::
				RouteRejected;
}

Fdemo_mapShanmenDivineSenseLogicalInputResult
Fdemo_mapShanmenDivineSenseLogicalInputAdapter::TryUse(
	Fdemo_mapShanmenDivineSenseProductController& Controller,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!IsActive())
	{
		return Reject(
			EInputStatus::AdapterInactive,
			TEXT("Divine Sense logical use requires an active input adapter."));
	}
	if (!IsValid())
	{
		return Reject(
			EInputStatus::AdapterInvalid,
			TEXT("Divine Sense logical input adapter invariants are invalid."));
	}
	FString BindingDiagnostic;
	if (!ValidateBinding(Controller, Coordinator, BindingDiagnostic))
	{
		return Reject(EInputStatus::BindingMismatch, BindingDiagnostic);
	}

	Fdemo_mapShanmenDivineSenseLogicalInputResult Result;
	FString AvailabilityDiagnostic;
	if (!TryBuildAvailability(
			Controller,
			Coordinator,
			Result.AvailabilityBefore,
			AvailabilityDiagnostic))
	{
		return Reject(EInputStatus::StateDesynchronized,
			AvailabilityDiagnostic);
	}
	if (Result.AvailabilityBefore.CanRetry())
	{
		Result.Status = EInputStatus::Busy;
		Result.Diagnostic =
			TEXT("Divine Sense logical input is busy with one pending retry.");
		Result.bPendingRetryStored = true;
		Result.AvailabilityAfter = Result.AvailabilityBefore;
		return Result;
	}
	if (!Result.AvailabilityBefore.CanUse())
	{
		Result.Status = EInputStatus::ProductUnavailable;
		Result.Diagnostic = AvailabilityDiagnostic;
		Result.AvailabilityAfter = Result.AvailabilityBefore;
		return Result;
	}

	Result.bProductRouteInvoked = true;
	Result.ProductRoute = Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
		Controller,
		Coordinator,
		World,
		SourceActor,
		SubjectActors,
		EvidenceProvider);
	Result.Diagnostic = Result.ProductRoute.Diagnostic;
	if (Result.ProductRoute.IsAccepted())
	{
		Result.Status = Result.ProductRoute.IsReplay()
			? EInputStatus::Replayed
			: EInputStatus::Applied;
	}
	else if (IsRetryableRejection(Result.ProductRoute))
	{
		PendingAttempt = Result.ProductRoute.Attempt;
		Result.Status = EInputStatus::RetryRequired;
		Result.bPendingRetryStored = true;
	}
	else
	{
		Result.Status = EInputStatus::ProductRejected;
	}

	if (!TryBuildAvailability(
			Controller,
			Coordinator,
			Result.AvailabilityAfter,
			AvailabilityDiagnostic))
	{
		Result.Status = EInputStatus::StateDesynchronized;
		Result.Diagnostic = AvailabilityDiagnostic;
		return Result;
	}
	Result.bPendingRetryStored = HasPendingRetry();
	if (!Result.IsValid())
	{
		Result.Status = EInputStatus::StateDesynchronized;
		Result.Diagnostic =
			TEXT("Divine Sense logical use produced inconsistent evidence.");
	}
	return Result;
}

Fdemo_mapShanmenDivineSenseLogicalInputResult
Fdemo_mapShanmenDivineSenseLogicalInputAdapter::TryRetry(
	Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!IsActive())
	{
		Fdemo_mapShanmenDivineSenseLogicalInputResult Result = Reject(
			EInputStatus::AdapterInactive,
			TEXT("Divine Sense retry requires an active input adapter."));
		Result.bRetryAttempt = true;
		return Result;
	}
	if (!IsValid())
	{
		Fdemo_mapShanmenDivineSenseLogicalInputResult Result = Reject(
			EInputStatus::AdapterInvalid,
			TEXT("Divine Sense logical input adapter invariants are invalid."));
		Result.bRetryAttempt = true;
		return Result;
	}
	FString BindingDiagnostic;
	if (!ValidateBinding(Controller, Coordinator, BindingDiagnostic))
	{
		Fdemo_mapShanmenDivineSenseLogicalInputResult Result = Reject(
			EInputStatus::BindingMismatch, BindingDiagnostic);
		Result.bRetryAttempt = true;
		return Result;
	}

	Fdemo_mapShanmenDivineSenseLogicalInputResult Result;
	Result.bRetryAttempt = true;
	FString AvailabilityDiagnostic;
	if (!TryBuildAvailability(
			Controller,
			Coordinator,
			Result.AvailabilityBefore,
			AvailabilityDiagnostic))
	{
		Result.Status = EInputStatus::StateDesynchronized;
		Result.Diagnostic = AvailabilityDiagnostic;
		return Result;
	}
	if (!Result.AvailabilityBefore.CanRetry())
	{
		Result.Status = EInputStatus::RetryUnavailable;
		Result.Diagnostic =
			TEXT("No Divine Sense route-issued attempt is pending retry.");
		Result.AvailabilityAfter = Result.AvailabilityBefore;
		return Result;
	}

	const Fdemo_mapShanmenDivineSenseProductUseAttempt Attempt =
		PendingAttempt;
	Result.bProductRouteInvoked = true;
	Result.ProductRoute = Fdemo_mapShanmenDivineSenseProductRoute::TryRetry(
		Controller,
		Coordinator,
		World,
		SourceActor,
		Attempt,
		SubjectActors,
		EvidenceProvider);
	Result.Diagnostic = Result.ProductRoute.Diagnostic;
	if (Result.ProductRoute.IsAccepted())
	{
		PendingAttempt = Fdemo_mapShanmenDivineSenseProductUseAttempt();
		Result.Status = Result.ProductRoute.IsReplay()
			? EInputStatus::Replayed
			: EInputStatus::Applied;
	}
	else if (IsRetryableRejection(Result.ProductRoute)
		&& Result.ProductRoute.Attempt.Matches(Attempt))
	{
		Result.Status = EInputStatus::RetryRequired;
	}
	else
	{
		Result.Status = EInputStatus::ProductRejected;
	}

	if (!TryBuildAvailability(
			Controller,
			Coordinator,
			Result.AvailabilityAfter,
			AvailabilityDiagnostic))
	{
		Result.Status = EInputStatus::StateDesynchronized;
		Result.Diagnostic = AvailabilityDiagnostic;
		return Result;
	}
	Result.bPendingRetryStored = HasPendingRetry();
	if (!Result.IsValid())
	{
		Result.Status = EInputStatus::StateDesynchronized;
		Result.Diagnostic =
			TEXT("Divine Sense explicit retry produced inconsistent evidence.");
	}
	return Result;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::TryCancelPending(
	Fdemo_mapShanmenDivineSenseLogicalInputCancellation& OutCancellation,
	FString& OutDiagnostic)
{
	OutCancellation =
		Fdemo_mapShanmenDivineSenseLogicalInputCancellation();
	OutDiagnostic.Reset();
	if (!IsActive() || !IsValid())
	{
		OutDiagnostic =
			TEXT("Divine Sense cancellation requires one valid active input adapter.");
		return false;
	}
	if (!HasPendingRetry())
	{
		OutDiagnostic =
			TEXT("No Divine Sense route-issued attempt is pending cancellation.");
		return false;
	}

	Fdemo_mapShanmenDivineSenseLogicalInputCancellation Cancellation;
	Cancellation.ControllerId = ControllerId;
	Cancellation.RunId = RunId;
	Cancellation.Attempt = PendingAttempt;
	if (!Cancellation.IsValid())
	{
		OutDiagnostic =
			TEXT("Divine Sense pending attempt could not produce cancellation evidence.");
		return false;
	}

	PendingAttempt = Fdemo_mapShanmenDivineSenseProductUseAttempt();
	if (!IsValid())
	{
		PendingAttempt = Cancellation.Attempt;
		OutDiagnostic =
			TEXT("Divine Sense pending cancellation failed adapter postconditions.");
		return false;
	}
	OutCancellation = MoveTemp(Cancellation);
	OutDiagnostic =
		TEXT("Cancelled one pending Divine Sense route-issued attempt.");
	return true;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::
TryProjectAvailability(
	const Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability& OutAvailability,
	FString& OutDiagnostic) const
{
	return TryBuildAvailability(
		Controller, Coordinator, OutAvailability, OutDiagnostic);
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::TryEnd(
	const Fdemo_mapShanmenDivineSenseProductController& Controller,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseLogicalInputEndSummary& OutSummary,
	FString& OutDiagnostic)
{
	OutSummary = Fdemo_mapShanmenDivineSenseLogicalInputEndSummary();
	OutDiagnostic.Reset();
	if (!IsActive() || !IsValid())
	{
		OutDiagnostic =
			TEXT("Divine Sense input teardown requires one valid active adapter.");
		return false;
	}
	if (!ValidateBinding(Controller, Coordinator, OutDiagnostic))
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseLogicalInputEndSummary Summary;
	Summary.ControllerId = ControllerId;
	Summary.RunId = RunId;
	Summary.PendingAttempt = PendingAttempt;
	if (!Summary.IsValid())
	{
		OutDiagnostic =
			TEXT("Divine Sense input teardown could not produce valid evidence.");
		return false;
	}

	ControllerId = FGuid();
	RunId = FGuid();
	PendingAttempt = Fdemo_mapShanmenDivineSenseProductUseAttempt();
	bActive = false;
	if (!IsValid() || !IsEmpty())
	{
		OutDiagnostic =
			TEXT("Divine Sense input teardown failed adapter postconditions.");
		return false;
	}

	OutSummary = MoveTemp(Summary);
	OutDiagnostic =
		TEXT("Divine Sense logical input released its product Run binding.");
	return true;
}

bool Fdemo_mapShanmenDivineSenseLogicalInputAdapter::Reset()
{
	if (IsActive())
	{
		return false;
	}
	ControllerId = FGuid();
	RunId = FGuid();
	PendingAttempt = Fdemo_mapShanmenDivineSenseProductUseAttempt();
	bActive = false;
	return IsValid() && IsEmpty();
}
