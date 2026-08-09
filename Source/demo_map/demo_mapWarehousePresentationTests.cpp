#include "demo_mapItemPresentation.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapSectNavigationWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Slate/WidgetRenderer.h"

namespace
{
	Fdemo_mapProfilePreparationStashRow WarehouseTestRow(
		const FGuid& Id,
		FName DefinitionId,
		int32 Quantity = 1,
		bool bSelected = false)
	{
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = Id;
		Row.ItemDefinitionId = DefinitionId;
		Row.StackCount = Quantity;
		Row.bSafeInPermanentStash = true;
		Row.bSelected = bSelected;
		if (const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId))
		{
			Row.ItemCategoryId = Definition->CategoryId;
			Row.CompatibleEquipmentSlotId =
				Definition->CompatibleSlotIds.Num() == 1
					? Definition->CompatibleSlotIds[0]
					: NAME_None;
		}
		return Row;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP3WarehouseCapacityProjectionTest,
	"demo_map.P3.Warehouse.CapacityAndLegacyProjection",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP3WarehouseCapacityProjectionTest::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	const FGuid First = FGuid::NewGuid();
	const FGuid Second = FGuid::NewGuid();
	Snapshot.OrderedPermanentStashRows.Add(
		WarehouseTestRow(
			First,
			Fdemo_mapItemIds::WeaponLevel1));
	Snapshot.OrderedPermanentStashRows.Add(
		WarehouseTestRow(
			Second,
			Fdemo_mapItemIds::SpiritWoodLevel1,
			3));

