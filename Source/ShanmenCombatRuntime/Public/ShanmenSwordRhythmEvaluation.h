#pragma once

#include "CoreMinimal.h"
#include "ShanmenSwordRhythmContributionBinding.h"

#include "ShanmenSwordRhythmEvaluation.generated.h"

/**
 * Immutable handoff from rhythm/contribution authorities to a later evaluator.
 *
 * The input freezes one accepted BasicSword rhythm receipt and its optional
 * next-action contribution binding. It deliberately carries no strength,
 * multiplier, stacking, decay, damage or attribute policy.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluationInput
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenSwordRhythmReceipt& RhythmReceipt,
		const FShanmenSwordRhythmContributionBindingReceipt&
			ContributionBinding,
		FShanmenSwordRhythmEvaluationInput& OutInput);

	bool IsValid() const;
	const FGuid& GetInputId() const { return InputId; }
	const FShanmenSwordRhythmReceipt& GetRhythmReceipt() const
	{
		return RhythmReceipt;
	}
	bool HasContributionBinding() const
	{
		return ContributionBinding.IsValid();
	}
	const FShanmenSwordRhythmContributionBindingReceipt&
	GetContributionBinding() const
	{
		return ContributionBinding;
	}
	int32 NumContributions() const
	{
		return HasContributionBinding()
			? ContributionBinding.NumContributions()
			: 0;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid InputId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmReceipt RhythmReceipt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmContributionBindingReceipt ContributionBinding;
};
