// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeB/demo_mapCodeBRunContainerPresentation.h"

namespace
{
	bool BuildCompositePresentationSnapshot(
		const demo_map_code_b::FCodeBSnapshot& PlayerSnapshot,
		const demo_map_code_b::FCodeBSnapshot& TargetSnapshot,
		demo_map_code_b::FCodeBSnapshot& OutComposite,
		FString& OutError)
	{
		OutComposite = demo_map_code_b::FCodeBSnapshot();
		OutComposite.Revision = FMath::Max(PlayerSnapshot.Revision, TargetSnapshot.Revision);
		for (const TPair<FName, demo_map_code_b::FCodeBItemDefinition>& Pair : PlayerSnapshot.Definitions)
		{
			OutComposite.Definitions.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FName, demo_map_code_b::FCodeBItemDefinition>& Pair : TargetSnapshot.Definitions)
		{
			if (const demo_map_code_b::FCodeBItemDefinition* Existing = OutComposite.Definitions.Find(Pair.Key);
				Existing && !(*Existing == Pair.Value))
			{
				OutError = TEXT("Code B cannot compose conflicting P6/Run-local item definitions.");
				return false;
			}
			OutComposite.Definitions.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, demo_map_code_b::FCodeBItemInstance>& Pair : PlayerSnapshot.Items)
		{
			OutComposite.Items.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, demo_map_code_b::FCodeBItemInstance>& Pair : TargetSnapshot.Items)
		{
			if (OutComposite.Items.Contains(Pair.Key))
			{
				OutError = TEXT("Code B cannot compose duplicate P6/Run-local item identities.");
				return false;
			}
			OutComposite.Items.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, demo_map_code_b::FCodeBContainer>& Pair : PlayerSnapshot.Containers)
		{
			OutComposite.Containers.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, demo_map_code_b::FCodeBContainer>& Pair : TargetSnapshot.Containers)
		{
			if (OutComposite.Containers.Contains(Pair.Key))
			{
				OutError = TEXT("Code B cannot compose duplicate P6/Run-local container identities.");
				return false;
			}
			OutComposite.Containers.Add(Pair.Key, Pair.Value);
		}
		demo_map_code_b::FCodeBRepository ValidationRepository;
		return ValidationRepository.LoadPersistedSnapshot(OutComposite, &OutError);
	}

	bool BuildSharedFrame(
		FCodeBOutOfRaidProfileStore& Store,
		const FCodeBRunInventorySession& Session,
		const demo_map_code_b::FCodeBSnapshot& TargetSnapshot,
		const FGuid& TargetContainerId,
		const FName TargetPaneId,
		FCodeBRunContainerPresentationFrame& OutFrame,
		FString& OutError)
	{
		OutFrame = FCodeBRunContainerPresentationFrame();
		demo_map_code_b::FCodeBSnapshot CompositeSnapshot;
		if (!BuildCompositePresentationSnapshot(
			Session.RepositorySnapshot, TargetSnapshot, CompositeSnapshot, OutError)) return false;
		TUniquePtr<demo_map_code_b::FCodeBRepository> Repository = MakeUnique<demo_map_code_b::FCodeBRepository>();
		if (!Repository->LoadPersistedSnapshot(CompositeSnapshot, &OutError)) return false;
		if (!Store.TryGetMatchedActiveRunHotbarProjection(
			Session.RunInstanceId, OutFrame.HotbarProjection, &OutError)) return false;
		OutFrame.Layout = Session.Layout;
		OutFrame.Layout.TransientPresentationContainers.Reset();
		OutFrame.Layout.TransientPresentationContainers.Add(TPair<FName, FGuid>(TargetPaneId, TargetContainerId));
		OutFrame.P6SnapshotRevision = Session.RepositorySnapshot.Revision;
		OutFrame.Repository = MoveTemp(Repository);
		return true;
	}
}

bool FCodeBRunItemInteractionDomain::BuildAcceptedCompositeSnapshot(
	const demo_map_code_b::FCodeBSnapshot& PlayerSnapshot,
	const demo_map_code_b::FCodeBSnapshot& TargetSnapshot,
	demo_map_code_b::FCodeBSnapshot& OutComposite,
	FString& OutError)
{
	return BuildCompositePresentationSnapshot(PlayerSnapshot, TargetSnapshot, OutComposite, OutError);
}

