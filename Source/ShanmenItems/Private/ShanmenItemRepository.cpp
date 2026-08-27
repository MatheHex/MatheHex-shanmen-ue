#include "ShanmenItemRepository.h"

#include "ShanmenDeterministicId.h"

namespace
{
	void SetError(EShanmenItemTransactionError* OutError, EShanmenItemTransactionError Error)
	{
		if (OutError)
		{
			*OutError = Error;
		}
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString EnumNumber(uint8 Value)
	{
		return FString::FromInt(static_cast<int32>(Value));
	}

	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	}
}

bool FShanmenItemRepository::TryLoadSnapshot(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	EShanmenItemTransactionError* OutError)
{
	FState Candidate;
	if (!TryBuildState(Snapshot, Candidate, OutError))
	{
		return false;
	}

	State = MoveTemp(Candidate);
	bInitialized = true;
	SetError(OutError, EShanmenItemTransactionError::None);
	return true;
}

bool FShanmenItemRepository::TryBuildState(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	FState& OutState,
	EShanmenItemTransactionError* OutError)
{
	if (Snapshot.AuthorityRevision < 0 || !Snapshot.Content.IsValid())
	{
		SetError(OutError, EShanmenItemTransactionError::InvalidSnapshot);
		return false;
	}

	FState Candidate;
	Candidate.AuthorityRevision = Snapshot.AuthorityRevision;
	Candidate.Content = Snapshot.Content;

	for (const FShanmenItemDefinition& Definition : Snapshot.Definitions)
	{
		if (!Definition.IsValid() || Candidate.Definitions.Contains(Definition.DefinitionId))
		{
			SetError(OutError, EShanmenItemTransactionError::InvalidSnapshot);
			return false;
		}
		Candidate.Definitions.Add(Definition.DefinitionId, Definition);
	}
	for (const FShanmenItemContainer& Container : Snapshot.Containers)
	{
		if (!Container.IsValid() || Candidate.Containers.Contains(Container.ContainerId))
		{
			SetError(OutError, EShanmenItemTransactionError::InvalidSnapshot);
			return false;
		}
		Candidate.Containers.Add(Container.ContainerId, Container);
	}
	for (const FShanmenItemInstance& Item : Snapshot.Items)
	{
		if (!Item.ItemInstanceId.IsValid() || Candidate.Items.Contains(Item.ItemInstanceId))
		{
			SetError(OutError, EShanmenItemTransactionError::InvalidSnapshot);
			return false;
		}
		Candidate.Items.Add(Item.ItemInstanceId, Item);
	}
	for (const FShanmenItemReservationSnapshot& Reservation : Snapshot.Reservations)
	{
		if (!Reservation.IsValid() || Candidate.Reservations.Contains(Reservation.ReservationId))
		{
			SetError(OutError, EShanmenItemTransactionError::InvalidSnapshot);
			return false;
		}
		Candidate.Reservations.Add(Reservation.ReservationId, Reservation);
	}
	for (const FShanmenItemProcessedRequestSnapshot& Processed : Snapshot.ProcessedRequests)
	{
		if (!Processed.IsValid() || Candidate.ProcessedRequests.Contains(Processed.RequestId))
		{
			SetError(OutError, EShanmenItemTransactionError::InvalidSnapshot);
			return false;
		}
		Candidate.ProcessedRequests.Add(Processed.RequestId, Processed);
	}

	if (!ValidateState(Candidate, OutError))
	{
		return false;
	}

	OutState = MoveTemp(Candidate);
	return true;
}

