#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapInputContextTypes.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapItemSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
	constexpr EAutomationTestFlags P5RuntimeFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	FString ReadP5Source(const TCHAR* Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(
			Text,
			*FPaths::Combine(
				FPaths::ProjectDir(),
				TEXT("Source/demo_map"),
				Name));
		return Text;
	}

	bool AddOne(
		Udemo_mapItemSubsystem* Items,
		FName DefinitionId,
		FGuid& OutId)
	{
		TArray<FGuid> Added;
		const Fdemo_mapItemOperationResult Result =
			Items->AddDefinition(
				DefinitionId,
				1,
				&Added);
		if (!Result.bSuccess || Added.Num() != 1)
		{
			return false;
		}
		OutId = Added[0];
		return OutId.IsValid();
	}
}

#define P5_RUNTIME_TEST(ClassName, Number, Name) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		ClassName, \
		"demo_map.P5RuntimeInterface." Number "." Name, \
		P5RuntimeFlags)

P5_RUNTIME_TEST(
	FP5Runtime01,
	"01",
	"RuntimeEntityLayoutUsesAuthority")
bool FP5Runtime01::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const Fdemo_mapEntityLoadoutView View =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerRuntimeView(
			Authority);
	TestTrue(
		TEXT("Runtime view valid"),
		View.bValid);
	TestEqual(
		TEXT("Runtime equipment cells follow the canonical equipment roles"),
		View.EquipmentSlots.Num(),
		Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num());
	TestEqual(
		TEXT("Fixed six base quick cells"),
		View.BaseQuickItemSlots.Num(),
		6);
	TestEqual(
		TEXT("No spatial capacity without spatial item"),
		View.SpatialStorageSlots.Num(),
		0);
	TestEqual(
		TEXT("Total mirrors authority"),
		View.TotalCarriedCapacity,
		Authority.GetInventoryCapacity());
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime02,
	"02",
	"SpatialCapacityTracksEquippedItem")
bool FP5Runtime02::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TArray<FGuid> Added;
	TestTrue(
		TEXT("Add backpack"),
		Authority.AddDefinition(
			Fdemo_mapItemIds::BackpackLevel2,
			1,
			&Added).bSuccess
			&& Added.Num() == 1);
	TestTrue(
		TEXT("Equip backpack"),
		Authority.Equip(
			Added[0],
			Fdemo_mapItemIds::BackpackSlot).bSuccess);
	const Fdemo_mapEntityLoadoutView View =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerRuntimeView(
			Authority);
	TestTrue(
		TEXT("Runtime spatial view valid"),
		View.bValid);
	TestEqual(
		TEXT("Spatial cells are total minus fixed six"),
		View.SpatialStorageSlots.Num(),
		Authority.GetInventoryCapacity() - 6);
	TestTrue(
		TEXT("Spatial equipment identity retained"),
		View.EquipmentSlots.Last().ItemInstanceId == Added[0]);
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime03,
	"03",
	"MovePreservesIdentityAndInvariants")
bool FP5Runtime03::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	TArray<FGuid> Backpack;
	TArray<FGuid> Pill;
	TestTrue(
		TEXT("Fixture materialized"),
		Authority.AddDefinition(
			Fdemo_mapItemIds::BackpackLevel2,
			1,
			&Backpack).bSuccess
			&& Authority.Equip(
				Backpack[0],
				Fdemo_mapItemIds::BackpackSlot).bSuccess
			&& Authority.AddDefinition(
				Fdemo_mapItemIds::HealingPillLevel1,
				1,
				&Pill).bSuccess);
	const int32 Source = Authority.FindInventorySlot(Pill[0]);
	TestTrue(
		TEXT("Move base to spatial"),
		Source >= 0
			&& Source < 6
			&& Authority.MoveInventorySlot(Source, 6).bSuccess);
	FString Error;
	TestTrue(
		TEXT("GUID moved without a copy"),
		Authority.GetInventorySlotSnapshot()[6] == Pill[0]
			&& Authority.FindInventorySlot(Pill[0]) == 6
			&& Authority.GetInstanceSnapshot().Contains(Pill[0]));
	TestTrue(
		TEXT("Post-move invariants"),
		Authority.ValidateInvariants(&Error));
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime04,
	"04",
	"MoveOutOfBaseClearsHotbar")
bool FP5Runtime04::RunTest(const FString&)
{
	Udemo_mapItemSubsystem* Items =
		NewObject<Udemo_mapItemSubsystem>(
			NewObject<UGameInstance>(GetTransientPackage()));
	FGuid Backpack;
	FGuid Pill;
	TestTrue(
		TEXT("Subsystem fixture"),
		AddOne(Items, Fdemo_mapItemIds::BackpackLevel2, Backpack)
			&& Items->Equip(
				Backpack,
				Fdemo_mapItemIds::BackpackSlot).bSuccess
			&& AddOne(
				Items,
				Fdemo_mapItemIds::HealingPillLevel1,
				Pill));
	const int32 Source =
		Items->GetAuthority().FindInventorySlot(Pill);
	TestTrue(
		TEXT("Legal base bind"),
		Items->BindHotbarSlot(3, Pill).bSuccess);
	TestTrue(
		TEXT("Move commits"),
		Items->MoveInventorySlot(Source, 6).bSuccess);
	TestFalse(
		TEXT("Illegal Runtime hotbar reference cleared"),
		Items->GetHotbarBindingSnapshot()
			.SlotBindings[2].IsValid());
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime05,
	"05",
	"UnifiedRuntimeActionsAreContextual")
