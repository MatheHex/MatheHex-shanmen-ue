#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenItemMigration.h"

#include "ShanmenItemAuthorityService.h"
#include "ShanmenItemRepository.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapRewardAffix.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	using namespace demo_map_code_b;

	FGuid MigrationGuid(uint32 Value)
	{
		return FGuid(0xC0DE1000u + Value, 0x00000010u, 0x00000001u, 0x00000001u);
	}

	FString NewMigrationRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.1.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp TargetContent()
	{
		FShanmenContentStamp Content;
		Content.Version = FName(TEXT("Shanmen.Items.0.0.10.P1.1"));
		Content.Digest = TEXT("P1.1.LegacyMigration.Contract.v1");
		return Content;
	}

	Fdemo_mapProfileSessionSnapshot SessionSnapshot(
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapProfileSessionSnapshot Snapshot;
		Snapshot.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation;
		Snapshot.ProfileId = Profile.ProfileId;
		Snapshot.SaveGeneration = Profile.SaveGeneration;
		Snapshot.PersistentSpiritStones = Profile.PersistentSpiritStones;
		Snapshot.TownLevel = Profile.TownLevel;
		Snapshot.OrderedPermanentStash = Profile.PermanentStash;
		Snapshot.ShopStock = Profile.ShopStock;
		Snapshot.PreparationLayout = Profile.PreparationLayout;
		Snapshot.WarehouseLayout = Profile.WarehouseLayout;
		Snapshot.LastSettlementId = Profile.LastSettlementId;
		return Snapshot;
	}

	bool BuildLegacyFixture(
		Fdemo_mapPersistentProfile& OutProfile,
		FCodeBOutOfRaidInventoryRecord& OutRecord,
		FString& OutError)
	{
		Fdemo_mapProfileRepository ProfileRepository;
		OutProfile = ProfileRepository.CreateFreshProfile();
		OutProfile.SaveGeneration = 7;
		Fdemo_mapPersistentItemRecord* Ring =
			OutProfile.PermanentStash.FindByPredicate([](
				const Fdemo_mapPersistentItemRecord& Item)
			{
				return Item.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman;
			});
		if (!Ring)
		{
			OutError = TEXT("Fresh Profile has no WindTalisman fixture.");
			return false;
		}
		OutProfile.PreparationLayout.SpatialRingItemInstanceId =
			Ring->ItemInstanceId;
		Ring->AffixSet.AffixSetEventId = MigrationGuid(90);
		Ring->AffixSet.AffixPolicyId =
			Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		Ring->AffixSet.Acquisition =
			Edemo_mapRewardAffixAcquisition::Natural;
		Fdemo_mapResolvedRewardAffix& RingAffix =
			Ring->AffixSet.Affixes.AddDefaulted_GetRef();
		RingAffix.AffixId = TEXT("Reward.Affix.Accessory.Haste.T1");
		RingAffix.Tier = Edemo_mapRewardAffixTier::Tier1;
		RingAffix.ResolvedMagnitudeScaled = -250;
		RingAffix.ResolvedValue = 50;

		Fdemo_mapPersistentItemRecord Child;
		Child.ItemInstanceId = MigrationGuid(101);
		Child.ItemDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		Child.StackCount = 2;
		Child.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Child.LegacySpatialParentItemInstanceId = Ring->ItemInstanceId;
		Child.RewardEventKind = Edemo_mapRewardEventKind::Jackpot;
		Child.RewardEventId = MigrationGuid(102);
		Child.RewardValueMultiplierBps =
			Fdemo_mapRewardEventRules::JackpotMultiplierBps;
		Child.RewardSourceRoleId = TEXT("Test.Migration.Source");
		Child.RareRewardEventId = MigrationGuid(103);
		Child.RareRewardPolicyId = TEXT("Reward.Rare.TestMigration");
		Child.RareRewardTierId = TEXT("Reward.Rare.Tier2");
		Child.RareRewardBonusValue = 222;
		OutProfile.PermanentStash.Add(Child);
		if (!ProfileRepository.ValidateProfile(OutProfile, &OutError))
		{
			return false;
		}

		const FString Root = NewMigrationRoot(TEXT("CodeBSource"));
		FCodeBOutOfRaidProfileStore Store(Root, OutProfile.ProfileId);
		FCodeBRepository CodeBRepository;
		FCodeBP2PlayerLayout Layout;
		const FCodeBOutOfRaidOpenResult Open = Store.OpenOrMigrate(
			SessionSnapshot(OutProfile), CodeBRepository, Layout);
		if (!Open.bSuccess)
		{
			OutError = Open.Diagnostic;
			return false;
		}
		OutRecord = Open.Record;
		IFileManager::Get().DeleteDirectory(*Root, false, true);
		return true;
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsSchema6MigrationTest,
	"Shanmen.0_0_10.Items.Migration.Schema6ToCurrent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsSchema6MigrationTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const FString Root = NewMigrationRoot(TEXT("Schema6"));
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	const Fdemo_mapProfileLoadResult Created =
		Repository.LoadOrCreateDefaultProfile(Storage);
	TestTrue(TEXT("Current Profile fixture committed"), Created.IsSuccess());

	TArray<uint8> CurrentBytes;
	TestTrue(TEXT("Current Profile bytes readable"),
		ReadBytes(Storage.PrimaryPath(), CurrentBytes));
	FUTF8ToTCHAR Converted(
		reinterpret_cast<const ANSICHAR*>(CurrentBytes.GetData()),
		CurrentBytes.Num());
	FString LegacyJson(Converted.Length(), Converted.Get());
	const FString CurrentToken = FString::Printf(
		TEXT("\"SchemaVersion\":%d"),
		Fdemo_mapPersistentProfile::CurrentSchemaVersion);
	TestEqual(TEXT("Exactly one Schema token downgraded"),
		LegacyJson.ReplaceInline(
			*CurrentToken, TEXT("\"SchemaVersion\":6"),
			ESearchCase::CaseSensitive), 1);
	FTCHARToUTF8 LegacyUtf8(*LegacyJson);
	TArray<uint8> LegacyBytes;
	LegacyBytes.Append(
		reinterpret_cast<const uint8*>(LegacyUtf8.Get()),
		LegacyUtf8.Length());
	TestTrue(TEXT("Schema 6 fixture replaces primary"),
		FFileHelper::SaveArrayToFile(LegacyBytes, *Storage.PrimaryPath()));

	const Fdemo_mapProfileLoadResult Migrated =
		Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Schema 6 atomically migrates to current"),
		Migrated.IsSuccess()
		&& Migrated.Profile.SchemaVersion
			== Fdemo_mapPersistentProfile::CurrentSchemaVersion);
	TestEqual(TEXT("Migration advances SaveGeneration once"),
		Migrated.Profile.SaveGeneration,
		Created.Profile.SaveGeneration + 1);
	TestEqual(TEXT("Migration preserves ordered item identities"),
		Migrated.Profile.PermanentStash,
		Created.Profile.PermanentStash);
	TArray<uint8> BackupBytes;
	TestTrue(TEXT("Backup preserves exact Schema 6 source bytes"),
		ReadBytes(Storage.BackupPath(), BackupBytes)
		&& BackupBytes == LegacyBytes);
	const Fdemo_mapProfileLoadResult Reopened =
		Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Subsequent current-schema load is read-only"),
		Reopened.IsSuccess() && Reopened.Profile == Migrated.Profile);
	IFileManager::Get().DeleteDirectory(*Root, false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsLegacyCandidateTest,
	"Shanmen.0_0_10.Items.Migration.DeterministicCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsLegacyCandidateTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Legacy source fixture builds"),
		BuildLegacyFixture(Profile, Record, Error));
	const Fdemo_mapPersistentProfile ProfileBefore = Profile;
	const FCodeBSnapshot CodeBBefore = Record.RepositorySnapshot;

	const Fdemo_mapShanmenItemMigrationResult First =
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, Record, TargetContent());
	const Fdemo_mapShanmenItemMigrationResult Second =
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, Record, TargetContent());
	TestTrue(TEXT("Matching sources produce one valid candidate"),
		First.IsSuccess() && First.Receipt.IsSuccess());
	TestTrue(TEXT("Same sources replay exact normalized snapshot"),
		Second.IsSuccess() && Second.Candidate == First.Candidate);
	TestTrue(TEXT("Migration identity and digest are deterministic"),
		Second.Receipt.MigrationId == First.Receipt.MigrationId
		&& Second.Receipt.CandidateDigest == First.Receipt.CandidateDigest);
	TestEqual(TEXT("Every legacy item is retained once"),
		First.Candidate.Items.Num(), Profile.PermanentStash.Num());
	TestTrue(TEXT("Migration is read-only for both sources"),
		Profile == ProfileBefore && Record.RepositorySnapshot == CodeBBefore);

	const FShanmenItemInstance* Parent =
		First.Candidate.Items.FindByPredicate([](
			const FShanmenItemInstance& Item)
		{
			return Item.DefinitionId == Fdemo_mapItemIds::WindTalisman;
		});
	const FShanmenItemInstance* Child =
		First.Candidate.Items.FindByPredicate([](
			const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == MigrationGuid(101);
		});
	TestTrue(TEXT("Item-owned child container topology is preserved"),
		Parent && Child && Parent->ChildContainerId.IsValid()
		&& Child->ParentContainerId == Parent->ChildContainerId);
	TestTrue(TEXT("Legacy reward provenance and affix values enter canonical authority"),
		Parent && Parent->RewardMetadata.Affixes.Num() == 1
		&& Parent->RewardMetadata.Affixes[0].AffixId
			== FName(TEXT("Reward.Affix.Accessory.Haste.T1"))
		&& Child
		&& Child->RewardMetadata.RewardEventKind
			== EShanmenItemRewardEventKind::Jackpot
		&& Child->RewardMetadata.RareRewardBonusValue == 222);
	FShanmenItemRepository Repository;
	EShanmenItemTransactionError LoadError =
		EShanmenItemTransactionError::None;
	TestTrue(TEXT("Candidate loads into the sole new authority"),
		Repository.TryLoadSnapshot(First.Candidate, &LoadError)
		&& Repository.CaptureSnapshot() == First.Candidate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsLegacyConflictTest,
	"Shanmen.0_0_10.Items.Migration.SourceConflictsFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsLegacyConflictTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Legacy source fixture builds"),
		BuildLegacyFixture(Profile, Record, Error));

	FShanmenContentStamp InvalidContent;
	TestTrue(TEXT("Missing target content is rejected"),
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, Record, InvalidContent).Error
			== Edemo_mapShanmenItemMigrationError::InvalidTargetContent);

	FCodeBOutOfRaidInventoryRecord WrongOwner = Record;
	WrongOwner.OwnerId = MigrationGuid(201);
	TestTrue(TEXT("Owner mismatch is rejected"),
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, WrongOwner, TargetContent()).Error
			== Edemo_mapShanmenItemMigrationError::SourceIdentityMismatch);

	FCodeBOutOfRaidInventoryRecord WrongFingerprint = Record;
	WrongFingerprint.Receipt.SourceFingerprint += TEXT(".tampered");
	TestTrue(TEXT("Fingerprint mismatch is rejected"),
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, WrongFingerprint, TargetContent()).Error
			== Edemo_mapShanmenItemMigrationError::SourceProvenanceMismatch);

	FCodeBOutOfRaidInventoryRecord QuantityConflict = Record;
	FCodeBItemInstance* Stack =
		QuantityConflict.RepositorySnapshot.Items.Find(MigrationGuid(101));
	if (Stack)
	{
		++Stack->Quantity;
	}
	TestTrue(TEXT("Quantity disagreement is rejected"),
		Stack && Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, QuantityConflict, TargetContent()).Error
			== Edemo_mapShanmenItemMigrationError::SourceItemMismatch);

	FCodeBOutOfRaidInventoryRecord HiddenContainer = Record;
	FCodeBContainer Extra;
	Extra.ContainerId = MigrationGuid(202);
	Extra.ContainerType = FName(TEXT("HiddenMigrationRoot"));
	Extra.Slots.Init(FGuid(), 1);
	HiddenContainer.RepositorySnapshot.Containers.Add(
		Extra.ContainerId, Extra);
	TestTrue(TEXT("Container outside committed closure is rejected"),
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, HiddenContainer, TargetContent()).Error
			== Edemo_mapShanmenItemMigrationError::SourceContainerMismatch);

	FCodeBOutOfRaidInventoryRecord ActiveRun = Record;
	ActiveRun.bHasActiveRunInventorySession = true;
	TestTrue(TEXT("Mid-Run migration is rejected before source writes"),
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, ActiveRun, TargetContent()).Error
			== Edemo_mapShanmenItemMigrationError::ActiveRunUnsupported);
	TestTrue(TEXT("Conflict checks leave original source unchanged"),
		Record.RepositorySnapshot.Items.Find(MigrationGuid(101))
		&& Record.RepositorySnapshot.Items.Find(MigrationGuid(101))->Quantity == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsMigrationAtomicLoadTest,
	"Shanmen.0_0_10.Items.Migration.AtomicLoadIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsMigrationAtomicLoadTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Legacy source fixture builds"),
		BuildLegacyFixture(Profile, Record, Error));
	const Fdemo_mapShanmenItemMigrationResult Migration =
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, Record, TargetContent());
	FShanmenItemRepository Repository;
	EShanmenItemTransactionError LoadError =
		EShanmenItemTransactionError::None;
	TestTrue(TEXT("Validated migration candidate loads"),
		Migration.IsSuccess()
		&& Repository.TryLoadSnapshot(Migration.Candidate, &LoadError));
	const FShanmenItemAuthoritySnapshot Before = Repository.CaptureSnapshot();

	FShanmenItemAuthoritySnapshot Corrupt = Migration.Candidate;
	FShanmenItemInstance* ChildOwner =
		Corrupt.Items.FindByPredicate([](const FShanmenItemInstance& Item)
		{
			return Item.ChildContainerId.IsValid();
		});
	if (ChildOwner)
	{
		ChildOwner->ChildContainerId = ChildOwner->ParentContainerId;
	}
	TestFalse(TEXT("Invalid child-container cycle cannot replace authority"),
		ChildOwner && Repository.TryLoadSnapshot(Corrupt, &LoadError));
	TestTrue(TEXT("Failed load preserves the exact previous authority"),
		Repository.CaptureSnapshot() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsPersistedMigrationOpenTest,
	"Shanmen.0_0_10.Items.Migration.PersistedIdempotentOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsPersistedMigrationOpenTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Legacy source fixture builds"),
		BuildLegacyFixture(Profile, Record, Error));
	const Fdemo_mapPersistentProfile ProfileBefore = Profile;
	const FCodeBSnapshot CodeBBefore = Record.RepositorySnapshot;
	const Fdemo_mapShanmenItemMigrationResult Migration =
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, Record, TargetContent());
	TestTrue(TEXT("Legacy migration candidate is valid"),
		Migration.IsSuccess());

	const FString Root = NewMigrationRoot(TEXT("PersistedAuthority"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, Profile.ProfileId);
	FShanmenItemAuthorityStore Store;
	const FShanmenItemOpenResult Created =
		Store.OpenOrCreateFromMigration(
			Migration.Candidate,
			Migration.Receipt.ToPersistenceEvidence(),
			Storage);
	TestTrue(TEXT("Validated migration atomically publishes generation one"),
		Created.Status == EShanmenItemOpenStatus::CreatedFromMigration
		&& Created.Document.SaveGeneration == 1);

	TArray<uint8> BeforeReopen;
	TestTrue(TEXT("Published authority bytes are readable"),
		ReadBytes(Storage.PrimaryPath(), BeforeReopen));
	const FShanmenItemOpenResult Reopened =
		Store.OpenOrCreateFromMigration(
			Migration.Candidate,
			Migration.Receipt.ToPersistenceEvidence(),
			Storage);
	TArray<uint8> AfterReopen;
	TestTrue(TEXT("Same migration reopens the durable authority read-only"),
		Reopened.Status == EShanmenItemOpenStatus::OpenedExisting
		&& Reopened.Document == Created.Document
		&& ReadBytes(Storage.PrimaryPath(), AfterReopen)
		&& AfterReopen == BeforeReopen);
	TestTrue(TEXT("Publish and reopen never mutate Code A or Code B"),
		Profile == ProfileBefore
		&& Record.RepositorySnapshot == CodeBBefore);

	IFileManager::Get().DeleteDirectory(*Root, false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemsAuthorizedLifecycleHandoffTest,
	"Shanmen.0_0_10.Items.Migration.AuthorizedLifecycleHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemsAuthorizedLifecycleHandoffTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Lifecycle handoff source fixture builds"),
		BuildLegacyFixture(Profile, Record, Error));
	const Fdemo_mapPersistentProfile ProfileBefore = Profile;
	const FCodeBSnapshot CodeBBefore = Record.RepositorySnapshot;
	const Fdemo_mapShanmenItemMigrationResult Migration =
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			Profile, Record, TargetContent());
	TestTrue(TEXT("Lifecycle handoff candidate is valid"),
		Migration.IsSuccess());

	const FString Root = NewMigrationRoot(TEXT("AuthorizedLifecycle"));
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, Profile.ProfileId);
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Absent authority is detected without importing"),
		Service.StartExisting(Storage).Status
			== EShanmenItemAuthorityStartStatus::MigrationRequired
		&& !IFileManager::Get().FileExists(*Storage.PrimaryPath()));

	const FShanmenItemMigrationEvidence Evidence =
		Migration.Receipt.ToPersistenceEvidence();
	const FShanmenItemMigrationAuthorization Authorization =
		FShanmenItemMigrationAuthorization::Explicit(
			Migration.Receipt.MigrationId);
	const FShanmenItemAuthorityStartResult Started =
		Service.StartFromAuthorizedMigration(
			Storage, Authorization, Migration.Candidate, Evidence);
	FShanmenItemAuthoritySnapshot Installed;
	TestTrue(TEXT("Exact reviewed MigrationId creates the sole authority"),
		Started.Status
			== EShanmenItemAuthorityStartStatus::CreatedFromMigration
		&& Started.DocumentGeneration == 1
		&& Service.TryCaptureSnapshot(Installed)
		&& Installed == Migration.Candidate);
	TestTrue(TEXT("Authorized lifecycle remains read-only to Code A and Code B"),
		Profile == ProfileBefore
		&& Record.RepositorySnapshot == CodeBBefore);

	IFileManager::Get().DeleteDirectory(*Root, false, true);
	return true;
}

#endif
