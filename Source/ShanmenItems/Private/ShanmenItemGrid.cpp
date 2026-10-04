#include "ShanmenItemGrid.h"
#include "ShanmenItemTypes.h"

bool FShanmenItemFootprint::operator==(const FShanmenItemFootprint& O) const
{
	return DefinitionId == O.DefinitionId && Width == O.Width && Height == O.Height
		&& EquipmentRole == O.EquipmentRole && bSecureAllowed == O.bSecureAllowed
		&& bStorageEquipment == O.bStorageEquipment;
}
bool FShanmenItemGridLayout::operator==(const FShanmenItemGridLayout& O) const
{
	return ContainerId == O.ContainerId && Width == O.Width && Height == O.Height
		&& Kind == O.Kind && EquipmentRole == O.EquipmentRole;
}
bool FShanmenItemGridSnapshot::IsEmpty() const
{
	return Footprints.IsEmpty() && Layouts.IsEmpty() && RotatedItems.IsEmpty();
}
void FShanmenItemGridSnapshot::Canonicalize()
{
	Footprints.Sort([](const auto& A, const auto& B) { return A.DefinitionId.LexicalLess(B.DefinitionId); });
	Layouts.Sort([](const auto& A, const auto& B) { return A.ContainerId.ToString() < B.ContainerId.ToString(); });
	RotatedItems.Sort([](const auto& A, const auto& B) { return A.ToString() < B.ToString(); });
}
bool FShanmenItemGridSnapshot::operator==(const FShanmenItemGridSnapshot& O) const
{
	return Footprints == O.Footprints && Layouts == O.Layouts && RotatedItems == O.RotatedItems;
}

namespace
{
	using EError = EShanmenItemTransactionError;
	bool Live(const FShanmenItemInstance& Item)
	{
		return Item.State != EShanmenItemInstanceState::Destroyed && Item.State != EShanmenItemInstanceState::Depleted;
	}
	const FShanmenItemFootprint* Footprint(const FShanmenItemAuthoritySnapshot& S, FName Id)
	{
		return S.Grid.Footprints.FindByPredicate([Id](const auto& F) { return F.DefinitionId == Id; });
	}
	bool Allowed(const FShanmenItemGridLayout& L, const FShanmenItemFootprint& F)
	{
		if (L.Kind == EShanmenItemGridKind::Secure && !F.bSecureAllowed) { return false; }
		if ((L.Kind == EShanmenItemGridKind::Carry || L.Kind == EShanmenItemGridKind::Secure) && F.bStorageEquipment) { return false; }
		return L.Kind != EShanmenItemGridKind::Equipment || (!F.EquipmentRole.IsNone() && L.EquipmentRole == F.EquipmentRole);
	}
	FIntPoint Size(const FShanmenItemGridLayout& L, const FShanmenItemFootprint& F, bool Rotated)
	{
		return L.Kind == EShanmenItemGridKind::Equipment ? FIntPoint(1, 1)
			: Rotated ? FIntPoint(F.Height, F.Width) : FIntPoint(F.Width, F.Height);
	}
}

EShanmenItemTransactionError FShanmenItemGridPolicy::CanPlace(const FShanmenItemAuthoritySnapshot& S,
	const FGuid& ItemId, const FGuid& ContainerId, int32 X, int32 Y, bool Rotated, bool IgnoreOriginal)
{
	const auto* Item = S.Items.FindByPredicate([&](const auto& I) { return I.ItemInstanceId == ItemId; });
	const auto* Layout = S.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == ContainerId; });
	const auto* Container = S.Containers.FindByPredicate([&](const auto& C) { return C.ContainerId == ContainerId; });
	if (!Item || !Live(*Item)) { return EError::ItemNotFound; }
	if (!Layout || !Container) { return EError::ContainerNotFound; }
	if (Layout->Width < 1 || Layout->Width > 64 || Layout->Height < 1 || Layout->Height > 64
		|| Container->Slots.Num() != Layout->Width * Layout->Height
		|| static_cast<uint8>(Layout->Kind) > static_cast<uint8>(EShanmenItemGridKind::World)) { return EError::InvariantViolation; }
	if (Item->OwnerId != Container->OwnerId || Item->RunId != Container->RunId) { return EError::ScopeMismatch; }
	const auto* F = Footprint(S, Item->DefinitionId);
	if (!F || F->Width < 1 || F->Width > 16 || F->Height < 1 || F->Height > 16
		|| !Allowed(*Layout, *F) || Item->ChildContainerId.IsValid()) { return EError::GridPolicyViolation; }
	const auto Extent = Size(*Layout, *F, Rotated);
	if (X < 0 || Y < 0 || X > Layout->Width - Extent.X || Y > Layout->Height - Extent.Y) { return EError::GridNoSpace; }
	for (const auto& Other : S.Items)
	{
		if (!Live(Other) || Other.ParentContainerId != ContainerId || (IgnoreOriginal && Other.ItemInstanceId == ItemId)) { continue; }
		const auto* OtherF = Footprint(S, Other.DefinitionId);
		if (!OtherF || Other.SlotIndex < 0 || Other.SlotIndex >= Container->Slots.Num()
			|| OtherF->Width < 1 || OtherF->Width > 16 || OtherF->Height < 1 || OtherF->Height > 16) { return EError::InvariantViolation; }
		const auto OtherExtent = Size(*Layout, *OtherF, S.Grid.RotatedItems.Contains(Other.ItemInstanceId));
		const int32 Ox = Other.SlotIndex % Layout->Width, Oy = Other.SlotIndex / Layout->Width;
		if (X < Ox + OtherExtent.X && X + Extent.X > Ox && Y < Oy + OtherExtent.Y && Y + Extent.Y > Oy) { return EError::GridNoSpace; }
	}
	return EError::None;
}