bool FP5Runtime05::RunTest(const FString&)
{
	Fdemo_mapItemInstance Pill;
	Pill.InstanceId = FGuid::NewGuid();
	Pill.DefinitionId = Fdemo_mapItemIds::HealingPillLevel1;
	Pill.Quantity = 2;
	Pill.OwnershipState =
		Edemo_mapItemOwnershipState::Inventory;
	const Fdemo_mapResolvedItemActions Base =
		Fdemo_mapItemPresentation::ResolveRuntimeActions(
			Pill,
			Edemo_mapItemPresentationContext::
				RuntimeBaseQuickItems);
	const Fdemo_mapResolvedItemActions Spatial =
		Fdemo_mapItemPresentation::ResolveRuntimeActions(
			Pill,
			Edemo_mapItemPresentationContext::
				RuntimeSpatialStorage);
	TestTrue(
		TEXT("Base supports detail, move, hotbar, use"),
		Base.Contains(Edemo_mapItemContextAction::ViewDetails)
			&& Base.Contains(
				Edemo_mapItemContextAction::
					MoveToSpatialStorage)
			&& Base.Contains(
				Edemo_mapItemContextAction::EquipToHotbar)
			&& Base.Contains(
				Edemo_mapItemContextAction::Use));
	TestTrue(
		TEXT("Spatial supports return and direct use but not hotbar"),
		Spatial.Contains(
			Edemo_mapItemContextAction::
				MoveToBaseQuickItems)
			&& Spatial.Contains(
				Edemo_mapItemContextAction::Use)
			&& !Spatial.Contains(
				Edemo_mapItemContextAction::EquipToHotbar));
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime06,
	"06",
	"InventoryUsesUnifiedActualBinding")
bool FP5Runtime06::RunTest(const FString&)
{
	TestTrue(
		TEXT("Thirty unified product actions"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::
				GetExactDefaultActions().Num() == 33);
	const Fdemo_mapInputActionDefinition* Inventory =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::Inventory);
	TestTrue(
		TEXT("Inventory defaults to Tab"),
		Inventory
			&& Inventory->DefaultKey == EKeys::Tab
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Inventory).IsValid());
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime07,
	"07",
	"InventoryContextRestoresExactly")
bool FP5Runtime07::RunTest(const FString&)
{
	Fdemo_mapInputContextReasons Open;
	Open.bInventory = true;
	const Fdemo_mapInputContextResolution Inventory =
		Fdemo_mapInputContextResolver::Resolve(Open);
	const Fdemo_mapInputContextResolution Closed =
		Fdemo_mapInputContextResolver::Resolve(
			Fdemo_mapInputContextReasons());
	TestTrue(
		TEXT("Open is non-pausing GameAndUI lock"),
		Inventory.Context == Edemo_mapInputContext::Inventory
			&& !Inventory.bGameplayAllowed
			&& Inventory.bUseGameAndUI
			&& Inventory.bOwnedMoveLookIgnoreRequired);
	TestTrue(
		TEXT("Close is immediate exact GameOnly"),
		Closed.Context == Edemo_mapInputContext::Gameplay
			&& Closed.bGameplayAllowed
			&& Closed.bUseGameOnly
			&& !Closed.bOwnedMoveLookIgnoreRequired);
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime08,
	"08",
	"LifecycleUsesCentralInputOwner")
bool FP5Runtime08::RunTest(const FString&)
{
	const FString Manager = ReadP5Source(
		TEXT("demo_mapV3ProgressionManager.cpp"));
	const FString Controller = ReadP5Source(
		TEXT("demo_mapPlayerController.cpp"));
	TestTrue(
		TEXT("Manager delegates open and close"),
		Manager.Contains(
			TEXT("BeginInventoryInputLock(InventoryWidget)"))
			&& Manager.Contains(
				TEXT("RestoreGameplayControlFromInventory()")));
	TestTrue(
		TEXT("New run and EndPlay clear inventory ownership"),
		Controller.Contains(
			TEXT("bInventoryInputLockHeld = false"))
			&& Controller.Contains(
				TEXT("InventoryFocusWidget.Reset()")));
	return true;
}

P5_RUNTIME_TEST(
	FP5Runtime09,
	"09",
	"HUDPublishesRequiredRuntimeSurfaces")
bool FP5Runtime09::RunTest(const FString&)
{
	const FString HUD = ReadP5Source(TEXT("demo_mapHUD.cpp"));
	TestTrue(
		TEXT("HUD has health equipment hotbar interaction progress inventory and extraction sources"),
		HUD.Contains(TEXT("Health: %d / %d"))
			&& HUD.Contains(TEXT("CURRENT WEAPON"))
			&& HUD.Contains(TEXT("BuildRuntimeHotbar"))
			&& HUD.Contains(TEXT("GetInteractionPrompt"))
			&& HUD.Contains(TEXT("SEARCH / USE PROGRESS"))
			&& HUD.Contains(
				TEXT("Fdemo_mapInputActionIds::Inventory"))
			&& HUD.Contains(TEXT("Extraction Available")));
	return true;
}

#undef P5_RUNTIME_TEST

#endif
