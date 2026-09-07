#include "demo_mapItemDefinitions.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardGenerationRegistry.h"
#include "demo_mapRewardJackpot.h"
#include "demo_mapRewardRareExtreme.h"
#include "demo_mapSearchContainerTypes.h"

#include <initializer_list>

const FName Fdemo_mapItemIds::InventoryContainer(TEXT("Run.Inventory"));
const FName Fdemo_mapItemIds::EquipmentContainer(TEXT("Run.Equipment"));
const FName Fdemo_mapItemIds::WorldContainer(TEXT("Run.World"));
const FName Fdemo_mapItemIds::SessionStashContainer(TEXT("Session.Stash"));
const FName Fdemo_mapItemIds::LocalPlayerOwner(TEXT("Player.Local"));

const FName Fdemo_mapItemIds::WeaponSlot(TEXT("Prototype.Slot.Weapon"));
const FName Fdemo_mapItemIds::ArmorSlot(TEXT("Prototype.Slot.Armor"));
const FName Fdemo_mapItemIds::AccessorySlot(TEXT("Prototype.Slot.Accessory"));
const FName Fdemo_mapItemIds::SpatialRingSlot(TEXT("Prototype.Slot.SpatialRing"));
const FName Fdemo_mapItemIds::BackpackSlot(TEXT("Prototype.Slot.Backpack"));

const FName Fdemo_mapItemIds::WeaponCategory(TEXT("Prototype.ItemCategory.Weapon"));
const FName Fdemo_mapItemIds::ArmorCategory(TEXT("Prototype.ItemCategory.Armor"));
const FName Fdemo_mapItemIds::AccessoryCategory(TEXT("Prototype.ItemCategory.Accessory"));
const FName Fdemo_mapItemIds::SpatialRingCategory(TEXT("Prototype.ItemCategory.SpatialRing"));
const FName Fdemo_mapItemIds::BackpackCategory(TEXT("Prototype.ItemCategory.Backpack"));
const FName Fdemo_mapItemIds::MaterialCategory(TEXT("Prototype.ItemCategory.Material"));
const FName Fdemo_mapItemIds::ConsumableCategory(TEXT("Prototype.ItemCategory.Consumable"));
const FName Fdemo_mapItemIds::CoreCategory(TEXT("Prototype.ItemCategory.Core"));
const FName Fdemo_mapItemIds::LootCategory(TEXT("Prototype.ItemCategory.Loot"));

const FName Fdemo_mapItemIds::TrainingBlade(TEXT("Prototype.Item.Weapon.TrainingBlade"));
const FName Fdemo_mapItemIds::TrainingVest(TEXT("Prototype.Item.Armor.TrainingVest"));
const FName Fdemo_mapItemIds::WindTalisman(TEXT("Prototype.Item.Accessory.WindTalisman"));
const FName Fdemo_mapItemIds::SpiritDust(TEXT("Prototype.Item.Material.SpiritDust"));
const FName Fdemo_mapItemIds::HeavyPracticeBlade(TEXT("Prototype.Item.Weapon.HeavyPracticeBlade"));
const FName Fdemo_mapItemIds::ReinforcedVest(TEXT("Prototype.Item.Armor.ReinforcedVest"));
const FName Fdemo_mapItemIds::EvasionCharm(TEXT("Prototype.Item.Accessory.EvasionCharm"));
const FName Fdemo_mapItemIds::HeartProtectingMirror(TEXT("Prototype.Item.Accessory.HeartProtectingMirror"));
const FName Fdemo_mapItemIds::IronShard(TEXT("Prototype.Item.Material.IronShard"));
const FName Fdemo_mapItemIds::AncientToken(TEXT("Prototype.Item.Loot.AncientToken"));

const FName Fdemo_mapItemIds::WeaponLevel1(TEXT("Prototype.Item.Weapon.Level1"));
const FName Fdemo_mapItemIds::WeaponLevel2(TEXT("Prototype.Item.Weapon.Level2"));
const FName Fdemo_mapItemIds::WeaponLevel3(TEXT("Prototype.Item.Weapon.Level3"));
const FName Fdemo_mapItemIds::WeaponLevel4(TEXT("Prototype.Item.Weapon.Level4"));
const FName Fdemo_mapItemIds::ArmorRobeLevel1(TEXT("Prototype.Item.Armor.Robe.Level1"));
const FName Fdemo_mapItemIds::ArmorRobeLevel2(TEXT("Prototype.Item.Armor.Robe.Level2"));
const FName Fdemo_mapItemIds::ArmorRobeLevel3(TEXT("Prototype.Item.Armor.Robe.Level3"));
const FName Fdemo_mapItemIds::ArmorRobeLevel4(TEXT("Prototype.Item.Armor.Robe.Level4"));
const FName Fdemo_mapItemIds::SpiritGuardRobe(TEXT("Prototype.Item.Armor.SpiritGuardRobe"));
const FName Fdemo_mapItemIds::AccessoryLevel1(TEXT("Prototype.Item.Accessory.Level1"));
const FName Fdemo_mapItemIds::AccessoryLevel2(TEXT("Prototype.Item.Accessory.Level2"));
const FName Fdemo_mapItemIds::AccessoryLevel3(TEXT("Prototype.Item.Accessory.Level3"));
const FName Fdemo_mapItemIds::AccessoryLevel4(TEXT("Prototype.Item.Accessory.Level4"));
const FName Fdemo_mapItemIds::BackpackLevel1(TEXT("Prototype.Item.Backpack.Level1"));
const FName Fdemo_mapItemIds::BackpackLevel2(TEXT("Prototype.Item.Backpack.Level2"));
const FName Fdemo_mapItemIds::SpiritWoodLevel1(TEXT("Prototype.Item.Material.SpiritWood.Level1"));
const FName Fdemo_mapItemIds::SpiritWoodLevel2(TEXT("Prototype.Item.Material.SpiritWood.Level2"));
const FName Fdemo_mapItemIds::SpiritWoodLevel3(TEXT("Prototype.Item.Material.SpiritWood.Level3"));
const FName Fdemo_mapItemIds::SpiritOreLevel1(TEXT("Prototype.Item.Material.SpiritOre.Level1"));
const FName Fdemo_mapItemIds::SpiritOreLevel2(TEXT("Prototype.Item.Material.SpiritOre.Level2"));
const FName Fdemo_mapItemIds::SpiritOreLevel3(TEXT("Prototype.Item.Material.SpiritOre.Level3"));
const FName Fdemo_mapItemIds::HealingPillLevel1(TEXT("Prototype.Item.Consumable.HealingPill.Level1"));
const FName Fdemo_mapItemIds::HealingPillLevel2(TEXT("Prototype.Item.Consumable.HealingPill.Level2"));
const FName Fdemo_mapItemIds::HealingPillLevel3(TEXT("Prototype.Item.Consumable.HealingPill.Level3"));
const FName Fdemo_mapItemIds::MeridianStabilizingPillLevel1(TEXT("Prototype.Item.Consumable.MeridianStabilizingPill.Level1"));
const FName Fdemo_mapItemIds::TrainingThrowingKnife(TEXT("Prototype.Item.Consumable.TrainingThrowingKnife"));
const FName Fdemo_mapItemIds::TrainingFlyingSword(TEXT("Prototype.Item.Weapon.TrainingFlyingSword"));
const FName Fdemo_mapItemIds::SoulBone(TEXT("Prototype.Item.Material.SoulBone"));
const FName Fdemo_mapItemIds::SpiritBone(TEXT("Prototype.Item.Material.SpiritBone"));
const FName Fdemo_mapItemIds::DaoBone(TEXT("Prototype.Item.Material.DaoBone"));
const FName Fdemo_mapItemIds::InnerCoreLevel5(TEXT("Prototype.Item.Core.Inner.Level5"));
const FName Fdemo_mapItemIds::InnerCoreLevel10(TEXT("Prototype.Item.Core.Inner.Level10"));
const FName Fdemo_mapItemIds::InnerCoreLevel15(TEXT("Prototype.Item.Core.Inner.Level15"));

const FName Fdemo_mapItemEffectIds::AttackBonus(TEXT("Effect.AttackBonus"));
const FName Fdemo_mapItemEffectIds::MaxHealthBonus(TEXT("Effect.MaxHealthBonus"));
const FName Fdemo_mapItemEffectIds::FlatDamageReduction(TEXT("Effect.FlatDamageReduction"));
const FName Fdemo_mapItemEffectIds::CooldownMultiplier(TEXT("Effect.CooldownMultiplier"));
const FName Fdemo_mapItemEffectIds::TotalCapacity(TEXT("Effect.TotalCapacity"));
const FName Fdemo_mapItemEffectIds::RingQuickCapacity(TEXT("Effect.RingQuickCapacity"));
const FName Fdemo_mapItemEffectIds::HealAmount(TEXT("Effect.HealAmount"));
const FName Fdemo_mapItemEffectIds::LethalVitalityFloor(TEXT("Effect.LethalVitalityFloor"));

const FName Fdemo_mapLootTableIds::EnemyMelee(TEXT("Prototype.LootTable.Enemy.Melee"));
const FName Fdemo_mapLootTableIds::EnemyRanged(TEXT("Prototype.LootTable.Enemy.Ranged"));
const FName Fdemo_mapLootTableIds::EnemyHeavy(TEXT("Prototype.LootTable.Enemy.Heavy"));

namespace
{
	Fdemo_mapModifierSpec MakeModifier(FName AttributeId, Edemo_mapModifierOperation Operation, float Value)
	{
		Fdemo_mapModifierSpec Modifier;
		Modifier.AttributeId = AttributeId;
		Modifier.Operation = Operation;
		Modifier.Value = Value;
		Modifier.Priority = 0;
		return Modifier;
	}

	Fdemo_mapItemEffectParameter MakeEffect(FName ParameterId, double Value)
	{
		Fdemo_mapItemEffectParameter Effect;
		Effect.ParameterId = ParameterId;
		Effect.Value = Value;
		return Effect;
	}

	Fdemo_mapItemDefinition MakeDefinition(
		FName Id,
		const TCHAR* DisplayName,
		const TCHAR* WorldLabelName,
		FName Category,
		int32 Level,
		int32 MaxStack,
		FName EquipmentSlotId,
		TArray<FName> CompatibleSlots,
		TArray<Fdemo_mapModifierSpec> Modifiers,
		TArray<Fdemo_mapItemEffectParameter> Effects,
		bool bPurchasable,
		bool bSellable,
		int64 BuyPrice,
		int64 SellPrice,
		int32 PrototypeValue,
		int32 MaxDurability = 0,
		int32 MaxCharges = 0,
		TArray<Edemo_mapItemGameplaySemantic> GameplaySemantics = {})
	{
		Fdemo_mapItemDefinition Definition;
		Definition.DefinitionId = Id;
		Definition.DisplayName = FText::FromString(DisplayName);
		Definition.WorldLabelName = WorldLabelName;
		Definition.CategoryId = Category;
		Definition.Level = Level;
		Definition.MaxStackSize = MaxStack;
		Definition.MaxDurability = MaxDurability;
		Definition.MaxCharges = MaxCharges;
		Definition.EquipmentSlotId = EquipmentSlotId;
		Definition.CompatibleSlotIds = MoveTemp(CompatibleSlots);
		Definition.Modifiers = MoveTemp(Modifiers);
		Definition.EffectParameters = MoveTemp(Effects);
		Definition.GameplaySemantics = MoveTemp(GameplaySemantics);
		Definition.bPurchasable = bPurchasable;
		Definition.bSellable = bSellable;
		Definition.BuyPrice = BuyPrice;
		Definition.SellPrice = SellPrice;
		Definition.PrototypeValue = PrototypeValue;
		Definition.WorldPresentationId = Category;
		Definition.ContentVersionId = Fdemo_mapItemDefinitions::GetContentVersionId();
		Definition.ContentDigest = Fdemo_mapItemDefinitions::GetContentDigest();
		Definition.bWorldDropEligible = true;
		Definition.bHotbarEligible = Category == Fdemo_mapItemIds::ConsumableCategory;
		return Definition;
	}