bool FShanmenItemRepository::ValidateState(
	const FState& Candidate,
	EShanmenItemTransactionError* OutError)
{
	auto Fail = [OutError]()
	{
		SetError(OutError, EShanmenItemTransactionError::InvariantViolation);
		return false;
	};

	if (Candidate.AuthorityRevision < 0 || !Candidate.Content.IsValid())
	{
		return Fail();
	}

	for (const TPair<FName, FShanmenItemDefinition>& Pair : Candidate.Definitions)
	{
		if (Pair.Key != Pair.Value.DefinitionId || !Pair.Value.IsValid())
		{
			return Fail();
		}
	}

	TSet<FGuid> PlacedItemIds;
	for (const TPair<FGuid, FShanmenItemContainer>& Pair : Candidate.Containers)
	{
		const FShanmenItemContainer& Container = Pair.Value;
		if (Pair.Key != Container.ContainerId || !Container.IsValid())
		{
			return Fail();
		}

		for (int32 SlotIndex = 0; SlotIndex < Container.Slots.Num(); ++SlotIndex)
		{
			const FGuid& ItemId = Container.Slots[SlotIndex];
			if (!ItemId.IsValid())
			{
				continue;
			}
			if (PlacedItemIds.Contains(ItemId))
			{
				return Fail();
			}
			const FShanmenItemInstance* Item = Candidate.Items.Find(ItemId);
			if (!Item
				|| Item->State == EShanmenItemInstanceState::Depleted
				|| Item->ParentContainerId != Container.ContainerId
				|| Item->SlotIndex != SlotIndex
				|| Item->RunId != Container.RunId
				|| Item->OwnerId != Container.OwnerId)
			{
				return Fail();
			}
			PlacedItemIds.Add(ItemId);
		}
	}

	TSet<FGuid> OwnedChildContainerIds;
	for (const TPair<FGuid, FShanmenItemInstance>& Pair : Candidate.Items)
	{
		const FShanmenItemInstance& Item = Pair.Value;
		const FShanmenItemDefinition* Definition = Candidate.Definitions.Find(Item.DefinitionId);
		if (Pair.Key != Item.ItemInstanceId
			|| !Item.ItemInstanceId.IsValid()
			|| !Item.RunId.IsValid()
			|| !Item.OwnerId.IsValid()
			|| !Definition
			|| Item.Revision < 0
			|| Item.Durability < 0
			|| Item.Durability > Definition->MaxDurability
			|| Item.Charges < 0
			|| Item.Charges > Definition->MaxCharges)
		{
			return Fail();
		}

		if (Item.ChildContainerId.IsValid())
		{
			const FShanmenItemContainer* ChildContainer =
				Candidate.Containers.Find(Item.ChildContainerId);
			if (!ChildContainer
				|| Item.ChildContainerId == Item.ParentContainerId
				|| ChildContainer->RunId != Item.RunId
				|| ChildContainer->OwnerId != Item.OwnerId
				|| OwnedChildContainerIds.Contains(Item.ChildContainerId))
			{
				return Fail();
			}
			OwnedChildContainerIds.Add(Item.ChildContainerId);
		}

		if (Item.State == EShanmenItemInstanceState::Depleted)
		{
			if (Item.Quantity != 0
				|| Item.ParentContainerId.IsValid()
				|| Item.ChildContainerId.IsValid()
				|| Item.SlotIndex != INDEX_NONE
				|| Item.DeploymentReservationId.IsValid()
				|| !Definition->Supports(EShanmenItemResourceKind::Quantity))
			{
				return Fail();
			}
		}
		else
		{
			if (Item.Quantity < 1
				|| Item.Quantity > Definition->MaxStack
				|| !Item.ParentContainerId.IsValid()
				|| Item.SlotIndex < 0
				|| !PlacedItemIds.Contains(Item.ItemInstanceId))
			{
				return Fail();
			}
		}

		if (Item.State == EShanmenItemInstanceState::Stored && Item.DeploymentReservationId.IsValid())
		{
			return Fail();
		}
		if (Item.State == EShanmenItemInstanceState::Deployed)
		{
			const FShanmenItemReservationSnapshot* Deployment = Candidate.Reservations.Find(Item.DeploymentReservationId);
			if (!Definition->Supports(EShanmenItemResourceKind::DeploymentLock)
				|| !Item.DeploymentReservationId.IsValid()
				|| !Deployment
				|| Deployment->ItemInstanceId != Item.ItemInstanceId
				|| Deployment->ResourceKind != EShanmenItemResourceKind::DeploymentLock
				|| Deployment->State != EShanmenItemReservationState::Committed)
			{
				return Fail();
			}
		}
	}

	// P1.1 intentionally preserves Code B's one-level item-owned storage. A
	// nested owner would make the migration graph ambiguous and can introduce a
	// container cycle, so deeper nesting fails before the authority is replaced.
	for (const TPair<FGuid, FShanmenItemInstance>& Pair : Candidate.Items)
	{
		const FShanmenItemInstance& Owner = Pair.Value;
		if (!Owner.ChildContainerId.IsValid())
		{
			continue;
		}
		const FShanmenItemContainer* Child =
			Candidate.Containers.Find(Owner.ChildContainerId);
		if (!Child)
		{
			return Fail();
		}
		for (const FGuid& ChildItemId : Child->Slots)
		{
			const FShanmenItemInstance* ChildItem =
				ChildItemId.IsValid() ? Candidate.Items.Find(ChildItemId) : nullptr;
			if (ChildItem && ChildItem->ChildContainerId.IsValid())
			{
				return Fail();
			}
		}
	}

	TSet<FGuid> ReserveRequestIds;
	for (const TPair<FGuid, FShanmenItemReservationSnapshot>& Pair : Candidate.Reservations)
	{
		const FShanmenItemReservationSnapshot& Reservation = Pair.Value;
		const FShanmenItemInstance* Item = Candidate.Items.Find(Reservation.ItemInstanceId);
		if (Pair.Key != Reservation.ReservationId
			|| !Reservation.IsValid()
			|| ReserveRequestIds.Contains(Reservation.ReserveRequestId)
			|| !Item
			|| Item->RunId != Reservation.RunId
			|| Item->OwnerId != Reservation.OwnerId
			|| Reservation.ItemRevisionAtReserve > Item->Revision)
		{
			return Fail();
		}

		const FShanmenItemDefinition* Definition = Candidate.Definitions.Find(Item->DefinitionId);
		if (!Definition || !Definition->Supports(Reservation.ResourceKind))
		{
			return Fail();
		}

		const FShanmenItemProcessedRequestSnapshot* ReserveCommand = Candidate.ProcessedRequests.Find(Reservation.ReserveRequestId);
		if (!ReserveCommand
			|| !ReserveCommand->Receipt.IsSuccess()
			|| ReserveCommand->Receipt.Operation != EShanmenItemTransactionOperation::Reserve
			|| ReserveCommand->Receipt.ReservationId != Reservation.ReservationId)
		{
			return Fail();
		}

		if (Reservation.State == EShanmenItemReservationState::Reserved
			&& (Item->State == EShanmenItemInstanceState::Depleted
				|| ((Reservation.ResourceKind == EShanmenItemResourceKind::Quantity
						|| Reservation.ResourceKind == EShanmenItemResourceKind::DeploymentLock)
					&& Item->State != EShanmenItemInstanceState::Stored)))
		{
			return Fail();
		}
		ReserveRequestIds.Add(Reservation.ReserveRequestId);
	}

	for (const TPair<FGuid, FShanmenItemInstance>& ItemPair : Candidate.Items)
	{
		const FGuid& ItemId = ItemPair.Key;
		bool bHasQuantityReservation = false;
		bool bHasOtherReservation = false;
		for (const TPair<FGuid, FShanmenItemReservationSnapshot>& ReservationPair : Candidate.Reservations)
		{
			const FShanmenItemReservationSnapshot& Reservation = ReservationPair.Value;
			if (Reservation.ItemInstanceId != ItemId || Reservation.State != EShanmenItemReservationState::Reserved)
			{
				continue;
			}
			if (Reservation.ResourceKind == EShanmenItemResourceKind::Quantity)
			{
				bHasQuantityReservation = true;
			}
			else
			{
				bHasOtherReservation = true;
			}
		}
		if (bHasQuantityReservation && bHasOtherReservation)
		{
			return Fail();
		}

		for (uint8 KindValue = 0; KindValue <= static_cast<uint8>(EShanmenItemResourceKind::Charges); ++KindValue)
		{
			const EShanmenItemResourceKind Kind = static_cast<EShanmenItemResourceKind>(KindValue);
			if (GetReservedAmount(Candidate, ItemId, Kind) > GetResourceTotal(ItemPair.Value, Kind))
			{
				return Fail();
			}
		}
	}

	TSet<FGuid> CommittedReservationIds;
	TSet<FGuid> CancelledReservationIds;
	TSet<FGuid> ReleasedReservationIds;
	for (const TPair<FGuid, FShanmenItemProcessedRequestSnapshot>& Pair : Candidate.ProcessedRequests)
	{
		const FShanmenItemProcessedRequestSnapshot& Processed = Pair.Value;
		if (Pair.Key != Processed.RequestId
			|| !Processed.IsValid()
			|| Processed.Receipt.AuthorityRevision > Candidate.AuthorityRevision)
		{
			return Fail();
		}
		if (Processed.Receipt.IsSuccess())
		{
			const FShanmenItemReservationSnapshot* Reservation = Candidate.Reservations.Find(Processed.Receipt.ReservationId);
			if (!Reservation
				|| Reservation->ItemInstanceId != Processed.Receipt.ItemInstanceId
				|| Reservation->ResourceKind != Processed.Receipt.ResourceKind
				|| Reservation->Amount != Processed.Receipt.Amount)
			{
				return Fail();
			}
			switch (Processed.Receipt.Operation)
			{
			case EShanmenItemTransactionOperation::Reserve:
				if (Processed.Receipt.Phase != EShanmenItemTransactionPhase::Reserved) return Fail();
				break;
			case EShanmenItemTransactionOperation::Commit:
				if (Processed.Receipt.Phase != EShanmenItemTransactionPhase::Committed) return Fail();
				CommittedReservationIds.Add(Reservation->ReservationId);
				break;
			case EShanmenItemTransactionOperation::Cancel:
				if (Processed.Receipt.Phase != EShanmenItemTransactionPhase::Cancelled) return Fail();
				CancelledReservationIds.Add(Reservation->ReservationId);
				break;
			case EShanmenItemTransactionOperation::ReleaseDeployment:
				if (Processed.Receipt.Phase != EShanmenItemTransactionPhase::Released) return Fail();
				ReleasedReservationIds.Add(Reservation->ReservationId);
				break;
			default:
				return Fail();
			}
		}
	}
	for (const TPair<FGuid, FShanmenItemReservationSnapshot>& Pair : Candidate.Reservations)
	{
		switch (Pair.Value.State)
		{
		case EShanmenItemReservationState::Reserved:
			break;
		case EShanmenItemReservationState::Committed:
			if (!CommittedReservationIds.Contains(Pair.Key)) return Fail();
			break;
		case EShanmenItemReservationState::Cancelled:
			if (!CancelledReservationIds.Contains(Pair.Key)) return Fail();
			break;
		case EShanmenItemReservationState::Released:
			if (!CommittedReservationIds.Contains(Pair.Key) || !ReleasedReservationIds.Contains(Pair.Key)) return Fail();
			break;
		default:
			return Fail();
		}
	}

	SetError(OutError, EShanmenItemTransactionError::None);
	return true;
}

