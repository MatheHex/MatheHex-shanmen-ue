#include "ShanmenSwordRhythm.h"

#include "ShanmenBasicSwordExecution.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool ActionsShareRhythmScope(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& ContentMatches(Left.GetContent(), Right.GetContent())
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool ObservationsMatch(
		const FShanmenSwordRhythmObservation& Left,
		const FShanmenSwordRhythmObservation& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetObservationId() == Right.GetObservationId()
			&& Left.GetTimelineId() == Right.GetTimelineId()
			&& Left.GetInputTick() == Right.GetInputTick()
			&& Left.GetAction().GetActivationId()
				== Right.GetAction().GetActivationId();
	}

	FGuid MakeDefinitionId(
		FName StyleDefinitionId,
		FName SupportedActionDefinitionId,
		FName RuleId,
		int64 LinkOpenOffsetTicks,
		int64 LinkCloseOffsetTicks)
	{
		if (StyleDefinitionId.IsNone()
			|| SupportedActionDefinitionId.IsNone()
			|| RuleId.IsNone()
			|| LinkOpenOffsetTicks < 0
			|| LinkCloseOffsetTicks <= LinkOpenOffsetTicks)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.Definition.r1"),
			{
				StyleDefinitionId.ToString(),
				SupportedActionDefinitionId.ToString(),
				RuleId.ToString(),
				FString::Printf(TEXT("%lld"), LinkOpenOffsetTicks),
				FString::Printf(TEXT("%lld"), LinkCloseOffsetTicks)
			});
	}

	FGuid MakeObservationId(
		const FShanmenCombatActionSnapshot& Action,
		const FGuid& TimelineId,
		int64 InputTick)
	{
		if (!Action.IsValid()
			|| Action.GetActionDefinitionId()
				!= FShanmenBasicSwordDefinition::CanonicalActionDefinitionId()
			|| !Action.GetSourceItemInstanceId().IsValid()
			|| !TimelineId.IsValid()
			|| InputTick < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.Observation.r1"),
			{
				GuidDigits(Action.GetRunId()),
				GuidDigits(Action.GetOwnerId()),
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				GuidDigits(Action.GetSourceItemInstanceId()),
				Action.GetActionDefinitionId().ToString(),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest,
				GuidDigits(TimelineId),
				FString::Printf(TEXT("%lld"), InputTick)
			});
	}

	EShanmenSwordRhythmBand Classify(
		const FShanmenSwordRhythmDefinition& Definition,
		const FShanmenSwordRhythmObservation& Previous,
		const FShanmenSwordRhythmObservation& Current)
	{
		if (!Definition.IsValid()
			|| !Previous.IsValid()
			|| !Current.IsValid()
			|| Previous.GetTimelineId() != Current.GetTimelineId()
			|| !ActionsShareRhythmScope(
				Previous.GetAction(), Current.GetAction())
			|| Previous.GetAction().GetActivationId()
				== Current.GetAction().GetActivationId()
			|| Current.GetInputTick() < Previous.GetInputTick())
		{
			return EShanmenSwordRhythmBand::Invalid;
		}

		const int64 DeltaTicks =
			Current.GetInputTick() - Previous.GetInputTick();
		if (DeltaTicks < Definition.GetLinkOpenOffsetTicks())
		{
			return EShanmenSwordRhythmBand::RestartedEarly;
		}
		return DeltaTicks < Definition.GetLinkCloseOffsetTicks()
			? EShanmenSwordRhythmBand::PreciseLinked
			: EShanmenSwordRhythmBand::RestartedLate;
	}

	FGuid MakeReceiptId(
		const FShanmenSwordRhythmDefinition& Definition,
		const FShanmenSwordRhythmObservation& Current,
		const FShanmenSwordRhythmObservation& Previous,
		int32 PreviousChainCount,
		int32 ResultingChainCount,
		EShanmenSwordRhythmBand Band)
	{
		if (!Definition.IsValid()
			|| !Current.IsValid()
			|| PreviousChainCount < 0
			|| ResultingChainCount <= 0
			|| Band == EShanmenSwordRhythmBand::Invalid)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.Receipt.r1"),
			{
				GuidDigits(Definition.GetDefinitionId()),
				GuidDigits(Current.GetObservationId()),
				Previous.IsValid()
					? GuidDigits(Previous.GetObservationId())
					: TEXT("NO_PREVIOUS_OBSERVATION"),
				FString::FromInt(PreviousChainCount),
				FString::FromInt(ResultingChainCount),
				FString::FromInt(static_cast<uint8>(Band))
			});
	}
}

FName FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId()
{
	return TEXT("Combat.Style.Sword.Taiji01");
}

