#include "demo_mapShanmenSpiritShieldHUDPresentation.h"

namespace
{
	using EShieldTone = Edemo_mapShanmenSpiritShieldHUDTone;
	using FShieldPresentation =
		Fdemo_mapShanmenSpiritShieldHUDPresentation;
	using EFeedbackReason =
		Edemo_mapShanmenSpiritShieldInputFeedbackReason;
	using EFeedbackTone = Edemo_mapShanmenSpiritShieldInputFeedbackTone;
	using FFeedback =
		Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation;
	using EImpactFeedbackKind =
		Edemo_mapShanmenSpiritShieldImpactFeedbackKind;
	using FImpactFeedback =
		Fdemo_mapShanmenSpiritShieldImpactFeedbackPresentation;

	constexpr float LowCapacityFraction = 0.25f;
	constexpr double ValueTolerance = 1.0e-4;

	FString FormatCapacity(const float Value)
	{
		const float Rounded = FMath::RoundToFloat(Value);
		return FMath::IsNearlyEqual(Value, Rounded, ValueTolerance)
			? FString::Printf(TEXT("%.0f"), Rounded)
			: FString::Printf(TEXT("%.1f"), Value);
	}

	FString ToneLabel(const EShieldTone Tone)
	{
		switch (Tone)
		{
		case EShieldTone::Stable:
			return TEXT("SPIRIT SHIELD");
		case EShieldTone::Low:
			return TEXT("SPIRIT SHIELD LOW");
		case EShieldTone::Depleted:
			return TEXT("SPIRIT SHIELD DEPLETED");
		default:
			return FString();
		}
	}
}

bool FShieldPresentation::TryProject(
	const bool bSessionActive,
	const float InAvailableCapacity,
	const float InMaximumCapacity,
	const int64 CurrentTick,
	const int64 DeadlineTick,
	const int64 TicksPerSecond,
	const FString& ActivationKeyLabel,
	FShieldPresentation& OutPresentation)
{
	OutPresentation = FShieldPresentation();
	const FString KeyLabel = ActivationKeyLabel.TrimStartAndEnd();
	if (!bSessionActive || !FMath::IsFinite(InAvailableCapacity)
		|| !FMath::IsFinite(InMaximumCapacity)
		|| InMaximumCapacity <= 0.0f || InAvailableCapacity < 0.0f
		|| InAvailableCapacity > InMaximumCapacity + ValueTolerance
		|| CurrentTick < 0 || DeadlineTick <= CurrentTick
		|| TicksPerSecond <= 0 || KeyLabel.IsEmpty())
	{
		return false;
	}

	FShieldPresentation Candidate;
	Candidate.AvailableCapacity = FMath::Min(
		InAvailableCapacity, InMaximumCapacity);
	Candidate.MaximumCapacity = InMaximumCapacity;
	Candidate.RemainingTicks = DeadlineTick - CurrentTick;
	Candidate.RemainingSeconds =
		static_cast<double>(Candidate.RemainingTicks)
		/ static_cast<double>(TicksPerSecond);
	if (FMath::IsNearlyZero(Candidate.AvailableCapacity, ValueTolerance))
	{
		Candidate.AvailableCapacity = 0.0f;
		Candidate.Tone = EShieldTone::Depleted;
	}
	else if (Candidate.AvailableCapacity / Candidate.MaximumCapacity
		<= LowCapacityFraction)
	{
		Candidate.Tone = EShieldTone::Low;
	}
	else
	{
		Candidate.Tone = EShieldTone::Stable;
	}

	const int64 RemainingTenths = FMath::Max<int64>(
		1,
		FMath::CeilToInt64(Candidate.RemainingSeconds * 10.0));
	Candidate.DisplayText = FString::Printf(
		TEXT("%s  %s / %s  ·  %lld.%llds  ·  [%s]"),
		*ToneLabel(Candidate.Tone),
		*FormatCapacity(Candidate.AvailableCapacity),
		*FormatCapacity(Candidate.MaximumCapacity),
		static_cast<long long>(RemainingTenths / 10),
		static_cast<long long>(RemainingTenths % 10),
		*KeyLabel);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = Candidate;
	return true;
}