bool FCodeBRunItemInteractionDomain::BuildNormalContainerFrame(
	const FString& StorageRoot,
	const FGuid& OwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName ExpectedDefinitionId,
	FCodeBNormalContainerPresentationFrame& OutFrame,
	FString& OutError)
{
	OutFrame = FCodeBNormalContainerPresentationFrame();
	OutError.Reset();
	if (!OwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid() || ExpectedDefinitionId.IsNone())
	{
		OutError = TEXT("Code B normal presentation frame requires exact Owner/Run/target/definition identity.");
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(StorageRoot, OwnerId);
	FCodeBRunInventorySession Session;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &OutError)) return false;
	if (!FCodeBOutOfRaidProfileStore::TryGetMatchedRunNormalContainerProjection(
		StorageRoot, OwnerId, RunInstanceId, SearchTargetId, OutFrame.Projection, &OutError)
		|| OutFrame.Projection.DefinitionId != ExpectedDefinitionId
		|| OutFrame.Projection.State != ECodeBNormalContainerState::Open)
	{
		if (OutError.IsEmpty()) OutError = TEXT("Code B normal presentation frame rejected a stale or non-open target projection.");
		return false;
	}
	const FCodeBRunLocalNormalContainerRecord* TargetRecord = Store.GetRecord().RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value)
		{ return Value.SearchTargetId == SearchTargetId; });
	if (!TargetRecord || TargetRecord->ContainerId != OutFrame.Projection.ContainerId)
	{
		OutError = TEXT("Code B normal presentation frame could not resolve the exact durable target graph.");
		return false;
	}
	return BuildSharedFrame(Store, Session, TargetRecord->ContainerSnapshot,
		OutFrame.Projection.ContainerId, FName(TEXT("NormalContainerTarget")), OutFrame.Shared, OutError);
}

bool FCodeBRunItemInteractionDomain::BuildBodyContainerFrame(
	const FString& StorageRoot,
	const FGuid& OwnerId,
	const FGuid& RunInstanceId,
	const FGuid& BodyTargetId,
	const FName ExpectedDefinitionId,
	FCodeBBodyContainerPresentationFrame& OutFrame,
	FString& OutError)
{
	OutFrame = FCodeBBodyContainerPresentationFrame();
	OutError.Reset();
	if (!OwnerId.IsValid() || !RunInstanceId.IsValid() || !BodyTargetId.IsValid() || ExpectedDefinitionId.IsNone())
	{
		OutError = TEXT("Code B body presentation frame requires exact Owner/Run/body/definition identity.");
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(StorageRoot, OwnerId);
	FCodeBRunInventorySession Session;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &OutError)) return false;
	if (!FCodeBOutOfRaidProfileStore::TryGetMatchedRunBodyContainerProjection(
		StorageRoot, OwnerId, RunInstanceId, BodyTargetId, OutFrame.Projection, &OutError)
		|| OutFrame.Projection.DefinitionId != ExpectedDefinitionId
		|| OutFrame.Projection.State != ECodeBBodyContainerState::Open)
	{
		if (OutError.IsEmpty()) OutError = TEXT("Code B body presentation frame rejected a stale or non-open target projection.");
		return false;
	}
	const FCodeBRunLocalBodyContainerRecord* TargetRecord = Store.GetRecord().RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value)
		{ return Value.BodyTargetId == BodyTargetId; });
	if (!TargetRecord || TargetRecord->ContainerId != OutFrame.Projection.ContainerId)
	{
		OutError = TEXT("Code B body presentation frame could not resolve the exact durable target graph.");
		return false;
	}
	return BuildSharedFrame(Store, Session, TargetRecord->ContainerSnapshot,
		OutFrame.Projection.ContainerId, FName(TEXT("BodyContainerTarget")), OutFrame.Shared, OutError);
}

