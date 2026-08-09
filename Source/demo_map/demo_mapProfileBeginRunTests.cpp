#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileBeginRunTransaction.h"
#include "demo_mapProfileRepository.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	struct FProductionSnapshot
	{
		TArray<FString> Paths;
		TArray<bool> Existed;
		TArray<TArray<uint8>> Bytes;

		FProductionSnapshot()
		{
			const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
			Paths = { Production.PrimaryPath(), Production.BackupPath(), Production.TempPath() };
			for (const FString& Path : Paths)
			{
				const bool bExists = IFileManager::Get().FileExists(*Path);
				Existed.Add(bExists);
				TArray<uint8> Value;
				if (bExists) ReadBytes(Path, Value);
				Bytes.Add(MoveTemp(Value));
			}
		}

		bool IsUnchanged() const
		{
			for (int32 Index = 0; Index < Paths.Num(); ++Index)
			{
				if (IFileManager::Get().FileExists(*Paths[Index]) != Existed[Index]) return false;
				if (Existed[Index])
				{
					TArray<uint8> After;
					if (!ReadBytes(Paths[Index], After) || After != Bytes[Index]) return false;
				}
			}
			return true;
		}
	};

	FString NewBeginRunRoot()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("Dev.D.UE.0.0.5.P1.0.r0"), TEXT("ItemEconomySchema"), FGuid::NewGuid().ToString(EGuidFormats::Digits), TEXT("ProfileBeginRun"));
	}

	Fdemo_mapPersistentProfile CreateCommitted(Fdemo_mapProfileRepository& Repository, const Fdemo_mapProfileStorageContext& Storage)
	{
		return Repository.LoadOrCreateDefaultProfile(Storage).Profile;
	}

	FGuid AddStashRecord(Fdemo_mapPersistentProfile& Profile, FName DefinitionId, int32 StackCount)
	{
		Fdemo_mapPersistentItemRecord Item;
		do Item.ItemInstanceId = FGuid::NewGuid(); while (Profile.PermanentStash.ContainsByPredicate([&Item](const Fdemo_mapPersistentItemRecord& Existing){ return Existing.ItemInstanceId == Item.ItemInstanceId; }));
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = StackCount;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item.ItemInstanceId;
	}

	Fdemo_mapBeginRunRequest RequestFor(const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapBeginRunRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		return Request;
	}

	bool IdsMatchItems(const Fdemo_mapPersistentProfile& Profile)
	{
		if (Profile.ActiveRun.DeployedItemIds.Num() != Profile.ActiveRun.ActiveRunItems.Num()) return false;
		for (int32 Index = 0; Index < Profile.ActiveRun.ActiveRunItems.Num(); ++Index)
		{
			if (Profile.ActiveRun.DeployedItemIds[Index] != Profile.ActiveRun.ActiveRunItems[Index].ItemInstanceId) return false;
		}
		return true;
	}

	bool FailureLeftStateUntouched(const Fdemo_mapPersistentProfile& BeforeProfile, const Fdemo_mapPersistentProfile& AfterProfile, const FString& PrimaryPath, const TArray<uint8>& BeforeBytes, const Fdemo_mapBeginRunResult& Result)
	{
		TArray<uint8> AfterBytes;
		return !Result.IsCommitted() && !Result.CommittedLoadoutPlan.IsSet() && !Result.ActiveRunId.IsValid()
			&& BeforeProfile == AfterProfile && ReadBytes(PrimaryPath, AfterBytes) && BeforeBytes == AfterBytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunEmptyTest, "demo_map.Profile.BeginRun.01.EmptyLoadoutCommitsActiveRun", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunEmptyTest::RunTest(const FString&)
{
	FProductionSnapshot Production;
	Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot());
	Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const Fdemo_mapPersistentProfile Before = Profile;
	const Fdemo_mapBeginRunResult Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, RequestFor(Profile), Repository, Storage);
	TestTrue(TEXT("Committed with launch plan"), Result.Status == Edemo_mapBeginRunStatus::Committed && Result.CommittedLoadoutPlan.IsSet() && Result.CommittedProfile.IsSet());
	TestTrue(TEXT("Unique prepared run"), Profile.ActiveRun.bHasActiveRun && Profile.ActiveRun.ActiveRunId.IsValid() && Profile.ActiveRun.ActiveRunId != Profile.ProfileId && Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::Prepared);
	TestTrue(TEXT("Empty loadout and stash order preserved"), Profile.ActiveRun.ActiveRunItems.IsEmpty() && Profile.ActiveRun.DeployedItemIds.IsEmpty() && Profile.PermanentStash == Before.PermanentStash);
	TestEqual(TEXT("Generation increments once"), Profile.SaveGeneration, Before.SaveGeneration + 1);
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunEquipmentTest, "demo_map.Profile.BeginRun.02.EquipmentSelectionMovesOriginalIds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunEquipmentTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot());
	Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const TArray<Fdemo_mapPersistentItemRecord> Before = Profile.PermanentStash;
	Fdemo_mapBeginRunRequest Request = RequestFor(Profile); Request.Loadout.WeaponItemInstanceId = Before[0].ItemInstanceId; Request.Loadout.ArmorItemInstanceId = Before[1].ItemInstanceId; Request.Loadout.AccessoryItemInstanceId = Before[2].ItemInstanceId;
	const Fdemo_mapBeginRunResult Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Equipment commit"), Result.IsCommitted() && Profile.PermanentStash.IsEmpty() && Profile.ActiveRun.ActiveRunItems.Num() == 3 && IdsMatchItems(Profile));
	if (!Result.IsCommitted() || Profile.ActiveRun.ActiveRunItems.Num() != 3) { AddError(Result.Diagnostic); return false; }
	const TArray<FName> Slots = { Fdemo_mapItemIds::WeaponSlot, Fdemo_mapItemIds::ArmorSlot, Fdemo_mapItemIds::AccessorySlot };
	for (int32 Index = 0; Index < 3; ++Index) TestTrue(TEXT("Original equipment record moved"), Profile.ActiveRun.ActiveRunItems[Index].ItemInstanceId == Before[Index].ItemInstanceId && Profile.ActiveRun.ActiveRunItems[Index].ItemDefinitionId == Before[Index].ItemDefinitionId && Profile.ActiveRun.ActiveRunItems[Index].StackCount == Before[Index].StackCount && Profile.ActiveRun.ActiveRunItems[Index].EquipmentSlotId == Slots[Index] && !Profile.ActiveRun.ActiveRunItems[Index].OriginRunId.IsValid());
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunMaterialsTest, "demo_map.Profile.BeginRun.03.WholeMaterialStacksMoveWithoutSplit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunMaterialsTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage);
	const FGuid Dust = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5); const FGuid Iron = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 4); TestTrue(TEXT("Fixture commit"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	Fdemo_mapBeginRunRequest Request = RequestFor(Profile); Request.Loadout.MaterialStackItemInstanceIds = { Dust, Iron };
	const Fdemo_mapBeginRunResult Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Whole stacks committed"), Result.IsCommitted() && Profile.ActiveRun.ActiveRunItems.Num() == 2 && Profile.ActiveRun.ActiveRunItems[0].ItemInstanceId == Dust && Profile.ActiveRun.ActiveRunItems[0].StackCount == 5 && Profile.ActiveRun.ActiveRunItems[1].ItemInstanceId == Iron && Profile.ActiveRun.ActiveRunItems[1].StackCount == 4);
	TestTrue(TEXT("No split or duplicate"), !Profile.PermanentStash.ContainsByPredicate([Dust,Iron](const Fdemo_mapPersistentItemRecord& Item){ return Item.ItemInstanceId == Dust || Item.ItemInstanceId == Iron; }) && IdsMatchItems(Profile));
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunMaximumTest, "demo_map.Profile.BeginRun.04.MixedMaximumSixRecords", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunMaximumTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage);
	const FGuid DustA = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 5); const FGuid Iron = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 5); const FGuid DustB = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2); TestTrue(TEXT("Fixture commit"), Repository.SaveProfile(Profile, Storage).IsSuccess());
	Fdemo_mapBeginRunRequest Request = RequestFor(Profile); Request.Loadout.WeaponItemInstanceId = Profile.PermanentStash[0].ItemInstanceId; Request.Loadout.ArmorItemInstanceId = Profile.PermanentStash[1].ItemInstanceId; Request.Loadout.AccessoryItemInstanceId = Profile.PermanentStash[2].ItemInstanceId; Request.Loadout.MaterialStackItemInstanceIds = { DustB, DustA, Iron };
	const Fdemo_mapBeginRunResult Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Maximum six committed"), Result.IsCommitted() && Profile.ActiveRun.ActiveRunItems.Num() == 6 && Profile.PermanentStash.IsEmpty() && IdsMatchItems(Profile));
	if (!Result.IsCommitted() || Profile.ActiveRun.DeployedItemIds.Num() != 6) { AddError(Result.Diagnostic); return false; }
	TestTrue(TEXT("Equipment then canonical materials"), Profile.ActiveRun.DeployedItemIds[3] == DustA && Profile.ActiveRun.DeployedItemIds[4] == Iron && Profile.ActiveRun.DeployedItemIds[5] == DustB);
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunMaterialOrderTest, "demo_map.Profile.BeginRun.05.MaterialCanonicalOrderUsesStashOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunMaterialOrderTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage);
	const FGuid A = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 1); const FGuid B = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 2); const FGuid C = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 3); Repository.SaveProfile(Profile, Storage);
	Fdemo_mapBeginRunRequest Request = RequestFor(Profile); Request.Loadout.MaterialStackItemInstanceIds = { C, A, B }; const Fdemo_mapBeginRunResult Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Request order ignored"), Result.IsCommitted() && Profile.ActiveRun.DeployedItemIds == TArray<FGuid>{ A, B, C }); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunSurvivorOrderTest, "demo_map.Profile.BeginRun.06.StashSurvivorOrderPreserved", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunSurvivorOrderTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage);
	const FGuid A = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 1); const FGuid B = AddStashRecord(Profile, Fdemo_mapItemIds::IronShard, 2); const FGuid C = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 3); Repository.SaveProfile(Profile, Storage); const TArray<FGuid> Expected = { Profile.PermanentStash[0].ItemInstanceId, Profile.PermanentStash[1].ItemInstanceId, Profile.PermanentStash[2].ItemInstanceId, A, C };
	Fdemo_mapBeginRunRequest Request = RequestFor(Profile); Request.Loadout.MaterialStackItemInstanceIds = { B }; const Fdemo_mapBeginRunResult Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage); TArray<FGuid> Actual; for (const auto& Item : Profile.PermanentStash) Actual.Add(Item.ItemInstanceId);
	TestTrue(TEXT("Middle removal preserves survivors"), Result.IsCommitted() && Actual == Expected); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunMaterialRejectTest, "demo_map.Profile.BeginRun.07.AncientTokenAndWrongCategoryRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunMaterialRejectTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const FGuid Token = AddStashRecord(Profile, Fdemo_mapItemIds::AncientToken, 1); Repository.SaveProfile(Profile, Storage); TArray<uint8> Bytes; ReadBytes(Storage.PrimaryPath(), Bytes); const Fdemo_mapPersistentProfile Before = Profile;
	Fdemo_mapBeginRunRequest TokenRequest = RequestFor(Profile); TokenRequest.Loadout.MaterialStackItemInstanceIds = { Token }; const auto TokenResult = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, TokenRequest, Repository, Storage); TestTrue(TEXT("AncientToken rejected"), TokenResult.Status == Edemo_mapBeginRunStatus::MaterialSelectionRejected && FailureLeftStateUntouched(Before, Profile, Storage.PrimaryPath(), Bytes, TokenResult));
	Fdemo_mapBeginRunRequest EquipmentAsMaterial = RequestFor(Profile); EquipmentAsMaterial.Loadout.MaterialStackItemInstanceIds = { Profile.PermanentStash[0].ItemInstanceId }; const auto WrongMaterial = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, EquipmentAsMaterial, Repository, Storage); TestTrue(TEXT("Equipment in materials rejected"), WrongMaterial.Status == Edemo_mapBeginRunStatus::MaterialSelectionRejected);
	Fdemo_mapBeginRunRequest MaterialAsEquipment = RequestFor(Profile); MaterialAsEquipment.Loadout.WeaponItemInstanceId = Token; const auto WrongSlot = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, MaterialAsEquipment, Repository, Storage); TestTrue(TEXT("Non-equipment in slot rejected"), WrongSlot.Status == Edemo_mapBeginRunStatus::EquipmentSlotOrCompatibilityRejected); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunDuplicateTest, "demo_map.Profile.BeginRun.08.DuplicateSelectionRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunDuplicateTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const FGuid Dust = AddStashRecord(Profile, Fdemo_mapItemIds::SpiritDust, 2); Repository.SaveProfile(Profile, Storage); TArray<uint8> Bytes; ReadBytes(Storage.PrimaryPath(), Bytes); const Fdemo_mapPersistentProfile Before = Profile;
	Fdemo_mapBeginRunRequest Cross = RequestFor(Profile); Cross.Loadout.WeaponItemInstanceId = Before.PermanentStash[0].ItemInstanceId; Cross.Loadout.ArmorItemInstanceId = Cross.Loadout.WeaponItemInstanceId; const auto A = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Cross, Repository, Storage); TestTrue(TEXT("Duplicate slots rejected"), A.Status == Edemo_mapBeginRunStatus::DuplicateSelection && FailureLeftStateUntouched(Before, Profile, Storage.PrimaryPath(), Bytes, A));
	Fdemo_mapBeginRunRequest Material = RequestFor(Profile); Material.Loadout.MaterialStackItemInstanceIds = { Dust, Dust }; const auto B = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Material, Repository, Storage); TestTrue(TEXT("Duplicate material rejected"), B.Status == Edemo_mapBeginRunStatus::DuplicateSelection); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunMissingTest, "demo_map.Profile.BeginRun.09.MissingStaleOrWrongOwnerRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunMissingTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); TArray<uint8> Bytes; ReadBytes(Storage.PrimaryPath(), Bytes); const Fdemo_mapPersistentProfile Before = Profile;
	Fdemo_mapBeginRunRequest Request = RequestFor(Profile); Request.Loadout.MaterialStackItemInstanceIds = { FGuid::NewGuid() }; const auto Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Request, Repository, Storage);
	TestTrue(TEXT("Missing stale ID rejected"), Result.Status == Edemo_mapBeginRunStatus::SelectedItemNotFound && FailureLeftStateUntouched(Before, Profile, Storage.PrimaryPath(), Bytes, Result)); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunLimitsTest, "demo_map.Profile.BeginRun.10.SlotCompatibilityAndLimitsRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunLimitsTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); TArray<FGuid> Materials; for (int32 Index = 0; Index < 4; ++Index) Materials.Add(AddStashRecord(Profile, Index % 2 ? Fdemo_mapItemIds::IronShard : Fdemo_mapItemIds::SpiritDust, 1)); Repository.SaveProfile(Profile, Storage);
	Fdemo_mapBeginRunRequest WrongSlot = RequestFor(Profile); WrongSlot.Loadout.WeaponItemInstanceId = Profile.PermanentStash[1].ItemInstanceId; const auto A = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, WrongSlot, Repository, Storage); TestTrue(TEXT("Wrong slot rejected"), A.Status == Edemo_mapBeginRunStatus::EquipmentSlotOrCompatibilityRejected);
	Fdemo_mapBeginRunRequest TooMany = RequestFor(Profile); TooMany.Loadout.MaterialStackItemInstanceIds = Materials; const auto B = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, TooMany, Repository, Storage); TestTrue(TEXT("Fourth material rejected"), B.Status == Edemo_mapBeginRunStatus::SelectionLimitExceeded && !B.CommittedLoadoutPlan.IsSet()); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunOptimisticTest, "demo_map.Profile.BeginRun.11.ExpectedIdentityAndGenerationProtectAgainstStaleIntent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunOptimisticTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); TArray<uint8> Bytes; ReadBytes(Storage.PrimaryPath(), Bytes); const Fdemo_mapPersistentProfile Before = Profile;
	Fdemo_mapBeginRunRequest WrongId = RequestFor(Profile); WrongId.ExpectedProfileId = FGuid::NewGuid(); const auto A = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, WrongId, Repository, Storage); TestTrue(TEXT("Identity mismatch"), A.Status == Edemo_mapBeginRunStatus::ProfileIdentityMismatch && FailureLeftStateUntouched(Before, Profile, Storage.PrimaryPath(), Bytes, A));
	Fdemo_mapBeginRunRequest Stale = RequestFor(Profile); --Stale.ExpectedSaveGeneration; const auto B = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, Stale, Repository, Storage); TestTrue(TEXT("Generation mismatch"), B.Status == Edemo_mapBeginRunStatus::ProfileGenerationMismatch && FailureLeftStateUntouched(Before, Profile, Storage.PrimaryPath(), Bytes, B)); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunExistingTest, "demo_map.Profile.BeginRun.12.ExistingActiveRunRejectsSecondBegin", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunExistingTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const auto First = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, RequestFor(Profile), Repository, Storage); TArray<uint8> Bytes; ReadBytes(Storage.PrimaryPath(), Bytes); const Fdemo_mapPersistentProfile Before = Profile; const FGuid RunId = Profile.ActiveRun.ActiveRunId;
	const auto Second = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, RequestFor(Profile), Repository, Storage); TestTrue(TEXT("Second begin rejected"), First.IsCommitted() && Second.Status == Edemo_mapBeginRunStatus::ActiveRunAlreadyExists && Profile.ActiveRun.ActiveRunId == RunId && FailureLeftStateUntouched(Before, Profile, Storage.PrimaryPath(), Bytes, Second)); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunPreCommitFailuresTest, "demo_map.Profile.BeginRun.13.PreCommitRepositoryFailuresPreserveOldProfile", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunPreCommitFailuresTest::RunTest(const FString&)
{
	FProductionSnapshot Production;
	for (Edemo_mapProfileFailureStage Stage : { Edemo_mapProfileFailureStage::WriteTemp, Edemo_mapProfileFailureStage::PrepareBackup, Edemo_mapProfileFailureStage::AtomicReplace })
	{
		Fdemo_mapProfileRepository Repository; Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const Fdemo_mapPersistentProfile Before = Profile; TArray<uint8> PrimaryBefore; ReadBytes(Storage.PrimaryPath(), PrimaryBefore); Storage.InjectedFailure = Stage;
		const auto Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, RequestFor(Profile), Repository, Storage); TArray<uint8> PrimaryAfter; ReadBytes(Storage.PrimaryPath(), PrimaryAfter);
		TestTrue(TEXT("Pre-commit status mapped"), Result.Status == Edemo_mapBeginRunStatus::RepositorySaveRejected && !Result.CommittedLoadoutPlan.IsSet() && Profile == Before && PrimaryBefore == PrimaryAfter);
		Storage.InjectedFailure = Edemo_mapProfileFailureStage::None; TestTrue(TEXT("Old primary or verified backup remains loadable"), Repository.LoadExistingProfile(Storage).IsSuccess());
	}
	TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunPostCommitTest, "demo_map.Profile.BeginRun.14.PostCommitVerificationFailureRequiresReload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunPostCommitTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewBeginRunRoot()); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const Fdemo_mapPersistentProfile Before = Profile; Storage.InjectedFailure = Edemo_mapProfileFailureStage::ReadBackCommittedPrimary;
	const auto Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, RequestFor(Profile), Repository, Storage); TestTrue(TEXT("Reload required and caller unchanged"), Result.Status == Edemo_mapBeginRunStatus::CommitOutcomeRequiresReload && Profile == Before && !Result.CommittedLoadoutPlan.IsSet() && !Result.ActiveRunId.IsValid());
	Storage.InjectedFailure = Edemo_mapProfileFailureStage::None; const auto Reload = Repository.LoadExistingProfile(Storage); TestTrue(TEXT("Reload determines one complete generation"), Reload.IsSuccess() && (Reload.Profile.SaveGeneration == Before.SaveGeneration || Reload.Profile.SaveGeneration == Before.SaveGeneration + 1) && ((Reload.Profile.SaveGeneration == Before.SaveGeneration) != Reload.Profile.ActiveRun.bHasActiveRun)); TestTrue(TEXT("Production untouched"), Production.IsUnchanged()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeginRunIsolationTest, "demo_map.Profile.BeginRun.15.AutomationIsolationAndNoProductionHook", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBeginRunIsolationTest::RunTest(const FString&)
{
	FProductionSnapshot Production; Fdemo_mapProfileRepository Repository; const FString Root = NewBeginRunRoot(); const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root); Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage); const auto Result = Fdemo_mapProfileBeginRunTransaction().Execute(Profile, RequestFor(Profile), Repository, Storage);
	TestTrue(TEXT("Isolated transaction succeeds"), Result.IsCommitted() && (Root.Contains(TEXT("Saved/Automation/Dev.D.UE.0.0.5.P1.0.r0/ItemEconomySchema")) || Root.Contains(TEXT("Saved\\Automation\\Dev.D.UE.0.0.5.P1.0.r0\\ItemEconomySchema")))); TestTrue(TEXT("Production profile bytes unchanged"), Production.IsUnchanged());
	TArray<FString> Sources; IFileManager::Get().FindFilesRecursive(Sources, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("demo_map")), TEXT("*.cpp"), true, false); bool bUnexpectedReference = false;
	for (const FString& Path : Sources)
	{
		FString Text; if (!FFileHelper::LoadFileToString(Text, *Path) || !Text.Contains(TEXT("Fdemo_mapProfileBeginRunTransaction"))) continue;
		if (!Path.EndsWith(TEXT("demo_mapProfileBeginRunTransaction.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileBeginRunTests.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSettlementTests.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSessionCoordinator.cpp")) && !Path.EndsWith(TEXT("demo_mapProfileSessionTests.cpp")) && !Path.EndsWith(TEXT("demo_mapFullSystemLoopTests.cpp"))) { bUnexpectedReference = true; AddError(FString::Printf(TEXT("Unexpected startup/runtime transaction reference: %s"), *Path)); }
	}
	TestFalse(TEXT("No normal startup, ItemSubsystem, Manager, UI, or map hook"), bUnexpectedReference); return true;
}

#endif