FShanmenItemAuthoritySnapshot FShanmenItemRepository::CaptureSnapshot() const
{
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!bInitialized)
	{
		return Snapshot;
	}

	Snapshot.AuthorityRevision = State.AuthorityRevision;
	Snapshot.Content = State.Content;
	State.Definitions.GenerateValueArray(Snapshot.Definitions);
	State.Containers.GenerateValueArray(Snapshot.Containers);
	State.Items.GenerateValueArray(Snapshot.Items);
	State.Reservations.GenerateValueArray(Snapshot.Reservations);
	State.ProcessedRequests.GenerateValueArray(Snapshot.ProcessedRequests);

	Snapshot.Definitions.Sort([](const FShanmenItemDefinition& Left, const FShanmenItemDefinition& Right)
	{
		return Left.DefinitionId.LexicalLess(Right.DefinitionId);
	});
	Snapshot.Containers.Sort([](const FShanmenItemContainer& Left, const FShanmenItemContainer& Right)
	{
		return GuidLess(Left.ContainerId, Right.ContainerId);
	});
	Snapshot.Items.Sort([](const FShanmenItemInstance& Left, const FShanmenItemInstance& Right)
	{
		return GuidLess(Left.ItemInstanceId, Right.ItemInstanceId);
	});
	Snapshot.Reservations.Sort([](const FShanmenItemReservationSnapshot& Left, const FShanmenItemReservationSnapshot& Right)
	{
		return GuidLess(Left.ReservationId, Right.ReservationId);
	});
	Snapshot.ProcessedRequests.Sort([](const FShanmenItemProcessedRequestSnapshot& Left, const FShanmenItemProcessedRequestSnapshot& Right)
	{
		return GuidLess(Left.RequestId, Right.RequestId);
	});
	return Snapshot;
}

