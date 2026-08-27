#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenItemAuthoritySubsystem.h"

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
	FGuid ProductGuid(uint32 Value)
	{
		return FGuid(
			0xC0DE1400u + Value,
			0x00000010u,
			0x00000001u,
			0x00000001u);
	}

	FString NewProductRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.4.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	void RemoveProductRoot(const FString& Root)
	{
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}

	Fdemo_mapProfileSessionSnapshot ProductSessionSnapshot(
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapProfileSessionSnapshot Snapshot;
		Snapshot.SessionState =
			Edemo_mapProfileSessionState::ReadyForPreparation;
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

	bool BuildProductLegacyFixture(
		Fdemo_mapPersistentProfile& OutProfile,
		FCodeBOutOfRaidInventoryRecord& OutRecord,
		FString& OutError)
	{
		Fdemo_mapProfileRepository ProfileRepository;
		OutProfile = ProfileRepository.CreateFreshProfile();
		OutProfile.SaveGeneration = 14;
		Fdemo_mapPersistentItemRecord* Ring =
			OutProfile.PermanentStash.FindByPredicate([](
				const Fdemo_mapPersistentItemRecord& Item)
			{
				return Item.ItemDefinitionId
					== Fdemo_mapItemIds::WindTalisman;
			});
		if (!Ring)
		{
			OutError = TEXT("Fresh Profile has no WindTalisman fixture.");
			return false;
		}
		OutProfile.PreparationLayout.SpatialRingItemInstanceId =
			Ring->ItemInstanceId;

		Fdemo_mapPersistentItemRecord Child;
		Child.ItemInstanceId = ProductGuid(101);
		Child.ItemDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		Child.StackCount = 2;
		Child.PersistentDomain =
			Edemo_mapPersistentDomain::PermanentStash;
		Child.LegacySpatialParentItemInstanceId = Ring->ItemInstanceId;
		OutProfile.PermanentStash.Add(Child);
		if (!ProfileRepository.ValidateProfile(OutProfile, &OutError))
		{
			return false;
		}

		const FString CodeBRoot = NewProductRoot(TEXT("LegacyCodeB"));
		Fdemo_map0909BSectWarehouseService Warehouse;
		Fdemo_map0909BWarehousePresentation Presentation;
		if (!Warehouse.OpenForSect(
			CodeBRoot,
			ProductSessionSnapshot(OutProfile),
			Edemo_map0909BTopState::AtSect,
			Presentation,
			OutError)
			|| !Warehouse.CaptureStableItemMigrationRecord(
				OutRecord, OutError))
		{
			RemoveProductRoot(CodeBRoot);
			return false;
		}
		RemoveProductRoot(CodeBRoot);
		return true;
	}

	struct FProductGameInstanceFixture
	{
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* ProfileSession = nullptr;

		bool Start(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P1.4 fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate the P1.4 GameInstance."));
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

		~FProductGameInstanceFixture()
		{
			Stop();
		}
	};

	const FShanmenItemInstance* FindQuantityItem(
		const FShanmenItemAuthoritySnapshot& Snapshot)
	{
		for (const FShanmenItemInstance& Item : Snapshot.Items)
		{
			const FShanmenItemDefinition* Definition =
				Snapshot.Definitions.FindByPredicate(
					[&Item](const FShanmenItemDefinition& Candidate)
					{
						return Candidate.DefinitionId == Item.DefinitionId;
					});
			if (Definition
				&& Definition->Supports(
					EShanmenItemResourceKind::Quantity)
				&& Item.Quantity > 0)
			{
				return &Item;
			}
		}
		return nullptr;
	}

	FShanmenItemReserveRequest ProductReserve(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemInstance& Item,
		uint32 Sequence)
	{
		FShanmenItemReserveRequest Request;
		Request.Context.RunId = Item.RunId;
		Request.Context.OwnerId = Item.OwnerId;
		Request.Context.RequestId = ProductGuid(300 + Sequence);
		Request.Context.Content = Snapshot.Content;
		Request.ItemInstanceId = Item.ItemInstanceId;
		Request.ResourceKind = EShanmenItemResourceKind::Quantity;
		Request.Amount = 1;
		Request.ExpectedItemRevision = Item.Revision;
		Request.PurposeId = TEXT("Test.ProductAuthority.Command");
		return Request;
	}

	FShanmenItemReservationActionRequest ProductCommit(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& OwnerId,
		const FGuid& RunId,
		const FGuid& ReservationId,
		uint32 Sequence)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context.RunId = RunId;
		Request.Context.OwnerId = OwnerId;
		Request.Context.RequestId = ProductGuid(400 + Sequence);
		Request.Context.Content = Snapshot.Content;
		Request.ReservationId = ReservationId;
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthorityLazyOwnerTest,
	"Shanmen.0_0_10.Items.ProductAuthority.LazyGameInstanceOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthorityLazyOwnerTest::RunTest(const FString&)
{
	const FString NeverBoundRoot = NewProductRoot(TEXT("NeverBound"));
	const FGuid OwnerId = ProductGuid(1);
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(NeverBoundRoot, OwnerId);
	FProductGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	TestTrue(TEXT("GameInstance exposes exactly one lazy product owner"),
		Fixture.Authority
			== Fixture.GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>()
		&& Fixture.Authority->GetLifecycleState()
			== Edemo_mapShanmenItemAuthorityLifecycleState::Unbound
		&& !Fixture.Authority->GetBoundOwnerId().IsValid());
	TestFalse(TEXT("GameInstance initialization performs no authority I/O"),
		IFileManager::Get().FileExists(*Storage.PrimaryPath())
		|| IFileManager::Get().FileExists(*Storage.BackupPath())
		|| IFileManager::Get().FileExists(*Storage.TempPath()));
	Fixture.Stop();
	RemoveProductRoot(NeverBoundRoot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthorityStableMigrationTest,
	"Shanmen.0_0_10.Items.ProductAuthority.StableLegacyBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthorityStableMigrationTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Stable legacy source fixture builds"),
		BuildProductLegacyFixture(Profile, Record, Error));
	const Fdemo_mapPersistentProfile ProfileBefore = Profile;
	const demo_map_code_b::FCodeBSnapshot CodeBBefore =
		Record.RepositorySnapshot;
	const int32 CodeBRevisionBefore = Record.PersistentRevision;
	const FString Root = NewProductRoot(TEXT("StableMigration"));
	const Fdemo_mapProfileStorageContext ProfileStorage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(Root, Profile.ProfileId);
	FProductGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}

	const Fdemo_mapShanmenItemAuthorityBindResult Missing =
		Fixture.Authority->BindExisting(ProfileStorage, Profile.ProfileId);
	TestTrue(TEXT("Missing authority waits without reading or writing legacy"),
		Missing.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::WaitingForStableLegacy
		&& !Missing.bLegacyInputsRead
		&& !IFileManager::Get().FileExists(*Storage.PrimaryPath()));
	const Fdemo_mapShanmenItemAuthorityBindResult Migrated =
		Fixture.Authority->BindFromStableLegacy(
			ProfileStorage, Profile.ProfileId, Profile, Record);
	FShanmenItemAuthoritySnapshot Snapshot;
	TestTrue(TEXT("Exact stable sources publish generation one"),
		Migrated.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::CreatedFromLegacy
		&& Migrated.bLegacyInputsRead
		&& Migrated.MigrationReceipt.IsSuccess()
		&& Migrated.AuthorityStart.DocumentGeneration == 1
		&& Fixture.Authority->TryCaptureSnapshot(Snapshot)
		&& IFileManager::Get().FileExists(*Storage.PrimaryPath()));
	const FShanmenContentStamp ProductContent =
		Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
	TestTrue(TEXT("Published snapshot uses the frozen product content stamp"),
		Snapshot.Content.Version == ProductContent.Version
		&& Snapshot.Content.Digest == ProductContent.Digest);
	TestTrue(TEXT("Binding remains read-only to both legacy sources"),
		Profile == ProfileBefore
		&& Record.PersistentRevision == CodeBRevisionBefore
		&& Record.RepositorySnapshot == CodeBBefore);

	Fixture.Stop();
	RemoveProductRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthorityExistingFirstTest,
	"Shanmen.0_0_10.Items.ProductAuthority.ExistingWinsBeforeLegacyRead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthorityExistingFirstTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Existing-first fixture builds"),
		BuildProductLegacyFixture(Profile, Record, Error));
	const FString Root = NewProductRoot(TEXT("ExistingFirst"));
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	FShanmenItemAuthorityDocument BeforeRestart;
	{
		FProductGameInstanceFixture Initial;
		if (!Initial.Start(*this))
		{
			return false;
		}
		TestTrue(TEXT("Initial authority publishes"),
			Initial.Authority->BindFromStableLegacy(
				Storage, Profile.ProfileId, Profile, Record).IsReady()
			&& Initial.Authority->TryGetDocument(BeforeRestart));
	}

	FProductGameInstanceFixture Restarted;
	if (!Restarted.Start(*this))
	{
		return false;
	}
	const Fdemo_mapPersistentProfile InvalidProfile;
	const FCodeBOutOfRaidInventoryRecord InvalidRecord;
	const Fdemo_mapShanmenItemAuthorityBindResult Opened =
		Restarted.Authority->BindFromStableLegacy(
			Storage,
			Profile.ProfileId,
			InvalidProfile,
			InvalidRecord);
	FShanmenItemAuthorityDocument AfterRestart;
	TestTrue(TEXT("Existing authority wins before malformed legacy is inspected"),
		Opened.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::OpenedExisting
		&& !Opened.bLegacyInputsRead
		&& Restarted.Authority->TryGetDocument(AfterRestart)
		&& AfterRestart == BeforeRestart);

	Restarted.Stop();
	RemoveProductRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthorityLegacyGateTest,
	"Shanmen.0_0_10.Items.ProductAuthority.ActiveRunAndIdentityGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthorityLegacyGateTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Legacy-gate fixture builds"),
		BuildProductLegacyFixture(Profile, Record, Error));
	const FString Root = NewProductRoot(TEXT("LegacyGate"));
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	const FShanmenItemStorageContext AuthorityStorage =
		FShanmenItemStorageContext::ForRoot(Root, Profile.ProfileId);
	FProductGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}

	Record.bHasActiveRunInventorySession = true;
	const Fdemo_mapShanmenItemAuthorityBindResult Active =
		Fixture.Authority->BindFromStableLegacy(
			Storage, Profile.ProfileId, Profile, Record);
	TestTrue(TEXT("Any active legacy Run blocks first-upgrade migration"),
		Active.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::LegacyNotStable
		&& Active.bLegacyInputsRead
		&& !IFileManager::Get().FileExists(*AuthorityStorage.PrimaryPath()));

	Record.bHasActiveRunInventorySession = false;
	const FGuid CorrectOwner = Record.OwnerId;
	Record.OwnerId = FGuid::NewGuid();
	const Fdemo_mapShanmenItemAuthorityBindResult WrongIdentity =
		Fixture.Authority->BindFromStableLegacy(
			Storage, Profile.ProfileId, Profile, Record);
	TestTrue(TEXT("Mismatched legacy identity is rejected without publishing"),
		WrongIdentity.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::LegacyRejected
		&& !IFileManager::Get().FileExists(*AuthorityStorage.PrimaryPath()));
	Record.OwnerId = CorrectOwner;
	TestTrue(TEXT("A later stable exact source may complete the same binding"),
		Fixture.Authority->BindFromStableLegacy(
			Storage, Profile.ProfileId, Profile, Record).IsReady());

	Fixture.Stop();
	RemoveProductRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthorityRecoveryTest,
	"Shanmen.0_0_10.Items.ProductAuthority.BindingAndRecoveryFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthorityRecoveryTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Recovery fixture builds"),
		BuildProductLegacyFixture(Profile, Record, Error));
	const FString Root = NewProductRoot(TEXT("Recovery"));
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	const FShanmenItemStorageContext AuthorityStorage =
		FShanmenItemStorageContext::ForRoot(Root, Profile.ProfileId);
	{
		FProductGameInstanceFixture Initial;
		if (!Initial.Start(*this))
		{
			return false;
		}
		TestTrue(TEXT("Recovery authority publishes"),
			Initial.Authority->BindFromStableLegacy(
				Storage, Profile.ProfileId, Profile, Record).IsReady());
		const FString OtherRoot = NewProductRoot(TEXT("WrongBinding"));
		TestTrue(TEXT("Ready owner rejects a different storage identity"),
			Initial.Authority->BindExisting(
				Fdemo_mapProfileStorageContext::ForRoot(OtherRoot),
				FGuid::NewGuid()).Status
				== Edemo_mapShanmenItemAuthorityBindStatus::BindingMismatch
			&& Initial.Authority->GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready);
		RemoveProductRoot(OtherRoot);
	}

	TestTrue(TEXT("Corrupt fixture replaces the sole primary"),
		FFileHelper::SaveStringToFile(
			TEXT("{not-valid-authority"), *AuthorityStorage.PrimaryPath()));
	IFileManager::Get().Delete(*AuthorityStorage.BackupPath());
	FProductGameInstanceFixture Corrupt;
	if (!Corrupt.Start(*this))
	{
		return false;
	}
	const Fdemo_mapShanmenItemAuthorityBindResult FailedOpen =
		Corrupt.Authority->BindExisting(Storage, Profile.ProfileId);
	const Fdemo_mapShanmenItemAuthorityBindResult NoFallback =
		Corrupt.Authority->BindFromStableLegacy(
			Storage, Profile.ProfileId, Profile, Record);
	TestTrue(TEXT("Corrupt durable authority is sticky recovery, never remigration"),
		(FailedOpen.Status
				== Edemo_mapShanmenItemAuthorityBindStatus::PersistenceFailure
			|| FailedOpen.Status
				== Edemo_mapShanmenItemAuthorityBindStatus::RecoveryRequired)
		&& NoFallback.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::RecoveryRequired
		&& !NoFallback.bLegacyInputsRead
		&& Corrupt.Authority->GetLifecycleState()
			== Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired);

	Corrupt.Stop();
	RemoveProductRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthorityCommandRestartTest,
	"Shanmen.0_0_10.Items.ProductAuthority.DurableCommandsAndRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthorityCommandRestartTest::RunTest(const FString&)
{
	Fdemo_mapPersistentProfile Profile;
	FCodeBOutOfRaidInventoryRecord Record;
	FString Error;
	TestTrue(TEXT("Command fixture builds"),
		BuildProductLegacyFixture(Profile, Record, Error));
	const FString Root = NewProductRoot(TEXT("CommandRestart"));
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(Root);
	FShanmenItemReserveRequest ReserveRequest;
	FShanmenItemReservationActionRequest CommitRequest;
	FShanmenItemTransactionReceipt ReservedReceipt;
	FShanmenItemTransactionReceipt CommittedReceipt;
	FShanmenItemAuthoritySnapshot BeforeRestart;
	{
		FProductGameInstanceFixture Initial;
		if (!Initial.Start(*this))
		{
			return false;
		}
		TestTrue(TEXT("Command authority binds"),
			Initial.Authority->BindFromStableLegacy(
				Storage, Profile.ProfileId, Profile, Record).IsReady());
		FShanmenItemAuthoritySnapshot BeforeCommand;
		TestTrue(TEXT("Command source snapshot is readable"),
			Initial.Authority->TryCaptureSnapshot(BeforeCommand));
		const FShanmenItemInstance* Item = FindQuantityItem(BeforeCommand);
		if (!Item)
		{
			AddError(TEXT("Migration fixture has no quantity-capable item."));
			return false;
		}
		ReserveRequest = ProductReserve(BeforeCommand, *Item, 1);
		const FShanmenItemDurableCommandResult Reserved =
			Initial.Authority->ReserveDurable(ReserveRequest);
		ReservedReceipt = Reserved.Receipt;
		CommitRequest = ProductCommit(
			BeforeCommand,
			Item->OwnerId,
			Item->RunId,
			Reserved.Receipt.ReservationId,
			2);
		const FShanmenItemDurableCommandResult Committed =
			Initial.Authority->CommitDurable(CommitRequest);
		CommittedReceipt = Committed.Receipt;
		TestTrue(TEXT("Product owner reports commands only after durability"),
			Reserved.IsCommandSuccess()
			&& Committed.IsCommandSuccess()
			&& Initial.Authority->TryCaptureSnapshot(BeforeRestart));
	}

	FProductGameInstanceFixture Restarted;
	if (!Restarted.Start(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot AfterRestart;
	TestTrue(TEXT("Restart adopts the exact durable authority"),
		Restarted.Authority->BindExisting(
			Storage, Profile.ProfileId).Status
			== Edemo_mapShanmenItemAuthorityBindStatus::OpenedExisting
		&& Restarted.Authority->TryCaptureSnapshot(AfterRestart)
		&& AfterRestart == BeforeRestart);
	const FShanmenItemDurableCommandResult ReserveReplay =
		Restarted.Authority->ReserveDurable(ReserveRequest);
	const FShanmenItemDurableCommandResult CommitReplay =
		Restarted.Authority->CommitDurable(CommitRequest);
	TestTrue(TEXT("Exact commands replay across GameInstance restart"),
		ReserveReplay.Status == EShanmenItemDurableCommandStatus::Replayed
		&& CommitReplay.Status == EShanmenItemDurableCommandStatus::Replayed
		&& ReserveReplay.Receipt == ReservedReceipt
		&& CommitReplay.Receipt == CommittedReceipt);

	Restarted.Stop();
	RemoveProductRoot(Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenProductAuthoritySourceCaptureTest,
	"Shanmen.0_0_10.Items.ProductAuthority.StableSourceCaptureAdapters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenProductAuthoritySourceCaptureTest::RunTest(const FString&)
{
	const FString Root = NewProductRoot(TEXT("SourceCapture"));
	FProductGameInstanceFixture Fixture;
	if (!Fixture.Start(*this))
	{
		return false;
	}
	Fdemo_mapPersistentProfile CapturedProfile;
	FString Diagnostic;
	TestFalse(TEXT("Uninitialized Profile session exposes no migration source"),
		Fixture.ProfileSession->TryCaptureStableProfileForItemMigration(
			CapturedProfile, &Diagnostic));
	const Fdemo_mapProfileSessionInitializeResult Initialized =
		Fixture.ProfileSession->InitializeSession(
			Fdemo_mapProfileStorageContext::ForRoot(Root));
	TestTrue(TEXT("Ready session exposes one immutable Profile copy"),
		Initialized.IsReady()
		&& Fixture.ProfileSession->TryCaptureStableProfileForItemMigration(
			CapturedProfile, &Diagnostic)
		&& CapturedProfile.ProfileId == Initialized.Snapshot.ProfileId
		&& !CapturedProfile.ActiveRun.bHasActiveRun);

	Fdemo_map0909BSectWarehouseService Warehouse;
	Fdemo_map0909BWarehousePresentation Presentation;
	FCodeBOutOfRaidInventoryRecord CapturedCodeB;
	TestTrue(TEXT("Opened warehouse exposes one stable read-only Code B copy"),
		Warehouse.OpenForSect(
			Root,
			ProductSessionSnapshot(CapturedProfile),
			Edemo_map0909BTopState::AtSect,
			Presentation,
			Diagnostic)
		&& Warehouse.CaptureStableItemMigrationRecord(
			CapturedCodeB, Diagnostic)
		&& CapturedCodeB.OwnerId == CapturedProfile.ProfileId
		&& !CapturedCodeB.bHasActiveRunInventorySession);

	Fixture.Stop();
	RemoveProductRoot(Root);
	return true;
}

#endif
