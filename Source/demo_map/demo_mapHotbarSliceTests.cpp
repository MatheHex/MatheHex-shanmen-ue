#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapAttributeComponent.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemPresentation.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapSectNavigationWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewP4StorageRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.7.P4.0.r0"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FGuid AddP4StashItem(
		Fdemo_mapPersistentProfile& Profile,
		FName DefinitionId,
		int32 Quantity)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = Quantity;
		Item.PersistentDomain =
			Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item.ItemInstanceId;
	}

	Fdemo_mapProfilePreparationStashRow P4Row(
		const FGuid& Id,
		FName DefinitionId,
		int32 Quantity,
		bool bSelected = true,
		bool bBaseQuick = true)
	{
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = Id;
		Row.ItemDefinitionId = DefinitionId;
		Row.StackCount = Quantity;
		Row.bSelected = bSelected;
		Row.bInBaseQuickItemArea = bBaseQuick;
		Row.bSafeInPermanentStash = true;
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

	struct FP4SessionFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;

		bool Start(
			FAutomationTestBase& Test,
			const Fdemo_mapProfileStorageContext& Storage)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine,
				NAME_None,
				RF_Transient);
			if (!GameInstance)
			{
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Session = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			return Session
				&& Session->InitializeSession(Storage).IsReady();
		}

		void Stop()
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
		}

		~FP4SessionFixture()
		{
			Stop();
		}
	};

	struct FP4RuntimeFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapItemSubsystem* Items = nullptr;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;

		FP4RuntimeFixture()
		{
			GameInstance = NewObject<UGameInstance>(
				GetTransientPackage());
			Items = NewObject<Udemo_mapItemSubsystem>(
				GameInstance);
			Attributes = NewObject<Udemo_mapAttributeComponent>(
				GetTransientPackage());
			Health = NewObject<Udemo_mapPlayerHealthComponent>(
				GetTransientPackage());
			Health->BindAttributeComponent(Attributes, true);
			Items->BindAttributeComponent(Attributes);
			Items->BindHealthComponent(Health);
			Items->BeginRun();
		}

		FGuid Add(
			FAutomationTestBase& Test,
			FName DefinitionId,
			int32 Quantity)
		{
			TArray<FGuid> Affected;
			Test.TestTrue(
				TEXT("Runtime item added"),
				Items->AddDefinition(
					DefinitionId,
					Quantity,
					&Affected).bSuccess
				&& Affected.Num() == 1);
			return Affected.Num() == 1
				? Affected[0]
				: FGuid();
		}

		Fdemo_mapItemUseResult Use(
			int32 Slot,
			const FGuid& ItemId,
			bool bInputAllowed = true)
		{
			Fdemo_mapItemUseIntent Intent;
			Intent.ExpectedRunId = Items->GetActiveRunId();
			Intent.HotbarSlotNumber = Slot;
			Intent.ExpectedItemInstanceId = ItemId;
			return Items->UseHotbarSlot(
				Intent,
				bInputAllowed);
		}
	};

	bool SaveP4Image(
		FAutomationTestBase& Test,
		FWidgetRenderer& Renderer,
		const TSharedRef<SWidget>& Widget,
		const FString& Path,
		const FString& Label)
	{
		UTextureRenderTarget2D* RenderTarget =
			Renderer.DrawWidget(
				Widget,
				FVector2D(1600.0f, 900.0f));
		FImage Image;
		const bool bSaved =
			RenderTarget
			&& FImageUtils::GetRenderTargetImage(
				RenderTarget,
				Image)
			&& FImageUtils::SaveImageByExtension(
				*Path,
				Image);
		Test.TestTrue(*Label, bSaved);
		return bSaved;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4PillLevelRuleTest,
	"demo_map.P4.Hotbar.01.PillLevelEqualsHeal",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4PillLevelRuleTest::RunTest(const FString&)
{
	const TArray<FName> Pills = {
		Fdemo_mapItemIds::HealingPillLevel1,
		Fdemo_mapItemIds::HealingPillLevel2,
		Fdemo_mapItemIds::HealingPillLevel3 };
	bool bExact = true;
	for (int32 Index = 0; Index < Pills.Num(); ++Index)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Pills[Index]);
		bExact &= Definition
			&& Definition->Level == Index + 1
			&& Definition->EffectParameters.Num() == 1
			&& Definition->EffectParameters[0].ParameterId
				== Fdemo_mapItemEffectIds::HealAmount
			&& Definition->EffectParameters[0].Value
				== Index + 1;
	}
	TestTrue(
		TEXT("Every pill heals exactly its item level"),
		bExact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4PreparationBindingTest,
	"demo_map.P4.Hotbar.02.BindReplaceAndPersistence",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4PreparationBindingTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(
			NewP4StorageRoot());
	Fdemo_mapPersistentProfile Profile =
		Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid PillOne = AddP4StashItem(
		Profile,
		Fdemo_mapItemIds::HealingPillLevel1,
		3);
	const FGuid PillTwo = AddP4StashItem(
		Profile,
		Fdemo_mapItemIds::HealingPillLevel2,
		2);
	TestTrue(
		TEXT("P4 binding fixture saved"),
		Repository.SaveProfile(Profile, Storage).IsSuccess());
	{
		FP4SessionFixture Fixture;
		if (!Fixture.Start(*this, Storage))
		{
			return false;
		}
		TestTrue(
			TEXT("Both pills enter Base Quick area"),
			Fixture.Session->SetPreparationMaterial(
				PillOne,
				true).IsAccepted()
			&& Fixture.Session->SetPreparationMaterial(
				PillTwo,
				true).IsAccepted());
		TestTrue(
			TEXT("Empty slot binds then occupied slot replaces"),
			Fixture.Session->SetPreparationHotbarSlot(
				2,
				PillOne).IsAccepted()
			&& Fixture.Session->SetPreparationHotbarSlot(
				2,
				PillTwo).IsAccepted());
		TestTrue(
			TEXT("A stable item may move to another Hotbar slot"),
			Fixture.Session->SetPreparationHotbarSlot(
				9,
				PillOne).IsAccepted());
		const Fdemo_mapProfilePreparationSnapshot Snapshot =
			Fixture.Session->GetPreparationSnapshot();
		TestTrue(
			TEXT("Replacement never duplicates an item"),
			Snapshot.HotbarBindings.SlotBindings[1] == PillTwo
			&& Snapshot.HotbarBindings.SlotBindings[8] == PillOne
			&& Snapshot.HotbarBindings.SlotBindings
				.FilterByPredicate(
					[&PillOne](const FGuid& Id)
					{
						return Id == PillOne;
					}).Num() == 1);
	}
	{
		FP4SessionFixture Reloaded;
		if (!Reloaded.Start(*this, Storage))
		{
			return false;
		}
		const Fdemo_mapProfilePreparationSnapshot ReloadedSnapshot =
			Reloaded.Session->GetPreparationSnapshot();
		TestTrue(
			TEXT("Reload retains exact 9-slot bindings"),
			ReloadedSnapshot.HotbarBindings.SlotBindings[1]
				== PillTwo
			&& ReloadedSnapshot.HotbarBindings.SlotBindings[8]
				== PillOne);
		TestTrue(
			TEXT("Moving an item out clears its binding"),
			Reloaded.Session->SetPreparationMaterial(
				PillOne,
				false).IsAccepted()
			&& !Reloaded.Session->GetPreparationSnapshot()
				.HotbarBindings.SlotBindings[8].IsValid());
	}
	IFileManager::Get().DeleteDirectory(
		*Storage.RootDirectory,
		false,
		true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4BaseQuickEligibilityTest,
	"demo_map.P4.Hotbar.03.BaseQuickEligibility",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4BaseQuickEligibilityTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(
			NewP4StorageRoot());
	Fdemo_mapPersistentProfile Profile =
		Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	const FGuid Backpack = AddP4StashItem(
		Profile,
		Fdemo_mapItemIds::BackpackLevel1,
		1);
	TArray<FGuid> Materials;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Materials.Add(AddP4StashItem(
			Profile,
			Fdemo_mapItemIds::SpiritWoodLevel1,
			Index + 1));
	}
	const FGuid Pill = AddP4StashItem(
		Profile,
		Fdemo_mapItemIds::HealingPillLevel1,
		2);
	TestTrue(
		TEXT("Eligibility fixture saved"),
		Repository.SaveProfile(Profile, Storage).IsSuccess());
	FP4SessionFixture Fixture;
	if (!Fixture.Start(*this, Storage))
	{
		return false;
	}
	TestTrue(
		TEXT("Spatial item expands carried capacity"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot,
			Backpack).IsAccepted());
	for (const FGuid& Material : Materials)
	{
		TestTrue(
			TEXT("Material selected"),
			Fixture.Session->SetPreparationMaterial(
				Material,
				true).IsAccepted());
	}
	TestTrue(
		TEXT("Pill selected into spatial storage"),
		Fixture.Session->SetPreparationMaterial(
			Pill,
			true).IsAccepted());
	TestTrue(
		TEXT("Seventh carried item cannot bind"),
		!Fixture.Session->SetPreparationHotbarSlot(
			1,
			Pill).IsAccepted());
	TestTrue(
		TEXT("After a Base Quick slot opens the same pill can bind"),
		Fixture.Session->SetPreparationMaterial(
			Materials[0],
			false).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			1,
			Pill).IsAccepted());
	TestTrue(
		TEXT("Removing then re-adding clears and does not auto-bind"),
		Fixture.Session->SetPreparationMaterial(
			Pill,
			false).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Pill,
			true).IsAccepted()
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[0].IsValid());
	Fixture.Stop();
	IFileManager::Get().DeleteDirectory(
		*Storage.RootDirectory,
		false,
		true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4PresentationTest,
	"demo_map.P4.Hotbar.04.SharedPresentationStates",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4PresentationTest::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	const FGuid Pill = FGuid::NewGuid();
	Snapshot.OrderedSelectedMaterialIds.Add(Pill);
	Snapshot.OrderedPermanentStashRows.Add(P4Row(
		Pill,
		Fdemo_mapItemIds::HealingPillLevel2,
		4));
	Snapshot.HotbarBindings.SlotBindings[0] = Pill;
	const TArray<Fdemo_mapHotbarSlotView> Ready =
		Fdemo_mapItemPresentation::BuildPreparationHotbar(
			Snapshot);
	const Fdemo_mapResolvedItemActions Actions =
		Fdemo_mapItemPresentation::ResolveActions(
			Snapshot.OrderedPermanentStashRows[0],
			Edemo_mapItemPresentationContext::
				TeleportQuickItems);
	TestTrue(
		TEXT("Preparation and P3 actions expose one ready binding"),
		Ready.Num() == 9
		&& Ready[0].State == Edemo_mapHotbarSlotState::Ready
		&& Ready[0].Quantity == 4
		&& Fdemo_mapItemPresentation::BuildHotbarSlotLabel(
			Ready[0],
			TEXT("1")).Contains(TEXT("×4"))
		&& Actions.Contains(
			Edemo_mapItemContextAction::EquipToHotbar));

	Snapshot.HotbarBindings.SlotBindings[1] =
		FGuid::NewGuid();
	const TArray<Fdemo_mapHotbarSlotView> Invalid =
		Fdemo_mapItemPresentation::BuildPreparationHotbar(
			Snapshot);
	TestTrue(
		TEXT("Stale identity is visibly invalid"),
		Invalid[1].State
			== Edemo_mapHotbarSlotState::InvalidBinding);
	Fdemo_mapProfilePreparationStashRow Spatial =
		Snapshot.OrderedPermanentStashRows[0];
	Spatial.bInBaseQuickItemArea = false;
	TestFalse(
		TEXT("Spatial-storage consumable has no bind action"),
		Fdemo_mapItemPresentation::ResolveActions(
			Spatial,
			Edemo_mapItemPresentationContext::
				TeleportQuickItems).Contains(
					Edemo_mapItemContextAction::
						EquipToHotbar));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4AtomicUseTest,
	"demo_map.P4.Hotbar.05.AtomicUseAndImmediateSync",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4AtomicUseTest::RunTest(const FString&)
{
	FP4RuntimeFixture Fixture;
	Fixture.Health->SetCurrentHealthForAutomation(1);
	const FGuid Pill = Fixture.Add(
		*this,
		Fdemo_mapItemIds::HealingPillLevel3,
		2);
	TestTrue(
		TEXT("Runtime binding accepts the real pill identity"),
		Fixture.Items->BindHotbarSlot(3, Pill).bSuccess);
	const Fdemo_mapItemUseResult First =
		Fixture.Use(3, Pill);
	TestTrue(
		TEXT("Level 3 heals 3 and consumes exactly one"),
		First.IsSuccess()
		&& First.HealRequested == 3
		&& First.HealApplied == 3
		&& First.BeforeStack == 2
		&& First.AfterStack == 1
		&& Fixture.Health->GetCurrentHealth() == 4
		&& Fixture.Items->GetHotbarBindingSnapshot()
			.SlotBindings[2] == Pill);

	Fixture.Items->AdvanceItemUseTimeForAutomation(2.0);
	Fixture.Health->SetCurrentHealthForAutomation(
		Fixture.Health->GetMaxHealth());
	const Fdemo_mapItemUseResult Full =
		Fixture.Use(3, Pill);
	TestTrue(
		TEXT("Full health changes no stack"),
		Full.Status == Edemo_mapItemUseStatus::FullHealth
		&& Fixture.Items->GetAuthority()
			.FindInstance(Pill)->Quantity == 1);
	Fixture.Health->SetCurrentHealthForAutomation(1);
	const Fdemo_mapItemUseResult Last =
		Fixture.Use(3, Pill);
	TestTrue(
		TEXT("Last successful use clears the same binding"),
		Last.IsSuccess()
		&& Last.AfterStack == 0
		&& Last.bBindingCleared
		&& !Fixture.Items->GetHotbarBindingSnapshot()
			.SlotBindings[2].IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4InputRegistryTest,
	"demo_map.P4.Hotbar.06.InputRegistryOneToNine",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4InputRegistryTest::RunTest(const FString&)
{
	const Fdemo_mapInputBindingSettings& Settings =
		Fdemo_mapInputBindingSettings::Get();
	const TArray<FKey> Expected = {
		EKeys::One,
		EKeys::Two,
		EKeys::Three,
		EKeys::Four,
		EKeys::Five,
		EKeys::Six,
		EKeys::Seven,
		EKeys::Eight,
		EKeys::Nine };
	bool bExact = true;
	for (int32 Index = 0; Index < Expected.Num(); ++Index)
	{
		bExact &=
			Settings.GetKey(
				Fdemo_mapInputActionRegistry::HotbarActionId(
					Index + 1))
				== Expected[Index];
	}
	TestTrue(
		TEXT("The unified registry owns exact 1..9 bindings"),
		bExact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP4VisibleSmokeTest,
	"demo_map.P4.Hotbar.07.VisibleSmoke",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FP4VisibleSmokeTest::RunTest(const FString&)
{
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	const FGuid PillOne = FGuid::NewGuid();
	const FGuid PillTwo = FGuid::NewGuid();
	Snapshot.OrderedSelectedMaterialIds = {
		PillOne,
		PillTwo };
	Snapshot.OrderedPermanentStashRows.Add(P4Row(
		PillOne,
		Fdemo_mapItemIds::HealingPillLevel1,
		5));
	Snapshot.OrderedPermanentStashRows.Add(P4Row(
		PillTwo,
		Fdemo_mapItemIds::HealingPillLevel3,
		2));
	Snapshot.HotbarBindings.SlotBindings[0] = PillOne;
	Snapshot.HotbarBindings.SlotBindings[4] = PillTwo;

	Udemo_mapSectNavigationWidget* Preparation =
		NewObject<Udemo_mapSectNavigationWidget>();
	TestNotNull(
		TEXT("Preparation Hotbar widget constructs"),
		Preparation);
	if (!Preparation)
	{
		return false;
	}
	Preparation->AddToRoot();
	Preparation->InitializeForLifecycle(nullptr, nullptr);
	Preparation->AutomationSetItemPresentationSnapshot(
		Snapshot);
	Preparation->AutomationOpenPage(
		Edemo_mapSectPage::TeleportArray);
	FWidgetRenderer Renderer(true);
	Renderer.DrawWidget(
		Preparation->TakeWidget(),
		FVector2D(1600.0f, 900.0f));
	Preparation->AutomationScrollToPageEnd();

	FString Output;
	if (FParse::Value(
		FCommandLine::Get(),
		TEXT("P4VisibleOutput="),
		Output))
	{
		const FString Directory =
			FPaths::ConvertRelativePathToFull(Output);
		IFileManager::Get().MakeDirectory(
			*Directory,
			true);
		SaveP4Image(
			*this,
			Renderer,
			Preparation->TakeWidget(),
			FPaths::Combine(
				Directory,
				TEXT("P4PreparationHotbar1600x900.png")),
			TEXT("Preparation Hotbar renders to 1600x900"));

		FP4RuntimeFixture Runtime;
		const FGuid RuntimePill = Runtime.Add(
			*this,
			Fdemo_mapItemIds::HealingPillLevel2,
			3);
		Runtime.Items->BindHotbarSlot(2, RuntimePill);
		const TArray<Fdemo_mapHotbarSlotView> RuntimeSlots =
			Fdemo_mapItemPresentation::BuildRuntimeHotbar(
				Runtime.Items->GetHotbarBindingSnapshot(),
				Runtime.Items->GetAuthority(),
				Runtime.Items->GetItemUseCooldownSnapshot());
		TSharedRef<SHorizontalBox> RuntimeStrip =
			SNew(SHorizontalBox);
		for (const Fdemo_mapHotbarSlotView& Slot :
			RuntimeSlots)
		{
			const FString KeyLabel = Fdemo_mapInputBindingSettings::
				Get().GetKey(
					Fdemo_mapInputActionRegistry::HotbarActionId(
						Slot.SlotNumber))
					.GetDisplayName().ToString();
			RuntimeStrip->AddSlot()
				.FillWidth(1.0f)
				.Padding(3.0f)
			[
				SNew(SBox)
				.MinDesiredHeight(72.0f)
				[
					SNew(SBorder)
						.BorderBackgroundColor(
							FLinearColor(
								0.035f,
								0.075f,
								0.11f,
								0.98f))
						.Padding(8.0f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(
									Fdemo_mapItemPresentation::
										BuildHotbarSlotLabel(
											Slot,
											KeyLabel)))
								.ColorAndOpacity(
									FSlateColor(
										FLinearColor::White))
						]
				]
			];
		}
		TSharedRef<SOverlay> RuntimeRoot =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
					.BorderBackgroundColor(
						FLinearColor(
							0.008f,
							0.014f,
							0.022f,
							1.0f))
			]
			+ SOverlay::Slot()
				.VAlign(VAlign_Bottom)
				.Padding(18.0f, 18.0f, 18.0f, 26.0f)
			[
				RuntimeStrip
			];
		SaveP4Image(
			*this,
			Renderer,
			RuntimeRoot,
			FPaths::Combine(
				Directory,
				TEXT("P4RuntimeHotbar1600x900.png")),
			TEXT("Runtime Hotbar HUD model renders to 1600x900"));
	}
	Preparation->RemoveFromRoot();
	return true;
}

#endif
