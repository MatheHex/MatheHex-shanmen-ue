#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionDriver.h"

enum class Edemo_mapShanmenSwordRhythmEffectCueHostStatus : uint8
{
	Completed,
	RetryPending,
	ConsumerRejected,
	HostInvalid,
	EventInvalid,
	AttemptInvalid,
	StateInvalid
};

/** Aggregate evidence for one deterministic Visual-then-Audio host pass. */
struct Fdemo_mapShanmenSwordRhythmEffectCueHostResult
{
	Edemo_mapShanmenSwordRhythmEffectCueHostStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueHostStatus::StateInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueDriverResult Visual;
	Fdemo_mapShanmenSwordRhythmEffectCueDriverResult Audio;

	bool IsSuccess() const;
	bool IsComplete() const;
	int32 NumAcknowledgedConsumers() const;
};

/**
 * Run-local owner of the canonical Visual and Audio consumer coordinators.
 *
 * Processing is caller-driven and deterministic: Visual is handled before
 * Audio, while acknowledgement and retry state remain independent. The host
 * owns no executors, assets, World state, timers, threads or ProductSession.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		const FGuid& VisualConsumerId,
		const FGuid& AudioConsumerId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost& OutHost);

	Fdemo_mapShanmenSwordRhythmEffectCueHostResult Process(
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event,
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsEmpty() const;
	const FGuid& GetRunId() const
	{
		return VisualCoordinator.GetScope().GetRunId();
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator&
	GetVisualCoordinator() const
	{
		return VisualCoordinator;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator&
	GetAudioCoordinator() const
	{
		return AudioCoordinator;
	}

private:
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator VisualCoordinator;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator AudioCoordinator;
};
