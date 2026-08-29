#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceExecutorAdapter.h"

/** Semantic identity of one active formation influence, independent of cause. */
struct Fdemo_mapShanmenFormationInfluenceLeaseKey
{
	FGuid RunId;
	FGuid OwnerId;
	FGuid SourceEntityId;
	FGuid DeploymentId;
	FGuid AreaId;
	FGuid SubjectEntityId;
	FName PolicyDefinitionId = NAME_None;
	FName InfluenceDefinitionId = NAME_None;
	FShanmenContentStamp Content;

	static bool TryFromIntent(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		Fdemo_mapShanmenFormationInfluenceLeaseKey& OutKey);
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Other) const;
};

/** Immutable read evidence for one currently active lease. */
struct Fdemo_mapShanmenFormationInfluenceLeaseSnapshot
{
	FGuid LeaseId;
	Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
	FGuid ApplyIntentId;

	bool IsValid() const;
};

/**
 * Pure-value in-memory executor for formation influence leases.
 *
 * It owns semantic Apply/Remove state and executor-side attempt replay only.
 * ProductHost/dispatch ledger remain the sole pending, ordering,
 * acknowledgement, retry, and seal authority. No World, Actor, GAS, timer,
 * async, persistence, magnitude, duration, or stacking behavior lives here.
 */
class Fdemo_mapShanmenFormationInfluenceLeaseExecutor final
	: public Idemo_mapShanmenFormationInfluenceExecutor
{
public:
	virtual Fdemo_mapShanmenFormationInfluenceExecutorResult Execute(
		const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation)
		override;

	bool IsConsistent() const;
	int32 GetActiveLeaseCount() const { return ActiveLeases.Num(); }
	int32 GetCompletedIntentCount() const { return CompletedIntents.Num(); }
	int32 GetAttemptCount() const { return Attempts.Num(); }
	bool TryGetActiveLease(
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key,
		Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& OutLease) const;
	bool TryGetAttemptResult(
		const FGuid& AttemptId,
		Fdemo_mapShanmenFormationInfluenceExecutorResult& OutResult) const;

private:
	struct FCompletedIntentRecord
	{
		Fdemo_mapShanmenFormationInfluenceIntent Intent;
		Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
	};

	struct FAttemptRecord
	{
		Fdemo_mapShanmenFormationInfluenceExecutorInvocation Invocation;
		Fdemo_mapShanmenFormationInfluenceExecutorResult Result;
	};

	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot* FindActiveLease(
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key);
	const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot* FindActiveLease(
		const Fdemo_mapShanmenFormationInfluenceLeaseKey& Key) const;
	const FCompletedIntentRecord* FindCompletedIntent(
		const FGuid& IntentId) const;
	const FAttemptRecord* FindAttempt(const FGuid& AttemptId) const;

	TArray<Fdemo_mapShanmenFormationInfluenceLeaseSnapshot> ActiveLeases;
	TArray<FCompletedIntentRecord> CompletedIntents;
	TArray<FAttemptRecord> Attempts;
};
