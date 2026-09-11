#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemTypes.h"
#include "demo_mapShanmenArmorResistanceProjection.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenArmorResistanceItemStatus : uint8
{
	Projected,
	NotApplicable,
	NoArmorEquipped,
	AuthorityNotReady,
	RunCorrelationInvalid,
	RunMismatch,
	SnapshotUnavailable,
	SnapshotStale,
	InputInvalid,
	ItemNotFound,
	ItemNotDeployed,
	DefinitionUnavailable,
	DefinitionMismatch,
	DeploymentEvidenceInvalid,
	ProjectionRejected
};

/** Read-only proof of the exact active-Run ArmorSlot item used by projection. */
struct Fdemo_mapShanmenArmorResistanceItemEvidence
{
	FGuid CorrelationId;
	FGuid ActiveRunId;
	FGuid OwnerId;
	FGuid ArmorItemInstanceId;
	FName ArmorDefinitionId = NAME_None;
	FGuid DeploymentReservationId;
	int32 AuthorityRevision = INDEX_NONE;
	int32 ItemRevision = INDEX_NONE;
	int32 DeploymentItemRevisionAtReserve = INDEX_NONE;
	FShanmenContentStamp AuthorityContent;
	FName CatalogContentVersionId = NAME_None;
	FString CatalogContentDigest;

	bool IsValid() const;
	bool operator==(
		const Fdemo_mapShanmenArmorResistanceItemEvidence& Other) const;
};

/** One atomic authority-selection and pure-projection result. */
struct Fdemo_mapShanmenArmorResistanceItemResult
{
	Edemo_mapShanmenArmorResistanceItemStatus Status =
		Edemo_mapShanmenArmorResistanceItemStatus::InputInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenArmorResistanceItemEvidence Evidence;
	Fdemo_mapShanmenArmorResistanceProjectionResult Projection;

	bool IsSuccess() const;
	bool HasArmorEvidence() const { return Evidence.IsValid(); }
	bool HasProjection() const;
};

/**
 * One-way read-only bridge from the durable active-Run ArmorSlot authority to
 * the P26.0 pure resistance projection. It never chooses an item, mutates the
 * authority, or invents resistance tuning absent from the canonical catalog.
 */
struct Fdemo_mapShanmenArmorResistanceItemAdapter
{
	/** Product facade: captures the current durable Run and item snapshot. */
	static Fdemo_mapShanmenArmorResistanceItemResult ProjectActiveRun(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FGuid& ExpectedActiveRunId,
		const FGuid& TargetEntityId,
		const FShanmenDefenseSnapshot& BaseDefense);

	/** Pure evidence gate used after the product captures immutable values. */
	static Fdemo_mapShanmenArmorResistanceItemResult ProjectFromEvidence(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FGuid& ExpectedActiveRunId,
		const FGuid& TargetEntityId,
		const FShanmenDefenseSnapshot& BaseDefense);
};
