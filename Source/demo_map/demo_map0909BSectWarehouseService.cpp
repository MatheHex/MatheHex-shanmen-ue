#include "demo_map0909BSectWarehouseService.h"

#include "CodeB/demo_mapCodeBP2.h"
#include "demo_map.h"

namespace
{
	FString GuidText(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::DigitsWithHyphensLower);
	}
}

bool Fdemo_map0909BSectWarehouseService::OpenForSect(
	const FString& StorageRoot,
	const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot,
	const Edemo_map0909BTopState CoordinatorState,
	Fdemo_map0909BWarehousePresentation& OutPresentation,
	FString& OutDiagnostic)
{
	OutPresentation = Fdemo_map0909BWarehousePresentation();
	OutDiagnostic.Reset();
	Reset();
	if (CoordinatorState != Edemo_map0909BTopState::AtSect)
	{
		OutDiagnostic = CoordinatorState == Edemo_map0909BTopState::PreparingStart
			|| CoordinatorState == Edemo_map0909BTopState::ActivatingWorld
			? TEXT("StartAttemptPending：出战尝试处理中，暂不接受仓库写入。")
			: TEXT("只有协调器确认 AtSect 时才能打开可整理的宗门仓库。");
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	if (StorageRoot.IsEmpty() || !ProfileSnapshot.ProfileId.IsValid())
	{
		OutDiagnostic = TEXT("宗门仓库无法取得 Owner-matched P5 Profile 存储根。");
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}

	Store = MakeUnique<FCodeBOutOfRaidProfileStore>(StorageRoot, ProfileSnapshot.ProfileId);
	const FCodeBOutOfRaidOpenResult Open = Store->OpenOrMigrate(ProfileSnapshot, Repository, Layout);
	if (!Open.bSuccess)
	{
		OutDiagnostic = Open.Diagnostic;
		OutPresentation.GateDiagnostic = OutDiagnostic;
		Reset();
		return false;
	}
	return BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
}

bool Fdemo_map0909BSectWarehouseService::ApplyDragIntent(
	const Fdemo_map0909BWarehouseIntent& Intent,
	const Edemo_map0909BTopState CoordinatorState,
	Fdemo_map0909BWarehousePresentation& OutPresentation,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Store || CoordinatorState != Edemo_map0909BTopState::AtSect)
	{
		OutDiagnostic = CoordinatorState == Edemo_map0909BTopState::PreparingStart
			|| CoordinatorState == Edemo_map0909BTopState::ActivatingWorld
			? TEXT("StartAttemptPending：出战尝试处理中，拒绝 P5 写事务。")
			: TEXT("当前协调器状态不允许写入宗门仓库。");
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}
	const FCodeBOutOfRaidInventoryRecord& Record = Store->GetRecord();
	if (Record.bHasActiveRunInventorySession)
	{
		OutDiagnostic = TEXT("Coordinator 与 P5 均确认已有真实活动 Run；宗门仓库只读。");
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}
	if (!Intent.ItemId.IsValid() || !Intent.SourceContainerId.IsValid()
		|| !Intent.TargetContainerId.IsValid() || Intent.SourceSlot == INDEX_NONE
		|| Intent.TargetSlot == INDEX_NONE || Intent.ExpectedGraphRevision != Repository.GetRevision())
	{
		OutDiagnostic = TEXT("仓库拖拽意图无效或 snapshot 已过期；未写入 P5。");
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}
	if (!IsP5LayoutContainer(Intent.SourceContainerId) || !IsP5LayoutContainer(Intent.TargetContainerId))
	{
		OutDiagnostic = TEXT("仓库拖拽只能在同一 P5 战备图的正式容器之间进行。");
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}
	const demo_map_code_b::FCodeBItemInstance* SourceItem = Repository.FindItem(Intent.ItemId);
	if (!SourceItem || SourceItem->ParentContainerId != Intent.SourceContainerId
		|| SourceItem->SlotIndex != Intent.SourceSlot || !ValidateCompleteSpatialClosure(Intent.ItemId, OutDiagnostic))
	{
		if (OutDiagnostic.IsEmpty())
		{
			OutDiagnostic = TEXT("仓库拖拽的 source ItemId、容器或空间图闭包不匹配。");
		}
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}

	demo_map_code_b::FCodeBTransactionRequest Request;
	Request.TransactionId = FGuid::NewGuid();
	Request.Operation = ResolveDragOperation(Repository, Intent);
	Request.ItemId = Intent.ItemId;
	Request.SourceContainerId = Intent.SourceContainerId;
	Request.SourceSlot = Intent.SourceSlot;
	Request.TargetContainerId = Intent.TargetContainerId;
	Request.TargetSlot = Intent.TargetSlot;
	Request.ExpectedRevision = Intent.ExpectedGraphRevision;
	const demo_map_code_b::FCodeBTransactionResult Transaction = Repository.ExecuteTransaction(Request);
	if (!Transaction.IsSuccess())
	{
		OutDiagnostic = TEXT("P5 整理事务被 Code B P1 拒绝：") + Transaction.Message;
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}

	FString CommitError;
	if (!Store->CommitAcceptedSnapshot(Repository.CaptureSnapshot(), &CommitError))
	{
		// P1 committed only in memory. Restore the pre-transaction durable graph
		// before returning a failure so no partial P5 view remains live.
		Repository.LoadPersistedSnapshot(Store->GetRecord().RepositorySnapshot, nullptr);
		OutDiagnostic = TEXT("P5 durable replacement failed; P1 graph was restored: ") + CommitError;
		BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
		return false;
	}

	UE_LOG(Logdemo_map, Log,
		TEXT("0_0_9BFIX_WAREHOUSE Event=P5TransactionCommitted OwnerId=%s ItemId=%s From=%s:%d To=%s:%d GraphRevision=%d PersistentRevision=%d"),
		*GuidText(Record.OwnerId), *GuidText(Intent.ItemId), *GuidText(Intent.SourceContainerId), Intent.SourceSlot,
		*GuidText(Intent.TargetContainerId), Intent.TargetSlot, Repository.GetRevision(), Store->GetPersistentRevision());
	return BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
}