bool FShieldPresentation::IsValid() const
{
	if (Tone == EShieldTone::Invalid
		|| !FMath::IsFinite(AvailableCapacity)
		|| !FMath::IsFinite(MaximumCapacity)
		|| AvailableCapacity < 0.0f || MaximumCapacity <= 0.0f
		|| AvailableCapacity > MaximumCapacity + ValueTolerance
		|| RemainingTicks <= 0 || !FMath::IsFinite(RemainingSeconds)
		|| RemainingSeconds <= 0.0 || DisplayText.IsEmpty())
	{
		return false;
	}
	if (Tone == EShieldTone::Depleted)
	{
		return AvailableCapacity == 0.0f;
	}
	if (AvailableCapacity <= 0.0f)
	{
		return false;
	}
	const bool bLow =
		AvailableCapacity / MaximumCapacity <= LowCapacityFraction;
	return Tone == (bLow ? EShieldTone::Low : EShieldTone::Stable);
}

bool FShieldPresentation::Matches(
	const FShieldPresentation& Other) const
{
	return IsValid() && Other.IsValid() && Tone == Other.Tone
		&& AvailableCapacity == Other.AvailableCapacity
		&& MaximumCapacity == Other.MaximumCapacity
		&& RemainingTicks == Other.RemainingTicks
		&& RemainingSeconds == Other.RemainingSeconds
		&& DisplayText == Other.DisplayText;
}

