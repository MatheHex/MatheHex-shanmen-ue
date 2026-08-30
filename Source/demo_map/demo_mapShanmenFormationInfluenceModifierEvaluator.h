#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "demo_mapShanmenFormationInfluenceIntentPlanner.h"

/** Authored collision behavior inside one channel and stack group. */
enum class Edemo_mapShanmenFormationInfluenceStackPolicy : uint8
{
	Additive,
	StrongestMagnitude,
	HighestPriority
};

/** Canonical per-specification explanation retained by an evaluation receipt. */
enum class Edemo_mapShanmenFormationInfluenceModifierDecision : uint8
{
	Applied,
	ChannelMismatch,
	RequiredTagsMissing,
	BlockedByTags,
	StackSuppressed
};

/**
 * Frozen authored modifier semantics for one existing influence policy.
 *
 * MagnitudeUnits is signed fixed-point evidence. Its scale is owned by the
 * channel consumer; this pure evaluator performs only exact integer stacking.
 */
struct Fdemo_mapShanmenFormationInfluenceModifierSpecification
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenFormationInfluencePolicy& Policy,
		FName ModifierDefinitionId,
		FGameplayTag Channel,
		const FGameplayTagContainer& RequiredSubjectTags,
		const FGameplayTagContainer& BlockedSubjectTags,
		int32 MagnitudeUnits,
		FName StackGroupId,
		Edemo_mapShanmenFormationInfluenceStackPolicy StackPolicy,
		int32 Priority,
		Fdemo_mapShanmenFormationInfluenceModifierSpecification& OutSpecification);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceModifierSpecification& Other)
		const;
	bool MatchesPolicy(
		const Fdemo_mapShanmenFormationInfluencePolicy& Policy) const;

	const FGuid& GetSpecificationId() const { return SpecificationId; }
	FName GetPolicyDefinitionId() const { return PolicyDefinitionId; }
	FName GetInfluenceDefinitionId() const { return InfluenceDefinitionId; }
	FName GetModifierDefinitionId() const { return ModifierDefinitionId; }
	const FGameplayTag& GetChannel() const { return Channel; }
	const FGameplayTagContainer& GetRequiredSubjectTags() const
	{
		return RequiredSubjectTags;
	}
	const FGameplayTagContainer& GetBlockedSubjectTags() const
	{
		return BlockedSubjectTags;
	}
	int32 GetMagnitudeUnits() const { return MagnitudeUnits; }
	FName GetStackGroupId() const { return StackGroupId; }
	Edemo_mapShanmenFormationInfluenceStackPolicy GetStackPolicy() const
	{
		return StackPolicy;
	}
	int32 GetPriority() const { return Priority; }
	const FShanmenContentStamp& GetContent() const { return Content; }

private:
	FGuid SpecificationId;
	FName PolicyDefinitionId = NAME_None;
	FName InfluenceDefinitionId = NAME_None;
	FName ModifierDefinitionId = NAME_None;
	FGameplayTag Channel;
	FGameplayTagContainer RequiredSubjectTags;
	FGameplayTagContainer BlockedSubjectTags;
	int32 MagnitudeUnits = 0;
	FName StackGroupId = NAME_None;
	Edemo_mapShanmenFormationInfluenceStackPolicy StackPolicy =
		Edemo_mapShanmenFormationInfluenceStackPolicy::Additive;
	int32 Priority = 0;
	FShanmenContentStamp Content;
};

/** Caller-supplied immutable values for one channel evaluation. */
struct Fdemo_mapShanmenFormationInfluenceEvaluationContext
{
	FGuid RunId;
	FGuid SubjectEntityId;
	FGameplayTag Channel;
	FGameplayTagContainer SubjectTags;
	FShanmenContentStamp Content;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceEvaluationContext& Other) const;
};

struct Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord
{
	Fdemo_mapShanmenFormationInfluenceModifierSpecification Specification;
	Edemo_mapShanmenFormationInfluenceModifierDecision Decision =
		Edemo_mapShanmenFormationInfluenceModifierDecision::ChannelMismatch;
	int32 ContributionMagnitudeUnits = 0;
	FGuid WinningSpecificationId;

	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord& Other)
		const;
};

/** Self-validating, input-order-independent pure-value evaluation evidence. */
struct Fdemo_mapShanmenFormationInfluenceEvaluationReceipt
{
	FGuid ReceiptId;
	Fdemo_mapShanmenFormationInfluenceEvaluationContext Context;
	TArray<Fdemo_mapShanmenFormationInfluenceModifierDecisionRecord> Decisions;
	int64 FinalMagnitudeUnits = 0;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceEvaluationReceipt& Other) const;
	int32 GetAppliedCount() const;
};

enum class Edemo_mapShanmenFormationInfluenceEvaluationStatus : uint8
{
	Evaluated,
	ContextInvalid,
	SpecificationInvalid,
	DuplicateSpecification,
	ContentMismatch,
	StackPolicyConflict,
	ReceiptRejected
};

struct Fdemo_mapShanmenFormationInfluenceEvaluationResult
{
	Edemo_mapShanmenFormationInfluenceEvaluationStatus Status =
		Edemo_mapShanmenFormationInfluenceEvaluationStatus::ContextInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceEvaluationReceipt Receipt;

	bool IsSuccess() const;
};

/** Pure deterministic evaluator; owns no lease, Host, World, Actor, or cadence. */
class Fdemo_mapShanmenFormationInfluenceModifierEvaluator
{
public:
	static Fdemo_mapShanmenFormationInfluenceEvaluationResult Evaluate(
		const Fdemo_mapShanmenFormationInfluenceEvaluationContext& Context,
		const TArray<
			Fdemo_mapShanmenFormationInfluenceModifierSpecification>&
			Specifications);
};