	const Fdemo_mapWarehouseView View =
		Fdemo_mapItemPresentation::BuildWarehouseView(Snapshot);
	TestTrue(
		TEXT("Central warehouse configuration reports 30/42"),
		Fdemo_mapWarehouseView::CurrentCapacity == 30
			&& Fdemo_mapWarehouseView::NextLevelCapacity == 42);
	TestTrue(
		TEXT("Legacy layout is deterministic and includes empty cells"),
		View.bValid
			&& View.Slots.Num() == 30
			&& View.UsedSlots == 2
			&& View.RemainingSlots == 28
			&& View.Slots[0].ItemInstanceId == First
			&& View.Slots[1].ItemInstanceId == Second
			&& !View.Slots[2].bOccupied);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP3WarehouseMoveTransactionTest,
	"demo_map.P3.Warehouse.MoveSwapAndPersistence",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP3WarehouseMoveTransactionTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	Fdemo_mapPersistentProfile Profile =
		Repository.CreateFreshProfile();
	const FGuid First = Profile.PermanentStash[0].ItemInstanceId;
	const FGuid Second = Profile.PermanentStash[1].ItemInstanceId;
	const FName FirstDefinition =
		Profile.PermanentStash[0].ItemDefinitionId;
	const int32 FirstQuantity =
		Profile.PermanentStash[0].StackCount;
	Fdemo_mapProfileStorageContext Storage;
	Storage.RootDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("P3WarehouseTransactionTest"),
		FGuid::NewGuid().ToString(EGuidFormats::Digits));

	Fdemo_mapWarehouseMoveIntent Move;
	Move.ExpectedProfileId = Profile.ProfileId;
	Move.ExpectedSaveGeneration = Profile.SaveGeneration;
	Move.SourceSlotIndex = 0;
	Move.TargetSlotIndex = 8;
	const Fdemo_mapWarehouseMoveResult MoveResult =
		Fdemo_mapPersistentWarehouseTransaction().Execute(
			Profile,
			Move,
			Repository,
			Storage);
	TestTrue(TEXT("Move to empty cell commits"), MoveResult.IsSuccess());
	TestTrue(
		TEXT("Move preserves identity, definition, and quantity"),
		Profile.WarehouseLayout.SlotItemInstanceIds[8] == First
			&& Profile.PermanentStash[0].ItemInstanceId == First
			&& Profile.PermanentStash[0].ItemDefinitionId
				== FirstDefinition
			&& Profile.PermanentStash[0].StackCount
				== FirstQuantity);

	Fdemo_mapWarehouseMoveIntent Swap;
	Swap.ExpectedProfileId = Profile.ProfileId;
	Swap.ExpectedSaveGeneration = Profile.SaveGeneration;
	Swap.SourceSlotIndex = 1;
	Swap.TargetSlotIndex = 8;
	const Fdemo_mapWarehouseMoveResult SwapResult =
		Fdemo_mapPersistentWarehouseTransaction().Execute(
			Profile,
			Swap,
			Repository,
			Storage);
	TestTrue(
		TEXT("Occupied target swaps without copying"),
		SwapResult.IsSuccess()
			&& Profile.WarehouseLayout.SlotItemInstanceIds[1]
				== First
			&& Profile.WarehouseLayout.SlotItemInstanceIds[8]
				== Second);

	const Fdemo_mapProfileLoadResult Reload =
		Repository.LoadExistingProfile(Storage);
	TestTrue(
		TEXT("Reload retains exact slot hints"),
		Reload.IsSuccess()
			&& Reload.Profile.WarehouseLayout
				== Profile.WarehouseLayout);
	IFileManager::Get().DeleteDirectory(
		*Storage.RootDirectory,
		false,
		true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP3UnifiedItemDetailTest,
	"demo_map.P3.Items.DetailMapping",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP3UnifiedItemDetailTest::RunTest(const FString&)
{
	const Fdemo_mapProfilePreparationStashRow Row =
		WarehouseTestRow(
			FGuid::NewGuid(),
			Fdemo_mapItemIds::HealingPillLevel2,
			4);
	const Fdemo_mapUnifiedItemDetailView Detail =
		Fdemo_mapItemPresentation::BuildDetail(Row);
	TestTrue(
		TEXT("Detail uses Definition and Instance data"),
		Detail.bValid
			&& Detail.ItemInstanceId == Row.ItemInstanceId
			&& Detail.Quantity == 4
			&& Detail.LevelLabel == TEXT("2阶")
			&& Detail.TypeLabel
				== Fdemo_mapItemIds::ConsumableCategory.ToString()
			&& !Detail.UseEffect.IsEmpty()
			&& Detail.SellValue == TEXT("120"));
	TestTrue(
		TEXT("Missing description is explicit rather than fabricated"),
		Detail.Description.Contains(TEXT("暂无")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP3ContextActionResolverTest,
	"demo_map.P3.Items.ContextActions",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP3ContextActionResolverTest::RunTest(const FString&)
{
	const Fdemo_mapProfilePreparationStashRow Weapon =
		WarehouseTestRow(
			FGuid::NewGuid(),
			Fdemo_mapItemIds::WeaponLevel1);
	const Fdemo_mapResolvedItemActions WarehouseActions =
		Fdemo_mapItemPresentation::ResolveActions(
			Weapon,
			Edemo_mapItemPresentationContext::Warehouse);
	TestTrue(
		TEXT("Warehouse equipment shows only implemented operations"),
		WarehouseActions.Contains(
			Edemo_mapItemContextAction::ViewDetails)
			&& WarehouseActions.Contains(
				Edemo_mapItemContextAction::Move)
			&& WarehouseActions.Contains(
				Edemo_mapItemContextAction::Equip)
			&& !WarehouseActions.Contains(
				Edemo_mapItemContextAction::Unequip));

	Fdemo_mapProfilePreparationStashRow Selected = Weapon;
	Selected.bSelected = true;
	const Fdemo_mapResolvedItemActions EquipmentActions =
		Fdemo_mapItemPresentation::ResolveActions(
			Selected,
			Edemo_mapItemPresentationContext::TeleportEquipment);
	TestTrue(
		TEXT("Equipped context exposes unequip/return but no move"),
		EquipmentActions.Contains(
			Edemo_mapItemContextAction::Unequip)
			&& EquipmentActions.Contains(
				Edemo_mapItemContextAction::ReturnToWarehouse)
			&& !EquipmentActions.Contains(
				Edemo_mapItemContextAction::Move)
			&& !EquipmentActions.Contains(
				Edemo_mapItemContextAction::Equip));

	const Fdemo_mapProfilePreparationStashRow Material =
		WarehouseTestRow(
			FGuid::NewGuid(),
			Fdemo_mapItemIds::SpiritWoodLevel1,
			2);
	const Fdemo_mapResolvedItemActions MaterialActions =
		Fdemo_mapItemPresentation::ResolveActions(
			Material,
			Edemo_mapItemPresentationContext::Warehouse);
	TestTrue(
		TEXT("Non-equipment never exposes Equip"),
		!MaterialActions.Contains(
			Edemo_mapItemContextAction::Equip));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP3UnifiedCellWidgetTest,
	"demo_map.P3.Items.CellWidget",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP3UnifiedCellWidgetTest::RunTest(const FString&)
{
	Udemo_mapItemCellWidget* Widget =
		NewObject<Udemo_mapItemCellWidget>();
	TestNotNull(TEXT("Unified item cell constructs"), Widget);
	if (!Widget) return false;
	Widget->AddToRoot();
	const Fdemo_mapProfilePreparationStashRow Row =
		WarehouseTestRow(
			FGuid::NewGuid(),
			Fdemo_mapItemIds::AccessoryLevel1);
	const Fdemo_mapUnifiedItemCellView Cell =
		Fdemo_mapItemPresentation::BuildCell(&Row, 7);
	Widget->InitializeCell(
		Cell,
		Fdemo_mapItemCellActivated());
	TestTrue(
		TEXT("Cell remains a presentation-only copy of view state"),
		Widget->GetCellView().SlotIndex == 7
			&& Widget->GetCellView().ItemInstanceId
				== Row.ItemInstanceId
			&& Widget->GetCellView().bOccupied);
	Widget->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP3WarehouseVisibleSmokeTest,
	"demo_map.P3.Warehouse.VisibleSmoke",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP3WarehouseVisibleSmokeTest::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	const FGuid Weapon = FGuid::NewGuid();
	const FGuid Backpack = FGuid::NewGuid();
	const FGuid Material = FGuid::NewGuid();
	Snapshot.SelectedBackpackId = Backpack;
	Snapshot.OrderedPermanentStashRows.Add(
		WarehouseTestRow(
			Weapon,
			Fdemo_mapItemIds::WeaponLevel2));
	Snapshot.OrderedPermanentStashRows.Add(
		WarehouseTestRow(
			Backpack,
			Fdemo_mapItemIds::BackpackLevel1,
			1,
			true));
	Snapshot.OrderedPermanentStashRows.Add(
		WarehouseTestRow(
			Material,
			Fdemo_mapItemIds::SpiritOreLevel2,
			6));

	Udemo_mapSectNavigationWidget* Widget =
		NewObject<Udemo_mapSectNavigationWidget>();
	TestNotNull(TEXT("P3 sect widget constructs"), Widget);
	if (!Widget) return false;
	Widget->AddToRoot();
	Widget->InitializeForLifecycle(nullptr, nullptr);
	FWidgetRenderer Renderer(true);
	Renderer.DrawWidget(
		Widget->TakeWidget(),
		FVector2D(32.0f, 32.0f));
	Widget->AutomationSetItemPresentationSnapshot(Snapshot);
	Widget->AutomationOpenPage(Edemo_mapSectPage::Warehouse);
	TestTrue(
		TEXT("Warehouse keeps thirty reusable permanent cells alongside P2 preparation cells"),
		Widget->GetUnifiedItemCellCount() >= 30);
	TestTrue(
		TEXT("Selecting an occupied cell opens its detail state"),
		Widget->AutomationActivateWarehouseCell(0)
			&& Widget->GetFocusedItemId() == Weapon);

	FString VisibleOutput;
	if (FParse::Value(
		FCommandLine::Get(),
		TEXT("P3VisibleOutput="),
		VisibleOutput))
	{
		const FString OutputDirectory =
			FPaths::GetPath(
				FPaths::ConvertRelativePathToFull(
					VisibleOutput));
		IFileManager::Get().MakeDirectory(
			*OutputDirectory,
			true);
		auto SaveWidget = [&Renderer, Widget, this](
			const FString& Path,
			const FString& Label)
		{
			UTextureRenderTarget2D* RenderTarget =
				Renderer.DrawWidget(
					Widget->TakeWidget(),
					FVector2D(1600.0, 900.0));
			FImage Image;
			const bool bSaved =
				RenderTarget
				&& FImageUtils::GetRenderTargetImage(
					RenderTarget,
					Image)
				&& FImageUtils::SaveImageByExtension(
					*Path,
					Image);
			TestTrue(*Label, bSaved);
		};
		SaveWidget(
			FPaths::Combine(
				OutputDirectory,
				TEXT("P3Warehouse1600x900.png")),
			TEXT("Warehouse page renders to 1600x900"));
		Widget->AutomationOpenPage(
			Edemo_mapSectPage::TeleportArray);
		TestTrue(
			TEXT("Teleport reuses unified cells"),
			Widget->GetUnifiedItemCellCount() > 30);
		SaveWidget(
			FPaths::Combine(
				OutputDirectory,
				TEXT("P3Teleport1600x900.png")),
			TEXT("Teleport page renders to 1600x900"));
	}
	Widget->RemoveFromRoot();
	return true;
}

#endif
