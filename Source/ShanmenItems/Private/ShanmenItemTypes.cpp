#include "ShanmenItemTypes.h"

#include "ShanmenItemTags.h"

namespace
{
	bool SameContent(const FShanmenContentStamp& Left, const FShanmenContentStamp& Right)
	{
		return Left.Version == Right.Version && Left.Digest == Right.Digest;
	}
}

bool FShanmenItemDefinition::IsValid() const
{
	const bool bQuantityIsExclusive = !Supports(EShanmenItemResourceKind::Quantity)
		|| (!Supports(EShanmenItemResourceKind::DeploymentLock)
			&& !Supports(EShanmenItemResourceKind::Durability)
			&& !Supports(EShanmenItemResourceKind::Charges));
	return !DefinitionId.IsNone()
		&& MaxStack >= 1
		&& MaxDurability >= 0
		&& MaxCharges >= 0
		&& bQuantityIsExclusive
		&& Supports(EShanmenItemResourceKind::Durability) == (MaxDurability > 0)
		&& Supports(EShanmenItemResourceKind::Charges) == (MaxCharges > 0);
}

bool FShanmenItemDefinition::Supports(EShanmenItemResourceKind Kind) const
{
	switch (Kind)
	{
	case EShanmenItemResourceKind::Quantity:
		return ItemTags.HasTagExact(FShanmenItemNativeTags::CapabilityConsumeQuantity());
	case EShanmenItemResourceKind::DeploymentLock:
		return ItemTags.HasTagExact(FShanmenItemNativeTags::CapabilityDeploy());
	case EShanmenItemResourceKind::Durability:
		return ItemTags.HasTagExact(FShanmenItemNativeTags::CapabilityDurability());
	case EShanmenItemResourceKind::Charges:
		return ItemTags.HasTagExact(FShanmenItemNativeTags::CapabilityCharges());
	default:
		return false;
	}
}

bool FShanmenItemDefinition::operator==(const FShanmenItemDefinition& Other) const
{
	return DefinitionId == Other.DefinitionId
		&& ItemTags == Other.ItemTags
		&& MaxStack == Other.MaxStack
		&& MaxDurability == Other.MaxDurability
		&& MaxCharges == Other.MaxCharges;
}

bool FShanmenItemContainer::IsValid() const
{
	return ContainerId.IsValid()
		&& RunId.IsValid()
		&& OwnerId.IsValid()
		&& !ContainerType.IsNone()
		&& !Slots.IsEmpty();
}

bool FShanmenItemContainer::operator==(const FShanmenItemContainer& Other) const
{
	return ContainerId == Other.ContainerId
		&& RunId == Other.RunId
		&& OwnerId == Other.OwnerId
		&& ContainerType == Other.ContainerType
		&& Slots == Other.Slots;
}

bool FShanmenItemInstance::operator==(const FShanmenItemInstance& Other) const
{
	return ItemInstanceId == Other.ItemInstanceId
		&& DefinitionId == Other.DefinitionId
		&& RunId == Other.RunId
		&& OwnerId == Other.OwnerId
		&& ParentContainerId == Other.ParentContainerId
		&& ChildContainerId == Other.ChildContainerId
		&& SlotIndex == Other.SlotIndex
		&& Quantity == Other.Quantity
		&& Durability == Other.Durability
		&& Charges == Other.Charges
		&& Revision == Other.Revision
		&& State == Other.State
		&& DeploymentReservationId == Other.DeploymentReservationId;
}

bool FShanmenItemReserveRequest::IsValid() const
{
	return Context.IsValid()
		&& ItemInstanceId.IsValid()
		&& Amount > 0
		&& ExpectedItemRevision >= 0
		&& !PurposeId.IsNone()
		&& (ResourceKind != EShanmenItemResourceKind::DeploymentLock || Amount == 1);
}

bool FShanmenItemReservationActionRequest::IsValid() const
{
	return Context.IsValid() && ReservationId.IsValid();
}

