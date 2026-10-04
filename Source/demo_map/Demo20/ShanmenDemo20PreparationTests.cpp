#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Catalog.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenItemRepository.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Algo/Reverse.h"

namespace
{
	constexpr auto PrepFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	FShanmenItemStorageContext Isolated(const TCHAR* Label)
	{
		return FShanmenItemStorageContext::ForRoot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Demo20.M1.Preparation"),
			Label, FGuid::NewGuid().ToString(EGuidFormats::Digits)), FShanmenDemo20Catalog::OwnerId());
	}
	const FShanmenItemInstance* Item(const FShanmenItemAuthoritySnapshot& S, FName Definition)
	{
		return S.Items.FindByPredicate([&](const auto& I) { return I.DefinitionId == Definition && I.State == EShanmenItemInstanceState::Stored; });
	}
	FShanmenItemGridRequest Edit(const FShanmenItemAuthoritySnapshot& S, FName Definition, FName Container, int32 X = 0, int32 Y = 0)
	{
		FShanmenItemGridRequest R; R.Context.OwnerId = FShanmenDemo20Catalog::OwnerId(); R.Context.RunId = FShanmenDemo20Catalog::ScopeId();
		R.Context.Content = S.Content; R.Context.RequestId = FGuid::NewGuid(); R.ExpectedAuthorityRevision = S.AuthorityRevision;
		if (const auto* I = Item(S,Definition)) { R.ItemInstanceId = I->ItemInstanceId; R.ExpectedItemRevision = I->Revision; }
		R.DestinationContainerId = FShanmenDemo20Catalog::ContainerId(Container); R.X = X; R.Y = Y;
		return R;
	}
	FShanmenItemGridRequest Equip(const FShanmenItemAuthoritySnapshot& S, FName Definition)
	{
		const auto* Footprint = S.Grid.Footprints.FindByPredicate([&](const auto& F) { return F.DefinitionId == Definition; });
		auto R = Edit(S,Definition,Footprint->EquipmentRole); R.Action = EShanmenItemGridAction::Equip;
		const auto* C = S.Containers.FindByPredicate([&](const auto& Value) { return Value.ContainerId == R.DestinationContainerId; });
		if (C->Slots[0].IsValid())
		{
			const auto* Target = S.Items.FindByPredicate([&](const auto& Value) { return Value.ItemInstanceId == C->Slots[0]; });
			R.ExpectedTargetRevision = Target->Revision;
		}
		return R;
	}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20NativeGenesisTest,"Shanmen.Demo20.Preparation.NativeGenesisNoRegrant",PrepFlags)
bool FDemo20NativeGenesisTest::RunTest(const FString&)
{
	const auto Initial = FShanmenDemo20Catalog::Initial();
	TestEqual(TEXT("13 immutable definitions"),Initial.Definitions.Num(),13);
	TestTrue(TEXT("Nonempty real storage geometry valid"),FShanmenItemGridPolicy::Validate(Initial));
	const auto Disk = Isolated(TEXT("Genesis")); FShanmenItemAuthorityService Service;
	const auto Start = Service.StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),Initial);
	TestTrue(TEXT("Native creation ready and durable"),Start.Status == EShanmenItemAuthorityStartStatus::CreatedNewProfile && Start.IsReady());
	FShanmenItemAuthorityDocument Before; if (!Service.TryGetDocument(Before)) return false;
	TestTrue(TEXT("No invented legacy schema or CodeB revision"),Before.Migration.IsNativeProfileGenesis()
		&& Before.Migration.SourceProfileSchema == 0 && Before.Migration.SourceCodeBPersistentRevision == 0);
	const auto Move = Edit(Before.Authority,TEXT("Material.Herb"),TEXT("Carry"),2,1);
	TestTrue(TEXT("Real persisted item edit"),Service.EditGridDurable(Move).IsCommandSuccess());
	FShanmenItemAuthorityDocument Committed; Service.TryGetDocument(Committed);
	TArray<uint8> BytesBefore,BytesAfter; FFileHelper::LoadFileToArray(BytesBefore,*Disk.PrimaryPath());
	FShanmenItemAuthorityService Restart;
	const auto Reopen = Restart.StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial(999999999));
	FShanmenItemAuthorityDocument Reopened; Restart.TryGetDocument(Reopened); FFileHelper::LoadFileToArray(BytesAfter,*Disk.PrimaryPath());
	TestTrue(TEXT("Changed configured starting currency cannot regrant or rewrite"),Reopen.IsReady() && !Reopen.bDiskStateChanged
		&& Reopened == Committed && BytesBefore == BytesAfter && Item(Reopened.Authority,TEXT("Currency.Test"))->Quantity == 1000000);
	TestTrue(TEXT("Accepted grid retry survives native restart without additional mutation"),Restart.EditGridDurable(Move).Receipt.IsSuccess());
	FShanmenItemAuthorityDocument Replay; Restart.TryGetDocument(Replay); TestTrue(TEXT("Replay exact snapshot/generation"),Replay == Committed);
	auto Reordered = Initial;
	Algo::Reverse(Reordered.Definitions); Algo::Reverse(Reordered.Grid.Footprints); Algo::Reverse(Reordered.Grid.StorageDefinitions);
	TestTrue(TEXT("Ready binding compares canonical catalog not input array order"),Restart.StartNativeProfile(Disk,
		FShanmenDemo20Catalog::ProductId(),Reordered).IsReady());
	FShanmenItemAuthorityService ReorderedRestart;
	TestTrue(TEXT("Fresh process accepts reordered equivalent catalog"),ReorderedRestart.StartNativeProfile(Disk,
		FShanmenDemo20Catalog::ProductId(),Reordered).IsReady());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20StorageTest,"Shanmen.Demo20.Preparation.StorageSwapOverflowAndUnequip",PrepFlags)
