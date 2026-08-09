#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentPreparationTransaction.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapSectNavigationWidget.h"
#include "demo_mapSpiritStoneTransaction.h"
#include "demo_mapTownProgressionRules.h"
#include "demo_mapTownUpgradeTransaction.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	constexpr EAutomationTestFlags P6IntegrationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	struct FP6IntegrationRoot
	{
		FString Path = FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.9.P6.0.r0"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));

		~FP6IntegrationRoot()
		{
			const FString Full = FPaths::ConvertRelativePathToFull(Path);
			const FString Allowed = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(
					FPaths::ProjectSavedDir(),
					TEXT("Automation"),
					TEXT("Dev.D.UE.0.0.9.P6.0.r0")));
			if (Full.StartsWith(Allowed))
			{
				IFileManager::Get().DeleteDirectory(*Full, false, true);
			}
		}

		Fdemo_mapProfileStorageContext Storage() const
		{
			return Fdemo_mapProfileStorageContext::ForRoot(Path);
		}
	};

	bool Check(
		FAutomationTestBase& Test,
		bool bCondition,
		const TCHAR* Message)
	{
		if (!bCondition)
		{
			Test.AddError(Message);
		}
		return bCondition;
	}

	Fdemo_mapPersistentItemRecord AddPermanentStack(
		Fdemo_mapPersistentProfile& Profile,
		FName DefinitionId,
		int32 Quantity)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = Quantity;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item;
	}

	int32 CountPermanentStacks(
		const Fdemo_mapPersistentProfile& Profile,
		FName DefinitionId)
	{
		int32 Total = 0;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
		{
			if (Item.PersistentDomain == Edemo_mapPersistentDomain::PermanentStash
				&& Item.ItemDefinitionId == DefinitionId)
			{
				Total += Item.StackCount;
			}
		}
		return Total;
	}

	Fdemo_mapPersistentPreparationLayout BuildStarterLayout(
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapPersistentPreparationLayout Layout;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
		{
			if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade)
			{
				Layout.WeaponItemInstanceId = Item.ItemInstanceId;
			}
			else if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest)
			{
				Layout.ArmorItemInstanceId = Item.ItemInstanceId;
			}
			else if (Item.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman)
			{
				Layout.AccessoryItemInstanceId = Item.ItemInstanceId;
			}
		}
		return Layout;
	}

	Fdemo_mapPersistentPreparationCommitResult CommitLayout(
		Fdemo_mapPersistentProfile& Profile,
		const Fdemo_mapPersistentPreparationLayout& Layout,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage)
	{
		Fdemo_mapPersistentPreparationCommitIntent Intent;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.Layout = Layout;
		return Fdemo_mapPersistentPreparationTransaction().Execute(
			Profile,
			Intent,
			Repository,
			Storage);
	}

	Fdemo_mapBeginRunResult BeginRun(
		Fdemo_mapPersistentProfile& Profile,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage)
	{
		Fdemo_mapBeginRunRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		Request.bRequireCommittedPreparationLayout = true;
		return Fdemo_mapProfileBeginRunTransaction().Execute(
			Profile,
			Request,
			Repository,
			Storage);
	}

	Fdemo_mapTownUpgradeIntent MakeTownUpgradeIntent(
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapTownUpgradeCost Cost;
		Fdemo_mapTownProgressionRules::TryGetNextLevelCost(
			Profile.TownLevel,
			Cost);
		Fdemo_mapTownUpgradeIntent Intent;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.ExpectedTownLevel = Profile.TownLevel;
		Intent.RequestedNextLevel = Profile.TownLevel + 1;
		Intent.ExpectedSpiritWoodCost = Cost.SpiritWood;
		Intent.ExpectedSpiritOreCost = Cost.SpiritOre;
		Intent.ExpectedSpiritStoneCost = Cost.SpiritStones;
		return Intent;
	}

	struct FP6SectWidgetFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		FString StorageRoot;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine unavailable."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine,
				NAME_None,
				RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("GameInstance allocation failed."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
			if (!Session)
			{
				Test.AddError(TEXT("Profile session unavailable."));
				return false;
			}
			StorageRoot = FPaths::Combine(
				FPaths::ProjectSavedDir(),
				TEXT("Automation"),
				TEXT("Dev.D.UE.0.0.9.P6.0.r0.Widget"),
				FGuid::NewGuid().ToString(EGuidFormats::Digits));
			return Session->InitializeSession(
				Fdemo_mapProfileStorageContext::ForRoot(StorageRoot)).IsReady();
		}

		Udemo_mapSectNavigationWidget* MakeWidget(
			FAutomationTestBase& Test)
		{
			Udemo_mapSectNavigationWidget* Widget =
				NewObject<Udemo_mapSectNavigationWidget>(
					GameInstance,
					NAME_None,
					RF_Transient);
			if (!Widget || !Widget->Initialize())
			{
				Test.AddError(TEXT("Sect navigation initialization failed."));
				return nullptr;
			}
			Widget->InitializeForLifecycle(Session, nullptr);
			return Widget;
		}

		~FP6SectWidgetFixture()
		{
			if (!GameInstance)
			{
				return;
			}
			GameInstance->Shutdown();
			Session = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
			const FString Full = FPaths::ConvertRelativePathToFull(StorageRoot);
			const FString Allowed = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(
					FPaths::ProjectSavedDir(),
					TEXT("Automation"),
					TEXT("Dev.D.UE.0.0.9.P6.0.r0.Widget")));
			if (!StorageRoot.IsEmpty() && Full.StartsWith(Allowed))
			{
				IFileManager::Get().DeleteDirectory(*Full, false, true);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapP6ProductLoopPersistenceTest,
	"demo_map.P6.Integration.ProductLoopPersistence",
	P6IntegrationFlags)

