#include "ShanmenDemo20InventoryTransfer.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemStackTransfer.h"

namespace
{
	bool Live(const FShanmenItemInstance& I)
	{
		return I.State == EShanmenItemInstanceState::Stored || I.State == EShanmenItemInstanceState::Deployed;
	}
}

bool FShanmenDemo20InventoryTransfer::Build(const FShanmenItemAuthoritySnapshot& Actual,
	const FGuid& Item, const FGuid& Destination, const FGuid& ActiveRun,
	FShanmenItemGridRequest& Out, FString& Reason)
{
	const auto* Source = Actual.Items.FindByPredicate([&](const auto& I) { return I.ItemInstanceId == Item && Live(I); });
	const auto* Layout = Actual.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == Destination; });
	if (!Source || !Layout || Layout->Kind == EShanmenItemGridKind::Equipment)
	{ Reason = TEXT("物品或目标容器不可用，请刷新后重新选择。"); return false; }
	if (Source->ParentContainerId == Destination)
	{ Reason = TEXT("物品已在目标背包内；整理位置请直接拖动。"); return false; }
	if (ActiveRun.IsValid())
	{
		FShanmenDemo20ActiveLoadout Active;
		if (!FShanmenDemo20Loadout::InspectActive(Actual, Active, Reason) || Active.RunId != ActiveRun)
		{ Reason = TEXT("当前探索身份未确认，未转移物品。"); return false; }
	}
	FShanmenItemGridRequest Base;
	Base.Context.OwnerId = FShanmenDemo20Catalog::OwnerId(); Base.Context.RunId = FShanmenDemo20Catalog::ScopeId();
	Base.Context.Content = Actual.Content; Base.Context.RequestId = FGuid::NewGuid();
	Base.ItemInstanceId = Item; Base.ExpectedAuthorityRevision = Actual.AuthorityRevision;
	Base.ExpectedItemRevision = Source->Revision;
	// Exact command validation on a private copy, never a display overlay or save.
	// Discard rejected copies so candidate probing cannot leave receipt side effects.
	auto Accept = [&](const FShanmenItemGridRequest& Intent)
	{
		FShanmenItemRepository Check;
		if (!Check.TryLoadSnapshot(Actual)) return false;
		const auto Receipt = ActiveRun.IsValid() ? Check.EditActiveRunGrid({Intent, ActiveRun}) : Check.EditGrid(Intent);
		if (!Receipt.bSuccess) return false;
		Out = Intent; Reason.Reset(); return true;
	};
	const auto* Definition = Actual.Definitions.FindByPredicate([&](const auto& D) { return D.DefinitionId == Source->DefinitionId; });
	TArray<int32> Targets;
	if (Definition) for (int32 N = 0; N < Actual.Items.Num(); ++N)
	{
		const auto& Target = Actual.Items[N];
		if (Live(Target) && Target.ParentContainerId == Destination && Target.DefinitionId == Source->DefinitionId
			&& Target.Quantity < Definition->MaxStack
			&& FShanmenItemStackTransferPolicy::MetadataCompatible(Source->RewardMetadata, Target.RewardMetadata)) Targets.Add(N);
	}
	Targets.Sort([&](int32 A, int32 B)
	{
		const auto& Left = Actual.Items[A]; const auto& Right = Actual.Items[B];
		return Left.SlotIndex != Right.SlotIndex ? Left.SlotIndex < Right.SlotIndex
			: Left.ItemInstanceId.ToString() < Right.ItemInstanceId.ToString();
	});
	for (int32 N : Targets)
	{
		auto Intent = Base; Intent.Action = EShanmenItemGridAction::Merge;
		Intent.MergeTargetId = Actual.Items[N].ItemInstanceId; Intent.ExpectedTargetRevision = Actual.Items[N].Revision;
		Intent.Amount = Source->Quantity;
		if (Accept(Intent)) return true;
	}
	const bool CurrentRotation = Actual.Grid.RotatedItems.Contains(Item);
	const auto* Footprint = Actual.Grid.Footprints.FindByPredicate([&](const auto& F) { return F.DefinitionId == Source->DefinitionId; });
	for (int32 Orientation = 0; Orientation < 2; ++Orientation)
	{
		if (Orientation == 1 && (!Footprint || Footprint->Width == Footprint->Height)) break;
		const bool Rotation = Orientation == 0 ? CurrentRotation : !CurrentRotation;
		for (int32 Y = 0; Y < Layout->Height; ++Y) for (int32 X = 0; X < Layout->Width; ++X)
		{
			if (FShanmenItemGridPolicy::CanPlace(Actual, Item, Destination, X, Y, Rotation) != EShanmenItemTransactionError::None) continue;
			auto Intent = Base; Intent.DestinationContainerId = Destination; Intent.X = X; Intent.Y = Y; Intent.bRotated = Rotation;
			if (Accept(Intent)) return true;
		}
	}
	Reason = TEXT("没有可合并堆叠或合法空格；请整理、丢弃或留在原处。物品未改变。");
	return false;
}
