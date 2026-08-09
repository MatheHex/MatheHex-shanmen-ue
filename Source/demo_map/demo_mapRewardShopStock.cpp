#include "demo_mapRewardShopStock.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardEventTypes.h"

const FName Fdemo_mapRewardShopStock::DefaultPolicyId(
	TEXT("Reward.ShopStock.Default"));

namespace
{
	uint64 HashUtf8(const FString& Text)
	{
		const FTCHARToUTF8 Utf8(*Text);
		uint64 Hash = 1469598103934665603ull;
		for (int32 Index = 0; Index < Utf8.Length(); ++Index)
		{
			Hash ^= static_cast<uint8>(Utf8.Get()[Index]);
			Hash *= 1099511628211ull;
		}
		return Hash == 0 ? 1 : Hash;
	}

	FGuid StableGuid(const FString& Identity, const TCHAR* DomainA, const TCHAR* DomainB)
	{
		const uint64 A = HashUtf8(Identity + TEXT("|") + DomainA);
		const uint64 B = HashUtf8(Identity + TEXT("|") + DomainB);
		FGuid Result(
			static_cast<uint32>(A >> 32),
			static_cast<uint32>(A),
			static_cast<uint32>(B >> 32),
			static_cast<uint32>(B));
		if (!Result.IsValid()) Result.D = 1;
		return Result;
	}

	FString BaseIdentity(
		const FGuid& ProfileId,
		FName PolicyId,
		int32 Generation,
		const Fdemo_mapRewardShopStockSlotPolicy* Slot,
		const TCHAR* Domain)
	{
		return FString::Printf(
			TEXT("%s|%s|%d|%s|%d|%s"),
			*ProfileId.ToString(EGuidFormats::Digits),
			*PolicyId.ToString(),
			Generation,
			Slot ? *Slot->SlotId.ToString() : TEXT("Event"),
			Slot ? Slot->SlotOrdinal : INDEX_NONE,
			Domain);
	}

	FName CategoryId(Edemo_mapShopStockCategory Category)
	{
		switch (Category)
		{
		case Edemo_mapShopStockCategory::Weapon:
			return Fdemo_mapItemIds::WeaponCategory;
		case Edemo_mapShopStockCategory::Robe:
			return Fdemo_mapItemIds::ArmorCategory;
		default:
			return Fdemo_mapItemIds::ConsumableCategory;
		}
	}

	bool IsDefinitionInSlot(
		const Fdemo_mapItemDefinition& Definition,
		const Fdemo_mapRewardShopStockSlotPolicy& Slot)
	{
		if (Definition.CategoryId != CategoryId(Slot.Category)
			|| !Definition.bPurchasable
			|| Definition.BuyPrice <= 0
			|| Definition.SellPrice <= 0)
		{
			return false;
		}
		if (Slot.Category == Edemo_mapShopStockCategory::HealingPill
			&& Definition.DefinitionId != Fdemo_mapItemIds::HealingPillLevel1
			&& Definition.DefinitionId != Fdemo_mapItemIds::HealingPillLevel2
			&& Definition.DefinitionId != Fdemo_mapItemIds::HealingPillLevel3)
		{
			return false;
		}
		return Slot.LevelWeights.ContainsByPredicate(
			[&Definition](const Fdemo_mapRewardShopStockLevelWeight& Weight)
			{
				return Weight.Level == Definition.Level;
			});
	}

	bool ChooseDefinition(
		const FGuid& ProfileId,
		int32 Generation,
		const Fdemo_mapRewardShopStockSlotPolicy& Slot,
		const TSet<FName>& Excluded,
		FName& OutDefinitionId)
	{
		struct FCandidate
		{
			FName DefinitionId;
			int32 Weight = 0;
		};
		TArray<FCandidate> Candidates;
		for (const Fdemo_mapItemDefinition& Definition :
			Fdemo_mapItemDefinitions::GetAll())
		{
			if (!Excluded.Contains(Definition.DefinitionId)
				&& IsDefinitionInSlot(Definition, Slot))
			{
				const Fdemo_mapRewardShopStockLevelWeight* Weight =
					Slot.LevelWeights.FindByPredicate(
						[&Definition](const auto& Candidate)
						{
							return Candidate.Level == Definition.Level;
						});
				if (Weight && Weight->WeightBps > 0)
				{
					Candidates.Add({ Definition.DefinitionId, Weight->WeightBps });
				}
			}
		}
		Candidates.Sort([](const FCandidate& A, const FCandidate& B)
		{
			return A.DefinitionId.LexicalLess(B.DefinitionId);
		});
		int32 TotalWeight = 0;
		for (const FCandidate& Candidate : Candidates)
		{
			if (TotalWeight > MAX_int32 - Candidate.Weight) return false;
			TotalWeight += Candidate.Weight;
		}
		if (Candidates.IsEmpty() || TotalWeight <= 0) return false;
		const FString Identity = BaseIdentity(
			ProfileId,
			Fdemo_mapRewardShopStock::DefaultPolicyId,
			Generation,
			&Slot,
			TEXT("Shop.Stock.Level"));
		int32 Roll = static_cast<int32>(HashUtf8(Identity) % static_cast<uint64>(TotalWeight));
		for (const FCandidate& Candidate : Candidates)
		{
			if (Roll < Candidate.Weight)
			{
				OutDefinitionId = Candidate.DefinitionId;
				return true;
			}
			Roll -= Candidate.Weight;
		}
		return false;
	}