bool Fdemo_mapP6ProductLoopPersistenceTest::RunTest(const FString&)
{
	FP6IntegrationRoot Root;
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Root.Storage();
	Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
	AddPermanentStack(Profile, Fdemo_mapItemIds::SpiritWoodLevel1, 30);
	AddPermanentStack(Profile, Fdemo_mapItemIds::SpiritOreLevel1, 30);
	Profile.PersistentSpiritStones = 100;
	if (!Check(*this,
		Repository.SaveProfile(Profile, Storage).IsSuccess(),
		TEXT("P6 product-loop fixture save failed.")))
	{
		return false;
	}

	const Fdemo_mapPersistentPreparationCommitResult Prepared = CommitLayout(
		Profile,
		BuildStarterLayout(Profile),
		Repository,
		Storage);
	if (!Check(*this,
		Prepared.IsSuccess(),
		TEXT("P6 warehouse-to-run preparation did not commit.")))
	{
		return false;
	}

	const Fdemo_mapBeginRunResult Begun = BeginRun(Profile, Repository, Storage);
	if (!Check(*this,
		Begun.IsCommitted() && Profile.ActiveRun.bHasActiveRun,
		TEXT("P6 teleport Start Run did not produce an active authoritative Run.")))
	{
		return false;
	}

	Fdemo_mapSpiritStonePickupIntent Pickup;
	Pickup.ExpectedProfileId = Profile.ProfileId;
	Pickup.ExpectedSaveGeneration = Profile.SaveGeneration;
	Pickup.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
	Pickup.PickupId = TEXT("P6.ProductLoop.NormalPickup");
	Pickup.SourceId = TEXT("P6.ProductLoop.NormalSource");
	Pickup.Value = 25;
	const Fdemo_mapSpiritStonePickupResult Picked =
		Fdemo_mapSpiritStoneTransaction().Execute(
			Profile,
			Pickup,
			Repository,
			Storage);
	if (!Check(*this,
		Picked.IsCommitted() && Profile.ActiveRun.RiskSpiritStones == 25,
		TEXT("P6 risk-stone pickup did not update the active run exactly once.")))
	{
		return false;
	}

	Fdemo_mapRuntimeSettlementSnapshot RuntimeSnapshot;
	RuntimeSnapshot.ActiveRunId = Profile.ActiveRun.ActiveRunId;
	RuntimeSnapshot.CommittedEndReason = Edemo_mapRunEndReason::Extraction;
	RuntimeSnapshot.bValid = true;
	Fdemo_mapProfileSettlementRequest SettlementRequest;
	SettlementRequest.ExpectedProfileId = Profile.ProfileId;
	SettlementRequest.ExpectedSaveGeneration = Profile.SaveGeneration;
	SettlementRequest.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
	SettlementRequest.RequestedEndReason = Edemo_mapRunEndReason::Extraction;
	SettlementRequest.RuntimeSnapshot = RuntimeSnapshot;
	const Fdemo_mapProfileSettlementResult Settled =
		Fdemo_mapProfileSettlementTransaction().Execute(
			Profile,
			SettlementRequest,
			Repository,
			Storage);
	if (!Check(*this,
		Settled.IsCommitted()
			&& !Profile.ActiveRun.bHasActiveRun
			&& Profile.ActiveRun.RiskSpiritStones == 0
			&& Profile.PersistentSpiritStones == 125,
		TEXT("P6 extraction did not close risk currency into durable sect resources.")))
	{
		return false;
	}

	const Fdemo_mapTownUpgradeResult Upgrade =
		Fdemo_mapTownUpgradeTransaction().Execute(
			Profile,
			MakeTownUpgradeIntent(Profile),
			Repository,
			Storage);
	if (!Check(*this,
		Upgrade.IsCommitted()
			&& Profile.TownLevel == 1
			&& Profile.PersistentSpiritStones == 25
			&& CountPermanentStacks(Profile, Fdemo_mapItemIds::SpiritWoodLevel1) == 0
			&& CountPermanentStacks(Profile, Fdemo_mapItemIds::SpiritOreLevel1) == 0,
		TEXT("P6 town feedback authority did not consume the exact persistent resources.")))
	{
		return false;
	}

	const Fdemo_mapProfileLoadResult Reloaded =
		Repository.LoadExistingProfile(Storage);
	return Check(*this,
		Reloaded.IsSuccess()
			&& Reloaded.Profile.TownLevel == 1
			&& Reloaded.Profile.PersistentSpiritStones == 25
			&& !Reloaded.Profile.ActiveRun.bHasActiveRun
			&& Reloaded.Profile.ActiveRun.RiskSpiritStones == 0
			&& Reloaded.Profile.PreparationLayout == Profile.PreparationLayout,
		TEXT("P6 restart persistence lost town, resource, or preparation state."));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapP6SectRouteClosureTest,
	"demo_map.P6.Integration.SectRouteClosure",
	P6IntegrationFlags)

