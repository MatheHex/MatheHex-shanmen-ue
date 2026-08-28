#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemTags.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemMetadataAdapter.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	void AppendRewardMetadata(
		const FShanmenItemRewardMetadata& Metadata,
		TArray<FString>& Parts)
	{
		Parts.Add(FString::FromInt(static_cast<int32>(Metadata.RewardEventKind)));
		Parts.Add(GuidDigits(Metadata.RewardEventId));
		Parts.Add(FString::FromInt(Metadata.RewardValueMultiplierBps));
		Parts.Add(Metadata.RewardSourceRoleId.ToString());
		Parts.Add(GuidDigits(Metadata.RareRewardEventId));
		Parts.Add(Metadata.RareRewardPolicyId.ToString());
		Parts.Add(Metadata.RareRewardTierId.ToString());
		Parts.Add(FString::Printf(TEXT("%lld"), Metadata.RareRewardBonusValue));
		Parts.Add(GuidDigits(Metadata.AffixSetEventId));
		Parts.Add(Metadata.AffixPolicyId.ToString());
		Parts.Add(FString::FromInt(static_cast<int32>(Metadata.AffixAcquisition)));
		Parts.Add(FString::FromInt(Metadata.Affixes.Num()));
		for (const FShanmenItemResolvedRewardAffix& Affix : Metadata.Affixes)
		{
			Parts.Add(Affix.AffixId.ToString());
			Parts.Add(FString::FromInt(static_cast<int32>(Affix.Tier)));
			Parts.Add(FString::FromInt(Affix.ResolvedMagnitudeScaled));
			Parts.Add(FString::Printf(TEXT("%lld"), Affix.ResolvedValue));
		}
	}

	FShanmenOperationContext MakeContext(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared,
		const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = Prepared.ScopeId;
		Context.OwnerId = Prepared.OwnerId;
		Context.RequestId = RequestId;
		Context.Content = Snapshot.Content;
		return Context;
	}

	FName EquipmentSlotFor(
		const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared,
		const FGuid& ItemId)
	{
		if (Prepared.WeaponItemInstanceId == ItemId)
			return Fdemo_mapItemIds::WeaponSlot;
		if (Prepared.ArmorItemInstanceId == ItemId)
			return Fdemo_mapItemIds::ArmorSlot;
		if (Prepared.AccessoryItemInstanceId == ItemId)
			return Fdemo_mapItemIds::AccessorySlot;
		if (Prepared.SpatialRingItemInstanceId == ItemId)
			return Fdemo_mapItemIds::SpatialRingSlot;
		if (Prepared.BackpackItemInstanceId == ItemId)
			return Fdemo_mapItemIds::BackpackSlot;
		return NAME_None;
	}

	bool BuildRuntimePlan(
		const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared,
		const FShanmenItemTransactionReceipt& Claim,
		Fdemo_mapCommittedRunLoadoutPlan& OutPlan,
		FString& OutDiagnostic)
	{
		OutPlan = Fdemo_mapCommittedRunLoadoutPlan();
		const bool bLifecycleStart = Claim.Operation
			== EShanmenItemTransactionOperation::ClaimPreparedRun
			|| Claim.Operation
				== EShanmenItemTransactionOperation::StartPreparedRun;
		if (!Prepared.IsValid() || !Claim.IsSuccess() || !bLifecycleStart
			|| Claim.ItemInstanceId != Prepared.BatchRequestId
			|| Claim.ReservationIds.Num() != Prepared.OrderedLines.Num())
		{
			OutDiagnostic =
				TEXT("Runtime plan requires one matching durable prepared-Run claim.");
			return false;
		}
		OutPlan.ProfileId = Prepared.OwnerId;
		OutPlan.CommittedGeneration = Claim.AuthorityRevision;
		OutPlan.ActiveRunId = Claim.ReservationId;
		for (const Fdemo_mapShanmenPreparedLoadoutLine& Line :
			Prepared.OrderedLines)
		{
			Fdemo_mapPersistentItemRecord& Item =
				OutPlan.OrderedItems.AddDefaulted_GetRef();
			Item.ItemInstanceId = Line.ItemInstanceId;
			Item.ItemDefinitionId = Line.ItemDefinitionId;
			Item.StackCount = Line.ResourceKind
				== EShanmenItemResourceKind::DeploymentLock ? 1 : Line.Amount;
			Item.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
			Item.EquipmentSlotId = EquipmentSlotFor(
				Prepared, Line.ItemInstanceId);
			if (Line.ResourceKind
				== EShanmenItemResourceKind::DeploymentLock)
			{
				if (Item.EquipmentSlotId.IsNone())
				{
					OutDiagnostic =
						TEXT("Prepared equipment line has no frozen Runtime slot.");
					return false;
				}
			}
			else if (Line.ResourceKind != EShanmenItemResourceKind::Quantity
				|| !Line.SourceContainerId.IsValid()
				|| Line.SourceSlotIndex < 0)
			{
				OutDiagnostic =
					TEXT("Prepared Quantity line has no recoverable source placement.");
				return false;
			}
			OutPlan.DeployedItemIds.Add(Line.ItemInstanceId);
		}
		OutPlan.HotbarItemInstanceIds = Prepared.HotbarItemInstanceIds;
		auto DefinitionFor = [&Prepared](const FGuid& ItemId)
		{
			const Fdemo_mapShanmenPreparedLoadoutLine* Line = ItemId.IsValid()
				? Prepared.OrderedLines.FindByPredicate(
					[&ItemId](const Fdemo_mapShanmenPreparedLoadoutLine& Candidate)
					{
						return Candidate.ItemInstanceId == ItemId;
					}) : nullptr;
			return Line ? Line->ItemDefinitionId : NAME_None;
		};
		const FName BackpackDefinition =
			DefinitionFor(Prepared.BackpackItemInstanceId);
		const FName RingDefinition =
			DefinitionFor(Prepared.SpatialRingItemInstanceId);
		const Fdemo_mapInventoryCapacityResult Capacity =
			Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
				BackpackDefinition, RingDefinition);
		if (!Capacity.bSuccess)
		{
			OutDiagnostic = Capacity.Diagnostic;
			return false;
		}
		OutPlan.RunInventoryCapacity = Capacity.Capacity;
		OutDiagnostic.Reset();
		return true;
	}

	FGuid MakeFinalizeRequestId(
		const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared,
		const FGuid& ActiveRunId,
		EShanmenItemRunTerminalReason TerminalReason,
		const TArray<FShanmenItemRunSecuredOriginal>& Originals,
		const TArray<FShanmenItemRunAcquiredItem>& AcquiredItems)
	{
		TArray<FString> Parts =
		{
			GuidDigits(Prepared.OwnerId), GuidDigits(Prepared.ScopeId),
			GuidDigits(Prepared.BatchRequestId), GuidDigits(ActiveRunId),
			FString::FromInt(static_cast<int32>(TerminalReason)),
			FString::FromInt(Originals.Num())
		};
		for (const FShanmenItemRunSecuredOriginal& Original : Originals)
		{
			Parts.Add(GuidDigits(Original.ItemInstanceId));
			Parts.Add(FString::FromInt(Original.RemainingQuantity));
		}
		Parts.Add(FString::FromInt(AcquiredItems.Num()));
		for (const FShanmenItemRunAcquiredItem& Acquired : AcquiredItems)
		{
			Parts.Add(GuidDigits(Acquired.ItemInstanceId));
			Parts.Add(Acquired.Definition.DefinitionId.ToString());
			Parts.Add(FString::FromInt(Acquired.Quantity));
			AppendRewardMetadata(Acquired.RewardMetadata, Parts);
			Parts.Add(Acquired.ChildContainerType.ToString());
			Parts.Add(FString::FromInt(Acquired.ChildContainerCapacity));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenRun.Finalize.r3"), Parts);
	}

	bool MapTerminalReason(
		Edemo_mapRunEndReason RuntimeReason,
		EShanmenItemRunTerminalReason& OutReason)
	{
		switch (RuntimeReason)
		{
		case Edemo_mapRunEndReason::Extraction:
			OutReason = EShanmenItemRunTerminalReason::Extraction;
			return true;
		case Edemo_mapRunEndReason::Death:
			OutReason = EShanmenItemRunTerminalReason::Death;
			return true;
		case Edemo_mapRunEndReason::Abandon:
			OutReason = EShanmenItemRunTerminalReason::Abandon;
			return true;
		default:
			OutReason = EShanmenItemRunTerminalReason::None;
			return false;
		}
	}

	bool BuildAcquiredItem(
		const Fdemo_mapRuntimeSettlementItem& RuntimeItem,
		FShanmenItemRunAcquiredItem& OutItem,
		FString& OutDiagnostic)
	{
		const Fdemo_mapItemDefinition* ProductDefinition =
			Fdemo_mapItemDefinitions::Find(RuntimeItem.ItemDefinitionId);
		if (!ProductDefinition || RuntimeItem.StackCount <= 0
			|| RuntimeItem.StackCount > ProductDefinition->MaxStackSize)
		{
			OutDiagnostic =
				TEXT("Runtime acquisition has an unknown definition or illegal stack size.");
			return false;
		}
		OutItem = FShanmenItemRunAcquiredItem();
		OutItem.ItemInstanceId = RuntimeItem.ItemInstanceId;
		OutItem.Definition.DefinitionId = ProductDefinition->DefinitionId;
		OutItem.Definition.MaxStack = ProductDefinition->MaxStackSize;
		OutItem.Quantity = RuntimeItem.StackCount;
		if (!Fdemo_mapShanmenItemMetadataAdapter::FromRuntimeItem(
				RuntimeItem, OutItem.RewardMetadata, OutDiagnostic))
		{
			return false;
		}
		const bool bQuantityDefinition =
			ProductDefinition->MaxStackSize > 1
			|| ProductDefinition->CategoryId
				== Fdemo_mapItemIds::MaterialCategory
			|| ProductDefinition->CategoryId
				== Fdemo_mapItemIds::ConsumableCategory;
		if (bQuantityDefinition)
		{
			OutItem.Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityConsumeQuantity());
		}
		else if (!ProductDefinition->CompatibleSlotIds.IsEmpty())
		{
			OutItem.Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityDeploy());
		}
		if (ProductDefinition->CategoryId
			== Fdemo_mapItemIds::BackpackCategory)
		{
			const Fdemo_mapSpatialStorageCapacityResult Capacity =
				Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(
					ProductDefinition->DefinitionId);
			if (!Capacity.bSuccess || Capacity.Capacity <= 0)
			{
				OutDiagnostic = Capacity.Diagnostic;
				return false;
			}
			OutItem.ChildContainerType = FName(TEXT("Backpack"));
			OutItem.ChildContainerCapacity = Capacity.Capacity;
		}
		else if (ProductDefinition->CategoryId
			== Fdemo_mapItemIds::SpatialRingCategory)
		{
			const Fdemo_mapSpatialRingCapacityResult Capacity =
				Fdemo_mapItemDefinitions::ResolveSpatialRingCapacity(
					ProductDefinition->DefinitionId);
			if (!Capacity.bSuccess || Capacity.Capacity <= 0)
			{
				OutDiagnostic = Capacity.Diagnostic;
				return false;
			}
			OutItem.ChildContainerType = FName(TEXT("SpatialRing"));
			OutItem.ChildContainerCapacity = Capacity.Capacity;
		}
		if (!OutItem.IsValid())
		{
			OutDiagnostic =
				TEXT("Runtime acquisition could not form one canonical authority item.");
			return false;
		}
		OutDiagnostic.Reset();
		return true;
	}
}