	Fdemo_mapRuntimeContainerSeedEntry ProfileEntry(
		Edemo_mapRuntimeContainerSection Section,
		int32 Slot,
		FName Definition,
		int32 Quantity)
	{
		Fdemo_mapRuntimeContainerSeedEntry Result;
		Result.Section = Section;
		Result.SlotIndex = Slot;
		Result.DefinitionId = Definition;
		Result.StackCount = Quantity;
		return Result;
	}

	Fdemo_mapFixedLootTableDefinition Profile(
		FName Id,
		Edemo_mapRuntimeContainerKind Kind,
		std::initializer_list<Fdemo_mapRuntimeContainerSeedEntry> Entries,
		int32 Total,
		FName FixedEquipmentDefinitionId = NAME_None)
	{
		Fdemo_mapFixedLootTableDefinition Result;
		Result.TableId = Id;
		Result.ContentVersionId = Fdemo_mapItemDefinitions::GetContentVersionId();
		Result.ContentDigest = Fdemo_mapItemDefinitions::GetContentDigest();
		Result.Kind = Kind;
		Result.CandidateWeight = 1;
		Result.NoDropWeight = 0;
		Result.FixedEquipmentDefinitionId = FixedEquipmentDefinitionId;
		for (const Fdemo_mapRuntimeContainerSeedEntry& Seed : Entries)
		{
			Result.Entries.Add(Seed);
		}
		Result.TotalPrototypeValue = Total;
		return Result;
	}

	Fdemo_mapRewardBudgetProfile GeneratedBudget(
		FName Id,
		int32 PlannedCount,
		int64 BaseValue)
	{
		Fdemo_mapRewardBudgetProfile Result;
		Result.ProfileId = Id;
		Result.PlannedSourceCount = PlannedCount;
		Result.BaseValue = BaseValue;
		Result.MinMultiplierBps = Fdemo_mapRewardGenerationRegistry::NormalMultiplierMinBps;
		Result.MaxMultiplierBps = Fdemo_mapRewardGenerationRegistry::NormalMultiplierMaxBps;
		return Result;
	}

	Fdemo_mapRewardPoolEntry GeneratedPool(
		const TCHAR* EntryId,
		FName DefinitionId,
		FName ItemTag,
		int64 Weight,
		int32 MaxStack)
	{
		Fdemo_mapRewardPoolEntry Result;
		Result.EntryId = FName(EntryId);
		Result.DefinitionId = DefinitionId;
		Result.ItemTags = { ItemTag };
		Result.RequiredSourceTags = { Fdemo_mapRewardTagIds::SourceContainerGeneral, Fdemo_mapRewardTagIds::SourceContainerHighValue };
		Result.Weight = Weight;
		Result.MinStack = 1;
		Result.MaxStack = MaxStack;
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
		Result.MinItemLevel = Definition ? Definition->Level : 0;
		Result.MaxItemLevel = Definition ? Definition->Level : MAX_int32;
		Result.MinUnitValue = 1;
		Result.MaxUnitValue = MAX_int64;
		return Result;
	}

	Fdemo_mapRewardProjectionSection GeneratedProjectionSection(
		FName Id,
		Edemo_mapRuntimeContainerSection Runtime,
		FName Tag,
		int32 Weight,
		int32 Capacity,
		int32 Priority)
	{
		Fdemo_mapRewardProjectionSection Result;
		Result.SectionId = Id;
		Result.RuntimeSection = Runtime;
		Result.SectionTags = { Tag };
		Result.BudgetWeightBps = Weight;
		Result.Capacity = Capacity;
		Result.bRequiredNonEmpty = true;
		Result.ResidualRedistributionPriority = Priority;
		return Result;
	}

	Fdemo_mapRewardSourceProjection GeneratedChestProjection(
		FName ProjectionId,
		FName RoleId,
		FName MarkerId,
		FName ProfileId,
		FName SourceTag,
		FName Fallback)
	{
		Fdemo_mapRewardSourceProjection Result;
		Result.ProjectionId = ProjectionId;
		Result.ContentVersionId = Fdemo_mapItemDefinitions::GetContentVersionId();
		Result.ContentDigest = Fdemo_mapItemDefinitions::GetContentDigest();
		Result.StableSourceRoleId = RoleId;
		Result.MarkerId = MarkerId;
		Result.BudgetProfileId = ProfileId;
		Result.JackpotPolicyId = Fdemo_mapRewardJackpotPolicyRegistry::DefaultPolicyId;
		Result.RareExtremePolicyId = Fdemo_mapRewardRareExtremePolicyRegistry::DefaultPolicyId;
		Result.AffixPolicyId = Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		Result.SourceTags = { Fdemo_mapRewardTagIds::SourceContainerGeneral, SourceTag };
		Result.Sections = { GeneratedProjectionSection(TEXT("Chest"), Edemo_mapRuntimeContainerSection::Chest, SourceTag, 10000, Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity, 0) };
		Result.FixedFallbackTableId = Fallback;
		return Result;
	}

	Fdemo_mapRewardSourceProjection GeneratedCorpseProjection(
		FName ProjectionId,
		FName EncounterId,
		FName ProfileId,
		FName Fallback)
	{
		Fdemo_mapRewardSourceProjection Result;
		Result.ProjectionId = ProjectionId;
		Result.ContentVersionId = Fdemo_mapItemDefinitions::GetContentVersionId();
		Result.ContentDigest = Fdemo_mapItemDefinitions::GetContentDigest();
		Result.StableSourceRoleId = EncounterId;
		Result.EncounterId = EncounterId;
		Result.BudgetProfileId = ProfileId;
		Result.JackpotPolicyId = Fdemo_mapRewardJackpotPolicyRegistry::DefaultPolicyId;
		Result.RareExtremePolicyId = Fdemo_mapRewardRareExtremePolicyRegistry::DefaultPolicyId;
		Result.AffixPolicyId = Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		Result.SourceTags = { Fdemo_mapRewardProjectionTagIds::SourceCorpse };
		Result.Sections = {
			GeneratedProjectionSection(TEXT("Equipment"), Edemo_mapRuntimeContainerSection::Equipment, Fdemo_mapRewardProjectionTagIds::SectionEquipment, 4000, Fdemo_mapSearchContainerPrototypeConfig::CorpseEquipmentCapacity, 0),
			GeneratedProjectionSection(TEXT("Backpack"), Edemo_mapRuntimeContainerSection::Backpack, Fdemo_mapRewardProjectionTagIds::SectionBackpack, 3500, Fdemo_mapSearchContainerPrototypeConfig::CorpseRewardBackpackCapacity, 1),
			GeneratedProjectionSection(TEXT("Body"), Edemo_mapRuntimeContainerSection::Body, Fdemo_mapRewardProjectionTagIds::SectionBody, 2500, Fdemo_mapSearchContainerPrototypeConfig::CorpseBodyCapacity, 2)
		};
		Result.FixedFallbackTableId = Fallback;
		return Result;
	}

	Fdemo_mapRewardSourceProjection GeneratedBossProjection()
	{
		Fdemo_mapRewardSourceProjection Result = GeneratedCorpseProjection(
			Fdemo_mapRewardProjectionIds::CorpseBossPrototype,
			Fdemo_mapEnemyEncounterIds::MainMeleeHeavy,
			Fdemo_mapRewardBudgetProfileIds::Boss,
			Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy);
		Result.StableSourceRoleId = Fdemo_mapRewardSourceRoleIds::BossPrototype;
		Result.SourceTags = {
			Fdemo_mapRewardProjectionTagIds::SourceBoss,
			Fdemo_mapRewardProjectionTagIds::SourceCorpse,
			Fdemo_mapRewardProjectionTagIds::ValueHigh,
			Fdemo_mapRewardProjectionTagIds::Generated
		};
		Result.Sections[0].Capacity = 3;
		Result.Sections[1].Capacity = 1;
		Result.Sections[2].Capacity = 2;
		Result.bAllowFixedFallbackOnFailure = false;
		Result.BaseSourceValue = 12000;
		Result.MinGeneratedStacks = 3;
		Result.MaxGeneratedStacks = 6;
		Result.RequiredEquipmentCount = 1;
		Result.SourceDisplayLabel = TEXT("BOSS REWARD");
		return Result;
	}

	Fdemo_mapRewardDistributionProfile GeneratedDistributionProfile(
		FName ProfileId,
		FName ProjectionId,
		FName BudgetProfileId,
		TArray<FName> SourceTags,
		int64 BaseSourceValue)
	{
		Fdemo_mapRewardDistributionProfile Result;
		Result.ProfileId = ProfileId;
		Result.ContentVersionId = Fdemo_mapItemDefinitions::GetContentVersionId();
		Result.ContentDigest = Fdemo_mapItemDefinitions::GetContentDigest();
		Result.ProjectionId = ProjectionId;
		Result.BudgetProfileId = BudgetProfileId;
		Result.SourceTags = MoveTemp(SourceTags);
		Result.BaseSourceValue = BaseSourceValue;
		Result.bAllowFixedFallbackOnFailure = false;
		return Result;
	}
}

bool Fdemo_mapRewardDistributionProfile::IsValid() const
{
	return !ProfileId.IsNone()
		&& !ProjectionId.IsNone()
		&& !BudgetProfileId.IsNone()
		&& !SourceTags.IsEmpty()
		&& !SourceTags.Contains(NAME_None)
		&& BaseSourceValue > 0
		&& Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			ContentVersionId,
			ContentDigest);
}

FName Fdemo_mapItemDefinitions::GetContentVersionId()
{
	return FName(TEXT("CodeB.Content.0.0.10.P21.0"));
}

const FString& Fdemo_mapItemDefinitions::GetContentDigest()
{
	// This is a content-contract digest, not a save migration key. Existing
	// persisted items keep their DefinitionId and are never remapped by P73.
	static const FString Digest(TEXT("A52AA4EEE9DBF314C017205BFC6E417B8C9A108D37471C7DDE088013B85FC685"));
	return Digest;
}

bool Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
	FName ContentVersionId,
	const FString& ContentDigest)
{
	return ContentVersionId == GetContentVersionId()
		&& ContentDigest == GetContentDigest();
}