bool FShanmenItemRepository::ValidateInvariants(EShanmenItemTransactionError* OutError) const
{
	if (!bInitialized)
	{
		SetError(OutError, EShanmenItemTransactionError::NotInitialized);
		return false;
	}
	return ValidateState(State, OutError);
}

const FShanmenItemDefinition* FShanmenItemRepository::FindDefinition(FName DefinitionId) const
{
	return bInitialized ? State.Definitions.Find(DefinitionId) : nullptr;
}

const FShanmenItemContainer* FShanmenItemRepository::FindContainer(const FGuid& ContainerId) const
{
	return bInitialized ? State.Containers.Find(ContainerId) : nullptr;
}

const FShanmenItemInstance* FShanmenItemRepository::FindItem(const FGuid& ItemInstanceId) const
{
	return bInitialized ? State.Items.Find(ItemInstanceId) : nullptr;
}

const FShanmenItemReservationSnapshot* FShanmenItemRepository::FindReservation(const FGuid& ReservationId) const
{
	return bInitialized ? State.Reservations.Find(ReservationId) : nullptr;
}

int32 FShanmenItemRepository::GetAvailableResource(
	const FGuid& ItemInstanceId,
	EShanmenItemResourceKind Kind) const
{
	return bInitialized ? GetAvailableResource(State, ItemInstanceId, Kind) : 0;
}

int32 FShanmenItemRepository::NumActiveReservations() const
{
	if (!bInitialized)
	{
		return 0;
	}
	int32 Count = 0;
	for (const TPair<FGuid, FShanmenItemReservationSnapshot>& Pair : State.Reservations)
	{
		Count += Pair.Value.State == EShanmenItemReservationState::Reserved ? 1 : 0;
	}
	return Count;
}

FGuid FShanmenItemRepository::Fingerprint(const FShanmenItemReserveRequest& Request)
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.Items.Command.Reserve.r1"),
		{
			GuidDigits(Request.Context.RunId),
			GuidDigits(Request.Context.OwnerId),
			GuidDigits(Request.Context.RequestId),
			Request.Context.Content.Version.ToString(),
			Request.Context.Content.Digest,
			GuidDigits(Request.ItemInstanceId),
			EnumNumber(static_cast<uint8>(Request.ResourceKind)),
			FString::FromInt(Request.Amount),
			FString::FromInt(Request.ExpectedItemRevision),
			Request.PurposeId.ToString()
		});
}

FGuid FShanmenItemRepository::Fingerprint(
	EShanmenItemTransactionOperation Operation,
	const FShanmenItemReservationActionRequest& Request)
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.Items.Command.Resolution.r1"),
		{
			EnumNumber(static_cast<uint8>(Operation)),
			GuidDigits(Request.Context.RunId),
			GuidDigits(Request.Context.OwnerId),
			GuidDigits(Request.Context.RequestId),
			Request.Context.Content.Version.ToString(),
			Request.Context.Content.Digest,
			GuidDigits(Request.ReservationId)
		});
}

FGuid FShanmenItemRepository::MakeReservationId(const FShanmenItemReserveRequest& Request)
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.Items.Reservation.r1"),
		{
			GuidDigits(Request.Context.RunId),
			GuidDigits(Request.Context.OwnerId),
			GuidDigits(Request.Context.RequestId),
			GuidDigits(Request.ItemInstanceId),
			EnumNumber(static_cast<uint8>(Request.ResourceKind)),
			FString::FromInt(Request.Amount),
			Request.PurposeId.ToString()
		});
}

FGuid FShanmenItemRepository::MakeReceiptId(
	const FGuid& RequestId,
	const FGuid& RequestFingerprint,
	EShanmenItemTransactionPhase Phase,
	EShanmenItemTransactionError Error)
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.Items.Receipt.r1"),
		{
			GuidDigits(RequestId),
			GuidDigits(RequestFingerprint),
			EnumNumber(static_cast<uint8>(Phase)),
			EnumNumber(static_cast<uint8>(Error))
		});
}