bool Fdemo_mapShanmenRunLifecycleAdapter::TryFindRecoverableActiveRun(
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	FGuid& OutActiveRunId,
	FString* OutDiagnostic)
{
	OutActiveRunId.Invalidate();
	Fdemo_mapShanmenRunCorrelation Correlation;
	if (!TryGetActiveRunCorrelation(Authority, Correlation, OutDiagnostic))
	{
		return false;
	}
	OutActiveRunId = Correlation.ActiveRunId;
	return true;
}

bool Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapShanmenRunCorrelation& OutCorrelation,
	FString* OutDiagnostic)
{
	OutCorrelation = Fdemo_mapShanmenRunCorrelation();
	Fdemo_mapShanmenPreparedLoadoutReceipt Prepared;
	FShanmenItemTransactionReceipt Lifecycle;
	FString Diagnostic;
	if (!Fdemo_mapShanmenPreparationAdapter::TryInspectActivePreparedLoadout(
			Authority, Prepared, Lifecycle, &Diagnostic))
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic = Diagnostic;
		}
		return false;
	}
	if (!Fdemo_mapShanmenRunCorrelation::Build(
			Prepared, Lifecycle, OutCorrelation, Diagnostic))
	{
		if (OutDiagnostic)
		{
			*OutDiagnostic = Diagnostic;
		}
		return false;
	}
	if (OutDiagnostic)
	{
		*OutDiagnostic =
			TEXT("Durable active-Run correlation was reconstructed read-only from ShanmenItems receipts.");
	}
	return true;
}

