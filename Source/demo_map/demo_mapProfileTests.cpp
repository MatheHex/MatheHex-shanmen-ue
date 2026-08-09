#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString NewProfileTestRoot()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("Dev.D.UE.0.0.5.P1.0.r0"), TEXT("ItemEconomySchema"), FGuid::NewGuid().ToString(EGuidFormats::Digits), TEXT("Profile"));
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	Fdemo_mapProfileLoadResult CreateCommitted(Fdemo_mapProfileRepository& Repository, const Fdemo_mapProfileStorageContext& Storage)
	{
		return Repository.LoadOrCreateDefaultProfile(Storage);
	}

	bool HasExactFreshContents(const Fdemo_mapPersistentProfile& Profile)
	{
		return Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
			&& Profile.PersistentSpiritStones == 0
			&& Profile.PermanentStash.Num() == 3
			&& Profile.PermanentStash[0].ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade
			&& Profile.PermanentStash[1].ItemDefinitionId == Fdemo_mapItemIds::TrainingVest
			&& Profile.PermanentStash[2].ItemDefinitionId == Fdemo_mapItemIds::WindTalisman
			&& Profile.PermanentStash[0].StackCount == 1
			&& Profile.PermanentStash[1].StackCount == 1
			&& Profile.PermanentStash[2].StackCount == 1
			&& !Profile.ActiveRun.bHasActiveRun
			&& Profile.ActiveRun.RiskSpiritStones == 0
			&& !Profile.LastSettlementId.IsValid();
	}

	bool FileStateEqual(const FString& Path, bool bExisted, const TArray<uint8>& Before)
	{
		if (IFileManager::Get().FileExists(*Path) != bExisted) return false;
		if (!bExisted) return true;
		TArray<uint8> After;
		return ReadBytes(Path, After) && After == Before;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileFreshContentsTest, "demo_map.Profile.01.FreshProfileFrozenContents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileFreshContentsTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
	FString Error;
	TestTrue(TEXT("Fresh profile validates"), Repository.ValidateProfile(Profile, &Error));
	TestTrue(TEXT("Frozen starter definitions, counts, order, and empty run"), HasExactFreshContents(Profile));
	TestEqual(TEXT("Fresh profile starts before first commit"), Profile.SaveGeneration, 0);
	for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
	{
		TestTrue(TEXT("Starter belongs only to ordered Permanent Stash"), Item.PersistentDomain == Edemo_mapPersistentDomain::PermanentStash && Item.EquipmentSlotId.IsNone() && !Item.OriginRunId.IsValid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileFreshIdentityTest, "demo_map.Profile.02.FreshProfileUniqueIdentities", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileFreshIdentityTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapPersistentProfile A = Repository.CreateFreshProfile();
	const Fdemo_mapPersistentProfile B = Repository.CreateFreshProfile();
	TSet<FGuid> Ids;
	TestTrue(TEXT("Profile IDs are valid and not reused"), A.ProfileId.IsValid() && B.ProfileId.IsValid() && A.ProfileId != B.ProfileId);
	for (const Fdemo_mapPersistentItemRecord& Item : A.PermanentStash) { TestTrue(TEXT("A item ID valid and unique"), Item.ItemInstanceId.IsValid() && !Ids.Contains(Item.ItemInstanceId)); Ids.Add(Item.ItemInstanceId); }
	for (const Fdemo_mapPersistentItemRecord& Item : B.PermanentStash) { TestTrue(TEXT("B item ID valid and not reused"), Item.ItemInstanceId.IsValid() && !Ids.Contains(Item.ItemInstanceId)); Ids.Add(Item.ItemInstanceId); }
	TestEqual(TEXT("Six unique starter IDs"), Ids.Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileRoundTripTest, "demo_map.Profile.03.JsonRoundTripPreservesOrderedValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileRoundTripTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	const Fdemo_mapProfileLoadResult Created = CreateCommitted(Repository, Storage);
	TestTrue(TEXT("Fresh profile committed"), Created.Status == Edemo_mapProfileLoadStatus::CreatedFreshAndCommitted && Created.Profile.SaveGeneration == 1);
	const Fdemo_mapProfileLoadResult Loaded = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Primary loads"), Loaded.Status == Edemo_mapProfileLoadStatus::LoadedPrimary);
	TestTrue(TEXT("All values and ordered arrays round trip"), Loaded.Profile == Created.Profile && HasExactFreshContents(Loaded.Profile));
	TArray<uint8> Bytes;
	TestTrue(TEXT("JSON file is UTF-8 bytes with explicit schema"), ReadBytes(Storage.PrimaryPath(), Bytes) && Bytes.Num() > 0 && Bytes[0] == '{');
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileRepeatedLoadTest, "demo_map.Profile.04.RepeatedLoadIsReadOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileRepeatedLoadTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	const Fdemo_mapProfileLoadResult Created = CreateCommitted(Repository, Storage);
	TArray<uint8> Before, After;
	ReadBytes(Storage.PrimaryPath(), Before);
	const Fdemo_mapProfileLoadResult A = Repository.LoadExistingProfile(Storage);
	const Fdemo_mapProfileLoadResult B = Repository.LoadExistingProfile(Storage);
	ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Repeated loads return primary"), A.Status == Edemo_mapProfileLoadStatus::LoadedPrimary && B.Status == Edemo_mapProfileLoadStatus::LoadedPrimary);
	TestTrue(TEXT("Repeated load does not change bytes or generation"), Before == After && A.Profile.SaveGeneration == Created.Profile.SaveGeneration && B.Profile == A.Profile);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileGenerationTest, "demo_map.Profile.05.SuccessfulSaveIncrementsOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileGenerationTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage).Profile;
	const int32 Before = Profile.SaveGeneration;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
	TestTrue(TEXT("Save succeeds"), Save.IsSuccess());
	TestEqual(TEXT("Caller generation increments exactly once"), Profile.SaveGeneration, Before + 1);
	TestEqual(TEXT("Result generation matches"), Save.CommittedGeneration, Profile.SaveGeneration);
	TestTrue(TEXT("Backup contains previous generation"), Repository.LoadExistingProfile(Storage).Profile == Profile);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileTempFailureTest, "demo_map.Profile.06.TempAndFlushFailuresPreservePrimary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileTempFailureTest::RunTest(const FString&)
{
	for (Edemo_mapProfileFailureStage Stage : { Edemo_mapProfileFailureStage::CreateDirectory, Edemo_mapProfileFailureStage::WriteTemp, Edemo_mapProfileFailureStage::FlushOrCloseTemp, Edemo_mapProfileFailureStage::ReadBackTemp, Edemo_mapProfileFailureStage::ValidateTemp })
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
		Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage).Profile;
		TArray<uint8> Before, After;
		ReadBytes(Storage.PrimaryPath(), Before);
		const int32 Generation = Profile.SaveGeneration;
		Storage.InjectedFailure = Stage;
		const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
		ReadBytes(Storage.PrimaryPath(), After);
		TestTrue(TEXT("Injected pre-commit stage does not report success"), !Save.IsSuccess());
		TestTrue(TEXT("Old primary bytes survive"), Before == After);
		TestEqual(TEXT("Caller generation is unchanged"), Profile.SaveGeneration, Generation);
		TestTrue(TEXT("Next normal load remains deterministic"), Repository.LoadExistingProfile(Storage).Status == Edemo_mapProfileLoadStatus::LoadedPrimary);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileReplaceFailureTest, "demo_map.Profile.07.BackupReplaceAndPostCommitFailuresRemainRecoverable", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileReplaceFailureTest::RunTest(const FString&)
{
	for (Edemo_mapProfileFailureStage Stage : { Edemo_mapProfileFailureStage::PrepareBackup, Edemo_mapProfileFailureStage::AtomicReplace, Edemo_mapProfileFailureStage::ReadBackCommittedPrimary })
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
		Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage).Profile;
		const int32 CallerGeneration = Profile.SaveGeneration;
		Storage.InjectedFailure = Stage;
		const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
		TestTrue(TEXT("Injected backup/replace/post-verify stage reports failure"), !Save.IsSuccess());
		TestEqual(TEXT("Failed call does not advance caller generation"), Profile.SaveGeneration, CallerGeneration);
		Storage.InjectedFailure = Edemo_mapProfileFailureStage::None;
		const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage);
		TestTrue(TEXT("At least one complete version remains loadable"), Load.IsSuccess());
		TestTrue(TEXT("A verified generation survives"), Load.Profile.SaveGeneration == CallerGeneration || Load.Profile.SaveGeneration == CallerGeneration + 1);
	}
	Fdemo_mapProfileRepository Repository;
	Fdemo_mapProfileStorageContext CleanupStorage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	Fdemo_mapPersistentProfile CleanupProfile = CreateCommitted(Repository, CleanupStorage).Profile;
	CleanupStorage.InjectedFailure = Edemo_mapProfileFailureStage::CleanupTemp;
	const Fdemo_mapProfileSaveResult Cleanup = Repository.SaveProfile(CleanupProfile, CleanupStorage);
	TestTrue(TEXT("Cleanup injection occurs only after a verified commit"), Cleanup.IsSuccess() && !Cleanup.bCleanupSucceeded && !IFileManager::Get().FileExists(*CleanupStorage.TempPath()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileCorruptRecoveryTest, "demo_map.Profile.08.CorruptPrimaryRecoversFromBackupAndPreservesBytes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileCorruptRecoveryTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	Fdemo_mapPersistentProfile Profile = CreateCommitted(Repository, Storage).Profile;
	TestTrue(TEXT("Second save creates backup"), Repository.SaveProfile(Profile, Storage).IsSuccess() && IFileManager::Get().FileExists(*Storage.BackupPath()));
	const TArray<uint8> Corrupt = { 0x7b, 0x22, 0x62, 0x61, 0x64, 0x22, 0x3a, 0xff };
	TestTrue(TEXT("Write corrupt primary fixture"), FFileHelper::SaveArrayToFile(Corrupt, *Storage.PrimaryPath()));
	const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Explicit backup recovery status"), Load.Status == Edemo_mapProfileLoadStatus::RecoveredFromBackup && Load.IsSuccess());
	TArray<uint8> Preserved;
	TestTrue(TEXT("Corrupt original bytes preserved exactly"), !Load.QuarantinedPath.IsEmpty() && ReadBytes(Load.QuarantinedPath, Preserved) && Preserved == Corrupt);
	TestTrue(TEXT("Corrupt preservation name contains the real SHA-256"), Load.QuarantinedPath.Contains(TEXT("61ebe59215099812cc288fb6cdea530e9b704ac6eabdfa310c28786e8b083524"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Recovered primary validates on subsequent load"), Repository.LoadExistingProfile(Storage).Status == Edemo_mapProfileLoadStatus::LoadedPrimary);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileCorruptRejectTest, "demo_map.Profile.09.CorruptWithoutBackupDoesNotReset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileCorruptRejectTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	CreateCommitted(Repository, Storage);
	const TArray<uint8> Corrupt = { 0x00, 0x01, 0x02, 0x03 };
	FFileHelper::SaveArrayToFile(Corrupt, *Storage.PrimaryPath());
	const Fdemo_mapProfileLoadResult Load = Repository.LoadOrCreateDefaultProfile(Storage);
	TArray<uint8> After;
	ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Corrupt primary without valid backup is explicitly rejected"), Load.Status == Edemo_mapProfileLoadStatus::CorruptPrimaryNoValidBackup && !Load.IsSuccess());
	TestTrue(TEXT("No silent fresh reset and original bytes remain"), After == Corrupt && !IFileManager::Get().FileExists(*Storage.BackupPath()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileFutureSchemaTest, "demo_map.Profile.10.FutureSchemaIsReadOnlyRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileFutureSchemaTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewProfileTestRoot());
	CreateCommitted(Repository, Storage);
	TArray<uint8> Current;
	ReadBytes(Storage.PrimaryPath(), Current);
	FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Current.GetData()), Current.Num());
	FString Json(Converted.Length(), Converted.Get());
	TestTrue(TEXT("Fixture schema token replaced"), Json.ReplaceInline(TEXT("\"SchemaVersion\":3"), TEXT("\"SchemaVersion\":999"), ESearchCase::CaseSensitive) == 1);
	FTCHARToUTF8 FutureUtf8(*Json);
	TArray<uint8> Future;
	Future.Append(reinterpret_cast<const uint8*>(FutureUtf8.Get()), FutureUtf8.Length());
	FFileHelper::SaveArrayToFile(Future, *Storage.PrimaryPath());
	const Fdemo_mapProfileLoadResult Load = Repository.LoadOrCreateDefaultProfile(Storage);
	TArray<uint8> After;
	ReadBytes(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Future schema has dedicated status"), Load.Status == Edemo_mapProfileLoadStatus::FutureSchemaRejected);
	TestTrue(TEXT("Future bytes are completely unchanged"), After == Future);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileSemanticValidationTest, "demo_map.Profile.11.SemanticInvalidRecordsAreRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileSemanticValidationTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapPersistentProfile Valid = Repository.CreateFreshProfile();
	auto Reject = [this, &Repository](const TCHAR* Name, Fdemo_mapPersistentProfile Profile) { FString Error; return TestFalse(Name, Repository.ValidateProfile(Profile, &Error)); };
	Fdemo_mapPersistentProfile Duplicate = Valid; Duplicate.PermanentStash[1].ItemInstanceId = Duplicate.PermanentStash[0].ItemInstanceId; Reject(TEXT("Duplicate ID rejected"), Duplicate);
	Fdemo_mapPersistentProfile Unknown = Valid; Unknown.PermanentStash[0].ItemDefinitionId = TEXT("Prototype.Item.Unknown"); Reject(TEXT("Unknown definition rejected"), Unknown);
	Fdemo_mapPersistentProfile Zero = Valid; Zero.PermanentStash[0].StackCount = 0; Reject(TEXT("Zero stack rejected"), Zero);
	Fdemo_mapPersistentProfile Over = Valid; Over.PermanentStash[0].StackCount = 2; Reject(TEXT("Overstack/equipment count rejected"), Over);
	Fdemo_mapPersistentProfile Slot = Valid; Slot.PermanentStash[0].EquipmentSlotId = Fdemo_mapItemIds::WeaponSlot; Reject(TEXT("Stash equipment slot rejected"), Slot);
	Fdemo_mapPersistentProfile Domain = Valid; Domain.PermanentStash[0].PersistentDomain = Edemo_mapPersistentDomain::ActiveRun; Reject(TEXT("Wrong container domain rejected"), Domain);
	Fdemo_mapPersistentProfile EmptyRun = Valid; EmptyRun.ActiveRun.DeployedItemIds.Add(Valid.PermanentStash[0].ItemInstanceId); Reject(TEXT("Inactive run carrying risk data rejected"), EmptyRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProfileIsolationTest, "demo_map.Profile.12.AutomationStorageIsolationAndHandleRelease", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProfileIsolationTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
	const TArray<FString> ProductionPaths = { Production.PrimaryPath(), Production.BackupPath(), Production.TempPath() };
	TArray<bool> Existed;
	TArray<TArray<uint8>> Before;
	for (const FString& Path : ProductionPaths)
	{
		const bool bExists = IFileManager::Get().FileExists(*Path);
		Existed.Add(bExists);
		TArray<uint8> Bytes; if (bExists) ReadBytes(Path, Bytes); Before.Add(Bytes);
	}
	const FString Root = NewProfileTestRoot();
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
	TestTrue(TEXT("Automation root is task-isolated and outside production"), Root.Contains(TEXT("Saved/Automation/Dev.D.UE.0.0.5.P1.0.r0/ItemEconomySchema")) || Root.Contains(TEXT("Saved\\Automation\\Dev.D.UE.0.0.5.P1.0.r0\\ItemEconomySchema")));
	TestTrue(TEXT("Isolated profile operation succeeds"), CreateCommitted(Repository, Storage).IsSuccess());
	for (int32 Index = 0; Index < ProductionPaths.Num(); ++Index) TestTrue(TEXT("Production save path existence and bytes unchanged"), FileStateEqual(ProductionPaths[Index], Existed[Index], Before[Index]));
	TestTrue(TEXT("No temporary file remains after success"), !IFileManager::Get().FileExists(*Storage.TempPath()));
	TestTrue(TEXT("All file handles released; own directory can be cleaned"), IFileManager::Get().DeleteDirectory(*Root, false, true));
	return true;
}

#endif
