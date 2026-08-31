#include "demo_mapShanmenCombatRunFixedTimeline.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FGuid MakeTimelineSampleId(const FGuid& TimelineId, const int64 Tick)
	{
		if (!TimelineId.IsValid() || Tick < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.RunFixedTimeline.Sample.r1"),
			{
				TimelineId.ToString(EGuidFormats::Digits),
				LexToString(Tick)
			});
	}
}

bool Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
	const FGuid& RequestedTimelineId,
	const int64 RequestedTick,
	Fdemo_mapShanmenCombatRunTimelineSample& OutSample)
{
	OutSample = Fdemo_mapShanmenCombatRunTimelineSample();
	Fdemo_mapShanmenCombatRunTimelineSample Candidate;
	Candidate.TimelineId = RequestedTimelineId;
	Candidate.CurrentTick = RequestedTick;
	Candidate.SampleId = MakeTimelineSampleId(
		Candidate.TimelineId,
		Candidate.CurrentTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSample = Candidate;
	return true;
}

bool Fdemo_mapShanmenCombatRunTimelineSample::IsValid() const
{
	return SampleId.IsValid()
		&& TimelineId.IsValid()
		&& CurrentTick >= 0
		&& SampleId == MakeTimelineSampleId(TimelineId, CurrentTick);
}

FGuid Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
	const FGuid& RequestedRunId)
{
	if (!RequestedRunId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.RunFixedTimeline.r1"),
		{
			RequestedRunId.ToString(EGuidFormats::Digits),
			LexToString(CanonicalTicksPerSecond())
		});
}

bool Fdemo_mapShanmenCombatRunFixedTimeline::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !RequestedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Combat Run timeline requires valid empty state and Run identity.");
		return false;
	}
	if (!IsEmpty())
	{
		if (IsActiveForRun(RequestedRunId))
		{
			OutDiagnostic =
				TEXT("Combat Run timeline is already active for this Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("Combat Run timeline rejects a second active Run.");
		return false;
	}

	const FGuid CandidateTimelineId = MakeTimelineId(RequestedRunId);
	if (!CandidateTimelineId.IsValid())
	{
		OutDiagnostic =
			TEXT("Combat Run timeline identity derivation failed closed.");
		return false;
	}
	RunId = RequestedRunId;
	TimelineId = CandidateTimelineId;
	CurrentTick = 0;
	SubTickSeconds = 0.0;
	OutDiagnostic = TEXT("Combat Run fixed timeline began at tick zero.");
	return IsValid();
}

bool Fdemo_mapShanmenCombatRunFixedTimeline::TryAdvance(
	const double DeltaSeconds,
	int64& OutAdvancedTicks,
	FString& OutDiagnostic)
{
	OutAdvancedTicks = 0;
	OutDiagnostic.Reset();
	if (!IsValid() || IsEmpty())
	{
		OutDiagnostic =
			TEXT("Combat Run timeline advance requires one active Run.");
		return false;
	}
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0)
	{
		OutDiagnostic =
			TEXT("Combat Run timeline rejects negative or non-finite delta.");
		return false;
	}

	const double TickRate = static_cast<double>(CanonicalTicksPerSecond());
	const double TotalSeconds = SubTickSeconds + DeltaSeconds;
	const double ScaledTicks = TotalSeconds * TickRate;
	if (!FMath::IsFinite(TotalSeconds)
		|| !FMath::IsFinite(ScaledTicks)
		|| ScaledTicks > static_cast<double>(MAX_int64 - CurrentTick))
	{
		OutDiagnostic = TEXT("Combat Run timeline advance would overflow.");
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
			TEXT("Combat Run timeline quantization failed closed.");
		return false;
	}

	CurrentTick += WholeTicks;
	SubTickSeconds = CandidateSubTick;
	OutAdvancedTicks = WholeTicks;
	OutDiagnostic = TEXT("Combat Run fixed timeline advanced monotonically.");
	return IsValid();
}

bool Fdemo_mapShanmenCombatRunFixedTimeline::TryCapture(
	Fdemo_mapShanmenCombatRunTimelineSample& OutSample) const
{
	OutSample = Fdemo_mapShanmenCombatRunTimelineSample();
	return IsValid()
		&& !IsEmpty()
		&& Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
			TimelineId,
			CurrentTick,
			OutSample);
}

bool Fdemo_mapShanmenCombatRunFixedTimeline::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Combat Run timeline end requires valid state and Run identity.");
		return false;
	}
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Combat Run timeline is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Combat Run timeline rejects mismatched Run teardown.");
		return false;
	}
	Reset();
	OutDiagnostic = TEXT("Combat Run fixed timeline ended with its Run.");
	return true;
}

void Fdemo_mapShanmenCombatRunFixedTimeline::Reset()
{
	RunId.Invalidate();
	TimelineId.Invalidate();
	CurrentTick = 0;
	SubTickSeconds = 0.0;
}

bool Fdemo_mapShanmenCombatRunFixedTimeline::IsValid() const
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

bool Fdemo_mapShanmenCombatRunFixedTimeline::IsEmpty() const
{
	return IsValid() && !RunId.IsValid();
}

bool Fdemo_mapShanmenCombatRunFixedTimeline::IsActiveForRun(
	const FGuid& RequestedRunId) const
{
	return IsValid()
		&& !IsEmpty()
		&& RequestedRunId.IsValid()
		&& RunId == RequestedRunId;
}
