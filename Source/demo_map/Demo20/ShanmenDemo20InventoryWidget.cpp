#include "ShanmenDemo20InventoryWidget.h"
#include "ShanmenDemo20World.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenItemRepository.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr float Cell = 32.f;
	const FLinearColor Paper(.9f,.9f,.82f), Jade(.18f,.56f,.43f), Gold(.8f,.68f,.42f);
	bool Live(const FShanmenItemInstance& I) { return I.State == EShanmenItemInstanceState::Stored; }
	FVector2D ItemSize(const FShanmenItemAuthoritySnapshot& S, const FShanmenItemInstance& I, bool Equipment, bool Rotate)
	{
		const auto* F = S.Grid.Footprints.FindByPredicate([&](const auto& Value) { return Value.DefinitionId == I.DefinitionId; });
		if (Equipment || !F) return FVector2D(Cell, Cell);
		return Rotate ? FVector2D(F->Height * Cell, F->Width * Cell) : FVector2D(F->Width * Cell, F->Height * Cell);
	}
}
void UShanmenDemo20InventoryWidget::InitializeForDemo(AShanmenDemo20GameMode* InHost)
{
	Host = InHost; SetIsFocusable(true); SetVisibility(ESlateVisibility::Visible); RefreshProjection();
}
void UShanmenDemo20InventoryWidget::RefreshProjection()
{
	if (Host.IsValid()) Host->TryCaptureItems(Projection);
	Dragging = false;
	if (!Projection.Items.ContainsByPredicate([&](const auto& I) { return I.ItemInstanceId == Selected && Live(I); })) Selected.Invalidate();
}
TArray<UShanmenDemo20InventoryWidget::FBoard> UShanmenDemo20InventoryWidget::Boards() const
{
	TArray<FBoard> Result;
	const FName Roles[] = {TEXT("Stash"), TEXT("Carry"), TEXT("Secure"), TEXT("Weapon"), TEXT("Armor"), TEXT("Backpack"), TEXT("SecureBox")};
	const FVector2D Positions[] = {{24,100},{448,100},{756,100},{448,400},{544,400},{640,400},{756,400}};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Roles); ++Index)
	{
		const auto Id = FShanmenDemo20Catalog::ContainerId(Roles[Index]);
		if (const auto* L = Projection.Grid.Layouts.FindByPredicate([&](const auto& Value) { return Value.ContainerId == Id; }))
		{
			FBoard Board; Board.Id = Id; Board.Origin = Positions[Index]; Board.Width = L->Width; Board.Height = L->Height;
			Board.Equipment = L->Kind == EShanmenItemGridKind::Equipment; Result.Add(Board);
		}
	}
	return Result;
}
const UShanmenDemo20InventoryWidget::FBoard* UShanmenDemo20InventoryWidget::HitBoard(FVector2D Point, const TArray<FBoard>& List) const
{
	return List.FindByPredicate([&](const auto& B) { return Point.X >= B.Origin.X && Point.Y >= B.Origin.Y
		&& Point.X < B.Origin.X + B.Width * Cell && Point.Y < B.Origin.Y + B.Height * Cell; });
}
const FShanmenItemInstance* UShanmenDemo20InventoryWidget::HitItem(FVector2D Point) const
{
	const auto List = Boards(); const auto* B = HitBoard(Point, List); if (!B) return nullptr;
	return Projection.Items.FindByPredicate([&](const auto& I)
	{
		if (!Live(I) || I.ParentContainerId != B->Id) return false;
		const auto Size = ItemSize(Projection, I, B->Equipment, Projection.Grid.RotatedItems.Contains(I.ItemInstanceId));
		const auto Pos = B->Origin + FVector2D(I.SlotIndex % B->Width * Cell, I.SlotIndex / B->Width * Cell);
		return Point.X >= Pos.X && Point.Y >= Pos.Y && Point.X < Pos.X + Size.X && Point.Y < Pos.Y + Size.Y;
	});
}
FShanmenItemGridRequest UShanmenDemo20InventoryWidget::Request(FGuid ItemId) const
{
	FShanmenItemGridRequest R; R.Context.OwnerId = FShanmenDemo20Catalog::OwnerId(); R.Context.RunId = FShanmenDemo20Catalog::ScopeId();
	R.Context.Content = Projection.Content; R.Context.RequestId = FGuid::NewGuid(); R.ItemInstanceId = ItemId;
	R.ExpectedAuthorityRevision = Projection.AuthorityRevision;
	if (const auto* I = Projection.Items.FindByPredicate([&](const auto& Value) { return Value.ItemInstanceId == ItemId; }))
		R.ExpectedItemRevision = I->Revision;
	return R;
}
bool UShanmenDemo20InventoryWidget::DropIntent(const FBoard& B, FVector2D Point, FShanmenItemGridRequest& R) const
{
	const auto* Source = Projection.Items.FindByPredicate([&](const auto& I) { return I.ItemInstanceId == Selected && Live(I); });
	if (!Source) return false;
	R = Request(Selected);
	const auto* Target = HitItem(Point);
	if (!B.Equipment && Target && Target->ItemInstanceId != Selected && Target->DefinitionId == Source->DefinitionId)
	{
		R.Action = EShanmenItemGridAction::Merge; R.MergeTargetId = Target->ItemInstanceId;
		R.Amount = Source->Quantity; R.ExpectedTargetRevision = Target->Revision;
	}
	else
	{
		R.DestinationContainerId = B.Id;
		R.X = FMath::FloorToInt((Point.X - B.Origin.X - GrabOffset.X) / Cell);
		R.Y = FMath::FloorToInt((Point.Y - B.Origin.Y - GrabOffset.Y) / Cell);
		R.bRotated = Rotated;
		if (B.Equipment)
		{
			R.Action = EShanmenItemGridAction::Equip; R.X = R.Y = 0; R.bRotated = false;
			if (Target) R.ExpectedTargetRevision = Target->Revision;
		}
	}
	return true;
}
void UShanmenDemo20InventoryWidget::UpdateDragPreview()
{
	PreviewValid = false;
	const auto List = Boards(); const auto* B = HitBoard(Cursor, List);
	FShanmenItemGridRequest Intent;
	if (!B || !DropIntent(*B, Cursor, Intent)) return;
	// An ephemeral validation copy uses the exact command semantics (swap,
	// shrink and partial merge included). It never publishes or saves anything.
	// Evaluate on pointer changes, not from NativePaint on every frame.
	FShanmenItemRepository Validation;
	if (Validation.TryLoadSnapshot(Projection)) PreviewValid = Validation.EditGrid(Intent).bSuccess;
}
void UShanmenDemo20InventoryWidget::Submit(FShanmenItemGridRequest Intent)
{
	if (!Host.IsValid()) return;
	const auto Result = Host->EditItemGrid(Intent);
	if (Result.IsCommandSuccess())
	{
		Feedback = TEXT("已确认并保存。");
		if (Intent.Action == EShanmenItemGridAction::Split) Selected = Result.Receipt.ItemInstanceId;
	}
	else if (Result.Status == EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack)
		Feedback = TEXT("保存失败，操作已回滚。原物品保留，请重试。");
	else if (Result.Status == EShanmenItemDurableCommandStatus::RecoveryRequired || Result.Status == EShanmenItemDurableCommandStatus::NotReady)
		Feedback = TEXT("物品权威不可用或正在恢复，未报告成功。请返回入口检查。");
	else if (Result.Receipt.Error == EShanmenItemTransactionError::GridNoSpace)
		Feedback = TEXT("空间不足或缩容放不下：请先整理内容，原物品未改变。");
	else if (Result.Receipt.Error == EShanmenItemTransactionError::StaleAuthorityRevision || Result.Receipt.Error == EShanmenItemTransactionError::StaleItemRevision)
		Feedback = TEXT("物品状态已改变，已刷新。请重新操作。");
	else if (Result.Receipt.Error == EShanmenItemTransactionError::GridPolicyViolation)
	{
		const auto* L = Projection.Grid.Layouts.FindByPredicate([&](const auto& V) { return V.ContainerId == Intent.DestinationContainerId; });
		Feedback = L && L->Kind == EShanmenItemGridKind::Secure
			? TEXT("安全格只允许丹药、材料与战利品，不允许武器、护具或储物装备。原物品保留。")
			: L && L->Kind == EShanmenItemGridKind::Carry
			? TEXT("普通背包未启用，或物品属于禁止嵌套的储物装备。请先检查已装备行囊。")
			: TEXT("物品类型与装备槽不匹配，或储物容器未启用。原物品保留。");
	}
	else Feedback = TEXT("容器规则或装备条件不允许这次操作，原物品未改变。");
	RefreshProjection();
}
bool UShanmenDemo20InventoryWidget::FirstFit(FShanmenItemGridRequest& R, FGuid Id, bool IgnoreOriginal) const
{
	const auto* L = Projection.Grid.Layouts.FindByPredicate([&](const auto& Value) { return Value.ContainerId == Id; });
	if (!L) return false;
	for (int32 Y = 0; Y < L->Height; ++Y) for (int32 X = 0; X < L->Width; ++X)
	{
		if (FShanmenItemGridPolicy::CanPlace(Projection, R.ItemInstanceId, Id, X, Y, R.bRotated, IgnoreOriginal) == EShanmenItemTransactionError::None)
		{ R.DestinationContainerId = Id; R.X = X; R.Y = Y; return true; }
	}
	return false;
}
void UShanmenDemo20InventoryWidget::Toolbar(int32 Index)
{
	if (Index == 4) { if (Host.IsValid()) Host->ToggleInventory(); return; }
	const auto* I = Projection.Items.FindByPredicate([&](const auto& Value) { return Value.ItemInstanceId == Selected && Live(Value); });
	if (!I) { Feedback = TEXT("请先点击选择一个物品。"); return; }
	auto R = Request(Selected); R.bRotated = Projection.Grid.RotatedItems.Contains(Selected);
	if (Index == 0)
	{
		const auto* L = Projection.Grid.Layouts.FindByPredicate([&](const auto& Value) { return Value.ContainerId == I->ParentContainerId; });
		if (!L || L->Kind == EShanmenItemGridKind::Equipment) { Feedback = TEXT("装备槽不需要旋转。"); return; }
		R.DestinationContainerId = I->ParentContainerId; R.X = I->SlotIndex % L->Width; R.Y = I->SlotIndex / L->Width;
		R.bRotated = !R.bRotated; Submit(R);
	}
	else if (Index == 1)
	{
		if (I->Quantity < 2) { Feedback = TEXT("数量不足，无法拆分。"); return; }
		R.Action = EShanmenItemGridAction::Split; R.Amount = I->Quantity / 2;
		if (!FirstFit(R, I->ParentContainerId, false)) { Feedback = TEXT("当前容器没有可放置拆分物品的空间。"); return; }
		Submit(R);
	}
	else if (Index == 2)
	{
		const FGuid Destination = I->ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
			? FShanmenDemo20Catalog::ContainerId(TEXT("Carry")) : FShanmenDemo20Catalog::ContainerId(TEXT("Stash"));
		if (!FirstFit(R, Destination)) { Feedback = TEXT("目标容器无合法空间，请手动整理。"); return; } Submit(R);
	}
	else if (Index == 3)
	{
		const auto* F = Projection.Grid.Footprints.FindByPredicate([&](const auto& Value) { return Value.DefinitionId == I->DefinitionId; });
		if (!F || F->EquipmentRole.IsNone()) { Feedback = TEXT("这个物品不是装备。"); return; }
		R.Action = EShanmenItemGridAction::Equip; R.bRotated = false; R.DestinationContainerId = FShanmenDemo20Catalog::ContainerId(F->EquipmentRole);
		const auto* C = Projection.Containers.FindByPredicate([&](const auto& Value) { return Value.ContainerId == R.DestinationContainerId; });
		if (C && C->Slots[0].IsValid())
		{
			const auto* Existing = Projection.Items.FindByPredicate([&](const auto& Value) { return Value.ItemInstanceId == C->Slots[0]; });
			if (Existing) R.ExpectedTargetRevision = Existing->Revision;
		}
		Submit(R);
	}
}
FReply UShanmenDemo20InventoryWidget::NativeOnMouseButtonDown(const FGeometry& G, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	Cursor = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	if (Cursor.Y >= 566 && Cursor.Y < 612)
	{
		const int32 Index = FMath::FloorToInt((Cursor.X - 24) / 176); if (Index >= 0 && Index < 5) Toolbar(Index);
		return FReply::Handled();
	}
	if (const auto* I = HitItem(Cursor))
	{
		Selected = I->ItemInstanceId; Rotated = Projection.Grid.RotatedItems.Contains(Selected); Dragging = true;
		DragStart = Cursor;
		const auto List = Boards(); const auto* B = HitBoard(Cursor, List);
		GrabOffset = B && !B->Equipment ? FVector2D(
			FMath::FloorToInt((Cursor.X - B->Origin.X) / Cell) - I->SlotIndex % B->Width,
			FMath::FloorToInt((Cursor.Y - B->Origin.Y) / Cell) - I->SlotIndex / B->Width) * Cell : FVector2D::ZeroVector;
		UpdateDragPreview();
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return FReply::Handled();
}
FReply UShanmenDemo20InventoryWidget::NativeOnMouseMove(const FGeometry& G, const FPointerEvent& E)
{
	Cursor = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	if (Dragging) UpdateDragPreview();
	return Dragging ? FReply::Handled() : FReply::Unhandled();
}
FReply UShanmenDemo20InventoryWidget::NativeOnMouseButtonUp(const FGeometry& G, const FPointerEvent& E)
{
	if (!Dragging || E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
	Dragging = false; Cursor = G.AbsoluteToLocal(E.GetScreenSpacePosition());
	if (FVector2D::Distance(Cursor, DragStart) < 4.f) return FReply::Handled().ReleaseMouseCapture();
	const auto List = Boards(); const auto* B = HitBoard(Cursor, List);
	if (B)
	{
		FShanmenItemGridRequest R;
		if (DropIntent(*B, Cursor, R)) Submit(R);
	}
	else Feedback = TEXT("拖放取消：物品仍在原位置。");
	return FReply::Handled().ReleaseMouseCapture();
}
void UShanmenDemo20InventoryWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& E)
{
	Dragging = false; Super::NativeOnMouseCaptureLost(E);
}
int32 UShanmenDemo20InventoryWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip,
	FSlateWindowElementList& Draw, int32 Layer, const FWidgetStyle& Style, bool Enabled) const
{
	const int32 Base = Super::NativePaint(Args, G, Clip, Draw, Layer, Style, Enabled);
	const auto* Brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	auto Box = [&](FVector2D Pos, FVector2D Size, FLinearColor Color, int32 Offset = 0)
	{
		FSlateDrawElement::MakeBox(Draw, Base + 1 + Offset, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), Brush, ESlateDrawEffect::None, Color);
	};
	auto Text = [&](FVector2D Pos, const FString& Value, int32 FontSize = 15, FLinearColor Color = Paper)
	{
		FSlateDrawElement::MakeText(Draw, Base + 5, G.ToPaintGeometry(FVector2D(980,40), FSlateLayoutTransform(Pos)),
			Value, FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FontSize), ESlateDrawEffect::None, Color);
	};
	Box({0,0},{960,640}, FLinearColor(.025f,.05f,.043f,.99f));
	Text({24,18},TEXT("山门 / 仓库与整备"),25,Gold);
	int32 Money = 0; for (const auto& I : Projection.Items) if (Live(I) && I.DefinitionId == TEXT("Currency.Test")) Money += I.Quantity;
	Text({550,26}, FString::Printf(TEXT("测试灵石  %d · 新档一次性初始化"), Money),14,Gold);
	Text({24,59},TEXT("拖动物品到格子；同类拖到一起合并。点击选择后使用下方操作。"),14);
	const auto List = Boards();
	for (const auto& B : List)
	{
		const auto* C = Projection.Containers.FindByPredicate([&](const auto& Value) { return Value.ContainerId == B.Id; });
		Text(B.Origin - FVector2D(0,24), FString::Printf(TEXT("%s %d×%d"), *FShanmenDemo20Catalog::ContainerName(C ? C->ContainerType : NAME_None), B.Width, B.Height),13,Gold);
		for (int32 Y = 0; Y < B.Height; ++Y) for (int32 X = 0; X < B.Width; ++X)
			Box(B.Origin + FVector2D(X*Cell,Y*Cell),{Cell-1,Cell-1},FLinearColor(.09f,.14f,.12f));
		for (const auto& I : Projection.Items)
		{
			if (!Live(I) || I.ParentContainerId != B.Id) continue;
			const auto Pos = B.Origin + FVector2D(I.SlotIndex % B.Width * Cell, I.SlotIndex / B.Width * Cell);
			const auto Size = ItemSize(Projection,I,B.Equipment,Projection.Grid.RotatedItems.Contains(I.ItemInstanceId));
			Box(Pos+FVector2D(2,2),Size-FVector2D(4,4), I.ItemInstanceId == Selected ? FLinearColor(.34f,.35f,.19f) : Jade,1);
			const FString Name = FShanmenDemo20Catalog::ItemName(I.DefinitionId).Left(FMath::Max(1, static_cast<int32>(Size.X / 15)-1));
			Text(Pos+FVector2D(4,3),Name,12);
			if (I.Quantity > 1) Text(Pos+FVector2D(4,Size.Y-16),FString::FromInt(I.Quantity),11);
		}
	}
	if (Dragging)
	{
		const auto* B = HitBoard(Cursor,List);
		const auto* I = Projection.Items.FindByPredicate([&](const auto& Value) { return Value.ItemInstanceId == Selected; });
		if (B && I)
		{
			const int32 X = B->Equipment ? 0 : FMath::FloorToInt((Cursor.X-B->Origin.X-GrabOffset.X)/Cell);
			const int32 Y = B->Equipment ? 0 : FMath::FloorToInt((Cursor.Y-B->Origin.Y-GrabOffset.Y)/Cell);
			Box(B->Origin+FVector2D(X*Cell,Y*Cell),ItemSize(Projection,*I,B->Equipment,Rotated),PreviewValid ? FLinearColor(.2f,.9f,.5f,.4f) : FLinearColor(.9f,.2f,.2f,.4f),2);
		}
	}
	Text({448,467},TEXT("安全格只接受丹药、材料与战利品。储物装备仅整备更换。"),12,Gold);
	if (const auto* I = Projection.Items.FindByPredicate([&](const auto& Value) { return Value.ItemInstanceId == Selected; }))
	{
		Text({24,480}, FString::Printf(TEXT("已选：%s  ×%d"),*FShanmenDemo20Catalog::ItemName(I->DefinitionId),I->Quantity),17);
		const auto* F = Projection.Grid.Footprints.FindByPredicate([&](const auto& V) { return V.DefinitionId == I->DefinitionId; });
		const auto* D = Projection.Definitions.FindByPredicate([&](const auto& V) { return V.DefinitionId == I->DefinitionId; });
		const bool IsRotated = Projection.Grid.RotatedItems.Contains(I->ItemInstanceId);
		Text({24,510}, FString::Printf(TEXT("%s  ·  占格 %d×%d  ·  堆叠上限 %d"),
			*FShanmenDemo20Catalog::ItemPurpose(I->DefinitionId), F ? (IsRotated ? F->Height : F->Width) : 1,
			F ? (IsRotated ? F->Width : F->Height) : 1, D ? D->MaxStack : 1),13);
	}
	Text({24,539},Feedback.IsEmpty() ? TEXT("所有移动、装备和数量变化均经物品权威确认后保存。") : Feedback,14,Gold);
	const TCHAR* Labels[] = {TEXT("旋转所选"),TEXT("拆分一半"),TEXT("便捷转移"),TEXT("装备 / 替换"),TEXT("返回入口")};
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Box({24.f+Index*176.f,566},{164,46},FLinearColor(.11f,.29f,.22f)); Text({36.f+Index*176.f,578},Labels[Index],15);
	}
	Text({24,622},TEXT("整备功能增量：探索、治疗、终局损失与恢复尚未接通；石庭练习不发放奖励。"),11,Gold);
	return Base + 6;
}