bool FCodeBRunItemInteractionDomain::BuildWorldDropFrame(
	const FString& StorageRoot,
	const FCodeBWorldDropPresentationRequest& Request,
	FCodeBWorldDropPresentationFrame& OutFrame,
	FString& OutError)
{
	OutFrame = FCodeBWorldDropPresentationFrame();
	OutError.Reset();
	if (!Request.IsValid())
	{
		OutError = TEXT("Code B WorldDrop presentation frame requires one exact Owner/Run/record/graph identity.");
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(StorageRoot, Request.OwnerId);
	FCodeBRunInventorySession Session;
	if (!Store.OpenMatchedActiveRunInventorySession(Request.RunInstanceId, Session, &OutError)) return false;
	const FCodeBWorldDropRecord* Record = Store.GetRecord().ActiveRunInventorySession.WorldDrops.FindByPredicate(
		[&Request](const FCodeBWorldDropRecord& Value)
		{
			return Value.OwnerId == Request.OwnerId
				&& Value.RunInstanceId == Request.RunInstanceId
				&& Value.WorldDropId == Request.WorldDropId
				&& Value.Ordinal == Request.Ordinal
				&& Value.WorldContainerId == Request.WorldContainerId
				&& Value.ItemId == Request.RootItemId
				&& Value.SpatialChildContainerId == Request.SpatialChildContainerId
				&& Value.MapRoute == Request.MapRoute
				&& Value.RecordRevision == Request.RecordRevision
				&& Value.ActionState == ECodeBWorldDropActionState::Available;
		});
	if (!Record)
	{
		OutError = TEXT("Code B WorldDrop presentation frame rejected a stale, moved, or non-available durable record.");
		return false;
	}
	TArray<FCodeBWorldDropProjection> Projections;
	if (!Store.TryGetMatchedActiveRunWorldDropProjections(Request.RunInstanceId, Projections, &OutError)) return false;
	const FCodeBWorldDropProjection* Projection = Projections.FindByPredicate(
		[&Request, &Session](const FCodeBWorldDropProjection& Value)
		{
			return Value.OwnerId == Request.OwnerId
				&& Value.RunInstanceId == Request.RunInstanceId
				&& Value.WorldDropId == Request.WorldDropId
				&& Value.Ordinal == Request.Ordinal
				&& Value.WorldContainerId == Request.WorldContainerId
				&& Value.ItemId == Request.RootItemId
				&& Value.SpatialChildContainerId == Request.SpatialChildContainerId
				&& Value.MapRoute == Request.MapRoute
				&& Value.RecordRevision == Request.RecordRevision
				&& Value.P6SnapshotRevision == Session.RepositorySnapshot.Revision;
		});
	if (!Projection)
	{
		OutError = TEXT("Code B WorldDrop presentation frame could not derive the accepted projection for its exact record.");
		return false;
	}
	if (!BuildSharedFrame(Store, Session, demo_map_code_b::FCodeBSnapshot(),
		Request.WorldContainerId, FName(TEXT("WorldDropTarget")), OutFrame.Shared, OutError)) return false;
	OutFrame.Projection = *Projection;
	OutFrame.Provenance = Record->Provenance;
	return true;
}

bool FCodeBRunItemInteractionDomain::CommitAcceptedActivePlayer(
	const FCodeBActivePlayerInteractionCommitRequest& Request,
	const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
	const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
	FCodeBActivePlayerInteractionCommitResult& OutResult,
	FString& OutError)
{
	OutResult = FCodeBActivePlayerInteractionCommitResult();
	OutError.Reset();
	if (Request.StorageRoot.IsEmpty() || !Request.OwnerId.IsValid() || !Request.RunInstanceId.IsValid()
		|| !Request.bTransientScopeCurrent || !Request.Store || CandidateSnapshot.Revision < 0
		|| !AcceptedCommand.ItemId.IsValid())
	{
		OutError = TEXT("Code B active-player handoff requires a current exact page scope and accepted P1 candidate.");
		return false;
	}
	if (!Request.Store->CommitAcceptedActiveRunInventorySnapshot(
		Request.RunInstanceId, CandidateSnapshot, &OutError)) return false;
	OutResult.OwnerId = Request.OwnerId;
	OutResult.RunInstanceId = Request.RunInstanceId;
	OutResult.P6SnapshotRevision = CandidateSnapshot.Revision;
	return true;
}

bool FCodeBRunItemInteractionDomain::CommitAcceptedNormalContainer(
	const FCodeBNormalContainerInteractionCommitRequest& Request,
	const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
	const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
	FCodeBNormalContainerInteractionCommitResult& OutResult,
	FString& OutError)
{
	OutResult = FCodeBNormalContainerInteractionCommitResult();
	OutError.Reset();
	if (Request.StorageRoot.IsEmpty() || !Request.OwnerId.IsValid() || !Request.RunInstanceId.IsValid()
		|| !Request.TargetId.IsValid() || Request.DefinitionId.IsNone()
		|| Request.ExpectedP6SnapshotRevision < 0 || Request.ExpectedTargetRevision < 0
		|| !Request.bTransientScopeCurrent || CandidateSnapshot.Revision < 0 || !AcceptedCommand.ItemId.IsValid())
	{
		OutError = TEXT("Code B normal-container handoff requires a current exact target scope and accepted P1 candidate.");
		return false;
	}
	if (!FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunNormalContainerTransfer(
		Request.StorageRoot, Request.OwnerId, Request.RunInstanceId, Request.TargetId,
		Request.DefinitionId, Request.ExpectedP6SnapshotRevision, Request.ExpectedTargetRevision,
		AcceptedCommand, CandidateSnapshot, &OutError)) return false;
	OutResult.OwnerId = Request.OwnerId;
	OutResult.RunInstanceId = Request.RunInstanceId;
	OutResult.TargetId = Request.TargetId;
	OutResult.P6SnapshotRevision = CandidateSnapshot.Revision;
	OutResult.TargetRevision = Request.ExpectedTargetRevision + 1;
	FCodeBNormalContainerPresentationFrame Frame;
	FString ReadbackError;
	if (BuildNormalContainerFrame(Request.StorageRoot, Request.OwnerId, Request.RunInstanceId,
		Request.TargetId, Request.DefinitionId, Frame, ReadbackError))
	{
		OutResult.Projection = Frame.Projection;
	}
	return true;
}

bool FCodeBRunItemInteractionDomain::CommitAcceptedBodyContainer(
	const FCodeBBodyContainerInteractionCommitRequest& Request,
	const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
	const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
	FCodeBBodyContainerInteractionCommitResult& OutResult,
	FString& OutError)
{
	OutResult = FCodeBBodyContainerInteractionCommitResult();
	OutError.Reset();
	if (Request.StorageRoot.IsEmpty() || !Request.OwnerId.IsValid() || !Request.RunInstanceId.IsValid()
		|| !Request.TargetId.IsValid() || Request.DefinitionId.IsNone()
		|| Request.ExpectedP6SnapshotRevision < 0 || Request.ExpectedTargetRevision < 0
		|| !Request.bTransientScopeCurrent || CandidateSnapshot.Revision < 0 || !AcceptedCommand.ItemId.IsValid())
	{
		OutError = TEXT("Code B body-container handoff requires a current exact target scope and accepted P1 candidate.");
		return false;
	}
	if (!FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunBodyContainerTransfer(
		Request.StorageRoot, Request.OwnerId, Request.RunInstanceId, Request.TargetId,
		Request.DefinitionId, Request.ExpectedP6SnapshotRevision, Request.ExpectedTargetRevision,
		AcceptedCommand, CandidateSnapshot, &OutError)) return false;
	OutResult.OwnerId = Request.OwnerId;
	OutResult.RunInstanceId = Request.RunInstanceId;
	OutResult.TargetId = Request.TargetId;
	OutResult.P6SnapshotRevision = CandidateSnapshot.Revision;
	OutResult.TargetRevision = Request.ExpectedTargetRevision + 1;
	FCodeBBodyContainerPresentationFrame Frame;
	FString ReadbackError;
	if (BuildBodyContainerFrame(Request.StorageRoot, Request.OwnerId, Request.RunInstanceId,
		Request.TargetId, Request.DefinitionId, Frame, ReadbackError))
	{
		OutResult.Projection = Frame.Projection;
	}
	return true;
}

bool FCodeBRunItemInteractionDomain::CommitAcceptedWorldDrop(
	const FCodeBWorldDropInteractionCommitRequest& Request,
	const demo_map_code_b::FCodeBP2Command& AcceptedCommand,
	const demo_map_code_b::FCodeBSnapshot& CandidateSnapshot,
	FCodeBWorldDropInteractionCommitResult& OutResult,
	FString& OutError)
{
	OutResult = FCodeBWorldDropInteractionCommitResult();
	OutError.Reset();
	if (Request.StorageRoot.IsEmpty() || !Request.OwnerId.IsValid() || !Request.RunInstanceId.IsValid()
		|| !Request.WorldDropId.IsValid() || Request.ExpectedWorldDropOrdinal <= 0
		|| Request.ExpectedWorldDropRecordRevision <= 0 || Request.ExpectedP6SnapshotRevision < 0
		|| !Request.bTransientScopeCurrent || CandidateSnapshot.Revision < 0 || !AcceptedCommand.ItemId.IsValid())
	{
		OutError = TEXT("Code B WorldDrop handoff requires a current exact record scope and accepted P1 candidate.");
		return false;
	}
	if (!FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunWorldDropPickup(
		Request.StorageRoot, Request.OwnerId, Request.RunInstanceId, Request.WorldDropId,
		Request.ExpectedWorldDropOrdinal, Request.ExpectedWorldDropRecordRevision,
		Request.ExpectedP6SnapshotRevision, AcceptedCommand, CandidateSnapshot, &OutError)) return false;
	OutResult.OwnerId = Request.OwnerId;
	OutResult.RunInstanceId = Request.RunInstanceId;
	OutResult.WorldDropId = Request.WorldDropId;
	OutResult.P6SnapshotRevision = CandidateSnapshot.Revision;
	OutResult.bReconcileActors = true;
	FCodeBOutOfRaidProfileStore ReadbackStore(Request.StorageRoot, Request.OwnerId);
	TArray<FCodeBWorldDropProjection> Projections;
	FString ReadbackError;
	if (ReadbackStore.TryGetMatchedActiveRunWorldDropProjections(Request.RunInstanceId, Projections, &ReadbackError))
	{
		if (const FCodeBWorldDropProjection* Projection = Projections.FindByPredicate(
			[&Request](const FCodeBWorldDropProjection& Value) { return Value.WorldDropId == Request.WorldDropId; }))
		{
			OutResult.Projection = *Projection;
		}
	}
	return true;
}