bool FShanmenItemReservationBatchRequest::IsValid() const
{
	if (!Context.IsValid() || ReservationIds.IsEmpty())
	{
		return false;
	}
	TSet<FGuid> Unique;
	for (const FGuid& ReservationId : ReservationIds)
	{
		if (!ReservationId.IsValid() || Unique.Contains(ReservationId))
		{
			return false;
		}
		Unique.Add(ReservationId);
	}
	return true;
}

bool FShanmenItemReservationAmendRequest::IsValid() const
{
	return Context.IsValid()
		&& ReservationId.IsValid()
		&& !ExpectedPurposeId.IsNone()
		&& !PurposeId.IsNone();
}

bool FShanmenItemRunClaimRequest::IsValid() const
{
	return Context.IsValid() && PreparedBatchRequestId.IsValid();
}

bool FShanmenItemRunSecuredOriginal::IsValid() const
{
	return ItemInstanceId.IsValid() && RemainingQuantity > 0;
}

bool FShanmenItemRunSecuredOriginal::operator==(
	const FShanmenItemRunSecuredOriginal& Other) const
{
	return ItemInstanceId == Other.ItemInstanceId
		&& RemainingQuantity == Other.RemainingQuantity;
}

bool FShanmenItemRunAcquiredItem::IsValid() const
{
	const bool bHasChild = !ChildContainerType.IsNone()
		|| ChildContainerCapacity != 0;
	return ItemInstanceId.IsValid()
		&& Definition.IsValid()
		&& Quantity > 0
		&& Quantity <= Definition.MaxStack
		&& ((!bHasChild
				&& ChildContainerType.IsNone()
				&& ChildContainerCapacity == 0)
			|| (!ChildContainerType.IsNone()
				&& ChildContainerCapacity > 0
				&& ChildContainerCapacity <= 4096));
}

bool FShanmenItemRunAcquiredItem::operator==(
	const FShanmenItemRunAcquiredItem& Other) const
{
	return ItemInstanceId == Other.ItemInstanceId
		&& Definition == Other.Definition
		&& Quantity == Other.Quantity
		&& ChildContainerType == Other.ChildContainerType
		&& ChildContainerCapacity == Other.ChildContainerCapacity;
}

bool FShanmenItemRunFinalizeRequest::IsValid() const
{
	if (!Context.IsValid() || !ActiveRunId.IsValid()
		|| TerminalReason == EShanmenItemRunTerminalReason::None)
	{
		return false;
	}
	TSet<FGuid> Unique;
	for (const FShanmenItemRunSecuredOriginal& Original : SecuredOriginals)
	{
		if (!Original.IsValid() || Unique.Contains(Original.ItemInstanceId))
		{
			return false;
		}
		Unique.Add(Original.ItemInstanceId);
	}
	for (const FShanmenItemRunAcquiredItem& Acquired : AcquiredItems)
	{
		if (!Acquired.IsValid()
			|| Unique.Contains(Acquired.ItemInstanceId))
		{
			return false;
		}
		Unique.Add(Acquired.ItemInstanceId);
	}
	if (TerminalReason != EShanmenItemRunTerminalReason::Extraction
		&& (!SecuredOriginals.IsEmpty() || !AcquiredItems.IsEmpty()))
	{
		return false;
	}
	return true;
}

FName FShanmenItemReservationPlacement::Encode(
	FName LogicalPurposeId,
	const FGuid& SourceContainerId,
	int32 SourceSlotIndex)
{
	if (LogicalPurposeId.IsNone() || !SourceContainerId.IsValid()
		|| SourceSlotIndex < 0)
	{
		return NAME_None;
	}
	return FName(*FString::Printf(
		TEXT("%s.PC%s.PS%08d"),
		*LogicalPurposeId.ToString(),
		*SourceContainerId.ToString(EGuidFormats::Digits),
		SourceSlotIndex));
}

