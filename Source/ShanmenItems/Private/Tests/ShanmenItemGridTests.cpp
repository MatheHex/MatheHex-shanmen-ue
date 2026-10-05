#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenItemAuthorityService.h"
#include "ShanmenItemTags.h"
#include "ShanmenItemStackTransfer.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

namespace
{
	const FGuid Owner(0x20201004, 1, 1, 1), Scope(0x20201004, 2, 1, 1);
	const FGuid Stash(0x20201004, 3, 1, 1), Carry(0x20201004, 3, 2, 1), Secure(0x20201004, 3, 3, 1), Weapon(0x20201004, 3, 4, 1);
	const FGuid MaterialStackAId(0x20201004, 4, 1, 1), MaterialStackBId(0x20201004, 4, 2, 1), Sword(0x20201004, 4, 3, 1);
	constexpr auto Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	FShanmenItemAuthoritySnapshot Fixture(bool Grid = true)
	{
		FShanmenItemAuthoritySnapshot S;
		S.Content.Version = TEXT("Grid.Fixture.v1"); S.Content.Digest = TEXT("Grid.NonzeroInventory.v1");
		FShanmenItemDefinition Material; Material.DefinitionId = TEXT("Material"); Material.MaxStack = 20;
		Material.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityConsumeQuantity());
		FShanmenItemDefinition SwordDefinition; SwordDefinition.DefinitionId = TEXT("Sword");
		SwordDefinition.ItemTags.AddTag(FShanmenItemNativeTags::CapabilityDeploy());
		S.Definitions = {Material, SwordDefinition};
		auto Container = [&](FGuid Id, int32 Width, int32 Height, EShanmenItemGridKind Kind, FName Role = NAME_None)
		{
			FShanmenItemContainer C; C.ContainerId = Id; C.OwnerId = Owner; C.RunId = Scope;
			C.ContainerType = TEXT("Fixture"); C.Slots.SetNum(Width * Height); S.Containers.Add(C);
			FShanmenItemGridLayout L; L.ContainerId = Id; L.Width = Width; L.Height = Height; L.Kind = Kind; L.EquipmentRole = Role;
			if (Grid) { S.Grid.Layouts.Add(L); }
		};
		Container(Stash, 6, 4, EShanmenItemGridKind::Stash); Container(Carry, 6, 4, EShanmenItemGridKind::Carry);
		Container(Secure, 2, 2, EShanmenItemGridKind::Secure); Container(Weapon, 1, 1, EShanmenItemGridKind::Equipment, TEXT("Weapon"));
		auto Item = [&](FGuid Id, FName Definition, int32 Quantity, int32 Anchor)
		{
			FShanmenItemInstance I; I.ItemInstanceId = Id; I.DefinitionId = Definition; I.OwnerId = Owner; I.RunId = Scope;
			I.Quantity = Quantity; I.ParentContainerId = Stash; I.SlotIndex = Anchor;
			S.Items.Add(I); S.Containers[0].Slots[Anchor] = Id;
		};
		Item(MaterialStackAId, TEXT("Material"), 12, 0); Item(MaterialStackBId, TEXT("Material"), 18, 3); Item(Sword, TEXT("Sword"), 1, 12);
		if (Grid)
		{
			FShanmenItemFootprint F; F.DefinitionId = TEXT("Material"); F.Width = 1; F.Height = 2; F.bSecureAllowed = true;
			S.Grid.Footprints.Add(F); F.DefinitionId = TEXT("Sword"); F.Width = 2; F.Height = 1;
			F.bSecureAllowed = false; F.EquipmentRole = TEXT("Weapon"); S.Grid.Footprints.Add(F);
		}
		FShanmenItemRepository R; R.TryLoadSnapshot(S);
		return R.CaptureSnapshot();
	}
	FShanmenItemGridRequest Request(const FShanmenItemRepository& Repo, FGuid Item, FGuid Destination, int32 X = 0, int32 Y = 0)
	{
		FShanmenItemGridRequest R; R.Context.OwnerId = Owner; R.Context.RunId = Scope;
		R.Context.RequestId = FGuid::NewGuid(); R.Context.Content = Repo.CaptureSnapshot().Content;
		R.ItemInstanceId = Item; R.DestinationContainerId = Destination; R.X = X; R.Y = Y;
		R.ExpectedAuthorityRevision = Repo.GetAuthorityRevision();
		if (const auto* I = Repo.FindItem(Item)) { R.ExpectedItemRevision = I->Revision; }
		return R;
	}
	int32 Total(const FShanmenItemAuthoritySnapshot& S)
	{
		int32 Value = 0; for (const auto& I : S.Items) { if (I.DefinitionId == TEXT("Material")) { Value += I.Quantity; } } return Value;
	}
	FShanmenItemMigrationEvidence Evidence(const FShanmenItemAuthoritySnapshot& S)
	{
		FShanmenItemMigrationEvidence E; E.MigrationId = FGuid(0x20201004, 5, 1, 1); E.OwnerId = Owner;
		E.SourceProfileSchema = 7; E.SourceSaveGeneration = 8; E.SourceCodeBPersistentRevision = 3; E.SourceCodeBRepositoryRevision = 4;
		E.DefinitionCount = S.Definitions.Num(); E.ContainerCount = S.Containers.Num(); E.ItemCount = S.Items.Num();
		E.SourceFingerprint = TEXT("Grid.Automation.IsolatedFixture"); E.CandidateDigest = TEXT("Grid.Automation.Candidate"); return E;
	}
	FShanmenItemStorageContext Storage(const TCHAR* Label)
	{
		return FShanmenItemStorageContext::ForRoot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Demo20.M1.Grid"),
			Label, FGuid::NewGuid().ToString(EGuidFormats::Digits)), Owner);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridGeometryTest, "Shanmen.0_0_10.Items.Grid.GeometryAndPreview", Flags)
