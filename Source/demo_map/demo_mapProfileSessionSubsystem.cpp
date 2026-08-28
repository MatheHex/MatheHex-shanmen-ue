#include "demo_mapProfileSessionSubsystem.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapProfileTradeTransaction.h"
#include "demo_mapRewardShopStock.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenPreparationAdapter.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"
#include "demo_mapTownProgressionRules.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"

namespace
{
	Udemo_mapShanmenItemAuthoritySubsystem* FindReadyPreparationAuthority(
		const Udemo_mapProfileSessionSubsystem& Session)
	{
		UGameInstance* GameInstance = Session.GetGameInstance();
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = GameInstance
			? GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>()
			: nullptr;
		return Authority
			&& Authority->GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready
			? Authority : nullptr;
	}

	Edemo_mapProfilePreparationSelectionStatus MapPreparationAdapterStatus(
		Edemo_mapShanmenPreparationAdapterStatus Status)
	{
		switch (Status)
		{
		case Edemo_mapShanmenPreparationAdapterStatus::Accepted:
		case Edemo_mapShanmenPreparationAdapterStatus::NoChange:
			return Edemo_mapProfilePreparationSelectionStatus::Accepted;
		case Edemo_mapShanmenPreparationAdapterStatus::CleanupPending:
			return Edemo_mapProfilePreparationSelectionStatus::AuthorityCleanupPending;
		case Edemo_mapShanmenPreparationAdapterStatus::ItemNotFound:
			return Edemo_mapProfilePreparationSelectionStatus::ItemNotFound;
		case Edemo_mapShanmenPreparationAdapterStatus::DuplicateSelection:
			return Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection;
		case Edemo_mapShanmenPreparationAdapterStatus::InvalidSlot:
		case Edemo_mapShanmenPreparationAdapterStatus::SlotRejected:
			return Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected;
		case Edemo_mapShanmenPreparationAdapterStatus::MaterialRejected:
		case Edemo_mapShanmenPreparationAdapterStatus::HotbarRejected:
			return Edemo_mapProfilePreparationSelectionStatus::MaterialRejected;
		case Edemo_mapShanmenPreparationAdapterStatus::SelectionLimitExceeded:
			return Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded;
		default:
			return Edemo_mapProfilePreparationSelectionStatus::AuthorityCommandRejected;
		}
	}

	bool IsPreparationEquipmentSlot(FName SlotId)
	{
		return SlotId == Fdemo_mapItemIds::WeaponSlot
			|| SlotId == Fdemo_mapItemIds::ArmorSlot
			|| SlotId == Fdemo_mapItemIds::AccessorySlot
			|| SlotId == Fdemo_mapItemIds::SpatialRingSlot
			|| SlotId == Fdemo_mapItemIds::BackpackSlot;
	}

	bool IsPreparationRunInventoryDefinition(FName DefinitionId)
	{
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
		return Definition
			&& (Definition->CategoryId == Fdemo_mapItemIds::MaterialCategory
				|| Definition->CategoryId == Fdemo_mapItemIds::ConsumableCategory);
	}

	FName ResolvePreparationSpatialItemDefinitionId(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		const Fdemo_mapPersistentPreparationLayout& Layout)
	{
		if (!Layout.BackpackItemInstanceId.IsValid())
		{
			return NAME_None;
		}
		const Fdemo_mapPersistentItemRecord* Item =
			SessionSnapshot.OrderedPermanentStash.FindByPredicate(
				[&Layout](const Fdemo_mapPersistentItemRecord& Candidate)
				{
					return Candidate.ItemInstanceId
						== Layout.BackpackItemInstanceId;
				});
		return Item ? Item->ItemDefinitionId : NAME_None;
	}

	FName ResolvePreparationRingDefinitionId(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		const Fdemo_mapPersistentPreparationLayout& Layout)
	{
		if (!Layout.SpatialRingItemInstanceId.IsValid())
		{
			return NAME_None;
		}
		const Fdemo_mapPersistentItemRecord* Item =
			SessionSnapshot.OrderedPermanentStash.FindByPredicate(
				[&Layout](const Fdemo_mapPersistentItemRecord& Candidate)
				{
					return Candidate.ItemInstanceId
						== Layout.SpatialRingItemInstanceId;
				});
		return Item ? Item->ItemDefinitionId : NAME_None;
	}

	bool CanPreparationInventoryFit(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		const Fdemo_mapPersistentPreparationLayout& Layout,
		FString& OutDiagnostic)
	{
		const Fdemo_mapInventoryCapacityResult Capacity =
			Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
				ResolvePreparationSpatialItemDefinitionId(
					SessionSnapshot,
					Layout),
				ResolvePreparationRingDefinitionId(
					SessionSnapshot,
					Layout));
		if (!Capacity.bSuccess)
		{
			OutDiagnostic = Capacity.Diagnostic;
			return false;
		}
		if (Layout.OrderedRunInventoryItemInstanceIds.Num()
			> Capacity.Capacity)
		{
			OutDiagnostic = FString::Printf(
				TEXT("容量不足：当前出战物品占用 %d 格，目标空间道具总容量为 %d 格。请先把物品移回仓库。"),
				Layout.OrderedRunInventoryItemInstanceIds.Num(),
				Capacity.Capacity);
			return false;
		}
		OutDiagnostic.Reset();
		return true;
	}

	const Fdemo_mapPersistentItemRecord* FindPreparationStashItem(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		const FGuid& ItemInstanceId)
	{
		return SessionSnapshot.OrderedPermanentStash.FindByPredicate(
			[&ItemInstanceId](
				const Fdemo_mapPersistentItemRecord& Candidate)
			{
				return Candidate.ItemInstanceId == ItemInstanceId;
			});
	}

	bool IsBaseQuickConsumable(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		const Fdemo_mapPersistentPreparationLayout& Layout,
		const FGuid& ItemInstanceId)
	{
		const int32 CarriedIndex =
			Layout.OrderedRunInventoryItemInstanceIds.IndexOfByKey(
				ItemInstanceId);
		const Fdemo_mapInventoryCapacityResult Capacity =
			Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
				ResolvePreparationSpatialItemDefinitionId(
					SessionSnapshot, Layout),
				ResolvePreparationRingDefinitionId(
					SessionSnapshot, Layout));
		if (CarriedIndex < 0 || !Capacity.bSuccess
			|| CarriedIndex >= Fdemo_mapPersistentPreparationLayout::
				BaseQuickItemSlotCount + Capacity.RingQuickCapacity)
		{
			return false;
		}
		const Fdemo_mapPersistentItemRecord* Item =
			FindPreparationStashItem(
				SessionSnapshot,
				ItemInstanceId);
		const Fdemo_mapItemDefinition* Definition = Item
			? Fdemo_mapItemDefinitions::Find(
				Item->ItemDefinitionId)
			: nullptr;
		return Item
			&& Definition
			&& Item->PersistentDomain
				== Edemo_mapPersistentDomain::PermanentStash
			&& Item->StackCount > 0
			&& Item->StackCount <= Definition->MaxStackSize
			&& Definition->CategoryId
				== Fdemo_mapItemIds::ConsumableCategory;
	}

	void ReconcilePreparationHotbar(
		const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
		Fdemo_mapPersistentPreparationLayout& Layout)
	{
		TSet<FGuid> Seen;
		for (FGuid& BoundId : Layout.HotbarItemInstanceIds)
		{
			if (!BoundId.IsValid())
			{
				continue;
			}
			if (Seen.Contains(BoundId)
				|| !IsBaseQuickConsumable(
					SessionSnapshot,
					Layout,
					BoundId))
			{
				BoundId.Invalidate();
				continue;
			}
			Seen.Add(BoundId);
		}
	}
}

void Udemo_mapProfileSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Coordinator = MakeUnique<Fdemo_mapProfileSessionCoordinator>();
	InjectedStorageRoot.Reset();
	ClearPreparationSelectionInternal();
	LastTradeResult.Reset();
	RecentSpiritStoneGain = 0;
	RecentSpiritStoneGainExpirySeconds = 0.0;
	bExplicitlyInitialized = false;
}

