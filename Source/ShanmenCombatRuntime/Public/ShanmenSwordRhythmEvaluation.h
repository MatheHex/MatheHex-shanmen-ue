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

/** Mutable authored mapping from one proven source kind to one named effect. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEffectSpecificationCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	EShanmenSwordRhythmContributionKind ContributionKind =
		EShanmenSwordRhythmContributionKind::Invalid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	FName EffectDefinitionId = NAME_None;
};

/** Immutable content-versioned mapping for one contribution kind. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEffectSpecification
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenSwordRhythmEffectSpecificationCapture& Capture,
		const FShanmenContentStamp& Content,
		FShanmenSwordRhythmEffectSpecification& OutSpecification);

	bool IsValid() const;
	const FGuid& GetSpecificationId() const { return SpecificationId; }
	EShanmenSwordRhythmContributionKind GetContributionKind() const
	{
		return ContributionKind;
	}
	FName GetEffectDefinitionId() const { return EffectDefinitionId; }
	const FShanmenContentStamp& GetContent() const { return Content; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid SpecificationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	EShanmenSwordRhythmContributionKind ContributionKind =
		EShanmenSwordRhythmContributionKind::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FName EffectDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenContentStamp Content;
};

/** Mutable authored input for one complete evaluator policy version. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluationPolicyCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	FName PolicyDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	FShanmenContentStamp Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	TArray<FShanmenSwordRhythmEffectSpecificationCapture> Specifications;
};

/**
 * Immutable authored vocabulary for translating every current contribution
 * kind into one named effect. It deliberately contains no numeric strength.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluationPolicy
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenSwordRhythmEvaluationPolicyCapture& Capture,
		FShanmenSwordRhythmEvaluationPolicy& OutPolicy);

	bool IsValid() const;
	bool TryFindSpecification(
		EShanmenSwordRhythmContributionKind ContributionKind,
		FShanmenSwordRhythmEffectSpecification& OutSpecification) const;
	const FGuid& GetPolicyId() const { return PolicyId; }
	FName GetPolicyDefinitionId() const { return PolicyDefinitionId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const TArray<FShanmenSwordRhythmEffectSpecification>& GetSpecifications()
		const
	{
		return Specifications;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid PolicyId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FName PolicyDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenContentStamp Content;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	TArray<FShanmenSwordRhythmEffectSpecification> Specifications;
};

/** One source fact translated into one authored, non-numeric effect token. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluatedEffect
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FShanmenSwordRhythmEvaluationInput& Input,
		const FShanmenSwordRhythmEffectSpecification& Specification,
		const FShanmenSwordRhythmContribution& Contribution,
		FShanmenSwordRhythmEvaluatedEffect& OutEffect);

	bool IsValid() const;
	const FGuid& GetEffectId() const { return EffectId; }
	const FGuid& GetEvaluationInputId() const { return EvaluationInputId; }
	const FShanmenSwordRhythmEffectSpecification& GetSpecification() const
	{
		return Specification;
	}
	const FShanmenSwordRhythmContribution& GetContribution() const
	{
		return Contribution;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid EffectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid EvaluationInputId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmEffectSpecification Specification;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmContribution Contribution;
};

/** Self-validating evidence produced by one pure policy evaluation. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluationReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSwordRhythmEvaluationPolicy& GetPolicy() const
	{
		return Policy;
	}
	const FShanmenSwordRhythmEvaluationInput& GetInput() const
	{
		return Input;
	}
	const TArray<FShanmenSwordRhythmEvaluatedEffect>& GetEffects() const
	{
		return Effects;
	}
	int32 NumEffects() const { return Effects.Num(); }

private:
	friend class FShanmenSwordRhythmEvaluator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmEvaluationPolicy Policy;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmEvaluationInput Input;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	TArray<FShanmenSwordRhythmEvaluatedEffect> Effects;
};

UENUM(BlueprintType)
enum class EShanmenSwordRhythmEvaluationStatus : uint8
{
	Evaluated,
	InputInvalid,
	PolicyInvalid,
	EffectMappingRejected,
	ReceiptRejected
};

/** Immutable-style result envelope for one pure evaluation call. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluationResult
{
	GENERATED_BODY()

public:
	bool IsSuccess() const;
	EShanmenSwordRhythmEvaluationStatus GetStatus() const { return Status; }
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FShanmenSwordRhythmEvaluationReceipt& GetReceipt() const
	{
		return Receipt;
	}

private:
	friend class FShanmenSwordRhythmEvaluator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	EShanmenSwordRhythmEvaluationStatus Status =
		EShanmenSwordRhythmEvaluationStatus::InputInvalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FString Diagnostic;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmEvaluationReceipt Receipt;
};

/** Pure deterministic translator; owns no World, Actor, clock or product state. */
class SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmEvaluator
{
public:
	static FShanmenSwordRhythmEvaluationResult Evaluate(
		const FShanmenSwordRhythmEvaluationInput& Input,
		const FShanmenSwordRhythmEvaluationPolicy& Policy);
};
