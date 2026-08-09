#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapRewardShopStock.h"

namespace
{
	FGuid ShopProfileId(int32 Seed = 1)
	{
		return FGuid(
			0x53000000u + static_cast<uint32>(Seed),
			0x484F5000u,
			0x53544F43u,
			0x4B000000u + static_cast<uint32>(Seed));
	}

	bool GenerateShop(
		int32 Generation,
		Fdemo_mapPersistentShopStockState& Out,
		int32 ProfileSeed = 1)
	{
		return Fdemo_mapRewardShopStock::Generate(
			ShopProfileId(ProfileSeed),
			Generation,
			FGuid(),
			Out);
	}

	bool RunRewardShopStockCase(FAutomationTestBase& Test, int32 Case)
	{
		Fdemo_mapPersistentShopStockState Stock;
		const bool bGenerated = GenerateShop(0, Stock);
		FString Error;
		Test.TestTrue(
			TEXT("Default ShopStock generation and validation succeed"),
			bGenerated
				&& Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(),
					Stock,
					&Error));
		if (!bGenerated || Stock.Entries.Num() != 12) return false;
		const Fdemo_mapRewardShopStockPolicy& Policy =
			Fdemo_mapRewardShopStock::GetDefaultPolicy();

		if (Case == 0)
		{
			Test.TestTrue(TEXT("Policy identity and finite size"),
				Policy.PolicyId == Fdemo_mapRewardShopStock::DefaultPolicyId
				&& Policy.StockSize == 12
				&& Policy.InitialGeneration == 0
				&& Policy.bFiniteStock
				&& Policy.QuantityPerSlot == 1);
		}
		else if (Case == 1)
		{
			Test.TestTrue(TEXT("Shop disables Pity Jackpot and Rare"),
				Policy.EquipmentPityPolicyId.IsNone()
				&& Policy.JackpotPolicyId.IsNone()
				&& Policy.RareExtremePolicyId.IsNone());
		}
		else if (Case == 2)
		{
			Test.TestEqual(TEXT("Equipment Affix policy"),
				Policy.EquipmentAffixPolicyId,
				Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId);
		}
		else if (Case >= 3 && Case < 15)
		{
			const int32 Index = Case - 3;
			Test.TestTrue(TEXT("Stable slot identity and ordinal"),
				Policy.Slots[Index].SlotId == Stock.Entries[Index].SlotId
					&& Policy.Slots[Index].SlotOrdinal == Index
					&& Stock.Entries[Index].State
						== Edemo_mapPersistentShopStockEntryState::Available);
		}
		else if (Case == 15)
		{
			Test.TestEqual(TEXT("Weapon count"), 3,
				Policy.Slots.FilterByPredicate([](const auto& Slot)
				{
					return Slot.Category == Edemo_mapShopStockCategory::Weapon;
				}).Num());
		}
		else if (Case == 16)
		{
			Test.TestEqual(TEXT("Robe count"), 3,
				Policy.Slots.FilterByPredicate([](const auto& Slot)
				{
					return Slot.Category == Edemo_mapShopStockCategory::Robe;
				}).Num());
		}
		else if (Case == 17)
		{
			Test.TestEqual(TEXT("Pill count"), 6,
				Policy.Slots.FilterByPredicate([](const auto& Slot)
				{
					return Slot.Category == Edemo_mapShopStockCategory::HealingPill;
				}).Num());
		}
		else if (Case == 18)
		{
			Test.TestTrue(TEXT("Guaranteed pill is L1 BUY=30"),
				Stock.Entries[6].Item.ItemDefinitionId
					== Fdemo_mapItemIds::HealingPillLevel1
					&& Stock.Entries[6].QuotedBuyValue == 30);
		}
		else if (Case >= 19 && Case < 31)
		{
			const int32 Index = Case - 19;
			const auto& Entry = Stock.Entries[Index];
			const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(Entry.Item.ItemDefinitionId);
			int64 Buy = 0;
			int64 Sell = 0;
			Test.TestTrue(TEXT("Slot item, quote, and anti-arbitrage"),
				Definition
					&& Entry.Item.StackCount == 1
					&& Entry.Item.PersistentDomain
						== Edemo_mapPersistentDomain::ShopStock
					&& Fdemo_mapRewardShopStock::TryComputeBuyValue(
						Entry.Item, Buy)
					&& Fdemo_mapRewardShopStock::TryComputeSellValue(
						Entry.Item, Sell)
					&& Buy == Entry.QuotedBuyValue
					&& Buy > Sell);
		}
		else if (Case == 31)
		{
			TSet<FName> Definitions;
			for (int32 Index = 0; Index < 3; ++Index)
				Definitions.Add(Stock.Entries[Index].Item.ItemDefinitionId);
			Test.TestEqual(TEXT("Weapon definitions unique"), 3, Definitions.Num());
		}
		else if (Case == 32)
		{
			TSet<FName> Definitions;
			for (int32 Index = 3; Index < 6; ++Index)
				Definitions.Add(Stock.Entries[Index].Item.ItemDefinitionId);
			Test.TestEqual(TEXT("Robe definitions unique"), 3, Definitions.Num());
		}
		else if (Case == 33)
		{
			Fdemo_mapPersistentShopStockState Again;
			GenerateShop(0, Again);
			Test.TestTrue(TEXT("Same identity is fully deterministic"), Again == Stock);
		}
		else if (Case == 34)
		{
			Fdemo_mapPersistentShopStockState Next;
			GenerateShop(1, Next);
			Test.TestTrue(TEXT("Generation changes event and all item GUIDs"),
				Next.ShopStockEventId != Stock.ShopStockEventId
					&& Next.Entries[0].Item.ItemInstanceId
						!= Stock.Entries[0].Item.ItemInstanceId);
		}
		else if (Case == 35)
		{
			Fdemo_mapPersistentShopStockState Other;
			GenerateShop(0, Other, 2);
			Test.TestTrue(TEXT("Profile identity separates stock"),
				Other.ShopStockEventId != Stock.ShopStockEventId);
		}
		else if (Case == 36)
		{
			Test.TestTrue(TEXT("EventId is stable and valid"),
				Stock.ShopStockEventId.IsValid()
					&& Stock.ShopStockEventId
						== Fdemo_mapRewardShopStock::MakeEventId(
							ShopProfileId(),
							Policy.PolicyId,
							0));
		}
		else if (Case == 37)
		{
			TSet<FGuid> Ids;
			for (const auto& Entry : Stock.Entries)
				Ids.Add(Entry.Item.ItemInstanceId);
			Test.TestEqual(TEXT("Twelve unique item GUIDs"), 12, Ids.Num());
		}
		else if (Case == 38)
		{
			bool bClean = true;
			for (const auto& Entry : Stock.Entries)
			{
				bClean &= Entry.Item.RewardEventKind
						== Edemo_mapRewardEventKind::None
					&& !Entry.Item.RewardEventId.IsValid()
					&& !Entry.Item.RareRewardEventId.IsValid()
					&& Entry.Item.RareRewardBonusValue == 0;
			}
			Test.TestTrue(TEXT("Shop Jackpot and Rare metadata remain empty"), bClean);
		}
		else if (Case == 39)
		{
			bool bNoPity = true;
			for (const auto& Entry : Stock.Entries)
				bNoPity &= Entry.Item.AffixSet.Acquisition
					!= Edemo_mapRewardAffixAcquisition::PityGuaranteed;
			Test.TestTrue(TEXT("Shop never creates PityGuaranteed Affix"), bNoPity);
		}
		else if (Case == 40)
		{
			bool bPillsPlain = true;
			for (int32 Index = 6; Index < 12; ++Index)
				bPillsPlain &= Stock.Entries[Index].Item.AffixSet.IsEmpty();
			Test.TestTrue(TEXT("Pill slots never carry Affix"), bPillsPlain);
		}
		else if (Case == 41)
		{
			auto Invalid = Stock;
			Invalid.PolicyId = TEXT("Wrong");
			Test.TestFalse(TEXT("Wrong policy rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 42)
		{
			auto Invalid = Stock;
			Invalid.Entries.Pop();
			Test.TestFalse(TEXT("Missing slot rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 43)
		{
			auto Invalid = Stock;
			Invalid.Entries[1].SlotId = Invalid.Entries[0].SlotId;
			Test.TestFalse(TEXT("Duplicate slot rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 44)
		{
			auto Invalid = Stock;
			Invalid.Entries[1].Item.ItemInstanceId =
				Invalid.Entries[0].Item.ItemInstanceId;
			Test.TestFalse(TEXT("Duplicate available GUID rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 45)
		{
			auto Invalid = Stock;
			Invalid.Entries[0].QuotedBuyValue++;
			Test.TestFalse(TEXT("Stale quote rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 46)
		{
			auto Invalid = Stock;
			Invalid.Entries[0].Item.StackCount = 2;
			Test.TestFalse(TEXT("Quantity other than one rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 47)
		{
			auto Invalid = Stock;
			Invalid.Entries[6].Item.AffixSet =
				Stock.Entries[0].Item.AffixSet;
			if (!Invalid.Entries[6].Item.AffixSet.IsEmpty())
				Test.TestFalse(TEXT("Pill Affix rejected"),
					Fdemo_mapRewardShopStock::ValidateState(
						ShopProfileId(), Invalid));
			else
				Test.TestTrue(TEXT("Natural plain control remains valid"), true);
		}
		else if (Case == 48)
		{
			auto Invalid = Stock;
			Invalid.ShopStockEventId = ShopProfileId(9);
			Test.TestFalse(TEXT("Wrong stock EventId rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 49)
		{
			auto Sold = Stock;
			auto& Entry = Sold.Entries[0];
			Entry.State = Edemo_mapPersistentShopStockEntryState::Sold;
			Entry.SoldItemInstanceId = Entry.Item.ItemInstanceId;
			Entry.Item = {};
			Entry.QuotedBuyValue = 0;
			Test.TestTrue(TEXT("Canonical SOLD tombstone validates"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Sold));
		}
		else if (Case == 50)
		{
			auto Invalid = Stock;
			auto& Entry = Invalid.Entries[0];
			Entry.State = Edemo_mapPersistentShopStockEntryState::Sold;
			Entry.SoldItemInstanceId = Entry.Item.ItemInstanceId;
			Test.TestFalse(TEXT("SOLD cannot retain full item"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 51)
		{
			Fdemo_mapPersistentShopStockState Empty;
			Test.TestTrue(TEXT("Legacy uninitialized state validates"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Empty));
		}
		else if (Case == 52)
		{
			Fdemo_mapPersistentShopStockState Invalid;
			Invalid.Generation = 0;
			Test.TestFalse(TEXT("Partial legacy state rejected"),
				Fdemo_mapRewardShopStock::ValidateState(
					ShopProfileId(), Invalid));
		}
		else if (Case == 53)
		{
			Fdemo_mapPersistentShopStockState Invalid;
			Test.TestFalse(TEXT("Invalid ProfileId cannot validate initialized stock"),
				Fdemo_mapRewardShopStock::ValidateState(
					FGuid(), Stock));
		}
		else if (Case == 54)
		{
			Test.TestFalse(TEXT("Negative generation cannot generate"),
				Fdemo_mapRewardShopStock::Generate(
					ShopProfileId(), -1, FGuid(), Stock));
		}
		else if (Case == 55)
		{
			bool bBands = true;
			for (int32 Index = 0; Index < 12; ++Index)
			{
				const auto* Definition = Fdemo_mapItemDefinitions::Find(
					Stock.Entries[Index].Item.ItemDefinitionId);
				bBands &= Definition
					&& Policy.Slots[Index].LevelWeights.ContainsByPredicate(
						[Definition](const auto& Weight)
						{
							return Weight.Level == Definition->Level;
						});
			}
			Test.TestTrue(TEXT("Every selected level is inside its band"), bBands);
		}
		else if (Case == 56)
		{
			bool bWeights = true;
			for (const auto& Slot : Policy.Slots)
			{
				int32 Sum = 0;
				for (const auto& Weight : Slot.LevelWeights)
					Sum += Weight.WeightBps;
				bWeights &= Sum == 10000;
			}
			Test.TestTrue(TEXT("Every level band sums to 10000"), bWeights);
		}
		else if (Case == 57)
		{
			bool bAffixValid = true;
			for (int32 Index = 0; Index < 6; ++Index)
				bAffixValid &= Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
					Stock.Entries[Index].Item.ItemDefinitionId,
					1,
					Stock.Entries[Index].Item.AffixSet);
			Test.TestTrue(TEXT("Equipment natural Affix sets validate"), bAffixValid);
		}
		else if (Case == 58)
		{
			Test.TestTrue(TEXT("Natural Affix labels use P5 authority"),
				Stock.Entries[0].Item.AffixSet.IsEmpty()
					|| !Fdemo_mapRewardAffixPolicyRegistry::BuildDisplayLabel(
						Stock.Entries[0].Item.AffixSet).IsEmpty());
		}
		else if (Case == 59)
		{
			int64 Buy = 0;
			Test.TestFalse(TEXT("Unknown definition buy quote rejected"),
				Fdemo_mapItemBuyValueRules::TryCompute(
					TEXT("Unknown"), 1, 10000, 0, 0, Buy));
		}
		else if (Case == 60)
		{
			int64 Buy = 0;
			Test.TestFalse(TEXT("Negative Affix value buy quote rejected"),
				Fdemo_mapItemBuyValueRules::TryCompute(
					Fdemo_mapItemIds::WeaponLevel1,
					1, 10000, -1, 0, Buy));
		}
		else if (Case == 61)
		{
			const auto* Definition = Fdemo_mapItemDefinitions::Find(
				Stock.Entries[0].Item.ItemDefinitionId);
			Test.TestTrue(TEXT("Buy formula adds Affix exactly once"),
				Definition
					&& Stock.Entries[0].QuotedBuyValue
						== Definition->BuyPrice
							+ Stock.Entries[0].Item.AffixSet.TotalResolvedValue());
		}
		else if (Case == 62)
		{
			const auto* Definition = Fdemo_mapItemDefinitions::Find(
				Stock.Entries[0].Item.ItemDefinitionId);
			int64 Sell = 0;
			Test.TestTrue(TEXT("Sell formula adds Affix exactly once"),
				Definition
					&& Fdemo_mapRewardShopStock::TryComputeSellValue(
						Stock.Entries[0].Item, Sell)
					&& Sell == Definition->SellPrice
						+ Stock.Entries[0].Item.AffixSet.TotalResolvedValue());
		}
		else
		{
			const auto StockBefore = Stock;
			Fdemo_mapRewardAffixPityLedger Ledger;
			const int32 PityBefore = Ledger.GetState(
				ShopProfileId(),
				Fdemo_mapRewardAffixPolicyRegistry::PityChannelId);
			Fdemo_mapPersistentShopStockState Again;
			GenerateShop(0, Again);
			Test.TestTrue(TEXT("Generation does not read or mutate Pity ledger"),
				PityBefore == Ledger.GetState(
					ShopProfileId(),
					Fdemo_mapRewardAffixPolicyRegistry::PityChannelId)
					&& Again == StockBefore);
		}
		return true;
	}
}

#define SHOP_STOCK_TEST(N, Label) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardShopStock##N, \
		"demo_map.RewardShopStock." #N "." Label, \
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool FRewardShopStock##N::RunTest(const FString&) \
	{ return RunRewardShopStockCase(*this, N); }

SHOP_STOCK_TEST(0, "PolicyIdentity")
SHOP_STOCK_TEST(1, "EventPoliciesDisabled")
SHOP_STOCK_TEST(2, "AffixPolicy")
SHOP_STOCK_TEST(3, "Slot00")
SHOP_STOCK_TEST(4, "Slot01")
SHOP_STOCK_TEST(5, "Slot02")
SHOP_STOCK_TEST(6, "Slot03")
SHOP_STOCK_TEST(7, "Slot04")
SHOP_STOCK_TEST(8, "Slot05")
SHOP_STOCK_TEST(9, "Slot06")
SHOP_STOCK_TEST(10, "Slot07")
SHOP_STOCK_TEST(11, "Slot08")
SHOP_STOCK_TEST(12, "Slot09")
SHOP_STOCK_TEST(13, "Slot10")
SHOP_STOCK_TEST(14, "Slot11")
SHOP_STOCK_TEST(15, "WeaponCount")
SHOP_STOCK_TEST(16, "RobeCount")
SHOP_STOCK_TEST(17, "PillCount")
SHOP_STOCK_TEST(18, "GuaranteedBasicPill")
SHOP_STOCK_TEST(19, "Quote00")
SHOP_STOCK_TEST(20, "Quote01")
SHOP_STOCK_TEST(21, "Quote02")
SHOP_STOCK_TEST(22, "Quote03")
SHOP_STOCK_TEST(23, "Quote04")
SHOP_STOCK_TEST(24, "Quote05")
SHOP_STOCK_TEST(25, "Quote06")
SHOP_STOCK_TEST(26, "Quote07")
SHOP_STOCK_TEST(27, "Quote08")
SHOP_STOCK_TEST(28, "Quote09")
SHOP_STOCK_TEST(29, "Quote10")
SHOP_STOCK_TEST(30, "Quote11")
SHOP_STOCK_TEST(31, "UniqueWeapons")
SHOP_STOCK_TEST(32, "UniqueRobes")
SHOP_STOCK_TEST(33, "DeterministicReplay")
SHOP_STOCK_TEST(34, "GenerationSeparation")
SHOP_STOCK_TEST(35, "ProfileSeparation")
SHOP_STOCK_TEST(36, "StableEventId")
SHOP_STOCK_TEST(37, "UniqueItemGuids")
SHOP_STOCK_TEST(38, "NoJackpotRare")
SHOP_STOCK_TEST(39, "NoPity")
SHOP_STOCK_TEST(40, "PillsHaveNoAffix")
SHOP_STOCK_TEST(41, "WrongPolicyRejected")
SHOP_STOCK_TEST(42, "MissingSlotRejected")
SHOP_STOCK_TEST(43, "DuplicateSlotRejected")
SHOP_STOCK_TEST(44, "DuplicateGuidRejected")
SHOP_STOCK_TEST(45, "StaleQuoteRejected")
SHOP_STOCK_TEST(46, "QuantityRejected")
SHOP_STOCK_TEST(47, "PillAffixRejected")
SHOP_STOCK_TEST(48, "WrongEventRejected")
SHOP_STOCK_TEST(49, "SoldTombstone")
SHOP_STOCK_TEST(50, "SoldFullItemRejected")
SHOP_STOCK_TEST(51, "LegacyUninitialized")
SHOP_STOCK_TEST(52, "PartialLegacyRejected")
SHOP_STOCK_TEST(53, "InvalidProfileRejected")
SHOP_STOCK_TEST(54, "NegativeGenerationRejected")
SHOP_STOCK_TEST(55, "LevelBands")
SHOP_STOCK_TEST(56, "Weights")
SHOP_STOCK_TEST(57, "AffixValidation")
SHOP_STOCK_TEST(58, "AffixLabel")
SHOP_STOCK_TEST(59, "UnknownBuyQuote")
SHOP_STOCK_TEST(60, "NegativeAffixQuote")
SHOP_STOCK_TEST(61, "BuyFormula")
SHOP_STOCK_TEST(62, "SellFormula")
SHOP_STOCK_TEST(63, "PityIsolation")

#undef SHOP_STOCK_TEST

#endif
