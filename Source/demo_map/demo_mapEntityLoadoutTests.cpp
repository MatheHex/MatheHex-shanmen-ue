#include "demo_mapEntityLoadoutPresenter.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapItemDefinitions.h"
#include "demo_mapProfilePreparationWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"

namespace
{
	Fdemo_mapProfilePreparationStashRow LoadoutRow(
		const FGuid& Id,
		FName DefinitionId,
		int32 Quantity = 1)
	{
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = Id;
		Row.ItemDefinitionId = DefinitionId;
		Row.StackCount = Quantity;
		Row.bSafeInPermanentStash = true;
		if (const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId))
		{
			Row.ItemCategoryId = Definition->CategoryId;
			Row.CompatibleEquipmentSlotId =
				Definition->CompatibleSlotIds.Num() == 1
					? Definition->CompatibleSlotIds[0]
					: NAME_None;
			Row.bMaterialSelectionEligible =
				Definition->CategoryId
					== Fdemo_mapItemIds::MaterialCategory
				|| Definition->CategoryId
					== Fdemo_mapItemIds::ConsumableCategory;
		}
		return Row;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP2EntityLoadoutCapacityTest,
	"demo_map.P2.EntityLoadout.CapacityLayers",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP2EntityLoadoutCapacityTest::RunTest(const FString&)
{
	const Fdemo_mapSpatialStorageCapacityResult EmptySpatial =
		Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(NAME_None);
	const Fdemo_mapSpatialStorageCapacityResult LevelOneSpatial =
		Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
			Fdemo_mapItemIds::BackpackLevel1);
	const Fdemo_mapSpatialStorageCapacityResult LevelTwoSpatial =
		Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
			Fdemo_mapItemIds::BackpackLevel2);
	const Fdemo_mapInventoryCapacityResult EmptyTotal =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(NAME_None);
	const Fdemo_mapInventoryCapacityResult LevelOneTotal =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			Fdemo_mapItemIds::BackpackLevel1);
	const Fdemo_mapInventoryCapacityResult LevelTwoTotal =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			Fdemo_mapItemIds::BackpackLevel2);

	TestTrue(
		TEXT("Fixed quick area is always six"),
		Fdemo_mapEntityLoadoutRules::BaseQuickItemSlotCount == 6
			&& EmptyTotal.bSuccess
			&& EmptyTotal.Capacity == 6);
	TestTrue(
		TEXT("Space-item capacity comes from Definition metadata"),
		EmptySpatial.bSuccess
			&& EmptySpatial.Capacity == 0
			&& LevelOneSpatial.bSuccess
			&& LevelOneSpatial.Capacity == 10
			&& LevelTwoSpatial.bSuccess
			&& LevelTwoSpatial.Capacity == 14);
	TestTrue(
		TEXT("Total carried capacity combines fixed and dynamic layers"),
		LevelOneTotal.bSuccess
			&& LevelOneTotal.Capacity == 16
			&& LevelTwoTotal.bSuccess
			&& LevelTwoTotal.Capacity == 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP2EntityLoadoutProjectionTest,
	"demo_map.P2.EntityLoadout.SharedProjection",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP2EntityLoadoutProjectionTest::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	const FGuid Weapon = FGuid::NewGuid();
	const FGuid SpatialItem = FGuid::NewGuid();
	Snapshot.SelectedWeaponId = Weapon;
	Snapshot.SelectedBackpackId = SpatialItem;
	Snapshot.OrderedPermanentStashRows.Add(
		LoadoutRow(Weapon, Fdemo_mapItemIds::WeaponLevel1));
	Snapshot.OrderedPermanentStashRows.Add(
		LoadoutRow(SpatialItem, Fdemo_mapItemIds::BackpackLevel1));

	TArray<FGuid> Selected;
	for (int32 Index = 0; Index < 7; ++Index)
	{
		const FGuid Id = FGuid::NewGuid();
		Selected.Add(Id);
		Snapshot.OrderedPermanentStashRows.Add(
			LoadoutRow(
				Id,
				Index % 2 == 0
					? Fdemo_mapItemIds::SpiritWoodLevel1
					: Fdemo_mapItemIds::HealingPillLevel1,
				Index + 1));
	}
	Snapshot.OrderedSelectedMaterialIds = Selected;
	const FGuid WarehouseOnly = FGuid::NewGuid();
	Snapshot.OrderedPermanentStashRows.Add(
		LoadoutRow(
			WarehouseOnly,
			Fdemo_mapItemIds::SpiritOreLevel1,
			3));

	const Fdemo_mapEntityLoadoutView View =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerPreparationView(
			Snapshot);
	TestTrue(
		TEXT("Shared equipment structure exposes 1/1/1/1 slots"),
		View.bValid
			&& View.EquipmentSlots.Num() == 4
			&& View.AccessorySlotCount == 1);
	TestTrue(
		TEXT("First six GUIDs stay in fixed quick cells"),
		View.BaseQuickItemSlots.Num() == 6
			&& View.BaseQuickItemSlots[0].ItemInstanceId
				== Selected[0]
			&& View.BaseQuickItemSlots[5].ItemInstanceId
				== Selected[5]);
	TestTrue(
		TEXT("Overflow enters Definition-backed space-item storage"),
		View.SpatialStorageSlots.Num() == 10
			&& View.SpatialStorageSlots[0].ItemInstanceId
				== Selected[6]
			&& View.SpatialStorageSlots[0].Quantity == 7);
	TestTrue(
		TEXT("Warehouse projection excludes selected GUIDs"),
		View.WarehouseSlots.Num() == 30
			&& View.WarehouseSlots.ContainsByPredicate(
				[&WarehouseOnly](
					const Fdemo_mapEntityItemSlotView& Cell)
				{
					return Cell.ItemInstanceId == WarehouseOnly;
				})
			&& !View.WarehouseSlots.ContainsByPredicate(
				[&Selected](
					const Fdemo_mapEntityItemSlotView& Cell)
				{
					return Selected.Contains(
						Cell.ItemInstanceId);
				}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP2EntityLoadoutCapacityGuardTest,
	"demo_map.P2.EntityLoadout.CapacityGuard",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP2EntityLoadoutCapacityGuardTest::RunTest(const FString&)
{
	FString Diagnostic;
	TestTrue(
		TEXT("Six items fit without a space item"),
		Fdemo_mapEntityLoadoutRules::CanFitSelectedItems(
			6,
			NAME_None,
			&Diagnostic));
	TestFalse(
		TEXT("Seventh item rejects without a space item"),
		Fdemo_mapEntityLoadoutRules::CanFitSelectedItems(
			7,
			NAME_None,
			&Diagnostic));
	TestTrue(
		TEXT("Capacity rejection explains why nothing moved"),
		Diagnostic.Contains(TEXT("移回仓库")));
	TestTrue(
		TEXT("Sixteen items fit level-one space item"),
		Fdemo_mapEntityLoadoutRules::CanFitSelectedItems(
			16,
			Fdemo_mapItemIds::BackpackLevel1,
			&Diagnostic));
	TestFalse(
		TEXT("Seventeen items reject level-one space item"),
		Fdemo_mapEntityLoadoutRules::CanFitSelectedItems(
			17,
			Fdemo_mapItemIds::BackpackLevel1,
			&Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP2EntityLoadoutConfigAndWidgetTest,
	"demo_map.P2.EntityLoadout.ConfigAndWidgetSmoke",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP2EntityLoadoutConfigAndWidgetTest::RunTest(const FString&)
{
	const Fdemo_mapEntityLoadoutView Configurable =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerPreparationView(
			Fdemo_mapProfilePreparationSnapshot(),
			3);
	TestTrue(
		TEXT("Accessory slot count is configurable"),
		Configurable.AccessorySlotCount == 3
			&& Configurable.EquipmentSlots.Num() == 6);

	Udemo_mapProfilePreparationWidget* Widget =
		NewObject<Udemo_mapProfilePreparationWidget>();
	TestNotNull(TEXT("Battle-preparation widget constructs"), Widget);
	if (!Widget)
	{
		return false;
	}
	Widget->AddToRoot();
	Widget->InitializeForSession(nullptr);
	TestTrue(
		TEXT("Battle-preparation widget exposes complete empty grids"),
		Widget->IsInterfaceBuilt()
			&& Widget->GetBaseQuickSlotCount() == 6
			&& Widget->GetSpatialStorageSlotCount() == 0
			&& Widget->GetWarehouseGridSlotCount() == 30);

	FString VisualOutputPath;
	if (FParse::Value(
			FCommandLine::Get(),
			TEXT("P2VisibleOutput="),
			VisualOutputPath))
	{
		VisualOutputPath = FPaths::ConvertRelativePathToFull(
			VisualOutputPath);
		IFileManager::Get().MakeDirectory(
			*FPaths::GetPath(VisualOutputPath),
			true);
		FWidgetRenderer Renderer(true);
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			Widget->TakeWidget(),
			FVector2D(1600.0, 900.0));
		FImage Image;
		const bool bRendered =
			RenderTarget
			&& FImageUtils::GetRenderTargetImage(
				RenderTarget,
				Image)
			&& FImageUtils::SaveImageByExtension(
				*VisualOutputPath,
				Image);
		TestTrue(
			TEXT("Battle-preparation widget renders to a 1600x900 PNG"),
			bRendered);
	}
	Widget->RemoveFromRoot();
	return true;
}

#endif
