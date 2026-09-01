#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.h"

#include "demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.generated.h"

/**
 * Immutable evidence that one complete cue invocation was published to the
 * caller-owned presentation outbox.
 *
 * Publication is the executor's terminal responsibility. This value does not
 * claim that an animation, VFX, or audio asset has already played.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Other)
		const;
	const FGuid& GetHandoffId() const { return HandoffId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
		GetInvocation() const
	{
		return Invocation;
	}
	Edemo_mapShanmenSwordRhythmEffectCueChannel GetChannel() const
	{
		return Invocation.GetRoute().GetChannel();
	}
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& GetCommands()
		const
	{
		return Invocation.GetCommands();
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor;

	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
			Invocation,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|PresentationHandoff", meta = (AllowPrivateAccess = "true"))
	FGuid HandoffId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|PresentationHandoff", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation Invocation;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus : uint8
{
	Consumed,
	AlreadyConsumed,
	ExecutorInvalid,
	HandoffIdInvalid,
	NoPendingHandoff,
	PendingHandoffMismatch,
	StateInvalid
};

struct Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult
{
	Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
			ExecutorInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Handoff;

	bool IsSuccess() const;
};

/**
 * Run-scoped concrete executor that publishes all-or-nothing cue batches into
 * one caller-readable presentation outbox.
 *
 * Succeeded means the immutable batch was accepted by this in-memory handoff
 * boundary. It deliberately does not mean that a presentation asset played.
 * One channel-specific instance has at most one pending handoff, preserves
 * accepted invocation evidence for exact replay, and owns no World, Actor,
 * UObject, asset, timer, scheduler, gameplay mutation, or persistence.
 */
class Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor final
	: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		Edemo_mapShanmenSwordRhythmEffectCueChannel Channel,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor&
			OutExecutor);

	virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
		override;

	bool IsValid() const;
	bool HasPendingHandoff() const
	{
		return IsValid() && bHasPendingHandoff;
	}
	const FGuid& GetRunId() const { return RunId; }
	Edemo_mapShanmenSwordRhythmEffectCueChannel GetChannel() const
	{
		return Channel;
	}
	int32 GetAcceptedInvocationCount() const
	{
		return AcceptedInvocations.Num();
	}
	bool TryGetPendingHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
		const;
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult
		Consume(const FGuid& HandoffId);
	void Reset();

private:
	struct FAcceptedInvocationRecord
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Handoff;
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
		bool bConsumed = false;
	};

	const FAcceptedInvocationRecord* FindAcceptedInvocation(
		const FGuid& InvocationId) const;
	FAcceptedInvocationRecord* FindAcceptedHandoff(const FGuid& HandoffId);
	const FAcceptedInvocationRecord* FindAcceptedHandoff(
		const FGuid& HandoffId) const;

	FGuid RunId;
	Edemo_mapShanmenSwordRhythmEffectCueChannel Channel =
		Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;
	bool bHasPendingHandoff = false;
	FGuid PendingHandoffId;
	TArray<FAcceptedInvocationRecord> AcceptedInvocations;
};