bool Fdemo_mapItemDefinitions::IsKnownContentIdentity(
	FName ContentVersionId,
	const FString& ContentDigest)
{
	return IsCurrentContentIdentity(ContentVersionId, ContentDigest)
		|| (ContentVersionId == FName(TEXT("CodeB.Content.0.0.10.P18.4"))
			&& ContentDigest == TEXT("D6EF276B3BB268D33A9E1242DC3620A7F33F2B4B966503E1056BA3E381C7B043"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.0.0.10.P17.0"))
			&& ContentDigest == TEXT("5C09D58AA2EB206FA39F2BE896F7B071149CE8FE3D4E75780CFE213432638399"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.0.0.10.P16.0"))
			&& ContentDigest == TEXT("9B789DB381BB934F772326A5217094F19C654D5AF51D5623EDB0A108DBA4CC7B"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.0.0.10.P11.7"))
			&& ContentDigest == TEXT("6D01652004E386DC469CB09FF0F3A77110C53841F6AF3F66F5600C3C9B9B4179"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.0.0.10.P7.7"))
			&& ContentDigest == TEXT("6C30E84A05386A7986A2344DB8247961E75F0FE2179F41927A0C7DF950F45A00"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.0.0.10.P5.4"))
			&& ContentDigest == TEXT("32A1BA2A026369525D43CB56C21311C661E59B22BDFA2C877FE93B5C58F637F4"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.P73.3"))
			&& ContentDigest == TEXT("A263AB7F10B960B30B584A8C67042597E2E1A8967E15B0F998A36D17E1EEA4B2"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.P73.2"))
			&& ContentDigest == TEXT("BCE122E15F3C3278A2AAC9C09D831DC64C7D13D196009D59161CC382F4C42E4B"))
		|| (ContentVersionId == FName(TEXT("CodeB.Content.P73.0"))
			&& ContentDigest == TEXT("8D596D4C347D9EAD2B63862D5A8F5D733B51D12A6A0F96A73698DD7F042A43D9"));
}

const TArray<Fdemo_mapItemDefinition>& Fdemo_mapItemDefinitions::GetAll()
{
	static const TArray<Fdemo_mapItemDefinition> Definitions = {
		MakeDefinition(Fdemo_mapItemIds::TrainingBlade, TEXT("训练武器"), TEXT("TRAINING BLADE"), Fdemo_mapItemIds::WeaponCategory, 0, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, { MakeModifier(Fdemo_mapAttributeIds::AttackPower, Edemo_mapModifierOperation::Add, 1.0f) }, {}, false, true, 0, 25, 100, 0, 0, { Edemo_mapItemGameplaySemantic::WeaponGuard, Edemo_mapItemGameplaySemantic::SwordQiSource }),
		MakeDefinition(Fdemo_mapItemIds::TrainingVest, TEXT("训练防具"), TEXT("TRAINING VEST"), Fdemo_mapItemIds::ArmorCategory, 0, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, { MakeModifier(Fdemo_mapAttributeIds::MaxHealth, Edemo_mapModifierOperation::Add, 2.0f) }, {}, false, true, 0, 25, 80),
		MakeDefinition(Fdemo_mapItemIds::WindTalisman, TEXT("疾风纳物戒"), TEXT("WIND SPATIAL RING"), Fdemo_mapItemIds::SpatialRingCategory, 0, 1, Fdemo_mapItemIds::SpatialRingSlot, { Fdemo_mapItemIds::SpatialRingSlot }, { MakeModifier(Fdemo_mapAttributeIds::MoveSpeed, Edemo_mapModifierOperation::Multiply, 1.10f) }, { MakeEffect(Fdemo_mapItemEffectIds::RingQuickCapacity, 4.0) }, false, true, 0, 25, 60),
		MakeDefinition(Fdemo_mapItemIds::SpiritDust, TEXT("灵尘"), TEXT("SPIRIT DUST"), Fdemo_mapItemIds::MaterialCategory, 0, 5, NAME_None, {}, {}, {}, false, true, 0, 5, 1),
		MakeDefinition(Fdemo_mapItemIds::HeavyPracticeBlade, TEXT("重型练习刀"), TEXT("HEAVY PRACTICE BLADE"), Fdemo_mapItemIds::WeaponCategory, 0, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, { MakeModifier(Fdemo_mapAttributeIds::AttackPower, Edemo_mapModifierOperation::Add, 2.0f) }, {}, false, false, 0, 0, 180, 0, 0, { Edemo_mapItemGameplaySemantic::WeaponGuard, Edemo_mapItemGameplaySemantic::SwordQiSource }),
		MakeDefinition(Fdemo_mapItemIds::ReinforcedVest, TEXT("加固训练甲"), TEXT("REINFORCED VEST"), Fdemo_mapItemIds::ArmorCategory, 0, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, { MakeModifier(Fdemo_mapAttributeIds::MaxHealth, Edemo_mapModifierOperation::Add, 3.0f) }, {}, false, false, 0, 0, 150),
		MakeDefinition(Fdemo_mapItemIds::EvasionCharm, TEXT("避影符"), TEXT("EVASION CHARM"), Fdemo_mapItemIds::AccessoryCategory, 0, 1, Fdemo_mapItemIds::AccessorySlot, { Fdemo_mapItemIds::AccessorySlot }, { MakeModifier(Fdemo_mapAttributeIds::DodgeChance, Edemo_mapModifierOperation::Add, 0.10f) }, {}, false, false, 0, 0, 120),
		MakeDefinition(Fdemo_mapItemIds::HeartProtectingMirror, TEXT("护心镜"), TEXT("HEART-PROTECTING MIRROR"), Fdemo_mapItemIds::AccessoryCategory, 1, 1, Fdemo_mapItemIds::AccessorySlot, { Fdemo_mapItemIds::AccessorySlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::LethalVitalityFloor, 1.0) }, false, true, 0, 100, 240, 0, 1, { Edemo_mapItemGameplaySemantic::LethalInterception }),
		MakeDefinition(Fdemo_mapItemIds::IronShard, TEXT("玄铁碎片"), TEXT("IRON SHARD"), Fdemo_mapItemIds::MaterialCategory, 0, 5, NAME_None, {}, {}, {}, false, true, 0, 10, 3),
		MakeDefinition(Fdemo_mapItemIds::AncientToken, TEXT("古旧令牌"), TEXT("ANCIENT TOKEN"), Fdemo_mapItemIds::LootCategory, 0, 1, NAME_None, {}, {}, {}, false, true, 0, 20, 500),

		MakeDefinition(Fdemo_mapItemIds::WeaponLevel1, TEXT("一阶兵器"), TEXT("TIER 1 WEAPON"), Fdemo_mapItemIds::WeaponCategory, 1, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::AttackBonus, 1.0) }, true, true, 100, 50, 50, 0, 0, { Edemo_mapItemGameplaySemantic::WeaponGuard, Edemo_mapItemGameplaySemantic::SwordQiSource }),
		MakeDefinition(Fdemo_mapItemIds::WeaponLevel2, TEXT("二阶兵器"), TEXT("TIER 2 WEAPON"), Fdemo_mapItemIds::WeaponCategory, 2, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::AttackBonus, 2.0) }, true, true, 200, 100, 100, 0, 0, { Edemo_mapItemGameplaySemantic::WeaponGuard, Edemo_mapItemGameplaySemantic::SwordQiSource }),
		MakeDefinition(Fdemo_mapItemIds::WeaponLevel3, TEXT("三阶兵器"), TEXT("TIER 3 WEAPON"), Fdemo_mapItemIds::WeaponCategory, 3, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::AttackBonus, 3.0) }, true, true, 400, 200, 200, 0, 0, { Edemo_mapItemGameplaySemantic::WeaponGuard, Edemo_mapItemGameplaySemantic::SwordQiSource }),
		MakeDefinition(Fdemo_mapItemIds::WeaponLevel4, TEXT("四阶兵器"), TEXT("TIER 4 WEAPON"), Fdemo_mapItemIds::WeaponCategory, 4, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::AttackBonus, 4.0) }, true, true, 800, 400, 400, 0, 0, { Edemo_mapItemGameplaySemantic::WeaponGuard, Edemo_mapItemGameplaySemantic::SwordQiSource }),

		MakeDefinition(Fdemo_mapItemIds::ArmorRobeLevel1, TEXT("一阶道袍"), TEXT("TIER 1 DAO ROBE"), Fdemo_mapItemIds::ArmorCategory, 1, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::MaxHealthBonus, 2.0), MakeEffect(Fdemo_mapItemEffectIds::FlatDamageReduction, 1.0) }, true, true, 100, 50, 50),
		MakeDefinition(Fdemo_mapItemIds::ArmorRobeLevel2, TEXT("二阶道袍"), TEXT("TIER 2 DAO ROBE"), Fdemo_mapItemIds::ArmorCategory, 2, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::MaxHealthBonus, 4.0), MakeEffect(Fdemo_mapItemEffectIds::FlatDamageReduction, 2.0) }, true, true, 200, 100, 100),
		MakeDefinition(Fdemo_mapItemIds::ArmorRobeLevel3, TEXT("三阶道袍"), TEXT("TIER 3 DAO ROBE"), Fdemo_mapItemIds::ArmorCategory, 3, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::MaxHealthBonus, 6.0), MakeEffect(Fdemo_mapItemEffectIds::FlatDamageReduction, 3.0) }, true, true, 400, 200, 200),
		MakeDefinition(Fdemo_mapItemIds::ArmorRobeLevel4, TEXT("四阶道袍"), TEXT("TIER 4 DAO ROBE"), Fdemo_mapItemIds::ArmorCategory, 4, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::MaxHealthBonus, 8.0), MakeEffect(Fdemo_mapItemEffectIds::FlatDamageReduction, 4.0) }, true, true, 800, 400, 400),
		MakeDefinition(Fdemo_mapItemIds::SpiritGuardRobe, TEXT("护体法袍"), TEXT("SPIRIT GUARD ROBE"), Fdemo_mapItemIds::ArmorCategory, 2, 1, Fdemo_mapItemIds::ArmorSlot, { Fdemo_mapItemIds::ArmorSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::FlatDamageReduction, 2.0) }, false, true, 0, 80, 160, 20),

		MakeDefinition(Fdemo_mapItemIds::AccessoryLevel1, TEXT("一阶纳物戒"), TEXT("TIER 1 SPATIAL RING"), Fdemo_mapItemIds::SpatialRingCategory, 1, 1, Fdemo_mapItemIds::SpatialRingSlot, { Fdemo_mapItemIds::SpatialRingSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::CooldownMultiplier, 0.95), MakeEffect(Fdemo_mapItemEffectIds::RingQuickCapacity, 6.0) }, true, true, 80, 40, 40),
		MakeDefinition(Fdemo_mapItemIds::AccessoryLevel2, TEXT("二阶纳物戒"), TEXT("TIER 2 SPATIAL RING"), Fdemo_mapItemIds::SpatialRingCategory, 2, 1, Fdemo_mapItemIds::SpatialRingSlot, { Fdemo_mapItemIds::SpatialRingSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::CooldownMultiplier, 0.90), MakeEffect(Fdemo_mapItemEffectIds::RingQuickCapacity, 8.0) }, false, true, 0, 80, 80),
		MakeDefinition(Fdemo_mapItemIds::AccessoryLevel3, TEXT("三阶纳物戒"), TEXT("TIER 3 SPATIAL RING"), Fdemo_mapItemIds::SpatialRingCategory, 3, 1, Fdemo_mapItemIds::SpatialRingSlot, { Fdemo_mapItemIds::SpatialRingSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::CooldownMultiplier, 0.85), MakeEffect(Fdemo_mapItemEffectIds::RingQuickCapacity, 10.0) }, false, true, 0, 160, 160),
		MakeDefinition(Fdemo_mapItemIds::AccessoryLevel4, TEXT("四阶纳物戒"), TEXT("TIER 4 SPATIAL RING"), Fdemo_mapItemIds::SpatialRingCategory, 4, 1, Fdemo_mapItemIds::SpatialRingSlot, { Fdemo_mapItemIds::SpatialRingSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::CooldownMultiplier, 0.80), MakeEffect(Fdemo_mapItemEffectIds::RingQuickCapacity, 12.0) }, false, true, 0, 320, 320),

		MakeDefinition(Fdemo_mapItemIds::BackpackLevel1, TEXT("吞天袋"), TEXT("HEAVEN BAG"), Fdemo_mapItemIds::BackpackCategory, 1, 1, Fdemo_mapItemIds::BackpackSlot, { Fdemo_mapItemIds::BackpackSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::TotalCapacity, 36.0) }, true, true, 120, 60, 60),
		MakeDefinition(Fdemo_mapItemIds::BackpackLevel2, TEXT("二阶吞天袋"), TEXT("TIER 2 HEAVEN BAG"), Fdemo_mapItemIds::BackpackCategory, 2, 1, Fdemo_mapItemIds::BackpackSlot, { Fdemo_mapItemIds::BackpackSlot }, {}, { MakeEffect(Fdemo_mapItemEffectIds::TotalCapacity, 36.0) }, false, true, 0, 150, 150),

		MakeDefinition(Fdemo_mapItemIds::SpiritWoodLevel1, TEXT("一阶灵木"), TEXT("TIER 1 SPIRIT WOOD"), Fdemo_mapItemIds::MaterialCategory, 1, 99, NAME_None, {}, {}, {}, false, true, 0, 10, 10),
		MakeDefinition(Fdemo_mapItemIds::SpiritWoodLevel2, TEXT("二阶灵木"), TEXT("TIER 2 SPIRIT WOOD"), Fdemo_mapItemIds::MaterialCategory, 2, 99, NAME_None, {}, {}, {}, false, true, 0, 25, 25),
		MakeDefinition(Fdemo_mapItemIds::SpiritWoodLevel3, TEXT("三阶灵木"), TEXT("TIER 3 SPIRIT WOOD"), Fdemo_mapItemIds::MaterialCategory, 3, 99, NAME_None, {}, {}, {}, false, true, 0, 60, 60),
		MakeDefinition(Fdemo_mapItemIds::SpiritOreLevel1, TEXT("一阶灵矿"), TEXT("TIER 1 SPIRIT ORE"), Fdemo_mapItemIds::MaterialCategory, 1, 99, NAME_None, {}, {}, {}, false, true, 0, 15, 15),
		MakeDefinition(Fdemo_mapItemIds::SpiritOreLevel2, TEXT("二阶灵矿"), TEXT("TIER 2 SPIRIT ORE"), Fdemo_mapItemIds::MaterialCategory, 2, 99, NAME_None, {}, {}, {}, false, true, 0, 35, 35),
		MakeDefinition(Fdemo_mapItemIds::SpiritOreLevel3, TEXT("三阶灵矿"), TEXT("TIER 3 SPIRIT ORE"), Fdemo_mapItemIds::MaterialCategory, 3, 99, NAME_None, {}, {}, {}, false, true, 0, 80, 80),

		MakeDefinition(Fdemo_mapItemIds::HealingPillLevel1, TEXT("一阶丹药"), TEXT("TIER 1 HEALING PILL"), Fdemo_mapItemIds::ConsumableCategory, 1, 20, NAME_None, {}, {}, { MakeEffect(Fdemo_mapItemEffectIds::HealAmount, 1.0) }, true, true, 30, 15, 15),
		MakeDefinition(Fdemo_mapItemIds::HealingPillLevel2, TEXT("二阶丹药"), TEXT("TIER 2 HEALING PILL"), Fdemo_mapItemIds::ConsumableCategory, 2, 20, NAME_None, {}, {}, { MakeEffect(Fdemo_mapItemEffectIds::HealAmount, 2.0) }, true, true, 60, 30, 30),
		MakeDefinition(Fdemo_mapItemIds::HealingPillLevel3, TEXT("三阶丹药"), TEXT("TIER 3 HEALING PILL"), Fdemo_mapItemIds::ConsumableCategory, 3, 20, NAME_None, {}, {}, { MakeEffect(Fdemo_mapItemEffectIds::HealAmount, 3.0) }, true, true, 120, 60, 60),
		MakeDefinition(Fdemo_mapItemIds::MeridianStabilizingPillLevel1, TEXT("一阶定脉丹"), TEXT("TIER 1 MERIDIAN STABILIZING PILL"), Fdemo_mapItemIds::ConsumableCategory, 1, 20, NAME_None, {}, {}, {}, true, true, 45, 22, 22, 0, 0, { Edemo_mapItemGameplaySemantic::MeridianShockTreatment }),
		MakeDefinition(Fdemo_mapItemIds::TrainingThrowingKnife, TEXT("练习飞刀"), TEXT("TRAINING THROWING KNIFE"), Fdemo_mapItemIds::ConsumableCategory, 1, 20, NAME_None, {}, {}, {}, true, true, 30, 15, 15, 0, 0, { Edemo_mapItemGameplaySemantic::ThrownWeapon }),

		MakeDefinition(Fdemo_mapItemIds::SoulBone, TEXT("魂骨"), TEXT("SOUL BONE"), Fdemo_mapItemIds::MaterialCategory, 1, 99, NAME_None, {}, {}, {}, false, true, 0, 100, 100),
		MakeDefinition(Fdemo_mapItemIds::SpiritBone, TEXT("灵骨"), TEXT("SPIRIT BONE"), Fdemo_mapItemIds::MaterialCategory, 2, 99, NAME_None, {}, {}, {}, false, true, 0, 250, 250),
		MakeDefinition(Fdemo_mapItemIds::DaoBone, TEXT("道骨"), TEXT("DAO BONE"), Fdemo_mapItemIds::MaterialCategory, 3, 99, NAME_None, {}, {}, {}, false, true, 0, 600, 600),
		MakeDefinition(Fdemo_mapItemIds::InnerCoreLevel5, TEXT("五级内丹"), TEXT("TIER 5 INNER CORE"), Fdemo_mapItemIds::CoreCategory, 5, 99, NAME_None, {}, {}, {}, false, true, 0, 150, 150),
		MakeDefinition(Fdemo_mapItemIds::InnerCoreLevel10, TEXT("十级内丹"), TEXT("TIER 10 INNER CORE"), Fdemo_mapItemIds::CoreCategory, 10, 99, NAME_None, {}, {}, {}, false, true, 0, 400, 400),
		MakeDefinition(Fdemo_mapItemIds::InnerCoreLevel15, TEXT("十五级内丹"), TEXT("TIER 15 INNER CORE"), Fdemo_mapItemIds::CoreCategory, 15, 99, NAME_None, {}, {}, {}, false, true, 0, 1000, 1000),

		// P21.0 is append-only so every pre-existing registry ordinal remains stable.
		MakeDefinition(Fdemo_mapItemIds::TrainingFlyingSword, TEXT("练习飞剑"), TEXT("TRAINING FLYING SWORD"), Fdemo_mapItemIds::WeaponCategory, 1, 1, Fdemo_mapItemIds::WeaponSlot, { Fdemo_mapItemIds::WeaponSlot }, {}, {}, true, true, 100, 50, 100, 0, 0, { Edemo_mapItemGameplaySemantic::FlyingSword })
	};
	return Definitions;
}

