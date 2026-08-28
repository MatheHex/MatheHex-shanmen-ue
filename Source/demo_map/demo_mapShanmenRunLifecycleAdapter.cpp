#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
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

	FGuid MakeClaimRequestId(
		const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenRun.Claim.r1"),
			{
				GuidDigits(Prepared.OwnerId), GuidDigits(Prepared.ScopeId),
				GuidDigits(Prepared.BatchRequestId),
				GuidDigits(Prepared.BatchReceiptId)
			});
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
		if (!Prepared.IsValid() || !Claim.IsSuccess()
			|| Claim.Operation
				!= EShanmenItemTransactionOperation::ClaimPreparedRun
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
		const TArray<FShanmenItemRunSecuredOriginal>& Originals)
	{
		TArray<FString> Parts =
		{
			GuidDigits(Prepared.OwnerId), GuidDigits(Prepared.ScopeId),
			GuidDigits(Prepared.BatchRequestId), GuidDigits(ActiveRunId),
			FString::FromInt(Originals.Num())
		};
		for (const FShanmenItemRunSecuredOriginal& Original : Originals)
		{
			Parts.Add(GuidDigits(Original.ItemInstanceId));
			Parts.Add(FString::FromInt(Original.RemainingQuantity));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenRun.FinalizeExtraction.r1"), Parts);
	}
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
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(Authority);
	if (!Prepared.IsCommitted())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::PreparedLoadoutRejected,
			Prepared.Diagnostic);
	}
	Result.PreparedLoadout = Prepared.Receipt;
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady,
			TEXT("Authority disappeared before prepared Run claim."));
	}
	FShanmenItemRunClaimRequest ClaimRequest;
	ClaimRequest.Context = MakeContext(
		Snapshot, Prepared.Receipt, MakeClaimRequestId(Prepared.Receipt));
	ClaimRequest.PreparedBatchRequestId = Prepared.Receipt.BatchRequestId;
	Result.ClaimCommand = Authority.ClaimPreparedRunDurable(ClaimRequest);
	if (!Result.ClaimCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::ClaimRejected,
			Result.ClaimCommand.Diagnostic);
	}
	Result.ActiveRunId = Result.ClaimCommand.Receipt.ReservationId;

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
		Prepared.Receipt, Result.ClaimCommand.Receipt,
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
	Result.Status = Result.ClaimCommand.Status
		== EShanmenItemDurableCommandStatus::Replayed
		? Edemo_mapShanmenRunLifecycleStatus::Resumed
		: Edemo_mapShanmenRunLifecycleStatus::Started;
	Result.Diagnostic = Result.Status
		== Edemo_mapShanmenRunLifecycleStatus::Started
		? TEXT("Prepared receipt was durably claimed and materialized into Runtime.")
		: TEXT("The same durable claim was replayed and rematerialized after Runtime restart.");
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
	if (Summary.Reason != Edemo_mapRunEndReason::Extraction
		|| Summary.RuntimeSnapshot.CommittedEndReason
			!= Edemo_mapRunEndReason::Extraction)
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::UnsupportedTerminalReason,
			TEXT("P1.9 finalizes extraction only; destructive equipment/child-container policy remains closed."));
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
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(Authority);
	if (!Prepared.IsCommitted())
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::PreparedLoadoutRejected,
			Prepared.Diagnostic);
	}
	TSet<FGuid> PreparedIds;
	for (const Fdemo_mapShanmenPreparedLoadoutLine& Line :
		Prepared.Receipt.OrderedLines)
	{
		PreparedIds.Add(Line.ItemInstanceId);
	}
	TMap<FGuid, int32> SecuredById;
	for (const Fdemo_mapRuntimeSettlementItem& Secured :
		Summary.RuntimeSnapshot.OrderedSecuredItems)
	{
		if (!PreparedIds.Contains(Secured.ItemInstanceId)
			|| Secured.StackCount <= 0
			|| SecuredById.Contains(Secured.ItemInstanceId))
		{
			return Reject(
				Edemo_mapShanmenRunLifecycleStatus::UnknownSecuredItem,
				TEXT("Settlement contains new loot, a duplicate, or an invalid stack; P1.9 refuses to invent its authority placement."));
		}
		SecuredById.Add(Secured.ItemInstanceId, Secured.StackCount);
	}
	TArray<FShanmenItemRunSecuredOriginal> Originals;
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

	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady,
			TEXT("Authority disappeared before terminal reconciliation."));
	}
	FShanmenItemRunFinalizeRequest Request;
	Request.Context = MakeContext(
		Snapshot, Prepared.Receipt,
		MakeFinalizeRequestId(Prepared.Receipt, Summary.RunId, Originals));
	Request.ActiveRunId = Summary.RunId;
	Request.TerminalReason = EShanmenItemRunTerminalReason::Extraction;
	Request.SecuredOriginals = Originals;
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
		TEXT("Runtime extraction reconciled every prepared original and published one durable terminal marker.");
	return Result;
}
