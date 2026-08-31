#pragma once

#include "CoreMinimal.h"
#include "ShanmenSwordRhythmEvaluation.h"
#include "demo_mapShanmenSwordRhythmProductHost.h"
#include "demo_mapShanmenSwordRhythmPresentation.h"

/**
 * Versioned product-owned timing content for the first Tai Chi sword rhythm.
 *
 * The fixed-tick values are authored here rather than inferred by GameMode,
 * input, animation or tests. A later balance change must create a new content
 * version/digest and therefore a new deterministic ConfigId.
 */
class Fdemo_mapShanmenSwordRhythmProductConfig
{
public:
	static FName CanonicalContentVersion();
	static FString CanonicalContentDigest();
	static FName CanonicalRuleId();
	static int64 CanonicalLinkOpenOffsetTicks();
	static int64 CanonicalLinkCloseOffsetTicks();
	static int64 CanonicalTimelineTicksPerSecond();
	static FGuid CanonicalConfigId();
	static bool TryCreateCanonical(
		Fdemo_mapShanmenSwordRhythmProductConfig& OutConfig);

	bool IsValid() const;
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FShanmenSwordRhythmDefinition& GetDefinition() const
	{
		return Definition;
	}
	int64 GetTimelineTicksPerSecond() const
	{
		return TimelineTicksPerSecond;
	}

private:
	FGuid ConfigId;
	FShanmenContentStamp Content;
	FShanmenSwordRhythmDefinition Definition;
	int64 TimelineTicksPerSecond = 0;
};

/**
 * Sole Run-lifecycle owner for canonical sword-rhythm product state.
 *
 * It installs one versioned config into P12.1's pure Host and preserves the
 * latest immutable receipt, read-only presentation state and the sole
 * Run/Owner/timeline-scoped pending-contribution ledger. Source receipts are
 * adapted into evidence here, but this Session owns no World, input,
 * animation, timer, damage multiplier or balance mutation.
 */
class Fdemo_mapShanmenSwordRhythmProductSession
{
public:
	bool TryBegin(const FGuid& RunId, FString& OutDiagnostic);
	bool TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FString& OutDiagnostic);
	/**
	 * Observes one completed BasicSword and returns any evidence bound to it.
	 * A valid rhythm receipt is always returned on success; OutBindingReceipt
	 * remains invalid when no earlier contribution was pending.
	 */
	bool TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FShanmenSwordRhythmContributionBindingReceipt& OutBindingReceipt,
		FString& OutDiagnostic);
	/**
	 * Full product handoff for a later independent evaluator. The immutable
	 * input is valid for every accepted BasicSword and carries the optional
	 * binding only when earlier contribution evidence was consumed.
	 */
	bool TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FShanmenSwordRhythmContributionBindingReceipt& OutBindingReceipt,
		FShanmenSwordRhythmEvaluationInput& OutEvaluationInput,
		FString& OutDiagnostic);
	bool TryRecordPerfectWeaponGuardContribution(
		const FShanmenWeaponGuardTimingProjectionReceipt& Receipt,
		FShanmenSwordRhythmContribution& OutContribution,
		FString& OutDiagnostic);
	bool TryRecordSpiritEvasionContribution(
		const FShanmenSpiritEvasionProjectionReceipt& Receipt,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmContribution& OutContribution,
		FString& OutDiagnostic);
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsEmpty() const;
	const FGuid& GetRunId() const { return Host.GetRunId(); }
	const Fdemo_mapShanmenSwordRhythmProductConfig& GetConfig() const
	{
		return Config;
	}
	const Fdemo_mapShanmenSwordRhythmProductHost& GetHost() const
	{
		return Host;
	}
	const FShanmenSwordRhythmReceipt& GetLastReceipt() const
	{
		return LastReceipt;
	}
	const FShanmenSwordRhythmEvaluationInput& GetLastEvaluationInput() const
	{
		return LastEvaluationInput;
	}
	const Fdemo_mapShanmenSwordRhythmPresentationState&
	GetPresentationState() const
	{
		return PresentationState;
	}
	const FShanmenSwordRhythmContributionBindingLedger&
	GetContributionBindingLedger() const
	{
		return ContributionBindings;
	}
	int32 NumRecordedObservations() const
	{
		return Host.IsValid() && !Host.IsEmpty()
			? Host.GetChain().NumRecordedObservations()
			: 0;
	}

private:
	bool TryRecordContribution(
		const FShanmenSwordRhythmContribution& Contribution,
		FString& OutDiagnostic);
	bool TryEnsureContributionBindingScope(
		const FShanmenCombatActionSnapshot& Action,
		const FGuid& TimelineId,
		FShanmenSwordRhythmContributionBindingLedger& InOutLedger,
		FString& OutDiagnostic) const;

	Fdemo_mapShanmenSwordRhythmProductConfig Config;
	Fdemo_mapShanmenSwordRhythmProductHost Host;
	FShanmenSwordRhythmReceipt LastReceipt;
	FShanmenSwordRhythmEvaluationInput LastEvaluationInput;
	Fdemo_mapShanmenSwordRhythmPresentationState PresentationState;
	FShanmenSwordRhythmContributionBindingLedger ContributionBindings;
};