	bool IsDefaultItem(const Fdemo_mapPersistentItemRecord& Item)
	{
		return !Item.ItemInstanceId.IsValid()
			&& Item.ItemDefinitionId.IsNone()
			&& Item.StackCount == 0
			&& Item.EquipmentSlotId.IsNone()
			&& !Item.OriginRunId.IsValid()
			&& Item.RewardEventKind == Edemo_mapRewardEventKind::None
			&& !Item.RewardEventId.IsValid()
			&& Item.RewardValueMultiplierBps
				== Fdemo_mapRewardEventRules::NormalMultiplierBps
			&& Item.RewardSourceRoleId.IsNone()
			&& !Item.RareRewardEventId.IsValid()
			&& Item.RareRewardPolicyId.IsNone()
			&& Item.RareRewardTierId.IsNone()
			&& Item.RareRewardBonusValue == 0
			&& Item.AffixSet.IsEmpty();
	}
}

bool Fdemo_mapRewardShopStockSlotPolicy::IsValid() const
{
	if (SlotId.IsNone() || SlotOrdinal < 0 || LevelWeights.IsEmpty()
		|| AffixBudget < 0)
	{
		return false;
	}
	int32 Total = 0;
	TSet<int32> Levels;
	for (const Fdemo_mapRewardShopStockLevelWeight& Weight : LevelWeights)
	{
		if (Weight.Level <= 0 || Weight.WeightBps <= 0
			|| Levels.Contains(Weight.Level)
			|| Total > 10000 - Weight.WeightBps)
		{
			return false;
		}
		Levels.Add(Weight.Level);
		Total += Weight.WeightBps;
	}
	return Total == 10000
		&& (Category != Edemo_mapShopStockCategory::HealingPill
			? AffixBudget > 0
			: AffixBudget == 0);
}

bool Fdemo_mapRewardShopStockPolicy::IsValid() const
{
	if (PolicyId != Fdemo_mapRewardShopStock::DefaultPolicyId
		|| StockSize != 12
		|| InitialGeneration != 0
		|| !bFiniteStock
		|| QuantityPerSlot != 1
		|| EquipmentAffixPolicyId
			!= Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId
		|| !EquipmentPityPolicyId.IsNone()
		|| !JackpotPolicyId.IsNone()
		|| !RareExtremePolicyId.IsNone()
		|| Slots.Num() != StockSize)
	{
		return false;
	}
	TSet<FName> Ids;
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!Slots[Index].IsValid()
			|| Slots[Index].SlotOrdinal != Index
			|| Ids.Contains(Slots[Index].SlotId))
		{
			return false;
		}
		Ids.Add(Slots[Index].SlotId);
	}
	return true;
}