bool FFeedback::TryProject(
	const Fdemo_mapShanmenSpiritShieldProductActivationResult& Result,
	const FString& ActivationKeyLabel,
	FFeedback& OutPresentation)
{
	OutPresentation = FFeedback();
	const FString KeyLabel = ActivationKeyLabel.TrimStartAndEnd();
	if (!Result.IsValid() || KeyLabel.IsEmpty())
	{
		return false;
	}

	FFeedback Candidate;
	if (Result.IsAccepted())
	{
		Candidate.Reason = EFeedbackReason::Activated;
		Candidate.Tone = EFeedbackTone::Success;
		Candidate.DisplayText = TEXT("SPIRIT SHIELD · ACTIVE");
	}
	else
	{
		switch (Result.Error)
		{
		case Edemo_mapShanmenSpiritShieldProductActivationError::AlreadyActive:
			Candidate.Reason = EFeedbackReason::AlreadyActive;
			Candidate.Tone = EFeedbackTone::Warning;
			Candidate.DisplayText =
				TEXT("SPIRIT SHIELD · ALREADY ACTIVE");
			break;
		case Edemo_mapShanmenSpiritShieldProductActivationError::ActionConflict:
			Candidate.Reason = EFeedbackReason::ActionBusy;
			Candidate.Tone = EFeedbackTone::Warning;
			Candidate.DisplayText = FString::Printf(
				TEXT("SPIRIT SHIELD · ACTION BUSY · TRY [%s] AGAIN"),
				*KeyLabel);
			break;
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				CoordinatorNotReady:
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				TimelineUnavailable:
			Candidate.Reason = EFeedbackReason::Unavailable;
			Candidate.Tone = EFeedbackTone::Warning;
			Candidate.DisplayText = TEXT("SPIRIT SHIELD · UNAVAILABLE");
			break;
		case Edemo_mapShanmenSpiritShieldProductActivationError::SessionRejected:
			if (Result.Begin.IsValid()
				&& Result.Begin.Status
					== EShanmenSpiritShieldActionStatus::Rejected
				&& Result.Begin.ResourceError
					== EShanmenActionResourceTransactionError::
						InsufficientAvailable)
			{
				Candidate.Reason = EFeedbackReason::InsufficientSpirit;
				Candidate.Tone = EFeedbackTone::Warning;
				Candidate.DisplayText = FString::Printf(
					TEXT("SPIRIT SHIELD · NEED %.0f SPIRIT"),
					Fdemo_mapShanmenSpiritShieldProductAuthority::
						CanonicalSpiritEnergyCost());
			}
			else
			{
				Candidate.Reason = EFeedbackReason::Failed;
				Candidate.Tone = EFeedbackTone::Error;
				Candidate.DisplayText =
					TEXT("SPIRIT SHIELD · ACTIVATION FAILED");
			}
			break;
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				ReservationRejected:
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				PolicyConstructionFailed:
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				ScheduleRejected:
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				SharedResourceRejected:
		case Edemo_mapShanmenSpiritShieldProductActivationError::
				StateDesynchronized:
			Candidate.Reason = EFeedbackReason::Failed;
			Candidate.Tone = EFeedbackTone::Error;
			Candidate.DisplayText =
				TEXT("SPIRIT SHIELD · ACTIVATION FAILED");
			break;
		default:
			return false;
		}
	}

	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FFeedback::IsValid() const
{
	if (Reason == EFeedbackReason::Invalid
		|| Tone == EFeedbackTone::Invalid
		|| DisplayText.TrimStartAndEnd().IsEmpty())
	{
		return false;
	}
	if (Reason == EFeedbackReason::Activated)
	{
		return Tone == EFeedbackTone::Success;
	}
	if (Reason == EFeedbackReason::Failed)
	{
		return Tone == EFeedbackTone::Error;
	}
	return Tone == EFeedbackTone::Warning;
}

bool FFeedback::Matches(const FFeedback& Other) const
{
	return IsValid() && Other.IsValid()
		&& Reason == Other.Reason
		&& Tone == Other.Tone
		&& DisplayText == Other.DisplayText;
}

bool FImpactFeedback::TryProject(
	const Fdemo_mapShanmenSpiritShieldImpactCommitResult& Result,
	FImpactFeedback& OutPresentation)
{
	OutPresentation = FImpactFeedback();
	if (!Result.DidConsumeCapacity())
	{
		return false;
	}

	const FShanmenSpiritShieldCapacityCommitReceipt& Receipt =
		Result.Capacity.Receipt;
	if (!Receipt.IsValid())
	{
		return false;
	}

	FImpactFeedback Candidate;
	Candidate.AbsorbedCapacity = Receipt.GetCommittedCapacity();
	Candidate.RemainingCapacity = Receipt.GetCapacityAfter();
	Candidate.Kind = Receipt.IsDepleted()
		? EImpactFeedbackKind::Depleted
		: EImpactFeedbackKind::Absorbed;
	Candidate.DisplayText = Receipt.IsDepleted()
		? FString::Printf(
			TEXT("SPIRIT SHIELD · ABSORBED %s · DEPLETED"),
			*FormatCapacity(Candidate.AbsorbedCapacity))
		: FString::Printf(
			TEXT("SPIRIT SHIELD · ABSORBED %s · %s LEFT"),
			*FormatCapacity(Candidate.AbsorbedCapacity),
			*FormatCapacity(Candidate.RemainingCapacity));
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FImpactFeedback::IsValid() const
{
	if (Kind == EImpactFeedbackKind::Invalid
		|| !FMath::IsFinite(AbsorbedCapacity)
		|| !FMath::IsFinite(RemainingCapacity)
		|| AbsorbedCapacity <= 0.0f || RemainingCapacity < 0.0f
		|| DisplayText.TrimStartAndEnd().IsEmpty())
	{
		return false;
	}
	return Kind == (RemainingCapacity == 0.0f
		? EImpactFeedbackKind::Depleted
		: EImpactFeedbackKind::Absorbed);
}

bool FImpactFeedback::Matches(const FImpactFeedback& Other) const
{
	return IsValid() && Other.IsValid()
		&& Kind == Other.Kind
		&& AbsorbedCapacity == Other.AbsorbedCapacity
		&& RemainingCapacity == Other.RemainingCapacity
		&& DisplayText == Other.DisplayText;
}
