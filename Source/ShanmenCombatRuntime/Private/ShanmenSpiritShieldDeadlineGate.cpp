#include "ShanmenSpiritShieldDeadlineGate.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeContractId(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		const FGuid& TimelineId,
		int64 StartTick,
		int64 DeadlineTick)
	{
		if (!Activation.IsValid() || !TimelineId.IsValid()
			|| StartTick < 0 || DeadlineTick <= StartTick)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.DeadlineContract.r1"),
			{
				GuidDigits(Activation.GetReceiptId()),
				GuidDigits(Activation.GetShieldInstanceId()),
				GuidDigits(TimelineId),
				FString::Printf(TEXT("%lld"), StartTick),
				FString::Printf(TEXT("%lld"), DeadlineTick)
			});
	}

	FGuid MakeObservationId(const FGuid& TimelineId, int64 ObservedTick)
	{
		if (!TimelineId.IsValid() || ObservedTick < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.TimelineObservation.r1"),
			{
				GuidDigits(TimelineId),
				FString::Printf(TEXT("%lld"), ObservedTick)
			});
	}

	FGuid MakeElapsedReceiptId(
		const FShanmenSpiritShieldDeadlineContract& Contract,
		const FShanmenSpiritShieldTimelineObservation& Observation,
		const FShanmenSpiritShieldDeactivationReceipt& Deactivation)
	{
		if (!Contract.IsValid() || !Observation.IsValid()
			|| !Deactivation.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.DeadlineElapsedReceipt.r1"),
			{
				GuidDigits(Contract.GetContractId()),
				GuidDigits(Observation.GetObservationId()),
				GuidDigits(Deactivation.GetReceiptId())
			});
	}

	bool ActivationsMatch(
		const FShanmenSpiritShieldActivationReceipt& Left,
		const FShanmenSpiritShieldActivationReceipt& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetReceiptId() == Right.GetReceiptId()
			&& Left.GetShieldInstanceId() == Right.GetShieldInstanceId();
	}
}

