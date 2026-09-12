#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationDiagramSelectionPort.h"

/** Mutable transport returned by the existing progression/save authority. */
struct Fdemo_mapShanmenFormationKnowledgeAuthorityCapture
{
	FGuid OwnerId;
	int64 AuthorityRevision = INDEX_NONE;
	FGuid CatalogId;
	FShanmenContentStamp Content;
	TArray<FName> KnownDiagramDefinitionIds;
};

/**
 * Immutable evidence from one formation-knowledge authority read.
 *
 * This value proves internal identity consistency only. The adapter still
 * verifies owner and active-catalog correspondence before projecting it.
 */
class Fdemo_mapShanmenFormationKnowledgeAuthorityRead
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& Capture,
		Fdemo_mapShanmenFormationKnowledgeAuthorityRead& OutRead,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetReadId() const { return ReadId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	const FGuid& GetCatalogId() const { return CatalogId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const TArray<FName>& GetKnownDiagramDefinitionIds() const
	{
		return KnownDiagramDefinitionIds;
	}

private:
	static FGuid BuildReadId(
		const FGuid& OwnerId,
		int64 AuthorityRevision,
		const FGuid& CatalogId,
		const FShanmenContentStamp& Content,
		const TArray<FName>& KnownDiagramDefinitionIds);

	FGuid ReadId;
	FGuid OwnerId;
	int64 AuthorityRevision = INDEX_NONE;
	FGuid CatalogId;
	FShanmenContentStamp Content;
	TArray<FName> KnownDiagramDefinitionIds;
};

enum class Edemo_mapShanmenFormationKnowledgeProjectionStatus : uint8
{
	Invalid,
	Projected,
	CatalogInvalid,
	OwnerInvalid,
	AuthorityUnavailable,
	AuthorityReadInvalid,
	OwnerMismatch,
	CatalogMismatch,
	KnowledgeRejected
};

/** Auditable result of one bounded authority read and projection. */
struct Fdemo_mapShanmenFormationKnowledgeProjectionResult
{
	Edemo_mapShanmenFormationKnowledgeProjectionStatus Status =
		Edemo_mapShanmenFormationKnowledgeProjectionStatus::Invalid;
	FString Diagnostic;
	int32 AuthorityReadCount = 0;
	Fdemo_mapShanmenFormationKnowledgeAuthorityRead AuthorityRead;
	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot Knowledge;

	bool IsValid() const;
	bool IsProjected() const;
};

/**
 * Single-direction, read-only adapter from progression/save authority to the
 * P27.6 formation-diagram knowledge contract.
 *
 * It owns no profile schema, unlock mutation, retry, inventory, loot rule,
 * World, Actor, UI, input, catalog loading, or product lifecycle.
 */
struct Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter
{
	using FReadAuthority = TFunctionRef<bool(
		const FGuid& RequestedOwnerId,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString& OutDiagnostic)>;

	static Fdemo_mapShanmenFormationKnowledgeProjectionResult Project(
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		const FGuid& RequestedOwnerId,
		FReadAuthority ReadAuthority);
};
