#include "demo_mapShanmenSwordRhythmEffectCueDelivery.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeScopeId(
		const FGuid& RunId,
		const FGuid& ConsumerId,
		const FName ConsumerRoleId)
	{
		if (!RunId.IsValid()
			|| !ConsumerId.IsValid()
			|| ConsumerRoleId.IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueConsumerScope.r1"),
			{
				GuidDigits(RunId),
				GuidDigits(ConsumerId),
				ConsumerRoleId.ToString()
			});
	}

	FGuid MakeDeliveryId(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Scope,
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event)
	{
		if (!Scope.IsValid()
			|| !Event.IsValid()
			|| Event.GetState().GetRunId() != Scope.GetRunId())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueDelivery.r1"),
			{
				GuidDigits(Scope.GetScopeId()),
				GuidDigits(Event.GetEventId()),
				GuidDigits(Event.GetState().GetPresentationStateId()),
				FString::FromInt(
					Event.GetState().GetObservationRevision())
			});
	}

	FGuid MakeAcknowledgementId(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery)
	{
		if (!Delivery.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueAcknowledgement.r1"),
			{
				GuidDigits(Delivery.GetScope().GetScopeId()),
				GuidDigits(Delivery.GetDeliveryId()),
				GuidDigits(Delivery.GetEvent().GetEventId())
			});
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePrepareResult PrepareResult(
		const Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery =
			Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt())
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePrepareResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Delivery = Delivery;
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgeResult AcknowledgeResult(
		const Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
			Acknowledgement =
				Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt())
	{
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgeResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Acknowledgement = Acknowledgement;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
	const FGuid& InRunId,
	const FGuid& InConsumerId,
	const FName InConsumerRoleId,
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& OutScope)
{
	OutScope = Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope();
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Candidate;
	Candidate.RunId = InRunId;
	Candidate.ConsumerId = InConsumerId;
	Candidate.ConsumerRoleId = InConsumerRoleId;
	Candidate.ScopeId = MakeScopeId(
		Candidate.RunId,
		Candidate.ConsumerId,
		Candidate.ConsumerRoleId);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutScope = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::IsValid() const
{
	return ScopeId.IsValid()
		&& ScopeId == MakeScopeId(RunId, ConsumerId, ConsumerRoleId);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Other) const
{
	return IsValid() && Other.IsValid() && ScopeId == Other.ScopeId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& InScope,
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& InEvent,
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt();
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt Candidate;
	Candidate.Scope = InScope;
	Candidate.Event = InEvent;
	Candidate.DeliveryId = MakeDeliveryId(Candidate.Scope, Candidate.Event);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt::IsValid() const
{
	return DeliveryId.IsValid()
		&& Scope.IsValid()
		&& Event.IsValid()
		&& Event.GetState().GetRunId() == Scope.GetRunId()
		&& DeliveryId == MakeDeliveryId(Scope, Event);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& DeliveryId == Other.DeliveryId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& InDelivery,
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt();
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt Candidate;
	Candidate.Delivery = InDelivery;
	Candidate.AcknowledgementId = MakeAcknowledgementId(Candidate.Delivery);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt::IsValid()
	const
{
	return AcknowledgementId.IsValid()
		&& Delivery.IsValid()
		&& AcknowledgementId == MakeAcknowledgementId(Delivery);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt& Other)
	const
{
	return IsValid()
		&& Other.IsValid()
		&& AcknowledgementId == Other.AcknowledgementId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& InScope,
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor& OutCursor)
{
	OutCursor = Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor();
	if (!InScope.IsValid())
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor Candidate;
	Candidate.Scope = InScope;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCursor = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::IsValid() const
{
	if (!Scope.IsValid())
	{
		return false;
	}
	if (!bHasAcknowledgement)
	{
		return !LastAcknowledgement.IsValid();
	}
	return LastAcknowledgement.IsValid()
		&& LastAcknowledgement.GetDelivery().GetScope().Matches(Scope);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::
	TryGetLastAcknowledgement(
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
			OutAcknowledgement) const
{
	OutAcknowledgement =
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt();
	if (!IsValid() || !bHasAcknowledgement)
	{
		return false;
	}
	OutAcknowledgement = LastAcknowledgement;
	return true;
}

Fdemo_mapShanmenSwordRhythmEffectCuePrepareResult
Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::Prepare(
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event) const
{
	if (!IsValid())
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::CursorInvalid,
			TEXT("Effect-cue delivery requires one valid consumer cursor."));
	}
	if (!Event.IsValid())
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::EventInvalid,
			TEXT("Effect-cue delivery requires one valid immutable event."));
	}
	if (Event.GetState().GetRunId() != Scope.GetRunId())
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::RunMismatch,
			TEXT("Effect-cue event belongs to a different Run."));
	}
	if (bHasAcknowledgement)
	{
		const auto& PreviousDelivery =
			LastAcknowledgement.GetDelivery();
		const auto& PreviousEvent = PreviousDelivery.GetEvent();
		if (PreviousEvent.Matches(Event))
		{
			return PrepareResult(
				Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::
					AlreadyAcknowledged,
				TEXT("This consumer already acknowledged the exact cue event."),
				PreviousDelivery);
		}
		if (Event.GetState().GetObservationRevision()
			<= PreviousEvent.GetState().GetObservationRevision())
		{
			return PrepareResult(
				Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::StaleEvent,
				TEXT("Cue event does not advance this consumer's acknowledged revision."));
		}
	}

	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt Delivery;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt::TryCreate(
			Scope, Event, Delivery))
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::
				DeliveryRejected,
			TEXT("Cue event could not form self-validating delivery evidence."));
	}
	return PrepareResult(
		Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::Prepared,
		TEXT("Cue event prepared without advancing the consumer cursor."),
		Delivery);
}

Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgeResult
Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::Acknowledge(
	const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery)
{
	if (!IsValid())
	{
		return AcknowledgeResult(
			Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
				CursorInvalid,
			TEXT("Effect-cue acknowledgement requires a valid cursor."));
	}
	if (!Delivery.IsValid())
	{
		return AcknowledgeResult(
			Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
				DeliveryInvalid,
			TEXT("Effect-cue acknowledgement requires valid delivery evidence."));
	}
	if (!Delivery.GetScope().Matches(Scope))
	{
		return AcknowledgeResult(
			Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
				ScopeMismatch,
			TEXT("Cue delivery belongs to a different consumer or Run scope."));
	}
	if (bHasAcknowledgement)
	{
		const auto& PreviousDelivery =
			LastAcknowledgement.GetDelivery();
		if (PreviousDelivery.Matches(Delivery))
		{
			return AcknowledgeResult(
				Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					AcknowledgementReplayed,
				TEXT("The exact cue acknowledgement replayed idempotently."),
				LastAcknowledgement);
		}
		if (Delivery.GetEvent().GetState().GetObservationRevision()
			<= PreviousDelivery.GetEvent().GetState().
				GetObservationRevision())
		{
			return AcknowledgeResult(
				Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					StaleDelivery,
				TEXT("Cue delivery does not advance the acknowledged revision."));
		}
	}

	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt
		Acknowledgement;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt::TryCreate(
			Delivery, Acknowledgement))
	{
		return AcknowledgeResult(
			Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
				AcknowledgementRejected,
			TEXT("Delivery could not form self-validating acknowledgement evidence."));
	}
	const auto Before = *this;
	bHasAcknowledgement = true;
	LastAcknowledgement = Acknowledgement;
	if (!IsValid())
	{
		*this = Before;
		return AcknowledgeResult(
			Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
				AcknowledgementRejected,
			TEXT("Cue cursor rejected its candidate acknowledgement state."));
	}
	return AcknowledgeResult(
		Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::Acknowledged,
		TEXT("Consumer acknowledged the cue event and advanced its cursor."),
		Acknowledgement);
}