bool FShanmenSpiritShieldDeadlineContract::TryCapture(
	const FShanmenSpiritShieldActivationReceipt& Activation,
	const FGuid& TimelineId,
	int64 StartTick,
	int64 DeadlineTick,
	FShanmenSpiritShieldDeadlineContract& OutContract)
{
	OutContract = FShanmenSpiritShieldDeadlineContract();
	FShanmenSpiritShieldDeadlineContract Candidate;
	Candidate.Activation = Activation;
	Candidate.TimelineId = TimelineId;
	Candidate.StartTick = StartTick;
	Candidate.DeadlineTick = DeadlineTick;
	Candidate.ContractId = MakeContractId(
		Activation, TimelineId, StartTick, DeadlineTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutContract = Candidate;
	return true;
}

bool FShanmenSpiritShieldDeadlineContract::IsValid() const
{
	return ContractId.IsValid() && Activation.IsValid()
		&& TimelineId.IsValid() && StartTick >= 0
		&& DeadlineTick > StartTick
		&& ContractId == MakeContractId(
			Activation, TimelineId, StartTick, DeadlineTick);
}

bool FShanmenSpiritShieldTimelineObservation::TryCapture(
	const FGuid& TimelineId,
	int64 ObservedTick,
	FShanmenSpiritShieldTimelineObservation& OutObservation)
{
	OutObservation = FShanmenSpiritShieldTimelineObservation();
	FShanmenSpiritShieldTimelineObservation Candidate;
	Candidate.TimelineId = TimelineId;
	Candidate.ObservedTick = ObservedTick;
	Candidate.ObservationId = MakeObservationId(TimelineId, ObservedTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutObservation = Candidate;
	return true;
}

bool FShanmenSpiritShieldTimelineObservation::IsValid() const
{
	return ObservationId.IsValid() && TimelineId.IsValid()
		&& ObservedTick >= 0
		&& ObservationId == MakeObservationId(TimelineId, ObservedTick);
}

bool FShanmenSpiritShieldDeadlineElapsedReceipt::IsValid() const
{
	return ReceiptId.IsValid() && Contract.IsValid()
		&& Observation.IsValid() && Deactivation.IsValid()
		&& Observation.GetTimelineId() == Contract.GetTimelineId()
		&& Observation.GetObservedTick() >= Contract.GetDeadlineTick()
		&& Deactivation.GetReason()
			== EShanmenSpiritShieldDeactivationReason::DurationElapsed
		&& ActivationsMatch(
			Contract.GetActivation(), Deactivation.GetActivation())
		&& ReceiptId == MakeElapsedReceiptId(
			Contract, Observation, Deactivation);
}

bool FShanmenSpiritShieldDeadlineResult::IsValid() const
{
	switch (Status)
	{
	case EShanmenSpiritShieldDeadlineStatus::Elapsed:
	case EShanmenSpiritShieldDeadlineStatus::AlreadyElapsed:
		return Error == EShanmenSpiritShieldDeadlineError::None
			&& Receipt.IsValid();
	case EShanmenSpiritShieldDeadlineStatus::Rejected:
		return Error != EShanmenSpiritShieldDeadlineError::None
			&& !Receipt.IsValid();
	default:
		return false;
	}
}

bool FShanmenSpiritShieldDeadlineResult::IsSuccess() const
{
	return IsValid()
		&& (Status == EShanmenSpiritShieldDeadlineStatus::Elapsed
			|| Status
				== EShanmenSpiritShieldDeadlineStatus::AlreadyElapsed);
}

bool FShanmenSpiritShieldDeadlineGate::TryCreate(
	const FShanmenSpiritShieldDeadlineContract& Contract,
	FShanmenSpiritShieldDeadlineGate& OutGate)
{
	OutGate.Reset();
	if (!Contract.IsValid())
	{
		return false;
	}
	FShanmenSpiritShieldDeadlineGate Candidate;
	Candidate.Contract = Contract;
	Candidate.bInitialized = true;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutGate = Candidate;
	return true;
}

bool FShanmenSpiritShieldDeadlineGate::IsValid() const
{
	if (!bInitialized || !Contract.IsValid())
	{
		return false;
	}
	return !ElapsedReceipt.GetReceiptId().IsValid()
		|| (ElapsedReceipt.IsValid()
			&& ElapsedReceipt.GetContract().GetContractId()
				== Contract.GetContractId());
}

FShanmenSpiritShieldDeadlineResult
FShanmenSpiritShieldDeadlineGate::TryElapse(
	FShanmenSpiritShieldRuntime& ShieldRuntime,
	const FShanmenSpiritShieldTimelineObservation& Observation)
{
	if (!Observation.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldDeadlineError::InvalidObservation);
	}
	if (!IsValid())
	{
		return Reject(EShanmenSpiritShieldDeadlineError::GateNotReady);
	}
	if (Observation.GetTimelineId() != Contract.GetTimelineId())
	{
		return Reject(EShanmenSpiritShieldDeadlineError::TimelineMismatch);
	}
	if (ElapsedReceipt.IsValid())
	{
		if (Observation.GetObservationId()
			!= ElapsedReceipt.GetObservation().GetObservationId())
		{
			return Reject(
				EShanmenSpiritShieldDeadlineError::DeadlineConflict);
		}
		FShanmenSpiritShieldDeadlineResult Result;
		Result.Status =
			EShanmenSpiritShieldDeadlineStatus::AlreadyElapsed;
		Result.Receipt = ElapsedReceipt;
		return Result;
	}
	if (!ShieldRuntime.IsValid()
		|| !ActivationsMatch(
			ShieldRuntime.GetActivationReceipt(), Contract.GetActivation()))
	{
		return Reject(EShanmenSpiritShieldDeadlineError::ShieldMismatch);
	}
	if (Observation.GetObservedTick() < Contract.GetDeadlineTick())
	{
		return Reject(
			EShanmenSpiritShieldDeadlineError::DeadlineNotReached);
	}

	FShanmenSpiritShieldDeactivationReceipt Deactivation;
	if (!ShieldRuntime.TryDeactivateForDeadline(
			Contract.GetActivation().GetShieldInstanceId(),
			Deactivation))
	{
		return Reject(
			EShanmenSpiritShieldDeadlineError::ShieldUnavailable);
	}

	FShanmenSpiritShieldDeadlineElapsedReceipt Candidate;
	Candidate.Contract = Contract;
	Candidate.Observation = Observation;
	Candidate.Deactivation = Deactivation;
	Candidate.ReceiptId = MakeElapsedReceiptId(
		Contract, Observation, Deactivation);
	if (!Candidate.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldDeadlineError::ShieldUnavailable);
	}
	ElapsedReceipt = Candidate;

	FShanmenSpiritShieldDeadlineResult Result;
	Result.Status = EShanmenSpiritShieldDeadlineStatus::Elapsed;
	Result.Receipt = ElapsedReceipt;
	return Result;
}

void FShanmenSpiritShieldDeadlineGate::Reset()
{
	*this = FShanmenSpiritShieldDeadlineGate();
}

FShanmenSpiritShieldDeadlineResult
FShanmenSpiritShieldDeadlineGate::Reject(
	EShanmenSpiritShieldDeadlineError Error) const
{
	FShanmenSpiritShieldDeadlineResult Result;
	Result.Status = EShanmenSpiritShieldDeadlineStatus::Rejected;
	Result.Error = Error;
	return Result;
}
