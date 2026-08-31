#include "demo_mapShanmenWeaponGuardFixedTimeline.h"

#include "ShanmenDeterministicId.h"

FGuid Fdemo_mapShanmenWeaponGuardFixedTimeline::MakeTimelineId(
	const FGuid& RequestedRunId)
{
	if (!RequestedRunId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Sword.WeaponGuard.FixedTimeline.r1"),
		{
			RequestedRunId.ToString(EGuidFormats::Digits),
			LexToString(CanonicalTicksPerSecond())
		});
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !RequestedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline requires valid empty state and Run identity.");
		return false;
	}
	if (!IsEmpty())
	{
		if (IsActiveForRun(RequestedRunId))
		{
			OutDiagnostic =
				TEXT("Weapon-guard timeline is already active for this Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("Weapon-guard timeline rejects a second active Run.");
		return false;
	}

	const FGuid CandidateTimelineId = MakeTimelineId(RequestedRunId);
	if (!CandidateTimelineId.IsValid())
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline identity derivation failed closed.");
		return false;
	}
	RunId = RequestedRunId;
	TimelineId = CandidateTimelineId;
	CurrentTick = 0;
	SubTickSeconds = 0.0;
	OutDiagnostic = TEXT("Weapon-guard fixed timeline began at tick zero.");
	return IsValid();
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::TryAdvance(
	const double DeltaSeconds,
	int64& OutAdvancedTicks,
	FString& OutDiagnostic)
{
	OutAdvancedTicks = 0;
	OutDiagnostic.Reset();
	if (!IsValid() || IsEmpty())
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline advance requires one active Run.");
		return false;
	}
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0)
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline rejects negative or non-finite delta.");
		return false;
	}

	const double TickRate = static_cast<double>(CanonicalTicksPerSecond());
	const double TotalSeconds = SubTickSeconds + DeltaSeconds;
	const double ScaledTicks = TotalSeconds * TickRate;
	if (!FMath::IsFinite(TotalSeconds)
		|| !FMath::IsFinite(ScaledTicks)
		|| ScaledTicks > static_cast<double>(MAX_int64 - CurrentTick))
	{
		OutDiagnostic = TEXT("Weapon-guard timeline advance would overflow.");
		return false;
	}

	const int64 WholeTicks = FMath::FloorToInt64(ScaledTicks + 1.0e-9);
	double CandidateSubTick =
		(ScaledTicks - static_cast<double>(WholeTicks)) / TickRate;
	if (CandidateSubTick < 0.0 && CandidateSubTick > -1.0e-10)
	{
		CandidateSubTick = 0.0;
	}
	const double TickSeconds = 1.0 / TickRate;
	if (WholeTicks < 0
		|| CandidateSubTick < 0.0
		|| CandidateSubTick >= TickSeconds
		|| !FMath::IsFinite(CandidateSubTick))
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline quantization failed closed.");
		return false;
	}

	CurrentTick += WholeTicks;
	SubTickSeconds = CandidateSubTick;
	OutAdvancedTicks = WholeTicks;
	OutDiagnostic = TEXT("Weapon-guard fixed timeline advanced monotonically.");
	return IsValid();
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::TryCapture(
	Fdemo_mapShanmenWeaponGuardInputTimelineSample& OutSample) const
{
	OutSample = Fdemo_mapShanmenWeaponGuardInputTimelineSample();
	return IsValid()
		&& !IsEmpty()
		&& Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
			TimelineId,
			CurrentTick,
			OutSample);
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline end requires valid state and Run identity.");
		return false;
	}
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Weapon-guard timeline is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Weapon-guard timeline rejects mismatched Run teardown.");
		return false;
	}
	Reset();
	OutDiagnostic = TEXT("Weapon-guard fixed timeline ended with its Run.");
	return true;
}

void Fdemo_mapShanmenWeaponGuardFixedTimeline::Reset()
{
	RunId.Invalidate();
	TimelineId.Invalidate();
	CurrentTick = 0;
	SubTickSeconds = 0.0;
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::IsValid() const
{
	if (!RunId.IsValid() && !TimelineId.IsValid())
	{
		return CurrentTick == 0 && SubTickSeconds == 0.0;
	}
	const double TickSeconds =
		1.0 / static_cast<double>(CanonicalTicksPerSecond());
	return RunId.IsValid()
		&& TimelineId.IsValid()
		&& TimelineId == MakeTimelineId(RunId)
		&& CurrentTick >= 0
		&& FMath::IsFinite(SubTickSeconds)
		&& SubTickSeconds >= 0.0
		&& SubTickSeconds < TickSeconds;
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::IsEmpty() const
{
	return IsValid() && !RunId.IsValid();
}

bool Fdemo_mapShanmenWeaponGuardFixedTimeline::IsActiveForRun(
	const FGuid& RequestedRunId) const
{
	return IsValid()
		&& !IsEmpty()
		&& RequestedRunId.IsValid()
		&& RunId == RequestedRunId;
}