bool FShanmenItemRepository::TryReplay(
	const FGuid& RequestId,
	const FGuid& RequestFingerprint,
	EShanmenItemTransactionOperation Operation,
	FShanmenItemTransactionReceipt& OutReceipt) const
{
	const FShanmenItemProcessedRequestSnapshot* Existing = State.ProcessedRequests.Find(RequestId);
	if (!Existing)
	{
		return false;
	}
	if (Existing->Fingerprint == RequestFingerprint && Existing->Receipt.Operation == Operation)
	{
		OutReceipt = Existing->Receipt;
	}
	else
	{
		OutReceipt = MakeRejected(
			Operation,
			RequestId,
			RequestFingerprint,
			EShanmenItemTransactionError::RequestIdConflict);
	}
	return true;
}

void FShanmenItemRepository::RecordProcessed(
	FState& Candidate,
	const FGuid& RequestId,
	const FGuid& RequestFingerprint,
	const FShanmenItemTransactionReceipt& Receipt) const
{
	if (!RequestId.IsValid() || !RequestFingerprint.IsValid() || !Receipt.IsValid())
	{
		return;
	}
	FShanmenItemProcessedRequestSnapshot Processed;
	Processed.RequestId = RequestId;
	Processed.Fingerprint = RequestFingerprint;
	Processed.Receipt = Receipt;
	Candidate.ProcessedRequests.Add(RequestId, Processed);
}

FShanmenItemTransactionReceipt FShanmenItemRepository::MakeRejected(
	EShanmenItemTransactionOperation Operation,
	const FGuid& RequestId,
	const FGuid& RequestFingerprint,
	EShanmenItemTransactionError Error) const
{
	FShanmenItemTransactionReceipt Receipt;
	Receipt.Operation = Operation;
	Receipt.Phase = EShanmenItemTransactionPhase::Rejected;
	Receipt.Error = Error;
	Receipt.RequestId = RequestId;
	Receipt.AuthorityRevision = bInitialized ? State.AuthorityRevision : 0;
	if (RequestId.IsValid() && RequestFingerprint.IsValid())
	{
		Receipt.ReceiptId = MakeReceiptId(RequestId, RequestFingerprint, Receipt.Phase, Error);
	}
	return Receipt;
}