bool FDemo20StorageTest::RunTest(const FString&)
{
	FShanmenItemRepository Repo; TestTrue(TEXT("Load"),Repo.TryLoadSnapshot(FShanmenDemo20Catalog::Initial()));
	const auto Before = Repo.CaptureSnapshot(); const auto LargeRequest = Equip(Before,TEXT("Backpack.Large"));
	TestTrue(TEXT("Atomic bag replacement"),Repo.EditGrid(LargeRequest).IsSuccess());
	auto S = Repo.CaptureSnapshot();
	const auto* Carry = S.Grid.Layouts.FindByPredicate([](const auto& L) { return L.Kind == EShanmenItemGridKind::Carry; });
	TestTrue(TEXT("Equipped large capacity 8x5"),Carry && Carry->Width == 8 && Carry->Height == 5);
	TestTrue(TEXT("Old bag moved to new bag's original stash anchor, quantity preserved"),Item(S,TEXT("Backpack.Small"))->ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
		&& S.Items.Num() == Before.Items.Num() && Item(S,TEXT("Backpack.Small"))->SlotIndex == Item(Before,TEXT("Backpack.Large"))->SlotIndex);
	TestTrue(TEXT("Item at large-only coordinate"),Repo.EditGrid(Edit(S,TEXT("Material.Herb"),TEXT("Carry"),7,3)).IsSuccess());
	S = Repo.CaptureSnapshot();
	TestEqual(TEXT("Shrinking refuses overflow"),Repo.EditGrid(Equip(S,TEXT("Backpack.Small"))).Error,EShanmenItemTransactionError::GridNoSpace);
	TestTrue(TEXT("Failed shrink preserves whole snapshot incl equipment/ledger/quantity"),Repo.CaptureSnapshot() == S);
	TestEqual(TEXT("Unequip nonempty bag also refused"),Repo.EditGrid(Edit(S,TEXT("Backpack.Large"),TEXT("Stash"),10,4)).Error,EShanmenItemTransactionError::GridNoSpace);
	TestTrue(TEXT("No ghost cleared contents"),Repo.CaptureSnapshot() == S);
	TestTrue(TEXT("Manually make room"),Repo.EditGrid(Edit(S,TEXT("Material.Herb"),TEXT("Carry"),2,2)).IsSuccess());
	S = Repo.CaptureSnapshot(); TestTrue(TEXT("Shrink after rearranging accepted"),Repo.EditGrid(Equip(S,TEXT("Backpack.Small"))).IsSuccess());
	S = Repo.CaptureSnapshot(); const auto* Herb = Item(S,TEXT("Material.Herb"));
	TestTrue(TEXT("Coordinates retained after width reanchor"),Herb->SlotIndex == 14 && Herb->Quantity == 12 && FShanmenItemGridPolicy::Validate(S));
	auto Tampered = S; Tampered.Grid.Layouts.FindByPredicate([](const auto& L) {return L.Kind == EShanmenItemGridKind::Carry;})->Width = 8;
	TestFalse(TEXT("Client cannot enlarge geometry independent of equipment"),FShanmenItemGridPolicy::Validate(Tampered));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20NativeFailureTest,"Shanmen.Demo20.Preparation.NativeFailureAndOriginIsolation",PrepFlags)