const Fdemo_mapItemDefinition* Fdemo_mapItemDefinitions::Find(FName DefinitionId)
{
	return GetAll().FindByPredicate([DefinitionId](const Fdemo_mapItemDefinition& Definition)
	{
		return Definition.DefinitionId == DefinitionId;
	});
}

const TArray<FName>& Fdemo_mapItemDefinitions::GetEquipmentSlotIds()
{
	static const TArray<FName> SlotIds = {
		Fdemo_mapItemIds::WeaponSlot,
		Fdemo_mapItemIds::ArmorSlot,
		Fdemo_mapItemIds::AccessorySlot,
		Fdemo_mapItemIds::SpatialRingSlot,
		Fdemo_mapItemIds::BackpackSlot
	};
	return SlotIds;
}

const TArray<FName>& Fdemo_mapItemDefinitions::GetPurchasableDefinitionIds()
{
	static const TArray<FName> DefinitionIds = []()
	{
		TArray<FName> Result;
		for (const Fdemo_mapItemDefinition& Definition : GetAll())
		{
			if (Definition.bPurchasable)
			{
				Result.Add(Definition.DefinitionId);
			}
		}
		return Result;
	}();
	return DefinitionIds;
}

const TArray<Fdemo_mapFixedLootTableDefinition>&
Fdemo_mapItemDefinitions::GetFixedLootProfiles()
{
	using Section = Edemo_mapRuntimeContainerSection;
	using Kind = Edemo_mapRuntimeContainerKind;
	static const TArray<Fdemo_mapFixedLootTableDefinition> Profiles = {
		Profile(Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard, Kind::Corpse, {
			ProfileEntry(Section::Equipment, 0, Fdemo_mapItemIds::WeaponLevel1, 1),
			ProfileEntry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritOreLevel1, 2),
			ProfileEntry(Section::Body, 0, Fdemo_mapItemIds::SoulBone, 1)}, 180, Fdemo_mapItemIds::WeaponLevel1),
		Profile(Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy, Kind::Corpse, {
			ProfileEntry(Section::Equipment, 0, Fdemo_mapItemIds::ArmorRobeLevel1, 1),
			ProfileEntry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritWoodLevel1, 2),
			ProfileEntry(Section::Body, 0, Fdemo_mapItemIds::SoulBone, 1)}, 170, Fdemo_mapItemIds::ArmorRobeLevel1),
		Profile(Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard, Kind::Corpse, {
			ProfileEntry(Section::Equipment, 0, Fdemo_mapItemIds::AccessoryLevel1, 1),
			ProfileEntry(Section::Backpack, 0, Fdemo_mapItemIds::HealingPillLevel1, 1),
			ProfileEntry(Section::Body, 0, Fdemo_mapItemIds::SoulBone, 1)}, 155, Fdemo_mapItemIds::AccessoryLevel1),
		Profile(Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced, Kind::Corpse, {
			ProfileEntry(Section::Equipment, 0, Fdemo_mapItemIds::WeaponLevel2, 1),
			ProfileEntry(Section::Equipment, 1, Fdemo_mapItemIds::BackpackLevel2, 1),
			ProfileEntry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritOreLevel2, 2),
			ProfileEntry(Section::Body, 0, Fdemo_mapItemIds::SpiritBone, 1),
			ProfileEntry(Section::Body, 1, Fdemo_mapItemIds::InnerCoreLevel10, 1)}, 970, Fdemo_mapItemIds::WeaponLevel2),
		Profile(Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced, Kind::Corpse, {
			ProfileEntry(Section::Equipment, 0, Fdemo_mapItemIds::ArmorRobeLevel2, 1),
			ProfileEntry(Section::Equipment, 1, Fdemo_mapItemIds::AccessoryLevel2, 1),
			ProfileEntry(Section::Backpack, 0, Fdemo_mapItemIds::SpiritWoodLevel3, 2),
			ProfileEntry(Section::Body, 0, Fdemo_mapItemIds::SpiritBone, 1),
			ProfileEntry(Section::Body, 1, Fdemo_mapItemIds::InnerCoreLevel10, 1)}, 950, Fdemo_mapItemIds::ArmorRobeLevel2),
		Profile(Fdemo_mapFixedLootTableIds::ChestMainA, Kind::Chest, {
			ProfileEntry(Section::Chest, 0, Fdemo_mapItemIds::SpiritWoodLevel1, 2),
			ProfileEntry(Section::Chest, 1, Fdemo_mapItemIds::HealingPillLevel1, 1)}, 35),
		Profile(Fdemo_mapFixedLootTableIds::ChestMainB, Kind::Chest, {
			ProfileEntry(Section::Chest, 0, Fdemo_mapItemIds::SpiritOreLevel1, 2),
			ProfileEntry(Section::Chest, 1, Fdemo_mapItemIds::HealingPillLevel1, 1)}, 45),
		Profile(Fdemo_mapFixedLootTableIds::ChestSideA, Kind::Chest, {
			ProfileEntry(Section::Chest, 0, Fdemo_mapItemIds::SpiritWoodLevel2, 2),
			ProfileEntry(Section::Chest, 1, Fdemo_mapItemIds::SpiritOreLevel2, 2),
			ProfileEntry(Section::Chest, 2, Fdemo_mapItemIds::HealingPillLevel2, 1)}, 150)
	};
	return Profiles;
}

