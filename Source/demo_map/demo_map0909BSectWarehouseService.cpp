#include "demo_map0909BSectWarehouseService.h"

#include "CodeB/demo_mapCodeBP2.h"
#include "demo_mapShanmenLegacyItemWriteFence.h"

bool Fdemo_map0909BSectWarehouseService::OpenForSect(
	const FString& StorageRoot,
	const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot,
	const Edemo_map0909BTopState CoordinatorState,
	Fdemo_map0909BWarehousePresentation& OutPresentation,
	FString& OutDiagnostic)
{
	OutPresentation = Fdemo_map0909BWarehousePresentation();
	OutDiagnostic.Reset();
	Reset();
	if (CoordinatorState != Edemo_map0909BTopState::AtSect)
	{
		OutDiagnostic = CoordinatorState == Edemo_map0909BTopState::PreparingStart
			|| CoordinatorState == Edemo_map0909BTopState::ActivatingWorld
			? TEXT("StartAttemptPending：出战尝试处理中，暂不接受仓库写入。")
			: TEXT("只有协调器确认 AtSect 时才能打开可整理的宗门仓库。");
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	if (StorageRoot.IsEmpty() || !ProfileSnapshot.ProfileId.IsValid())
	{
		OutDiagnostic = TEXT("宗门仓库无法取得 Owner-matched P5 Profile 存储根。");
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}

	Store = MakeUnique<FCodeBOutOfRaidProfileStore>(StorageRoot, ProfileSnapshot.ProfileId);
	const FCodeBOutOfRaidOpenResult Open = Store->OpenOrMigrate(ProfileSnapshot, Repository, Layout);
	if (!Open.bSuccess)
	{
		OutDiagnostic = Open.Diagnostic;
		OutPresentation.GateDiagnostic = OutDiagnostic;
		Reset();
		return false;
	}
	BoundStorageRoot = StorageRoot;
	BoundOwnerId = ProfileSnapshot.ProfileId;
	return BuildPresentation(CoordinatorState, OutPresentation, OutDiagnostic);
}

bool Fdemo_map0909BSectWarehouseService::CaptureLoadoutSelection(
	FCodeBLoadoutSelection& OutSelection,
	FString& OutDiagnostic) const
{
	OutSelection = FCodeBLoadoutSelection();
	OutDiagnostic.Reset();
	if (!Store)
	{
		OutDiagnostic = TEXT("P5 warehouse service is not enrolled for this sect session.");
		return false;
	}
	return FCodeBOutOfRaidProfileStore::BuildLoadoutSelection(
		Store->GetRecord(), OutSelection, &OutDiagnostic);
}

bool Fdemo_map0909BSectWarehouseService::CaptureStableItemMigrationRecord(
	FCodeBOutOfRaidInventoryRecord& OutRecord,
	FString& OutDiagnostic) const
{
	OutRecord = FCodeBOutOfRaidInventoryRecord();
	OutDiagnostic.Reset();
	if (!Store || !Store->IsOpen())
	{
		OutDiagnostic =
			TEXT("Stable item migration requires an open Code B out-of-raid record.");
		return false;
	}
	const FCodeBOutOfRaidInventoryRecord& Record = Store->GetRecord();
	if (!Record.OwnerId.IsValid()
		|| Record.bHasActiveRunInventorySession
		|| Record.Receipt.State != ECodeBOutOfRaidHandoffState::Committed
		|| !Record.RunLocalNormalContainers.IsEmpty()
		|| !Record.RunLocalBodyContainers.IsEmpty())
	{
		OutDiagnostic =
			TEXT("Code B is not a committed, terminal out-of-raid migration source.");
		return false;
	}
	OutRecord = Record;
	return true;
}

bool Fdemo_map0909BSectWarehouseService::AreLegacyItemWritesRetired(
	FString* OutDiagnostic) const
{
	const Fdemo_mapShanmenLegacyItemWriteFenceProbe Probe =
		Fdemo_mapShanmenLegacyItemWriteFence::Inspect(
			BoundStorageRoot, BoundOwnerId);
	if (OutDiagnostic)
	{
		*OutDiagnostic = Probe.Diagnostic;
	}
	return Probe.IsRetired();
}

void Fdemo_map0909BSectWarehouseService::Reset()
{
	Store.Reset();
	Repository = demo_map_code_b::FCodeBRepository();
	Layout = demo_map_code_b::FCodeBP2PlayerLayout();
	BoundStorageRoot.Reset();
	BoundOwnerId.Invalidate();
}

bool Fdemo_map0909BSectWarehouseService::BuildPresentation(
	const Edemo_map0909BTopState CoordinatorState,
	Fdemo_map0909BWarehousePresentation& OutPresentation,
	FString& OutDiagnostic) const
{
	OutPresentation = Fdemo_map0909BWarehousePresentation();
	if (!Store)
	{
		if (OutDiagnostic.IsEmpty()) OutDiagnostic = TEXT("P5 warehouse service is not open.");
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	FString ProjectionError;
	if (!demo_map_code_b::FCodeBP2ProjectionBuilder::Build(
		Repository, Layout, OutPresentation.Projection, nullptr, &ProjectionError))
	{
		OutDiagnostic = ProjectionError;
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	FString SelectionError;
	if (!FCodeBOutOfRaidProfileStore::BuildLoadoutSelection(
		Store->GetRecord(), OutPresentation.LoadoutSelection, &SelectionError))
	{
		OutDiagnostic = SelectionError;
		OutPresentation.GateDiagnostic = OutDiagnostic;
		return false;
	}
	OutPresentation.bOpen = true;
	OutPresentation.OwnerId = Store->GetRecord().OwnerId;
	OutPresentation.PersistentRevision = Store->GetPersistentRevision();
	FString FenceDiagnostic;
	const bool bRetired = AreLegacyItemWritesRetired(&FenceDiagnostic);
	OutPresentation.bCanWrite = CoordinatorState == Edemo_map0909BTopState::AtSect
		&& !Store->GetRecord().bHasActiveRunInventorySession
		&& !bRetired;
	OutPresentation.GateDiagnostic = OutPresentation.bCanWrite
		? TEXT("AtSect：可提交同图 P5 整理事务。")
		: (bRetired
			? FenceDiagnostic
			: (CoordinatorState == Edemo_map0909BTopState::PreparingStart
			|| CoordinatorState == Edemo_map0909BTopState::ActivatingWorld
			? TEXT("StartAttemptPending：出战尝试处理中，P5 写入暂时拒绝。")
			: TEXT("只有确认 InRun 才锁定仓库写入。")));
	return true;
}