bool FDemo20NativeFailureTest::RunTest(const FString&)
{
	const auto Initial = FShanmenDemo20Catalog::Initial(); auto Disk = Isolated(TEXT("Failure"));
	Disk.InjectedFailure = EShanmenItemStoreFailureStage::AtomicReplace;
	FShanmenItemAuthorityService Failed;
	TestFalse(TEXT("Publication failure not ready"),Failed.StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),Initial).IsReady());
	FShanmenItemAuthoritySnapshot None; TestFalse(TEXT("No uncommitted initial quantities exposed"),Failed.TryCaptureSnapshot(None));
	Disk.InjectedFailure = EShanmenItemStoreFailureStage::None;
	FShanmenItemAuthorityService Good; TestTrue(TEXT("Safe startup retry"),Good.StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),Initial).IsReady());
	FShanmenItemAuthorityDocument Saved; Good.TryGetDocument(Saved);
	FShanmenItemAuthorityStore Store;
	TestEqual(TEXT("Legacy migration port cannot import native marker"),Store.OpenOrCreateFromMigration(Initial,Saved.Migration,Disk).Status,EShanmenItemOpenStatus::InvalidRequest);
	TestEqual(TEXT("Other product cannot adopt same owner file"),Store.OpenOrCreateNativeProfile(TEXT("Other.Product"),Initial,Disk).Status,EShanmenItemOpenStatus::MigrationConflict);
	FShanmenItemAuthorityService Other;
	TestFalse(TEXT("Service also enforces origin"),Other.StartNativeProfile(Disk,TEXT("Other.Product"),Initial).IsReady());
	auto DifferentRoot = Isolated(TEXT("OtherRoot"));
	TestFalse(TEXT("Ready service cannot rebound"),Good.StartNativeProfile(DifferentRoot,FShanmenDemo20Catalog::ProductId(),Initial).IsReady());
	TestFalse(TEXT("Invalid money rejected not silently clamped"),FShanmenDemo20Catalog::Initial(0).Content.IsValid());
	const auto CorruptDisk = Isolated(TEXT("CorruptWithoutBackup"));
	TestTrue(TEXT("Create separate native file"),Store.OpenOrCreateNativeProfile(FShanmenDemo20Catalog::ProductId(),Initial,CorruptDisk).IsSuccess());
	FFileHelper::SaveStringToFile(TEXT("{corrupt"),*CorruptDisk.PrimaryPath(),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FShanmenItemAuthorityService Corrupt;
	TestFalse(TEXT("Corrupt durable file cannot be silently initialized again"),Corrupt.StartNativeProfile(CorruptDisk,FShanmenDemo20Catalog::ProductId(),Initial).IsReady());
	FString CorruptBytes; FFileHelper::LoadFileToString(CorruptBytes,*CorruptDisk.PrimaryPath());
	TestEqual(TEXT("Corrupt evidence retained"),CorruptBytes,FString(TEXT("{corrupt")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20StorageDurableTest,"Shanmen.Demo20.Preparation.StorageRollbackAndReopen",PrepFlags)
bool FDemo20StorageDurableTest::RunTest(const FString&)
{
	const auto Disk = Isolated(TEXT("StorageRollback")); FShanmenItemAuthorityService Service;
	Service.StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthorityDocument Before; if (!Service.TryGetDocument(Before)) return false;
	const auto Intent = Equip(Before.Authority,TEXT("Backpack.Large"));
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::AtomicReplace);
	const auto Failed = Service.EditGridDurable(Intent);
	TestEqual(TEXT("Save failure rolls back both equipment and capacity"),Failed.Status,EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack);
	FShanmenItemAuthorityDocument After; Service.TryGetDocument(After); TestTrue(TEXT("Exact authority restored"),After == Before);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
	const auto Success = Service.EditGridDurable(Intent); TestTrue(TEXT("Same failed ID may retry safely"),Success.IsCommandSuccess());
	Service.TryGetDocument(After); FShanmenItemAuthorityService Restart;
	TestTrue(TEXT("Reopen"),Restart.StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady());
	const auto Replay = Restart.EditGridDurable(Intent); TestTrue(TEXT("Replay exact receipt"),Replay.Receipt == Success.Receipt);
	FShanmenItemAuthorityDocument Reopened; Restart.TryGetDocument(Reopened); TestTrue(TEXT("Whole state/generation stable"),Reopened == After);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20NativeSubsystemTest,"Shanmen.Demo20.Preparation.NativeSubsystemPort",PrepFlags)
bool FDemo20NativeSubsystemTest::RunTest(const FString&)
{
	auto* GI = NewObject<UGameInstance>(); GI->Init();
	auto* Subsystem = GI->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!TestNotNull(TEXT("Existing sole GameInstance subsystem"),Subsystem)) { GI->Shutdown(); return false; }
	const auto Disk = Isolated(TEXT("Subsystem"));
	const auto Bound = Subsystem->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(Disk.RootDirectory),Disk.OwnerId,
		FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	TestTrue(TEXT("Native ready without legacy reads"),Bound.IsReady() && !Bound.bLegacyInputsRead);
	FShanmenItemAuthoritySnapshot S; Subsystem->TryCaptureSnapshot(S);
	TestTrue(TEXT("Same subsystem grid command"),Subsystem->EditGridDurable(Edit(S,TEXT("Material.Herb"),TEXT("Carry"),3,1)).IsCommandSuccess());
	GI->Shutdown(); return true;
}
#endif