bool FShanmenGridGeometryTest::RunTest(const FString&)
{
	const auto S = Fixture(); TestTrue(TEXT("Nonempty catalog/geometry is valid"), FShanmenItemGridPolicy::Validate(S));
	TestEqual(TEXT("Unrotated sword over right edge rejected"), FShanmenItemGridPolicy::CanPlace(S, Sword, Carry, 5, 0, false), EShanmenItemTransactionError::GridNoSpace);
	TestEqual(TEXT("Rotated sword fits same right column"), FShanmenItemGridPolicy::CanPlace(S, Sword, Carry, 5, 0, true), EShanmenItemTransactionError::None);
	TestEqual(TEXT("Cannot split into footprint's occupied second cell"), FShanmenItemGridPolicy::CanPlace(S, MaterialStackAId, Stash, 0, 1, false, false), EShanmenItemTransactionError::GridNoSpace);
	auto Invalid = S; Invalid.Grid.Layouts[0].Width = 0;
	TestFalse(TEXT("Invalid zero-width snapshot rejected without divide-by-zero"), FShanmenItemGridPolicy::Validate(Invalid));
	TestEqual(TEXT("Public preview also rejects zero width"), FShanmenItemGridPolicy::CanPlace(Invalid, MaterialStackAId, Invalid.Grid.Layouts[0].ContainerId, 0, 0, false), EShanmenItemTransactionError::InvariantViolation);
	Invalid = S; Invalid.Items[1].SlotIndex = 6; Invalid.Containers[0].Slots[3].Invalidate(); Invalid.Containers[0].Slots[6] = MaterialStackBId;
	FShanmenItemRepository R; TestFalse(TEXT("Anchors distinct but footprints overlap: authority rejects"), R.TryLoadSnapshot(Invalid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridMoveTest, "Shanmen.0_0_10.Items.Grid.MoveRotationReplay", Flags)
bool FShanmenGridMoveTest::RunTest(const FString&)
{
	FShanmenItemRepository R; TestTrue(TEXT("Load valid fixture"), R.TryLoadSnapshot(Fixture()));
	auto Q = Request(R, Sword, Carry, 5, 0); Q.bRotated = true;
	const auto Result = R.EditGrid(Q); TestTrue(TEXT("Move and rotation committed together"), Result.IsSuccess());
	const auto AcceptedRequest = Q;
	const auto After = R.CaptureSnapshot();
	TestTrue(TEXT("Correct destination/anchor/orientation and quantity"), R.FindItem(Sword)->ParentContainerId == Carry
		&& R.FindItem(Sword)->SlotIndex == 5 && After.Grid.RotatedItems.Contains(Sword) && Total(After) == 30);
	TestTrue(TEXT("Exact accepted replay does not mutate"), R.EditGrid(Q) == Result && R.CaptureSnapshot() == After);
	Q.X = 4; TestEqual(TEXT("Changed accepted identity conflicts"), R.EditGrid(Q).Error, EShanmenItemTransactionError::RequestIdConflict);
	Q = Request(R, Sword, Carry, 5, 0); TestEqual(TEXT("Illegal turn rejected"), R.EditGrid(Q).Error, EShanmenItemTransactionError::GridNoSpace);
	TestTrue(TEXT("Rejected placement is entirely unchanged"), R.CaptureSnapshot() == After);
	FShanmenItemRepository Restored; TestTrue(TEXT("Replay survives snapshot reload"), Restored.TryLoadSnapshot(After)
		&& Restored.EditGrid(AcceptedRequest) == Result && Restored.CaptureSnapshot() == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridStacksTest, "Shanmen.0_0_10.Items.Grid.SplitPartialAndFullMerge", Flags)
bool FShanmenGridStacksTest::RunTest(const FString&)
{
	FShanmenItemRepository R; R.TryLoadSnapshot(Fixture()); auto Q = Request(R, MaterialStackAId, Carry);
	Q.Action = EShanmenItemGridAction::Split; Q.Amount = 4;
	const auto Split = R.EditGrid(Q); TestTrue(TEXT("Split creates new identity and preserves total"), Split.IsSuccess()
		&& Split.ItemInstanceId != MaterialStackAId && R.FindItem(MaterialStackAId)->Quantity == 8 && R.FindItem(Split.ItemInstanceId)->Quantity == 4 && Total(R.CaptureSnapshot()) == 30);
	const auto AfterSplit = R.CaptureSnapshot(); TestTrue(TEXT("Split retry creates no second item"), R.EditGrid(Q) == Split && R.CaptureSnapshot() == AfterSplit);
	Q = Request(R, MaterialStackAId, FGuid()); Q.Action = EShanmenItemGridAction::Merge; Q.MergeTargetId = MaterialStackBId;
	Q.Amount = 8; Q.ExpectedTargetRevision = R.FindItem(MaterialStackBId)->Revision;
	const auto Partial = R.EditGrid(Q); TestTrue(TEXT("Only room for two: remainder stays at original anchor"), Partial.IsSuccess() && Partial.Amount == 2
		&& R.FindItem(MaterialStackBId)->Quantity == 20 && R.FindItem(MaterialStackAId)->Quantity == 6 && R.FindItem(MaterialStackAId)->SlotIndex == 0 && Total(R.CaptureSnapshot()) == 30);
	Q = Request(R, Split.ItemInstanceId, FGuid()); Q.Action = EShanmenItemGridAction::Merge; Q.MergeTargetId = MaterialStackAId;
	Q.Amount = 4; Q.ExpectedTargetRevision = R.FindItem(MaterialStackAId)->Revision;
	TestTrue(TEXT("Full merge retains source tombstone, not a duplicate live item"), R.EditGrid(Q).IsSuccess()
		&& R.FindItem(Split.ItemInstanceId)->State == EShanmenItemInstanceState::Depleted && R.FindItem(MaterialStackAId)->Quantity == 10
		&& Total(R.CaptureSnapshot()) == 30 && R.ValidateInvariants());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridPoliciesTest, "Shanmen.0_0_10.Items.Grid.SecureEquipmentAndScope", Flags)
bool FShanmenGridPoliciesTest::RunTest(const FString&)
{
	FShanmenItemRepository R; R.TryLoadSnapshot(Fixture()); const auto Before = R.CaptureSnapshot();
	TestEqual(TEXT("Weapon cannot enter secure"), R.EditGrid(Request(R, Sword, Secure)).Error, EShanmenItemTransactionError::GridPolicyViolation);
	TestEqual(TEXT("Material cannot equip as weapon"), R.EditGrid(Request(R, MaterialStackAId, Weapon)).Error, EShanmenItemTransactionError::GridPolicyViolation);
	auto Q = Request(R, Sword, Weapon); Q.Context.OwnerId = FGuid::NewGuid();
	TestEqual(TEXT("Different owner fails closed"), R.EditGrid(Q).Error, EShanmenItemTransactionError::ScopeMismatch);
	Q = Request(R, Sword, Weapon); Q.ExpectedAuthorityRevision = 1;
	TestEqual(TEXT("Stale preview fails closed"), R.EditGrid(Q).Error, EShanmenItemTransactionError::StaleAuthorityRevision);
	TestTrue(TEXT("Every rejection preserved exact snapshot"), R.CaptureSnapshot() == Before);
	TestTrue(TEXT("Sword equips in one logical slot despite 2x1 footprint"), R.EditGrid(Request(R, Sword, Weapon)).IsSuccess());
	TestTrue(TEXT("Material legally enters secure"), R.EditGrid(Request(R, MaterialStackAId, Secure)).IsSuccess());
	Q = Request(R, MaterialStackBId, Secure); TestEqual(TEXT("Secure occupied footprint blocks another stack"), R.EditGrid(Q).Error, EShanmenItemTransactionError::GridNoSpace);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridReservationTest, "Shanmen.0_0_10.Items.Grid.ReservationsAndLegacyBoundary", Flags)
bool FShanmenGridReservationTest::RunTest(const FString&)
{
	FShanmenItemRepository R; R.TryLoadSnapshot(Fixture());
	FShanmenItemReserveRequest Reserve; Reserve.Context = Request(R, MaterialStackAId, Carry).Context; Reserve.ItemInstanceId = MaterialStackAId;
	Reserve.Amount = 2; Reserve.PurposeId = TEXT("Pending.Test"); Reserve.ResourceKind = EShanmenItemResourceKind::Quantity;
	TestTrue(TEXT("Reserve actual quantity"), R.Reserve(Reserve).IsSuccess()); const auto Before = R.CaptureSnapshot();
	TestEqual(TEXT("Grid cannot bypass a pending resource intent"), R.EditGrid(Request(R, MaterialStackAId, Carry)).Error, EShanmenItemTransactionError::GridPolicyViolation);
	TestTrue(TEXT("Reserved quantity is unchanged"), R.CaptureSnapshot() == Before);
	FShanmenItemRepository Legacy; TestTrue(TEXT("Legacy snapshot loads unchanged"), Legacy.TryLoadSnapshot(Fixture(false)));
	TestEqual(TEXT("Grid command never falls back to legacy writer"), Legacy.EditGrid(Request(Legacy, MaterialStackAId, Carry)).Error, EShanmenItemTransactionError::GridPolicyViolation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridDurableTest, "Shanmen.0_0_10.Items.Grid.DurableReopenRollback", Flags)
bool FShanmenGridDurableTest::RunTest(const FString&)
{
	const auto S = Fixture(); const auto E = Evidence(S); const auto Disk = Storage(TEXT("Durable"));
	FShanmenItemAuthorityService Service;
	TestTrue(TEXT("Explicit fixture initialization creates grid in sole authority document"), Service.StartFromAuthorizedMigration(Disk,
		FShanmenItemMigrationAuthorization::Explicit(E.MigrationId), S, E).IsReady());
	FShanmenItemRepository R; R.TryLoadSnapshot(S); auto Q = Request(R, Sword, Carry, 5, 0); Q.bRotated = true;
	const auto Moved = Service.EditGridDurable(Q); TestTrue(TEXT("Only durably saved move reports success"), Moved.IsCommandSuccess());
	FShanmenItemAuthoritySnapshot Before, After; Service.TryCaptureSnapshot(Before); R.TryLoadSnapshot(Before);
	Q = Request(R, MaterialStackAId, Secure); Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::WriteTemp);
	const auto Failed = Service.EditGridDurable(Q); Service.TryCaptureSnapshot(After);
	TestTrue(TEXT("Failed save rolls back full placement and quantity"), !Failed.IsCommandSuccess()
		&& Failed.Status == EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack && Before == After);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Same failed request can safely retry"), Service.EditGridDurable(Q).IsCommandSuccess());
	Service.TryCaptureSnapshot(After);
	FShanmenItemAuthorityService Reopened; FShanmenItemAuthoritySnapshot Reloaded;
	TestTrue(TEXT("Reopen retains exact geometry, nonzero stacks and accepted ledger"), Reopened.StartExisting(Disk).IsReady()
		&& Reopened.TryCaptureSnapshot(Reloaded) && Reloaded == After && Total(Reloaded) == 30);
	TestTrue(TEXT("Reopened exact retry writes nothing"), Reopened.EditGridDurable(Q).Status == EShanmenItemDurableCommandStatus::Replayed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridSchema4Test, "Shanmen.0_0_10.Items.Grid.Schema4NonzeroMigration", Flags)
bool FShanmenGridSchema4Test::RunTest(const FString&)
{
	const auto S = Fixture(false); const auto E = Evidence(S); const auto Disk = Storage(TEXT("NMinusOne"));
	FShanmenItemAuthorityStore Store; const auto Created = Store.OpenOrCreateFromMigration(S, E, Disk);
	TestTrue(TEXT("Create independent isolated N-1 fixture source"), Created.IsSuccess());
	FString Json; FFileHelper::LoadFileToString(Json, *Disk.PrimaryPath()); TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(Json), Root) || !Root.IsValid()) { AddError(TEXT("Fixture unavailable")); return false; }
	const auto Authority = Root->GetObjectField(TEXT("Authority")); Authority->RemoveField(TEXT("Grid"));
	FString OldWire; FJsonSerializer::Serialize(Authority.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OldWire));
	FTCHARToUTF8 Utf8(*OldWire); uint8 Hash[SHA256_DIGEST_LENGTH]{}; SHA256(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), Hash);
	FString OldDigest; for (uint8 Byte : Hash) { OldDigest += FString::Printf(TEXT("%02X"), Byte); }
	FString Computed; TestTrue(TEXT("Historical codec matches independent old wire hash"), FShanmenItemAuthorityStore::ComputeLegacySchema4SnapshotDigest(S, Computed) && Computed == OldDigest);
	Root->SetNumberField(TEXT("SchemaVersion"), 4); Root->SetStringField(TEXT("InitialSnapshotDigest"), OldDigest); Root->SetStringField(TEXT("SnapshotDigest"), OldDigest);
	FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json));
	TestTrue(TEXT("Write exact schema-4 bytes"), FFileHelper::SaveStringToFile(Json, *Disk.PrimaryPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	FString Tampered = Json;
	TestTrue(TEXT("Tamper nonzero N-1 quantity with old digest unchanged"), Tampered.ReplaceInline(TEXT("\"Quantity\":12"), TEXT("\"Quantity\":11")) == 1
		&& FFileHelper::SaveStringToFile(Tampered, *Disk.PrimaryPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	TestFalse(TEXT("N-1 payload must validate before normalization"), Store.LoadExisting(Disk).IsSuccess());
	TestTrue(TEXT("Restore exact historical fixture"), FFileHelper::SaveStringToFile(Json, *Disk.PrimaryPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	TArray<uint8> Before, After; FFileHelper::LoadFileToArray(Before, *Disk.PrimaryPath());
	const auto Loaded = Store.LoadExisting(Disk); FFileHelper::LoadFileToArray(After, *Disk.PrimaryPath());
	TestTrue(TEXT("N-1 read validates old digest, preserves 30 units and does not rewrite"), Loaded.IsSuccess() && Loaded.bSchemaUpgraded
		&& Loaded.Document.Authority == S && Total(Loaded.Document.Authority) == 30 && Before == After);
	const auto Upgraded = Store.OpenOrCreateFromMigration(S, E, Disk);
	TestTrue(TEXT("Authorized reopen normalizes once, preserves initial evidence"), Upgraded.IsSuccess()
		&& Upgraded.bDiskStateChanged && Upgraded.Document.SchemaVersion == FShanmenItemAuthorityDocument::CurrentSchemaVersion
		&& Upgraded.Document.SaveGeneration == Created.Document.SaveGeneration + 1
		&& Upgraded.Document.Authority == S && Upgraded.Document.InitialSnapshotDigest == OldDigest);
	TestTrue(TEXT("Repeat open is write-free"), !Store.OpenOrCreateFromMigration(S, E, Disk).bDiskStateChanged);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridSchema5Test, "Shanmen.0_0_10.Items.Grid.Schema5NonzeroLedgerMigration", Flags)
bool FShanmenGridSchema5Test::RunTest(const FString&)
{
	const auto Initial = Fixture(); const auto Origin = Evidence(Initial); const auto SourceDisk = Storage(TEXT("Schema5Source"));
	FShanmenItemAuthorityStore Store; auto Created = Store.OpenOrCreateFromMigration(Initial, Origin, SourceDisk);
	if (!TestTrue(TEXT("Create fixture"), Created.IsSuccess())) return false;
	FShanmenItemRepository Repo; Repo.TryLoadSnapshot(Initial);
	const auto Move = Request(Repo, Sword, Carry, 3, 1); const auto Accepted = Repo.EditGrid(Move);
	const auto Current = Repo.CaptureSnapshot();
	TestTrue(TEXT("Fixture has nonzero grid and accepted ledger"),Accepted.IsSuccess() && Current.ProcessedRequests.Num() == 1 && Total(Current) == 30);
	TestTrue(TEXT("Publish fixture with history"),Store.SaveAuthority(Created.Document, Current, SourceDisk).IsSuccess());
	FString Json; FFileHelper::LoadFileToString(Json,*SourceDisk.PrimaryPath()); TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<TCHAR>::Create(Json),Root)) return false;
	const auto Authority = Root->GetObjectField(TEXT("Authority")); Authority->GetObjectField(TEXT("Grid"))->RemoveField(TEXT("StorageDefinitions"));
	FString Wire; FJsonSerializer::Serialize(Authority.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Wire));
	FTCHARToUTF8 Utf8(*Wire); uint8 Hash[SHA256_DIGEST_LENGTH]{}; SHA256(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length(),Hash);
	FString CurrentDigest; for (uint8 Byte : Hash) CurrentDigest += FString::Printf(TEXT("%02X"),Byte);
	FString Computed, InitialDigest;
	TestTrue(TEXT("Schema-5 codec equals independent old wire hash"),FShanmenItemAuthorityStore::ComputeLegacySchema5SnapshotDigest(Current,Computed) && Computed == CurrentDigest);
	FShanmenItemAuthorityStore::ComputeLegacySchema5SnapshotDigest(Initial,InitialDigest);
	Root->SetNumberField(TEXT("SchemaVersion"),5); Root->SetStringField(TEXT("InitialSnapshotDigest"),InitialDigest); Root->SetStringField(TEXT("SnapshotDigest"),CurrentDigest);
	FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json));
	const auto OldDisk = Storage(TEXT("Schema5WithoutBackup")); IFileManager::Get().MakeDirectory(*OldDisk.StorageDirectory(),true);
	FString Tampered = Json; TestEqual(TEXT("Tamper real quantity"),Tampered.ReplaceInline(TEXT("\"Quantity\":12"),TEXT("\"Quantity\":11")),1);
	FFileHelper::SaveStringToFile(Tampered,*OldDisk.PrimaryPath(),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TestFalse(TEXT("Old digest validated before current normalization"),Store.LoadExisting(OldDisk).IsSuccess());
	FFileHelper::SaveStringToFile(Json,*OldDisk.PrimaryPath(),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TArray<uint8> Before,After; FFileHelper::LoadFileToArray(Before,*OldDisk.PrimaryPath());
	const auto Read = Store.LoadExisting(OldDisk); FFileHelper::LoadFileToArray(After,*OldDisk.PrimaryPath());
	TestTrue(TEXT("N-1 readonly load preserves exact nonempty inventory/geometry/ledger bytes"),Read.IsSuccess() && Read.bSchemaUpgraded
		&& Read.Document.Authority == Current && Before == After);
	const auto Normalized = Store.OpenOrCreateFromMigration(Initial,Origin,OldDisk);
	TestTrue(TEXT("Normalize once preserves original initial evidence and current ledger"),Normalized.IsSuccess() && Normalized.bDiskStateChanged
		&& Normalized.Document.SaveGeneration == Created.Document.SaveGeneration + 1 && Normalized.Document.InitialSnapshotDigest == InitialDigest
		&& Normalized.Document.Authority == Current);
	FShanmenItemRepository Restored; Restored.TryLoadSnapshot(Normalized.Document.Authority);
	TestTrue(TEXT("Accepted request remains replayable after schema migration"),Restored.EditGrid(Move) == Accepted && Restored.CaptureSnapshot() == Current);
	TestFalse(TEXT("Second normalization write-free"),Store.OpenOrCreateFromMigration(Initial,Origin,OldDisk).bDiskStateChanged);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridMetadataTest, "Shanmen.0_0_10.Items.Grid.OrdinaryOriginsAndSpecialRewardIsolation", Flags)
bool FShanmenGridMetadataTest::RunTest(const FString&)
{
	FShanmenItemRewardMetadata A,B; A.RewardSourceRoleId=TEXT("Enemy.Melee"); B.RewardSourceRoleId=TEXT("Chest.North");
	const auto BeforeA=A,BeforeB=B;
	TestTrue(TEXT("Ordinary roles may differ"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	TestTrue(TEXT("Prepared ordinary metadata may mix"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,{}));
	TestTrue(TEXT("Compatibility does not erase birth records"),A==BeforeA && B==BeforeB);
	A.RewardEventKind=EShanmenItemRewardEventKind::Jackpot; A.RewardEventId=FGuid::NewGuid(); A.RewardValueMultiplierBps=A.JackpotMultiplierBps;
	TestFalse(TEXT("Jackpot cannot become ordinary"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	B=A; TestTrue(TEXT("Exact same special birth still supported"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	B.RewardSourceRoleId=TEXT("Chest.North"); TestFalse(TEXT("Special origins are not stripped"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	B=A; B.RewardEventId=FGuid::NewGuid(); TestFalse(TEXT("Different jackpot events cannot mix"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	A=BeforeA; B=BeforeB; A.RareRewardEventId=FGuid::NewGuid(); A.RareRewardPolicyId=TEXT("Rare.Policy"); A.RareRewardTierId=TEXT("Rare.Tier1"); A.RareRewardBonusValue=10;
	TestTrue(TEXT("Valid rare fixture"),A.IsValid()); TestFalse(TEXT("Rare bonus retained, not ordinary"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	A=BeforeA; A.AffixSetEventId=FGuid::NewGuid(); A.AffixPolicyId=TEXT("Affix.Policy"); A.AffixAcquisition=EShanmenItemRewardAffixAcquisition::Natural;
	FShanmenItemResolvedRewardAffix Affix; Affix.AffixId=TEXT("Affix.Attack"); Affix.Tier=EShanmenItemRewardAffixTier::Tier1;
	Affix.ResolvedMagnitudeScaled=10; Affix.ResolvedValue=5; A.Affixes.Add(Affix);
	TestTrue(TEXT("Valid affix fixture"),A.IsValid()); TestFalse(TEXT("Affix is not discarded for stacking"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B));
	A=BeforeA; A.RewardValueMultiplierBps=9999; B=A;
	TestFalse(TEXT("Equal invalid metadata is not compatible"),FShanmenItemStackTransferPolicy::MetadataCompatible(A,B)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShanmenGridLegacyStackTest, "Shanmen.0_0_10.Items.Grid.LegacyStackReceiptsReplayWithoutRewrite", Flags)
bool FShanmenGridLegacyStackTest::RunTest(const FString&)
{
	FShanmenItemRepository R; if (!TestTrue(TEXT("Nonzero legacy fixture"),R.TryLoadSnapshot(Fixture()))) return false;
	auto Q=Request(R,MaterialStackAId,FGuid()); Q.Action=EShanmenItemGridAction::Merge; Q.MergeTargetId=MaterialStackBId; Q.Amount=8;
	Q.ExpectedTargetRevision=R.FindItem(MaterialStackBId)->Revision;
	if (!TestTrue(TEXT("Current partial merge"),R.EditGrid(Q).IsSuccess())) return false;
	auto Historical=R.CaptureSnapshot();
	// The old command fingerprint and receipt identity do not depend on the
	// new purpose/witness. Model actual pre-r2 empty transfer fields exactly.
	for (auto& P:Historical.ProcessedRequests) { P.Receipt.PurposeId=TEXT("Grid.Merge"); P.Receipt.ReservationIds.Reset(); }
	FShanmenItemRepository Old; if (!TestTrue(TEXT("Historical shape loads unchanged"),Old.TryLoadSnapshot(Historical))) return false;
	TestTrue(TEXT("Historical exact replay, no inferred recipient/witness rewrite"),Old.EditGrid(Q)==Historical.ProcessedRequests[0].Receipt && Old.CaptureSnapshot()==Historical);
	auto Split=Request(Old,MaterialStackAId,Carry); Split.Action=EShanmenItemGridAction::Split; Split.Amount=2;
	const auto Result=Old.EditGrid(Split); TestTrue(TEXT("Next real command is witnessed"),Result.IsSuccess() && FShanmenItemStackTransferPolicy::HasWitness(Result));
	TestEqual(TEXT("Conservation uses nonzero thirty, not a zero baseline"),Total(Old.CaptureSnapshot()),30); return true;
}
#endif
