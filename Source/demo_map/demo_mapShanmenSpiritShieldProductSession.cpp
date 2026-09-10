#include "demo_mapShanmenSpiritShieldProductSession.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	Fdemo_mapShanmenSpiritShieldProductActivationResult RejectActivation(
		const Edemo_mapShanmenSpiritShieldProductActivationError Error,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenPlayerActionGateResult& ActionGate =
			Fdemo_mapShanmenPlayerActionGateResult())
	{
		Fdemo_mapShanmenSpiritShieldProductActivationResult Result;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic ? Diagnostic : TEXT("Rejected");
		Result.ActionGate = ActionGate;
		return Result;
	}

	Fdemo_mapShanmenSpiritShieldProductTimelineResult RejectTimeline(
		const Edemo_mapShanmenSpiritShieldProductTimelineError Error,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSpiritShieldProductTimelineResult Result;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic ? Diagnostic : TEXT("Rejected");
		return Result;
	}
}

bool Fdemo_mapPlayerSpiritShieldActionReservation::IsValid() const
{
	FGameplayTagContainer ExpectedSourceTags;
	ExpectedSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	return ActivationSequence > 0
		&& Action.IsValid()
		&& Action.GetOwnerId() == Action.GetSourceEntityId()
		&& !Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId()
		&& Action.GetContent().Version
			== Fdemo_mapShanmenSpiritShieldProductAuthority::
				CanonicalContentVersion()
		&& Action.GetContent().Digest
			== Fdemo_mapShanmenSpiritShieldProductAuthority::
				CanonicalContentDigest()
		&& Action.GetSourceTags() == ExpectedSourceTags
		&& Action.GetActivationId() == FShanmenCombatIdFactory::MakeActivationId(
			Action.GetRunId(),
			Action.GetSourceEntityId(),
			Action.GetActionDefinitionId(),
			ActivationSequence);
}

FName Fdemo_mapShanmenSpiritShieldProductAuthority::CanonicalContentVersion()
{
	return TEXT("0.0.10.P25.1");
}

FString Fdemo_mapShanmenSpiritShieldProductAuthority::CanonicalContentDigest()
{
	return TEXT("Shanmen.SpiritShield.ProductSession.r1");
}

FName Fdemo_mapShanmenSpiritShieldProductAuthority::CanonicalRuleId()
{
	return TEXT("Defense.Spell.SpiritShield.Basic01");
}

FName Fdemo_mapShanmenSpiritShieldProductAuthority::CanonicalCostRuleId()
{
	return TEXT("Spell.SpiritShield.SpiritEnergy.Basic01");
}

float Fdemo_mapShanmenSpiritShieldProductAuthority::
	CanonicalMaximumCapacity()
{
	return 30.0f;
}

float Fdemo_mapShanmenSpiritShieldProductAuthority::
	CanonicalSpiritEnergyCost()
{
	return 20.0f;
}

int64 Fdemo_mapShanmenSpiritShieldProductAuthority::CanonicalDurationTicks()
{
	return 90;
}

bool Fdemo_mapShanmenSpiritShieldProductAuthority::
	TryCreateCanonicalDefinition(FShanmenSpiritShieldDefinition& OutDefinition)
{
	OutDefinition = FShanmenSpiritShieldDefinition();
	FShanmenSpiritShieldDefinitionCapture Capture;
	Capture.ActionDefinitionId =
		FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
	Capture.RuleId = CanonicalRuleId();
	Capture.MaximumCapacity = CanonicalMaximumCapacity();
	Capture.RequiredDamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysical());
	Capture.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	return FShanmenSpiritShieldDefinition::TryCapture(
		Capture, OutDefinition);
}

bool Fdemo_mapShanmenSpiritShieldProductAuthority::TryCreateCanonicalCost(
	FShanmenActionResourceCost& OutCost)
{
	OutCost = FShanmenActionResourceCost();
	FShanmenActionResourceCostCapture Capture;
	Capture.RuleId = CanonicalCostRuleId();
	Capture.ResourceChannel =
		FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
	Capture.Amount = CanonicalSpiritEnergyCost();
	return FShanmenActionResourceCost::TryCapture(Capture, OutCost);
}

FGuid Fdemo_mapShanmenSpiritShieldProductAuthority::MakeTransactionId(
	const Fdemo_mapPlayerSpiritShieldActionReservation& Reservation)
{
	return Reservation.IsValid()
		? FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.SpiritShield.SharedSpiritEnergyTransaction.r1"),
			{ GuidDigits(Reservation.GetActivationId()) })
		: FGuid();
}

FGuid Fdemo_mapShanmenSpiritShieldProductAuthority::MakeCommandId(
	const Fdemo_mapPlayerSpiritShieldActionReservation& Reservation)
{
	return Reservation.IsValid()
		? FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.SpiritShield.ActivationCommand.r1"),
			{ GuidDigits(Reservation.GetActivationId()) })
		: FGuid();
}