FShanmenItemTransactionReceipt FShanmenItemRepository::Reserve(const FShanmenItemReserveRequest& Request)
{
	const FGuid RequestFingerprint = Fingerprint(Request);
	FShanmenItemTransactionReceipt Replayed;
	if (bInitialized && Request.Context.RequestId.IsValid()
		&& TryReplay(Request.Context.RequestId, RequestFingerprint, EShanmenItemTransactionOperation::Reserve, Replayed))
	{
		return Replayed;
	}

	auto Reject = [this, &Request, &RequestFingerprint](EShanmenItemTransactionError Error)
	{
		FShanmenItemTransactionReceipt Receipt = MakeRejected(
			EShanmenItemTransactionOperation::Reserve,
			Request.Context.RequestId,
			RequestFingerprint,
			Error);
		if (bInitialized && Error != EShanmenItemTransactionError::RequestIdConflict && Receipt.IsValid())
		{
			FState Candidate = State;
			++Candidate.AuthorityRevision;
			Receipt.AuthorityRevision = Candidate.AuthorityRevision;
			RecordProcessed(Candidate, Request.Context.RequestId, RequestFingerprint, Receipt);
			EShanmenItemTransactionError Ignored;
			if (ValidateState(Candidate, &Ignored))
			{
				State = MoveTemp(Candidate);
			}
		}
		return Receipt;
	};

	if (!bInitialized)
	{
		return Reject(EShanmenItemTransactionError::NotInitialized);
	}
	if (!Request.IsValid())
	{
		return Reject(EShanmenItemTransactionError::InvalidRequest);
	}
	if (!IsSameContent(Request.Context.Content, State.Content))
	{
		return Reject(EShanmenItemTransactionError::ContentMismatch);
	}

	const FShanmenItemInstance* Item = State.Items.Find(Request.ItemInstanceId);
	if (!Item)
	{
		return Reject(EShanmenItemTransactionError::ItemNotFound);
	}
	if (Item->RunId != Request.Context.RunId || Item->OwnerId != Request.Context.OwnerId)
	{
		return Reject(EShanmenItemTransactionError::ScopeMismatch);
	}
	if (Item->Revision != Request.ExpectedItemRevision)
	{
		return Reject(EShanmenItemTransactionError::StaleItemRevision);
	}
	if (Item->State == EShanmenItemInstanceState::Depleted
		|| ((Request.ResourceKind == EShanmenItemResourceKind::Quantity
				|| Request.ResourceKind == EShanmenItemResourceKind::DeploymentLock)
			&& Item->State != EShanmenItemInstanceState::Stored))
	{
		return Reject(EShanmenItemTransactionError::DeploymentMismatch);
	}

	const FShanmenItemDefinition* Definition = State.Definitions.Find(Item->DefinitionId);
	if (!Definition || !Definition->Supports(Request.ResourceKind))
	{
		return Reject(EShanmenItemTransactionError::ResourceUnsupported);
	}

	bool bHasActiveQuantity = false;
	bool bHasActiveOther = false;
	bool bHasActiveDeployment = false;
	bool bHasAnyActive = false;
	for (const TPair<FGuid, FShanmenItemReservationSnapshot>& Pair : State.Reservations)
	{
		const FShanmenItemReservationSnapshot& Existing = Pair.Value;
		if (Existing.ItemInstanceId != Request.ItemInstanceId || Existing.State != EShanmenItemReservationState::Reserved)
		{
			continue;
		}
		bHasAnyActive = true;
		bHasActiveQuantity |= Existing.ResourceKind == EShanmenItemResourceKind::Quantity;
		bHasActiveOther |= Existing.ResourceKind != EShanmenItemResourceKind::Quantity;
		bHasActiveDeployment |= Existing.ResourceKind == EShanmenItemResourceKind::DeploymentLock;
	}
	if ((Request.ResourceKind == EShanmenItemResourceKind::DeploymentLock && bHasAnyActive)
		|| (Request.ResourceKind != EShanmenItemResourceKind::DeploymentLock && bHasActiveDeployment)
		|| (Request.ResourceKind == EShanmenItemResourceKind::Quantity && bHasActiveOther)
		|| (Request.ResourceKind != EShanmenItemResourceKind::Quantity
			&& Request.ResourceKind != EShanmenItemResourceKind::DeploymentLock
			&& bHasActiveQuantity))
	{
		return Reject(EShanmenItemTransactionError::InsufficientResource);
	}

	const int32 AvailableBefore = GetAvailableResource(State, Item->ItemInstanceId, Request.ResourceKind);
	if (AvailableBefore < Request.Amount)
	{
		return Reject(EShanmenItemTransactionError::InsufficientResource);
	}

	FState Candidate = State;
	FShanmenItemReservationSnapshot Reservation;
	Reservation.ReservationId = MakeReservationId(Request);
	Reservation.ReserveRequestId = Request.Context.RequestId;
	Reservation.RunId = Request.Context.RunId;
	Reservation.OwnerId = Request.Context.OwnerId;
	Reservation.ItemInstanceId = Request.ItemInstanceId;
	Reservation.ResourceKind = Request.ResourceKind;
	Reservation.Amount = Request.Amount;
	Reservation.ItemRevisionAtReserve = Request.ExpectedItemRevision;
	Reservation.PurposeId = Request.PurposeId;
	Reservation.State = EShanmenItemReservationState::Reserved;
	if (!Reservation.IsValid() || Candidate.Reservations.Contains(Reservation.ReservationId))
	{
		return Reject(EShanmenItemTransactionError::InvariantViolation);
	}
	Candidate.Reservations.Add(Reservation.ReservationId, Reservation);
	++Candidate.AuthorityRevision;

	FShanmenItemTransactionReceipt Receipt;
	Receipt.bSuccess = true;
	Receipt.Operation = EShanmenItemTransactionOperation::Reserve;
	Receipt.Phase = EShanmenItemTransactionPhase::Reserved;
	Receipt.Error = EShanmenItemTransactionError::None;
	Receipt.RequestId = Request.Context.RequestId;
	Receipt.ReservationId = Reservation.ReservationId;
	Receipt.ItemInstanceId = Request.ItemInstanceId;
	Receipt.ResourceKind = Request.ResourceKind;
	Receipt.Amount = Request.Amount;
	Receipt.ResourceBefore = GetResourceTotal(*Item, Request.ResourceKind);
	Receipt.ResourceAfter = Receipt.ResourceBefore;
	Receipt.AvailableAfter = GetAvailableResource(Candidate, Request.ItemInstanceId, Request.ResourceKind);
	Receipt.ItemRevision = Item->Revision;
	Receipt.AuthorityRevision = Candidate.AuthorityRevision;
	Receipt.ReceiptId = MakeReceiptId(Request.Context.RequestId, RequestFingerprint, Receipt.Phase, Receipt.Error);
	RecordProcessed(Candidate, Request.Context.RequestId, RequestFingerprint, Receipt);

	EShanmenItemTransactionError ValidationError;
	if (!Receipt.IsValid() || !ValidateState(Candidate, &ValidationError))
	{
		return Reject(EShanmenItemTransactionError::InvariantViolation);
	}
	State = MoveTemp(Candidate);
	return Receipt;
}

FShanmenItemTransactionReceipt FShanmenItemRepository::Commit(
	const FShanmenItemReservationActionRequest& Request)
{
	return ResolveReservation(EShanmenItemTransactionOperation::Commit, Request);
}

FShanmenItemTransactionReceipt FShanmenItemRepository::Cancel(
	const FShanmenItemReservationActionRequest& Request)
{
	return ResolveReservation(EShanmenItemTransactionOperation::Cancel, Request);
}

FShanmenItemTransactionReceipt FShanmenItemRepository::ReleaseDeployment(
	const FShanmenItemReservationActionRequest& Request)
{
	return ResolveReservation(EShanmenItemTransactionOperation::ReleaseDeployment, Request);
}

