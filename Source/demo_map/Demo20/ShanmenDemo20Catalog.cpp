#include "ShanmenDemo20Catalog.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"

namespace
{
	struct FEntry { const TCHAR* Id; const TCHAR* Name; int32 Width; int32 Height; int32 Stack; const TCHAR* Role; bool Secure; };
	const FEntry Entries[] = {
		{TEXT("Sword.Plain"), TEXT("青锋剑"), 1, 3, 1, TEXT("Weapon"), false},
		{TEXT("Sword.Heavy"), TEXT("玄铁剑"), 2, 3, 1, TEXT("Weapon"), false},
		{TEXT("Armor.Robe"), TEXT("素纹道袍"), 2, 2, 1, TEXT("Armor"), false},
		{TEXT("Armor.Leather"), TEXT("护身皮甲"), 2, 3, 1, TEXT("Armor"), false},
		{TEXT("Heal.Pill"), TEXT("回春丹"), 1, 1, 10, TEXT(""), true},
		{TEXT("Material.Herb"), TEXT("灵草"), 1, 2, 20, TEXT(""), true},
		{TEXT("Material.Ore"), TEXT("玄铁矿"), 2, 1, 10, TEXT(""), true},
		{TEXT("Trophy.Jade"), TEXT("碎玉佩"), 1, 1, 5, TEXT(""), true},
		{TEXT("Trophy.Scroll"), TEXT("残卷"), 2, 2, 3, TEXT(""), true},
		{TEXT("Backpack.Small"), TEXT("行囊 · 六列"), 2, 2, 1, TEXT("Backpack"), false},
		{TEXT("Backpack.Large"), TEXT("乾坤行囊 · 八列"), 2, 3, 1, TEXT("Backpack"), false},
		{TEXT("Secure.Basic"), TEXT("护命匣"), 2, 2, 1, TEXT("SecureBox"), false},
		{TEXT("Currency.Test"), TEXT("测试灵石"), 1, 1, 1000000000, TEXT(""), false}
	};
	FGuid Identity(const FString& Role) { return FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Profile.r1"), {Role}); }
}

FGuid FShanmenDemo20Catalog::OwnerId() { return Identity(TEXT("Owner")); }
FGuid FShanmenDemo20Catalog::ScopeId() { return Identity(TEXT("PreparationScope")); }
FGuid FShanmenDemo20Catalog::ContainerId(FName Role) { return Identity(TEXT("Container:") + Role.ToString()); }
FString FShanmenDemo20Catalog::ItemName(FName DefinitionId)
{
	for (const auto& E : Entries) { if (DefinitionId == FName(E.Id)) { return E.Name; } }
	return TEXT("未知物品");
}
FString FShanmenDemo20Catalog::ContainerName(FName Role)
{
	if (Role == TEXT("Stash")) return TEXT("局外仓库");
	if (Role == TEXT("Carry")) return TEXT("普通背包");
	if (Role == TEXT("Secure")) return TEXT("安全格 · 死亡保留");
	if (Role == TEXT("Weapon")) return TEXT("武器");
	if (Role == TEXT("Armor")) return TEXT("护具");
	if (Role == TEXT("Backpack")) return TEXT("背包装备");
	if (Role == TEXT("SecureBox")) return TEXT("安全格装备");
	return TEXT("测试钱包");
}

FString FShanmenDemo20Catalog::ItemPurpose(FName Id)
{
	if (Id == TEXT("Backpack.Small")) return TEXT("背包装备：普通背包 6×4，仅整备更换");
	if (Id == TEXT("Backpack.Large")) return TEXT("背包装备：普通背包 8×5，仅整备更换");
	if (Id == TEXT("Secure.Basic")) return TEXT("安全格装备：2×2；死亡保留接线开发中");
	if (Id == TEXT("Heal.Pill")) return TEXT("治疗丹药；正式治疗接线开发中");
	if (Id == TEXT("Sword.Plain") || Id == TEXT("Sword.Heavy")) return TEXT("剑类武器；固定属性接线开发中");
	if (Id == TEXT("Armor.Robe") || Id == TEXT("Armor.Leather")) return TEXT("护具；正式防御接线开发中");
	if (Id == TEXT("Material.Herb") || Id == TEXT("Material.Ore")) return TEXT("材料，可存入安全格；制作不在本阶段范围");
	if (Id == TEXT("Trophy.Jade") || Id == TEXT("Trophy.Scroll")) return TEXT("战利品，可存入安全格；出售不在本阶段范围");
	return TEXT("测试货币，仅新档初始化一次；本阶段无商店");
}

FShanmenItemAuthoritySnapshot FShanmenDemo20Catalog::Initial(int32 TestMoney)
{
	FShanmenItemAuthoritySnapshot S;
	if (TestMoney < 1 || TestMoney > 1000000000) return S;
	S.Content.Version = TEXT("Demo20.Catalog.r1"); S.Content.Digest = TEXT("Demo20.FixedCatalog.13.GridStorage.r1");
	for (const auto& E : Entries)
	{
		FShanmenItemDefinition D; D.DefinitionId = E.Id; D.MaxStack = E.Stack;
		D.ItemTags.AddTag(E.Role[0] ? FShanmenItemNativeTags::CapabilityDeploy() : FShanmenItemNativeTags::CapabilityConsumeQuantity());
		S.Definitions.Add(D);
		FShanmenItemFootprint F; F.DefinitionId = E.Id; F.Width = E.Width; F.Height = E.Height;
		F.EquipmentRole = E.Role; F.bSecureAllowed = E.Secure;
		F.bStorageEquipment = F.EquipmentRole == TEXT("Backpack") || F.EquipmentRole == TEXT("SecureBox");
		S.Grid.Footprints.Add(F);
	}
	auto Storage = [&](FName Id, EShanmenItemGridKind Kind, int32 Width, int32 Height)
	{
		FShanmenItemStorageDefinition D; D.DefinitionId = Id; D.Kind = Kind; D.Width = Width; D.Height = Height; S.Grid.StorageDefinitions.Add(D);
	};
	Storage(TEXT("Backpack.Small"), EShanmenItemGridKind::Carry, 6, 4);
	Storage(TEXT("Backpack.Large"), EShanmenItemGridKind::Carry, 8, 5);
	Storage(TEXT("Secure.Basic"), EShanmenItemGridKind::Secure, 2, 2);
	auto Container = [&](FName Role, EShanmenItemGridKind Kind, int32 Width, int32 Height, bool Grid = true)
	{
		FShanmenItemContainer C; C.ContainerId = ContainerId(Role); C.RunId = ScopeId(); C.OwnerId = OwnerId();
		C.ContainerType = Role; C.Slots.SetNum(Width * Height); S.Containers.Add(C);
		if (Grid)
		{
			FShanmenItemGridLayout L; L.ContainerId = C.ContainerId; L.Kind = Kind; L.Width = Width; L.Height = Height;
			if (Kind == EShanmenItemGridKind::Equipment) L.EquipmentRole = Role;
			S.Grid.Layouts.Add(L);
		}
	};
	Container(TEXT("Stash"), EShanmenItemGridKind::Stash, 12, 8);
	Container(TEXT("Carry"), EShanmenItemGridKind::Carry, 6, 4);
	Container(TEXT("Secure"), EShanmenItemGridKind::Secure, 2, 2);
	for (FName Role : {FName(TEXT("Weapon")), FName(TEXT("Armor")), FName(TEXT("Backpack")), FName(TEXT("SecureBox"))})
		Container(Role, EShanmenItemGridKind::Equipment, 1, 1);
	Container(TEXT("Wallet"), EShanmenItemGridKind::Stash, 1, 1, false);
	auto Item = [&](FName Id, FName Role, int32 Anchor, int32 Quantity)
	{
		FShanmenItemInstance I; I.ItemInstanceId = Identity(TEXT("StartingItem:") + Id.ToString()); I.DefinitionId = Id;
		I.OwnerId = OwnerId(); I.RunId = ScopeId(); I.ParentContainerId = ContainerId(Role); I.SlotIndex = Anchor; I.Quantity = Quantity;
		S.Items.Add(I); S.Containers.FindByPredicate([&](const auto& C) { return C.ContainerId == I.ParentContainerId; })->Slots[Anchor] = I.ItemInstanceId;
	};
	Item(TEXT("Sword.Plain"), TEXT("Weapon"), 0, 1); Item(TEXT("Armor.Robe"), TEXT("Armor"), 0, 1);
	Item(TEXT("Backpack.Small"), TEXT("Backpack"), 0, 1); Item(TEXT("Secure.Basic"), TEXT("SecureBox"), 0, 1);
	Item(TEXT("Sword.Heavy"), TEXT("Stash"), 0, 1); Item(TEXT("Armor.Leather"), TEXT("Stash"), 3, 1);
	Item(TEXT("Backpack.Large"), TEXT("Stash"), 6, 1); Item(TEXT("Material.Herb"), TEXT("Stash"), 48, 12);
	Item(TEXT("Material.Ore"), TEXT("Stash"), 51, 6); Item(TEXT("Trophy.Scroll"), TEXT("Stash"), 55, 1);
	Item(TEXT("Heal.Pill"), TEXT("Carry"), 0, 8); Item(TEXT("Trophy.Jade"), TEXT("Secure"), 0, 1);
	Item(TEXT("Currency.Test"), TEXT("Wallet"), 0, TestMoney);
	FShanmenItemRepository R;
	return R.TryLoadSnapshot(S) ? R.CaptureSnapshot() : FShanmenItemAuthoritySnapshot();
}
