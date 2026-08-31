#pragma once

#include "CoreMinimal.h"
#include "ShanmenSwordRhythm.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"

struct Fdemo_mapBasicSwordProductExecutionResult;

/**
 * Run-local bridge from completed product BasicSword actions to the pure
 * sword-rhythm chain. It owns no input, animation, damage, timer or content
 * defaults; callers provide one frozen definition and exact Run-clock samples.
 */
class Fdemo_mapShanmenSwordRhythmProductHost
{
public:
	bool TryBegin(
		const FGuid& RunId,
		const FShanmenSwordRhythmDefinition& Definition,
		FString& OutDiagnostic);
	bool TryObserveExecutedBasicSword(
		const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		FShanmenSwordRhythmReceipt& OutReceipt,
		FString& OutDiagnostic);
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsEmpty() const;
	const FGuid& GetRunId() const { return RunId; }
	const FShanmenSwordRhythmDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenSwordRhythmChain& GetChain() const { return Chain; }

private:
	FGuid RunId;
	FShanmenSwordRhythmDefinition Definition;
	FShanmenSwordRhythmChain Chain;
};
