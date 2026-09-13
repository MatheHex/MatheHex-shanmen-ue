#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationScatterDeploymentCommit.h"
#include "demo_mapShanmenFormationWorldAdapter.h"

enum class Edemo_mapShanmenFormationScatterWorldPlacementHandoffStatus : uint8
{
	Invalid,
	Ready,
	DeploymentEvidenceInvalid,
	PlacementIntentRejected,
	CompletionEvidenceInvalid
};

/** One canonical P27.21 anchor mapped to the existing world-placement intent. */
class Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff
{
public:
	bool IsValid() const;
	const FGuid& GetHandoffId() const { return HandoffId; }
	const FGuid& GetDeploymentCommitEvidenceId() const
	{
		return DeploymentCommitEvidenceId;
	}
	const FGuid& GetDeploymentHandoffId() const
	{
		return DeploymentHandoffId;
	}
	int32 GetAnchorOrder() const { return AnchorOrder; }
	const Fdemo_mapShanmenFormationAnchorPlacementIntent&
	GetPlacementIntent() const
	{
		return PlacementIntent;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder;

	static FGuid BuildHandoffId(
		const Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff&
			Handoff);

	FGuid HandoffId;
	FGuid DeploymentCommitEvidenceId;
	FGuid DeploymentHandoffId;
	int32 AnchorOrder = INDEX_NONE;
	Fdemo_mapShanmenFormationAnchorPlacementIntent PlacementIntent;
};

/**
 * Immutable, side-effect-free proof that every P27.21 anchor has one exact
 * existing world-placement intent in canonical deployment order.
 */
class Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence
{
public:
	bool IsValid() const;
	const FGuid& GetEvidenceId() const { return EvidenceId; }
	const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
	GetDeploymentEvidence() const
	{
		return DeploymentEvidence;
	}
	const TArray<
		Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff>&
	GetHandoffs() const
	{
		return Handoffs;
	}
	int64 GetTotalCommittedQuantity() const
	{
		return TotalCommittedQuantity;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			Other) const;

private:
	friend struct Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder;

	static FGuid BuildEvidenceId(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			Evidence);

	FGuid EvidenceId;
	Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence
		DeploymentEvidence;
	TArray<Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff>
		Handoffs;
	int64 TotalCommittedQuantity = 0;
};

/** Result of one bounded, read-only P27.21-to-placement conversion. */
struct Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult
{
	Edemo_mapShanmenFormationScatterWorldPlacementHandoffStatus Status =
		Edemo_mapShanmenFormationScatterWorldPlacementHandoffStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Evidence;

	bool IsValid() const;
	bool IsReady() const;
};

/**
 * Pure bridge from immutable P27.21 completion evidence to the already
 * established placement-intent contract. It neither publishes placement nor
 * mutates resource or deployment state.
 */
struct Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder
{
	static Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult Build(
		const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
			DeploymentEvidence);

	static bool BuildAnchorHandoff(
		const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
			DeploymentEvidence,
		int32 AnchorIndex,
		Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff&
			OutHandoff);
};