Fdemo_mapShanmenRunStartResult
Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Udemo_mapItemSubsystem& Runtime)
{
	Fdemo_mapShanmenRunStartResult Result;
	auto Reject = [&Result](
		Edemo_mapShanmenRunLifecycleStatus Status,
		const FString& Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady,
			TEXT("Prepared Run requires the Ready authority and Runtime on the Game Thread."));
	}

	const Fdemo_mapShanmenPreparedLoadoutResult Prepared =
		Fdemo_mapShanmenPreparationAdapter::StartPreparedLoadout(Authority);
	if (!Prepared.IsCommitted())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::PreparedLoadoutRejected,
			Prepared.Diagnostic);
	}
	Result.PreparedLoadout = Prepared.Receipt;
	Result.StartCommand = Prepared.Command;
	if (!Result.StartCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityStartRejected,
			TEXT("Prepared loadout has no durable atomic-start receipt."));
	}
	Result.ActiveRunId = Result.StartCommand.Receipt.ReservationId;
	if (!Fdemo_mapShanmenRunCorrelation::Build(
			Result.PreparedLoadout, Result.StartCommand.Receipt,
			Result.RunCorrelation, Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenRunLifecycleStatus::AuthorityStartRejected;
		return Result;
	}

	if (Runtime.GetRunState() == Edemo_mapRunState::Active)
	{
		return Runtime.GetActiveRunId() == Result.ActiveRunId
			? Reject(Edemo_mapShanmenRunLifecycleStatus::Resumed,
				TEXT("Runtime already owns the exact durable ActiveRunId."))
			: Reject(Edemo_mapShanmenRunLifecycleStatus::RuntimeConflict,
				TEXT("Runtime owns a different active Run and cannot consume this receipt."));
	}
	const Fdemo_mapItemOperationResult PreparedRuntime =
		Runtime.PrepareForPersistentRun();
	if (!PreparedRuntime.bSuccess)
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::RuntimeConflict,
			PreparedRuntime.Diagnostic);
	}
	Fdemo_mapPreparedRunRuntimeRequest RuntimeRequest;
	if (!BuildRuntimePlan(
		Prepared.Receipt, Result.StartCommand.Receipt,
		RuntimeRequest.CommittedPlan, Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenRunLifecycleStatus::RuntimeMaterializationRejected;
		return Result;
	}
	Result.RuntimeResult = Runtime.MaterializePreparedRun(RuntimeRequest);
	if (!Result.RuntimeResult.IsMaterialized())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::RuntimeMaterializationRejected,
			Result.RuntimeResult.Diagnostic);
	}
	Result.Status = Result.StartCommand.Status
		== EShanmenItemDurableCommandStatus::Replayed
		? Edemo_mapShanmenRunLifecycleStatus::Resumed
		: Edemo_mapShanmenRunLifecycleStatus::Started;
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenRunLifecycleStatus::Started
		? TEXT("Preparation commit and ActiveRun publication were durably atomic before Runtime materialization.")
		: TEXT("The same atomic prepared Run was reconstructed and rematerialized after Runtime restart.");
	return Result;
}

