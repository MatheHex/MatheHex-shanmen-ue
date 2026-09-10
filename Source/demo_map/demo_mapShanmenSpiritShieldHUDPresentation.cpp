#include "demo_mapShanmenSpiritShieldHUDPresentation.h"

namespace
{
	using EShieldTone = Edemo_mapShanmenSpiritShieldHUDTone;
	using FShieldPresentation =
		Fdemo_mapShanmenSpiritShieldHUDPresentation;

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
