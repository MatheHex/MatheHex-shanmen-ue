#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationKnowledgeAuthorityAdapter.h"

enum class Edemo_mapShanmenFormationDiagramAccessStatus : uint8
{
	Invalid,
	Selected,
	CatalogInvalid,
	OwnerInvalid,
	RequestInvalid,
	DiagramUnknown,
	KnowledgeProjectionRejected,
	SelectionRejected
};

/**
 * Auditable result of one bounded formation-diagram access resolution.
 *
 * Successful evidence binds the active catalog, requested owner, one
 * authority revision and one selected diagram. Rejected nested results are
 * retained so callers can distinguish unavailable authority, stale content,
 * malformed claims and an authored-but-unknown-to-the-owner diagram.
 */
struct Fdemo_mapShanmenFormationDiagramAccessResult
{
	Edemo_mapShanmenFormationDiagramAccessStatus Status =
		Edemo_mapShanmenFormationDiagramAccessStatus::Invalid;
	FString Diagnostic;
	FGuid ActiveCatalogId;
	FShanmenContentStamp ActiveContent;
	FGuid RequestedOwnerId;
	FName RequestedDiagramDefinitionId;
	int32 KnowledgeProjectionCount = 0;
	int32 SelectionInvocationCount = 0;
	Fdemo_mapShanmenFormationKnowledgeProjectionResult KnowledgeProjection;
	Fdemo_mapShanmenFormationDiagramSelectionResult Selection;

	bool IsValid() const;
	bool IsSelected() const;
};

/**
 * One-call read-only composition of the active authored catalog, the existing
 * progression/save knowledge authority and the P27.6 selection contract.
 *
 * It owns no catalog loading, save schema, unlock mutation, loot rule,
 * inventory, UI, input, retry, World, Actor or deployment lifecycle.
 */
struct Fdemo_mapShanmenFormationDiagramAccessAdapter
{
	using FReadAuthority =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::FReadAuthority;

	static Fdemo_mapShanmenFormationDiagramAccessResult Resolve(
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		const FGuid& RequestedOwnerId,
		FName RequestedDiagramDefinitionId,
		FReadAuthority ReadAuthority);
};