const TArray<Fdemo_mapRewardBudgetProfile>&
Fdemo_mapItemDefinitions::GetGeneratedRewardBudgetProfiles()
{
	static const TArray<Fdemo_mapRewardBudgetProfile> Profiles = {
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::EnemyStandard, 10, 1200),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::EnemyElite, 3, 4500),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::ContainerHighValue, 15, 2000),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::Boss, 1, 12000),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::ContainerBasic, 120, 400)};
	return Profiles;
}

const TArray<Fdemo_mapRewardBudgetProfile>&
Fdemo_mapItemDefinitions::GetGeneratedRewardM01BudgetProfiles()
{
	static const TArray<Fdemo_mapRewardBudgetProfile> Profiles = {
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::M01EnemyLow, 4, 900),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::M01EnemyMid, 6, 1400),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::M01ResourceTier1, 48, 250),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::M01ResourceTier2, 48, 400),
		GeneratedBudget(Fdemo_mapRewardBudgetProfileIds::M01ResourceTier3, 24, 700)};
	return Profiles;
}

const Fdemo_mapRewardBudgetProfile*
Fdemo_mapItemDefinitions::FindGeneratedRewardBudgetProfile(FName ProfileId)
{
	const Fdemo_mapRewardBudgetProfile* Core = GetGeneratedRewardBudgetProfiles().FindByPredicate([ProfileId](const auto& Profile) { return Profile.ProfileId == ProfileId; });
	return Core ? Core : GetGeneratedRewardM01BudgetProfiles().FindByPredicate([ProfileId](const auto& Profile) { return Profile.ProfileId == ProfileId; });
}

const TArray<Fdemo_mapRewardPoolEntry>&
Fdemo_mapItemDefinitions::GetGeneratedRewardPool()
{
	static const TArray<Fdemo_mapRewardPoolEntry> Entries = {
		GeneratedPool(TEXT("P1.Pool.Weapon.L1"), Fdemo_mapItemIds::WeaponLevel1, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 36, 1), GeneratedPool(TEXT("P1.Pool.Weapon.L2"), Fdemo_mapItemIds::WeaponLevel2, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 24, 1), GeneratedPool(TEXT("P1.Pool.Weapon.L3"), Fdemo_mapItemIds::WeaponLevel3, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 12, 1), GeneratedPool(TEXT("P1.Pool.Weapon.L4"), Fdemo_mapItemIds::WeaponLevel4, Fdemo_mapRewardTagIds::ItemEquipmentWeapon, 5, 1),
		GeneratedPool(TEXT("P1.Pool.Robe.L1"), Fdemo_mapItemIds::ArmorRobeLevel1, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 36, 1), GeneratedPool(TEXT("P1.Pool.Robe.L2"), Fdemo_mapItemIds::ArmorRobeLevel2, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 24, 1), GeneratedPool(TEXT("P1.Pool.Robe.L3"), Fdemo_mapItemIds::ArmorRobeLevel3, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 12, 1), GeneratedPool(TEXT("P1.Pool.Robe.L4"), Fdemo_mapItemIds::ArmorRobeLevel4, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 5, 1),
		GeneratedPool(TEXT("P5.4.Pool.Robe.SpiritGuard"), Fdemo_mapItemIds::SpiritGuardRobe, Fdemo_mapRewardTagIds::ItemEquipmentRobe, 8, 1),
		GeneratedPool(TEXT("P1.Pool.Accessory.L1"), Fdemo_mapItemIds::AccessoryLevel1, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 34, 1), GeneratedPool(TEXT("P1.Pool.Accessory.L2"), Fdemo_mapItemIds::AccessoryLevel2, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 22, 1), GeneratedPool(TEXT("P1.Pool.Accessory.L3"), Fdemo_mapItemIds::AccessoryLevel3, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 11, 1), GeneratedPool(TEXT("P1.Pool.Accessory.L4"), Fdemo_mapItemIds::AccessoryLevel4, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 4, 1),
		GeneratedPool(TEXT("P17.0.Pool.Accessory.HeartMirror"), Fdemo_mapItemIds::HeartProtectingMirror, Fdemo_mapRewardTagIds::ItemEquipmentAccessory, 6, 1),
		GeneratedPool(TEXT("P1.Pool.Backpack.L1"), Fdemo_mapItemIds::BackpackLevel1, Fdemo_mapRewardTagIds::ItemEquipmentBackpack, 24, 1), GeneratedPool(TEXT("P1.Pool.Backpack.L2"), Fdemo_mapItemIds::BackpackLevel2, Fdemo_mapRewardTagIds::ItemEquipmentBackpack, 14, 1),
		GeneratedPool(TEXT("P1.Pool.Pill.L1"), Fdemo_mapItemIds::HealingPillLevel1, Fdemo_mapRewardTagIds::ItemConsumablePill, 48, 8), GeneratedPool(TEXT("P1.Pool.Pill.L2"), Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapRewardTagIds::ItemConsumablePill, 32, 6), GeneratedPool(TEXT("P1.Pool.Pill.L3"), Fdemo_mapItemIds::HealingPillLevel3, Fdemo_mapRewardTagIds::ItemConsumablePill, 18, 4),
		GeneratedPool(TEXT("P1.Pool.Wood.L1"), Fdemo_mapItemIds::SpiritWoodLevel1, Fdemo_mapRewardTagIds::ItemMaterialWood, 52, 12), GeneratedPool(TEXT("P1.Pool.Wood.L2"), Fdemo_mapItemIds::SpiritWoodLevel2, Fdemo_mapRewardTagIds::ItemMaterialWood, 34, 10), GeneratedPool(TEXT("P1.Pool.Wood.L3"), Fdemo_mapItemIds::SpiritWoodLevel3, Fdemo_mapRewardTagIds::ItemMaterialWood, 18, 8),
		GeneratedPool(TEXT("P1.Pool.Ore.L1"), Fdemo_mapItemIds::SpiritOreLevel1, Fdemo_mapRewardTagIds::ItemMaterialOre, 50, 12), GeneratedPool(TEXT("P1.Pool.Ore.L2"), Fdemo_mapItemIds::SpiritOreLevel2, Fdemo_mapRewardTagIds::ItemMaterialOre, 32, 10), GeneratedPool(TEXT("P1.Pool.Ore.L3"), Fdemo_mapItemIds::SpiritOreLevel3, Fdemo_mapRewardTagIds::ItemMaterialOre, 16, 8),
		GeneratedPool(TEXT("P1.Pool.Bone.Soul"), Fdemo_mapItemIds::SoulBone, Fdemo_mapRewardTagIds::ItemBodyBone, 20, 4), GeneratedPool(TEXT("P1.Pool.Bone.Spirit"), Fdemo_mapItemIds::SpiritBone, Fdemo_mapRewardTagIds::ItemBodyBone, 10, 3), GeneratedPool(TEXT("P1.Pool.Bone.Dao"), Fdemo_mapItemIds::DaoBone, Fdemo_mapRewardTagIds::ItemBodyBone, 4, 2),
		GeneratedPool(TEXT("P1.Pool.Core.L5"), Fdemo_mapItemIds::InnerCoreLevel5, Fdemo_mapRewardTagIds::ItemBodyInnerCore, 16, 4), GeneratedPool(TEXT("P1.Pool.Core.L10"), Fdemo_mapItemIds::InnerCoreLevel10, Fdemo_mapRewardTagIds::ItemBodyInnerCore, 8, 2), GeneratedPool(TEXT("P1.Pool.Core.L15"), Fdemo_mapItemIds::InnerCoreLevel15, Fdemo_mapRewardTagIds::ItemBodyInnerCore, 3, 1)};
	return Entries;
}

const TArray<Fdemo_mapRewardSourceProjection>&
Fdemo_mapItemDefinitions::GetGeneratedRewardProjectionProfiles()
{
	static const TArray<Fdemo_mapRewardSourceProjection> Profiles = {
		GeneratedChestProjection(Fdemo_mapRewardProjectionIds::ChestMainWood, Fdemo_mapRewardSourceIds::ChestMainA, Fdemo_mapFixedLootTableIds::MarkerChestMainA, Fdemo_mapRewardBudgetProfileIds::ContainerBasic, Fdemo_mapRewardProjectionTagIds::SourceContainerWood, Fdemo_mapFixedLootTableIds::ChestMainA),
		GeneratedChestProjection(Fdemo_mapRewardProjectionIds::ChestMainOre, Fdemo_mapRewardSourceIds::ChestMainB, Fdemo_mapFixedLootTableIds::MarkerChestMainB, Fdemo_mapRewardBudgetProfileIds::ContainerBasic, Fdemo_mapRewardProjectionTagIds::SourceContainerOre, Fdemo_mapFixedLootTableIds::ChestMainB),
		GeneratedChestProjection(Fdemo_mapRewardProjectionIds::ChestSideHighValue, Fdemo_mapRewardSourceIds::ChestSideHighValue, Fdemo_mapFixedLootTableIds::MarkerChestSideA, Fdemo_mapRewardBudgetProfileIds::ContainerHighValue, Fdemo_mapRewardTagIds::SourceContainerHighValue, Fdemo_mapFixedLootTableIds::ChestSideA),
		GeneratedCorpseProjection(Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard, Fdemo_mapEnemyEncounterIds::MainMeleeStandard, Fdemo_mapRewardBudgetProfileIds::EnemyStandard, Fdemo_mapFixedLootTableIds::CorpseMainMeleeStandard),
		GeneratedCorpseProjection(Fdemo_mapRewardProjectionIds::CorpseMainMeleeHeavy, Fdemo_mapEnemyEncounterIds::MainMeleeHeavy, Fdemo_mapRewardBudgetProfileIds::EnemyStandard, Fdemo_mapFixedLootTableIds::CorpseMainMeleeHeavy),
		GeneratedCorpseProjection(Fdemo_mapRewardProjectionIds::CorpseMainRangedStandard, Fdemo_mapEnemyEncounterIds::MainRangedStandard, Fdemo_mapRewardBudgetProfileIds::EnemyStandard, Fdemo_mapFixedLootTableIds::CorpseMainRangedStandard),
		GeneratedCorpseProjection(Fdemo_mapRewardProjectionIds::CorpseSideMeleeEnhanced, Fdemo_mapEnemyEncounterIds::SideMeleeEnhanced, Fdemo_mapRewardBudgetProfileIds::EnemyElite, Fdemo_mapFixedLootTableIds::CorpseSideMeleeEnhanced),
		GeneratedCorpseProjection(Fdemo_mapRewardProjectionIds::CorpseSideRangedEnhanced, Fdemo_mapEnemyEncounterIds::SideRangedEnhanced, Fdemo_mapRewardBudgetProfileIds::EnemyElite, Fdemo_mapFixedLootTableIds::CorpseSideRangedEnhanced),
		GeneratedBossProjection()
	};
	return Profiles;
}