FShanmenItemTransactionReceipt FShanmenItemRepository::ResolveReservation(
	EShanmenItemTransactionOperation Operation,
	const FShanmenItemReservationActionRequest& Request)
{
	const FGuid RequestFingerprint = Fingerprint(Operation, Request);
	FShanmenItemTransactionReceipt Replayed;
	if (bInitialized && Request.Context.RequestId.IsValid()
		&& TryReplay(Request.Context.RequestId, RequestFingerprint, Operation, Replayed))
	{
		return Replayed;
	}

	auto Reject = [this, Operation, &Request, &RequestFingerprint](EShanmenItemTransactionError Error)
	{
		FShanmenItemTransactionReceipt Receipt = MakeRejected(
			Operation,
			Request.Context.RequestId,
			RequestFingerprint,
			Error);
		if (bInitialized && Error != EShanmenItemTransactionError::RequestIdConflict && Receipt.IsValid())
		{
			FState Candidate = State;
			++Candidate.AuthorityRevision;
			Receipt.AuthorityRevision = Candidate.AuthorityRevision;
			RecordProcessed(Candidate, Request.Context.RequestId, RequestFingerprint, Receipt);
			EShanmenItemTransactionError Ignored;
			if (ValidateState(Candidate, &Ignored))
			{
				State = MoveTemp(Candidate);
			}
		}
		return Receipt;
	};

	if (!bInitialized)
	{
		return Reject(EShanmenItemTransactionError::NotInitialized);
	}
	if (!Request.IsValid()
		|| (Operation != EShanmenItemTransactionOperation::Commit
			&& Operation != EShanmenItemTransactionOperation::Cancel
			&& Operation != EShanmenItemTransactionOperation::ReleaseDeployment))
	{
		return Reject(EShanmenItemTransactionError::InvalidRequest);
	}
	if (!IsSameContent(Request.Context.Content, State.Content))
	{
		return Reject(EShanmenItemTransactionError::ContentMismatch);
	}

	const FShanmenItemReservationSnapshot* ExistingReservation = State.Reservations.Find(Request.ReservationId);
	if (!ExistingReservation)
	{
		return Reject(EShanmenItemTransactionError::ReservationNotFound);
	}
	if (ExistingReservation->RunId != Request.Context.RunId || ExistingReservation->OwnerId != Request.Context.OwnerId)
	{
		return Reject(EShanmenItemTransactionError::ReservationScopeMismatch);
	}

	if (Operation == EShanmenItemTransactionOperation::Commit)
	{
		if (ExistingReservation->State == EShanmenItemReservationState::Cancelled)
		{
			return Reject(EShanmenItemTransactionError::ReservationAlreadyCancelled);
		}
		if (ExistingReservation->State == EShanmenItemReservationState::Committed
			|| ExistingReservation->State == EShanmenItemReservationState::Released)
		{
			return Reject(EShanmenItemTransactionError::ReservationAlreadyCommitted);
		}
	}
	else if (Operation == EShanmenItemTransactionOperation::Cancel)
	{
		if (ExistingReservation->State == EShanmenItemReservationState::Cancelled)
		{
			return Reject(EShanmenItemTransactionError::ReservationAlreadyCancelled);
		}
		if (ExistingReservation->State == EShanmenItemReservationState::Committed
			|| ExistingReservation->State == EShanmenItemReservationState::Released)
		{
			return Reject(EShanmenItemTransactionError::ReservationAlreadyCommitted);
		}
	}
	else if (ExistingReservation->State != EShanmenItemReservationState::Committed)
	{
		return Reject(EShanmenItemTransactionError::ReservationNotCommitted);
	}

	FState Candidate = State;
	FShanmenItemReservationSnapshot* Reservation = Candidate.Reservations.Find(Request.ReservationId);
	FShanmenItemInstance* Item = Reservation ? Candidate.Items.Find(Reservation->ItemInstanceId) : nullptr;
	if (!Reservation || !Item)
	{
		return Reject(EShanmenItemTransactionError::InvariantViolation);
	}

	const int32 ResourceBefore = GetResourceTotal(*Item, Reservation->ResourceKind);
	EShanmenItemTransactionPhase Phase = EShanmenItemTransactionPhase::Rejected;
	if (Operation == EShanmenItemTransactionOperation::Commit)
	{
		if (Reservation->ResourceKind == EShanmenItemResourceKind::DeploymentLock)
		{
			if (Item->State != EShanmenItemInstanceState::Stored || Item->DeploymentReservationId.IsValid())
			{
				return Reject(EShanmenItemTransactionError::DeploymentMismatch);
			}
			Item->State = EShanmenItemInstanceState::Deployed;
			Item->DeploymentReservationId = Reservation->ReservationId;
		}
		else
		{
			if (ResourceBefore < Reservation->Amount)
			{
				return Reject(EShanmenItemTransactionError::InvariantViolation);
			}
			switch (Reservation->ResourceKind)
			{
			case EShanmenItemResourceKind::Quantity:
				Item->Quantity -= Reservation->Amount;
				if (Item->Quantity == 0)
				{
					FShanmenItemContainer* Container = Candidate.Containers.Find(Item->ParentContainerId);
					if (!Container
						|| !Container->Slots.IsValidIndex(Item->SlotIndex)
						|| Container->Slots[Item->SlotIndex] != Item->ItemInstanceId)
					{
						return Reject(EShanmenItemTransactionError::InvariantViolation);
					}
					Container->Slots[Item->SlotIndex].Invalidate();
					Item->ParentContainerId.Invalidate();
					Item->SlotIndex = INDEX_NONE;
					Item->State = EShanmenItemInstanceState::Depleted;
				}
				break;
			case EShanmenItemResourceKind::Durability:
				Item->Durability -= Reservation->Amount;
				break;
			case EShanmenItemResourceKind::Charges:
				Item->Charges -= Reservation->Amount;
				break;
			default:
				return Reject(EShanmenItemTransactionError::ResourceUnsupported);
			}
		}
		++Item->Revision;
		Reservation->State = EShanmenItemReservationState::Committed;
		Phase = EShanmenItemTransactionPhase::Committed;
	}
	else if (Operation == EShanmenItemTransactionOperation::Cancel)
	{
		Reservation->State = EShanmenItemReservationState::Cancelled;
		Phase = EShanmenItemTransactionPhase::Cancelled;
	}
	else
	{
		if (Reservation->ResourceKind != EShanmenItemResourceKind::DeploymentLock
			|| Item->State != EShanmenItemInstanceState::Deployed
			|| Item->DeploymentReservationId != Reservation->ReservationId)
		{
			return Reject(EShanmenItemTransactionError::DeploymentMismatch);
		}
		Item->State = EShanmenItemInstanceState::Stored;
		Item->DeploymentReservationId.Invalidate();
		++Item->Revision;
		Reservation->State = EShanmenItemReservationState::Released;
		Phase = EShanmenItemTransactionPhase::Released;
	}
	++Candidate.AuthorityRevision;

	FShanmenItemTransactionReceipt Receipt;
	Receipt.bSuccess = true;
	Receipt.Operation = Operation;
	Receipt.Phase = Phase;
	Receipt.Error = EShanmenItemTransactionError::None;
	Receipt.RequestId = Request.Context.RequestId;
	Receipt.ReservationId = Reservation->ReservationId;
	Receipt.ItemInstanceId = Reservation->ItemInstanceId;
	Receipt.ResourceKind = Reservation->ResourceKind;
	Receipt.Amount = Reservation->Amount;
	Receipt.ResourceBefore = ResourceBefore;
	Receipt.ResourceAfter = GetResourceTotal(*Item, Reservation->ResourceKind);
	Receipt.AvailableAfter = GetAvailableResource(Candidate, Reservation->ItemInstanceId, Reservation->ResourceKind);
	Receipt.ItemRevision = Item->Revision;
	Receipt.AuthorityRevision = Candidate.AuthorityRevision;
	Receipt.ReceiptId = MakeReceiptId(Request.Context.RequestId, RequestFingerprint, Receipt.Phase, Receipt.Error);
	RecordProcessed(Candidate, Request.Context.RequestId, RequestFingerprint, Receipt);

	EShanmenItemTransactionError ValidationError;
	if (!Receipt.IsValid() || !ValidateState(Candidate, &ValidationError))
	{
		return Reject(EShanmenItemTransactionError::InvariantViolation);
	}
	State = MoveTemp(Candidate);
	return Receipt;
}

