#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenItemTags.h"

FShanmenItemTransactionReceipt FShanmenItemRepository::EditGrid(const FShanmenItemGridRequest& R)
{
	using EError = EShanmenItemTransactionError;
	constexpr auto Operation = EShanmenItemTransactionOperation::EditGrid;
	const FGuid FingerprintId = FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Grid.Command.r1"),
		{R.Context.RunId.ToString(), R.Context.OwnerId.ToString(), R.Context.RequestId.ToString(),
		R.Context.Content.Version.ToString(), R.Context.Content.Digest, FString::FromInt(static_cast<int32>(R.Action)),
		R.ItemInstanceId.ToString(), R.DestinationContainerId.ToString(), R.MergeTargetId.ToString(),
		FString::FromInt(R.X), FString::FromInt(R.Y), R.bRotated ? TEXT("1") : TEXT("0"), FString::FromInt(R.Amount),
		FString::FromInt(R.ExpectedAuthorityRevision), FString::FromInt(R.ExpectedItemRevision), FString::FromInt(R.ExpectedTargetRevision)});
	FShanmenItemTransactionReceipt Replayed;
	if (bInitialized && TryReplay(R.Context.RequestId, FingerprintId, Operation, Replayed)) { return Replayed; }
	auto Reject = [&](EError Error) { return MakeRejected(Operation, R.Context.RequestId, FingerprintId, Error); };
	if (!bInitialized) { return Reject(EError::NotInitialized); }
	if (!R.Context.IsValid() || !R.ItemInstanceId.IsValid() || static_cast<uint8>(R.Action) > static_cast<uint8>(EShanmenItemGridAction::Equip)
		|| R.ExpectedAuthorityRevision < 0 || R.ExpectedItemRevision < 0 || R.ExpectedTargetRevision < 0) { return Reject(EError::InvalidRequest); }
	if (!IsSameContent(R.Context.Content, State.Content)) { return Reject(EError::ContentMismatch); }
	if (R.ExpectedAuthorityRevision != State.AuthorityRevision) { return Reject(EError::StaleAuthorityRevision); }
	if (State.AuthorityRevision == MAX_int32) { return Reject(EError::InvariantViolation); }
	const auto* Original = State.Items.Find(R.ItemInstanceId);
	if (!Original) { return Reject(EError::ItemNotFound); }
	if (Original->RunId != R.Context.RunId || Original->OwnerId != R.Context.OwnerId) { return Reject(EError::ScopeMismatch); }
	if (Original->Revision != R.ExpectedItemRevision) { return Reject(EError::StaleItemRevision); }
	auto Editable = [&](const FShanmenItemInstance& Item)
	{
		for (const auto& Pair : State.Reservations)
		{
			if (Pair.Value.ItemInstanceId == Item.ItemInstanceId && Pair.Value.State == EShanmenItemReservationState::Reserved) { return false; }
		}
		return Item.State == EShanmenItemInstanceState::Stored && Item.Revision < MAX_int32 && !Item.ChildContainerId.IsValid();
	};
	if (!Editable(*Original)) { return Reject(EError::GridPolicyViolation); }
	if (!State.Grid.Layouts.ContainsByPredicate([&](const auto& L) { return L.ContainerId == Original->ParentContainerId; }))
	{
		return Reject(EError::GridPolicyViolation); // No silent fallback to legacy single-slot writers.
	}
	FState Candidate = State;
	FGuid ResultItemId = R.ItemInstanceId;
	int32 Transferred = 1;
	const int32 BeforeQuantity = Original->Quantity;
	if (R.Action == EShanmenItemGridAction::Equip)
	{
		const auto* Destination = State.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == R.DestinationContainerId; });
		const auto* SourceLayout = State.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == Original->ParentContainerId; });
		if (!Destination || Destination->Kind != EShanmenItemGridKind::Equipment || !SourceLayout
			|| R.DestinationContainerId == Original->ParentContainerId || R.MergeTargetId.IsValid()
			|| R.Amount != 0 || R.X != 0 || R.Y != 0 || R.bRotated) { return Reject(EError::InvalidRequest); }
		const FGuid DisplacedId = State.Containers.FindChecked(R.DestinationContainerId).Slots[0];
		const auto* Displaced = State.Items.Find(DisplacedId);
		if (DisplacedId.IsValid() && (!Displaced || !Editable(*Displaced))) { return Reject(EError::GridPolicyViolation); }
		if (R.ExpectedTargetRevision != (Displaced ? Displaced->Revision : 0)) { return Reject(EError::StaleItemRevision); }
		auto Detach = [&](FGuid Id)
		{
			auto& I = Candidate.Items.FindChecked(Id);
			Candidate.Containers.FindChecked(I.ParentContainerId).Slots[I.SlotIndex].Invalidate();
			I.ParentContainerId.Invalidate(); I.SlotIndex = INDEX_NONE;
		};
		Detach(R.ItemInstanceId); if (Displaced) { Detach(DisplacedId); }
		FShanmenItemRepository Preview; Preview.State = Candidate; Preview.bInitialized = true;
		auto Placement = FShanmenItemGridPolicy::CanPlace(Preview.CaptureSnapshot(), R.ItemInstanceId, R.DestinationContainerId, 0, 0, false);
		if (Placement != EError::None) { return Reject(Placement); }
		if (Displaced)
		{
			Placement = FShanmenItemGridPolicy::CanPlace(Preview.CaptureSnapshot(), DisplacedId, Original->ParentContainerId,
				Original->SlotIndex % SourceLayout->Width, Original->SlotIndex / SourceLayout->Width,
				State.Grid.RotatedItems.Contains(R.ItemInstanceId));
			if (Placement != EError::None) { return Reject(Placement); }
			auto& OldEquipment = Candidate.Items.FindChecked(DisplacedId);
			OldEquipment.ParentContainerId = Original->ParentContainerId; OldEquipment.SlotIndex = Original->SlotIndex; ++OldEquipment.Revision;
			Candidate.Containers.FindChecked(Original->ParentContainerId).Slots[Original->SlotIndex] = DisplacedId;
			Candidate.Grid.RotatedItems.Remove(DisplacedId);
			if (State.Grid.RotatedItems.Contains(R.ItemInstanceId)) { Candidate.Grid.RotatedItems.Add(DisplacedId); }
		}
		auto& NewEquipment = Candidate.Items.FindChecked(R.ItemInstanceId);
		NewEquipment.ParentContainerId = R.DestinationContainerId; NewEquipment.SlotIndex = 0; ++NewEquipment.Revision;
		Candidate.Containers.FindChecked(R.DestinationContainerId).Slots[0] = R.ItemInstanceId;
		Candidate.Grid.RotatedItems.Remove(R.ItemInstanceId);
	}
	else if (R.Action == EShanmenItemGridAction::Merge)
	{
		if (R.ItemInstanceId == R.MergeTargetId || R.DestinationContainerId.IsValid() || R.Amount < 1 || R.X != 0 || R.Y != 0 || R.bRotated)
		{
			return Reject(EError::InvalidRequest);
		}
		const auto* Target = State.Items.Find(R.MergeTargetId);
		if (!Target) { return Reject(EError::ItemNotFound); }
		if (Target->Revision != R.ExpectedTargetRevision) { return Reject(EError::StaleItemRevision); }
		const auto* Definition = State.Definitions.Find(Original->DefinitionId);
		if (!Editable(*Target) || Target->OwnerId != Original->OwnerId || Target->RunId != Original->RunId
			|| Target->DefinitionId != Original->DefinitionId || !(Target->RewardMetadata == Original->RewardMetadata)
			|| !State.Grid.Layouts.ContainsByPredicate([&](const auto& L) { return L.ContainerId == Target->ParentContainerId; })
			|| !Definition || !Definition->Supports(EShanmenItemResourceKind::Quantity)) { return Reject(EError::GridPolicyViolation); }
		Transferred = FMath::Min3(R.Amount, Original->Quantity, Definition->MaxStack - Target->Quantity);
		if (Transferred < 1) { return Reject(EError::InsufficientResource); }
		auto& SourceEdit = Candidate.Items.FindChecked(R.ItemInstanceId);
		auto& TargetEdit = Candidate.Items.FindChecked(R.MergeTargetId);
		SourceEdit.Quantity -= Transferred; TargetEdit.Quantity += Transferred;
		++SourceEdit.Revision; ++TargetEdit.Revision;
		if (SourceEdit.Quantity == 0)
		{
			Candidate.Containers.FindChecked(SourceEdit.ParentContainerId).Slots[SourceEdit.SlotIndex].Invalidate();
			SourceEdit.ParentContainerId.Invalidate(); SourceEdit.SlotIndex = INDEX_NONE;
			SourceEdit.State = EShanmenItemInstanceState::Depleted;
			Candidate.Grid.RotatedItems.Remove(R.ItemInstanceId);
		}
	}
	else
	{
		if (!R.DestinationContainerId.IsValid() || R.MergeTargetId.IsValid()
			|| (R.Action == EShanmenItemGridAction::Move && R.Amount != 0)) { return Reject(EError::InvalidRequest); }
		const bool Split = R.Action == EShanmenItemGridAction::Split;
		const auto* Definition = State.Definitions.Find(Original->DefinitionId);
		if (Split && (R.Amount < 1 || R.Amount >= Original->Quantity || !Definition
			|| !Definition->Supports(EShanmenItemResourceKind::Quantity))) { return Reject(EError::InvalidRequest); }
		const auto PlacementError = FShanmenItemGridPolicy::CanPlace(CaptureSnapshot(), R.ItemInstanceId,
			R.DestinationContainerId, R.X, R.Y, R.bRotated, !Split);
		if (PlacementError != EError::None) { return Reject(PlacementError); }
		const auto& Layout = *Candidate.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == R.DestinationContainerId; });
		FShanmenItemInstance Moved = *Original;
		Moved.ParentContainerId = R.DestinationContainerId; Moved.SlotIndex = R.Y * Layout.Width + R.X;
		if (Split)
		{
			ResultItemId = FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Grid.Split.r1"),
				{R.Context.OwnerId.ToString(), R.Context.RunId.ToString(), R.Context.RequestId.ToString(), R.ItemInstanceId.ToString()});
			if (Candidate.Items.Contains(ResultItemId)) { return Reject(EError::InvariantViolation); }
			Moved.ItemInstanceId = ResultItemId; Moved.Quantity = R.Amount; Moved.Revision = 0;
			auto& SourceEdit = Candidate.Items.FindChecked(R.ItemInstanceId);
			SourceEdit.Quantity -= R.Amount; ++SourceEdit.Revision;
			Transferred = R.Amount;
		}
		else
		{
			Candidate.Containers.FindChecked(Original->ParentContainerId).Slots[Original->SlotIndex].Invalidate();
			++Moved.Revision;
		}
		Candidate.Containers.FindChecked(R.DestinationContainerId).Slots[Moved.SlotIndex] = ResultItemId;
		Candidate.Items.Add(ResultItemId, Moved);
		Candidate.Grid.RotatedItems.Remove(ResultItemId);
		if (R.bRotated) { Candidate.Grid.RotatedItems.Add(ResultItemId); }
		Candidate.Grid.Canonicalize();
	}
	FShanmenItemRepository StorageCandidate; StorageCandidate.State = Candidate; StorageCandidate.bInitialized = true;
	auto Resized = StorageCandidate.CaptureSnapshot();
	const auto CapacityError = FShanmenItemGridPolicy::ReconcileStorage(Resized);
	if (CapacityError != EError::None) { return Reject(CapacityError); }
	Candidate.Grid = MoveTemp(Resized.Grid);
	for (const auto& C : Resized.Containers) { Candidate.Containers.Add(C.ContainerId, C); }
	for (const auto& I : Resized.Items) { Candidate.Items.Add(I.ItemInstanceId, I); }
	Candidate.Grid.Canonicalize();
	++Candidate.AuthorityRevision;
	FShanmenItemTransactionReceipt Receipt;
	Receipt.bSuccess = true; Receipt.Operation = Operation; Receipt.Phase = EShanmenItemTransactionPhase::Committed;
	Receipt.Error = EError::None;
	Receipt.RequestId = R.Context.RequestId; Receipt.ItemInstanceId = ResultItemId;
	Receipt.Amount = Transferred; Receipt.ResourceKind = EShanmenItemResourceKind::Quantity;
	Receipt.ResourceBefore = BeforeQuantity; Receipt.ResourceAfter = Candidate.Items.FindChecked(R.ItemInstanceId).Quantity;
	Receipt.AvailableAfter = Receipt.ResourceAfter; Receipt.ItemRevision = Candidate.Items.FindChecked(ResultItemId).Revision;
	Receipt.AuthorityRevision = Candidate.AuthorityRevision;
	Receipt.PurposeId = R.Action == EShanmenItemGridAction::Equip ? TEXT("Grid.Equip") : R.Action == EShanmenItemGridAction::Move ? TEXT("Grid.Move")
		: R.Action == EShanmenItemGridAction::Split ? TEXT("Grid.Split") : TEXT("Grid.Merge");
	Receipt.ReceiptId = MakeReceiptId(R.Context.RequestId, FingerprintId, Receipt.Phase, Receipt.Error);
	RecordProcessed(Candidate, R.Context.RequestId, FingerprintId, Receipt);
	EError Error;
	if (!Receipt.IsValid() || !ValidateState(Candidate, &Error)) { return Reject(EError::InvariantViolation); }
	State = MoveTemp(Candidate);
	return Receipt;
}