void Udemo_mapProfileSessionSubsystem::Deinitialize()
{
	ClearPreparationSelectionInternal();
	LastTradeResult.Reset();
	RecentSpiritStoneGain = 0;
	RecentSpiritStoneGainExpirySeconds = 0.0;
	bExplicitlyInitialized = false;
	InjectedStorageRoot.Reset();
	Coordinator.Reset();
	Super::Deinitialize();
}

Fdemo_mapProfileSessionInitializeResult Udemo_mapProfileSessionSubsystem::InitializeSession(
	const Fdemo_mapProfileStorageContext& Storage)
{
	if (!IsInGameThread())
	{
		return RejectInitialize(TEXT("Profile Session initialization is restricted to the Game Thread."));
	}
	ClearPreparationSelectionInternal();
	LastTradeResult.Reset();
	if (!Coordinator.IsValid())
	{
		return RejectInitialize(TEXT("Profile Session Subsystem is not in an initialized GameInstance lifecycle."));
	}
	if (Storage.RootDirectory.IsEmpty())
	{
		return RejectInitialize(TEXT("Profile Session initialization requires an explicitly injected storage root."));
	}
	if (bExplicitlyInitialized)
	{
		const FString Diagnostic = Storage.RootDirectory == InjectedStorageRoot
			? TEXT("Profile Session is already initialized; the existing in-memory session was returned without disk I/O.")
			: TEXT("Profile Session is already initialized; storage switching was rejected without disk I/O.");
		return CurrentInitializeResult(Diagnostic);
	}

	InjectedStorageRoot = Storage.RootDirectory;
	bExplicitlyInitialized = true;
	const Fdemo_mapProfileSessionInitializeResult Result = Coordinator->InitializeSession(Storage);
	SynchronizePreparationWithSession(Result.Snapshot, Result.Diagnostic);
	return Result;
}

Fdemo_mapProfileSessionBeginResult Udemo_mapProfileSessionSubsystem::BeginRun(
	const Fdemo_mapBeginRunRequest& Request)
{
	if (!IsInGameThread())
	{
		return RejectBegin(TEXT("Profile Session BeginRun is restricted to the Game Thread."));
	}
	if (!bExplicitlyInitialized || !Coordinator.IsValid())
	{
		return RejectBegin(TEXT("Profile Session BeginRun requires explicit session initialization."));
	}

	UGameInstance* GameInstance = GetGameInstance();
	Udemo_mapItemSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<Udemo_mapItemSubsystem>()
		: nullptr;
	if (!Runtime)
	{
		return RejectBegin(TEXT("The owning GameInstance has no ItemSubsystem Runtime Authority."));
	}
	const Fdemo_mapProfileSessionBeginResult Result = Coordinator->BeginRun(Request, *Runtime);
	ApplyBeginResultToPreparation(Result);
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Udemo_mapProfileSessionSubsystem::CommitRuntimeSettlement(
	const Fdemo_mapSettlementSummary& Summary)
{
	if (!IsInGameThread())
	{
		return RejectSettlement(
			Edemo_mapProfileSessionSettlementStatus::SessionStateRejected,
			TEXT("Profile Session Settlement is restricted to the Game Thread."));
	}
	if (!bExplicitlyInitialized || !Coordinator.IsValid())
	{
		return RejectSettlement(
			Edemo_mapProfileSessionSettlementStatus::SessionStateRejected,
			TEXT("Profile Session Settlement requires explicit session initialization."));
	}
	const Fdemo_mapProfileSessionSettlementResult Result = Coordinator->CommitRuntimeSettlement(Summary);
	SynchronizePreparationWithSession(Result.Snapshot, Result.Diagnostic);
	return Result;
}

Fdemo_mapProfileGeneratedRewardSourceResult
Udemo_mapProfileSessionSubsystem::CommitGeneratedRewardSource(
	const Fdemo_mapRewardSourceAcceptanceReceipt& Receipt)
{
	Fdemo_mapProfileGeneratedRewardSourceResult Result;
	if (!IsInGameThread() || !bExplicitlyInitialized || !Coordinator.IsValid())
	{
		Result.Diagnostic =
			TEXT("Generated reward source commit requires an initialized Profile Session on the Game Thread.");
		return Result;
	}
	return Coordinator->CommitGeneratedRewardSource(Receipt);
}

TArray<Fdemo_mapPersistentGeneratedRewardSource>
Udemo_mapProfileSessionSubsystem::GetActiveGeneratedRewardSources() const
{
	return Coordinator.IsValid()
		? Coordinator->GetActiveGeneratedRewardSources()
		: TArray<Fdemo_mapPersistentGeneratedRewardSource>();
}

Fdemo_mapProfileSessionSettlementResult Udemo_mapProfileSessionSubsystem::RetryPendingSettlement()
{
	if (!IsInGameThread())
	{
		return RejectSettlement(
			Edemo_mapProfileSessionSettlementStatus::SessionStateRejected,
			TEXT("Profile Session Settlement retry is restricted to the Game Thread."));
	}
	if (!bExplicitlyInitialized || !Coordinator.IsValid())
	{
		return RejectSettlement(
			Edemo_mapProfileSessionSettlementStatus::NoPendingSettlement,
			TEXT("Profile Session Settlement retry requires explicit session initialization."));
	}
	const Fdemo_mapProfileSessionSettlementResult Result = Coordinator->RetryPendingSettlement();
	SynchronizePreparationWithSession(Result.Snapshot, Result.Diagnostic);
	return Result;
}

Fdemo_mapProfileTradeResult Udemo_mapProfileSessionSubsystem::SubmitTradeIntent(
	const Fdemo_mapProfileTradeIntent& Intent)
{
	Fdemo_mapProfileTradeResult Result;
	Result.Kind = Intent.Kind;
	if (!IsInGameThread())
	{
		Result.Status = Edemo_mapProfileTradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Profile trade is restricted to the Game Thread.");
		LastTradeResult = Result;
		return Result;
	}
	if (!bExplicitlyInitialized || !Coordinator.IsValid())
	{
		Result.Status = Edemo_mapProfileTradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Profile trade requires explicit session initialization.");
		LastTradeResult = Result;
		return Result;
	}

	Result = Coordinator->SubmitTrade(Intent, TSet<FGuid>());
	LastTradeResult = Result;
	PreparationDiagnostic = Result.Diagnostic;
	if (Result.IsCommitted())
	{
		const Fdemo_mapProfileSessionSnapshot SessionSnapshot = Coordinator->GetSnapshot();
		if (bPreparationIdentityBound)
		{
			PreparationProfileId = SessionSnapshot.ProfileId;
			PreparationSaveGeneration = SessionSnapshot.SaveGeneration;
		}
		else
		{
			BindPreparationIdentity(SessionSnapshot);
		}
	}
	return Result;
}

Fdemo_mapProfileSessionSnapshot Udemo_mapProfileSessionSubsystem::GetSnapshot() const
{
	return Coordinator.IsValid()
		? Coordinator->GetSnapshot()
		: Fdemo_mapProfileSessionSnapshot();
}

bool Udemo_mapProfileSessionSubsystem::
TryCaptureStableProfileForItemMigration(
	Fdemo_mapPersistentProfile& OutProfile,
	FString* OutDiagnostic) const
{
	OutProfile = Fdemo_mapPersistentProfile();
	if (!IsInGameThread())
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic =
				TEXT("Stable Profile capture is restricted to the Game Thread.");
		}
		return false;
	}
	if (!bExplicitlyInitialized || !Coordinator.IsValid())
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic =
				TEXT("Stable Profile capture requires an explicitly initialized session.");
		}
		return false;
	}
	return Coordinator->TryCaptureStableProfileForItemMigration(
		OutProfile, OutDiagnostic);
}