const Fdemo_mapRewardShopStockPolicy&
Fdemo_mapRewardShopStock::GetDefaultPolicy()
{
	static const Fdemo_mapRewardShopStockPolicy Policy = []
	{
		Fdemo_mapRewardShopStockPolicy Result;
		Result.PolicyId = DefaultPolicyId;
		Result.StockSize = 12;
		Result.InitialGeneration = 0;
		Result.bFiniteStock = true;
		Result.QuantityPerSlot = 1;
		Result.EquipmentAffixPolicyId =
			Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		Result.Slots = {
			{ TEXT("Shop.Stock.Weapon.01"), 0, Edemo_mapShopStockCategory::Weapon, {{1,7000},{2,3000}}, 360 },
			{ TEXT("Shop.Stock.Weapon.02"), 1, Edemo_mapShopStockCategory::Weapon, {{1,2500},{2,5000},{3,2500}}, 360 },
			{ TEXT("Shop.Stock.Weapon.03"), 2, Edemo_mapShopStockCategory::Weapon, {{2,2500},{3,5000},{4,2500}}, 360 },
			{ TEXT("Shop.Stock.Robe.01"), 3, Edemo_mapShopStockCategory::Robe, {{1,7000},{2,3000}}, 810 },
			{ TEXT("Shop.Stock.Robe.02"), 4, Edemo_mapShopStockCategory::Robe, {{1,2500},{2,5000},{3,2500}}, 810 },
			{ TEXT("Shop.Stock.Robe.03"), 5, Edemo_mapShopStockCategory::Robe, {{2,2500},{3,5000},{4,2500}}, 810 },
			{ TEXT("Shop.Stock.Pill.LowMid.01"), 6, Edemo_mapShopStockCategory::HealingPill, {{1,10000}}, 0 },
			{ TEXT("Shop.Stock.Pill.LowMid.02"), 7, Edemo_mapShopStockCategory::HealingPill, {{1,7000},{2,3000}}, 0 },
			{ TEXT("Shop.Stock.Pill.LowMid.03"), 8, Edemo_mapShopStockCategory::HealingPill, {{1,7000},{2,3000}}, 0 },
			{ TEXT("Shop.Stock.Pill.LowMid.04"), 9, Edemo_mapShopStockCategory::HealingPill, {{1,7000},{2,3000}}, 0 },
			{ TEXT("Shop.Stock.Pill.LowMid.05"), 10, Edemo_mapShopStockCategory::HealingPill, {{1,7000},{2,3000}}, 0 },
			{ TEXT("Shop.Stock.Pill.MidHigh.01"), 11, Edemo_mapShopStockCategory::HealingPill, {{2,7500},{3,2500}}, 0 }
		};
		return Result;
	}();
	return Policy;
}

FGuid Fdemo_mapRewardShopStock::MakeEventId(
	const FGuid& ProfileId,
	FName PolicyId,
	int32 Generation)
{
	if (!ProfileId.IsValid() || PolicyId.IsNone() || Generation < 0)
	{
		return FGuid();
	}
	const FString Identity = BaseIdentity(
		ProfileId,
		PolicyId,
		Generation,
		nullptr,
		TEXT("Shop.Stock.Event"));
	return StableGuid(
		Identity,
		TEXT("Shop.Stock.EventId.A"),
		TEXT("Shop.Stock.EventId.B"));
}