bool Fdemo_map0909BSectWarehouseService::CaptureLoadoutSelection(
	FCodeBLoadoutSelection& OutSelection,
	FString& OutDiagnostic) const
{
	OutSelection = FCodeBLoadoutSelection();
	OutDiagnostic.Reset();
	if (!Store)
	{
		OutDiagnostic = TEXT("P5 warehouse service is not enrolled for this sect session.");
		return false;
	}
	return FCodeBOutOfRaidProfileStore::BuildLoadoutSelection(
		Store->GetRecord(), OutSelection, &OutDiagnostic);
}

void Fdemo_map0909BSectWarehouseService::Reset()
{
	Store.Reset();
	Repository = demo_map_code_b::FCodeBRepository();
	Layout = demo_map_code_b::FCodeBP2PlayerLayout();
}

bool Fdemo_map0909BSectWarehouseService::BuildPresentation(
	const Edemo_map0909BTopState CoordinatorState,
	Fdemo_map0909BWarehousePresentation& OutPresentation,
	FString& OutDiagnostic) const
{
	OutPresentation = Fdemo_map0909BWarehousePresentation();
	if (!Store)
	{
		if (OutDiagnostic.IsEmpty()) OutDiagnostic = TEXT("P5 warehouse service is not open.");
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	FString ProjectionError;
	if (!demo_map_code_b::FCodeBP2ProjectionBuilder::Build(
		Repository, Layout, OutPresentation.Projection, nullptr, &ProjectionError))
	{
		OutDiagnostic = ProjectionError;
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	FString SelectionError;
	if (!FCodeBOutOfRaidProfileStore::BuildLoadoutSelection(
		Store->GetRecord(), OutPresentation.LoadoutSelection, &SelectionError))
	{
		OutDiagnostic = SelectionError;
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	OutPresentation.bOpen = true;
	OutPresentation.OwnerId = Store->GetRecord().OwnerId;
	OutPresentation.PersistentRevision = Store->GetPersistentRevision();
	OutPresentation.bCanWrite = CoordinatorState == Edemo_map0909BTopState::AtSect
		&& !Store->GetRecord().bHasActiveRunInventorySession;
	OutPresentation.GateDiagnostic = OutPresentation.bCanWrite
		? TEXT("AtSect：可提交同图 P5 整理事务。")
		: (CoordinatorState == Edemo_map0909BTopState::PreparingStart
			|| CoordinatorState == Edemo_map0909BTopState::ActivatingWorld
			? TEXT("StartAttemptPending：出战尝试处理中，P5 写入暂时拒绝。")
			: TEXT("只有确认 InRun 才锁定仓库写入。"));
	return true;
}

bool Fdemo_map0909BSectWarehouseService::IsP5LayoutContainer(const FGuid& ContainerId) const
{
	for (const TPair<FName, FGuid>& Pair : Layout.GetOrderedContainers())
	{
		if (Pair.Value == ContainerId)
		{
			return true;
		}
	}
	return false;
}

bool Fdemo_map0909BSectWarehouseService::ValidateCompleteSpatialClosure(
	const FGuid& RootItemId,
	FString& OutDiagnostic) const
{
	TSet<FGuid> VisitedItems;
	TSet<FGuid> VisitedContainers;
	TFunction<bool(const FGuid&)> VisitItem;
	VisitItem = [this, &VisitedItems, &VisitedContainers, &OutDiagnostic, &VisitItem](const FGuid& ItemId)
	{
		if (!ItemId.IsValid() || VisitedItems.Contains(ItemId))
		{
			OutDiagnostic = TEXT("空间图出现无效或循环 ItemId。");
			return false;
		}
		const demo_map_code_b::FCodeBItemInstance* Item = Repository.FindItem(ItemId);
		if (!Item)
		{
			OutDiagnostic = TEXT("空间图引用了不存在的 ItemId。");
			return false;
		}
		VisitedItems.Add(ItemId);
		if (!Item->ChildContainerId.IsValid())
		{
			return true;
		}
		if (VisitedContainers.Contains(Item->ChildContainerId))
		{
			OutDiagnostic = TEXT("空间图出现 child-container 循环。");
			return false;
		}
		const demo_map_code_b::FCodeBContainer* Child = Repository.FindContainer(Item->ChildContainerId);
		if (!Child)
		{
			OutDiagnostic = TEXT("空间 parent 缺少其正式 child container。");
			return false;
		}
		VisitedContainers.Add(Item->ChildContainerId);
		for (const FGuid& ChildItemId : Child->Slots)
		{
			if (!ChildItemId.IsValid()) continue;
			const demo_map_code_b::FCodeBItemInstance* ChildItem = Repository.FindItem(ChildItemId);
			if (!ChildItem || ChildItem->ParentContainerId != Child->ContainerId || !VisitItem(ChildItemId))
			{
				if (OutDiagnostic.IsEmpty()) OutDiagnostic = TEXT("空间 child graph 不是完整的同一 P5 闭包。");
				return false;
			}
		}
		return true;
	};
	return VisitItem(RootItemId);
}

demo_map_code_b::ECodeBOperation Fdemo_map0909BSectWarehouseService::ResolveDragOperation(
	const demo_map_code_b::FCodeBRepository& InRepository,
	const Fdemo_map0909BWarehouseIntent& Intent)
{
	const demo_map_code_b::FCodeBContainer* Source = InRepository.FindContainer(Intent.SourceContainerId);
	const demo_map_code_b::FCodeBContainer* Target = InRepository.FindContainer(Intent.TargetContainerId);
	if (!Source || !Target)
	{
		return demo_map_code_b::ECodeBOperation::Move;
	}
	const bool bTargetOccupied = Target->Slots.IsValidIndex(Intent.TargetSlot)
		&& Target->Slots[Intent.TargetSlot].IsValid();
	if (bTargetOccupied)
	{
		return demo_map_code_b::ECodeBOperation::Swap;
	}
	if (Target->IsEquipment())
	{
		return demo_map_code_b::ECodeBOperation::Equip;
	}
	return Source->IsEquipment()
		? demo_map_code_b::ECodeBOperation::Unequip
		: demo_map_code_b::ECodeBOperation::Move;
}