int32 FShanmenItemRepository::GetResourceTotal(
	const FShanmenItemInstance& Item,
	EShanmenItemResourceKind Kind)
{
	if (Item.State == EShanmenItemInstanceState::Depleted)
	{
		return 0;
	}
	switch (Kind)
	{
	case EShanmenItemResourceKind::Quantity:
		return Item.Quantity;
	case EShanmenItemResourceKind::DeploymentLock:
		return Item.State == EShanmenItemInstanceState::Stored ? 1 : 0;
	case EShanmenItemResourceKind::Durability:
		return Item.Durability;
	case EShanmenItemResourceKind::Charges:
		return Item.Charges;
	default:
		return 0;
	}
}

int32 FShanmenItemRepository::GetReservedAmount(
	const FState& Candidate,
	const FGuid& ItemInstanceId,
	EShanmenItemResourceKind Kind,
	const FGuid& ExcludedReservationId)
{
	int32 Reserved = 0;
	for (const TPair<FGuid, FShanmenItemReservationSnapshot>& Pair : Candidate.Reservations)
	{
		const FShanmenItemReservationSnapshot& Reservation = Pair.Value;
		if (Reservation.ReservationId != ExcludedReservationId
			&& Reservation.ItemInstanceId == ItemInstanceId
			&& Reservation.ResourceKind == Kind
			&& Reservation.State == EShanmenItemReservationState::Reserved)
		{
			Reserved += Reservation.Amount;
		}
	}
	return Reserved;
}

int32 FShanmenItemRepository::GetAvailableResource(
	const FState& Candidate,
	const FGuid& ItemInstanceId,
	EShanmenItemResourceKind Kind)
{
	const FShanmenItemInstance* Item = Candidate.Items.Find(ItemInstanceId);
	if (!Item)
	{
		return 0;
	}
	return FMath::Max(0, GetResourceTotal(*Item, Kind) - GetReservedAmount(Candidate, ItemInstanceId, Kind));
}

bool FShanmenItemRepository::IsSameContent(
	const FShanmenContentStamp& Left,
	const FShanmenContentStamp& Right)
{
	return Left.Version == Right.Version && Left.Digest == Right.Digest;
}