bool FShanmenItemGridPolicy::Validate(const FShanmenItemAuthoritySnapshot& S)
{
	// Bounded derived occupancy, not a second inventory. Do not scan every item
	// against every other item while validating a large persisted source history.
	if (S.Grid.Layouts.Num() > 4096 || S.Grid.Footprints.Num() > 4096) { return false; }
	TSet<FName> KnownDefinitions;
	for (const auto& D : S.Definitions) { KnownDefinitions.Add(D.DefinitionId); }
	TMap<FName, const FShanmenItemFootprint*> Definitions;
	for (const auto& F : S.Grid.Footprints)
	{
		if (F.DefinitionId.IsNone() || Definitions.Contains(F.DefinitionId) || F.Width < 1 || F.Width > 16
			|| F.Height < 1 || F.Height > 16 || (F.bStorageEquipment && (F.bSecureAllowed || F.EquipmentRole.IsNone()))
			|| !KnownDefinitions.Contains(F.DefinitionId)) { return false; }
		Definitions.Add(F.DefinitionId, &F);
	}
	TMap<FGuid, const FShanmenItemContainer*> Containers;
	for (const auto& C : S.Containers) { Containers.Add(C.ContainerId, &C); }
	TMap<FGuid, int32> LayoutIndices;
	TArray<TBitArray<>> Occupancy;
	int32 TotalCells = 0;
	for (const auto& L : S.Grid.Layouts)
	{
		const auto* C = Containers.FindRef(L.ContainerId);
		if (!L.ContainerId.IsValid() || LayoutIndices.Contains(L.ContainerId) || !C || L.Width < 1 || L.Width > 64
			|| L.Height < 1 || L.Height > 64 || C->Slots.Num() != L.Width * L.Height
			|| static_cast<uint8>(L.Kind) > static_cast<uint8>(EShanmenItemGridKind::World)
			|| (L.Kind == EShanmenItemGridKind::Equipment ? (L.EquipmentRole.IsNone() || L.Width != 1 || L.Height != 1) : !L.EquipmentRole.IsNone())) { return false; }
		TotalCells += L.Width * L.Height;
		if (TotalCells > 1024 * 1024) { return false; }
		LayoutIndices.Add(L.ContainerId, Occupancy.Num());
		Occupancy.Add(TBitArray<>(false, L.Width * L.Height));
	}
	TMap<FGuid, const FShanmenItemInstance*> Items;
	for (const auto& I : S.Items) { Items.Add(I.ItemInstanceId, &I); }
	TSet<FGuid> Rotations;
	for (const auto& Id : S.Grid.RotatedItems)
	{
		const auto* Item = Items.FindRef(Id);
		if (!Id.IsValid() || Rotations.Contains(Id) || !Item || !Definitions.Contains(Item->DefinitionId)) { return false; }
		Rotations.Add(Id);
	}
	for (const auto& I : S.Items)
	{
		const auto* LayoutIndex = LayoutIndices.Find(I.ParentContainerId);
		if (!Live(I) || !LayoutIndex) { continue; }
		const auto& L = S.Grid.Layouts[*LayoutIndex];
		const auto* F = Definitions.FindRef(I.DefinitionId);
		const auto* C = Containers.FindRef(I.ParentContainerId);
		if (!F || !Allowed(L, *F) || I.ChildContainerId.IsValid() || I.OwnerId != C->OwnerId || I.RunId != C->RunId
			|| I.SlotIndex < 0 || I.SlotIndex >= C->Slots.Num()) { return false; }
		const auto Extent = Size(L, *F, Rotations.Contains(I.ItemInstanceId));
		const int32 X = I.SlotIndex % L.Width, Y = I.SlotIndex / L.Width;
		if (X > L.Width - Extent.X || Y > L.Height - Extent.Y) { return false; }
		auto& Cells = Occupancy[*LayoutIndex];
		for (int32 Dy = 0; Dy < Extent.Y; ++Dy)
		{
			for (int32 Dx = 0; Dx < Extent.X; ++Dx)
			{
				const int32 Index = (Y + Dy) * L.Width + X + Dx;
				if (Cells[Index]) { return false; }
				Cells[Index] = true;
			}
		}
	}
	return true;
}