const Fdemo_mapRewardSourceProjection*
Fdemo_mapItemDefinitions::FindGeneratedRewardProjectionProfile(
	FName ProjectionId)
{
	return GetGeneratedRewardProjectionProfiles().FindByPredicate(
		[ProjectionId](const Fdemo_mapRewardSourceProjection& Profile)
		{
			return Profile.ProjectionId == ProjectionId;
		});
}

const TArray<Fdemo_mapRewardDistributionProfile>&
Fdemo_mapItemDefinitions::GetGeneratedRewardDistributionProfiles()
{
	using Tags = Fdemo_mapRewardProjectionTagIds;
	static const TArray<Fdemo_mapRewardDistributionProfile> Profiles = {
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Low.Melee"), Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard, Fdemo_mapRewardBudgetProfileIds::M01EnemyLow, { Tags::SourceCorpse, Tags::RiskLow, Tags::Tier1, Tags::Generated }, 900),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Low.Ranged"), Fdemo_mapRewardProjectionIds::CorpseMainRangedStandard, Fdemo_mapRewardBudgetProfileIds::M01EnemyLow, { Tags::SourceCorpse, Tags::RiskLow, Tags::Tier1, Tags::Generated }, 900),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Mid.Melee"), Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard, Fdemo_mapRewardBudgetProfileIds::M01EnemyMid, { Tags::SourceCorpse, Tags::RiskMid, Tags::Tier2, Tags::Generated }, 1400),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Mid.Ranged"), Fdemo_mapRewardProjectionIds::CorpseMainRangedStandard, Fdemo_mapRewardBudgetProfileIds::M01EnemyMid, { Tags::SourceCorpse, Tags::RiskMid, Tags::Tier2, Tags::Generated }, 1400),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Mid.Bruiser"), Fdemo_mapRewardProjectionIds::CorpseMainMeleeHeavy, Fdemo_mapRewardBudgetProfileIds::M01EnemyMid, { Tags::SourceCorpse, Tags::RiskMid, Tags::Tier2, Tags::Generated }, 1400),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Elite"), Fdemo_mapRewardProjectionIds::CorpseSideMeleeEnhanced, Fdemo_mapRewardBudgetProfileIds::EnemyElite, { Tags::SourceCorpse, Tags::RiskHigh, Tags::Tier3, Tags::Generated }, 4500),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Enemy.Boss"), Fdemo_mapRewardProjectionIds::CorpseBossPrototype, Fdemo_mapRewardBudgetProfileIds::Boss, { Tags::SourceBoss, Tags::SourceCorpse, Tags::ValueHigh, Tags::Generated }, 12000),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Resource.Wood.Tier1"), Fdemo_mapRewardProjectionIds::ChestMainWood, Fdemo_mapRewardBudgetProfileIds::M01ResourceTier1, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerWood, Tags::Tier1, Tags::RiskLow, Tags::Generated }, 250),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Resource.Ore.Tier1"), Fdemo_mapRewardProjectionIds::ChestMainOre, Fdemo_mapRewardBudgetProfileIds::M01ResourceTier1, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerOre, Tags::Tier1, Tags::RiskLow, Tags::Generated }, 250),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Resource.Wood.Tier2"), Fdemo_mapRewardProjectionIds::ChestMainWood, Fdemo_mapRewardBudgetProfileIds::M01ResourceTier2, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerWood, Tags::Tier2, Tags::RiskMid, Tags::Generated }, 400),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Resource.Ore.Tier2"), Fdemo_mapRewardProjectionIds::ChestMainOre, Fdemo_mapRewardBudgetProfileIds::M01ResourceTier2, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerOre, Tags::Tier2, Tags::RiskMid, Tags::Generated }, 400),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Resource.Wood.Tier3"), Fdemo_mapRewardProjectionIds::ChestMainWood, Fdemo_mapRewardBudgetProfileIds::M01ResourceTier3, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerWood, Tags::Tier3, Tags::RiskHigh, Tags::Generated }, 700),
		GeneratedDistributionProfile(TEXT("M01.Distribution.Resource.Ore.Tier3"), Fdemo_mapRewardProjectionIds::ChestMainOre, Fdemo_mapRewardBudgetProfileIds::M01ResourceTier3, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerOre, Tags::Tier3, Tags::RiskHigh, Tags::Generated }, 700),
		GeneratedDistributionProfile(TEXT("M01.Distribution.HighValue"), Fdemo_mapRewardProjectionIds::ChestSideHighValue, Fdemo_mapRewardBudgetProfileIds::ContainerHighValue, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::ValueHigh, Tags::Tier3, Tags::RiskHigh, Tags::Generated }, 2000),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Enemy.Standard.Melee"), Fdemo_mapRewardProjectionIds::CorpseMainMeleeStandard, Fdemo_mapRewardBudgetProfileIds::EnemyStandard, { Tags::SourceCorpse }, 1200),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Enemy.Standard.Ranged"), Fdemo_mapRewardProjectionIds::CorpseMainRangedStandard, Fdemo_mapRewardBudgetProfileIds::EnemyStandard, { Tags::SourceCorpse }, 1200),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Enemy.Elite"), Fdemo_mapRewardProjectionIds::CorpseSideMeleeEnhanced, Fdemo_mapRewardBudgetProfileIds::EnemyElite, { Tags::SourceCorpse }, 4500),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Enemy.Boss"), Fdemo_mapRewardProjectionIds::CorpseBossPrototype, Fdemo_mapRewardBudgetProfileIds::Boss, { Tags::SourceBoss, Tags::SourceCorpse, Tags::ValueHigh, Tags::Generated }, 12000),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Container.Wood"), Fdemo_mapRewardProjectionIds::ChestMainWood, Fdemo_mapRewardBudgetProfileIds::ContainerBasic, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerWood }, 400),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Container.Ore"), Fdemo_mapRewardProjectionIds::ChestMainOre, Fdemo_mapRewardBudgetProfileIds::ContainerBasic, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::SourceContainerOre }, 400),
		GeneratedDistributionProfile(TEXT("P8.Distribution.Container.HighValue"), Fdemo_mapRewardProjectionIds::ChestSideHighValue, Fdemo_mapRewardBudgetProfileIds::ContainerHighValue, { Fdemo_mapRewardTagIds::SourceContainerGeneral, Tags::ValueHigh }, 2000)
	};
	return Profiles;
}

const Fdemo_mapRewardDistributionProfile*
Fdemo_mapItemDefinitions::FindGeneratedRewardDistributionProfile(FName ProfileId)
{
	return GetGeneratedRewardDistributionProfiles().FindByPredicate(
		[ProfileId](const Fdemo_mapRewardDistributionProfile& Profile)
		{
			return Profile.ProfileId == ProfileId;
		});
}

const Fdemo_mapFixedLootTableDefinition*
Fdemo_mapItemDefinitions::FindFixedLootProfile(FName ProfileId)
{
	return GetFixedLootProfiles().FindByPredicate(
		[ProfileId](const Fdemo_mapFixedLootTableDefinition& Profile)
		{
			return Profile.TableId == ProfileId;
		});
}

FName Fdemo_mapItemDefinitions::GetEnemyLootProfileId(
	Edemo_mapEnemyLootArchetype Archetype)
{
	switch (Archetype)
	{
	case Edemo_mapEnemyLootArchetype::Melee: return Fdemo_mapLootTableIds::EnemyMelee;
	case Edemo_mapEnemyLootArchetype::Ranged: return Fdemo_mapLootTableIds::EnemyRanged;
	case Edemo_mapEnemyLootArchetype::Heavy: return Fdemo_mapLootTableIds::EnemyHeavy;
	default: return NAME_None;
	}
}

const TArray<Fdemo_mapLootTableEntry>*
Fdemo_mapItemDefinitions::FindEnemyLootProfile(FName ProfileId)
{
	static const TArray<Fdemo_mapLootTableEntry> Melee = {
		{ Fdemo_mapItemIds::SpiritDust, 2 }};
	static const TArray<Fdemo_mapLootTableEntry> Ranged = {
		{ Fdemo_mapItemIds::IronShard, 2 }};
	static const TArray<Fdemo_mapLootTableEntry> Heavy = {
		{ Fdemo_mapItemIds::AncientToken, 1 }};
	if (ProfileId == Fdemo_mapLootTableIds::EnemyMelee) return &Melee;
	if (ProfileId == Fdemo_mapLootTableIds::EnemyRanged) return &Ranged;
	if (ProfileId == Fdemo_mapLootTableIds::EnemyHeavy) return &Heavy;
	return nullptr;
}

Fdemo_mapSpatialStorageCapacityResult
Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
	FName BackpackDefinitionId)
{
	Fdemo_mapSpatialStorageCapacityResult Result;
	Result.SpatialItemDefinitionId = BackpackDefinitionId;
	if (BackpackDefinitionId.IsNone())
	{
		Result.bSuccess = true;
		return Result;
	}

	const Fdemo_mapItemDefinition* Definition = Find(BackpackDefinitionId);
	if (!Definition)
	{
		Result.Diagnostic = TEXT("Equipped space-item Definition is unknown.");
		return Result;
	}
	if (Definition->CategoryId != Fdemo_mapItemIds::BackpackCategory
		|| Definition->EquipmentSlotId != Fdemo_mapItemIds::BackpackSlot
		|| Definition->CompatibleSlotIds != TArray<FName>({ Fdemo_mapItemIds::BackpackSlot })
		|| Definition->MaxStackSize != 1)
	{
		Result.Diagnostic = TEXT("Space-item capacity metadata is inconsistent with its category or slot.");
		return Result;
	}

	const Fdemo_mapItemEffectParameter* CapacityEffect = nullptr;
	for (const Fdemo_mapItemEffectParameter& Effect : Definition->EffectParameters)
	{
		if (Effect.ParameterId != Fdemo_mapItemEffectIds::TotalCapacity)
		{
			continue;
		}
		if (CapacityEffect)
		{
			Result.Diagnostic = TEXT("Space-item Definition contains duplicate TotalCapacity metadata.");
			return Result;
		}
		CapacityEffect = &Effect;
	}
	if (!CapacityEffect
		|| !FMath::IsFinite(CapacityEffect->Value)
		|| CapacityEffect->Value < 6.0
		|| CapacityEffect->Value > 36.0
		|| CapacityEffect->Value > static_cast<double>(MAX_int32)
		|| FMath::TruncToDouble(CapacityEffect->Value) != CapacityEffect->Value)
	{
		Result.Diagnostic = TEXT("Space-bag capacity must be one finite integer from 6 through 36.");
		return Result;
	}

	Result.bSuccess = true;
	Result.Capacity = static_cast<int32>(CapacityEffect->Value);
	return Result;
}