bool Fdemo_mapShanmenSpiritShieldProductActivationResult::IsValid() const
{
	if (Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenSpiritShieldProductActivationStatus::Rejected)
	{
		return Error
			!= Edemo_mapShanmenSpiritShieldProductActivationError::None;
	}
	return Error == Edemo_mapShanmenSpiritShieldProductActivationError::None
		&& ActionGate.IsAuthorized()
		&& Reservation.IsValid()
		&& Schedule.IsValid()
		&& Begin.IsSuccess()
		&& Begin.Status == EShanmenSpiritShieldActionStatus::Begun
		&& Commit.IsSuccess()
		&& Commit.Status == EShanmenSpiritShieldActionStatus::Activated
		&& SharedResource.IsSuccess();
}

bool Fdemo_mapShanmenSpiritShieldProductActivationResult::IsAccepted() const
{
	return IsValid()
		&& Status
			== Edemo_mapShanmenSpiritShieldProductActivationStatus::Activated;
}

bool Fdemo_mapShanmenSpiritShieldProductTimelineResult::IsValid() const
{
	if (Diagnostic.IsEmpty())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenSpiritShieldProductTimelineStatus::Rejected:
		return Error
			!= Edemo_mapShanmenSpiritShieldProductTimelineError::None;
	case Edemo_mapShanmenSpiritShieldProductTimelineStatus::Waiting:
		return Error == Edemo_mapShanmenSpiritShieldProductTimelineError::None
			&& Observation.IsValid() && !Closure.IsValid();
	case Edemo_mapShanmenSpiritShieldProductTimelineStatus::Closed:
	case Edemo_mapShanmenSpiritShieldProductTimelineStatus::AlreadyClosed:
		return Error == Edemo_mapShanmenSpiritShieldProductTimelineError::None
			&& Observation.IsValid() && Closure.IsSuccess();
	default:
		return false;
	}
}

bool Fdemo_mapShanmenSpiritShieldProductTimelineResult::IsSuccess() const
{
	return IsValid()
		&& Status
			!= Edemo_mapShanmenSpiritShieldProductTimelineStatus::Rejected;
}

