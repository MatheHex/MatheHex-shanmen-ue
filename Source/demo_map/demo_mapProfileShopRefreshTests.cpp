#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapProfileShopStockTransaction.h"
#include "demo_mapProfileTradeTransaction.h"
#include "demo_mapRewardShopStock.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	FString ShopRefreshRoot(int32 Case)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.6.P6.0.r3"),
			TEXT("Tests"),
			TEXT("ProfileShopRefresh"),
			FString::Printf(
				TEXT("%02d-%s"),
				Case,
				*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	}

	Fdemo_mapProfileTradeIntent BuySlotIntent(
		const Fdemo_mapPersistentProfile& Profile,
		int32 SlotIndex)
	{
		Fdemo_mapProfileTradeIntent Intent;
		Intent.Kind = Edemo_mapProfileTradeKind::Buy;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.ShopStockPolicyId = Profile.ShopStock.PolicyId;
		Intent.ShopStockGeneration = Profile.ShopStock.Generation;
		Intent.ShopStockEventId = Profile.ShopStock.ShopStockEventId;
		const auto& Entry = Profile.ShopStock.Entries[SlotIndex];
		Intent.ShopSlotId = Entry.SlotId;
		Intent.ItemInstanceId = Entry.Item.ItemInstanceId;
		Intent.ExpectedBuyValue = Entry.QuotedBuyValue;
		return Intent;
	}

	bool RunProfileShopRefreshCase(FAutomationTestBase& Test, int32 Case)
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
		Test.TestTrue(TEXT("Fresh profile has valid Generation 0 stock"),
			Profile.ShopStock.bInitialized
				&& Profile.ShopStock.Generation == 0
				&& Profile.ShopStock.Entries.Num() == 12);

		if (Case == 0)
		{
			Profile.ShopStock = {};
			const auto Result =
				Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
					Profile,
					Repository,
					Fdemo_mapProfileStorageContext::ForRoot(
						ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Legacy missing stock initializes Generation 0"),
				Result.Status
					== Edemo_mapProfileShopStockEnsureStatus::Initialized
					&& Profile.ShopStock.Generation == 0);
		}
		else if (Case == 1)
		{
			const auto Before = Profile;
			const auto Result =
				Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
					Profile,
					Repository,
					Fdemo_mapProfileStorageContext::ForRoot(
						ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Reopen is unchanged and read-only"),
				Result.Status
					== Edemo_mapProfileShopStockEnsureStatus::Unchanged
					&& Profile == Before);
		}
		else if (Case >= 2 && Case <= 8)
		{
			const FGuid OldEvent = Profile.ShopStock.ShopStockEventId;
			Profile.LastSettlementId = FGuid(
				0x71000000u + static_cast<uint32>(Case),
				1, 2, 3);
			const auto Result =
				Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
					Profile,
					Repository,
					Fdemo_mapProfileStorageContext::ForRoot(
						ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("One pending terminal refreshes exactly once"),
				Result.Status
					== Edemo_mapProfileShopStockEnsureStatus::Refreshed
					&& Profile.ShopStock.Generation == 1
					&& Profile.ShopStock.ShopStockEventId != OldEvent
					&& Profile.ShopStock.LastAppliedTerminalId
						== Profile.LastSettlementId);
		}
		else if (Case == 9)
		{
			Profile.LastSettlementId = FGuid(9,8,7,6);
			const auto Storage =
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case));
			Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
				Profile, Repository, Storage);
			const auto Before = Profile;
			const auto Second =
				Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
					Profile, Repository, Storage);
			Test.TestTrue(TEXT("Repeated terminal cannot refresh twice"),
				Second.Status
					== Edemo_mapProfileShopStockEnsureStatus::Unchanged
					&& Profile == Before);
		}
		else if (Case == 10)
		{
			const auto Before = Profile;
			Profile.LastSettlementId = FGuid(10,8,7,6);
			const auto Storage =
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case));
			Fdemo_mapProfileStorageContext FailureStorage = Storage;
			FailureStorage.InjectedFailure =
				Edemo_mapProfileFailureStage::WriteTemp;
			const auto Failed =
				Fdemo_mapProfileShopStockTransaction().EnsureForPreparation(
					Profile, Repository, FailureStorage);
			Profile.LastSettlementId = Before.LastSettlementId;
			Test.TestTrue(TEXT("Refresh repository failure preserves old stock"),
				Failed.Status
					== Edemo_mapProfileShopStockEnsureStatus::RepositorySaveRejected
					&& Profile.ShopStock == Before.ShopStock);
		}
		else if (Case >= 11 && Case <= 29)
		{
			const int32 Slot = (Case - 11) % 12;
			const int64 Quote = Profile.ShopStock.Entries[Slot].QuotedBuyValue;
			Profile.PersistentSpiritStones = Quote + 1000;
			const FGuid ExpectedGuid =
				Profile.ShopStock.Entries[Slot].Item.ItemInstanceId;
			const int64 BalanceBefore = Profile.PersistentSpiritStones;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile,
				BuySlotIntent(Profile, Slot),
				{},
				Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Instance-bound buy is atomic and same-GUID"),
				Result.Result.IsCommitted()
					&& Profile.PersistentSpiritStones
						== BalanceBefore - Quote
					&& Profile.ShopStock.Entries[Slot].State
						== Edemo_mapPersistentShopStockEntryState::Sold
					&& Profile.ShopStock.Entries[Slot].SoldItemInstanceId
						== ExpectedGuid
					&& Profile.PermanentStash.ContainsByPredicate(
						[ExpectedGuid](const auto& Item)
						{
							return Item.ItemInstanceId == ExpectedGuid
								&& Item.PersistentDomain
									== Edemo_mapPersistentDomain::PermanentStash;
						}));
		}
		else if (Case == 30)
		{
			auto Intent = BuySlotIntent(Profile, 6);
			const auto Before = Profile;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Insufficient funds is zero mutation"),
				Result.Result.Status
					== Edemo_mapProfileTradeStatus::InsufficientFunds
					&& Profile == Before);
		}
		else if (Case == 31)
		{
			Profile.PersistentSpiritStones = 10000;
			auto Intent = BuySlotIntent(Profile, 0);
			--Intent.ExpectedSaveGeneration;
			const auto Before = Profile;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Stale SaveGeneration rejected"),
				Result.Result.Status == Edemo_mapProfileTradeStatus::StaleIntent
					&& Profile == Before);
		}
		else if (Case == 32)
		{
			Profile.PersistentSpiritStones = 10000;
			auto Intent = BuySlotIntent(Profile, 0);
			++Intent.ShopStockGeneration;
			const auto Before = Profile;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Stale stock generation rejected"),
				Result.Result.Status
					== Edemo_mapProfileTradeStatus::ShopStockStale
					&& Profile == Before);
		}
		else if (Case == 33)
		{
			Profile.PersistentSpiritStones = 10000;
			auto Intent = BuySlotIntent(Profile, 0);
			Intent.ItemInstanceId = FGuid::NewGuid();
			const auto Before = Profile;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Stale GUID rejected"),
				Result.Result.Status
					== Edemo_mapProfileTradeStatus::ShopStockStale
					&& Profile == Before);
		}
		else if (Case == 34)
		{
			Profile.PersistentSpiritStones = 10000;
			auto Intent = BuySlotIntent(Profile, 0);
			++Intent.ExpectedBuyValue;
			const auto Before = Profile;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			Test.TestTrue(TEXT("Stale quote rejected"),
				Result.Result.Status
					== Edemo_mapProfileTradeStatus::ShopQuoteMismatch
					&& Profile == Before);
		}
		else if (Case == 35)
		{
			Profile.PersistentSpiritStones = 10000;
			const auto Storage =
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case));
			auto Intent = BuySlotIntent(Profile, 0);
			const auto First = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository, Storage);
			Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
			const auto BeforeSecond = Profile;
			const auto Second = Fdemo_mapProfileTradeTransaction().Execute(
				Profile, Intent, {}, Repository, Storage);
			Test.TestTrue(TEXT("Double click cannot buy SOLD slot"),
				First.Result.IsCommitted()
					&& Second.Result.Status
						== Edemo_mapProfileTradeStatus::ShopSlotSold
					&& Profile == BeforeSecond);
		}
		else if (Case == 36)
		{
			const FString Root = ShopRefreshRoot(Case);
			const auto Storage =
				Fdemo_mapProfileStorageContext::ForRoot(Root);
			const auto Saved = Repository.SaveProfile(Profile, Storage);
			const auto Loaded = Repository.LoadExistingProfile(Storage);
			Test.TestTrue(TEXT("ShopStock Profile round-trip"),
				Saved.IsSuccess()
					&& Loaded.IsSuccess()
					&& Loaded.Profile.ShopStock == Profile.ShopStock);
		}
		else if (Case == 37)
		{
			Profile.ShopStock = {};
			const FString Root = ShopRefreshRoot(Case);
			const auto Storage =
				Fdemo_mapProfileStorageContext::ForRoot(Root);
			const auto Saved = Repository.SaveProfile(Profile, Storage);
			const auto Loaded = Repository.LoadExistingProfile(Storage);
			Test.TestTrue(TEXT("Schema3 missing ShopStock remains optional"),
				Saved.IsSuccess()
					&& Loaded.IsSuccess()
					&& !Loaded.Profile.ShopStock.bInitialized);
		}
		else if (Case == 38)
		{
			auto Invalid = Profile;
			Invalid.ShopStock.Entries[1].SlotId =
				Invalid.ShopStock.Entries[0].SlotId;
			Test.TestFalse(TEXT("Repository rejects duplicate Shop slot"),
				Repository.ValidateProfile(Invalid));
		}
		else
		{
			Profile.PersistentSpiritStones = 10000;
			const int32 Slot = 0;
			const auto ItemBefore = Profile.ShopStock.Entries[Slot].Item;
			const auto Result = Fdemo_mapProfileTradeTransaction().Execute(
				Profile,
				BuySlotIntent(Profile, Slot),
				{},
				Repository,
				Fdemo_mapProfileStorageContext::ForRoot(
					ShopRefreshRoot(Case)));
			const auto* ItemAfter =
				Profile.PermanentStash.FindByPredicate(
					[&ItemBefore](const auto& Item)
					{
						return Item.ItemInstanceId
							== ItemBefore.ItemInstanceId;
					});
			Test.TestTrue(TEXT("Affix metadata survives same-GUID purchase"),
				Result.Result.IsCommitted()
					&& ItemAfter
					&& ItemAfter->AffixSet == ItemBefore.AffixSet);
		}
		return true;
	}
}

