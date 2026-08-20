// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"

#include "Templates/UniquePtr.h"

/** Read-only P6 + Run-local target composition for the transient P3/P4 page. */
struct FCodeBRunContainerPresentationFrame
{
	TUniquePtr<demo_map_code_b::FCodeBRepository> Repository;
	demo_map_code_b::FCodeBP2PlayerLayout Layout;
	FCodeBHotbarProjection HotbarProjection;
	int32 P6SnapshotRevision = INDEX_NONE;
};

struct FCodeBNormalContainerPresentationFrame
{
	FCodeBRunContainerPresentationFrame Shared;
	FCodeBNormalContainerProjection Projection;
};

struct FCodeBBodyContainerPresentationFrame
{
	FCodeBRunContainerPresentationFrame Shared;
	FCodeBBodyContainerProjection Projection;
};

/** Immutable actor-to-record identity for the opened P31 WorldDrop page. */
struct FCodeBWorldDropPresentationRequest
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
	int32 Ordinal = 0;
	FGuid WorldContainerId;
	FGuid RootItemId;
	FGuid SpatialChildContainerId;
	FName MapRoute = NAME_None;
	int32 RecordRevision = INDEX_NONE;

	bool IsValid() const
	{
		return OwnerId.IsValid() && RunInstanceId.IsValid() && WorldDropId.IsValid()
			&& Ordinal > 0 && WorldContainerId.IsValid() && RootItemId.IsValid()
			&& !MapRoute.IsNone() && RecordRevision > 0;
	}
};

struct FCodeBWorldDropPresentationFrame
{
	FCodeBRunContainerPresentationFrame Shared;
	FCodeBWorldDropProjection Projection;
	FString Provenance;
};

/** Type-specific, transient source handoff for the active-player page. */
struct FCodeBActivePlayerInteractionCommitRequest
{
	FString StorageRoot;
	FGuid OwnerId;
	FGuid RunInstanceId;
	bool bTransientScopeCurrent = false;
	FCodeBOutOfRaidProfileStore* Store = nullptr;
};

struct FCodeBNormalContainerInteractionCommitRequest
{
	FString StorageRoot;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid TargetId;
	FName DefinitionId = NAME_None;
	int32 ExpectedP6SnapshotRevision = INDEX_NONE;
	int32 ExpectedTargetRevision = INDEX_NONE;
	bool bTransientScopeCurrent = false;
};

struct FCodeBBodyContainerInteractionCommitRequest
{
	FString StorageRoot;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid TargetId;
	FName DefinitionId = NAME_None;
	int32 ExpectedP6SnapshotRevision = INDEX_NONE;
	int32 ExpectedTargetRevision = INDEX_NONE;
	bool bTransientScopeCurrent = false;
};

struct FCodeBWorldDropInteractionCommitRequest
{
	FString StorageRoot;
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
	int32 ExpectedP6SnapshotRevision = INDEX_NONE;
	int32 ExpectedWorldDropOrdinal = 0;
	int32 ExpectedWorldDropRecordRevision = INDEX_NONE;
	bool bTransientScopeCurrent = false;
};

struct FCodeBActivePlayerInteractionCommitResult
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	int32 P6SnapshotRevision = INDEX_NONE;
};

struct FCodeBNormalContainerInteractionCommitResult
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid TargetId;
	int32 P6SnapshotRevision = INDEX_NONE;
	int32 TargetRevision = INDEX_NONE;
	TOptional<FCodeBNormalContainerProjection> Projection;
};

struct FCodeBBodyContainerInteractionCommitResult
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid TargetId;
	int32 P6SnapshotRevision = INDEX_NONE;
	int32 TargetRevision = INDEX_NONE;
	TOptional<FCodeBBodyContainerProjection> Projection;
};

/** Missing projection is an explicit no-longer-available WorldDrop, never an ordinary failure. */
struct FCodeBWorldDropInteractionCommitResult
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
	int32 P6SnapshotRevision = INDEX_NONE;
	TOptional<FCodeBWorldDropProjection> Projection;
	bool bReconcileActors = false;
};

/**
 * Projection-side source adapter. It owns exact Owner/Run/target read checks
 * and transient P6 + target graph composition; Store and P1 retain all
 * durable and item-graph authority.
 */
class FCodeBRunItemInteractionDomain
{
public:
	static bool BuildAcceptedCompositeSnapshot(
		const demo_map_code_b::FCodeBSnapshot& PlayerSnapshot,
		const demo_map_code_b::FCodeBSnapshot& TargetSnapshot,
		demo_map_code_b::FCodeBSnapshot& OutComposite,
		FString& OutError);
	static bool BuildNormalContainerFrame(
		const FString& StorageRoot, const FGuid& OwnerId, const FGuid& RunInstanceId,
		const FGuid& SearchTargetId, FName ExpectedDefinitionId,
		FCodeBNormalContainerPresentationFrame& OutFrame, FString& OutError);
	static bool BuildBodyContainerFrame(
		const FString& StorageRoot, const FGuid& OwnerId, const FGuid& RunInstanceId,
		const FGuid& BodyTargetId, FName ExpectedDefinitionId,
		FCodeBBodyContainerPresentationFrame& OutFrame, FString& OutError);
	static bool BuildWorldDropFrame(
		const FString& StorageRoot, const FCodeBWorldDropPresentationRequest& Request,
		FCodeBWorldDropPresentationFrame& OutFrame, FString& OutError);
	static bool CommitAcceptedActivePlayer(
		const FCodeBActivePlayerInteractionCommitRequest& Request,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
		FCodeBActivePlayerInteractionCommitResult& OutResult, FString& OutError);
	static bool CommitAcceptedNormalContainer(
		const FCodeBNormalContainerInteractionCommitRequest& Request,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
		FCodeBNormalContainerInteractionCommitResult& OutResult, FString& OutError);
	static bool CommitAcceptedBodyContainer(
		const FCodeBBodyContainerInteractionCommitRequest& Request,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
		FCodeBBodyContainerInteractionCommitResult& OutResult, FString& OutError);
	static bool CommitAcceptedWorldDrop(
		const FCodeBWorldDropInteractionCommitRequest& Request,
		const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
		const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
		FCodeBWorldDropInteractionCommitResult& OutResult, FString& OutError);
};
/** Compatibility spelling for P72.2 consumers; production P72.3 paths use the domain name above. */
using FCodeBRunContainerPresentationFactory = FCodeBRunItemInteractionDomain;