bool Fdemo_mapP6SectRouteClosureTest::RunTest(const FString&)
{
	FP6SectWidgetFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Udemo_mapSectNavigationWidget* Widget = Fixture.MakeWidget(*this);
	if (!Widget)
	{
		return false;
	}
	TestTrue(
		TEXT("P6 sect surface builds a single home router"),
		Widget->IsInterfaceBuilt()
			&& Widget->GetCurrentPage() == Edemo_mapSectPage::Home
			&& Widget->GetBuildingEntryCount() >= 4);

	Widget->AutomationOpenWarehouseFromTeleport();
	TestEqual(
		TEXT("P6 teleport-to-warehouse route keeps the teleport return context"),
		static_cast<uint8>(Widget->GetCurrentPage()),
		static_cast<uint8>(Edemo_mapSectPage::Warehouse));
	Widget->AutomationReturnFromCurrentPage();
	TestEqual(
		TEXT("P6 warehouse back action returns to the selected M01 teleport route"),
		static_cast<uint8>(Widget->GetCurrentPage()),
		static_cast<uint8>(Edemo_mapSectPage::TeleportArray));

	Widget->AutomationOpenWarehouseFromHome();
	Widget->AutomationReturnFromCurrentPage();
	TestEqual(
		TEXT("P6 home-to-warehouse route still returns home"),
		static_cast<uint8>(Widget->GetCurrentPage()),
		static_cast<uint8>(Edemo_mapSectPage::Home));

	Widget->AutomationOpenPage(Edemo_mapSectPage::Town);
	Widget->AutomationScrollToPageEnd();
	return Check(*this,
		Widget->GetCurrentPage() == Edemo_mapSectPage::Town,
		TEXT("P6 town page remains reachable and scrollable after route transitions."));
}

#endif
