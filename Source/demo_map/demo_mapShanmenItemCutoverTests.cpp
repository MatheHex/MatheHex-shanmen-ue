#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenItemCutover.h"

#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewCutoverRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.5.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	void RemoveCutoverRoot(const FString& Root)
	{
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	bool MakeLegalItemMutation(Fdemo_mapPersistentProfile& Profile)
	{
		const Fdemo_mapPersistentItemRecord* Blade =
			Profile.PermanentStash.FindByPredicate(
				[](const Fdemo_mapPersistentItemRecord& Item)
				{
					return Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade;
				});
		if (!Blade || !Blade->ItemInstanceId.IsValid()
			|| Profile.PreparationLayout.WeaponItemInstanceId == Blade->ItemInstanceId)
		{
			return false;
		}
		Profile.PreparationLayout.WeaponItemInstanceId = Blade->ItemInstanceId;
		return true;
	}

	struct FCutoverGameInstanceFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* ProfileSession = nullptr;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P1.5 fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate the P1.5 GameInstance."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			ProfileSession = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			return Authority && ProfileSession;
		}

		void Stop()
		{
			if (!GameInstance)
			{
				return;
			}
			GameInstance->Shutdown();
			Authority = nullptr;
			ProfileSession = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
		}

		~FCutoverGameInstanceFixture()
		{
			Stop();
		}
	};

	bool OpenStableSources(
		FAutomationTestBase& Test,
		const FString& Root,
		FCutoverGameInstanceFixture& Fixture,
		Fdemo_mapProfileStorageContext& OutStorage,
		Fdemo_mapPersistentProfile& OutProfile,
		Fdemo_map0909BSectWarehouseService& OutWarehouse,
		FCodeBOutOfRaidInventoryRecord& OutCodeB)
	{
		OutStorage = Fdemo_mapProfileStorageContext::ForRoot(Root);
		const Fdemo_mapProfileSessionInitializeResult Initialized =
			Fixture.ProfileSession->InitializeSession(OutStorage);
		FString Diagnostic;
		if (!Initialized.IsReady()
			|| !Fixture.ProfileSession->TryCaptureStableProfileForItemMigration(
				OutProfile, &Diagnostic))
		{
			Test.AddError(FString::Printf(
				TEXT("Stable Profile fixture failed: %s"), *Diagnostic));
			return false;
		}
		Fdemo_map0909BWarehousePresentation Presentation;
		if (!OutWarehouse.OpenForSect(
			Root,
			Initialized.Snapshot,
			Edemo_map0909BTopState::AtSect,
			Presentation,
			Diagnostic)
			|| !OutWarehouse.CaptureStableItemMigrationRecord(
				OutCodeB, Diagnostic))
		{
			Test.AddError(FString::Printf(
				TEXT("Stable Code B fixture failed: %s"), *Diagnostic));
			return false;
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemCutoverRetiresLegacyWritersTest,
	"Shanmen.0_0_10.Items.Cutover.StablePublishRetiresLegacyWriters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemCutoverRetiresLegacyWritersTest::RunTest(const FString&)
{
	const FString Root = NewCutoverRoot(TEXT("RetireWriters"));
	FCutoverGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Fdemo_mapProfileStorageContext Storage;
	Fdemo_mapPersistentProfile Profile;
	Fdemo_map0909BSectWarehouseService Warehouse;
	FCodeBOutOfRaidInventoryRecord CodeB;
	if (!OpenStableSources(
		*this, Root, Fixture, Storage, Profile, Warehouse, CodeB))
	{
		RemoveCutoverRoot(Root);
		return false;
	}

	demo_map_code_b::FCodeBRepository LegacyRepository;
	demo_map_code_b::FCodeBP2PlayerLayout LegacyLayout;
	FCodeBOutOfRaidProfileStore LegacyStore(Root, Profile.ProfileId);
	TestTrue(TEXT("Independent legacy Code B writer opens before cutover"),
		LegacyStore.OpenOrMigrate(
			Fixture.ProfileSession->GetSnapshot(),
			LegacyRepository,
			LegacyLayout).bSuccess);
	TArray<uint8> CodeBBefore;
	TestTrue(TEXT("Code B bytes exist before cutover"),
		ReadBytes(LegacyStore.GetPrimaryPath(), CodeBBefore));

	const Fdemo_mapShanmenItemCutoverResult Cutover =
		Fdemo_mapShanmenItemCutoverCoordinator::Execute(
			Storage,
			Profile.ProfileId,
			*Fixture.Authority,
			*Fixture.ProfileSession,
			Warehouse);
	FString FenceDiagnostic;
	TestTrue(TEXT("Stable sources publish once and close the shared fence"),
		Cutover.Status
			== Edemo_mapShanmenItemCutoverStatus::CreatedFromLegacy
		&& Cutover.IsReady()
		&& Cutover.bProfileSourceRead
		&& Cutover.bCodeBSourceRead
		&& Cutover.SourceProfileGeneration == Profile.SaveGeneration
		&& Cutover.SourceCodeBPersistentRevision
			== CodeB.PersistentRevision
		&& Cutover.Fence.IsRetired()
		&& Fixture.ProfileSession->AreLegacyItemWritesRetired(
			&FenceDiagnostic)
		&& Warehouse.AreLegacyItemWritesRetired(&FenceDiagnostic));

	Fdemo_mapProfileRepository ProfileRepository;
	Fdemo_mapPersistentProfile IllegalItemWrite = Profile;
	if (!MakeLegalItemMutation(IllegalItemWrite))
	{
		AddError(TEXT("Fresh Profile has no legal preparation mutation fixture."));
		return false;
	}
	const Fdemo_mapProfileSaveResult RejectedProfile =
		ProfileRepository.SaveProfile(IllegalItemWrite, Storage);
	TestTrue(TEXT("Profile item mutation is rejected before temporary I/O"),
		!RejectedProfile.IsSuccess()
		&& !RejectedProfile.bDiskStateChanged
		&& RejectedProfile.Diagnostic.Contains(TEXT("retired")));

	FString CodeBError;
	const bool bCodeBWrite = LegacyStore.CommitAcceptedSnapshot(
		LegacyRepository.CaptureSnapshot(), &CodeBError);
	TArray<uint8> CodeBAfter;
	TestTrue(TEXT("Every Code B writer is fenced at its sole SaveRecord gate"),
		!bCodeBWrite
		&& CodeBError.Contains(TEXT("retired"))
		&& ReadBytes(LegacyStore.GetPrimaryPath(), CodeBAfter)
		&& CodeBAfter == CodeBBefore
		&& LegacyStore.GetPersistentRevision() == CodeB.PersistentRevision);

	Fdemo_mapPersistentProfile CurrencyOnly = Profile;
	++CurrencyOnly.PersistentSpiritStones;
	const Fdemo_mapProfileSaveResult CurrencySaved =
		ProfileRepository.SaveProfile(CurrencyOnly, Storage);
	Fdemo_mapProfileLoadResult Reloaded =
		ProfileRepository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Non-item Profile progression remains writable"),
		CurrencySaved.IsSuccess()
		&& Reloaded.IsSuccess()
		&& Reloaded.Profile.PersistentSpiritStones
			== Profile.PersistentSpiritStones + 1
		&& Fdemo_mapShanmenLegacyItemWriteFence::PreservesRetiredProfileItems(
			Profile, Reloaded.Profile));

	const Fdemo_mapShanmenItemCutoverResult Replayed =
		Fdemo_mapShanmenItemCutoverCoordinator::Execute(
			Storage,
			Profile.ProfileId,
			*Fixture.Authority,
			*Fixture.ProfileSession,
			Warehouse);
	TestTrue(TEXT("Ready authority replay never rereads changed legacy files"),
		Replayed.Status == Edemo_mapShanmenItemCutoverStatus::AlreadyReady
		&& !Replayed.bProfileSourceRead
		&& !Replayed.bCodeBSourceRead
		&& Replayed.Fence.IsRetired());

	Fixture.Stop();
	RemoveCutoverRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemCutoverMissingSourcesTest,
	"Shanmen.0_0_10.Items.Cutover.MissingSourceDoesNotRetire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemCutoverMissingSourcesTest::RunTest(const FString&)
{
	const FString Root = NewCutoverRoot(TEXT("MissingSource"));
	FCutoverGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Fdemo_map0909BSectWarehouseService UnopenedWarehouse;
	const FGuid OwnerId = FGuid::NewGuid();
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	const Fdemo_mapShanmenItemCutoverResult Result =
		Fdemo_mapShanmenItemCutoverCoordinator::Execute(
			Storage,
			OwnerId,
			*Fixture.Authority,
			*Fixture.ProfileSession,
			UnopenedWarehouse);
	const Fdemo_mapShanmenLegacyItemWriteFenceProbe Fence =
		Fdemo_mapShanmenLegacyItemWriteFence::Inspect(Root, OwnerId);
	TestTrue(TEXT("Missing stable source leaves both authorities untouched"),
		Result.Status
			== Edemo_mapShanmenItemCutoverStatus::LegacyNotStable
		&& !Result.bProfileSourceRead
		&& !Result.bCodeBSourceRead
		&& !Fence.IsRetired()
		&& Fixture.Authority->GetLifecycleState()
			== Edemo_mapShanmenItemAuthorityLifecycleState::WaitingForStableLegacy);

	Fixture.Stop();
	RemoveCutoverRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemCutoverRecoveryFenceTest,
	"Shanmen.0_0_10.Items.Cutover.CorruptAuthorityStillRetiresLegacy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemCutoverRecoveryFenceTest::RunTest(const FString&)
{
	const FString Root = NewCutoverRoot(TEXT("RecoveryFence"));
	FCutoverGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Fdemo_mapProfileStorageContext Storage;
	Fdemo_mapPersistentProfile Profile;
	Fdemo_map0909BSectWarehouseService Warehouse;
	FCodeBOutOfRaidInventoryRecord CodeB;
	if (!OpenStableSources(
		*this, Root, Fixture, Storage, Profile, Warehouse, CodeB))
	{
		RemoveCutoverRoot(Root);
		return false;
	}

	demo_map_code_b::FCodeBRepository LegacyRepository;
	demo_map_code_b::FCodeBP2PlayerLayout LegacyLayout;
	FCodeBOutOfRaidProfileStore LegacyStore(Root, Profile.ProfileId);
	TestTrue(TEXT("Legacy Code B recovery fixture opens"),
		LegacyStore.OpenOrMigrate(
			Fixture.ProfileSession->GetSnapshot(),
			LegacyRepository,
			LegacyLayout).bSuccess);
	const FShanmenItemStorageContext AuthorityStorage =
		FShanmenItemStorageContext::ForRoot(Root, Profile.ProfileId);
	IFileManager::Get().MakeDirectory(
		*FPaths::GetPath(AuthorityStorage.PrimaryPath()), true);
	TestTrue(TEXT("Corrupt authority marker is written for recovery fixture"),
		FFileHelper::SaveStringToFile(
			TEXT("{corrupt-authority"), *AuthorityStorage.PrimaryPath()));

	const Fdemo_mapShanmenItemCutoverResult Result =
		Fdemo_mapShanmenItemCutoverCoordinator::Execute(
			Storage,
			Profile.ProfileId,
			*Fixture.Authority,
			*Fixture.ProfileSession,
			Warehouse);
	TestTrue(TEXT("Corrupt new authority enters recovery without legacy fallback"),
		Result.Status
			== Edemo_mapShanmenItemCutoverStatus::RecoveryRequired
		&& !Result.bProfileSourceRead
		&& !Result.bCodeBSourceRead
		&& Result.Fence.IsRetired()
		&& Fixture.Authority->GetLifecycleState()
			== Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired);

	Fdemo_mapProfileRepository ProfileRepository;
	Fdemo_mapPersistentProfile IllegalProfile = Profile;
	if (!MakeLegalItemMutation(IllegalProfile))
	{
		AddError(TEXT("Recovery Profile has no legal preparation mutation fixture."));
		return false;
	}
	const Fdemo_mapProfileSaveResult ProfileWrite =
		ProfileRepository.SaveProfile(IllegalProfile, Storage);
	FString CodeBError;
	const bool bCodeBWrite = LegacyStore.CommitAcceptedSnapshot(
		LegacyRepository.CaptureSnapshot(), &CodeBError);
	TestTrue(TEXT("Recovery marker keeps both legacy durable writers closed"),
		!ProfileWrite.IsSuccess()
		&& !ProfileWrite.bDiskStateChanged
		&& !bCodeBWrite
		&& CodeBError.Contains(TEXT("retired")));

	Fixture.Stop();
	RemoveCutoverRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenItemCutoverRestartPresentationTest,
	"Shanmen.0_0_10.Items.Cutover.RestartSourcesAreReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenItemCutoverRestartPresentationTest::RunTest(const FString&)
{
	const FString Root = NewCutoverRoot(TEXT("RestartReadOnly"));
	Fdemo_mapProfileStorageContext Storage;
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord CodeB;
	{
		FCutoverGameInstanceFixture Initial;
		if (!Initial.Start(*this))
		{
			return false;
		}
		Fdemo_map0909BSectWarehouseService Warehouse;
		if (!OpenStableSources(
			*this, Root, Initial, Storage, Profile, Warehouse, CodeB))
		{
			RemoveCutoverRoot(Root);
			return false;
		}
		TestTrue(TEXT("Initial cutover succeeds"),
			Fdemo_mapShanmenItemCutoverCoordinator::Execute(
				Storage,
				Profile.ProfileId,
				*Initial.Authority,
				*Initial.ProfileSession,
				Warehouse).IsReady());
	}

	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Restart fixture captures legacy Profile bytes"),
		ReadBytes(Storage.PrimaryPath(), ProfileBefore));
	FCutoverGameInstanceFixture Restarted;
	if (!Restarted.Start(*this))
	{
		return false;
	}
	const Fdemo_mapProfileSessionInitializeResult Initialized =
		Restarted.ProfileSession->InitializeSession(Storage);
	Fdemo_map0909BSectWarehouseService Warehouse;
	Fdemo_map0909BWarehousePresentation Presentation;
	FString Diagnostic;
	const bool bWarehouseOpen = Warehouse.OpenForSect(
		Root,
		Initialized.Snapshot,
		Edemo_map0909BTopState::AtSect,
		Presentation,
		Diagnostic);
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Restart keeps legacy sources readable but not writable"),
		Initialized.IsReady()
		&& Restarted.ProfileSession->AreLegacyItemWritesRetired(&Diagnostic)
		&& bWarehouseOpen
		&& Presentation.bOpen
		&& !Presentation.bCanWrite
		&& Warehouse.AreLegacyItemWritesRetired(&Diagnostic)
		&& ReadBytes(Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);

	const Fdemo_mapShanmenItemCutoverResult Existing =
		Fdemo_mapShanmenItemCutoverCoordinator::Execute(
			Storage,
			Profile.ProfileId,
			*Restarted.Authority,
			*Restarted.ProfileSession,
			Warehouse);
	TestTrue(TEXT("Restart opens authority before reading either legacy source"),
		Existing.Status
			== Edemo_mapShanmenItemCutoverStatus::OpenedExisting
		&& !Existing.bProfileSourceRead
		&& !Existing.bCodeBSourceRead
		&& Existing.Fence.IsRetired());

	Restarted.Stop();
	RemoveCutoverRoot(Root);
	return true;
}

#endif
