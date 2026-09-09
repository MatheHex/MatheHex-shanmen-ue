#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapUILayoutPolicy.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr EAutomationTestFlags P7Flags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	FString NewP7Root()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.7.P7.0.r0"),
			FGuid::NewGuid().ToString(
				EGuidFormats::Digits));
	}

	struct FScopedInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedInputConfig()
		{
			Path =
				Fdemo_mapInputBindingSettings::Get()
					.GetConfigPath();
			bExisted =
				IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedInputConfig()
		{
			if (bExisted)
			{
				FFileHelper::SaveArrayToFile(Bytes, *Path);
			}
			else
			{
				IFileManager::Get().Delete(*Path, false, true);
			}
			Fdemo_mapInputBindingSettings::Get().Load();
		}
	};

	struct FP7WidgetFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;

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
			Session = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			if (!Session)
			{
				Test.AddError(TEXT("Profile session unavailable."));
				return false;
			}
			return Session->InitializeSession(
				Fdemo_mapProfileStorageContext::ForRoot(
					NewP7Root())).IsReady();
		}

		Udemo_mapProfilePreparationWidget* MakeWidget(
			FAutomationTestBase& Test)
		{
			Udemo_mapProfilePreparationWidget* Widget =
				NewObject<Udemo_mapProfilePreparationWidget>(
					GameInstance,
					NAME_None,
					RF_Transient);
			if (!Widget || !Widget->Initialize())
			{
				Test.AddError(TEXT("P7 Widget initialization failed."));
				return nullptr;
			}
			Widget->InitializeForSession(Session);
			return Widget;
		}

		~FP7WidgetFixture()
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

	bool SaveP7WidgetImage(
		FAutomationTestBase& Test,
		FWidgetRenderer& Renderer,
		const TSharedRef<SWidget>& Widget,
		const FString& Path,
		FVector2D Size,
		const FString& Label)
	{
		UTextureRenderTarget2D* RenderTarget =
			Renderer.DrawWidget(Widget, Size);
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
	FP7Integration01,
	"demo_map.P7Integration.01.RegistryCoversProductActions",
	P7Flags)
