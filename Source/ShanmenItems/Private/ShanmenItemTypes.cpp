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
		&& AuthorityRevision == Other.AuthorityRevision;
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