bool FShanmenSwordRhythmDefinition::TryCapture(
	const FShanmenSwordRhythmDefinitionCapture& Capture,
	FShanmenSwordRhythmDefinition& OutDefinition)
{
	OutDefinition = FShanmenSwordRhythmDefinition();
	FShanmenSwordRhythmDefinition Candidate;
	Candidate.StyleDefinitionId = Capture.StyleDefinitionId;
	Candidate.SupportedActionDefinitionId =
		FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
	Candidate.RuleId = Capture.RuleId;
	Candidate.LinkOpenOffsetTicks = Capture.LinkOpenOffsetTicks;
	Candidate.LinkCloseOffsetTicks = Capture.LinkCloseOffsetTicks;
	Candidate.DefinitionId = MakeDefinitionId(
		Candidate.StyleDefinitionId,
		Candidate.SupportedActionDefinitionId,
		Candidate.RuleId,
		Candidate.LinkOpenOffsetTicks,
		Candidate.LinkCloseOffsetTicks);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutDefinition = Candidate;
	return true;
}

bool FShanmenSwordRhythmDefinition::IsValid() const
{
	return DefinitionId.IsValid()
		&& StyleDefinitionId == CanonicalStyleDefinitionId()
		&& SupportedActionDefinitionId
			== FShanmenBasicSwordDefinition::CanonicalActionDefinitionId()
		&& !RuleId.IsNone()
		&& LinkOpenOffsetTicks >= 0
		&& LinkCloseOffsetTicks > LinkOpenOffsetTicks
		&& DefinitionId == MakeDefinitionId(
			StyleDefinitionId,
			SupportedActionDefinitionId,
			RuleId,
			LinkOpenOffsetTicks,
			LinkCloseOffsetTicks);
}