bool Udemo_mapProfileSessionSubsystem::AreLegacyItemWritesRetired(
	FString* OutDiagnostic) const
{
	if (!bExplicitlyInitialized || !Coordinator.IsValid())
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic =
				TEXT("Legacy item write-fence status requires an initialized Profile Session.");
		}
		return false;
	}
	return Coordinator->AreLegacyItemWritesRetired(OutDiagnostic);
}

Fdemo_mapProfilePreparationSnapshot Udemo_mapProfileSessionSubsystem::GetPreparationSnapshot() const
{
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	Fdemo_mapProfilePreparationSnapshot Snapshot;
	Snapshot.SessionState = SessionSnapshot.SessionState;
	Snapshot.ProfileId = SessionSnapshot.ProfileId;
	Snapshot.SaveGeneration = SessionSnapshot.SaveGeneration;
	Snapshot.PersistentSpiritStones = SessionSnapshot.PersistentSpiritStones;
	Snapshot.ShopStockPolicyId = SessionSnapshot.ShopStock.PolicyId;
	Snapshot.ShopStockGeneration = SessionSnapshot.ShopStock.Generation;
	Snapshot.ShopStockEventId =
		SessionSnapshot.ShopStock.ShopStockEventId;
	for (const Fdemo_mapPersistentShopStockEntry& Entry :
		SessionSnapshot.ShopStock.Entries)
	{
		Fdemo_mapProfileShopStockRow Row;
		Row.SlotId = Entry.SlotId;
		Row.SlotOrdinal = Entry.SlotOrdinal;
		Row.State = Entry.State;
		if (Entry.State
			== Edemo_mapPersistentShopStockEntryState::Available)
		{
			Row.ItemInstanceId = Entry.Item.ItemInstanceId;
			Row.ItemDefinitionId = Entry.Item.ItemDefinitionId;
			Row.AffixSet = Entry.Item.AffixSet;
			Row.BuyPrice = Entry.QuotedBuyValue;
			Fdemo_mapRewardShopStock::TryComputeSellValue(
				Entry.Item,
				Row.SellPrice);
			if (const Fdemo_mapItemDefinition* Definition =
				Fdemo_mapItemDefinitions::Find(
					Entry.Item.ItemDefinitionId))
			{
				Row.DisplayName = Definition->DisplayName.ToString();
				Row.ItemCategoryId = Definition->CategoryId;
				Row.Level = Definition->Level;
			}
		}
		Snapshot.OrderedShopStockRows.Add(MoveTemp(Row));
	}
	Snapshot.LastSettlementId = SessionSnapshot.LastSettlementId;
	Snapshot.LastTerminalReason = SessionSnapshot.LastTerminalReason;
	Snapshot.LastTradeResult = LastTradeResult;
	Snapshot.SelectedWeaponId = SessionSnapshot.PreparationLayout.WeaponItemInstanceId;
	Snapshot.SelectedArmorId = SessionSnapshot.PreparationLayout.ArmorItemInstanceId;
	Snapshot.SelectedAccessoryId = SessionSnapshot.PreparationLayout.AccessoryItemInstanceId;
	Snapshot.SelectedSpatialRingId = SessionSnapshot.PreparationLayout.SpatialRingItemInstanceId;
	Snapshot.SelectedBackpackId = SessionSnapshot.PreparationLayout.BackpackItemInstanceId;
	Snapshot.OrderedSelectedMaterialIds = SessionSnapshot.PreparationLayout.OrderedRunInventoryItemInstanceIds;
	Snapshot.HotbarBindings.SlotBindings = SessionSnapshot.PreparationLayout.HotbarItemInstanceIds;
	Snapshot.WarehouseLayout = SessionSnapshot.WarehouseLayout;
	Snapshot.bCanStartRun = SessionSnapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
		&& SessionSnapshot.bCanBeginRun
		&& SessionSnapshot.ProfileId.IsValid();
	Snapshot.bCanRetrySettlement = SessionSnapshot.bCanRetrySettlement;
	if (!PreparationDiagnostic.IsEmpty())
	{
		Snapshot.VisibleDiagnostic = PreparationDiagnostic;
	}
	else
	{
		Snapshot.VisibleDiagnostic = SessionSnapshot.VisibleDiagnostic;
	}
	const Fdemo_mapInventoryCapacityResult PreparationCapacity =
		Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
			ResolvePreparationSpatialItemDefinitionId(
				SessionSnapshot,
				SessionSnapshot.PreparationLayout),
			ResolvePreparationRingDefinitionId(
				SessionSnapshot,
				SessionSnapshot.PreparationLayout));
	const int32 QuickItemCapacity =
		Fdemo_mapPersistentPreparationLayout::BaseQuickItemSlotCount
		+ (PreparationCapacity.bSuccess
			? PreparationCapacity.RingQuickCapacity
			: 0);

	for (const Fdemo_mapPersistentItemRecord& Item : SessionSnapshot.OrderedPermanentStash)
	{
		Fdemo_mapProfilePreparationStashRow Row;
			Row.ItemInstanceId = Item.ItemInstanceId;
			Row.ItemDefinitionId = Item.ItemDefinitionId;
			Row.StackCount = Item.StackCount;
			Row.RewardEventKind = Item.RewardEventKind;
			Row.RewardEventId = Item.RewardEventId;
			Row.RewardValueMultiplierBps = Item.RewardValueMultiplierBps;
			Row.RewardSourceRoleId = Item.RewardSourceRoleId;
			Row.RareRewardEventId = Item.RareRewardEventId;
			Row.RareRewardPolicyId = Item.RareRewardPolicyId;
			Row.RareRewardTierId = Item.RareRewardTierId;
			Row.RareRewardBonusValue = Item.RareRewardBonusValue;
			Row.AffixSet = Item.AffixSet;
			Row.bSafeInPermanentStash = Item.PersistentDomain == Edemo_mapPersistentDomain::PermanentStash;
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
		if (Definition)
		{
			Row.ItemCategoryId = Definition->CategoryId;
			Row.CompatibleEquipmentSlotId = Definition->CompatibleSlotIds.Num() == 1
				? Definition->CompatibleSlotIds[0]
				: NAME_None;
			Row.bMaterialSelectionEligible = IsPreparationRunInventoryDefinition(Item.ItemDefinitionId);
		}
		Row.bSelected = IsPreparationItemSelected(Item.ItemInstanceId);
		const int32 CarriedIndex =
			Snapshot.OrderedSelectedMaterialIds.IndexOfByKey(
				Item.ItemInstanceId);
		Row.bInBaseQuickItemArea =
			CarriedIndex >= 0
			&& CarriedIndex < QuickItemCapacity;
		Snapshot.OrderedPermanentStashRows.Add(Row);
	}

	// Once cutover owns a Ready authority, every item-bearing preparation
	// field is projected from ShanmenItems. Profile data above remains useful
	// for non-item presentation only and cannot become a fallback write path.
	if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindReadyPreparationAuthority(*this))
	{
		FShanmenItemAuthoritySnapshot AuthoritySnapshot;
		Fdemo_mapShanmenPreparationAuthorityProjection Projection;
		FString ProjectionDiagnostic;
		if (Authority->TryCaptureSnapshot(AuthoritySnapshot)
			&& Fdemo_mapShanmenPreparationAdapter::BuildProjection(
				AuthoritySnapshot, SessionSnapshot.ProfileId,
				Projection, &ProjectionDiagnostic))
		{
			Snapshot.SaveGeneration = Projection.AuthorityRevision;
			Snapshot.OrderedPermanentStashRows = MoveTemp(Projection.OrderedRows);
			Snapshot.WarehouseLayout = MoveTemp(Projection.WarehouseLayout);
			Snapshot.SelectedWeaponId = Projection.SelectedWeaponId;
			Snapshot.SelectedArmorId = Projection.SelectedArmorId;
			Snapshot.SelectedAccessoryId = Projection.SelectedAccessoryId;
			Snapshot.SelectedSpatialRingId = Projection.SelectedSpatialRingId;
			Snapshot.SelectedBackpackId = Projection.SelectedBackpackId;
			Snapshot.OrderedSelectedMaterialIds =
				MoveTemp(Projection.OrderedSelectedMaterialIds);
			Snapshot.HotbarBindings = MoveTemp(Projection.HotbarBindings);
			FGuid RecoverableRunId;
			FString RecoveryDiagnostic;
			const bool bRecoverable =
				Fdemo_mapShanmenRunLifecycleAdapter::
					TryFindRecoverableActiveRun(
						*Authority, RecoverableRunId,
						&RecoveryDiagnostic);
			const bool bHasPreparedSelection =
				Projection.SelectedWeaponId.IsValid()
				|| Projection.SelectedArmorId.IsValid()
				|| Projection.SelectedAccessoryId.IsValid()
				|| Projection.SelectedSpatialRingId.IsValid()
				|| Projection.SelectedBackpackId.IsValid()
				|| !Projection.OrderedSelectedMaterialIds.IsEmpty();
			Snapshot.bCanStartRun = bRecoverable || bHasPreparedSelection;
			Snapshot.VisibleDiagnostic = bRecoverable
				? FString::Printf(
					TEXT("可恢复同一活动远征 %s；再次开始只会重建 Runtime，不会创建第二个 Run。"),
					*RecoverableRunId.ToString(
						EGuidFormats::DigitsWithHyphens))
				: !bHasPreparedSelection
					? TEXT("请先选择至少一件装备或一组完整局内物资，再使用原子 Run-start。")
				: PreparationDiagnostic.IsEmpty()
					? TEXT("ShanmenItems 战备已就绪；开始按钮使用单次原子 Run-start。")
					: PreparationDiagnostic;
		}
		else
		{
			Snapshot.bCanStartRun = false;
			Snapshot.VisibleDiagnostic = ProjectionDiagnostic.IsEmpty()
				? TEXT("Ready ShanmenItems authority could not be projected; preparation fails closed.")
				: ProjectionDiagnostic;
		}
	}
	return Snapshot;
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::SetPreparationEquipment(
	FName SlotId,
	const FGuid& ItemInstanceId)
{
	if (!IsInGameThread())
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::SessionNotReady, TEXT("Preparation selection is restricted to the Game Thread."));
	}
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	FString ContextDiagnostic;
	if (!EnsurePreparationContext(SessionSnapshot, ContextDiagnostic))
	{
		return RejectPreparation(
			SessionSnapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
				? Edemo_mapProfilePreparationSelectionStatus::StaleSelectionCleared
				: Edemo_mapProfilePreparationSelectionStatus::SessionNotReady,
			ContextDiagnostic);
	}
	if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindReadyPreparationAuthority(*this))
	{
		const Fdemo_mapShanmenPreparationAdapterResult Adapter =
			Fdemo_mapShanmenPreparationAdapter::SelectEquipment(
				*Authority, SlotId, ItemInstanceId);
		PreparationDiagnostic = Adapter.Diagnostic;
		const Edemo_mapProfilePreparationSelectionStatus Status =
			MapPreparationAdapterStatus(Adapter.Status);
		return Status == Edemo_mapProfilePreparationSelectionStatus::Accepted
			? AcceptPreparation(Adapter.Diagnostic)
			: RejectPreparation(Status, Adapter.Diagnostic);
	}
	if (!IsPreparationEquipmentSlot(SlotId))
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected, TEXT("Preparation equipment selection requires Weapon, Armor, Accessory, SpatialRing, or Backpack."));
	}

	Fdemo_mapPersistentPreparationLayout Layout = SessionSnapshot.PreparationLayout;
	FGuid* Target = SlotId == Fdemo_mapItemIds::WeaponSlot ? &Layout.WeaponItemInstanceId
		: (SlotId == Fdemo_mapItemIds::ArmorSlot ? &Layout.ArmorItemInstanceId
			: (SlotId == Fdemo_mapItemIds::AccessorySlot ? &Layout.AccessoryItemInstanceId
				: (SlotId == Fdemo_mapItemIds::SpatialRingSlot ? &Layout.SpatialRingItemInstanceId : &Layout.BackpackItemInstanceId)));
	if (!ItemInstanceId.IsValid())
	{
		Target->Invalidate();
		if (SlotId == Fdemo_mapItemIds::BackpackSlot
			|| SlotId == Fdemo_mapItemIds::SpatialRingSlot)
		{
			FString CapacityDiagnostic;
			if (!CanPreparationInventoryFit(
				SessionSnapshot,
				Layout,
				CapacityDiagnostic))
			{
				return RejectPreparation(
					Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded,
					CapacityDiagnostic);
			}
		}
		return CommitPreparationLayout(SessionSnapshot, Layout, TEXT("Persistent equipment slot cleared."));
	}

	const Fdemo_mapPersistentItemRecord* Item = SessionSnapshot.OrderedPermanentStash.FindByPredicate(
		[&ItemInstanceId](const Fdemo_mapPersistentItemRecord& Candidate)
		{
			return Candidate.ItemInstanceId == ItemInstanceId;
		});
	if (!Item || Item->PersistentDomain != Edemo_mapPersistentDomain::PermanentStash)
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::ItemNotFound, TEXT("Preparation equipment ItemInstanceId is missing, stale, or not owned by the current Permanent Stash."));
	}
	if ((Layout.WeaponItemInstanceId == ItemInstanceId && Target != &Layout.WeaponItemInstanceId)
		|| (Layout.ArmorItemInstanceId == ItemInstanceId && Target != &Layout.ArmorItemInstanceId)
		|| (Layout.AccessoryItemInstanceId == ItemInstanceId && Target != &Layout.AccessoryItemInstanceId)
		|| (Layout.SpatialRingItemInstanceId == ItemInstanceId && Target != &Layout.SpatialRingItemInstanceId)
		|| (Layout.BackpackItemInstanceId == ItemInstanceId && Target != &Layout.BackpackItemInstanceId)
		|| Layout.OrderedRunInventoryItemInstanceIds.Contains(ItemInstanceId))
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection, TEXT("The same ItemInstanceId cannot occupy two Preparation selections."));
	}
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item->ItemDefinitionId);
	if (!Definition || Item->StackCount != 1 || !Definition->CompatibleSlotIds.Contains(SlotId))
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected, TEXT("Preparation equipment selection violates the existing Definition quantity or slot compatibility."));
	}

	*Target = ItemInstanceId;
	if (SlotId == Fdemo_mapItemIds::BackpackSlot
		|| SlotId == Fdemo_mapItemIds::SpatialRingSlot)
	{
		FString CapacityDiagnostic;
		if (!CanPreparationInventoryFit(
			SessionSnapshot,
			Layout,
			CapacityDiagnostic))
		{
			return RejectPreparation(
				Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded,
				CapacityDiagnostic);
		}
	}
	return CommitPreparationLayout(SessionSnapshot, Layout, TEXT("Persistent equipment selection committed."));
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::SetPreparationMaterial(
	const FGuid& ItemInstanceId,
	bool bSelected)
{
	if (!IsInGameThread())
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::SessionNotReady, TEXT("Preparation selection is restricted to the Game Thread."));
	}
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	FString ContextDiagnostic;
	if (!EnsurePreparationContext(SessionSnapshot, ContextDiagnostic))
	{
		return RejectPreparation(
			SessionSnapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
				? Edemo_mapProfilePreparationSelectionStatus::StaleSelectionCleared
				: Edemo_mapProfilePreparationSelectionStatus::SessionNotReady,
			ContextDiagnostic);
	}
	if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindReadyPreparationAuthority(*this))
	{
		const Fdemo_mapShanmenPreparationAdapterResult Adapter =
			Fdemo_mapShanmenPreparationAdapter::SelectMaterial(
				*Authority, ItemInstanceId, bSelected);
		PreparationDiagnostic = Adapter.Diagnostic;
		const Edemo_mapProfilePreparationSelectionStatus Status =
			MapPreparationAdapterStatus(Adapter.Status);
		return Status == Edemo_mapProfilePreparationSelectionStatus::Accepted
			? AcceptPreparation(Adapter.Diagnostic)
			: RejectPreparation(Status, Adapter.Diagnostic);
	}
	const Fdemo_mapPersistentItemRecord* Item = SessionSnapshot.OrderedPermanentStash.FindByPredicate(
		[&ItemInstanceId](const Fdemo_mapPersistentItemRecord& Candidate)
		{
			return Candidate.ItemInstanceId == ItemInstanceId;
		});
	if (!Item || Item->PersistentDomain != Edemo_mapPersistentDomain::PermanentStash)
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::ItemNotFound, TEXT("Preparation material ItemInstanceId is missing, stale, or not owned by the current Permanent Stash."));
	}
	const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Item->ItemDefinitionId);
	if (!Definition || !IsPreparationRunInventoryDefinition(Item->ItemDefinitionId) || Item->StackCount <= 0
		|| Item->StackCount > Definition->MaxStackSize || !Item->EquipmentSlotId.IsNone())
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::MaterialRejected, TEXT("RunInventory accepts complete Material or Consumable Permanent Stash records."));
	}
	Fdemo_mapPersistentPreparationLayout Layout = SessionSnapshot.PreparationLayout;
	if (bSelected)
	{
		if (Layout.WeaponItemInstanceId == ItemInstanceId || Layout.ArmorItemInstanceId == ItemInstanceId
			|| Layout.AccessoryItemInstanceId == ItemInstanceId || Layout.SpatialRingItemInstanceId == ItemInstanceId
			|| Layout.BackpackItemInstanceId == ItemInstanceId
			|| Layout.OrderedRunInventoryItemInstanceIds.Contains(ItemInstanceId))
		{
			return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::DuplicateSelection, TEXT("The same ItemInstanceId cannot appear twice in Preparation."));
		}
		const Fdemo_mapInventoryCapacityResult Capacity =
			Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
				ResolvePreparationSpatialItemDefinitionId(
					SessionSnapshot,
					Layout),
				ResolvePreparationRingDefinitionId(
					SessionSnapshot,
					Layout));
		if (!Capacity.bSuccess)
		{
			return RejectPreparation(
				Edemo_mapProfilePreparationSelectionStatus::MaterialRejected,
				Capacity.Diagnostic);
		}
		if (Layout.OrderedRunInventoryItemInstanceIds.Num()
			>= Capacity.Capacity)
		{
			return RejectPreparation(
				Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded,
				FString::Printf(
					TEXT("当前战备已占满 %d 格；装备更高容量空间道具或先移回仓库。"),
					Capacity.Capacity));
		}
		Layout.OrderedRunInventoryItemInstanceIds.Add(ItemInstanceId);
	}
	else
	{
		Layout.OrderedRunInventoryItemInstanceIds.Remove(ItemInstanceId);
	}
	ReconcilePreparationHotbar(SessionSnapshot, Layout);
	return CommitPreparationLayout(SessionSnapshot, Layout, TEXT("Persistent RunInventory selection committed."));
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::SetPreparationHotbarSlot(
	int32 ExternalSlotNumber,
	const FGuid& ItemInstanceId)
{
	if (!IsInGameThread())
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::SessionNotReady, TEXT("Hotbar selection is restricted to the Game Thread."));
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	FString ContextDiagnostic;
	if (!EnsurePreparationContext(SessionSnapshot, ContextDiagnostic))
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::SessionNotReady, ContextDiagnostic);
	if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindReadyPreparationAuthority(*this))
	{
		const Fdemo_mapShanmenPreparationAdapterResult Adapter =
			Fdemo_mapShanmenPreparationAdapter::SetHotbarSlot(
				*Authority, ExternalSlotNumber, ItemInstanceId);
		PreparationDiagnostic = Adapter.Diagnostic;
		const Edemo_mapProfilePreparationSelectionStatus Status =
			MapPreparationAdapterStatus(Adapter.Status);
		return Status == Edemo_mapProfilePreparationSelectionStatus::Accepted
			? AcceptPreparation(Adapter.Diagnostic)
			: RejectPreparation(Status, Adapter.Diagnostic);
	}
	if (ExternalSlotNumber < 1 || ExternalSlotNumber > Fdemo_mapPersistentPreparationLayout::HotbarSlotCount)
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::MaterialRejected, TEXT("Hotbar slot must be 1..9."));

	Fdemo_mapPersistentPreparationLayout Layout = SessionSnapshot.PreparationLayout;
	if (!ItemInstanceId.IsValid())
	{
		Layout.HotbarItemInstanceIds[
			ExternalSlotNumber - 1].Invalidate();
		return CommitPreparationLayout(
			SessionSnapshot,
			Layout,
			TEXT("Persistent Hotbar slot cleared."));
	}
	if (!IsBaseQuickConsumable(
		SessionSnapshot,
		Layout,
		ItemInstanceId))
	{
		return RejectPreparation(
			Edemo_mapProfilePreparationSelectionStatus::
				MaterialRejected,
			TEXT("Hotbar accepts only positive-quantity Consumables in the fixed six-cell Base Quick Item area."));
	}
	for (FGuid& BoundId : Layout.HotbarItemInstanceIds)
	{
		if (BoundId == ItemInstanceId)
		{
			BoundId.Invalidate();
		}
	}
	Layout.HotbarItemInstanceIds[ExternalSlotNumber - 1] = ItemInstanceId;
	return CommitPreparationLayout(SessionSnapshot, Layout, TEXT("Persistent Hotbar selection committed."));
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::ClearPreparationSelection()
{
	if (!IsInGameThread())
	{
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::SessionNotReady, TEXT("Preparation selection is restricted to the Game Thread."));
	}
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	FString ContextDiagnostic;
	if (!EnsurePreparationContext(SessionSnapshot, ContextDiagnostic))
	{
		return RejectPreparation(
			SessionSnapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
				? Edemo_mapProfilePreparationSelectionStatus::StaleSelectionCleared
				: Edemo_mapProfilePreparationSelectionStatus::SessionNotReady,
			ContextDiagnostic);
	}
	if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindReadyPreparationAuthority(*this))
	{
		bool bChanged = false;
		const TArray<FGuid> SelectedMaterials =
			GetPreparationSnapshot().OrderedSelectedMaterialIds;
		for (const FGuid& ItemId : SelectedMaterials)
		{
			const Fdemo_mapShanmenPreparationAdapterResult Adapter =
				Fdemo_mapShanmenPreparationAdapter::SelectMaterial(
					*Authority, ItemId, false);
			if (!Adapter.IsAccepted())
			{
				PreparationDiagnostic = Adapter.Diagnostic;
				return RejectPreparation(
					MapPreparationAdapterStatus(Adapter.Status),
					Adapter.Diagnostic);
			}
			bChanged |= Adapter.Status
				== Edemo_mapShanmenPreparationAdapterStatus::Accepted;
		}
		for (FName SlotId : Fdemo_mapItemDefinitions::GetEquipmentSlotIds())
		{
			const Fdemo_mapShanmenPreparationAdapterResult Adapter =
				Fdemo_mapShanmenPreparationAdapter::SelectEquipment(
					*Authority, SlotId, FGuid());
			if (!Adapter.IsAccepted())
			{
				PreparationDiagnostic = Adapter.Diagnostic;
				return RejectPreparation(
					MapPreparationAdapterStatus(Adapter.Status),
					Adapter.Diagnostic);
			}
			bChanged |= Adapter.Status
				== Edemo_mapShanmenPreparationAdapterStatus::Accepted;
		}
		PreparationDiagnostic = bChanged
			? TEXT("All authority-native preparation selections were cleared.")
			: TEXT("Authority-native preparation selections were already empty.");
		return AcceptPreparation(PreparationDiagnostic);
	}
	return CommitPreparationLayout(
		SessionSnapshot,
		Fdemo_mapPersistentPreparationLayout(),
		TEXT("Persistent preparation layout cleared."));
}

