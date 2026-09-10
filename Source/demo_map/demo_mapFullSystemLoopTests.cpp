#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPersistentPreparationTransaction.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "demo_mapProfileTradeTransaction.h"
#include "demo_mapSpiritStoneTransaction.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	struct FP8Root
	{
		FString Path = FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("P8FullSystemLoop"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));

		~FP8Root()
		{
			const FString Full = FPaths::ConvertRelativePathToFull(Path);
			const FString Allowed = FPaths::ConvertRelativePathToFull(
				FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("P8FullSystemLoop")));
			if (Full.StartsWith(Allowed)) IFileManager::Get().DeleteDirectory(*Full, false, true);
		}

		Fdemo_mapProfileStorageContext Storage() const
		{
			return Fdemo_mapProfileStorageContext::ForRoot(Path);
		}
	};

	bool Check(FAutomationTestBase& Test, bool bCondition, const TCHAR* Message)
	{
		if (!bCondition) Test.AddError(Message);
		return bCondition;
	}

	Fdemo_mapPersistentItemRecord AddRecord(
		Fdemo_mapPersistentProfile& Profile,
		FName DefinitionId,
		int32 StackCount = 1)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = StackCount;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item;
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& Out)
	{
		return FFileHelper::LoadFileToArray(Out, *Path);
	}

	bool DowngradeToSchemaTwo(const FString& Path)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *Path)) return false;
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;
		Root->SetNumberField(TEXT("SchemaVersion"), 2);
		Root->RemoveField(TEXT("PreparationLayout"));
		const TSharedPtr<FJsonObject>* Run = nullptr;
		if (!Root->TryGetObjectField(TEXT("ActiveRun"), Run)) return false;
		(*Run)->RemoveField(TEXT("ConsumedSpiritStoneSourceIds"));
		FString Output;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);
		if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer)) return false;
		return FFileHelper::SaveStringToFile(Output, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool MakeFutureSchema(const FString& Path)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *Path)) return false;
		const FString CurrentToken = FString::Printf(
			TEXT("\"SchemaVersion\":%d"),
			Fdemo_mapPersistentProfile::CurrentSchemaVersion);
		const FString FutureToken = FString::Printf(
			TEXT("\"SchemaVersion\":%d"),
			Fdemo_mapPersistentProfile::CurrentSchemaVersion + 1);
		return Json.ReplaceInline(*CurrentToken, *FutureToken) == 1
			&& FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	Fdemo_mapPersistentPreparationLayout StarterLayout(const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapPersistentPreparationLayout Layout;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
		{
			if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade) Layout.WeaponItemInstanceId = Item.ItemInstanceId;
			if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest) Layout.ArmorItemInstanceId = Item.ItemInstanceId;
			if (Item.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman) Layout.SpatialRingItemInstanceId = Item.ItemInstanceId;
		}
		return Layout;
	}

	Fdemo_mapPersistentPreparationCommitResult CommitLayout(
		Fdemo_mapPersistentProfile& Profile,
		const Fdemo_mapPersistentPreparationLayout& Layout,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage)
	{
		Fdemo_mapPersistentPreparationCommitIntent Intent;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.Layout = Layout;
		return Fdemo_mapPersistentPreparationTransaction().Execute(Profile, Intent, Repository, Storage);
	}

	Fdemo_mapBeginRunResult BeginCommitted(
		Fdemo_mapPersistentProfile& Profile,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage)
	{
		Fdemo_mapBeginRunRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		Request.bRequireCommittedPreparationLayout = true;
		return Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
	}

	Fdemo_mapRuntimeSettlementSnapshot ExtractionSnapshot(
		const Fdemo_mapPersistentProfile& Profile,
		const TSet<FGuid>& Excluded = TSet<FGuid>())
	{
		Fdemo_mapRuntimeSettlementSnapshot Snapshot;
		Snapshot.ActiveRunId = Profile.ActiveRun.ActiveRunId;
		Snapshot.CommittedEndReason = Edemo_mapRunEndReason::Extraction;
		Snapshot.bValid = true;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.ActiveRun.ActiveRunItems)
		{
			if (Excluded.Contains(Item.ItemInstanceId)) continue;
			Fdemo_mapRuntimeSettlementItem Runtime;
			Runtime.ItemInstanceId = Item.ItemInstanceId;
			Runtime.ItemDefinitionId = Item.ItemDefinitionId;
			Runtime.StackCount = Item.StackCount;
			Runtime.OriginRunId = Item.OriginRunId;
			Snapshot.OrderedSecuredItems.Add(Runtime);
		}
		return Snapshot;
	}

	Fdemo_mapProfileSettlementResult Settle(
		Fdemo_mapPersistentProfile& Profile,
		Edemo_mapRunEndReason Reason,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage,
		const TOptional<Fdemo_mapRuntimeSettlementSnapshot>& Snapshot = TOptional<Fdemo_mapRuntimeSettlementSnapshot>())
	{
		Fdemo_mapProfileSettlementRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		Request.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
		Request.RequestedEndReason = Reason;
		Request.RuntimeSnapshot = Snapshot;
		return Fdemo_mapProfileSettlementTransaction().Execute(Profile, Request, Repository, Storage);
	}

	Fdemo_mapSpiritStonePickupResult Pickup(
		Fdemo_mapPersistentProfile& Profile,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage,
		TFunctionRef<void(Fdemo_mapSpiritStonePickupIntent&)> Mutate =
			[](Fdemo_mapSpiritStonePickupIntent&) {})
	{
		Fdemo_mapSpiritStonePickupIntent Intent;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
		Mutate(Intent);
		return Fdemo_mapSpiritStoneTransaction().Execute(Profile, Intent, Repository, Storage);
	}

	bool PrepareSimpleRun(
		Fdemo_mapPersistentProfile& Profile,
		Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage,
		Fdemo_mapPersistentPreparationLayout Layout)
	{
		return Repository.SaveProfile(Profile, Storage).IsSuccess()
			&& CommitLayout(Profile, Layout, Repository, Storage).IsSuccess()
			&& BeginCommitted(Profile, Repository, Storage).IsCommitted();
	}

	bool RunCase(int32 Index, FAutomationTestBase& Test)
	{
		FP8Root Root;
		Fdemo_mapProfileRepository Repository;
		const Fdemo_mapProfileStorageContext Storage = Root.Storage();

		switch (Index)
		{
		case 1:
		{
			const Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			return Check(Test,
				Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
				&& Profile.PreparationLayout.IsEmpty()
				&& Profile.PreparationLayout.HotbarItemInstanceIds.Num() == 9,
				TEXT("Fresh current-Schema default is not exact."));
		}
		case 2:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			if (!Repository.SaveProfile(Profile, Storage).IsSuccess()) return Check(Test, false, TEXT("Fixture save failed."));
			const int32 Before = Profile.SaveGeneration;
			if (!DowngradeToSchemaTwo(Storage.PrimaryPath())) return Check(Test, false, TEXT("Schema 2 fixture failed."));
			const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage);
			return Check(Test, Load.IsSuccess()
				&& Load.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
				&& Load.Profile.SaveGeneration == Before + 1 && Load.Profile.PreparationLayout.IsEmpty(), TEXT("Idle Schema 2 migration failed."));
		}
		case 3:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Fdemo_mapPersistentItemRecord Item = Profile.PermanentStash[0];
			Profile.PermanentStash.RemoveAt(0);
			Profile.ActiveRun.bHasActiveRun = true;
			Profile.ActiveRun.ActiveRunId = FGuid::NewGuid();
			Profile.ActiveRun.ActiveRunState = Edemo_mapPersistentActiveRunState::Prepared;
			Item.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
			Item.EquipmentSlotId = Fdemo_mapItemIds::WeaponSlot;
			Profile.ActiveRun.ActiveRunItems.Add(Item);
			Profile.ActiveRun.DeployedItemIds.Add(Item.ItemInstanceId);
			if (!Repository.SaveProfile(Profile, Storage).IsSuccess() || !DowngradeToSchemaTwo(Storage.PrimaryPath()))
				return Check(Test, false, TEXT("Active Schema 2 fixture failed."));
			const FGuid RunId = Profile.ActiveRun.ActiveRunId;
			const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage);
			return Check(Test, Load.IsSuccess() && Load.Profile.ActiveRun.ActiveRunId == RunId
				&& Load.Profile.ActiveRun.ActiveRunItems[0].ItemInstanceId == Item.ItemInstanceId
				&& Load.Profile.ActiveRun.ConsumedSpiritStoneSourceIds.IsEmpty(), TEXT("Active Schema 2 migration failed."));
		}
		case 4:
		{
			const Fdemo_mapProfileLoadResult Created = Repository.LoadOrCreateDefaultProfile(Storage);
			TArray<uint8> Before, After;
			ReadBytes(Storage.PrimaryPath(), Before);
			const Fdemo_mapProfileLoadResult A = Repository.LoadExistingProfile(Storage);
			const Fdemo_mapProfileLoadResult B = Repository.LoadExistingProfile(Storage);
			ReadBytes(Storage.PrimaryPath(), After);
			return Check(Test, Created.IsSuccess() && A.IsSuccess() && B.IsSuccess()
				&& Before == After && A.Profile == B.Profile, TEXT("Current-Schema pure reload changed bytes."));
		}
		case 5:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Repository.SaveProfile(Profile, Storage);
			MakeFutureSchema(Storage.PrimaryPath());
			TArray<uint8> Before, After; ReadBytes(Storage.PrimaryPath(), Before);
			const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage);
			ReadBytes(Storage.PrimaryPath(), After);
			return Check(Test, Load.Status == Edemo_mapProfileLoadStatus::FutureSchemaRejected && Before == After, TEXT("Future schema was not read-only rejected."));
		}
		case 6:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Profile.PreparationLayout.HotbarItemInstanceIds.SetNum(8);
			return Check(Test, Repository.ValidateProfile(Profile), TEXT("Retired preparation metadata still blocks otherwise valid Profiles."));
		}
		case 7:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const Fdemo_mapPersistentItemRecord A = AddRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2);
			const Fdemo_mapPersistentItemRecord B = AddRecord(Profile, Fdemo_mapItemIds::IronShard, 3);
			Repository.SaveProfile(Profile, Storage);
			Fdemo_mapPersistentPreparationLayout Layout;
			Layout.OrderedRunInventoryItemInstanceIds = { B.ItemInstanceId, A.ItemInstanceId };
			const auto Commit = CommitLayout(Profile, Layout, Repository, Storage);
			const auto Load = Repository.LoadExistingProfile(Storage);
			return Check(Test, Commit.IsSuccess() && Load.Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds == Layout.OrderedRunInventoryItemInstanceIds, TEXT("Persistent layout order changed."));
		}
		case 8:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Backpack = AddRecord(Profile, Fdemo_mapItemIds::BackpackLevel2);
			Fdemo_mapPersistentPreparationLayout Layout = StarterLayout(Profile);
			Layout.BackpackItemInstanceId = Backpack.ItemInstanceId;
			Profile.PreparationLayout = Layout;
			return Check(Test, Repository.ValidateProfile(Profile), TEXT("Current equipment compatibility rejected."));
		}
		case 9:
		{
			const Fdemo_mapInventoryCapacityResult Base =
				Fdemo_mapItemDefinitions::ResolveInventoryCapacity(NAME_None);
			const Fdemo_mapInventoryCapacityResult Level1 =
				Fdemo_mapItemDefinitions::ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel1);
			const Fdemo_mapInventoryCapacityResult Level2 =
				Fdemo_mapItemDefinitions::ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel2);
			return Check(Test,
				Base.bSuccess && Level1.bSuccess && Level2.bSuccess
				&& Base.Capacity == 6
				&& Level1.Capacity == 42
				&& Level2.Capacity == 42,
				TEXT("Current carried capacities are not base 6 and backpack 42/42."));
		}
		case 10:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Material = AddRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2);
			const auto Consumable = AddRecord(Profile, Fdemo_mapItemIds::HealingPillLevel1, 1);
			const auto Core = AddRecord(Profile, Fdemo_mapItemIds::InnerCoreLevel10, 1);
			Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds = { Material.ItemInstanceId, Consumable.ItemInstanceId };
			const bool bGood = Repository.ValidateProfile(Profile);
			Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds.Add(Core.ItemInstanceId);
			return Check(Test, bGood && Repository.ValidateProfile(Profile), TEXT("Retired RunInventory metadata still affects Profile validity."));
		}
		case 11:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Stack = AddRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5);
			Fdemo_mapPersistentPreparationLayout Layout;
			Layout.OrderedRunInventoryItemInstanceIds.Add(Stack.ItemInstanceId);
			if (!PrepareSimpleRun(Profile, Repository, Storage, Layout)) return Check(Test, false, TEXT("Run fixture failed."));
			const auto* Deployed = Profile.ActiveRun.ActiveRunItems.FindByPredicate([&](const auto& I){ return I.ItemInstanceId == Stack.ItemInstanceId; });
			return Check(Test, Deployed && Deployed->StackCount == 5, TEXT("Whole stack identity was split or copied."));
		}
		case 12:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			return Check(Test, Profile.PreparationLayout.HotbarItemInstanceIds.Num() == 9
				&& !Profile.PreparationLayout.HotbarItemInstanceIds.ContainsByPredicate([](const FGuid& Id){ return Id.IsValid(); }),
				TEXT("Hotbar is not exactly nine persisted empty slots."));
		}
		case 13:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Pill = AddRecord(Profile, Fdemo_mapItemIds::HealingPillLevel1);
			Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds.Add(Pill.ItemInstanceId);
			Profile.PreparationLayout.HotbarItemInstanceIds[0] = Pill.ItemInstanceId;
			Profile.PreparationLayout.HotbarItemInstanceIds[1] = Pill.ItemInstanceId;
			return Check(Test, Repository.ValidateProfile(Profile), TEXT("Retired Hotbar metadata still affects Profile validity."));
		}
		case 14:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Pill = AddRecord(Profile, Fdemo_mapItemIds::HealingPillLevel1);
			Profile.PreparationLayout.HotbarItemInstanceIds[0] = Pill.ItemInstanceId;
			return Check(Test, Repository.ValidateProfile(Profile), TEXT("Retired Hotbar metadata still affects Profile validity."));
		}
		case 15:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Repository.SaveProfile(Profile, Storage);
			const int32 Generation = Profile.SaveGeneration;
			TArray<uint8> Before, After; ReadBytes(Storage.PrimaryPath(), Before);
			const auto Result = CommitLayout(Profile, Profile.PreparationLayout, Repository, Storage);
			ReadBytes(Storage.PrimaryPath(), After);
			return Check(Test, Result.Status == Edemo_mapPersistentPreparationCommitStatus::NoOp
				&& Profile.SaveGeneration == Generation && Before == After, TEXT("Layout no-op changed generation or bytes."));
		}
		case 16:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Repository.SaveProfile(Profile, Storage);
			const int32 Before = Profile.SaveGeneration;
			Fdemo_mapPersistentPreparationLayout Layout = StarterLayout(Profile);
			const auto Result = CommitLayout(Profile, Layout, Repository, Storage);
			return Check(Test, Result.Status == Edemo_mapPersistentPreparationCommitStatus::Committed
				&& Profile.SaveGeneration == Before + 1, TEXT("Layout commit did not increment once."));
		}
		case 17:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Repository.SaveProfile(Profile, Storage);
			const Fdemo_mapPersistentProfile Before = Profile;
			Fdemo_mapProfileStorageContext Failing = Storage;
			Failing.InjectedFailure = Edemo_mapProfileFailureStage::WriteTemp;
			const auto Result = CommitLayout(Profile, StarterLayout(Profile), Repository, Failing);
			return Check(Test, !Result.IsSuccess() && Profile == Before, TEXT("Layout precommit failure was not atomic."));
		}
		case 18:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Repository.SaveProfile(Profile, Storage);
			const Fdemo_mapPersistentPreparationLayout Layout = StarterLayout(Profile);
			Fdemo_mapProfileStorageContext Failing = Storage;
			Failing.InjectedFailure = Edemo_mapProfileFailureStage::ReadBackCommittedPrimary;
			const auto Result = CommitLayout(Profile, Layout, Repository, Failing);
			const auto Reload = Repository.LoadExistingProfile(Storage);
			return Check(Test, Result.Status == Edemo_mapPersistentPreparationCommitStatus::CommitOutcomeRequiresReload
				&& Reload.IsSuccess() && Reload.Profile.PreparationLayout == Layout, TEXT("Layout postcommit reconcile state was not durable."));
		}
		case 19:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Repository.SaveProfile(Profile, Storage);
			const Fdemo_mapPersistentPreparationLayout Layout = StarterLayout(Profile);
			CommitLayout(Profile, Layout, Repository, Storage);
			Fdemo_mapBeginRunRequest Request;
			Request.ExpectedProfileId = Profile.ProfileId;
			Request.ExpectedSaveGeneration = Profile.SaveGeneration;
			Request.Loadout = Fdemo_mapLoadoutSelection();
			Request.bRequireCommittedPreparationLayout = true;
			const auto Begin = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
			return Check(Test, Begin.IsCommitted() && Profile.ActiveRun.DeployedItemIds.Num() == 3, TEXT("BeginRun did not use committed layout."));
		}
		case 20:
		case 21:
		case 22:
		case 23:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Backpack = AddRecord(Profile, Fdemo_mapItemIds::BackpackLevel2);
			const auto Pill = AddRecord(Profile, Fdemo_mapItemIds::HealingPillLevel1);
			Fdemo_mapPersistentPreparationLayout Layout;
			Layout.BackpackItemInstanceId = Backpack.ItemInstanceId;
			Layout.OrderedRunInventoryItemInstanceIds.Add(Pill.ItemInstanceId);
			Layout.HotbarItemInstanceIds[0] = Pill.ItemInstanceId;
			if (!PrepareSimpleRun(Profile, Repository, Storage, Layout)) return Check(Test, false, TEXT("Runtime fixture failed."));
			Udemo_mapItemSubsystem* Items = NewObject<Udemo_mapItemSubsystem>(
				NewObject<UGameInstance>(GetTransientPackage()));
			Items->ResetForAutomation();
			if (Index == 23) Items->SetPreparedRunFailureAfterMutationForAutomation(1);
			Fdemo_mapPreparedRunRuntimeRequest RuntimeRequest;
			RuntimeRequest.CommittedPlan.ProfileId = Profile.ProfileId;
			RuntimeRequest.CommittedPlan.CommittedGeneration = Profile.SaveGeneration;
			RuntimeRequest.CommittedPlan.ActiveRunId = Profile.ActiveRun.ActiveRunId;
			RuntimeRequest.CommittedPlan.OrderedItems = Profile.ActiveRun.ActiveRunItems;
			RuntimeRequest.CommittedPlan.DeployedItemIds = Profile.ActiveRun.DeployedItemIds;
			RuntimeRequest.CommittedPlan.HotbarItemInstanceIds = Profile.PreparationLayout.HotbarItemInstanceIds;
			const Fdemo_mapInventoryCapacityResult Capacity =
				Fdemo_mapItemDefinitions::ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel2);
			if (!Check(Test, Capacity.bSuccess, TEXT("Runtime capacity fixture failed to resolve."))) return false;
			RuntimeRequest.CommittedPlan.RunInventoryCapacity = Capacity.Capacity;
			const auto Runtime = Items->MaterializePreparedRun(RuntimeRequest);
			if (Index == 23)
				return Check(Test, !Runtime.IsMaterialized() && Items->GetRunState() == Edemo_mapRunState::Inactive
					&& Items->GetAuthority().GetInstanceSnapshot().IsEmpty(), TEXT("BeginRun Runtime failure did not fully roll back."));
			if (!Runtime.IsMaterialized()) return Check(Test, false, TEXT("Runtime materialization failed."));
			if (Index == 20)
				return Check(Test, Items->GetAuthority().FindInstance(Backpack.ItemInstanceId)
					&& Items->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::BackpackSlot) == Backpack.ItemInstanceId, TEXT("Backpack GUID changed."));
			if (Index == 21)
				return Check(Test, Items->GetAuthority().FindInstance(Pill.ItemInstanceId)
					&& Items->GetAuthority().FindInstance(Pill.ItemInstanceId)->Quantity == 1, TEXT("RunInventory GUID changed."));
			return Check(Test, Items->GetHotbarBindingSnapshot().SlotBindings[0] == Pill.ItemInstanceId, TEXT("Runtime Hotbar GUID changed."));
		}
		case 24:
		case 25:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Pill = AddRecord(Profile, Fdemo_mapItemIds::HealingPillLevel1);
			Fdemo_mapPersistentPreparationLayout Layout;
			Layout.OrderedRunInventoryItemInstanceIds.Add(Pill.ItemInstanceId);
			Layout.HotbarItemInstanceIds[0] = Pill.ItemInstanceId;
			if (!PrepareSimpleRun(Profile, Repository, Storage, Layout)) return Check(Test, false, TEXT("Settlement fixture failed."));
			TSet<FGuid> Excluded;
			if (Index == 25) Excluded.Add(Pill.ItemInstanceId);
			const auto Result = Settle(Profile, Edemo_mapRunEndReason::Extraction, Repository, Storage, ExtractionSnapshot(Profile, Excluded));
			const bool bRetained = Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds.Contains(Pill.ItemInstanceId);
			return Check(Test, Result.IsCommitted() && bRetained == (Index == 24)
				&& Profile.PreparationLayout.HotbarItemInstanceIds[0].IsValid() == (Index == 24), TEXT("Extraction layout reconciliation failed."));
		}
		case 26:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Fdemo_mapPersistentPreparationLayout Layout = StarterLayout(Profile);
			if (!PrepareSimpleRun(Profile, Repository, Storage, Layout)) return Check(Test, false, TEXT("Death fixture failed."));
			Fdemo_mapRuntimeSettlementSnapshot Snapshot;
			Snapshot.ActiveRunId = Profile.ActiveRun.ActiveRunId;
			Snapshot.CommittedEndReason = Edemo_mapRunEndReason::Death;
			Snapshot.bValid = true;
			const auto Result = Settle(Profile, Edemo_mapRunEndReason::Death, Repository, Storage, Snapshot);
			return Check(Test, Result.IsCommitted() && Profile.PreparationLayout.IsEmpty(), TEXT("Death did not clear lost layout references."));
		}
		case 27:
			return Check(Test, Fdemo_mapSpiritStoneRules::FixedWorldPickupValueForFutureTasks == 20
				&& Fdemo_mapSpiritStonePickupContract::Value == 20
				&& Fdemo_mapSpiritStonePickupContract::PickupId == FName(TEXT("P8.SpiritStone.Main.Fixed20")),
				TEXT("Fixed Spirit Stone rules are wrong."));
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			if (!PrepareSimpleRun(Profile, Repository, Storage, Fdemo_mapPersistentPreparationLayout()))
				return Check(Test, false, TEXT("Spirit Stone fixture failed."));
			if (Index == 30)
			{
				const auto Wrong = Pickup(Profile, Repository, Storage, [](auto& Intent){ Intent.ExpectedActiveRunId = FGuid::NewGuid(); });
				const auto Stale = Pickup(Profile, Repository, Storage, [](auto& Intent){ --Intent.ExpectedSaveGeneration; });
				return Check(Test, Wrong.Status == Edemo_mapSpiritStonePickupStatus::RunIdMismatch
					&& Stale.Status == Edemo_mapSpiritStonePickupStatus::ProfileGenerationMismatch, TEXT("Wrong run or stale pickup was accepted."));
			}
			if (Index == 31)
			{
				Profile.ActiveRun.RiskSpiritStones = MAX_int64 - 10;
				const auto Overflow = Pickup(Profile, Repository, Storage);
				return Check(Test, Overflow.Status == Edemo_mapSpiritStonePickupStatus::CurrencyOverflow
					&& Profile.ActiveRun.RiskSpiritStones == MAX_int64 - 10, TEXT("Pickup overflow was not atomic."));
			}
			if (Index == 32)
			{
				const Fdemo_mapPersistentProfile Before = Profile;
				Fdemo_mapProfileStorageContext Failing = Storage;
				Failing.InjectedFailure = Edemo_mapProfileFailureStage::WriteTemp;
				const auto Result = Pickup(Profile, Repository, Failing);
				return Check(Test, !Result.IsCommitted() && Profile == Before, TEXT("Pickup precommit failure was not atomic."));
			}
			if (Index == 33)
			{
				Fdemo_mapProfileStorageContext Failing = Storage;
				Failing.InjectedFailure = Edemo_mapProfileFailureStage::ReadBackCommittedPrimary;
				const auto Ambiguous = Pickup(Profile, Repository, Failing);
				const auto Reload = Repository.LoadExistingProfile(Storage);
				Fdemo_mapPersistentProfile Reconciled = Reload.Profile;
				const auto Replay = Pickup(Reconciled, Repository, Storage);
				return Check(Test, Ambiguous.Status == Edemo_mapSpiritStonePickupStatus::CommitOutcomeRequiresReload
					&& Reload.Profile.ActiveRun.RiskSpiritStones == 20
					&& Replay.Status == Edemo_mapSpiritStonePickupStatus::DuplicateSource, TEXT("Postcommit pickup replay was possible."));
			}
			const auto First = Pickup(Profile, Repository, Storage);
			if (Index == 28)
				return Check(Test, First.IsCommitted() && Profile.ActiveRun.RiskSpiritStones == 20
					&& Profile.PersistentSpiritStones == 0 && Profile.ActiveRun.ConsumedSpiritStoneSourceIds.Num() == 1, TEXT("Pickup success state is wrong."));
			const auto Duplicate = Pickup(Profile, Repository, Storage);
			return Check(Test, Duplicate.Status == Edemo_mapSpiritStonePickupStatus::DuplicateSource
				&& Profile.ActiveRun.RiskSpiritStones == 20, TEXT("Duplicate source changed Risk."));
		}
		case 34:
		case 35:
		case 36:
		case 37:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Profile.PersistentSpiritStones = 5;
			if (!PrepareSimpleRun(Profile, Repository, Storage, Fdemo_mapPersistentPreparationLayout())
				|| !Pickup(Profile, Repository, Storage).IsCommitted())
				return Check(Test, false, TEXT("Currency settlement fixture failed."));
			const Edemo_mapRunEndReason Reason = Index == 34 ? Edemo_mapRunEndReason::Extraction
				: (Index == 35 ? Edemo_mapRunEndReason::Death
					: (Index == 36 ? Edemo_mapRunEndReason::Abandon : Edemo_mapRunEndReason::RecoveredAbandon));
			TOptional<Fdemo_mapRuntimeSettlementSnapshot> Snapshot;
			if (Reason != Edemo_mapRunEndReason::RecoveredAbandon)
			{
				Fdemo_mapRuntimeSettlementSnapshot Value;
				Value.ActiveRunId = Profile.ActiveRun.ActiveRunId;
				Value.CommittedEndReason = Reason;
				Value.bValid = true;
				Snapshot = Value;
			}
			const auto Result = Settle(Profile, Reason, Repository, Storage, Snapshot);
			const int64 ExpectedPersistent = Reason == Edemo_mapRunEndReason::Extraction ? 25 : 5;
			return Check(Test, Result.IsCommitted() && Profile.PersistentSpiritStones == ExpectedPersistent
				&& Profile.ActiveRun.RiskSpiritStones == 0 && Profile.ActiveRun.ConsumedSpiritStoneSourceIds.IsEmpty(), TEXT("Unified settlement currency result is wrong."));
		}
		case 38:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			if (!PrepareSimpleRun(Profile, Repository, Storage, Fdemo_mapPersistentPreparationLayout())
				|| !Pickup(Profile, Repository, Storage).IsCommitted())
				return Check(Test, false, TEXT("First-event fixture failed."));
			const FGuid RunId = Profile.ActiveRun.ActiveRunId;
			Fdemo_mapRuntimeSettlementSnapshot Snapshot;
			Snapshot.ActiveRunId = RunId; Snapshot.CommittedEndReason = Edemo_mapRunEndReason::Death; Snapshot.bValid = true;
			const auto First = Settle(Profile, Edemo_mapRunEndReason::Death, Repository, Storage, Snapshot);
			Fdemo_mapProfileSettlementRequest Retry;
			Retry.ExpectedProfileId = Profile.ProfileId;
			Retry.ExpectedSaveGeneration = Profile.SaveGeneration;
			Retry.ExpectedActiveRunId = RunId;
			Retry.RequestedEndReason = Edemo_mapRunEndReason::Extraction;
			Retry.RuntimeSnapshot = ExtractionSnapshot(Profile);
			const auto Second = Fdemo_mapProfileSettlementTransaction().Execute(Profile, Retry, Repository, Storage);
			return Check(Test, First.IsCommitted() && Second.Status == Edemo_mapProfileSettlementStatus::AlreadyCommitted
				&& Profile.PersistentSpiritStones == 0, TEXT("Settlement first-event-wins currency failed."));
		}
		case 39:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Profile.PersistentSpiritStones = MAX_int64 - 5;
			if (!PrepareSimpleRun(Profile, Repository, Storage, Fdemo_mapPersistentPreparationLayout()))
				return Check(Test, false, TEXT("Overflow settlement fixture failed."));
			Profile.ActiveRun.RiskSpiritStones = 20;
			const Fdemo_mapPersistentProfile Before = Profile;
			const auto Result = Settle(Profile, Edemo_mapRunEndReason::Extraction, Repository, Storage, ExtractionSnapshot(Profile));
			return Check(Test, Result.Status == Edemo_mapProfileSettlementStatus::CurrencyOverflow && Profile == Before, TEXT("Settlement overflow did not fully roll back."));
		}
		case 40:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Profile.PersistentSpiritStones = 7;
			if (!PrepareSimpleRun(Profile, Repository, Storage, Fdemo_mapPersistentPreparationLayout())
				|| !Pickup(Profile, Repository, Storage).IsCommitted())
				return Check(Test, false, TEXT("Summary fixture failed."));
			const auto Result = Settle(Profile, Edemo_mapRunEndReason::Extraction, Repository, Storage, ExtractionSnapshot(Profile));
			return Check(Test, Result.RiskBefore == 20 && Result.RiskTransferred == 20 && Result.RiskLost == 0
				&& Result.PersistentBefore == 7 && Result.PersistentAfter == 27, TEXT("Settlement currency summary is not exact."));
		}
		case 41:
			return Check(Test, Fdemo_mapInputActionRegistry::ValidateExactDefaults()
				&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num() == 33
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::SpiritShield)->DefaultKey == EKeys::H
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)->DefaultKey == EKeys::X
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::ControlledWeaponRedirect)->DefaultKey == EKeys::C
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::SpiritEvasion)->DefaultKey == EKeys::SpaceBar
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::WeaponGuard)->DefaultKey == EKeys::RightMouseButton
				&& Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::Interact)->DefaultKey == EKeys::G,
				TEXT("Input registry exact defaults failed."));
		case 42:
		{
			TMap<FName, FKey> Bindings;
			for (const auto& Action : Fdemo_mapInputActionRegistry::GetExactDefaultActions()) Bindings.Add(Action.ActionId, Action.DefaultKey);
			Bindings[Fdemo_mapInputActionIds::Interact] = EKeys::W;
			FString Error;
			const bool bDuplicateRejected = !Fdemo_mapInputBindingSettings::ValidateBindings(Bindings, Error);
			const bool bReservedRejected = !Fdemo_mapInputBindingSettings::ValidateKey(EKeys::Escape, Error);
			return Check(Test, bDuplicateRejected && bReservedRejected, TEXT("Duplicate or reserved key was accepted."));
		}
		case 43:
		{
			Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
			Settings.RestoreDefaults();
			const auto Applied = Settings.ApplyOverride(Fdemo_mapInputActionIds::Interact, EKeys::K);
			const bool bGood = Applied.IsSuccess() && Settings.GetKey(Fdemo_mapInputActionIds::Interact) == EKeys::K;
			Settings.RestoreDefaults();
			return Check(Test, bGood, TEXT("Input remap did not apply immediately in memory."));
		}
		case 44:
		{
			Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
			Settings.RestoreDefaults();
			Settings.ApplyOverride(Fdemo_mapInputActionIds::Interact, EKeys::K);
			const auto* Interact = Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::Interact);
			const bool bGood = Interact && Interact->bRequiresReleasedEvent
				&& Settings.GetKey(Fdemo_mapInputActionIds::Interact) == EKeys::K;
			Settings.RestoreDefaults();
			return Check(Test, bGood, TEXT("Remapped Interact lost Pressed/Released contract."));
		}
		case 45:
		{
			Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
			Settings.RestoreDefaults();
			const auto Applied = Settings.ApplyOverride(Fdemo_mapInputActionIds::Hotbar1, EKeys::J);
			const bool bGood = Applied.IsSuccess() && Settings.GetKey(Fdemo_mapInputActionRegistry::HotbarActionId(1)) == EKeys::J;
			Settings.RestoreDefaults();
			return Check(Test, bGood, TEXT("Hotbar action remap failed."));
		}
		case 46:
		{
			Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
			Settings.RestoreDefaults();
			Settings.ApplyOverride(Fdemo_mapInputActionIds::Interact, EKeys::K);
			const auto Reload = Settings.Load();
			const bool bGood = Reload.IsSuccess() && Settings.GetKey(Fdemo_mapInputActionIds::Interact) == EKeys::K;
			Settings.RestoreDefaults();
			return Check(Test, bGood, TEXT("Input settings reload persistence failed."));
		}
		case 47:
		{
			Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
			Settings.ApplyOverride(Fdemo_mapInputActionIds::Interact, EKeys::K);
			const auto Restored = Settings.RestoreDefaults();
			return Check(Test, Restored.IsSuccess() && Settings.GetKey(Fdemo_mapInputActionIds::Interact) == EKeys::G
				&& Settings.GetBindings().Num() == 33, TEXT("Restore defaults was not exact."));
		}
		case 48:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			const auto Backpack = AddRecord(Profile, Fdemo_mapItemIds::BackpackLevel2);
			const auto Pill = AddRecord(Profile, Fdemo_mapItemIds::HealingPillLevel1);
			Fdemo_mapProfilePreparationSnapshot Snapshot;
			Snapshot.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation;
			Snapshot.ProfileId = Profile.ProfileId;
			Snapshot.PersistentSpiritStones = 20;
			Snapshot.SelectedBackpackId = Backpack.ItemInstanceId;
			Snapshot.OrderedSelectedMaterialIds.Add(Pill.ItemInstanceId);
			Snapshot.HotbarBindings.SlotBindings[0] = Pill.ItemInstanceId;
			for (const auto& Item : Profile.PermanentStash)
			{
				Fdemo_mapProfilePreparationStashRow Row;
				Row.ItemInstanceId = Item.ItemInstanceId; Row.ItemDefinitionId = Item.ItemDefinitionId; Row.StackCount = Item.StackCount; Row.bSafeInPermanentStash = true;
				Snapshot.OrderedPermanentStashRows.Add(Row);
			}
			for (const auto& Item : { Backpack, Pill })
			{
				Fdemo_mapProfilePreparationStashRow Row;
				Row.ItemInstanceId = Item.ItemInstanceId; Row.ItemDefinitionId = Item.ItemDefinitionId; Row.StackCount = Item.StackCount; Row.bSafeInPermanentStash = true;
				Snapshot.OrderedPermanentStashRows.Add(Row);
			}
			const auto View = Fdemo_mapProfilePreparationPresenter::BuildViewState(Snapshot);
			const Fdemo_mapInventoryCapacityResult ExpectedCapacity =
				Fdemo_mapItemDefinitions::ResolveInventoryCapacity(Fdemo_mapItemIds::BackpackLevel2);
			const TArray<FName>& ExpectedSlotIds = Fdemo_mapItemDefinitions::GetEquipmentSlotIds();
			bool bEquipmentSlotsMatch = View.OrderedEquipmentSlots.Num() == ExpectedSlotIds.Num();
			for (int32 SlotIndex = 0; bEquipmentSlotsMatch && SlotIndex < ExpectedSlotIds.Num(); ++SlotIndex)
			{
				bEquipmentSlotsMatch = View.OrderedEquipmentSlots[SlotIndex].SlotId == ExpectedSlotIds[SlotIndex];
			}
			return Check(Test, ExpectedCapacity.bSuccess
				&& View.PersistentSpiritStones == 20
				&& View.RunInventoryCapacity == ExpectedCapacity.Capacity
				&& View.HotbarBindings.SlotBindings[0] == Pill.ItemInstanceId
				&& bEquipmentSlotsMatch, TEXT("Preparation integrated view refresh failed."));
		}
		case 49:
		{
			Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
			Profile.PersistentSpiritStones = 20;
			const auto Core = AddRecord(Profile, Fdemo_mapItemIds::InnerCoreLevel10);
			const Fdemo_mapPersistentShopStockEntry* PillStock =
				Profile.ShopStock.Entries.FindByPredicate([](const auto& Entry)
				{
					return Entry.State == Edemo_mapPersistentShopStockEntryState::Available
						&& Entry.Item.ItemDefinitionId == Fdemo_mapItemIds::HealingPillLevel1;
				});
			if (!Check(Test, PillStock != nullptr, TEXT("Guaranteed Level1 Healing Pill stock was absent.")))
			{
				return false;
			}
			const FGuid PillStockId = PillStock->Item.ItemInstanceId;
			const FName PillSlotId = PillStock->SlotId;
			Repository.SaveProfile(Profile, Storage);
			Fdemo_mapProfileTradeIntent Sell;
			Sell.Kind = Edemo_mapProfileTradeKind::Sell;
			Sell.ExpectedProfileId = Profile.ProfileId;
			Sell.ExpectedSaveGeneration = Profile.SaveGeneration;
			Sell.ItemInstanceId = Core.ItemInstanceId;
			const auto Sold = Fdemo_mapProfileTradeTransaction().Execute(Profile, Sell, {}, Repository, Storage).Result;
			Fdemo_mapProfileTradeIntent Buy;
			Buy.Kind = Edemo_mapProfileTradeKind::Buy;
			Buy.ExpectedProfileId = Profile.ProfileId;
			Buy.ExpectedSaveGeneration = Profile.SaveGeneration;
			Buy.ItemInstanceId = PillStockId;
			Buy.ShopStockPolicyId = Profile.ShopStock.PolicyId;
			Buy.ShopStockGeneration = Profile.ShopStock.Generation;
			Buy.ShopStockEventId = Profile.ShopStock.ShopStockEventId;
			Buy.ShopSlotId = PillSlotId;
			Buy.ExpectedBuyValue = 30;
			const auto Bought = Fdemo_mapProfileTradeTransaction().Execute(Profile, Buy, {}, Repository, Storage).Result;
			return Check(Test, Sold.IsCommitted() && Bought.IsCommitted() && Sold.BalanceAfter == 420
				&& Bought.BalanceAfter == 390 && Bought.ItemInstanceId == PillStockId
				&& Bought.ItemInstanceId != Core.ItemInstanceId
				&& Profile.ShopStock.Entries.ContainsByPredicate([&](const auto& Entry)
					{
						return Entry.SlotId == PillSlotId
							&& Entry.State == Edemo_mapPersistentShopStockEntryState::Sold
							&& Entry.SoldItemInstanceId == PillStockId;
					})
				&& !Profile.PermanentStash.ContainsByPredicate([&](const auto& I){ return I.ItemInstanceId == Core.ItemInstanceId; }),
				TEXT("End-to-end trade balance or identity failed."));
		}
		case 50:
		{
			const Fdemo_mapPersistentProfile Profile;
			return Check(Test,
				FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), TEXT("demo_map.uproject")))
				&& !FPaths::DirectoryExists(FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins")))
				&& Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion,
				TEXT("Protected scope or cleanup invariant failed."));
		}
		default:
			return Check(Test, false, TEXT("Unknown P8 test case."));
		}
	}
}

