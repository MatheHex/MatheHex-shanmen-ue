#pragma once

#include "CoreMinimal.h"
#include "ShanmenFormationDeployment.h"
#include "demo_mapShanmenFormationScatterResourceCommit.h"

class FShanmenActionOrchestrator;

enum class Edemo_mapShanmenFormationScatterDeploymentCommitStatus : uint8
{
	Invalid,
	Committed,
	Replayed,
	ResourceEvidenceInvalid,
	ActionRuntimeInvalid,
	DeploymentInvalid,
	IdentityMismatch,
	DeploymentTerminal,
	AnchorShapeInvalid,
	ExistingAnchorConflict,
	AnchorCommitRejected,
	CompletionEvidenceInvalid
};

/** One exact P27.20 anchor fulfillment accepted by the deployment kernel. */
class Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff
{
public:
	bool IsValid() const;
	const FGuid& GetHandoffId() const { return HandoffId; }
	const FGuid& GetResourceEvidenceId() const
	{
		return ResourceEvidenceId;
	}
	const Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment&
	GetResourceFulfillment() const
	{
		return ResourceFulfillment;
	}
	const FShanmenFormationAnchorFulfillmentEvidence&
	GetDeploymentEvidence() const
	{
		return DeploymentEvidence;
	}
	const FShanmenFormationDeploymentReceipt& GetDeploymentReceipt() const
	{
		return DeploymentReceipt;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterDeploymentCommitter;
	friend class Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence;

	static FGuid BuildHandoffId(
		const Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff&
			Handoff);

	FGuid HandoffId;
	FGuid ResourceEvidenceId;
	Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment
		ResourceFulfillment;
	FShanmenFormationAnchorFulfillmentEvidence DeploymentEvidence;
	FShanmenFormationDeploymentReceipt DeploymentReceipt;
};

/**
 * Immutable proof that every committed scatter resource fulfillment was
 * published to the matching deployment anchor in canonical order.
 */
class Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence
{
public:
	bool IsValid() const;
	const FGuid& GetEvidenceId() const { return EvidenceId; }
	const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
	GetResourceEvidence() const
	{
		return ResourceEvidence;
	}
	const FShanmenFormationDeployment& GetDeployment() const
	{
		return Deployment;
	}
	const TArray<
		Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff>&
	GetHandoffs() const
	{
		return Handoffs;
	}
	int64 GetTotalCommittedQuantity() const
	{
		return TotalCommittedQuantity;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterDeploymentCommitter;

	static FGuid BuildEvidenceId(
		const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
			Evidence);

	FGuid EvidenceId;
	Fdemo_mapShanmenFormationScatterResourceCommitEvidence ResourceEvidence;
	FShanmenFormationDeployment Deployment;
	TArray<Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff>
		Handoffs;
	int64 TotalCommittedQuantity = 0;
};

/** Result of one bounded copy-validate-publish deployment commit. */
struct Fdemo_mapShanmenFormationScatterDeploymentCommitResult
{
	Edemo_mapShanmenFormationScatterDeploymentCommitStatus Status =
		Edemo_mapShanmenFormationScatterDeploymentCommitStatus::Invalid;
	FString Diagnostic;
	int32 InitialCommittedAnchorCount = INDEX_NONE;
	TArray<FShanmenFormationDeploymentReceipt> NewCommitReceipts;
	Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence Evidence;

	bool IsValid() const;
	bool IsCommitted() const;
};

/**
 * Runtime-only bridge from P27.20 resource proof to the existing formation
 * deployment kernel. It commits a candidate copy in canonical anchor order,
 * validates the complete result, and only then publishes the new deployment.
 * It owns no item mutation, World, Actor, input, timing, persistence or UI.
 */
struct Fdemo_mapShanmenFormationScatterDeploymentCommitter
{
	static Fdemo_mapShanmenFormationScatterDeploymentCommitResult Commit(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			ResourceEvidence);

	/** Deterministic bridge for one canonical anchor fulfillment. */
	static bool BuildAnchorEvidence(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			ResourceEvidence,
		int32 AnchorIndex,
		FShanmenFormationAnchorFulfillmentEvidence& OutEvidence);

private:
	static bool BuildCompletionEvidence(
		const Fdemo_mapShanmenFormationScatterResourceCommitEvidence&
			ResourceEvidence,
		const FShanmenFormationDeployment& Deployment,
		Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
			OutEvidence);
};