Fdemo_mapShanmenSpiritShieldProductActivationResult
Fdemo_mapShanmenSpiritShieldProductSession::TryActivate(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseProductController& SpiritEnergyController,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
	TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()> AuthorizeAction)
{
	if (IsActive())
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::AlreadyActive,
			TEXT("An active Spirit Shield already owns the product session."));
	}
	if (!Coordinator.IsReady())
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::
				CoordinatorNotReady,
			TEXT("Spirit Shield requires one ready combat Run."));
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId()
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				Coordinator.GetRunId()))
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::
				TimelineUnavailable,
			TEXT("Spirit Shield requires the active Run fixed timeline."));
	}

	const Fdemo_mapShanmenPlayerActionGateResult ActionGate =
		AuthorizeAction();
	if (!ActionGate.IsAuthorized())
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::ActionConflict,
			TEXT("Spirit Shield could not acquire the player action lane."),
			ActionGate);
	}

	Fdemo_mapPlayerSpiritShieldActionReservation NewReservation;
	FString ReservationDiagnostic;
	if (!Coordinator.TryReservePlayerSpiritShieldAction(
			NewReservation, ReservationDiagnostic))
	{
		Fdemo_mapShanmenSpiritShieldProductActivationResult Result =
			RejectActivation(
				Edemo_mapShanmenSpiritShieldProductActivationError::
					ReservationRejected,
				*ReservationDiagnostic,
				ActionGate);
		return Result;
	}

	FShanmenSpiritShieldDefinition Definition;
	FShanmenActionResourceCost Cost;
	if (!Fdemo_mapShanmenSpiritShieldProductAuthority::
			TryCreateCanonicalDefinition(Definition)
		|| !Fdemo_mapShanmenSpiritShieldProductAuthority::
			TryCreateCanonicalCost(Cost))
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::
				PolicyConstructionFailed,
			TEXT("Canonical Spirit Shield policy failed closed."),
			ActionGate);
	}
	const int64 StartTick = TimelineSample.GetCurrentTick();
	const int64 Duration =
		Fdemo_mapShanmenSpiritShieldProductAuthority::CanonicalDurationTicks();
	if (StartTick > MAX_int64 - Duration)
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::ScheduleRejected,
			TEXT("Spirit Shield deadline would overflow the Run timeline."),
			ActionGate);
	}
	FShanmenSpiritShieldSchedule Schedule;
	if (!FShanmenSpiritShieldSchedule::TryCapture(
			TimelineSample.GetTimelineId(),
			StartTick,
			StartTick + Duration,
			Schedule))
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::ScheduleRejected,
			TEXT("Spirit Shield schedule capture failed closed."),
			ActionGate);
	}

	Fdemo_mapShanmenSpiritShieldProductSession Candidate;
	Candidate.Reservation = NewReservation;
	Fdemo_mapShanmenDivineSenseProductController ControllerCandidate =
		SpiritEnergyController;
	FShanmenSpiritShieldActionResult Begin;
	FShanmenSpiritShieldActionResult Commit;
	const FGuid TransactionId =
		Fdemo_mapShanmenSpiritShieldProductAuthority::MakeTransactionId(
			NewReservation);
	const FGuid CommandId =
		Fdemo_mapShanmenSpiritShieldProductAuthority::MakeCommandId(
			NewReservation);
	const Fdemo_mapShanmenSharedSpiritEnergyTransactionResult Shared =
		ControllerCandidate.ApplySharedSpiritEnergyTransaction(
			Coordinator,
			TransactionId,
			CommandId,
			[&](FShanmenActionResourceAuthority& Authority)
			{
				Begin = FShanmenSpiritShieldSession::Begin(
					NewReservation.GetAction(),
					Definition,
					Cost,
					Schedule,
					Authority,
					Candidate.Session);
				if (!Begin.IsSuccess())
				{
					return false;
				}
				Commit = Candidate.Session.Commit(Authority);
				return Commit.IsSuccess()
					&& Candidate.Session.IsValid()
					&& Candidate.Session.HasActivatedAuthorities();
			});
	if (!Shared.IsSuccess())
	{
		Fdemo_mapShanmenSpiritShieldProductActivationResult Result =
			RejectActivation(
				(Begin.IsValid() || Commit.IsValid())
					? Edemo_mapShanmenSpiritShieldProductActivationError::
						SessionRejected
					: Edemo_mapShanmenSpiritShieldProductActivationError::
						SharedResourceRejected,
				*Shared.Diagnostic,
				ActionGate);
		Result.Reservation = NewReservation;
		Result.Schedule = Schedule;
		Result.Begin = Begin;
		Result.Commit = Commit;
		Result.SharedResource = Shared;
		return Result;
	}

	Candidate.SharedResourceReceipt = Shared.Receipt;
	Candidate.bHasActivation = true;
	if (!Candidate.IsValid() || !ControllerCandidate.IsValid())
	{
		return RejectActivation(
			Edemo_mapShanmenSpiritShieldProductActivationError::
				StateDesynchronized,
			TEXT("Spirit Shield and shared SpiritEnergy proofs did not agree."),
			ActionGate);
	}

	SpiritEnergyController = MoveTemp(ControllerCandidate);
	*this = MoveTemp(Candidate);
	Fdemo_mapShanmenSpiritShieldProductActivationResult Result;
	Result.Status =
		Edemo_mapShanmenSpiritShieldProductActivationStatus::Activated;
	Result.Error = Edemo_mapShanmenSpiritShieldProductActivationError::None;
	Result.Diagnostic =
		TEXT("Spirit Shield activated from the shared Run SpiritEnergy ledger.");
	Result.ActionGate = ActionGate;
	Result.Reservation = Reservation;
	Result.Schedule = Session.GetSchedule();
	Result.Begin = Begin;
	Result.Commit = Commit;
	Result.SharedResource = Shared;
	return Result;
}