#define DEMO_MAP_P8_TEST(ClassName, Number, Suffix) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, "demo_map.FullSystemLoop." #Number "." Suffix, EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString&) { return RunCase(Number, *this); }

DEMO_MAP_P8_TEST(FP8FullSystemLoop01, 1, "FreshCurrentSchema")
DEMO_MAP_P8_TEST(FP8FullSystemLoop02, 2, "SchemaTwoIdleMigrationToCurrent")
DEMO_MAP_P8_TEST(FP8FullSystemLoop03, 3, "SchemaTwoActiveRunMigration")
DEMO_MAP_P8_TEST(FP8FullSystemLoop04, 4, "CurrentSchemaReloadIdempotent")
DEMO_MAP_P8_TEST(FP8FullSystemLoop05, 5, "FutureSchemaReadOnlyReject")
DEMO_MAP_P8_TEST(FP8FullSystemLoop06, 6, "RetiredPreparationMetadataIgnored")
DEMO_MAP_P8_TEST(FP8FullSystemLoop07, 7, "PersistentLayoutStableOrder")
DEMO_MAP_P8_TEST(FP8FullSystemLoop08, 8, "CurrentEquipmentCompatibility")
DEMO_MAP_P8_TEST(FP8FullSystemLoop09, 9, "CurrentInventoryCapacityContract")
DEMO_MAP_P8_TEST(FP8FullSystemLoop10, 10, "RunInventoryMaterialConsumableOnly")
DEMO_MAP_P8_TEST(FP8FullSystemLoop11, 11, "RunInventoryWholeStackIdentity")
DEMO_MAP_P8_TEST(FP8FullSystemLoop12, 12, "HotbarExactlyNinePersistedSlots")
DEMO_MAP_P8_TEST(FP8FullSystemLoop13, 13, "HotbarDuplicateReject")
DEMO_MAP_P8_TEST(FP8FullSystemLoop14, 14, "HotbarRequiresSelectedConsumable")
DEMO_MAP_P8_TEST(FP8FullSystemLoop15, 15, "LayoutNoOpGenerationStable")
DEMO_MAP_P8_TEST(FP8FullSystemLoop16, 16, "LayoutCommitGenerationOnce")
DEMO_MAP_P8_TEST(FP8FullSystemLoop17, 17, "LayoutPrecommitFailureAtomic")
DEMO_MAP_P8_TEST(FP8FullSystemLoop18, 18, "LayoutPostcommitReloadReconcile")
DEMO_MAP_P8_TEST(FP8FullSystemLoop19, 19, "BeginRunUsesCommittedLayout")
DEMO_MAP_P8_TEST(FP8FullSystemLoop20, 20, "BeginRunBackpackSameGuid")
DEMO_MAP_P8_TEST(FP8FullSystemLoop21, 21, "BeginRunRunInventorySameGuid")
DEMO_MAP_P8_TEST(FP8FullSystemLoop22, 22, "BeginRunRuntimeHotbarSameGuid")
DEMO_MAP_P8_TEST(FP8FullSystemLoop23, 23, "BeginRunFailureFullRollback")
DEMO_MAP_P8_TEST(FP8FullSystemLoop24, 24, "ExtractionRetainsSurvivingReferences")
DEMO_MAP_P8_TEST(FP8FullSystemLoop25, 25, "ConsumedItemClearsPersistentReferences")
DEMO_MAP_P8_TEST(FP8FullSystemLoop26, 26, "DeathClearsLostReferences")
DEMO_MAP_P8_TEST(FP8FullSystemLoop27, 27, "SpiritStoneRulesFixedTwentyNonItem")
DEMO_MAP_P8_TEST(FP8FullSystemLoop28, 28, "SpiritStonePickupSuccess")
DEMO_MAP_P8_TEST(FP8FullSystemLoop29, 29, "SpiritStoneDuplicateSourceReject")
DEMO_MAP_P8_TEST(FP8FullSystemLoop30, 30, "SpiritStoneWrongRunAndStaleReject")
DEMO_MAP_P8_TEST(FP8FullSystemLoop31, 31, "SpiritStoneOverflowReject")
DEMO_MAP_P8_TEST(FP8FullSystemLoop32, 32, "SpiritStonePrecommitFailureAtomic")
DEMO_MAP_P8_TEST(FP8FullSystemLoop33, 33, "SpiritStonePostcommitReloadNoReplay")
DEMO_MAP_P8_TEST(FP8FullSystemLoop34, 34, "ExtractionTransfersRiskExactlyOnce")
DEMO_MAP_P8_TEST(FP8FullSystemLoop35, 35, "DeathLosesRisk")
DEMO_MAP_P8_TEST(FP8FullSystemLoop36, 36, "AbandonLosesRisk")
DEMO_MAP_P8_TEST(FP8FullSystemLoop37, 37, "RecoveredAbandonLosesRisk")
DEMO_MAP_P8_TEST(FP8FullSystemLoop38, 38, "SettlementFirstEventWinsCurrency")
DEMO_MAP_P8_TEST(FP8FullSystemLoop39, 39, "SettlementOverflowFullRollback")
DEMO_MAP_P8_TEST(FP8FullSystemLoop40, 40, "SettlementSummaryExact")
DEMO_MAP_P8_TEST(FP8FullSystemLoop41, 41, "InputRegistryExactDefaults")
DEMO_MAP_P8_TEST(FP8FullSystemLoop42, 42, "InputDuplicateAndReservedReject")
DEMO_MAP_P8_TEST(FP8FullSystemLoop43, 43, "InputRemapImmediateApply")
DEMO_MAP_P8_TEST(FP8FullSystemLoop44, 44, "InteractPressedReleasedAfterRemap")
DEMO_MAP_P8_TEST(FP8FullSystemLoop45, 45, "HotbarActionAfterRemap")
DEMO_MAP_P8_TEST(FP8FullSystemLoop46, 46, "InputSettingsReloadPersistence")
DEMO_MAP_P8_TEST(FP8FullSystemLoop47, 47, "RestoreDefaultsExact")
DEMO_MAP_P8_TEST(FP8FullSystemLoop48, 48, "PreparationViewIntegratedRefresh")
DEMO_MAP_P8_TEST(FP8FullSystemLoop49, 49, "EndToEndTradeBalanceAndIdentity")
DEMO_MAP_P8_TEST(FP8FullSystemLoop50, 50, "ProtectedScopesAndCurrentSchema")

#undef DEMO_MAP_P8_TEST

#endif