bool FShanmenSwordRhythmObservation::TryCapture(
	const FShanmenCombatActionSnapshot& Action,
	const FGuid& TimelineId,
	int64 InputTick,
	FShanmenSwordRhythmObservation& OutObservation)
{
	OutObservation = FShanmenSwordRhythmObservation();
	FShanmenSwordRhythmObservation Candidate;
	Candidate.Action = Action;
	Candidate.TimelineId = TimelineId;
	Candidate.InputTick = InputTick;
	Candidate.ObservationId = MakeObservationId(
		Candidate.Action, Candidate.TimelineId, Candidate.InputTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutObservation = Candidate;
	return true;
}

bool FShanmenSwordRhythmObservation::IsValid() const
{
	return ObservationId.IsValid()
		&& Action.IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenBasicSwordDefinition::CanonicalActionDefinitionId()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& TimelineId.IsValid()
		&& InputTick >= 0
		&& ObservationId == MakeObservationId(Action, TimelineId, InputTick);
}

bool FShanmenSwordRhythmReceipt::IsValid() const
{
	if (!ReceiptId.IsValid()
		|| !Definition.IsValid()
		|| !CurrentObservation.IsValid()
		|| CurrentObservation.GetAction().GetActionDefinitionId()
			!= Definition.GetSupportedActionDefinitionId()
		|| PreviousChainCount < 0
		|| ResultingChainCount <= 0)
	{
		return false;
	}

	bool bContractMatches = false;
	if (Band == EShanmenSwordRhythmBand::Started)
	{
		bContractMatches = !PreviousObservation.IsValid()
			&& PreviousChainCount == 0
			&& ResultingChainCount == 1;
	}
	else if (PreviousObservation.IsValid()
		&& PreviousObservation.GetAction().GetActionDefinitionId()
			== Definition.GetSupportedActionDefinitionId())
	{
		const EShanmenSwordRhythmBand ExpectedBand = Classify(
			Definition, PreviousObservation, CurrentObservation);
		const bool bPreciseCountMatches =
			Band == EShanmenSwordRhythmBand::PreciseLinked
			&& PreviousChainCount > 0
			&& PreviousChainCount < MAX_int32
			&& ResultingChainCount == PreviousChainCount + 1;
		const bool bRestartCountMatches =
			(Band == EShanmenSwordRhythmBand::RestartedEarly
				|| Band == EShanmenSwordRhythmBand::RestartedLate)
			&& PreviousChainCount > 0
			&& ResultingChainCount == 1;
		bContractMatches = Band == ExpectedBand
			&& (bPreciseCountMatches || bRestartCountMatches);
	}

	return bContractMatches
		&& ReceiptId == MakeReceiptId(
			Definition,
			CurrentObservation,
			PreviousObservation,
			PreviousChainCount,
			ResultingChainCount,
			Band);
}

bool FShanmenSwordRhythmChain::TryCreate(
	const FShanmenSwordRhythmDefinition& Definition,
	FShanmenSwordRhythmChain& OutChain)
{
	OutChain.Reset();
	if (!Definition.IsValid())
	{
		return false;
	}
	FShanmenSwordRhythmChain Candidate;
	Candidate.Definition = Definition;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutChain = Candidate;
	return true;
}

bool FShanmenSwordRhythmChain::IsValid() const
{
	if (!Definition.IsValid())
	{
		return false;
	}
	if (ReceiptsByActivation.IsEmpty())
	{
		return !LastObservation.IsValid() && CurrentChainCount == 0;
	}
	if (!LastObservation.IsValid() || CurrentChainCount <= 0)
	{
		return false;
	}

	const FShanmenSwordRhythmReceipt* LastReceipt =
		ReceiptsByActivation.Find(
			LastObservation.GetAction().GetActivationId());
	if (LastReceipt == nullptr
		|| !LastReceipt->IsValid()
		|| !ObservationsMatch(
			LastReceipt->GetCurrentObservation(), LastObservation)
		|| LastReceipt->GetResultingChainCount() != CurrentChainCount)
	{
		return false;
	}

	for (const TPair<FGuid, FShanmenSwordRhythmReceipt>& Pair
		: ReceiptsByActivation)
	{
		if (!Pair.Key.IsValid()
			|| !Pair.Value.IsValid()
			|| Pair.Key
				!= Pair.Value.GetCurrentObservation()
					.GetAction().GetActivationId()
			|| Pair.Value.GetDefinition().GetDefinitionId()
				!= Definition.GetDefinitionId())
		{
			return false;
		}
	}
	return true;
}

bool FShanmenSwordRhythmChain::TryObserve(
	const FShanmenSwordRhythmObservation& Observation,
	FShanmenSwordRhythmReceipt& OutReceipt)
{
	OutReceipt = FShanmenSwordRhythmReceipt();
	if (!IsValid()
		|| !Observation.IsValid()
		|| Observation.GetAction().GetActionDefinitionId()
			!= Definition.GetSupportedActionDefinitionId())
	{
		return false;
	}

	const FGuid ActivationId =
		Observation.GetAction().GetActivationId();
	if (const FShanmenSwordRhythmReceipt* Existing =
		ReceiptsByActivation.Find(ActivationId))
	{
		if (!ObservationsMatch(
			Existing->GetCurrentObservation(), Observation))
		{
			return false;
		}
		OutReceipt = *Existing;
		return true;
	}

	FShanmenSwordRhythmReceipt Candidate;
	Candidate.Definition = Definition;
	Candidate.CurrentObservation = Observation;
	if (ReceiptsByActivation.IsEmpty())
	{
		Candidate.Band = EShanmenSwordRhythmBand::Started;
		Candidate.PreviousChainCount = 0;
		Candidate.ResultingChainCount = 1;
	}
	else
	{
		if (!ActionsShareRhythmScope(
				LastObservation.GetAction(), Observation.GetAction())
			|| LastObservation.GetTimelineId()
				!= Observation.GetTimelineId()
			|| Observation.GetInputTick()
				< LastObservation.GetInputTick())
		{
			return false;
		}
		Candidate.PreviousObservation = LastObservation;
		Candidate.PreviousChainCount = CurrentChainCount;
		Candidate.Band = Classify(
			Definition, LastObservation, Observation);
		if (Candidate.Band == EShanmenSwordRhythmBand::Invalid
			|| (Candidate.Band
					== EShanmenSwordRhythmBand::PreciseLinked
				&& CurrentChainCount == MAX_int32))
		{
			return false;
		}
		Candidate.ResultingChainCount =
			Candidate.Band == EShanmenSwordRhythmBand::PreciseLinked
				? CurrentChainCount + 1
				: 1;
	}
	Candidate.ReceiptId = MakeReceiptId(
		Candidate.Definition,
		Candidate.CurrentObservation,
		Candidate.PreviousObservation,
		Candidate.PreviousChainCount,
		Candidate.ResultingChainCount,
		Candidate.Band);
	if (!Candidate.IsValid())
	{
		return false;
	}

	ReceiptsByActivation.Add(ActivationId, Candidate);
	LastObservation = Observation;
	CurrentChainCount = Candidate.GetResultingChainCount();
	OutReceipt = Candidate;
	return true;
}

void FShanmenSwordRhythmChain::Reset()
{
	*this = FShanmenSwordRhythmChain();
}

bool FShanmenSwordRhythmChain::IsEmpty() const
{
	return IsValid() && ReceiptsByActivation.IsEmpty();
}