Fdemo_mapInventoryCapacityResult
Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
	FName BackpackDefinitionId,
	FName SpatialRingDefinitionId)
{
	Fdemo_mapInventoryCapacityResult Result;
	Result.BackpackDefinitionId = BackpackDefinitionId;
	Result.SpatialRingDefinitionId = SpatialRingDefinitionId;
	const Fdemo_mapSpatialStorageCapacityResult Spatial =
		ResolveSpatialStorageCapacity(BackpackDefinitionId);
	const Fdemo_mapSpatialRingCapacityResult Ring =
		ResolveSpatialRingCapacity(SpatialRingDefinitionId);
	if (!Spatial.bSuccess)
	{
		Result.Diagnostic = Spatial.Diagnostic;
		return Result;
	}
	if (!Ring.bSuccess)
	{
		Result.Diagnostic = Ring.Diagnostic;
		return Result;
	}

	Result.bSuccess = true;
	Result.bFits = true;
	Result.RingQuickCapacity = Ring.Capacity;
	Result.SpatialBagCapacity = Spatial.Capacity;
	Result.Capacity =
		Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack
		+ Ring.Capacity
		+ Spatial.Capacity;
	return Result;
}

Fdemo_mapSpatialRingCapacityResult
Fdemo_mapItemDefinitions::ResolveSpatialRingCapacity(
	FName SpatialRingDefinitionId)
{
	Fdemo_mapSpatialRingCapacityResult Result;
	Result.SpatialRingDefinitionId = SpatialRingDefinitionId;
	if (SpatialRingDefinitionId.IsNone())
	{
		Result.bSuccess = true;
		return Result;
	}

	const Fdemo_mapItemDefinition* Definition = Find(SpatialRingDefinitionId);
	if (!Definition)
	{
		Result.Diagnostic = TEXT("Equipped spatial-ring Definition is unknown.");
		return Result;
	}
	if (Definition->CategoryId != Fdemo_mapItemIds::SpatialRingCategory
		|| Definition->EquipmentSlotId != Fdemo_mapItemIds::SpatialRingSlot
		|| !Definition->CompatibleSlotIds.Contains(Fdemo_mapItemIds::SpatialRingSlot)
		|| Definition->MaxStackSize != 1)
	{
		Result.Diagnostic = TEXT("Spatial-ring metadata is inconsistent with its equipment slot.");
		return Result;
	}

	const Fdemo_mapItemEffectParameter* CapacityEffect = nullptr;
	for (const Fdemo_mapItemEffectParameter& Effect : Definition->EffectParameters)
	{
		if (Effect.ParameterId != Fdemo_mapItemEffectIds::RingQuickCapacity)
		{
			continue;
		}
		if (CapacityEffect)
		{
			Result.Diagnostic = TEXT("Spatial-ring Definition contains duplicate RingQuickCapacity metadata.");
			return Result;
		}
		CapacityEffect = &Effect;
	}
	// Conventional accessories retain their old behavior and simply grant zero
	// ring slots.  A dedicated spatial ring must declare a valid 4—12 count.
	if (!CapacityEffect)
	{
		Result.bSuccess = true;
		return Result;
	}
	if (!FMath::IsFinite(CapacityEffect->Value)
		|| CapacityEffect->Value < 4.0
		|| CapacityEffect->Value > 12.0
		|| FMath::TruncToDouble(CapacityEffect->Value) != CapacityEffect->Value)
	{
		Result.Diagnostic = TEXT("Spatial-ring capacity must be one finite integer from 4 through 12.");
		return Result;
	}
	Result.bSuccess = true;
	Result.Capacity = static_cast<int32>(CapacityEffect->Value);
	return Result;
}

