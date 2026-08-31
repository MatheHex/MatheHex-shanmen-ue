#include "demo_mapShanmenSwordRhythmPresentation.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsKnownBand(const EShanmenSwordRhythmBand Band)
	{
		return Band == EShanmenSwordRhythmBand::Started
			|| Band == EShanmenSwordRhythmBand::PreciseLinked
			|| Band == EShanmenSwordRhythmBand::RestartedEarly
			|| Band == EShanmenSwordRhythmBand::RestartedLate;
	}

	FGuid MakePresentationStateId(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State)
	{
		if (!State.GetRunId().IsValid()
			|| !State.GetConfigId().IsValid()
			|| State.GetContentVersion().IsNone()
			|| State.GetContentDigest().IsEmpty()
			|| !State.GetReceiptId().IsValid()
			|| !State.GetActivationId().IsValid()
			|| !State.GetTimelineId().IsValid()
			|| State.GetStyleDefinitionId().IsNone()
			|| State.GetRuleId().IsNone()
			|| !IsKnownBand(State.GetBand()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.PresentationState.r1"),
			{
				GuidDigits(State.GetRunId()),
				GuidDigits(State.GetConfigId()),
				State.GetContentVersion().ToString(),
				State.GetContentDigest(),
				GuidDigits(State.GetReceiptId()),
				GuidDigits(State.GetActivationId()),
				GuidDigits(State.GetTimelineId()),
				State.GetStyleDefinitionId().ToString(),
				State.GetRuleId().ToString(),
				LexToString(State.GetLinkOpenOffsetTicks()),
				LexToString(State.GetLinkCloseOffsetTicks()),
				LexToString(State.GetTimelineTicksPerSecond()),
				LexToString(State.GetPreviousInputTick()),
				LexToString(State.GetCurrentInputTick()),
				FString::FromInt(State.GetObservationRevision()),
				FString::FromInt(State.GetPreviousChainCount()),
				FString::FromInt(State.GetResultingChainCount()),
				FString::FromInt(static_cast<int32>(State.GetBand()))
			});
	}

	Fdemo_mapShanmenSwordRhythmPresentationProjectionResult Reject(
		const Edemo_mapShanmenSwordRhythmPresentationProjectionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmPresentationProjectionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmPresentationState::IsValid() const
{
	if (!PresentationStateId.IsValid()
		|| !RunId.IsValid()
		|| !ConfigId.IsValid()
		|| ContentVersion.IsNone()
		|| ContentDigest.IsEmpty()
		|| !ReceiptId.IsValid()
		|| !ActivationId.IsValid()
		|| !TimelineId.IsValid()
		|| StyleDefinitionId.IsNone()
		|| RuleId.IsNone()
		|| LinkOpenOffsetTicks < 0
		|| LinkCloseOffsetTicks <= LinkOpenOffsetTicks
		|| TimelineTicksPerSecond <= 0
		|| CurrentInputTick < 0
		|| ObservationRevision <= 0
		|| PreviousChainCount < 0
		|| ResultingChainCount <= 0
		|| !IsKnownBand(Band))
	{
		return false;
	}

	if (Band == EShanmenSwordRhythmBand::Started)
	{
		if (PreviousInputTick != INDEX_NONE
			|| PreviousChainCount != 0
			|| ResultingChainCount != 1)
		{
			return false;
		}
	}
	else
	{
		if (PreviousInputTick < 0 || CurrentInputTick < PreviousInputTick
			|| PreviousChainCount <= 0)
		{
			return false;
		}
		const int64 OffsetTicks = CurrentInputTick - PreviousInputTick;
		const EShanmenSwordRhythmBand ExpectedBand =
			OffsetTicks < LinkOpenOffsetTicks
			? EShanmenSwordRhythmBand::RestartedEarly
			: (OffsetTicks < LinkCloseOffsetTicks
				? EShanmenSwordRhythmBand::PreciseLinked
				: EShanmenSwordRhythmBand::RestartedLate);
		if (Band != ExpectedBand)
		{
			return false;
		}
		if (Band == EShanmenSwordRhythmBand::PreciseLinked
			&& PreviousChainCount == MAX_int32)
		{
			return false;
		}
		const int32 ExpectedCount =
			Band == EShanmenSwordRhythmBand::PreciseLinked
			? PreviousChainCount + 1
			: 1;
		if (ResultingChainCount != ExpectedCount)
		{
			return false;
		}
	}

	return TimelineId
			== Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId)
		&& PresentationStateId == MakePresentationStateId(*this);
}

bool Fdemo_mapShanmenSwordRhythmPresentationState::Matches(
	const Fdemo_mapShanmenSwordRhythmPresentationState& Other) const
{
	return IsValid() && Other.IsValid()
		&& PresentationStateId == Other.PresentationStateId;
}

Fdemo_mapShanmenSwordRhythmPresentationProjectionResult
Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
	const Fdemo_mapShanmenSwordRhythmProductConfig& Config,
	const FGuid& RunId,
	const FShanmenSwordRhythmReceipt& Receipt,
	const int32 ObservationRevision)
{
	if (!Config.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::ConfigInvalid,
			TEXT("Sword-rhythm presentation requires one valid product config."));
	}
	if (!RunId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::RunInvalid,
			TEXT("Sword-rhythm presentation requires one valid Run identity."));
	}
	if (!Receipt.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::ReceiptInvalid,
			TEXT("Sword-rhythm presentation requires one valid immutable receipt."));
	}
	if (ObservationRevision <= 0)
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::RevisionInvalid,
			TEXT("Sword-rhythm presentation revision must be positive."));
	}

	const FShanmenSwordRhythmObservation& Current =
		Receipt.GetCurrentObservation();
	if (Receipt.GetDefinition().GetDefinitionId()
			!= Config.GetDefinition().GetDefinitionId()
		|| Current.GetAction().GetRunId() != RunId
		|| Current.GetTimelineId()
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId))
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::IdentityMismatch,
			TEXT("Receipt, config and Run identities do not form one presentation scope."));
	}

	Fdemo_mapShanmenSwordRhythmPresentationState Candidate;
	Candidate.RunId = RunId;
	Candidate.ConfigId = Config.GetConfigId();
	Candidate.ContentVersion = Config.GetContent().Version;
	Candidate.ContentDigest = Config.GetContent().Digest;
	Candidate.ReceiptId = Receipt.GetReceiptId();
	Candidate.ActivationId = Current.GetAction().GetActivationId();
	Candidate.TimelineId = Current.GetTimelineId();
	Candidate.StyleDefinitionId =
		Config.GetDefinition().GetStyleDefinitionId();
	Candidate.RuleId = Config.GetDefinition().GetRuleId();
	Candidate.LinkOpenOffsetTicks =
		Config.GetDefinition().GetLinkOpenOffsetTicks();
	Candidate.LinkCloseOffsetTicks =
		Config.GetDefinition().GetLinkCloseOffsetTicks();
	Candidate.TimelineTicksPerSecond = Config.GetTimelineTicksPerSecond();
	Candidate.PreviousInputTick = Receipt.HasPreviousObservation()
		? Receipt.GetPreviousObservation().GetInputTick()
		: INDEX_NONE;
	Candidate.CurrentInputTick = Current.GetInputTick();
	Candidate.ObservationRevision = ObservationRevision;
	Candidate.PreviousChainCount = Receipt.GetPreviousChainCount();
	Candidate.ResultingChainCount = Receipt.GetResultingChainCount();
	Candidate.Band = Receipt.GetBand();
	Candidate.PresentationStateId = MakePresentationStateId(Candidate);
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::ProjectionRejected,
			TEXT("Sword-rhythm presentation candidate failed self-validation."));
	}

	Fdemo_mapShanmenSwordRhythmPresentationProjectionResult Result;
	Result.Status =
		Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::Projected;
	Result.Diagnostic =
		TEXT("Immutable sword-rhythm receipt projected to one read-only state.");
	Result.State = Candidate;
	return Result;
}
