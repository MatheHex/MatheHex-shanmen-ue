#include "demo_mapShanmenSwordRhythmProductHost.h"

#include "demo_mapCombatRunCoordinator.h"

bool Fdemo_mapShanmenSwordRhythmProductHost::TryBegin(
	const FGuid& RequestedRunId,
	const FShanmenSwordRhythmDefinition& RequestedDefinition,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedDefinition.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Host requires valid empty state, Run identity and definition.");
		return false;
	}
	if (!IsEmpty())
	{
		if (RunId == RequestedRunId
			&& Definition.GetDefinitionId()
				== RequestedDefinition.GetDefinitionId())
		{
			OutDiagnostic =
				TEXT("Sword-rhythm product Host is already active for this exact Run and definition.");
			return true;
		}
		OutDiagnostic =
			TEXT("Sword-rhythm product Host rejects a second active Run or definition.");
		return false;
	}

	FShanmenSwordRhythmChain CandidateChain;
	if (!FShanmenSwordRhythmChain::TryCreate(
			RequestedDefinition,
			CandidateChain))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Host could not create its pure chain.");
		return false;
	}

	Fdemo_mapShanmenSwordRhythmProductHost Candidate;
	Candidate.RunId = RequestedRunId;
	Candidate.Definition = RequestedDefinition;
	Candidate.Chain = CandidateChain;
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Host candidate failed structural validation.");
		return false;
	}
	*this = Candidate;
	OutDiagnostic =
		TEXT("Sword-rhythm product Host began from caller-owned timing content without deriving defaults.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductHost::TryObserveExecutedBasicSword(
	const Fdemo_mapBasicSwordProductExecutionResult& ProductResult,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
	FShanmenSwordRhythmReceipt& OutReceipt,
	FString& OutDiagnostic)
{
	OutReceipt = FShanmenSwordRhythmReceipt();
	OutDiagnostic.Reset();
	if (!IsValid() || IsEmpty())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm observation requires one valid active product Host.");
		return false;
	}
	if (!ProductResult.IsExecuted())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm observation rejects a BasicSword result that did not close normally.");
		return false;
	}
	if (ProductResult.Action.GetRunId() != RunId)
	{
		OutDiagnostic =
			TEXT("Sword-rhythm observation rejects a BasicSword action from another Run.");
		return false;
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId()
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm observation requires an immutable sample from this Run's canonical timeline.");
		return false;
	}

	FShanmenSwordRhythmObservation Observation;
	if (!FShanmenSwordRhythmObservation::TryCapture(
			ProductResult.Action,
			TimelineSample.GetTimelineId(),
			TimelineSample.GetCurrentTick(),
			Observation))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm observation could not freeze the accepted BasicSword fact.");
		return false;
	}

	FShanmenSwordRhythmChain CandidateChain = Chain;
	FShanmenSwordRhythmReceipt CandidateReceipt;
	if (!CandidateChain.TryObserve(Observation, CandidateReceipt))
	{
		OutDiagnostic =
			TEXT("Sword-rhythm pure chain rejected the observation without mutation.");
		return false;
	}
	Fdemo_mapShanmenSwordRhythmProductHost Candidate = *this;
	Candidate.Chain = CandidateChain;
	if (!Candidate.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Host rejected an invalid post-observation state.");
		return false;
	}

	*this = Candidate;
	OutReceipt = CandidateReceipt;
	OutDiagnostic =
		TEXT("Completed product BasicSword action was recorded on the Run timeline.");
	return true;
}

bool Fdemo_mapShanmenSwordRhythmProductHost::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Host end requires valid state and Run identity.");
		return false;
	}
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Sword-rhythm product Host is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Sword-rhythm product Host rejects mismatched Run teardown.");
		return false;
	}
	Reset();
	OutDiagnostic = TEXT("Sword-rhythm product Host ended with its Run.");
	return true;
}

void Fdemo_mapShanmenSwordRhythmProductHost::Reset()
{
	*this = Fdemo_mapShanmenSwordRhythmProductHost();
}

bool Fdemo_mapShanmenSwordRhythmProductHost::IsValid() const
{
	if (!RunId.IsValid())
	{
		return !Definition.IsValid() && !Chain.IsValid();
	}
	if (!Definition.IsValid()
		|| !Chain.IsValid()
		|| Chain.GetDefinition().GetDefinitionId()
			!= Definition.GetDefinitionId())
	{
		return false;
	}
	if (Chain.NumRecordedObservations() == 0)
	{
		return !Chain.GetLastObservation().IsValid();
	}
	const FShanmenSwordRhythmObservation& Last = Chain.GetLastObservation();
	return Last.IsValid()
		&& Last.GetAction().GetRunId() == RunId
		&& Last.GetTimelineId()
			== Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId);
}

bool Fdemo_mapShanmenSwordRhythmProductHost::IsEmpty() const
{
	return IsValid() && !RunId.IsValid();
}