bool FP7Integration01::RunTest(const FString&)
{
	const TArray<Fdemo_mapInputActionDefinition>& Actions =
		Fdemo_mapInputActionRegistry::GetExactDefaultActions();
	TestTrue(
		TEXT("Registry is exact, labeled, and includes Back"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Actions.Num() == 31
			&& Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
			&& Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				->DefaultKey == EKeys::X
			&& Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
			&& Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				->DefaultKey == EKeys::C
			&& Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionIds::SpiritEvasion)
			&& Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionIds::Back)
			&& Fdemo_mapInputActionRegistry::DisplayLabel(
				Fdemo_mapInputActionIds::Interact)
				.Contains(TEXT("搜索"))
			&& Fdemo_mapInputActionRegistry::DisplayLabel(
				Fdemo_mapInputActionIds::Inventory)
				.Contains(TEXT("物品")));
	for (int32 Slot = 1; Slot <= 9; ++Slot)
	{
		TestNotNull(
			*FString::Printf(TEXT("Hotbar %d registered"), Slot),
			Fdemo_mapInputActionRegistry::Find(
				Fdemo_mapInputActionRegistry::HotbarActionId(
					Slot)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration02,
	"demo_map.P7Integration.02.ConflictSwapIsStable",
	P7Flags)
bool FP7Integration02::RunTest(const FString&)
{
	FScopedInputConfig Config;
	Fdemo_mapInputBindingSettings& Settings =
		Fdemo_mapInputBindingSettings::Get();
	Settings.RestoreDefaults();
	const Fdemo_mapInputBindingResult Result =
		Settings.ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::Interact,
			EKeys::W);
	FString ValidationDiagnostic;
	TestTrue(
		TEXT("Conflict swaps target and displaced actions atomically"),
		Result.IsSuccess()
			&& Result.Diagnostic.Contains(TEXT("Conflict resolved"))
			&& Settings.GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::W
			&& Settings.GetKey(
				Fdemo_mapInputActionIds::MoveForward) == EKeys::G);
	TestTrue(
		TEXT("Swapped result remains a valid complete map"),
		Fdemo_mapInputBindingSettings::ValidateBindings(
			Settings.GetBindings(),
			ValidationDiagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration03,
	"demo_map.P7Integration.03.LegacyMissingActionsReceiveDefaults",
	P7Flags)
bool FP7Integration03::RunTest(const FString&)
{
	FScopedInputConfig Config;
	FString Legacy =
		TEXT("[ShanmenInputBindings]\nVersion=1\n");
	for (const Fdemo_mapInputActionDefinition& Action :
		Fdemo_mapInputActionRegistry::GetExactDefaultActions())
	{
		if (Action.ActionId == Fdemo_mapInputActionIds::Back)
		{
			continue;
		}
		Legacy += FString::Printf(
			TEXT("%s=%s\n"),
			*Action.ActionId.ToString(),
			*Action.DefaultKey.GetFName().ToString());
	}
	const FString Path =
		Fdemo_mapInputBindingSettings::Get().GetConfigPath();
	IFileManager::Get().MakeDirectory(
		*FPaths::GetPath(Path),
		true);
	TestTrue(
		TEXT("Legacy fixture written"),
		FFileHelper::SaveStringToFile(Legacy, *Path));
	const Fdemo_mapInputBindingResult Load =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(
		TEXT("Missing newer action is migrated without losing old keys"),
		Load.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get()
				.GetBindings().Num() == 31
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Back)
				== EKeys::BackSpace
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact)
				== EKeys::G);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration04,
	"demo_map.P7Integration.04.ResponsiveCommon16By9Policy",
	P7Flags)
bool FP7Integration04::RunTest(const FString&)
{
	const Fdemo_mapUILayoutProfile Compact =
		Fdemo_mapUILayoutPolicy::Resolve(FIntPoint(1280, 720));
	const Fdemo_mapUILayoutProfile Full =
		Fdemo_mapUILayoutPolicy::Resolve(FIntPoint(1920, 1080));
	TestTrue(
		TEXT("Both target resolutions are supported 16:9"),
		Fdemo_mapUILayoutPolicy::IsSupportedCommon16By9(
			FIntPoint(1280, 720))
			&& Fdemo_mapUILayoutPolicy::IsSupportedCommon16By9(
				FIntPoint(1920, 1080)));
	TestTrue(
		TEXT("Compact and full layouts keep visible grid columns"),
		Compact.bCompact
			&& Compact.UnifiedGridColumns == 4
			&& !Full.bCompact
			&& Full.UnifiedGridColumns == 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration05,
	"demo_map.P7Integration.05.ShopUsesUnifiedSelectableGrids",
	P7Flags)
bool FP7Integration05::RunTest(const FString&)
{
	FP7WidgetFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	Udemo_mapProfilePreparationWidget* Widget =
		Fixture.MakeWidget(*this);
	if (!Widget) return false;
	const Fdemo_mapProfilePreparationViewState& View =
		Widget->GetViewState();
	TestTrue(
		TEXT("Shop and sell inventory are materialized as unified cells"),
		Widget->GetShopButtonCount()
				== View.OrderedShopRows.Num()
			&& Widget->GetShopSellCellCount()
				== View.OrderedPermanentStashRows.Num()
			&& Widget->GetShopGridColumnCount() >= 3);
	if (!View.OrderedShopRows.IsEmpty())
	{
		Widget->HandleShopCellActivated(0);
		TestTrue(
			TEXT("Selecting a shop cell exposes price, stock, and detail"),
			Widget->GetSelectedShopRowIndex() == 0
				&& Widget->GetShopDetailText()
					.Contains(TEXT("购买"))
				&& Widget->GetShopDetailText()
					.Contains(TEXT("库存")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration06,
	"demo_map.P7Integration.06.SellSelectionShowsAuthorityValue",
	P7Flags)
bool FP7Integration06::RunTest(const FString&)
{
	FP7WidgetFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	Udemo_mapProfilePreparationWidget* Widget =
		Fixture.MakeWidget(*this);
	if (!Widget) return false;
	const int32 SellableIndex =
		Widget->GetViewState()
			.OrderedPermanentStashRows.IndexOfByPredicate(
				[](const Fdemo_mapProfilePreparationRowView& Row)
				{
					return Row.bCanSell;
				});
	TestTrue(TEXT("Fresh profile has a sellable authority row"), SellableIndex != INDEX_NONE);
	if (SellableIndex != INDEX_NONE)
	{
		const int64 Expected =
			Widget->GetViewState()
				.OrderedPermanentStashRows[SellableIndex]
				.TotalSellPrice;
		Widget->HandleSellCellActivated(SellableIndex);
		TestTrue(
			TEXT("Sell detail exposes the exact transaction value"),
			Widget->GetSelectedSellRowIndex() == SellableIndex
				&& Widget->GetShopDetailText().Contains(
					FString::Printf(TEXT("%lld"), Expected)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration07,
	"demo_map.P7Integration.07.RegistryDrivenKeySelectorsAndPersistence",
	P7Flags)
bool FP7Integration07::RunTest(const FString&)
{
	FScopedInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FP7WidgetFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	Udemo_mapProfilePreparationWidget* Widget =
		Fixture.MakeWidget(*this);
	if (!Widget) return false;
	TestEqual(
		TEXT("Every registry action owns one key selector"),
		Widget->GetInputSelectorCount(),
		Fdemo_mapInputActionRegistry::
			GetExactDefaultActions().Num());
	Widget->HandleInputKeySelected(
		Fdemo_mapInputActionIds::Interact,
		EKeys::W);
	const Fdemo_mapInputBindingResult Reload =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(
		TEXT("Widget conflict swap is explicit and survives reload"),
		Widget->GetInputBindingDiagnostic().Contains(
			TEXT("Conflict resolved"))
			&& Reload.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact)
				== EKeys::W
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::MoveForward)
				== EKeys::G);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration08,
	"demo_map.P7Integration.08.TradeRefreshKeepsSingleAuthority",
	P7Flags)
bool FP7Integration08::RunTest(const FString&)
{
	FP7WidgetFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	Udemo_mapProfilePreparationWidget* Widget =
		Fixture.MakeWidget(*this);
	if (!Widget) return false;
	const int32 SellableIndex =
		Widget->GetViewState()
			.OrderedPermanentStashRows.IndexOfByPredicate(
				[](const Fdemo_mapProfilePreparationRowView& Row)
				{
					return Row.bCanSell;
				});
	if (SellableIndex == INDEX_NONE)
	{
		AddError(TEXT("No sellable row for trade refresh."));
		return false;
	}
	const FGuid SellId =
		Widget->GetViewState()
			.OrderedPermanentStashRows[SellableIndex]
			.ItemInstanceId;
	const int64 Before =
		Widget->GetViewState().PersistentSpiritStones;
	const Fdemo_mapProfileTradeResult Sold =
		Widget->RequestSell(SellId);
	TestTrue(
		TEXT("Sell commits through ProfileTrade and refreshes all projections"),
		Sold.IsCommitted()
			&& Widget->GetViewState().PersistentSpiritStones
				> Before
			&& !Widget->GetViewState()
				.OrderedPermanentStashRows
				.ContainsByPredicate(
					[SellId](
						const Fdemo_mapProfilePreparationRowView& Row)
					{
						return Row.ItemInstanceId == SellId;
					})
			&& Widget->GetShopSellCellCount()
				== Widget->GetViewState()
					.OrderedPermanentStashRows.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP7Integration09,
	"demo_map.P7Integration.09.VisibleShopAndInputAtTwoResolutions",
	P7Flags)
bool FP7Integration09::RunTest(const FString&)
{
	FString Output;
	if (!FParse::Value(
		FCommandLine::Get(),
		TEXT("P7VisibleOutput="),
		Output))
	{
		AddWarning(
			TEXT("P7VisibleOutput not supplied; structural render path only."));
		return true;
	}
	const FString Directory =
		FPaths::ConvertRelativePathToFull(Output);
	IFileManager::Get().MakeDirectory(*Directory, true);
	FP7WidgetFixture Fixture;
	if (!Fixture.Start(*this)) return false;
	Udemo_mapProfilePreparationWidget* Widget =
		Fixture.MakeWidget(*this);
	if (!Widget) return false;
	Widget->AddToRoot();
	FWidgetRenderer Renderer(true);

	Widget->AutomationShowShopTab();
	const bool bShop = SaveP7WidgetImage(
		*this,
		Renderer,
		Widget->TakeWidget(),
		FPaths::Combine(
			Directory,
			TEXT("P7_Shop_1280x720.png")),
		FVector2D(1280.0f, 720.0f),
		TEXT("Shop grid renders at 1280x720"));

	Widget->AutomationShowInputSettingsTab();
	const bool bInput = SaveP7WidgetImage(
		*this,
		Renderer,
		Widget->TakeWidget(),
		FPaths::Combine(
			Directory,
			TEXT("P7_InputSettings_1920x1080.png")),
		FVector2D(1920.0f, 1080.0f),
		TEXT("Input settings render at 1920x1080"));
	Widget->RemoveFromRoot();
	return bShop && bInput;
}

#endif