Fdemo_mapWarehouseMoveResult
Udemo_mapProfileSessionSubsystem::MoveWarehouseItem(
	int32 SourceSlotIndex,
	int32 TargetSlotIndex)
{
	Fdemo_mapWarehouseMoveResult Result;
	if (!IsInGameThread() || !Coordinator.IsValid())
	{
		Result.Status =
			Edemo_mapWarehouseMoveStatus::SessionNotReady;
		Result.Diagnostic =
			TEXT("Warehouse moves require the Game Thread and an initialized Profile Session.");
		return Result;
	}
	if (FindReadyPreparationAuthority(*this))
	{
		Result.Status = Edemo_mapWarehouseMoveStatus::SessionNotReady;
		Result.Diagnostic =
			TEXT("P1.6 cutover rejects the legacy Profile warehouse move path; an authority-native move transaction is required.");
		PreparationDiagnostic = Result.Diagnostic;
		return Result;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = GetSnapshot();
	Fdemo_mapWarehouseMoveIntent Intent;
	Intent.ExpectedProfileId = Snapshot.ProfileId;
	Intent.ExpectedSaveGeneration = Snapshot.SaveGeneration;
	Intent.SourceSlotIndex = SourceSlotIndex;
	Intent.TargetSlotIndex = TargetSlotIndex;
	Result = Coordinator->MoveWarehouseItem(Intent);
	PreparationDiagnostic = Result.Diagnostic;
	return Result;
}

Fdemo_mapPlayerItemDropResult
Udemo_mapProfileSessionSubsystem::ExecutePreparationItemDrop(
	const Fdemo_mapPlayerItemDropIntent& Intent)
{
	Fdemo_mapPlayerItemDropResult Result;
	Result.SourceItemInstanceId = Intent.ExpectedSourceItemInstanceId;
	auto Reject = [&Result](const FString& Diagnostic)
	{
		Result.Kind = Edemo_mapPlayerItemDropKind::Reject;
		Result.Operation = Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::UIInvalidSelection,
			Diagnostic,
			Result.SourceItemInstanceId);
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	const Fdemo_mapProfileSessionSnapshot Snapshot = GetSnapshot();
	Result.CommittedAuthorityRevision = Snapshot.SaveGeneration;
	if (!IsInGameThread() || !bExplicitlyInitialized || !Coordinator.IsValid()
		|| Snapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation)
	{
		return Reject(TEXT("战备拖拽仅在已初始化的准备阶段可用。"));
	}
	if (FindReadyPreparationAuthority(*this))
	{
		return Reject(TEXT("P1.6 cutover rejects legacy preparation drag/drop; use the authority-native equipment selection route."));
	}
	if (Intent.ExpectedAuthorityRevision != Snapshot.SaveGeneration)
	{
		return Reject(TEXT("战备拖拽已过期：存档版本已变化。"));
	}

	Fdemo_mapPersistentPreparationLayout Layout = Snapshot.PreparationLayout;
	auto EquipmentRef = [&Layout](FName SlotId) -> FGuid*
	{
		if (SlotId == Fdemo_mapItemIds::WeaponSlot) return &Layout.WeaponItemInstanceId;
		if (SlotId == Fdemo_mapItemIds::ArmorSlot) return &Layout.ArmorItemInstanceId;
		if (SlotId == Fdemo_mapItemIds::AccessorySlot) return &Layout.AccessoryItemInstanceId;
		if (SlotId == Fdemo_mapItemIds::SpatialRingSlot) return &Layout.SpatialRingItemInstanceId;
		if (SlotId == Fdemo_mapItemIds::BackpackSlot) return &Layout.BackpackItemInstanceId;
		return nullptr;
	};
	auto InventoryIndex = [](Edemo_mapPlayerItemArea Area, int32 Index)
	{
		return Area == Edemo_mapPlayerItemArea::BaseQuickItems ? Index
			: (Area == Edemo_mapPlayerItemArea::SpatialStorage
				? Fdemo_mapPersistentPreparationLayout::BaseQuickItemSlotCount + Index
				: INDEX_NONE);
	};
	auto RemoveFromLayout = [&Layout, &EquipmentRef](const FGuid& ItemId)
	{
		for (FName Slot : Fdemo_mapItemDefinitions::GetEquipmentSlotIds())
		{
			if (FGuid* Value = EquipmentRef(Slot); Value && *Value == ItemId)
			{
				Value->Invalidate();
			}
		}
		Layout.OrderedRunInventoryItemInstanceIds.Remove(ItemId);
	};

	FGuid SourceId;
	if (Intent.SourceArea == Edemo_mapPlayerItemArea::Warehouse)
	{
		Fdemo_mapPersistentProfile Projection;
		Projection.WarehouseLayout = Snapshot.WarehouseLayout;
		Projection.PermanentStash = Snapshot.OrderedPermanentStash;
		const TArray<FGuid> WarehouseSlots = Fdemo_mapWarehouseSlotProjection::Build(Projection);
		if (!WarehouseSlots.IsValidIndex(Intent.SourceSlotIndex))
			return Reject(TEXT("仓库拖拽源格无效。"));
		SourceId = WarehouseSlots[Intent.SourceSlotIndex];
	}
	else if (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment)
	{
		const FGuid* Slot = EquipmentRef(Intent.SourceEquipmentSlotId);
		if (!Slot) return Reject(TEXT("战备源装备槽无效。"));
		SourceId = *Slot;
	}
	else
	{
		const int32 Index = InventoryIndex(Intent.SourceArea, Intent.SourceSlotIndex);
		if (!Layout.OrderedRunInventoryItemInstanceIds.IsValidIndex(Index))
			return Reject(TEXT("战备源物品格无效或为空。"));
		SourceId = Layout.OrderedRunInventoryItemInstanceIds[Index];
	}
	if (!SourceId.IsValid() || SourceId != Intent.ExpectedSourceItemInstanceId)
		return Reject(TEXT("战备源物品已变化或不匹配。"));
	const Fdemo_mapPersistentItemRecord* SourceItem =
		FindPreparationStashItem(Snapshot, SourceId);
	const Fdemo_mapItemDefinition* SourceDefinition = SourceItem
		? Fdemo_mapItemDefinitions::Find(SourceItem->ItemDefinitionId) : nullptr;
	if (!SourceItem || !SourceDefinition
		|| SourceItem->PersistentDomain != Edemo_mapPersistentDomain::PermanentStash)
		return Reject(TEXT("战备物品不属于当前永久仓库。"));

	Edemo_mapPlayerItemDropKind Kind = Edemo_mapPlayerItemDropKind::Move;
	if (Intent.TargetArea == Edemo_mapPlayerItemArea::Warehouse)
	{
		if (Intent.SourceArea == Edemo_mapPlayerItemArea::Warehouse)
			return Reject(TEXT("仓库内部拖拽应由仓库位置事务处理。"));
		RemoveFromLayout(SourceId);
		Kind = Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment
			? Edemo_mapPlayerItemDropKind::Unequip
			: Edemo_mapPlayerItemDropKind::Move;
	}
	else if (Intent.TargetArea == Edemo_mapPlayerItemArea::Equipment)
	{
		FGuid* Target = EquipmentRef(Intent.TargetEquipmentSlotId);
		if (!Target || !SourceDefinition->CompatibleSlotIds.Contains(Intent.TargetEquipmentSlotId)
			|| SourceItem->StackCount != 1)
			return Reject(TEXT("目标装备槽与物品定义不兼容。"));
		const FGuid Existing = *Target;
		if (Intent.SourceArea == Edemo_mapPlayerItemArea::Equipment
			&& Intent.SourceEquipmentSlotId != Intent.TargetEquipmentSlotId)
		{
			FGuid* SourceSlot = EquipmentRef(Intent.SourceEquipmentSlotId);
			const Fdemo_mapPersistentItemRecord* ExistingItem = FindPreparationStashItem(Snapshot, Existing);
			const Fdemo_mapItemDefinition* ExistingDefinition = ExistingItem
				? Fdemo_mapItemDefinitions::Find(ExistingItem->ItemDefinitionId) : nullptr;
			if (Existing.IsValid() && (!ExistingDefinition
				|| !ExistingDefinition->CompatibleSlotIds.Contains(Intent.SourceEquipmentSlotId)))
				return Reject(TEXT("目标装备不能回填到源槽，交换已拒绝。"));
			*SourceSlot = Existing;
		}
		else
		{
			RemoveFromLayout(SourceId);
		}
		*Target = SourceId;
		Kind = Existing.IsValid() ? Edemo_mapPlayerItemDropKind::Swap : Edemo_mapPlayerItemDropKind::Equip;
	}
	else
	{
		const int32 TargetIndex = InventoryIndex(Intent.TargetArea, Intent.TargetSlotIndex);
		if (TargetIndex < 0 || !IsPreparationRunInventoryDefinition(SourceItem->ItemDefinitionId))
			return Reject(TEXT("战备基础六格与空间格只接收完整材料或消耗品。"));
		const int32 SourceIndex = Layout.OrderedRunInventoryItemInstanceIds.IndexOfByKey(SourceId);
		if (Intent.SourceArea == Edemo_mapPlayerItemArea::Warehouse)
		{
			if (Layout.OrderedRunInventoryItemInstanceIds.Contains(SourceId))
				return Reject(TEXT("同一物品不能同时占用两个战备位置。"));
			if (TargetIndex < Layout.OrderedRunInventoryItemInstanceIds.Num())
			{
				Layout.OrderedRunInventoryItemInstanceIds[TargetIndex] = SourceId;
				Kind = Edemo_mapPlayerItemDropKind::Swap;
			}
			else
			{
				Layout.OrderedRunInventoryItemInstanceIds.Insert(SourceId,
					FMath::Min(TargetIndex, Layout.OrderedRunInventoryItemInstanceIds.Num()));
			}
		}
		else if (SourceIndex != INDEX_NONE)
		{
			if (TargetIndex >= Layout.OrderedRunInventoryItemInstanceIds.Num())
			{
				Layout.OrderedRunInventoryItemInstanceIds.RemoveAt(SourceIndex);
				Layout.OrderedRunInventoryItemInstanceIds.Add(SourceId);
			}
			else if (TargetIndex != SourceIndex)
			{
				Layout.OrderedRunInventoryItemInstanceIds.Swap(SourceIndex, TargetIndex);
				Kind = Edemo_mapPlayerItemDropKind::Swap;
			}
		}
		else
		{
			return Reject(TEXT("装备不能直接放入战备物品格。"));
		}
	}
	ReconcilePreparationHotbar(Snapshot, Layout);
	FString CapacityDiagnostic;
	if (!CanPreparationInventoryFit(Snapshot, Layout, CapacityDiagnostic))
		return Reject(CapacityDiagnostic);
	const Fdemo_mapProfilePreparationSelectionResult Commit =
		CommitPreparationLayout(Snapshot, Layout, TEXT("P2 战备拖拽已持久化提交。"));
	Result.CommittedAuthorityRevision = Commit.Snapshot.SaveGeneration;
	Result.Kind = Commit.IsAccepted() ? Kind : Edemo_mapPlayerItemDropKind::Reject;
	Result.bCommitted = Commit.IsAccepted();
	Result.Operation = Commit.IsAccepted()
		? Fdemo_mapItemOperationResult::Success(SourceId)
		: Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InvariantViolation,
			Commit.Diagnostic,
			SourceId);
	Result.Diagnostic = Commit.Diagnostic;
	return Result;
}

