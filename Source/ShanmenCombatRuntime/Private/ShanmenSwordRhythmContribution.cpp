#include "ShanmenSwordRhythmContribution.h"

#include "ShanmenBasicSwordExecution.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenWeaponGuard.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	void AppendCanonicalTags(
		const FGameplayTagContainer& Container,
		TArray<FString>& Parts)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
	}

	bool KindMatchesAction(
		EShanmenSwordRhythmContributionKind Kind,
		const FShanmenCombatActionSnapshot& Action)
	{
		if (!Action.IsValid())
		{
			return false;
		}
		switch (Kind)
		{
		case EShanmenSwordRhythmContributionKind::PreciseSwordLink:
			return Action.GetActionDefinitionId()
				== FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		case EShanmenSwordRhythmContributionKind::PerfectWeaponGuard:
			return Action.GetActionDefinitionId()
				== FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		case EShanmenSwordRhythmContributionKind::SpiritEvasion:
			return Action.GetActionDefinitionId()
				== FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		default:
			return false;
		}
	}

	FGuid MakeContributionId(
		EShanmenSwordRhythmContributionKind Kind,
		const FShanmenCombatActionSnapshot& Action,
		const FGuid& SourceReceiptId,
		const FGuid& TimelineId,
		int64 ObservedTick)
	{
		if (!KindMatchesAction(Kind, Action)
			|| !SourceReceiptId.IsValid()
			|| !TimelineId.IsValid()
			|| ObservedTick < 0)
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			FString::FromInt(static_cast<uint8>(Kind)),
			GuidDigits(Action.GetRunId()),
			GuidDigits(Action.GetOwnerId()),
			GuidDigits(Action.GetActivationId()),
			GuidDigits(Action.GetSourceEntityId()),
			Action.GetSourceItemInstanceId().IsValid()
				? GuidDigits(Action.GetSourceItemInstanceId())
				: TEXT("NO_SOURCE_ITEM"),
			Action.GetActionDefinitionId().ToString(),
			Action.GetContent().Version.ToString(),
			Action.GetContent().Digest
		};
		AppendCanonicalTags(Action.GetSourceTags(), Parts);
		Parts.Add(GuidDigits(SourceReceiptId));
		Parts.Add(GuidDigits(TimelineId));
		Parts.Add(FString::Printf(TEXT("%lld"), ObservedTick));
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.Rhythm.Contribution.r1"), Parts);
	}

}

bool FShanmenSwordRhythmContribution::TryFinalize(
	EShanmenSwordRhythmContributionKind Kind,
	const FShanmenCombatActionSnapshot& Action,
	const FGuid& SourceReceiptId,
	const FGuid& TimelineId,
	int64 ObservedTick,
	FShanmenSwordRhythmContribution& OutContribution)
{
	OutContribution = FShanmenSwordRhythmContribution();
	FShanmenSwordRhythmContribution Candidate;
	Candidate.Kind = Kind;
	Candidate.Action = Action;
	Candidate.SourceReceiptId = SourceReceiptId;
	Candidate.TimelineId = TimelineId;
	Candidate.ObservedTick = ObservedTick;
	Candidate.ContributionId = MakeContributionId(
		Candidate.Kind,
		Candidate.Action,
		Candidate.SourceReceiptId,
		Candidate.TimelineId,
		Candidate.ObservedTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutContribution = Candidate;
	return true;
}

bool FShanmenSwordRhythmContribution::TryCapturePreciseSwordLink(
	const FShanmenSwordRhythmReceipt& Receipt,
	FShanmenSwordRhythmContribution& OutContribution)
{
	OutContribution = FShanmenSwordRhythmContribution();
	if (!Receipt.IsValid()
		|| Receipt.GetBand() != EShanmenSwordRhythmBand::PreciseLinked)
	{
		return false;
	}
	const FShanmenSwordRhythmObservation& Observation =
		Receipt.GetCurrentObservation();
	return TryFinalize(
		EShanmenSwordRhythmContributionKind::PreciseSwordLink,
		Observation.GetAction(),
		Receipt.GetReceiptId(),
		Observation.GetTimelineId(),
		Observation.GetInputTick(),
		OutContribution);
}

bool FShanmenSwordRhythmContribution::TryCapturePerfectWeaponGuard(
	const FShanmenWeaponGuardTimingProjectionReceipt& Receipt,
	FShanmenSwordRhythmContribution& OutContribution)
{
	OutContribution = FShanmenSwordRhythmContribution();
	if (!Receipt.IsValid()
		|| Receipt.GetBand() != EShanmenWeaponGuardTimingBand::Perfect)
	{
		return false;
	}
	const FShanmenWeaponGuardTimelineObservation& Observation =
		Receipt.GetObservation();
	return TryFinalize(
		EShanmenSwordRhythmContributionKind::PerfectWeaponGuard,
		Receipt.GetPolicy().GetWindow().GetAction(),
		Receipt.GetReceiptId(),
		Observation.GetTimelineId(),
		Observation.GetObservedTick(),
		OutContribution);
}

bool FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
	const FShanmenSpiritEvasionProjectionReceipt& Receipt,
	const FGuid& TimelineId,
	int64 ObservedTick,
	FShanmenSwordRhythmContribution& OutContribution)
{
	OutContribution = FShanmenSwordRhythmContribution();
	if (!Receipt.IsValid())
	{
		return false;
	}
	return TryFinalize(
		EShanmenSwordRhythmContributionKind::SpiritEvasion,
		Receipt.GetWindow().GetAction(),
		Receipt.GetProjectionId(),
		TimelineId,
		ObservedTick,
		OutContribution);
}

bool FShanmenSwordRhythmContribution::IsValid() const
{
	return ContributionId.IsValid()
		&& KindMatchesAction(Kind, Action)
		&& SourceReceiptId.IsValid()
		&& TimelineId.IsValid()
		&& ObservedTick >= 0
		&& ContributionId == MakeContributionId(
			Kind, Action, SourceReceiptId, TimelineId, ObservedTick);
}