bool Fdemo_mapItemDefinitions::Validate(FString* OutError)
{
	if (GetAll().Num() != 43 || GetEquipmentSlotIds().Num() != 5)
	{
		if (OutError) *OutError = TEXT("The current registry must contain 43 definitions and expose five active runtime slots.");
		return false;
	}
	TSet<FName> DefinitionIds;
	TSet<FString> WorldLabels;
	TSet<FName> SlotIds;
	int32 ThrownWeaponDefinitionCount = 0;
	int32 WeaponGuardDefinitionCount = 0;
	int32 SwordQiSourceDefinitionCount = 0;
	int32 FlyingSwordDefinitionCount = 0;
	int32 MeridianShockTreatmentDefinitionCount = 0;
	int32 LethalInterceptionDefinitionCount = 0;
	for (FName SlotId : GetEquipmentSlotIds())
	{
		if (SlotId.IsNone() || SlotIds.Contains(SlotId))
		{
			if (OutError) *OutError = TEXT("Invalid or duplicate equipment slot ID.");
			return false;
		}
		SlotIds.Add(SlotId);
	}
	for (const Fdemo_mapItemDefinition& Definition : GetAll())
	{
		TSet<Edemo_mapItemGameplaySemantic> UniqueSemantics;
		for (Edemo_mapItemGameplaySemantic Semantic : Definition.GameplaySemantics)
		{
			if (Semantic == Edemo_mapItemGameplaySemantic::None
				|| UniqueSemantics.Contains(Semantic))
			{
				if (OutError) *OutError = FString::Printf(TEXT("Invalid or duplicate gameplay semantic on definition: %s"), *Definition.DefinitionId.ToString());
				return false;
			}
			UniqueSemantics.Add(Semantic);
		}
		const bool bThrownWeapon = Definition.HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::ThrownWeapon);
		const bool bWeaponGuard = Definition.HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::WeaponGuard);
		const bool bSwordQiSource = Definition.HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::SwordQiSource);
		const bool bMeridianShockTreatment = Definition.HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::MeridianShockTreatment);
		const bool bLethalInterception = Definition.HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::LethalInterception);
		const bool bFlyingSword = Definition.HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::FlyingSword);
		if (bThrownWeapon)
		{
			++ThrownWeaponDefinitionCount;
		}
		if (bWeaponGuard)
		{
			++WeaponGuardDefinitionCount;
		}
		if (bSwordQiSource)
		{
			++SwordQiSourceDefinitionCount;
		}
		if (bMeridianShockTreatment)
		{
			++MeridianShockTreatmentDefinitionCount;
		}
		if (bLethalInterception)
		{
			++LethalInterceptionDefinitionCount;
		}
		if (bFlyingSword)
		{
			++FlyingSwordDefinitionCount;
		}
		if (Definition.DefinitionId.IsNone()
			|| DefinitionIds.Contains(Definition.DefinitionId)
			|| Definition.DisplayName.ToString().IsEmpty()
			|| Definition.WorldLabelName.IsEmpty()
			|| WorldLabels.Contains(Definition.WorldLabelName)
			|| Definition.CategoryId.IsNone()
			|| Definition.Level < 0
			|| Definition.MaxStackSize <= 0
			|| Definition.MaxDurability < 0
			|| Definition.MaxCharges < 0
			|| (Definition.MaxStackSize > 1
				&& (Definition.MaxDurability > 0 || Definition.MaxCharges > 0))
			|| Definition.GridWidth != 1
			|| Definition.GridHeight != 1
			|| Definition.BuyPrice < 0
			|| Definition.SellPrice < 0
			|| (Definition.bPurchasable && Definition.BuyPrice <= 0)
			|| (!Definition.bPurchasable && Definition.BuyPrice != 0)
			|| (Definition.bSellable && Definition.SellPrice <= 0)
			|| (!Definition.bSellable && Definition.SellPrice != 0)
			|| Definition.WorldPresentationId.IsNone()
			|| !IsCurrentContentIdentity(
				Definition.ContentVersionId,
				Definition.ContentDigest)
			|| (Definition.bHotbarEligible
				!= (Definition.CategoryId == Fdemo_mapItemIds::ConsumableCategory))
			|| (bThrownWeapon
				&& (Definition.CategoryId != Fdemo_mapItemIds::ConsumableCategory
					|| Definition.MaxStackSize <= 1
					|| !Definition.bHotbarEligible
					|| !Definition.EquipmentSlotId.IsNone()
					|| !Definition.CompatibleSlotIds.IsEmpty()
					|| Definition.MaxDurability != 0
					|| Definition.MaxCharges != 0))
			|| (bWeaponGuard
				&& (Definition.CategoryId != Fdemo_mapItemIds::WeaponCategory
					|| Definition.MaxStackSize != 1
					|| Definition.EquipmentSlotId
						!= Fdemo_mapItemIds::WeaponSlot
					|| Definition.CompatibleSlotIds
						!= TArray<FName>({ Fdemo_mapItemIds::WeaponSlot })
					|| bThrownWeapon))
			|| (bSwordQiSource
				&& (Definition.CategoryId != Fdemo_mapItemIds::WeaponCategory
					|| Definition.MaxStackSize != 1
					|| Definition.EquipmentSlotId
						!= Fdemo_mapItemIds::WeaponSlot
					|| Definition.CompatibleSlotIds
						!= TArray<FName>({ Fdemo_mapItemIds::WeaponSlot })
					|| bThrownWeapon))
			|| (bFlyingSword
				&& (Definition.DefinitionId
						!= Fdemo_mapItemIds::TrainingFlyingSword
					|| Definition.CategoryId
						!= Fdemo_mapItemIds::WeaponCategory
					|| Definition.MaxStackSize != 1
					|| Definition.EquipmentSlotId
						!= Fdemo_mapItemIds::WeaponSlot
					|| Definition.CompatibleSlotIds
						!= TArray<FName>({ Fdemo_mapItemIds::WeaponSlot })
					|| Definition.bHotbarEligible
					|| Definition.MaxDurability != 0
					|| Definition.MaxCharges != 0
					|| bThrownWeapon
					|| bWeaponGuard
					|| bSwordQiSource))
			|| (bMeridianShockTreatment
				&& (Definition.DefinitionId
						!= Fdemo_mapItemIds::MeridianStabilizingPillLevel1
					|| Definition.CategoryId
						!= Fdemo_mapItemIds::ConsumableCategory
					|| Definition.MaxStackSize <= 1
					|| !Definition.bHotbarEligible
					|| !Definition.EquipmentSlotId.IsNone()
					|| !Definition.CompatibleSlotIds.IsEmpty()
					|| Definition.MaxDurability != 0
					|| Definition.MaxCharges != 0
					|| bThrownWeapon
					|| bWeaponGuard))
			|| (bLethalInterception
				&& (Definition.DefinitionId
						!= Fdemo_mapItemIds::HeartProtectingMirror
					|| Definition.CategoryId
						!= Fdemo_mapItemIds::AccessoryCategory
					|| Definition.MaxStackSize != 1
					|| Definition.MaxDurability != 0
					|| Definition.MaxCharges != 1
					|| Definition.EquipmentSlotId
						!= Fdemo_mapItemIds::AccessorySlot
					|| Definition.CompatibleSlotIds
						!= TArray<FName>({ Fdemo_mapItemIds::AccessorySlot })
					|| Definition.bHotbarEligible
					|| Definition.EffectParameters.Num() != 1
					|| Definition.EffectParameters[0].ParameterId
						!= Fdemo_mapItemEffectIds::LethalVitalityFloor
					|| !FMath::IsNearlyEqual(
						Definition.EffectParameters[0].Value, 1.0)
					|| bThrownWeapon
					|| bWeaponGuard
					|| bMeridianShockTreatment))
			|| (!Definition.EquipmentSlotId.IsNone()
				&& (Definition.MaxStackSize != 1
					|| Definition.CompatibleSlotIds.IsEmpty()
					|| !SlotIds.Contains(Definition.EquipmentSlotId)))
			|| (Definition.EquipmentSlotId.IsNone()
				&& !Definition.CompatibleSlotIds.IsEmpty()))
		{
			if (OutError) *OutError = FString::Printf(TEXT("Invalid or duplicate item definition: %s"), *Definition.DefinitionId.ToString());
			return false;
		}
		DefinitionIds.Add(Definition.DefinitionId);
		WorldLabels.Add(Definition.WorldLabelName);
		for (FName CompatibleSlot : Definition.CompatibleSlotIds)
		{
			if (!SlotIds.Contains(CompatibleSlot))
			{
				if (OutError) *OutError = FString::Printf(TEXT("Unknown compatible slot: %s"), *CompatibleSlot.ToString());
				return false;
			}
		}
		for (const Fdemo_mapModifierSpec& Modifier : Definition.Modifiers)
		{
			if (!Fdemo_mapAttributeDefinitions::Find(Modifier.AttributeId) || !FMath::IsFinite(Modifier.Value) || (Modifier.Operation == Edemo_mapModifierOperation::Multiply && Modifier.Value < 0.0f))
			{
				if (OutError) *OutError = FString::Printf(TEXT("Invalid modifier on definition: %s"), *Definition.DefinitionId.ToString());
				return false;
			}
		}
		TSet<FName> EffectIds;
		for (const Fdemo_mapItemEffectParameter& Effect : Definition.EffectParameters)
		{
			if (Effect.ParameterId.IsNone() || EffectIds.Contains(Effect.ParameterId) || !FMath::IsFinite(Effect.Value))
			{
				if (OutError) *OutError = FString::Printf(TEXT("Invalid effect metadata on definition: %s"), *Definition.DefinitionId.ToString());
				return false;
			}
			EffectIds.Add(Effect.ParameterId);
		}
	}
	if (ThrownWeaponDefinitionCount != 1)
	{
		if (OutError) *OutError = TEXT("P7.7 requires exactly one canonical thrown-weapon product definition.");
		return false;
	}
	if (WeaponGuardDefinitionCount != 6)
	{
		if (OutError) *OutError = TEXT("P11.7 requires exactly six explicit weapon-guard product definitions.");
		return false;
	}
	if (SwordQiSourceDefinitionCount != 6)
	{
		if (OutError) *OutError = TEXT("P18.4 requires exactly six explicit Sword Qi source definitions.");
		return false;
	}
	if (MeridianShockTreatmentDefinitionCount != 1)
	{
		if (OutError) *OutError = TEXT("P16.0 requires exactly one canonical Meridian Shock treatment definition.");
		return false;
	}
	if (LethalInterceptionDefinitionCount != 1)
	{
		if (OutError) *OutError = TEXT("P17.0 requires exactly one canonical lethal-interception artifact definition.");
		return false;
	}
	if (FlyingSwordDefinitionCount != 1)
	{
		if (OutError) *OutError = TEXT("P21.0 requires exactly one canonical flying-sword product definition.");
		return false;
	}
	TSet<FName> FixedProfileIds;
	int32 CorpseProfileCount = 0;
	int32 ChestProfileCount = 0;
	for (const Fdemo_mapFixedLootTableDefinition& Profile : GetFixedLootProfiles())
	{
		if (Profile.TableId.IsNone()
			|| FixedProfileIds.Contains(Profile.TableId)
			|| !Profile.IsValid(OutError))
		{
			if (OutError && OutError->IsEmpty())
			{
				*OutError = TEXT("P73 fixed loot profile is invalid or duplicated.");
			}
			return false;
		}
		FixedProfileIds.Add(Profile.TableId);
		if (Profile.Kind == Edemo_mapRuntimeContainerKind::Corpse) ++CorpseProfileCount;
		else ++ChestProfileCount;
	}
	if (FixedProfileIds.Num() != 8 || CorpseProfileCount != 5 || ChestProfileCount != 3)
	{
		if (OutError) *OutError = TEXT("P73 fixed loot profile catalog must contain five Corpse and three Chest profiles.");
		return false;
	}
	if (GetPurchasableDefinitionIds() != TArray<FName>({
		Fdemo_mapItemIds::WeaponLevel1,
		Fdemo_mapItemIds::WeaponLevel2,
		Fdemo_mapItemIds::WeaponLevel3,
		Fdemo_mapItemIds::WeaponLevel4,
		Fdemo_mapItemIds::ArmorRobeLevel1,
		Fdemo_mapItemIds::ArmorRobeLevel2,
		Fdemo_mapItemIds::ArmorRobeLevel3,
		Fdemo_mapItemIds::ArmorRobeLevel4,
		Fdemo_mapItemIds::AccessoryLevel1,
		Fdemo_mapItemIds::BackpackLevel1,
		Fdemo_mapItemIds::HealingPillLevel1,
		Fdemo_mapItemIds::HealingPillLevel2,
		Fdemo_mapItemIds::HealingPillLevel3,
		Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
		Fdemo_mapItemIds::TrainingThrowingKnife,
		Fdemo_mapItemIds::TrainingFlyingSword }))
	{
		if (OutError) *OutError = TEXT("Purchasable definition ordering drifted from the current content catalog.");
		return false;
	}
	const Fdemo_mapSpatialStorageCapacityResult EmptyStorage =
		ResolveSpatialStorageCapacity(NAME_None);
	const Fdemo_mapSpatialStorageCapacityResult Level1Storage =
		ResolveSpatialStorageCapacity(Fdemo_mapItemIds::BackpackLevel1);
	const Fdemo_mapSpatialStorageCapacityResult Level2Storage =
		ResolveSpatialStorageCapacity(Fdemo_mapItemIds::BackpackLevel2);
	const Fdemo_mapInventoryCapacityResult BaseCapacity =
		ResolveInventoryCapacity(NAME_None);
	const Fdemo_mapInventoryCapacityResult Level1Capacity =
		ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel1);
	const Fdemo_mapInventoryCapacityResult Level2Capacity =
		ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel2);
	const Fdemo_mapSpatialRingCapacityResult WindRing =
		ResolveSpatialRingCapacity(Fdemo_mapItemIds::WindTalisman);
	const Fdemo_mapInventoryCapacityResult HeavenBagAndRing =
		ResolveInventoryCapacity(
			Fdemo_mapItemIds::BackpackLevel1,
			Fdemo_mapItemIds::WindTalisman);
	if (!BaseCapacity.bSuccess
		|| !Level1Capacity.bSuccess
		|| !Level2Capacity.bSuccess
		|| !WindRing.bSuccess
		|| !HeavenBagAndRing.bSuccess
		|| !EmptyStorage.bSuccess
		|| !Level1Storage.bSuccess
		|| !Level2Storage.bSuccess
		|| BaseCapacity.Capacity != 6
		|| Level1Capacity.Capacity != 42
		|| Level2Capacity.Capacity != 42
		|| EmptyStorage.Capacity != 0
		|| Level1Storage.Capacity != 36
		|| Level2Storage.Capacity != 36
		|| WindRing.Capacity != 4
		|| HeavenBagAndRing.Capacity != 46
		|| HeavenBagAndRing.RingQuickCapacity != 4
		|| HeavenBagAndRing.SpatialBagCapacity != 36)
	{
		if (OutError) *OutError = TEXT("Nested capacity contract must resolve to base 6, ring 4, bag 36, and total 46.");
		return false;
	}
	TSet<FName> ProjectionIds;
	TSet<FName> ProjectionRoles;
	for (const Fdemo_mapRewardSourceProjection& Projection :
		GetGeneratedRewardProjectionProfiles())
	{
		if (!Projection.IsPolicyPrototypeValid()
			|| ProjectionIds.Contains(Projection.ProjectionId)
			|| ProjectionRoles.Contains(Projection.StableSourceRoleId)
			|| !FindGeneratedRewardBudgetProfile(
				Projection.BudgetProfileId)
			|| !FindFixedLootProfile(Projection.FixedFallbackTableId))
		{
			if (OutError)
			{
				*OutError = TEXT("P73.2 generated projection manifest is invalid or duplicated.");
			}
			return false;
		}
		ProjectionIds.Add(Projection.ProjectionId);
		ProjectionRoles.Add(Projection.StableSourceRoleId);
	}
	if (ProjectionIds.Num() != 9)
	{
		if (OutError)
		{
			*OutError = TEXT("P73.2 requires nine canonical generated projection profiles.");
		}
		return false;
	}
	TSet<FName> DistributionProfileIds;
	for (const Fdemo_mapRewardDistributionProfile& Profile :
		GetGeneratedRewardDistributionProfiles())
	{
		const Fdemo_mapRewardSourceProjection* Projection =
			FindGeneratedRewardProjectionProfile(Profile.ProjectionId);
		const Fdemo_mapRewardBudgetProfile* Budget =
			FindGeneratedRewardBudgetProfile(Profile.BudgetProfileId);
		if (!Profile.IsValid()
			|| DistributionProfileIds.Contains(Profile.ProfileId)
			|| !Projection
			|| !Budget
			|| Budget->BaseValue != Profile.BaseSourceValue
			|| !IsCurrentContentIdentity(
				Profile.ContentVersionId,
				Profile.ContentDigest))
		{
			if (OutError)
			{
				*OutError = TEXT("P73.3 generated distribution manifest is invalid, duplicated, or conflicts with its budget contract.");
			}
			return false;
		}
		DistributionProfileIds.Add(Profile.ProfileId);
	}
	if (DistributionProfileIds.Num() != 21)
	{
		if (OutError)
		{
			*OutError = TEXT("P73.3 requires twenty-one canonical M01/P8 distribution profiles.");
		}
		return false;
	}
	return true;
}

FName Fdemo_mapLootTables::GetTableId(Edemo_mapEnemyLootArchetype Archetype)
{
	return Fdemo_mapItemDefinitions::GetEnemyLootProfileId(Archetype);
}

const TArray<Fdemo_mapLootTableEntry>* Fdemo_mapLootTables::Find(FName TableId)
{
	return Fdemo_mapItemDefinitions::FindEnemyLootProfile(TableId);
}

bool Fdemo_mapLootTables::Validate(FString* OutError)
{
	TArray<TPair<FName, TPair<FName, int32>>> Expected;
	Expected.Emplace(Fdemo_mapLootTableIds::EnemyMelee, TPair<FName, int32>(Fdemo_mapItemIds::SpiritDust, 2));
	Expected.Emplace(Fdemo_mapLootTableIds::EnemyRanged, TPair<FName, int32>(Fdemo_mapItemIds::IronShard, 2));
	Expected.Emplace(Fdemo_mapLootTableIds::EnemyHeavy, TPair<FName, int32>(Fdemo_mapItemIds::AncientToken, 1));
	for (const auto& Pair : Expected)
	{
		const TArray<Fdemo_mapLootTableEntry>* Entries = Find(Pair.Key);
		if (!Entries || Entries->Num() != 1 || (*Entries)[0].DefinitionId != Pair.Value.Key || (*Entries)[0].Quantity != Pair.Value.Value || !Fdemo_mapItemDefinitions::Find((*Entries)[0].DefinitionId))
		{
			if (OutError) *OutError = FString::Printf(TEXT("Invalid fixed loot table: %s"), *Pair.Key.ToString());
			return false;
		}
	}
	return true;
}