Fdemo_mapProfileSessionBeginResult Udemo_mapProfileSessionSubsystem::StartRunWithoutPreparation()
{
	if (!IsInGameThread())
	{
		return RejectBegin(TEXT("Start Run is restricted to the Game Thread."));
	}
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	if (SessionSnapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
		|| !SessionSnapshot.bCanBeginRun
		|| !SessionSnapshot.ProfileId.IsValid())
	{
		return RejectBegin(TEXT("Start Run requires an initialized, ready Profile session."));
	}

	// Product Start Run is deliberately independent of every legacy preparation
	// selection: empty/missing/stale layouts and spatial references are data only,
	// never a start gate or an initializer for the Runtime authority.
	Fdemo_mapBeginRunRequest Request;
	Request.ExpectedProfileId = SessionSnapshot.ProfileId;
	Request.ExpectedSaveGeneration = SessionSnapshot.SaveGeneration;
	Request.bRequireCommittedPreparationLayout = false;
	UE_LOG(LogTemp, Log, TEXT("P4X_START_RUN_DIRECT profile=%s generation=%d preparation_layout_ignored=1"),
		*SessionSnapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens), SessionSnapshot.SaveGeneration);
	return BeginRun(Request);
}

Fdemo_mapProfileSessionBeginResult Udemo_mapProfileSessionSubsystem::StartPreparedRun()
{
	if (!IsInGameThread())
	{
		return RejectBegin(TEXT("Preparation Start Run is restricted to the Game Thread."));
	}
	const Fdemo_mapProfileSessionSnapshot SessionSnapshot = GetSnapshot();
	FString ContextDiagnostic;
	if (!EnsurePreparationContext(SessionSnapshot, ContextDiagnostic))
	{
		return RejectBegin(ContextDiagnostic);
	}

	Fdemo_mapBeginRunRequest Request;
	Request.ExpectedProfileId = SessionSnapshot.ProfileId;
	Request.ExpectedSaveGeneration = SessionSnapshot.SaveGeneration;
	Request.bRequireCommittedPreparationLayout = true;
	return BeginRun(Request);
}

