#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.h"

enum class
	Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus : uint8
{
	Published,
	Queued,
	AlreadyCaptured,
	ControllerInvalid,
	SessionInvalid,
	RunMismatch,
	RevisionConflict,
	PrepareRejected,
	DispatchIncomplete,
	StateInvalid
};

/** Evidence for one caller-driven current ProductSession capture. */
struct Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult
{
	Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus::
			ControllerInvalid;
	FString Diagnostic;
	int32 ObservationRevision = 0;
	int32 QueuedDispatchCount = 0;
	int32 PublishedDispatchCount = 0;

	bool IsSuccess() const;
	bool IsPublished() const
	{
		return Status
			== Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus::
				Published;
	}
};

/** Run-teardown evidence captured before presentation-local state is reset. */
struct Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary
{
	FGuid RunId;
	int32 CapturedDispatchCount = 0;
	int32 PublishedDispatchCount = 0;
	int32 QueuedDispatchCount = 0;
	int32 VisualAcceptedInvocationCount = 0;
	int32 AudioAcceptedInvocationCount = 0;
	bool bVisualPending = false;
	bool bAudioPending = false;
};

/**
 * Caller-owned Run integration for ProductSession effect-cue presentation.
 *
 * Every observed presentation revision is frozen into the existing prepared
 * dispatch contract. Revisions wait in strict FIFO order while either
 * channel-specific handoff remains pending. Explicit handoff consumption
 * releases capacity and synchronously pumps the next frozen revision. The
 * controller owns no World, Actor, UObject, asset, timer, thread, gameplay
 * mutation, persistence, or playback-completion claim.
 */
class Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController
{
public:
	bool TryBegin(const FGuid& RunId, FString& OutDiagnostic);
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult
	TryPublishCurrent(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session);

	bool TryGetPendingVisualHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
		const;
	bool TryGetPendingAudioHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
		const;
	bool TryConsumeVisualHandoff(
		const FGuid& HandoffId,
		FString& OutDiagnostic);
	bool TryConsumeAudioHandoff(
		const FGuid& HandoffId,
		FString& OutDiagnostic);
	bool TryAcknowledgeVisualHandoff(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement&
			Acknowledgement,
		FString& OutDiagnostic);
	bool TryAcknowledgeAudioHandoff(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement&
			Acknowledgement,
		FString& OutDiagnostic);

	bool TryEnd(
		const FGuid& ExpectedRunId,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary&
			OutSummary,
		FString& OutDiagnostic);
	bool IsValid() const;
	bool IsEmpty() const;
	const FGuid& GetRunId() const { return RunId; }
	int32 GetLastCapturedRevision() const { return LastCapturedRevision; }
	int32 GetLastPublishedRevision() const { return LastPublishedRevision; }
	int32 GetPublishedDispatchCount() const
	{
		return PublishedDispatchCount;
	}
	int32 GetQueuedDispatchCount() const { return PendingDispatches.Num(); }
	bool HasPendingVisualHandoff() const
	{
		return VisualExecutor.HasPendingHandoff();
	}
	bool HasPendingAudioHandoff() const
	{
		return AudioExecutor.HasPendingHandoff();
	}
	void Reset();

private:
	enum class EPumpStatus : uint8
	{
		Idle,
		Blocked,
		Published,
		DispatchIncomplete,
		StateInvalid
	};

	struct FPumpResult
	{
		EPumpStatus Status = EPumpStatus::Idle;
		FString Diagnostic;
		int32 PublishedCount = 0;
	};

	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed
	MakeSeed(const FGuid& RunId);
	FPumpResult PumpPendingDispatches();
	bool TryConsumeHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor&
			Executor,
		const FGuid& HandoffId,
		FString& OutDiagnostic);
	bool TryAcknowledgeHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor&
			Executor,
		Edemo_mapShanmenSwordRhythmEffectCueChannel ExpectedChannel,
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement&
			Acknowledgement,
		FString& OutDiagnostic);

	FGuid RunId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed Seed;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor
		VisualExecutor;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor
		AudioExecutor;
	TArray<
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch>
		PendingDispatches;
	int32 LastCapturedRevision = 0;
	int32 LastPublishedRevision = 0;
	int32 PublishedDispatchCount = 0;
};