bool FShanmenItemReservationPlacement::Decode(
	FName EncodedPurposeId,
	FName& OutLogicalPurposeId,
	FGuid& OutSourceContainerId,
	int32& OutSourceSlotIndex)
{
	OutLogicalPurposeId = NAME_None;
	OutSourceContainerId.Invalidate();
	OutSourceSlotIndex = INDEX_NONE;
	const FString Encoded = EncodedPurposeId.ToString();
	const int32 ContainerMarker = Encoded.Find(
		TEXT(".PC"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (ContainerMarker <= 0)
	{
		return false;
	}
	const int32 SlotMarker = Encoded.Find(
		TEXT(".PS"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (SlotMarker != ContainerMarker + 35
		|| SlotMarker + 11 != Encoded.Len())
	{
		return false;
	}
	FGuid ContainerId;
	const FString ContainerText = Encoded.Mid(ContainerMarker + 3, 32);
	const FString SlotText = Encoded.Mid(SlotMarker + 3, 8);
	if (!FGuid::ParseExact(
		ContainerText, EGuidFormats::Digits, ContainerId)
		|| !SlotText.IsNumeric())
	{
		return false;
	}
	const int32 SlotIndex = FCString::Atoi(*SlotText);
	if (SlotIndex < 0)
	{
		return false;
	}
	OutLogicalPurposeId = FName(*Encoded.Left(ContainerMarker));
	OutSourceContainerId = ContainerId;
	OutSourceSlotIndex = SlotIndex;
	return !OutLogicalPurposeId.IsNone();
}

FName FShanmenItemRunLifecyclePurpose::Active()
{
	return FName(TEXT("Shanmen.RunLifecycle.Active.r1"));
}

FName FShanmenItemRunLifecyclePurpose::Extraction()
{
	return FName(TEXT("Shanmen.RunLifecycle.Extraction.r1"));
}

FName FShanmenItemRunLifecyclePurpose::Death()
{
	return FName(TEXT("Shanmen.RunLifecycle.Death.r1"));
}

FName FShanmenItemRunLifecyclePurpose::Abandon()
{
	return FName(TEXT("Shanmen.RunLifecycle.Abandon.r1"));
}

FName FShanmenItemRunLifecyclePurpose::RecoveredStorage()
{
	return FName(TEXT("Shanmen.RunLifecycle.RecoveredStorage.r1"));
}

bool FShanmenItemTransactionReceipt::IsSuccess() const
{
	return bSuccess && Error == EShanmenItemTransactionError::None && Phase != EShanmenItemTransactionPhase::Rejected;
}

bool FShanmenItemTransactionReceipt::IsValid() const
{
	if (!RequestId.IsValid() || !ReceiptId.IsValid() || AuthorityRevision < 0)
	{
		return false;
	}
	if (!bSuccess)
	{
		return Phase == EShanmenItemTransactionPhase::Rejected
			&& Error != EShanmenItemTransactionError::None;
	}
	if (Operation == EShanmenItemTransactionOperation::CommitBatch)
	{
		if (Error != EShanmenItemTransactionError::None
			|| Phase != EShanmenItemTransactionPhase::Committed
			|| ReservationIds.IsEmpty())
		{
			return false;
		}
		TSet<FGuid> Unique;
		for (const FGuid& ReservationIdEntry : ReservationIds)
		{
			if (!ReservationIdEntry.IsValid()
				|| Unique.Contains(ReservationIdEntry))
			{
				return false;
			}
			Unique.Add(ReservationIdEntry);
		}
		return true;
	}
	if (Operation == EShanmenItemTransactionOperation::ClaimPreparedRun)
	{
		return Error == EShanmenItemTransactionError::None
			&& Phase == EShanmenItemTransactionPhase::Committed
			&& ReservationId.IsValid()
			&& ItemInstanceId.IsValid()
			&& Amount == ReservationIds.Num()
			&& Amount > 0
			&& PurposeId == FShanmenItemRunLifecyclePurpose::Active();
	}
	if (Operation == EShanmenItemTransactionOperation::FinalizePreparedRun)
	{
		const bool bTerminalPurpose =
			PurposeId == FShanmenItemRunLifecyclePurpose::Extraction()
			|| PurposeId == FShanmenItemRunLifecyclePurpose::Death()
			|| PurposeId == FShanmenItemRunLifecyclePurpose::Abandon();
		return Error == EShanmenItemTransactionError::None
			&& Phase == EShanmenItemTransactionPhase::Released
			&& ReservationId.IsValid()
			&& ItemInstanceId.IsValid()
			&& Amount == ReservationIds.Num()
			&& Amount > 0
			&& bTerminalPurpose;
	}
	if (Operation == EShanmenItemTransactionOperation::AmendReservationPurpose
		&& PurposeId.IsNone())
	{
		return false;
	}
	return Error == EShanmenItemTransactionError::None
		&& Phase != EShanmenItemTransactionPhase::Rejected
		&& ReservationId.IsValid()
		&& ItemInstanceId.IsValid()
		&& Amount > 0
		&& ResourceBefore >= 0
		&& ResourceAfter >= 0
		&& AvailableAfter >= 0
		&& ItemRevision >= 0;
}

bool FShanmenItemTransactionReceipt::operator==(const FShanmenItemTransactionReceipt& Other) const
{
	return bSuccess == Other.bSuccess
		&& Operation == Other.Operation
		&& Phase == Other.Phase
		&& Error == Other.Error
		&& ReceiptId == Other.ReceiptId
		&& RequestId == Other.RequestId
		&& ReservationId == Other.ReservationId
		&& ItemInstanceId == Other.ItemInstanceId
		&& ResourceKind == Other.ResourceKind
		&& Amount == Other.Amount
		&& ResourceBefore == Other.ResourceBefore
		&& ResourceAfter == Other.ResourceAfter
		&& AvailableAfter == Other.AvailableAfter
		&& ItemRevision == Other.ItemRevision
		&& AuthorityRevision == Other.AuthorityRevision
		&& PurposeId == Other.PurposeId
		&& ReservationIds == Other.ReservationIds;
}

bool FShanmenItemReservationSnapshot::IsValid() const
{
	return ReservationId.IsValid()
		&& ReserveRequestId.IsValid()
		&& RunId.IsValid()
		&& OwnerId.IsValid()
		&& ItemInstanceId.IsValid()
		&& Amount > 0
		&& ItemRevisionAtReserve >= 0
		&& !PurposeId.IsNone()
		&& (ResourceKind != EShanmenItemResourceKind::DeploymentLock || Amount == 1);
}

bool FShanmenItemReservationSnapshot::operator==(const FShanmenItemReservationSnapshot& Other) const
{
	return ReservationId == Other.ReservationId
		&& ReserveRequestId == Other.ReserveRequestId
		&& RunId == Other.RunId
		&& OwnerId == Other.OwnerId
		&& ItemInstanceId == Other.ItemInstanceId
		&& ResourceKind == Other.ResourceKind
		&& Amount == Other.Amount
		&& ItemRevisionAtReserve == Other.ItemRevisionAtReserve
		&& PurposeId == Other.PurposeId
		&& State == Other.State;
}

bool FShanmenItemProcessedRequestSnapshot::IsValid() const
{
	return RequestId.IsValid()
		&& Fingerprint.IsValid()
		&& Receipt.IsValid()
		&& Receipt.RequestId == RequestId;
}

bool FShanmenItemProcessedRequestSnapshot::operator==(const FShanmenItemProcessedRequestSnapshot& Other) const
{
	return RequestId == Other.RequestId
		&& Fingerprint == Other.Fingerprint
		&& Receipt == Other.Receipt;
}

bool FShanmenItemAuthoritySnapshot::operator==(const FShanmenItemAuthoritySnapshot& Other) const
{
	return AuthorityRevision == Other.AuthorityRevision
		&& SameContent(Content, Other.Content)
		&& Definitions == Other.Definitions
		&& Containers == Other.Containers
		&& Items == Other.Items
		&& Reservations == Other.Reservations
		&& ProcessedRequests == Other.ProcessedRequests;
}
