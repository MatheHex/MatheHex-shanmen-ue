#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritEvasion.h"
#include "ShanmenSwordRhythm.h"
#include "ShanmenWeaponPerfectGuard.h"

#include "ShanmenSwordRhythmContribution.generated.h"

/**
 * Already-proven combat facts that may contribute to a later sword action.
 * This vocabulary deliberately carries no strength, multiplier or consumption rule.
 */
UENUM(BlueprintType)
enum class EShanmenSwordRhythmContributionKind : uint8
{
	Invalid,
	PreciseSwordLink,
	PerfectWeaponGuard,
	SpiritEvasion
};

/**
 * Immutable, replay-stable evidence that one eligible combat fact occurred.
 *
 * A later authority may decide whether and how to consume this fact. This value
 * does not mutate a rhythm chain, apply damage, own a clock or define balance.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmContribution
{
	GENERATED_BODY()

public:
	static bool TryCapturePreciseSwordLink(
		const FShanmenSwordRhythmReceipt& Receipt,
		FShanmenSwordRhythmContribution& OutContribution);
	static bool TryCapturePerfectWeaponGuard(
		const FShanmenWeaponGuardTimingProjectionReceipt& Receipt,
		FShanmenSwordRhythmContribution& OutContribution);
	static bool TryCaptureSpiritEvasion(
		const FShanmenSpiritEvasionProjectionReceipt& Receipt,
		const FGuid& TimelineId,
		int64 ObservedTick,
		FShanmenSwordRhythmContribution& OutContribution);

	bool IsValid() const;
	const FGuid& GetContributionId() const { return ContributionId; }
	EShanmenSwordRhythmContributionKind GetKind() const { return Kind; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FGuid& GetSourceReceiptId() const { return SourceReceiptId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetObservedTick() const { return ObservedTick; }

private:
	static bool TryFinalize(
		EShanmenSwordRhythmContributionKind Kind,
		const FShanmenCombatActionSnapshot& Action,
		const FGuid& SourceReceiptId,
		const FGuid& TimelineId,
		int64 ObservedTick,
		FShanmenSwordRhythmContribution& OutContribution);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid ContributionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	EShanmenSwordRhythmContributionKind Kind =
		EShanmenSwordRhythmContributionKind::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid SourceReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	int64 ObservedTick = INDEX_NONE;
};
