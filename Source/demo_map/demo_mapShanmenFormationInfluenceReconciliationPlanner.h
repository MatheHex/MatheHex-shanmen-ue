#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceIntentPlanner.h"

enum class Edemo_mapShanmenFormationInfluenceReconciliationMode : uint8
{
	Prime,
	Rebase,
	Reset,
	Terminal
};

/** One immutable Area/policy/coverage scope used as old or new truth. */
struct Fdemo_mapShanmenFormationInfluenceScope
{
	Fdemo_mapShanmenFormationAreaSnapshot Area;
	Fdemo_mapShanmenFormationInfluencePolicy Policy;
	Fdemo_mapShanmenFormationCoverageReceipt Coverage;

	bool IsValid() const;
};

/**
 * Self-contained lifecycle reconciliation evidence.
 *
 * Prime has Current only. Rebase has Previous and Current. Reset/Terminal have
 * Previous only. Removes are canonical before applies, and every intent cites
 * ReconciliationId as its exact cause.
 */
struct Fdemo_mapShanmenFormationInfluenceReconciliationBatch
{
	FGuid BatchId;
	FGuid ReconciliationId;
	Edemo_mapShanmenFormationInfluenceReconciliationMode Mode =
		Edemo_mapShanmenFormationInfluenceReconciliationMode::Prime;
	FGuid SourceEntityId;
	TOptional<Fdemo_mapShanmenFormationInfluenceScope> Previous;
	TOptional<Fdemo_mapShanmenFormationInfluenceScope> Current;
	TArray<Fdemo_mapShanmenFormationInfluenceIntent> Intents;
	int32 ApplyCount = 0;
	int32 RemoveCount = 0;

	bool IsValid() const;
	bool IsNoOp() const
	{
		return IsValid() && Intents.IsEmpty();
	}
};

enum class Edemo_mapShanmenFormationInfluenceReconciliationStatus : uint8
{
	Planned,
	ModeInvalid,
	SourceInvalid,
	PreviousScopeInvalid,
	CurrentScopeInvalid,
	ScopeMismatch,
	ReceiptRejected
};

struct Fdemo_mapShanmenFormationInfluenceReconciliationResult
{
	Edemo_mapShanmenFormationInfluenceReconciliationStatus Status =
		Edemo_mapShanmenFormationInfluenceReconciliationStatus::ModeInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceReconciliationBatch Batch;

	bool IsSuccess() const;
};

/** Pure planner for non-Advance influence lifecycle reconciliation. */
class Fdemo_mapShanmenFormationInfluenceReconciliationPlanner
{
public:
	static Fdemo_mapShanmenFormationInfluenceReconciliationResult PlanPrime(
		const FGuid& SourceEntityId,
		const Fdemo_mapShanmenFormationInfluenceScope& Current);
	static Fdemo_mapShanmenFormationInfluenceReconciliationResult PlanRebase(
		const FGuid& SourceEntityId,
		const Fdemo_mapShanmenFormationInfluenceScope& Previous,
		const Fdemo_mapShanmenFormationInfluenceScope& Current);
	static Fdemo_mapShanmenFormationInfluenceReconciliationResult PlanClear(
		Edemo_mapShanmenFormationInfluenceReconciliationMode Mode,
		const FGuid& SourceEntityId,
		const Fdemo_mapShanmenFormationInfluenceScope& Previous);
};
