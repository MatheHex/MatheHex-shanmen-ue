#include "demo_mapSectNavigationWidget.h"
#include "demo_mapTownProgressionRules.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP1SectNavigationDescriptorTest,
	"demo_map.P1.SectNavigation.Descriptors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1SectNavigationDescriptorTest::RunTest(const FString& Parameters)
{
	const Fdemo_mapTownUpgradeCost ExpectedTownCosts[] = {
		{ 30, 30, 100 }, { 40, 40, 500 }, { 50, 50, 2500 },
		{ 60, 60, 12500 }, { 70, 70, 62500 }
	};
	for (int32 Level = 0; Level < UE_ARRAY_COUNT(ExpectedTownCosts); ++Level)
	{
		Fdemo_mapTownUpgradeCost Actual;
		TestTrue(TEXT("Town contract has the level"),
			Fdemo_mapTownProgressionRules::TryGetNextLevelCost(Level, Actual));
		TestTrue(TEXT("Town contract matches"),
			Actual.SpiritWood == ExpectedTownCosts[Level].SpiritWood
				&& Actual.SpiritOre == ExpectedTownCosts[Level].SpiritOre
				&& Actual.SpiritStones == ExpectedTownCosts[Level].SpiritStones);
	}
	Fdemo_mapTownUpgradeCost Unused;
	TestFalse(TEXT("Town contract stops at level 5"),
		Fdemo_mapTownProgressionRules::TryGetNextLevelCost(5, Unused));
	TestTrue(TEXT("Deferred random-stone contract is fixed"),
		Fdemo_mapTownProgressionRules::NormalSpiritStoneMin == 15
			&& Fdemo_mapTownProgressionRules::NormalSpiritStoneMax == 25
			&& Fdemo_mapTownProgressionRules::EliteSpiritStoneMin == 35
			&& Fdemo_mapTownProgressionRules::EliteSpiritStoneMax == 55
			&& Fdemo_mapTownProgressionRules::BossSpiritStoneMin == 80
			&& Fdemo_mapTownProgressionRules::BossSpiritStoneMax == 120);
	const TArray<Fdemo_mapSectBuildingDescriptor> Descriptors =
		Udemo_mapSectNavigationWidget::BuildDefaultBuildingDescriptors();
	TestEqual(TEXT("Home exposes six building entries"), Descriptors.Num(), 6);

	TSet<Edemo_mapSectPage> UniquePages;
	int32 AvailableCount = 0;
	int32 ClosedCount = 0;
	for (const Fdemo_mapSectBuildingDescriptor& Descriptor : Descriptors)
	{
		UniquePages.Add(Descriptor.Page);
		TestFalse(TEXT("Every building has a name"), Descriptor.Name.IsEmpty());
		TestFalse(TEXT("Every building has a status"), Descriptor.StatusLabel.IsEmpty());
		if (Descriptor.bAvailable)
		{
			++AvailableCount;
		}
		else
		{
			++ClosedCount;
			TestTrue(
				TEXT("Closed building communicates its state"),
				Descriptor.StatusLabel.Contains(TEXT("未开放")));
		}
	}
	TestEqual(TEXT("All six routes are unique"), UniquePages.Num(), 6);
	TestEqual(TEXT("Three P1 buildings are available"), AvailableCount, 3);
	TestEqual(TEXT("Three P1 buildings are explicit placeholders"), ClosedCount, 3);
	TestEqual(
		TEXT("Teleport page title"),
		Udemo_mapSectNavigationWidget::PageTitle(
			Edemo_mapSectPage::TeleportArray),
		FString(TEXT("传送阵")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP1SectNavigationWidgetSmokeTest,
	"demo_map.P1.SectNavigation.WidgetSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1SectNavigationWidgetSmokeTest::RunTest(const FString& Parameters)
{
	Udemo_mapSectNavigationWidget* Widget =
		NewObject<Udemo_mapSectNavigationWidget>();
	TestNotNull(TEXT("Sect navigation widget can be constructed"), Widget);
	if (!Widget)
	{
		return false;
	}
	Widget->AddToRoot();
	Widget->InitializeForLifecycle(nullptr, nullptr);
	TestTrue(TEXT("Programmatic interface is built"), Widget->IsInterfaceBuilt());
	TestEqual(
		TEXT("Initial page is sect home"),
		Widget->GetCurrentPage(),
		Edemo_mapSectPage::Home);
	TestEqual(
		TEXT("Home renders all six building entries"),
		Widget->GetBuildingEntryCount(),
		6);

	const TArray<Edemo_mapSectPage> RoutedPages = {
		Edemo_mapSectPage::TeleportArray,
		Edemo_mapSectPage::Warehouse,
		Edemo_mapSectPage::Town,
		Edemo_mapSectPage::ClosedBuildingOne,
		Edemo_mapSectPage::ClosedBuildingTwo,
		Edemo_mapSectPage::ClosedBuildingThree
	};
	for (const Edemo_mapSectPage Page : RoutedPages)
	{
		Widget->AutomationOpenPage(Page);
		TestEqual(
			*FString::Printf(TEXT("Route opens %s"), *Udemo_mapSectNavigationWidget::PageTitle(Page)),
			Widget->GetCurrentPage(),
			Page);
	}
	Widget->AutomationOpenWarehouseFromTeleport();
	TestEqual(
		TEXT("External warehouse entry keeps the selected M01 teleport page"),
		Widget->GetCurrentPage(),
		Edemo_mapSectPage::TeleportArray);
	TestFalse(
		TEXT("Unavailable external warehouse entry reports a visible diagnostic"),
		Widget->GetTeleportFeedbackForAutomation().IsEmpty());
	Widget->AutomationReturnFromCurrentPage();
	TestEqual(
		TEXT("Failed external warehouse entry returns through ordinary sect navigation"),
		Widget->GetCurrentPage(),
		Edemo_mapSectPage::Home);
	Widget->AutomationOpenWarehouseFromHome();
	Widget->AutomationReturnFromCurrentPage();
	TestEqual(
		TEXT("Direct home-to-warehouse return restores sect home"),
		Widget->GetCurrentPage(),
		Edemo_mapSectPage::Home);
	Widget->ShowHomePage();
	TestEqual(
		TEXT("Return operation restores sect home"),
		Widget->GetCurrentPage(),
		Edemo_mapSectPage::Home);
	TestEqual(
		TEXT("Home rebuild retains six entries"),
		Widget->GetBuildingEntryCount(),
		6);
	Widget->RemoveFromRoot();
	return true;
}

#endif
