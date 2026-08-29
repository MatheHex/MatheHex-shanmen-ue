#pragma once

#include "CoreMinimal.h"
#include "ShanmenFormationDeployment.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationMaterialStatus : uint8
{
	PlanReady,
	Prepared,
	Replayed,
	Committed,
	Cancelled,
	AuthorityNotReady,
	RunCorrelationInvalid,
	SnapshotUnavailable,
	SnapshotStale,
	DeploymentInvalid,
	DeploymentMismatch,
	AnchorInvalid,
	LifecycleInvalid,
	ItemAuthorityInvalid,
	QuantityUnavailable,
	ConflictingIntent,
	RequestInvalid,
	PrepareRejectedRolledBack,
	RollbackRecoveryRequired,
	PreparationInvalid,
	AttemptCancelled,
	CommitRecoveryRequired,
	CancelRecoveryRequired,
	EvidenceInvalid
};

/** Frozen requirement copied from one P8.0 anchor for plan self-validation. */
struct Fdemo_mapShanmenFormationMaterialRequirement
{
	FName MaterialDefinitionId = NAME_None;
	int32 Quantity = 0;

	bool IsValid() const
	{
		return !MaterialDefinitionId.IsNone() && Quantity > 0;
	}
};

/** One exact active-Run stack selected for an anchor material transaction. */
struct Fdemo_mapShanmenFormationMaterialPlanLine
{
	FName MaterialDefinitionId = NAME_None;
	FGuid ItemInstanceId;
	int32 Quantity = 0;
	int32 ExpectedQuantityBefore = 0;
	FGuid IntentId;
	FShanmenItemRunQuantityIntentRequest PrepareRequest;
	FShanmenItemTransactionReceipt ExistingPrepareReceipt;
	FShanmenItemTransactionReceipt ExistingFinalizeReceipt;

	bool IsValid() const;
};

/**
 * One restart-reconstructible material attempt for one exact formation anchor.
 * Durable receipts remain the authority; this transient value only correlates
 * their deterministic identities and builds P8.0 fulfillment evidence.
 */
struct Fdemo_mapShanmenFormationMaterialResult
{
	Edemo_mapShanmenFormationMaterialStatus Status =
		Edemo_mapShanmenFormationMaterialStatus::RequestInvalid;
	FString Diagnostic;
	FGuid AttemptId;
	FGuid TransactionId;
	FGuid ScopeId;
	FGuid OwnerId;
	FGuid ActiveRunId;
	FGuid DeploymentId;
	FName AnchorDefinitionId = NAME_None;
	FShanmenContentStamp Content;
	TArray<Fdemo_mapShanmenFormationMaterialRequirement> Requirements;
	TArray<Fdemo_mapShanmenFormationMaterialPlanLine> Lines;
	TArray<FShanmenItemDurableCommandResult> PrepareCommands;
	TArray<FShanmenItemDurableCommandResult> FinalizeCommands;
	TArray<FShanmenItemTransactionReceipt> PrepareReceipts;
	TArray<FShanmenItemTransactionReceipt> FinalizeReceipts;
	FShanmenFormationAnchorFulfillmentEvidence Evidence;

	bool HasPlan() const;
	bool IsPrepared() const;
	bool IsCommitted() const;
	bool IsCancelled() const;
};

/**
 * One-way bridge from P8.0 formation requirements into ShanmenItems.
 *
 * Each selected physical stack uses the existing durable active-Run Quantity
 * intent. Prepare failures cancel every already-prepared line. Once any commit
 * succeeds, the attempt is forward-only and retries complete the remaining
 * deterministic commits. Fulfillment evidence exists only after every line is
 * durably committed; no legacy or Runtime inventory is mutated here.
 */
struct Fdemo_mapShanmenFormationMaterialAdapter
{
	/** Pure snapshot gate and deterministic multi-stack allocation builder. */
	static Fdemo_mapShanmenFormationMaterialResult BuildPlan(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenFormationDeployment& Deployment,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);

	/** Build one terminal request for an exact plan line. */
	static bool BuildFinalizeRequest(
		const Fdemo_mapShanmenFormationMaterialResult& Preparation,
		int32 LineIndex,
		bool bCommit,
		FShanmenItemRunQuantityIntentFinalizeRequest& OutRequest);

	/** Validate committed receipts and produce the sole P8.0 evidence envelope. */
	static Fdemo_mapShanmenFormationMaterialResult BuildCommittedEvidence(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenFormationMaterialResult& Preparation,
		const TArray<FShanmenItemTransactionReceipt>& CommitReceipts);

	/** Capture the ready authority and durably prepare every selected stack. */
	static Fdemo_mapShanmenFormationMaterialResult PrepareMaterials(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenFormationDeployment& Deployment,
		FName AnchorDefinitionId,
		const FGuid& AttemptId);

	/** Commit every prepared line; partial progress is completed by exact replay. */
	static Fdemo_mapShanmenFormationMaterialResult CommitMaterials(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenFormationMaterialResult& Preparation);

	/** Cancel an attempt before any material line has committed. */
	static Fdemo_mapShanmenFormationMaterialResult CancelMaterials(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenFormationMaterialResult& Preparation);
};
