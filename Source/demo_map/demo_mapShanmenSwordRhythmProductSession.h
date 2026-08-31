#pragma once

#include "CoreMinimal.h"
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
 * latest immutable receipt and read-only presentation state. It owns no World,
 * input, animation, timer, damage multiplier or balance mutation.
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
	const Fdemo_mapShanmenSwordRhythmPresentationState&
	GetPresentationState() const
	{
		return PresentationState;
	}
	int32 NumRecordedObservations() const
	{
		return Host.IsValid() && !Host.IsEmpty()
			? Host.GetChain().NumRecordedObservations()
			: 0;
	}

private:
	Fdemo_mapShanmenSwordRhythmProductConfig Config;
	Fdemo_mapShanmenSwordRhythmProductHost Host;
	FShanmenSwordRhythmReceipt LastReceipt;
	Fdemo_mapShanmenSwordRhythmPresentationState PresentationState;
};