bool Fdemo_mapRewardShopStock::Generate(
	const FGuid& ProfileId,
	int32 Generation,
	const FGuid& LastAppliedTerminalId,
	Fdemo_mapPersistentShopStockState& OutState,
	FString* OutError)
{
	auto Fail = [OutError](const FString& Message)
	{
		if (OutError) *OutError = Message;
		return false;
	};
	const Fdemo_mapRewardShopStockPolicy& Policy = GetDefaultPolicy();
	if (!ProfileId.IsValid() || Generation < 0 || !Policy.IsValid())
	{
		return Fail(TEXT("ShopStock generation request or default policy is invalid."));
	}
	Fdemo_mapPersistentShopStockState Candidate;
	Candidate.bInitialized = true;
	Candidate.PolicyId = Policy.PolicyId;
	Candidate.Generation = Generation;
	Candidate.ShopStockEventId =
		MakeEventId(ProfileId, Policy.PolicyId, Generation);
	Candidate.LastAppliedTerminalId = LastAppliedTerminalId;
	TSet<FName> UsedWeapons;
	TSet<FName> UsedRobes;
	for (const Fdemo_mapRewardShopStockSlotPolicy& Slot : Policy.Slots)
	{
		const TSet<FName>& Excluded =
			Slot.Category == Edemo_mapShopStockCategory::Weapon
				? UsedWeapons
				: Slot.Category == Edemo_mapShopStockCategory::Robe
					? UsedRobes
					: TSet<FName>();
		FName DefinitionId;
		if (!ChooseDefinition(ProfileId, Generation, Slot, Excluded, DefinitionId))
		{
			return Fail(FString::Printf(
				TEXT("No deterministic legal definition remained for %s."),
				*Slot.SlotId.ToString()));
		}
		if (Slot.Category == Edemo_mapShopStockCategory::Weapon)
		{
			UsedWeapons.Add(DefinitionId);
		}
		else if (Slot.Category == Edemo_mapShopStockCategory::Robe)
		{
			UsedRobes.Add(DefinitionId);
		}

		Fdemo_mapPersistentShopStockEntry Entry;
		Entry.SlotId = Slot.SlotId;
		Entry.SlotOrdinal = Slot.SlotOrdinal;
		Entry.State = Edemo_mapPersistentShopStockEntryState::Available;
		Entry.Item.ItemDefinitionId = DefinitionId;
		Entry.Item.StackCount = 1;
		Entry.Item.PersistentDomain = Edemo_mapPersistentDomain::ShopStock;
		Entry.Item.ItemInstanceId = StableGuid(
			BaseIdentity(
				ProfileId,
				Policy.PolicyId,
				Generation,
				&Slot,
				TEXT("Shop.Stock.ItemEvent")),
			TEXT("Shop.Stock.ItemEvent.A"),
			TEXT("Shop.Stock.ItemEvent.B"));

		if (Slot.AffixBudget > 0)
		{
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(DefinitionId);
			Fdemo_mapRewardPlannedStack Stack;
			Stack.DefinitionId = DefinitionId;
			Stack.StackCount = 1;
			Stack.UnitValue = Definition ? Definition->SellPrice : 0;
			Stack.TotalValue = Stack.UnitValue;
			Stack.SlotIndex = Slot.SlotOrdinal;
			TArray<Fdemo_mapRewardPlannedStack> Stacks{ Stack };
			Fdemo_mapRewardAffixPlanTrace Trace;
			if (!Fdemo_mapRewardAffixPlanner::Apply(
					Fdemo_mapRewardAffixPolicyRegistry::GetDefault(),
					Candidate.ShopStockEventId,
					Slot.SlotId,
					Policy.PolicyId,
					Slot.AffixBudget,
					0,
					Stacks,
					Trace)
				|| Stacks.Num() != 1
				|| Stacks[0].AffixSet.Acquisition
					== Edemo_mapRewardAffixAcquisition::PityGuaranteed)
			{
				return Fail(FString::Printf(
					TEXT("Natural Shop Affix planning failed for %s."),
					*Slot.SlotId.ToString()));
			}
			Entry.Item.AffixSet = Stacks[0].AffixSet;
		}
		if (!TryComputeBuyValue(Entry.Item, Entry.QuotedBuyValue, OutError))
		{
			return false;
		}
		Candidate.Entries.Add(MoveTemp(Entry));
	}
	if (!ValidateState(ProfileId, Candidate, OutError)) return false;
	OutState = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapRewardShopStock::ValidateState(
	const FGuid& ProfileId,
	const Fdemo_mapPersistentShopStockState& State,
	FString* OutError)
{
	auto Fail = [OutError](const FString& Message)
	{
		if (OutError) *OutError = Message;
		return false;
	};
	if (!State.bInitialized)
	{
		return State.PolicyId.IsNone()
			&& State.Generation == INDEX_NONE
			&& !State.ShopStockEventId.IsValid()
			&& !State.LastAppliedTerminalId.IsValid()
			&& State.Entries.IsEmpty()
			? true
			: Fail(TEXT("Uninitialized ShopStock contains partial state."));
	}
	const Fdemo_mapRewardShopStockPolicy& Policy = GetDefaultPolicy();
	if (!ProfileId.IsValid() || !Policy.IsValid()
		|| State.PolicyId != Policy.PolicyId
		|| State.Generation < 0
		|| State.ShopStockEventId
			!= MakeEventId(ProfileId, State.PolicyId, State.Generation)
		|| State.Entries.Num() != Policy.StockSize)
	{
		return Fail(TEXT("ShopStock identity, generation, event, or entry count is invalid."));
	}
	TSet<FName> Slots;
	TSet<FGuid> ItemIds;
	TSet<FName> WeaponDefinitions;
	TSet<FName> RobeDefinitions;
	for (int32 Index = 0; Index < State.Entries.Num(); ++Index)
	{
		const Fdemo_mapPersistentShopStockEntry& Entry =
			State.Entries[Index];
		const Fdemo_mapRewardShopStockSlotPolicy& Slot =
			Policy.Slots[Index];
		if (Entry.SlotId != Slot.SlotId
			|| Entry.SlotOrdinal != Slot.SlotOrdinal
			|| Slots.Contains(Entry.SlotId))
		{
			return Fail(TEXT("ShopStock slot identity or ordering is invalid."));
		}
		Slots.Add(Entry.SlotId);
		if (Entry.State == Edemo_mapPersistentShopStockEntryState::Sold)
		{
			if (!Entry.SoldItemInstanceId.IsValid()
				|| ItemIds.Contains(Entry.SoldItemInstanceId)
				|| Entry.QuotedBuyValue != 0
				|| !IsDefaultItem(Entry.Item))
			{
				return Fail(TEXT("ShopStock SOLD tombstone is invalid."));
			}
			ItemIds.Add(Entry.SoldItemInstanceId);
			continue;
		}
		const Fdemo_mapPersistentItemRecord& Item = Entry.Item;
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
		if (!Item.ItemInstanceId.IsValid()
			|| ItemIds.Contains(Item.ItemInstanceId)
			|| Entry.SoldItemInstanceId.IsValid()
			|| !Definition
			|| !IsDefinitionInSlot(*Definition, Slot)
			|| Item.StackCount != 1
			|| Item.PersistentDomain != Edemo_mapPersistentDomain::ShopStock
			|| !Item.EquipmentSlotId.IsNone()
			|| Item.OriginRunId.IsValid()
			|| Item.RewardEventKind != Edemo_mapRewardEventKind::None
			|| Item.RewardEventId.IsValid()
			|| Item.RewardValueMultiplierBps
				!= Fdemo_mapRewardEventRules::NormalMultiplierBps
			|| !Item.RewardSourceRoleId.IsNone()
			|| Item.RareRewardEventId.IsValid()
			|| !Item.RareRewardPolicyId.IsNone()
			|| !Item.RareRewardTierId.IsNone()
			|| Item.RareRewardBonusValue != 0
			|| Item.AffixSet.Acquisition
				== Edemo_mapRewardAffixAcquisition::PityGuaranteed)
		{
			return Fail(TEXT("ShopStock available item metadata is invalid."));
		}
		FString AffixError;
		if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
				Item.ItemDefinitionId,
				Item.StackCount,
				Item.AffixSet,
				&AffixError))
		{
			return Fail(AffixError);
		}
		if (Slot.Category == Edemo_mapShopStockCategory::HealingPill
			&& !Item.AffixSet.IsEmpty())
		{
			return Fail(TEXT("Healing Pill ShopStock entries cannot carry Affixes."));
		}
		if (Slot.Category == Edemo_mapShopStockCategory::Weapon
			&& WeaponDefinitions.Contains(Item.ItemDefinitionId))
		{
			return Fail(TEXT("Weapon ShopStock definitions must be unique within a generation."));
		}
		if (Slot.Category == Edemo_mapShopStockCategory::Robe
			&& RobeDefinitions.Contains(Item.ItemDefinitionId))
		{
			return Fail(TEXT("Robe ShopStock definitions must be unique within a generation."));
		}
		if (Slot.Category == Edemo_mapShopStockCategory::Weapon)
			WeaponDefinitions.Add(Item.ItemDefinitionId);
		if (Slot.Category == Edemo_mapShopStockCategory::Robe)
			RobeDefinitions.Add(Item.ItemDefinitionId);
		ItemIds.Add(Item.ItemInstanceId);
		int64 Buy = 0;
		int64 Sell = 0;
		if (!TryComputeBuyValue(Item, Buy, OutError)
			|| !TryComputeSellValue(Item, Sell, OutError)
			|| Buy != Entry.QuotedBuyValue
			|| Buy < Sell)
		{
			return Fail(TEXT("ShopStock quote is invalid or allows buy/sell arbitrage."));
		}
	}
	const Fdemo_mapPersistentShopStockEntry& BasicPill = State.Entries[6];
	return BasicPill.State == Edemo_mapPersistentShopStockEntryState::Sold
		|| (BasicPill.Item.ItemDefinitionId
				== Fdemo_mapItemIds::HealingPillLevel1
			&& BasicPill.QuotedBuyValue == 30)
		? true
		: Fail(TEXT("Guaranteed Level1 Healing Pill slot drifted from BUY=30."));
}

bool Fdemo_mapRewardShopStock::TryComputeBuyValue(
	const Fdemo_mapPersistentItemRecord& Item,
	int64& OutValue,
	FString* OutError)
{
	return Fdemo_mapItemBuyValueRules::TryCompute(
		Item.ItemDefinitionId,
		Item.StackCount,
		Item.RewardValueMultiplierBps,
		Item.AffixSet.TotalResolvedValue(),
		Item.RareRewardBonusValue,
		OutValue,
		OutError);
}

bool Fdemo_mapRewardShopStock::TryComputeSellValue(
	const Fdemo_mapPersistentItemRecord& Item,
	int64& OutValue,
	FString* OutError)
{
	return Fdemo_mapItemSellValueRules::TryCompute(
		Item.ItemDefinitionId,
		Item.StackCount,
		Item.RewardValueMultiplierBps,
		Item.AffixSet.TotalResolvedValue(),
		Item.RareRewardBonusValue,
		OutValue,
		OutError);
}
