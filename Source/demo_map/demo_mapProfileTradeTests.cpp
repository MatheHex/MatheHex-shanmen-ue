#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionCoordinator.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapProfileTradeTransaction.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewTradeRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.6.P6.0.r3"),
			TEXT("ProfileTrade"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FGuid FindTradeItem(const Fdemo_mapProfileSessionSnapshot& Snapshot, FName DefinitionId)
	{
		for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
		{
			if (Item.ItemDefinitionId == DefinitionId) return Item.ItemInstanceId;
		}
		return FGuid();
	}

	Fdemo_mapProfileTradeIntent BuyIntent(const Fdemo_mapProfileSessionSnapshot& Snapshot, FName DefinitionId)
	{
		Fdemo_mapProfileTradeIntent Intent;
		Intent.Kind = Edemo_mapProfileTradeKind::Buy;
		Intent.ExpectedProfileId = Snapshot.ProfileId;
		Intent.ExpectedSaveGeneration = Snapshot.SaveGeneration;
		const Fdemo_mapPersistentShopStockEntry* Entry =
			Snapshot.ShopStock.Entries.FindByPredicate(
				[DefinitionId](const Fdemo_mapPersistentShopStockEntry& Candidate)
				{
					return Candidate.State
							== Edemo_mapPersistentShopStockEntryState::Available
						&& Candidate.Item.ItemDefinitionId == DefinitionId;
				});
		if (Entry)
		{
			Intent.ItemInstanceId = Entry->Item.ItemInstanceId;
			Intent.ShopStockPolicyId = Snapshot.ShopStock.PolicyId;
			Intent.ShopStockGeneration = Snapshot.ShopStock.Generation;
			Intent.ShopStockEventId = Snapshot.ShopStock.ShopStockEventId;
			Intent.ShopSlotId = Entry->SlotId;
			Intent.ExpectedBuyValue = Entry->QuotedBuyValue;
		}
		return Intent;
	}

	Fdemo_mapProfileTradeIntent BuySlotIntent(
		const Fdemo_mapProfileSessionSnapshot& Snapshot,
		FName SlotId)
	{
		Fdemo_mapProfileTradeIntent Intent;
		Intent.Kind = Edemo_mapProfileTradeKind::Buy;
		Intent.ExpectedProfileId = Snapshot.ProfileId;
		Intent.ExpectedSaveGeneration = Snapshot.SaveGeneration;
		const Fdemo_mapPersistentShopStockEntry* Entry =
			Snapshot.ShopStock.Entries.FindByPredicate(
				[SlotId](const Fdemo_mapPersistentShopStockEntry& Candidate)
				{
					return Candidate.SlotId == SlotId;
				});
		if (Entry)
		{
			Intent.ItemInstanceId = Entry->State
					== Edemo_mapPersistentShopStockEntryState::Available
				? Entry->Item.ItemInstanceId
				: Entry->SoldItemInstanceId;
			Intent.ShopStockPolicyId = Snapshot.ShopStock.PolicyId;
			Intent.ShopStockGeneration = Snapshot.ShopStock.Generation;
			Intent.ShopStockEventId = Snapshot.ShopStock.ShopStockEventId;
			Intent.ShopSlotId = Entry->SlotId;
			Intent.ExpectedBuyValue = Entry->State
					== Edemo_mapPersistentShopStockEntryState::Available
				? Entry->QuotedBuyValue
				: 1;
		}
		return Intent;
	}

	void AddShopStockProjection(
		const Fdemo_mapPersistentShopStockState& Stock,
		Fdemo_mapProfilePreparationSnapshot& Snapshot)
	{
		Snapshot.ShopStockPolicyId = Stock.PolicyId;
		Snapshot.ShopStockGeneration = Stock.Generation;
		Snapshot.ShopStockEventId = Stock.ShopStockEventId;
		for (const Fdemo_mapPersistentShopStockEntry& Entry : Stock.Entries)
		{
			Fdemo_mapProfileShopStockRow Row;
			Row.SlotId = Entry.SlotId;
			Row.SlotOrdinal = Entry.SlotOrdinal;
			Row.State = Entry.State;
			if (Entry.State == Edemo_mapPersistentShopStockEntryState::Available)
			{
				Row.ItemInstanceId = Entry.Item.ItemInstanceId;
				Row.ItemDefinitionId = Entry.Item.ItemDefinitionId;
				Row.AffixSet = Entry.Item.AffixSet;
				Row.BuyPrice = Entry.QuotedBuyValue;
				if (const Fdemo_mapItemDefinition* Definition =
					Fdemo_mapItemDefinitions::Find(Entry.Item.ItemDefinitionId))
				{
					Row.DisplayName = Definition->DisplayName.ToString();
					Row.ItemCategoryId = Definition->CategoryId;
					Row.Level = Definition->Level;
					Row.SellPrice = Definition->SellPrice
						+ Entry.Item.AffixSet.TotalResolvedValue();
				}
			}
			Snapshot.OrderedShopStockRows.Add(MoveTemp(Row));
		}
	}

	Fdemo_mapProfileTradeIntent SellIntent(const Fdemo_mapProfileSessionSnapshot& Snapshot, const FGuid& ItemId)
	{
		Fdemo_mapProfileTradeIntent Intent;
		Intent.Kind = Edemo_mapProfileTradeKind::Sell;
		Intent.ExpectedProfileId = Snapshot.ProfileId;
		Intent.ExpectedSaveGeneration = Snapshot.SaveGeneration;
		Intent.ItemInstanceId = ItemId;
		return Intent;
	}

	struct FTradeFixture
	{
		FString Root = NewTradeRoot();
		Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
		Fdemo_mapProfileSessionCoordinator Coordinator;

		bool Start(FAutomationTestBase& Test)
		{
			const Fdemo_mapProfileSessionInitializeResult Init = Coordinator.InitializeSession(Storage);
			if (!Init.IsReady()) Test.AddError(Init.Diagnostic);
			return Init.IsReady();
		}

		Fdemo_mapProfileTradeResult Sell(FName DefinitionId, const TSet<FGuid>& Selected = {})
		{
			const Fdemo_mapProfileSessionSnapshot Snapshot = Coordinator.GetSnapshot();
			return Coordinator.SubmitTrade(SellIntent(Snapshot, FindTradeItem(Snapshot, DefinitionId)), Selected);
		}

		Fdemo_mapProfileTradeResult Buy(FName DefinitionId)
		{
			const Fdemo_mapProfileSessionSnapshot Snapshot = Coordinator.GetSnapshot();
			return Coordinator.SubmitTrade(BuyIntent(Snapshot, DefinitionId), {});
		}
	};

	bool PrepareTradeProfile(
		const FString& Root,
		int64 Balance,
		TFunction<void(Fdemo_mapPersistentProfile&)> Mutate = {})
	{
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
		Fdemo_mapProfileLoadResult Load = Repository.LoadOrCreateDefaultProfile(Storage);
		if (!Load.IsSuccess()) return false;
		Load.Profile.PersistentSpiritStones = Balance;
		if (Mutate) Mutate(Load.Profile);
		return Repository.SaveProfile(Load.Profile, Storage).IsSuccess();
	}

	bool LoadBytes(const FString& Path, TArray<uint8>& Out)
	{
		return FFileHelper::LoadFileToArray(Out, *Path);
	}

	struct FTradeUIFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;

		bool Start(FAutomationTestBase& Test, const FString& Root)
		{
			if (!GEngine) return false;
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance) return false;
			GameInstance->AddToRoot();
			GameInstance->Init();
			Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
			const auto Init = Session
				? Session->InitializeSession(Fdemo_mapProfileStorageContext::ForRoot(Root))
				: Fdemo_mapProfileSessionInitializeResult();
			if (!Init.IsReady()) Test.AddError(Init.Diagnostic);
			return Init.IsReady();
		}

		Udemo_mapProfilePreparationWidget* MakeWidget()
		{
			Udemo_mapProfilePreparationWidget* Widget = NewObject<Udemo_mapProfilePreparationWidget>(
				GameInstance, NAME_None, RF_Transient);
			if (!Widget || !Widget->Initialize()) return nullptr;
			Widget->InitializeForSession(Session);
			return Widget;
		}

		~FTradeUIFixture()
		{
			if (!GameInstance) return;
			GameInstance->Shutdown();
			Session = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade01, "demo_map.ProfileTrade.01.CatalogExactImmutableOrderAndPrices", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade01::RunTest(const FString&)
{
	const TArray<Fdemo_mapProfileShopCatalogRow> Rows = Fdemo_mapProfileTradeTransaction::BuildCatalog();
	const TArray<FName> Expected = { Fdemo_mapItemIds::WeaponLevel1, Fdemo_mapItemIds::WeaponLevel2, Fdemo_mapItemIds::WeaponLevel3, Fdemo_mapItemIds::WeaponLevel4,
		Fdemo_mapItemIds::ArmorRobeLevel1, Fdemo_mapItemIds::ArmorRobeLevel2, Fdemo_mapItemIds::ArmorRobeLevel3, Fdemo_mapItemIds::ArmorRobeLevel4,
		Fdemo_mapItemIds::AccessoryLevel1, Fdemo_mapItemIds::BackpackLevel1, Fdemo_mapItemIds::HealingPillLevel1,
		Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapItemIds::HealingPillLevel3 };
	const TArray<int64> Prices = { 100, 200, 400, 800, 100, 200, 400, 800, 80, 120, 30, 60, 120 };
	bool bExact = Rows.Num() == Expected.Num();
	for (int32 Index = 0; bExact && Index < Rows.Num(); ++Index)
		bExact = Rows[Index].ItemDefinitionId == Expected[Index] && Rows[Index].BuyPrice == Prices[Index];
	TestTrue(TEXT("Catalog is exactly the P1 ordered purchasable registry projection"), bExact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade02, "demo_map.ProfileTrade.02.FreshBalanceGenerationRiskAndStash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade02::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto S = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Fresh committed profile is gen1 balance0 risk0 with three safe records"),
		S.SaveGeneration == 1 && S.PersistentSpiritStones == 0 && S.RiskSpiritStones == 0 && S.OrderedPermanentStash.Num() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade03, "demo_map.ProfileTrade.03.InsufficientBuyIsAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade03::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto Before = F.Coordinator.GetSnapshot(); TArray<uint8> A, B; LoadBytes(F.Storage.PrimaryPath(), A);
	const auto R = F.Buy(Fdemo_mapItemIds::HealingPillLevel1); LoadBytes(F.Storage.PrimaryPath(), B); const auto After = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Insufficient buy changes no bytes balance generation or stash"), R.Status == Edemo_mapProfileTradeStatus::InsufficientFunds && A == B
		&& Before.SaveGeneration == After.SaveGeneration && Before.PersistentSpiritStones == After.PersistentSpiritStones && Before.OrderedPermanentStash == After.OrderedPermanentStash);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade04, "demo_map.ProfileTrade.04.SellWholeBladeCreditsTwentyFive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade04::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const FGuid Blade = FindTradeItem(F.Coordinator.GetSnapshot(), Fdemo_mapItemIds::TrainingBlade);
	const auto R = F.Sell(Fdemo_mapItemIds::TrainingBlade); const auto S = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Whole Blade record sold for 25 and removed"), R.IsCommitted() && R.TotalPrice == 25 && S.PersistentSpiritStones == 25
		&& S.SaveGeneration == 2 && !S.OrderedPermanentStash.ContainsByPredicate([&Blade](const auto& I){ return I.ItemInstanceId == Blade; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade05, "demo_map.ProfileTrade.05.StackSellUsesCheckedWholeRecordTotal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade05::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); FGuid Dust;
	if (!PrepareTradeProfile(Root, 0, [&Dust](Fdemo_mapPersistentProfile& P){ Fdemo_mapPersistentItemRecord I; Dust = I.ItemInstanceId = FGuid::NewGuid(); I.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust; I.StackCount = 3; P.PermanentStash.Add(I); })) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	const auto S0 = F.Coordinator.GetSnapshot(); const auto R = F.Coordinator.SubmitTrade(SellIntent(S0, Dust), {}); const auto S1 = F.Coordinator.GetSnapshot();
	const auto* D = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::SpiritDust);
	TestTrue(TEXT("Whole stack sale credits unit price times stack"), D && R.IsCommitted() && R.TotalPrice == D->SellPrice * 3 && S1.PersistentSpiritStones == R.TotalPrice);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade06, "demo_map.ProfileTrade.06.ScriptedSellSellBuyEndsTwentyGenFour", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade06::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false;
	const auto A = F.Sell(Fdemo_mapItemIds::TrainingBlade); const auto B = F.Sell(Fdemo_mapItemIds::TrainingVest); const auto C = F.Buy(Fdemo_mapItemIds::HealingPillLevel1); const auto S = F.Coordinator.GetSnapshot();
	const auto* Pill = S.OrderedPermanentStash.FindByPredicate([](const auto& I){ return I.ItemDefinitionId == Fdemo_mapItemIds::HealingPillLevel1; });
	TestTrue(TEXT("Required Phase A sequence closes at balance20 gen4 with one stable Pill"), A.IsCommitted() && B.IsCommitted() && C.IsCommitted()
		&& S.PersistentSpiritStones == 20 && S.SaveGeneration == 4 && Pill && Pill->ItemInstanceId.IsValid() && Pill->StackCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade07, "demo_map.ProfileTrade.07.SoldSlotCannotBeBoughtTwice", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade07::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, 100)) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	Fdemo_mapProfileTradeIntent Intent = BuyIntent(
		F.Coordinator.GetSnapshot(),
		Fdemo_mapItemIds::HealingPillLevel1);
	const auto A = F.Coordinator.SubmitTrade(Intent, {});
	Intent.ExpectedSaveGeneration =
		F.Coordinator.GetSnapshot().SaveGeneration;
	const auto B = F.Coordinator.SubmitTrade(Intent, {});
	TestTrue(TEXT("Finite slot moves its one GUID once then remains SOLD"),
		A.IsCommitted()
		&& A.ItemInstanceId.IsValid()
		&& B.Status == Edemo_mapProfileTradeStatus::ShopSlotSold
		&& F.Coordinator.GetSnapshot().OrderedPermanentStash.Num() == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade08, "demo_map.ProfileTrade.08.StaleProfileIdRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade08::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; auto I = BuyIntent(F.Coordinator.GetSnapshot(), Fdemo_mapItemIds::HealingPillLevel1); I.ExpectedProfileId = FGuid::NewGuid();
	TestTrue(TEXT("Stale ProfileId rejects"), F.Coordinator.SubmitTrade(I, {}).Status == Edemo_mapProfileTradeStatus::StaleIntent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade09, "demo_map.ProfileTrade.09.StaleGenerationRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade09::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; auto I = SellIntent(F.Coordinator.GetSnapshot(), FindTradeItem(F.Coordinator.GetSnapshot(), Fdemo_mapItemIds::TrainingBlade)); --I.ExpectedSaveGeneration;
	TestTrue(TEXT("Stale generation rejects"), F.Coordinator.SubmitTrade(I, {}).Status == Edemo_mapProfileTradeStatus::StaleIntent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade10, "demo_map.ProfileTrade.10.UnknownDefinitionRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade10::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false;
	TestTrue(TEXT("Definition-only unknown buy cannot create an instance-bound quote"), F.Coordinator.SubmitTrade(BuyIntent(F.Coordinator.GetSnapshot(), TEXT("Prototype.Item.Unknown")), {}).Status == Edemo_mapProfileTradeStatus::InvalidIntent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade11, "demo_map.ProfileTrade.11.NonPurchasableDefinitionRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade11::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false;
	TestTrue(TEXT("Definition-only sell item cannot create an instance-bound quote"), F.Coordinator.SubmitTrade(BuyIntent(F.Coordinator.GetSnapshot(), Fdemo_mapItemIds::TrainingBlade), {}).Status == Edemo_mapProfileTradeStatus::InvalidIntent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade12, "demo_map.ProfileTrade.12.BuyCallerInstanceIdRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade12::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; auto I = BuyIntent(F.Coordinator.GetSnapshot(), Fdemo_mapItemIds::HealingPillLevel1); I.ItemInstanceId = FGuid::NewGuid();
	TestTrue(TEXT("Caller cannot inject a different stock identity"), F.Coordinator.SubmitTrade(I, {}).Status == Edemo_mapProfileTradeStatus::ShopStockStale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade13, "demo_map.ProfileTrade.13.MissingSellIdRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade13::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false;
	TestTrue(TEXT("Missing sell identity rejects"), F.Coordinator.SubmitTrade(SellIntent(F.Coordinator.GetSnapshot(), FGuid::NewGuid()), {}).Status == Edemo_mapProfileTradeStatus::ItemNotFound);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade14, "demo_map.ProfileTrade.14.SelectedDeploymentSellRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade14::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto S = F.Coordinator.GetSnapshot(); const FGuid Blade = FindTradeItem(S, Fdemo_mapItemIds::TrainingBlade);
	TestTrue(TEXT("Selected item sell rejects"), F.Coordinator.SubmitTrade(SellIntent(S, Blade), { Blade }).Status == Edemo_mapProfileTradeStatus::ItemSelectedForDeployment);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade15, "demo_map.ProfileTrade.15.UnrelatedSelectionDoesNotBlockSell", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade15::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto S = F.Coordinator.GetSnapshot(); const FGuid Blade = FindTradeItem(S, Fdemo_mapItemIds::TrainingBlade); const FGuid Vest = FindTradeItem(S, Fdemo_mapItemIds::TrainingVest);
	TestTrue(TEXT("Only the selected identity is blocked"), F.Coordinator.SubmitTrade(SellIntent(S, Vest), { Blade }).IsCommitted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade16, "demo_map.ProfileTrade.16.PrecommitFailurePreservesBeforeExactly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade16::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto Before = F.Coordinator.GetSnapshot(); TArray<uint8> A, B; LoadBytes(F.Storage.PrimaryPath(), A);
	F.Coordinator.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp); const auto R = F.Sell(Fdemo_mapItemIds::TrainingBlade); LoadBytes(F.Storage.PrimaryPath(), B); const auto After = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Precommit failure is exact Before"), R.Status == Edemo_mapProfileTradeStatus::PersistentCommitRejected && A == B
		&& Before.SaveGeneration == After.SaveGeneration && Before.PersistentSpiritStones == After.PersistentSpiritStones && Before.OrderedPermanentStash == After.OrderedPermanentStash);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade17, "demo_map.ProfileTrade.17.PostcommitAmbiguityReloadsIntendedAfterWithoutReplay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade17::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; F.Coordinator.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary);
	const auto R = F.Sell(Fdemo_mapItemIds::TrainingBlade); const auto S = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("One reload reconciles IntendedAfter exactly once"), R.Status == Edemo_mapProfileTradeStatus::ReconciledAfterReload && S.SaveGeneration == 2 && S.PersistentSpiritStones == 25 && S.OrderedPermanentStash.Num() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade18, "demo_map.ProfileTrade.18.RiskCurrencyAndActiveRunRemainUntouched", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade18::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto Before = F.Coordinator.GetSnapshot(); const auto R = F.Sell(Fdemo_mapItemIds::TrainingBlade); const auto After = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Trade touches only Persistent balance and Permanent Stash"), R.IsCommitted() && Before.RiskSpiritStones == 0 && After.RiskSpiritStones == 0 && !After.ActiveRunId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade19, "demo_map.ProfileTrade.19.AllFiniteSlotsCanMoveToPermanentStash", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade19::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, 100000)) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	TArray<FName> Slots;
	for (const Fdemo_mapPersistentShopStockEntry& Entry :
		F.Coordinator.GetSnapshot().ShopStock.Entries)
	{
		Slots.Add(Entry.SlotId);
	}
	bool bAll = Slots.Num() == 12;
	for (FName SlotId : Slots)
	{
		const Fdemo_mapProfileSessionSnapshot Snapshot =
			F.Coordinator.GetSnapshot();
		bAll &= F.Coordinator.SubmitTrade(
			BuySlotIntent(Snapshot, SlotId), {}).IsCommitted();
	}
	TestTrue(TEXT("All twelve finite stock instances move into the unbounded Permanent Stash"), bAll && F.Coordinator.GetSnapshot().OrderedPermanentStash.Num() == 15);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade20, "demo_map.ProfileTrade.20.SellPreservesRemainingOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade20::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; const auto Before = F.Coordinator.GetSnapshot(); const FGuid Vest = FindTradeItem(Before, Fdemo_mapItemIds::TrainingVest);
	const auto R = F.Coordinator.SubmitTrade(SellIntent(Before, Vest), {}); const auto After = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Middle removal preserves exact remaining order"), R.IsCommitted() && After.OrderedPermanentStash.Num() == 2
		&& After.OrderedPermanentStash[0] == Before.OrderedPermanentStash[0] && After.OrderedPermanentStash[1] == Before.OrderedPermanentStash[2]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade21, "demo_map.ProfileTrade.21.BalanceOverflowRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade21::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, MAX_int64)) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	TestTrue(TEXT("Credit overflow rejects"), F.Sell(Fdemo_mapItemIds::TrainingBlade).Status == Edemo_mapProfileTradeStatus::ArithmeticOverflow);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade22, "demo_map.ProfileTrade.22.InvalidStackRejectedBeforePersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade22::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; Fdemo_mapPersistentProfile P = Repository.CreateFreshProfile(); P.PermanentStash[0].StackCount = 0;
	Fdemo_mapProfileSessionSnapshot Snapshot; Snapshot.ProfileId = P.ProfileId; Snapshot.SaveGeneration = P.SaveGeneration;
	const auto R = Fdemo_mapProfileTradeTransaction().Execute(P, SellIntent(Snapshot, P.PermanentStash[0].ItemInstanceId), {}, Repository, Fdemo_mapProfileStorageContext::ForRoot(NewTradeRoot()));
	TestTrue(TEXT("Invalid stack rejects before repository write"), R.Result.Status == Edemo_mapProfileTradeStatus::InvalidStack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade23, "demo_map.ProfileTrade.23.OnlyReadyForPreparationStateAllowed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade23::RunTest(const FString&)
{
	bool bExact = true;
	for (uint8 Value = 0; Value <= static_cast<uint8>(Edemo_mapProfileSessionState::FatalProfileError); ++Value)
		bExact &= Fdemo_mapProfileTradeTransaction::IsSessionStateAllowed(Value)
			== (Value == static_cast<uint8>(Edemo_mapProfileSessionState::ReadyForPreparation));
	TestTrue(TEXT("Trade state gate is exact"), bExact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade24, "demo_map.ProfileTrade.24.PresenterPureBalanceCatalogAndSellProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade24::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot S; S.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation; S.PersistentSpiritStones = 100;
	AddShopStockProjection(
		Fdemo_mapProfileRepository().CreateFreshProfile().ShopStock,
		S);
	Fdemo_mapProfilePreparationStashRow Row; Row.ItemInstanceId = FGuid::NewGuid(); Row.ItemDefinitionId = Fdemo_mapItemIds::TrainingBlade; Row.StackCount = 1; S.OrderedPermanentStashRows.Add(Row);
	const FGuid BeforeId = S.OrderedPermanentStashRows[0].ItemInstanceId;
	const int32 BeforeStack = S.OrderedPermanentStashRows[0].StackCount;
	const auto V = Fdemo_mapProfilePreparationPresenter::BuildViewState(S);
	TestTrue(TEXT("Pure presenter projects finite stock without source mutation"), V.PersistentSpiritStones == 100 && V.OrderedShopRows.Num() == 12
		&& V.OrderedPermanentStashRows[0].bCanSell && V.OrderedPermanentStashRows[0].TotalSellPrice == 25
		&& S.OrderedPermanentStashRows[0].ItemInstanceId == BeforeId && S.OrderedPermanentStashRows[0].StackCount == BeforeStack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade25, "demo_map.ProfileTrade.25.WidgetSellRefreshesAuthority", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade25::RunTest(const FString&)
{
	FTradeUIFixture F; if (!F.Start(*this, NewTradeRoot())) return false; Udemo_mapProfilePreparationWidget* W = F.MakeWidget(); if (!W) return false;
	const FGuid Blade = FindTradeItem(F.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); const auto R = W->RequestSell(Blade);
	TestTrue(TEXT("Widget submits intent and refreshes authority view"), R.IsCommitted() && W->GetViewState().PersistentSpiritStones == 25 && W->GetSellButtonCount() == 2 && W->GetShopButtonCount() == 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade26, "demo_map.ProfileTrade.26.WidgetBuyRefreshesStableNewRow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade26::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, 30)) return false; FTradeUIFixture F; if (!F.Start(*this, Root)) return false; Udemo_mapProfilePreparationWidget* W = F.MakeWidget(); if (!W) return false;
	const auto R = W->RequestBuy(Fdemo_mapItemIds::HealingPillLevel1);
	TestTrue(TEXT("Widget buy appends stable row and refreshes balance"), R.IsCommitted() && W->GetViewState().PersistentSpiritStones == 0
		&& W->GetViewState().OrderedPermanentStashRows.ContainsByPredicate([&R](const auto& Row){ return Row.ItemInstanceId == R.ItemInstanceId && Row.StackCount == 1; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade27, "demo_map.ProfileTrade.27.SelectedSellFailurePreservesSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade27::RunTest(const FString&)
{
	FTradeUIFixture F; if (!F.Start(*this, NewTradeRoot())) return false; Udemo_mapProfilePreparationWidget* W = F.MakeWidget(); if (!W) return false;
	const FGuid Blade = FindTradeItem(F.Session->GetSnapshot(), Fdemo_mapItemIds::TrainingBlade); W->SelectEquipment(Fdemo_mapItemIds::WeaponSlot, Blade); const auto R = W->RequestSell(Blade);
	TestTrue(TEXT("Failed selected sell leaves selection intact"), R.Status == Edemo_mapProfileTradeStatus::ItemSelectedForDeployment && W->GetViewState().OrderedEquipmentSlots[0].ItemInstanceId == Blade);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade28, "demo_map.ProfileTrade.28.ReloadReadOnlyPreservesBytesAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade28::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; F.Sell(Fdemo_mapItemIds::TrainingBlade); F.Sell(Fdemo_mapItemIds::TrainingVest); const auto Buy = F.Buy(Fdemo_mapItemIds::HealingPillLevel1); const auto Before = F.Coordinator.GetSnapshot(); TArray<uint8> A, B; LoadBytes(F.Storage.PrimaryPath(), A);
	Fdemo_mapProfileSessionCoordinator Reloaded; const auto Init = Reloaded.InitializeSession(F.Storage); LoadBytes(F.Storage.PrimaryPath(), B); const auto After = Reloaded.GetSnapshot();
	TestTrue(TEXT("Second session reload is read-only and preserves exact durable identity"), Init.IsReady() && A == B && After.ProfileId == Before.ProfileId
		&& After.SaveGeneration == 4 && After.PersistentSpiritStones == 20 && FindTradeItem(After, Fdemo_mapItemIds::HealingPillLevel1) == Buy.ItemInstanceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade29, "demo_map.ProfileTrade.29.BuyPrecommitFailurePreservesBeforeExactly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade29::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, 30)) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	const auto Before = F.Coordinator.GetSnapshot(); TArray<uint8> A, B; LoadBytes(F.Storage.PrimaryPath(), A);
	F.Coordinator.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::WriteTemp);
	const auto R = F.Buy(Fdemo_mapItemIds::HealingPillLevel1); LoadBytes(F.Storage.PrimaryPath(), B); const auto After = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Buy precommit failure is exact Before"), R.Status == Edemo_mapProfileTradeStatus::PersistentCommitRejected && A == B
		&& Before.SaveGeneration == After.SaveGeneration && Before.PersistentSpiritStones == After.PersistentSpiritStones && Before.OrderedPermanentStash == After.OrderedPermanentStash);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade30, "demo_map.ProfileTrade.30.BuyPostcommitAmbiguityReloadsOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade30::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, 30)) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	const int32 BeforeGeneration = F.Coordinator.GetSnapshot().SaveGeneration;
	F.Coordinator.SetNextRepositoryFailureForAutomation(Edemo_mapProfileFailureStage::ReadBackCommittedPrimary);
	const auto R = F.Buy(Fdemo_mapItemIds::HealingPillLevel1); const auto After = F.Coordinator.GetSnapshot();
	TestTrue(TEXT("Buy ambiguity reloads IntendedAfter without replay"), R.Status == Edemo_mapProfileTradeStatus::ReconciledAfterReload
		&& After.SaveGeneration == BeforeGeneration + 1 && After.PersistentSpiritStones == 0 && After.OrderedPermanentStash.Num() == 4
		&& FindTradeItem(After, Fdemo_mapItemIds::HealingPillLevel1) == R.ItemInstanceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade31, "demo_map.ProfileTrade.31.BuyAppendPreservesOldStashOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade31::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); if (!PrepareTradeProfile(Root, 30)) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	const auto Before = F.Coordinator.GetSnapshot(); const auto R = F.Buy(Fdemo_mapItemIds::HealingPillLevel1); const auto After = F.Coordinator.GetSnapshot();
	bool bOrder = R.IsCommitted() && After.OrderedPermanentStash.Num() == Before.OrderedPermanentStash.Num() + 1;
	for (int32 Index = 0; bOrder && Index < Before.OrderedPermanentStash.Num(); ++Index) bOrder = Before.OrderedPermanentStash[Index] == After.OrderedPermanentStash[Index];
	bOrder &= After.OrderedPermanentStash.Last().ItemInstanceId == R.ItemInstanceId;
	TestTrue(TEXT("Buy appends after every old ordered record"), bOrder);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade32, "demo_map.ProfileTrade.32.HistoricalNonSellableDefinitionRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade32::RunTest(const FString&)
{
	const FString Root = NewTradeRoot(); FGuid Heavy;
	if (!PrepareTradeProfile(Root, 0, [&Heavy](Fdemo_mapPersistentProfile& P){ Fdemo_mapPersistentItemRecord I; Heavy = I.ItemInstanceId = FGuid::NewGuid(); I.ItemDefinitionId = Fdemo_mapItemIds::HeavyPracticeBlade; I.StackCount = 1; P.PermanentStash.Add(I); })) return false;
	FTradeFixture F; F.Root = Root; F.Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); if (!F.Start(*this)) return false;
	TestTrue(TEXT("Historical explicitly non-sellable item rejects"), F.Coordinator.SubmitTrade(SellIntent(F.Coordinator.GetSnapshot(), Heavy), {}).Status == Edemo_mapProfileTradeStatus::NotSellable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTrade33, "demo_map.ProfileTrade.33.SchemaTwoAndNoTradeJournalJsonKeys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTrade33::RunTest(const FString&)
{
	FTradeFixture F; if (!F.Start(*this)) return false; F.Sell(Fdemo_mapItemIds::TrainingBlade);
	FString Json; const bool bRead = FFileHelper::LoadFileToString(Json, *F.Storage.PrimaryPath());
	const FString CurrentSchemaToken = FString::Printf(
		TEXT("\"SchemaVersion\":%d"),
		Fdemo_mapPersistentProfile::CurrentSchemaVersion);
	TestTrue(TEXT("Trade keeps current Profile JSON shape with no journal"), bRead && Json.Contains(CurrentSchemaToken)
		&& Json.Contains(TEXT("\"PersistentSpiritStones\"")) && Json.Contains(TEXT("\"PermanentStash\""))
		&& Json.Contains(TEXT("\"ShopStock\"")) && Json.Contains(TEXT("\"ActiveRun\""))
		&& !Json.Contains(TEXT("TradeJournal")) && !Json.Contains(TEXT("\"Shop\"")));
	return true;
}

#endif