Fdemo_mapShanmenSpiritShieldProductTimelineResult
Fdemo_mapShanmenSpiritShieldProductSession::ObserveTimeline(
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	if (!bHasActivation || !Session.IsValid())
	{
		return RejectTimeline(
			Edemo_mapShanmenSpiritShieldProductTimelineError::SessionNotActive,
			TEXT("Spirit Shield has no initialized product session."));
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId()
			!= Session.GetSchedule().GetTimelineId())
	{
		return RejectTimeline(
			Edemo_mapShanmenSpiritShieldProductTimelineError::TimelineMismatch,
			TEXT("Spirit Shield rejected a foreign Run timeline sample."));
	}
	FShanmenSpiritShieldTimelineObservation Observation;
	if (!FShanmenSpiritShieldTimelineObservation::TryCapture(
			TimelineSample.GetTimelineId(),
			TimelineSample.GetCurrentTick(),
			Observation))
	{
		return RejectTimeline(
			Edemo_mapShanmenSpiritShieldProductTimelineError::
				ObservationRejected,
			TEXT("Spirit Shield timeline observation failed closed."));
	}

	Fdemo_mapShanmenSpiritShieldProductTimelineResult Result;
	Result.Error = Edemo_mapShanmenSpiritShieldProductTimelineError::None;
	Result.Observation = Observation;
	if (IsClosed())
	{
		Result.Closure = Session.ObserveDeadline(Observation);
		Result.Status =
			Edemo_mapShanmenSpiritShieldProductTimelineStatus::AlreadyClosed;
		Result.Diagnostic = TEXT("Spirit Shield deadline closure replayed.");
		return Result;
	}
	if (!IsActive())
	{
		return RejectTimeline(
			Edemo_mapShanmenSpiritShieldProductTimelineError::SessionNotActive,
			TEXT("Spirit Shield is not active on the Run timeline."));
	}
	if (TimelineSample.GetCurrentTick() < GetDeadlineTick())
	{
		Result.Status =
			Edemo_mapShanmenSpiritShieldProductTimelineStatus::Waiting;
		Result.Diagnostic = TEXT("Spirit Shield remains active before deadline.");
		return Result;
	}

	Result.Closure = Session.ObserveDeadline(Observation);
	if (!Result.Closure.IsSuccess())
	{
		return RejectTimeline(
			Edemo_mapShanmenSpiritShieldProductTimelineError::
				ObservationRejected,
			TEXT("Spirit Shield deadline owner rejected the due sample."));
	}
	Result.Status = Edemo_mapShanmenSpiritShieldProductTimelineStatus::Closed;
	Result.Diagnostic = TEXT("Spirit Shield closed at its fixed Run deadline.");
	if (!IsClosed() || !IsValid())
	{
		return RejectTimeline(
			Edemo_mapShanmenSpiritShieldProductTimelineError::
				StateDesynchronized,
			TEXT("Spirit Shield deadline closed nested state inconsistently."));
	}
	return Result;
}

bool Fdemo_mapShanmenSpiritShieldProductSession::TryReleaseOwner(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!bHasActivation)
	{
		OutDiagnostic = TEXT("Spirit Shield product session was already empty.");
		return true;
	}
	Fdemo_mapShanmenSpiritShieldProductSession Candidate = *this;
	if (Candidate.IsActive())
	{
		const FShanmenSpiritShieldActionResult Closed = Candidate.Session.Close(
			EShanmenSpiritShieldDeactivationReason::OwnerEnded);
		if (!Closed.IsSuccess() || !Candidate.IsClosed())
		{
			OutDiagnostic =
				TEXT("Spirit Shield rejected exact owner teardown.");
			return false;
		}
	}
	Candidate.Session.Reset();
	Candidate = Fdemo_mapShanmenSpiritShieldProductSession();
	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT("Spirit Shield product owner released.");
	return true;
}

bool Fdemo_mapShanmenSpiritShieldProductSession::Reset()
{
	if (IsActive())
	{
		return false;
	}
	Session.Reset();
	*this = Fdemo_mapShanmenSpiritShieldProductSession();
	return true;
}

bool Fdemo_mapShanmenSpiritShieldProductSession::IsValid() const
{
	if (!bHasActivation)
	{
		return !Reservation.IsValid()
			&& !SharedResourceReceipt.IsValid()
			&& !Session.IsValid();
	}
	if (!Reservation.IsValid()
		|| !SharedResourceReceipt.IsValid()
		|| !Session.IsValid()
		|| SharedResourceReceipt.GetTransactionId()
			!= Fdemo_mapShanmenSpiritShieldProductAuthority::MakeTransactionId(
				Reservation)
		|| SharedResourceReceipt.GetCommandId()
			!= Fdemo_mapShanmenSpiritShieldProductAuthority::MakeCommandId(
				Reservation)
		|| SharedResourceReceipt.GetResourceAfter().GetOwnerEntityId()
			!= Reservation.GetAction().GetSourceEntityId()
		|| Session.GetSchedule().GetDeadlineTick()
			- Session.GetSchedule().GetStartTick()
				!= Fdemo_mapShanmenSpiritShieldProductAuthority::
					CanonicalDurationTicks())
	{
		return false;
	}
	const EShanmenSpiritShieldActionState State =
		Session.GetActionCoordinator().GetState();
	return State == EShanmenSpiritShieldActionState::Activated
		|| State == EShanmenSpiritShieldActionState::Completed
		|| State == EShanmenSpiritShieldActionState::Interrupted;
}

bool Fdemo_mapShanmenSpiritShieldProductSession::IsActive() const
{
	return bHasActivation && Session.IsValid()
		&& Session.GetActionCoordinator().GetState()
			== EShanmenSpiritShieldActionState::Activated;
}

bool Fdemo_mapShanmenSpiritShieldProductSession::IsClosed() const
{
	if (!bHasActivation || !Session.IsValid())
	{
		return false;
	}
	const EShanmenSpiritShieldActionState State =
		Session.GetActionCoordinator().GetState();
	return State == EShanmenSpiritShieldActionState::Completed
		|| State == EShanmenSpiritShieldActionState::Interrupted;
}
