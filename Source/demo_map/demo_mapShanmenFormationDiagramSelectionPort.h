#pragma once

#include "CoreMinimal.h"
#include "ShanmenCoreTypes.h"
#include "ShanmenFormationDeployment.h"

/** Mutable authored input for one versioned formation-diagram catalog. */
struct Fdemo_mapShanmenFormationDiagramCatalogCapture
{
	FShanmenContentStamp Content;
	TArray<FShanmenFormationDiagramCapture> Diagrams;
};

/** Immutable, content-stamped set of authored formation diagrams. */
class Fdemo_mapShanmenFormationDiagramCatalog
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenFormationDiagramCatalogCapture& Capture,
		Fdemo_mapShanmenFormationDiagramCatalog& OutCatalog,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetCatalogId() const { return CatalogId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const TArray<FShanmenFormationDiagramDefinition>& GetDiagrams() const
	{
		return Diagrams;
	}
	const FShanmenFormationDiagramDefinition* FindDiagram(
		FName DiagramDefinitionId) const;

private:
	static FGuid BuildCatalogId(
		const FShanmenContentStamp& Content,
		const TArray<FShanmenFormationDiagramDefinition>& Diagrams);

	FGuid CatalogId;
	FShanmenContentStamp Content;
	TArray<FShanmenFormationDiagramDefinition> Diagrams;
};

/**
 * Immutable proof of which authored diagrams one owner currently knows.
 *
 * A future progression/save authority produces this snapshot. This contract
 * validates and binds its evidence but does not own or mutate unlock state.
 */
class Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		const FGuid& OwnerId,
		int64 AuthorityRevision,
		const TArray<FName>& KnownDiagramDefinitionIds,
		Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot& OutSnapshot,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool Contains(FName DiagramDefinitionId) const;
	const FGuid& GetSnapshotId() const { return SnapshotId; }
	const FGuid& GetCatalogId() const { return CatalogId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const TArray<FName>& GetKnownDiagramDefinitionIds() const
	{
		return KnownDiagramDefinitionIds;
	}

private:
	static FGuid BuildSnapshotId(
		const FShanmenContentStamp& Content,
		const FGuid& CatalogId,
		const FGuid& OwnerId,
		int64 AuthorityRevision,
		const TArray<FName>& KnownDiagramDefinitionIds);

	FGuid SnapshotId;
	FGuid CatalogId;
	FGuid OwnerId;
	int64 AuthorityRevision = INDEX_NONE;
	FShanmenContentStamp Content;
	TArray<FName> KnownDiagramDefinitionIds;
};

/** Immutable owner-bound selection of one known authored diagram. */
class Fdemo_mapShanmenFormationDiagramSelection
{
public:
	bool IsValid() const;
	const FGuid& GetSelectionId() const { return SelectionId; }
	const FGuid& GetCatalogId() const { return CatalogId; }
	const FGuid& GetKnowledgeSnapshotId() const
	{
		return KnowledgeSnapshotId;
	}
	const FGuid& GetOwnerId() const { return OwnerId; }
	int64 GetKnowledgeAuthorityRevision() const
	{
		return KnowledgeAuthorityRevision;
	}
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FShanmenFormationDiagramDefinition& GetDiagram() const
	{
		return Diagram;
	}

private:
	friend struct Fdemo_mapShanmenFormationDiagramSelectionPort;

	static FGuid BuildSelectionId(
		const FGuid& CatalogId,
		const FGuid& KnowledgeSnapshotId,
		const FGuid& OwnerId,
		int64 KnowledgeAuthorityRevision,
		const FShanmenContentStamp& Content,
		const FShanmenFormationDiagramDefinition& Diagram);

	FGuid SelectionId;
	FGuid CatalogId;
	FGuid KnowledgeSnapshotId;
	FGuid OwnerId;
	int64 KnowledgeAuthorityRevision = INDEX_NONE;
	FShanmenContentStamp Content;
	FShanmenFormationDiagramDefinition Diagram;
};

enum class Edemo_mapShanmenFormationDiagramSelectionStatus : uint8
{
	Invalid,
	Selected,
	CatalogInvalid,
	KnowledgeInvalid,
	CatalogMismatch,
	RequestInvalid,
	DiagramUnknown,
	DiagramUnavailable
};

/** Auditable result of resolving one diagram against one knowledge snapshot. */
struct Fdemo_mapShanmenFormationDiagramSelectionResult
{
	Edemo_mapShanmenFormationDiagramSelectionStatus Status =
		Edemo_mapShanmenFormationDiagramSelectionStatus::Invalid;
	Fdemo_mapShanmenFormationDiagramSelection Selection;
	FString Diagnostic;

	bool IsValid() const;
	bool IsSelected() const;
};

/**
 * Stateless authored-content and player-knowledge selection seam.
 *
 * It owns no catalog loading, save data, unlock mutation, inventory, UI,
 * input, World, Actor, GameMode, effect, recipe, retry or product state.
 */
struct Fdemo_mapShanmenFormationDiagramSelectionPort
{
	static Fdemo_mapShanmenFormationDiagramSelectionResult Select(
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		const Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot& Knowledge,
		FName RequestedDiagramDefinitionId);
};