Fdemo_mapSpiritStonePickupResult Udemo_mapProfileSessionSubsystem::CollectFixedSpiritStone()
{
	return CollectSpiritStone(
		Fdemo_mapSpiritStonePickupContract::PickupId,
		Fdemo_mapSpiritStonePickupContract::SourceId,
		Fdemo_mapSpiritStonePickupContract::Value);
}

Fdemo_mapSpiritStonePickupResult Udemo_mapProfileSessionSubsystem::CollectSpiritStone(
	FName PickupId,
	FName SourceId,
	int64 Value)
{
	Fdemo_mapSpiritStonePickupResult Result;
	if (!IsInGameThread() || !bExplicitlyInitialized || !Coordinator.IsValid())
	{
		Result.Status = Edemo_mapSpiritStonePickupStatus::RunNotActive;
		Result.Diagnostic = TEXT("Fixed Spirit Stone collection requires an initialized Game Thread session.");
		return Result;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = Coordinator->GetSnapshot();
	Fdemo_mapSpiritStonePickupIntent Intent;
	Intent.ExpectedProfileId = Snapshot.ProfileId;
	Intent.ExpectedSaveGeneration = Snapshot.SaveGeneration;
	Intent.ExpectedActiveRunId = Snapshot.ActiveRunId;
	Intent.PickupId = PickupId;
	Intent.SourceId = SourceId;
	Intent.Value = Value;
	Result = Coordinator->CollectSpiritStone(Intent);
	if (Result.IsCommitted())
	{
		RecentSpiritStoneGain = Value;
		RecentSpiritStoneGainExpirySeconds = FPlatformTime::Seconds() + 2.5;
	}
	return Result;
}

int64 Udemo_mapProfileSessionSubsystem::GetRecentSpiritStoneGain() const
{
	return RecentSpiritStoneGain > 0
		&& FPlatformTime::Seconds() <= RecentSpiritStoneGainExpirySeconds
		? RecentSpiritStoneGain
		: 0;
}

Fdemo_mapTownUpgradeResult Udemo_mapProfileSessionSubsystem::RequestTownUpgrade()
{
	Fdemo_mapTownUpgradeResult Result;
	if (!IsInGameThread() || !bExplicitlyInitialized || !Coordinator.IsValid())
	{
		Result.Status = Edemo_mapTownUpgradeStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Town upgrade requires an initialized Game Thread session.");
		return Result;
	}
	const Fdemo_mapProfileSessionSnapshot Snapshot = Coordinator->GetSnapshot();
	Fdemo_mapTownUpgradeCost Cost;
	if (!Fdemo_mapTownProgressionRules::TryGetNextLevelCost(Snapshot.TownLevel, Cost))
	{
		Result.Status = Edemo_mapTownUpgradeStatus::MaxTownLevel;
		Result.Diagnostic = TEXT("Town construction is complete at level 5.");
		return Result;
	}
	Fdemo_mapTownUpgradeIntent Intent;
	Intent.ExpectedProfileId = Snapshot.ProfileId;
	Intent.ExpectedSaveGeneration = Snapshot.SaveGeneration;
	Intent.ExpectedTownLevel = Snapshot.TownLevel;
	Intent.RequestedNextLevel = Snapshot.TownLevel + 1;
	Intent.ExpectedSpiritWoodCost = Cost.SpiritWood;
	Intent.ExpectedSpiritOreCost = Cost.SpiritOre;
	Intent.ExpectedSpiritStoneCost = Cost.SpiritStones;
	return Coordinator->SubmitTownUpgrade(Intent);
}

#if WITH_DEV_AUTOMATION_TESTS
void Udemo_mapProfileSessionSubsystem::SetNextRepositoryFailureForAutomation(
	Edemo_mapProfileFailureStage Stage)
{
	if (Coordinator.IsValid())
	{
		Coordinator->SetNextRepositoryFailureForAutomation(Stage);
	}
}
#endif

Fdemo_mapProfileSessionInitializeResult Udemo_mapProfileSessionSubsystem::RejectInitialize(
	const FString& Diagnostic) const
{
	Fdemo_mapProfileSessionInitializeResult Result;
	Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionBeginResult Udemo_mapProfileSessionSubsystem::RejectBegin(
	const FString& Diagnostic) const
{
	Fdemo_mapProfileSessionBeginResult Result;
	Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Udemo_mapProfileSessionSubsystem::RejectSettlement(
	Edemo_mapProfileSessionSettlementStatus Status,
	const FString& Diagnostic) const
{
	Fdemo_mapProfileSessionSettlementResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = GetSnapshot();
	return Result;
}

Fdemo_mapProfileSessionInitializeResult Udemo_mapProfileSessionSubsystem::CurrentInitializeResult(
	const FString& Diagnostic) const
{
	Fdemo_mapProfileSessionInitializeResult Result;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = GetSnapshot();
	switch (Result.Snapshot.SessionState)
	{
	case Edemo_mapProfileSessionState::ReadyForPreparation:
		Result.Status = Edemo_mapProfileSessionInitializeStatus::Ready;
		break;
	case Edemo_mapProfileSessionState::FatalProfileError:
		Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
		break;
	default:
		Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveryRequired;
		break;
	}
	return Result;
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::AcceptPreparation(
	const FString& Diagnostic) const
{
	Fdemo_mapProfilePreparationSelectionResult Result;
	Result.Status = Edemo_mapProfilePreparationSelectionStatus::Accepted;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = GetPreparationSnapshot();
	return Result;
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::RejectPreparation(
	Edemo_mapProfilePreparationSelectionStatus Status,
	const FString& Diagnostic) const
{
	Fdemo_mapProfilePreparationSelectionResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = GetPreparationSnapshot();
	Result.Snapshot.VisibleDiagnostic = Diagnostic;
	return Result;
}

Fdemo_mapProfilePreparationSelectionResult Udemo_mapProfileSessionSubsystem::CommitPreparationLayout(
	const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
	const Fdemo_mapPersistentPreparationLayout& Layout,
	const FString& SuccessDiagnostic)
{
	if (!Coordinator.IsValid())
		return RejectPreparation(Edemo_mapProfilePreparationSelectionStatus::SessionNotReady, TEXT("Profile coordinator is unavailable."));
	Fdemo_mapPersistentPreparationCommitIntent Intent;
	Intent.ExpectedProfileId = SessionSnapshot.ProfileId;
	Intent.ExpectedSaveGeneration = SessionSnapshot.SaveGeneration;
	Intent.Layout = Layout;
	const Fdemo_mapPersistentPreparationCommitResult Commit = Coordinator->CommitPreparationLayout(Intent);
	PreparationDiagnostic = Commit.IsSuccess() ? SuccessDiagnostic : Commit.Diagnostic;
	if (!Commit.IsSuccess())
		return RejectPreparation(
			Commit.Status == Edemo_mapPersistentPreparationCommitStatus::ProfileGenerationMismatch
				? Edemo_mapProfilePreparationSelectionStatus::StaleSelectionCleared
				: Edemo_mapProfilePreparationSelectionStatus::MaterialRejected,
			Commit.Diagnostic);
	BindPreparationIdentity(Coordinator->GetSnapshot());
	return AcceptPreparation(PreparationDiagnostic);
}

bool Udemo_mapProfileSessionSubsystem::EnsurePreparationContext(
	const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
	FString& OutDiagnostic)
{
	if (SessionSnapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
		|| !SessionSnapshot.bCanBeginRun
		|| !SessionSnapshot.ProfileId.IsValid())
	{
		OutDiagnostic = TEXT("Preparation selection requires an explicitly initialized ReadyForPreparation session.");
		ClearPreparationSelectionInternal(OutDiagnostic);
		return false;
	}
	BindPreparationIdentity(SessionSnapshot);
	return true;
}

bool Udemo_mapProfileSessionSubsystem::IsPreparationIdentityCurrent(
	const Fdemo_mapProfileSessionSnapshot& SessionSnapshot) const
{
	return bPreparationIdentityBound
		&& PreparationProfileId == SessionSnapshot.ProfileId
		&& PreparationSaveGeneration == SessionSnapshot.SaveGeneration;
}

bool Udemo_mapProfileSessionSubsystem::IsPreparationItemSelected(const FGuid& ItemInstanceId) const
{
	const Fdemo_mapPersistentPreparationLayout& Layout = GetSnapshot().PreparationLayout;
	return Layout.WeaponItemInstanceId == ItemInstanceId
		|| Layout.ArmorItemInstanceId == ItemInstanceId
		|| Layout.AccessoryItemInstanceId == ItemInstanceId
		|| Layout.SpatialRingItemInstanceId == ItemInstanceId
		|| Layout.BackpackItemInstanceId == ItemInstanceId
		|| Layout.OrderedRunInventoryItemInstanceIds.Contains(ItemInstanceId);
}

void Udemo_mapProfileSessionSubsystem::BindPreparationIdentity(
	const Fdemo_mapProfileSessionSnapshot& SessionSnapshot)
{
	PreparationProfileId = SessionSnapshot.ProfileId;
	PreparationSaveGeneration = SessionSnapshot.SaveGeneration;
	bPreparationIdentityBound = true;
}

void Udemo_mapProfileSessionSubsystem::ClearPreparationSelectionInternal(const FString& Diagnostic)
{
	PreparationProfileId.Invalidate();
	PreparationSaveGeneration = 0;
	SelectedWeaponId.Invalidate();
	SelectedArmorId.Invalidate();
	SelectedAccessoryId.Invalidate();
	SelectedMaterialIds.Reset();
	PreparationDiagnostic = Diagnostic;
	bPreparationIdentityBound = false;
}

void Udemo_mapProfileSessionSubsystem::ApplyBeginResultToPreparation(
	const Fdemo_mapProfileSessionBeginResult& Result)
{
	if (Result.Status == Edemo_mapProfileSessionBeginStatus::PersistentCommitRejected
		&& Result.Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation
		&& IsPreparationIdentityCurrent(Result.Snapshot))
	{
		PreparationDiagnostic = Result.Diagnostic;
		return;
	}
	ClearPreparationSelectionInternal(Result.Diagnostic);
}

void Udemo_mapProfileSessionSubsystem::SynchronizePreparationWithSession(
	const Fdemo_mapProfileSessionSnapshot& SessionSnapshot,
	const FString& Diagnostic)
{
	if (SessionSnapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation
		|| (bPreparationIdentityBound && !IsPreparationIdentityCurrent(SessionSnapshot)))
	{
		ClearPreparationSelectionInternal(Diagnostic);
	}
}