Fdemo_mapShanmenRunFinalizeResult
Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapSettlementSummary& Summary)
{
	Fdemo_mapShanmenRunFinalizeResult Result;
	auto Reject = [&Result](
		Edemo_mapShanmenRunLifecycleStatus Status,
		const FString& Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady,
			TEXT("Settlement finalization requires the Ready authority on the Game Thread."));
	}
	if (!Summary.bValid || !Summary.RuntimeSnapshot.bValid
		|| !Summary.RunId.IsValid()
		|| Summary.RunId != Summary.RuntimeSnapshot.ActiveRunId)
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::SettlementInvalid,
			TEXT("Runtime settlement snapshot is absent, stale, or internally inconsistent."));
	}
	if (Summary.Reason != Summary.RuntimeSnapshot.CommittedEndReason)
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::SettlementInvalid,
			TEXT("Runtime summary and immutable settlement snapshot disagree on terminal reason."));
	}
	EShanmenItemRunTerminalReason TerminalReason =
		EShanmenItemRunTerminalReason::None;
	if (!MapTerminalReason(Summary.Reason, TerminalReason))
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::UnsupportedTerminalReason,
			TEXT("Only Extraction, Death, and player Abandon close a claimed prepared Run."));
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady,
			TEXT("Authority disappeared before terminal reconciliation."));
	}
	const FShanmenItemProcessedRequestSnapshot* ExistingFinalize =
		Snapshot.ProcessedRequests.FindByPredicate(
			[&Summary](const FShanmenItemProcessedRequestSnapshot& Processed)
			{
				return Processed.Receipt.IsSuccess()
					&& Processed.Receipt.Operation
						== EShanmenItemTransactionOperation::FinalizePreparedRun
					&& Processed.Receipt.ReservationId == Summary.RunId;
			});
	if (ExistingFinalize)
	{
		Result.Status = Edemo_mapShanmenRunLifecycleStatus::NoChange;
		Result.Diagnostic =
			TEXT("The exact ActiveRunId already has a durable terminal marker.");
		Result.ActiveRunId = Summary.RunId;
		Result.FinalizeCommand.Status =
			EShanmenItemDurableCommandStatus::Replayed;
		Result.FinalizeCommand.Receipt = ExistingFinalize->Receipt;
		return Result;
	}

	const Fdemo_mapShanmenPreparedLoadoutResult Prepared =
		Fdemo_mapShanmenPreparationAdapter::StartPreparedLoadout(Authority);
	if (!Prepared.IsCommitted())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::PreparedLoadoutRejected,
			Prepared.Diagnostic);
	}
	TMap<FGuid, const Fdemo_mapShanmenPreparedLoadoutLine*> PreparedById;
	for (const Fdemo_mapShanmenPreparedLoadoutLine& Line :
		Prepared.Receipt.OrderedLines)
	{
		if (PreparedById.Contains(Line.ItemInstanceId))
		{
			return Reject(
				Edemo_mapShanmenRunLifecycleStatus::PreparedLoadoutRejected,
				TEXT("Prepared receipt contains a duplicate item identity."));
		}
		PreparedById.Add(Line.ItemInstanceId, &Line);
	}
	TMap<FGuid, int32> SecuredById;
	TArray<FShanmenItemRunAcquiredItem> AcquiredItems;
	for (const Fdemo_mapRuntimeSettlementItem& Secured :
		Summary.RuntimeSnapshot.OrderedSecuredItems)
	{
		if (!Secured.ItemInstanceId.IsValid()
			|| Secured.StackCount <= 0
			|| SecuredById.Contains(Secured.ItemInstanceId)
			|| AcquiredItems.ContainsByPredicate(
				[&Secured](const FShanmenItemRunAcquiredItem& Candidate)
				{
					return Candidate.ItemInstanceId
						== Secured.ItemInstanceId;
				}))
		{
			return Reject(
				Edemo_mapShanmenRunLifecycleStatus::UnknownSecuredItem,
				TEXT("Settlement contains a duplicate identity or invalid stack."));
		}
		const Fdemo_mapShanmenPreparedLoadoutLine* const* PreparedLine =
			PreparedById.Find(Secured.ItemInstanceId);
		if (PreparedLine && *PreparedLine)
		{
			if (Secured.ItemDefinitionId
				!= (*PreparedLine)->ItemDefinitionId)
			{
				return Reject(
					Edemo_mapShanmenRunLifecycleStatus::UnknownSecuredItem,
					TEXT("Prepared Runtime identity changed definition before settlement."));
			}
			SecuredById.Add(Secured.ItemInstanceId, Secured.StackCount);
			continue;
		}
		if (TerminalReason != EShanmenItemRunTerminalReason::Extraction
			|| Secured.OriginRunId != Summary.RunId)
		{
			return Reject(
				Edemo_mapShanmenRunLifecycleStatus::AcquiredItemRejected,
				TEXT("Only an extraction may import a Runtime identity created by the exact active Run."));
		}
		FShanmenItemRunAcquiredItem Acquired;
		FString AcquisitionDiagnostic;
		if (!BuildAcquiredItem(
				Secured, Acquired, AcquisitionDiagnostic))
		{
			return Reject(
				Edemo_mapShanmenRunLifecycleStatus::AcquiredItemRejected,
				AcquisitionDiagnostic);
		}
		AcquiredItems.Add(MoveTemp(Acquired));
	}
	if (TerminalReason != EShanmenItemRunTerminalReason::Extraction
		&& !Summary.RuntimeSnapshot.OrderedSecuredItems.IsEmpty())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::SettlementInvalid,
			TEXT("Destructive Runtime settlement must not report secured items."));
	}
	AcquiredItems.Sort([](
		const FShanmenItemRunAcquiredItem& Left,
		const FShanmenItemRunAcquiredItem& Right)
	{
		return GuidDigits(Left.ItemInstanceId)
			< GuidDigits(Right.ItemInstanceId);
	});
	TArray<FShanmenItemRunSecuredOriginal> Originals;
	if (TerminalReason == EShanmenItemRunTerminalReason::Extraction)
	{
		for (const Fdemo_mapShanmenPreparedLoadoutLine& Line :
			Prepared.Receipt.OrderedLines)
		{
			const int32 Remaining = SecuredById.FindRef(Line.ItemInstanceId);
			if (Remaining <= 0)
			{
				continue;
			}
			FShanmenItemRunSecuredOriginal& Original =
				Originals.AddDefaulted_GetRef();
			Original.ItemInstanceId = Line.ItemInstanceId;
			Original.RemainingQuantity = Remaining;
		}
	}

	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady,
			TEXT("Authority disappeared before terminal reconciliation."));
	}
	FShanmenItemRunFinalizeRequest Request;
	Request.Context = MakeContext(
		Snapshot, Prepared.Receipt,
		MakeFinalizeRequestId(
			Prepared.Receipt, Summary.RunId, TerminalReason,
			Originals, AcquiredItems));
	Request.ActiveRunId = Summary.RunId;
	Request.TerminalReason = TerminalReason;
	Request.SecuredOriginals = Originals;
	Request.AcquiredItems = AcquiredItems;
	Result.ActiveRunId = Summary.RunId;
	Result.FinalizeCommand = Authority.FinalizePreparedRunDurable(Request);
	if (!Result.FinalizeCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::FinalizeRejected,
			Result.FinalizeCommand.Diagnostic);
	}
	Result.Status = Result.FinalizeCommand.Status
		== EShanmenItemDurableCommandStatus::Replayed
		? Edemo_mapShanmenRunLifecycleStatus::NoChange
		: Edemo_mapShanmenRunLifecycleStatus::Finalized;
	Result.Diagnostic =
		TerminalReason == EShanmenItemRunTerminalReason::Extraction
		? TEXT("Runtime extraction restored prepared survivors, imported canonical loot with reward metadata, and published one durable terminal marker.")
		: TEXT("Destructive Runtime outcome converted every prepared identity to an audit tombstone and published one durable terminal marker.");
	return Result;
}