#define SHOP_REFRESH_TEST(N, Label) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileShopRefresh##N, \
		"demo_map.ProfileShopRefresh." #N "." Label, \
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool FProfileShopRefresh##N::RunTest(const FString&) \
	{ return RunProfileShopRefreshCase(*this, N); }

SHOP_REFRESH_TEST(0, "LegacyInitialGeneration")
SHOP_REFRESH_TEST(1, "ReopenNoRefresh")
SHOP_REFRESH_TEST(2, "ExtractionRefresh")
SHOP_REFRESH_TEST(3, "DeathRefresh")
SHOP_REFRESH_TEST(4, "AbandonRefresh")
SHOP_REFRESH_TEST(5, "RestartPendingRefresh")
SHOP_REFRESH_TEST(6, "GenerationIncrement")
SHOP_REFRESH_TEST(7, "OldStockReplaced")
SHOP_REFRESH_TEST(8, "NewStockAvailable")
SHOP_REFRESH_TEST(9, "TerminalIdempotence")
SHOP_REFRESH_TEST(10, "RefreshRollback")
SHOP_REFRESH_TEST(11, "BuySlot00")
SHOP_REFRESH_TEST(12, "BuySlot01")
SHOP_REFRESH_TEST(13, "BuySlot02")
SHOP_REFRESH_TEST(14, "BuySlot03")
SHOP_REFRESH_TEST(15, "BuySlot04")
SHOP_REFRESH_TEST(16, "BuySlot05")
SHOP_REFRESH_TEST(17, "BuySlot06")
SHOP_REFRESH_TEST(18, "BuySlot07")
SHOP_REFRESH_TEST(19, "BuySlot08")
SHOP_REFRESH_TEST(20, "BuySlot09")
SHOP_REFRESH_TEST(21, "BuySlot10")
SHOP_REFRESH_TEST(22, "BuySlot11")
SHOP_REFRESH_TEST(23, "SameGuidTransfer")
SHOP_REFRESH_TEST(24, "SoldTombstone")
SHOP_REFRESH_TEST(25, "ExactDebit")
SHOP_REFRESH_TEST(26, "SingleCommit")
SHOP_REFRESH_TEST(27, "WarehouseOwnership")
SHOP_REFRESH_TEST(28, "EquipmentAffixTransfer")
SHOP_REFRESH_TEST(29, "PillTransfer")
SHOP_REFRESH_TEST(30, "InsufficientFundsRollback")
SHOP_REFRESH_TEST(31, "StaleSaveGeneration")
SHOP_REFRESH_TEST(32, "StaleStockGeneration")
SHOP_REFRESH_TEST(33, "StaleGuid")
SHOP_REFRESH_TEST(34, "StaleQuote")
SHOP_REFRESH_TEST(35, "DuplicateBuy")
SHOP_REFRESH_TEST(36, "ProfileRoundTrip")
SHOP_REFRESH_TEST(37, "OptionalSchema3")
SHOP_REFRESH_TEST(38, "InvalidJsonShape")
SHOP_REFRESH_TEST(39, "AffixPersistence")

#undef SHOP_REFRESH_TEST

#endif
