#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"

FGuid FShanmenItemBasicSupplyRequest::MakeRequestId(const FGuid& Owner, const FGuid& Scope,
	const FGuid& Death, FName Policy)
{
	if (!Owner.IsValid() || !Scope.IsValid() || !Death.IsValid() || Policy.IsNone()) return {};
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.BasicSupply.Request.r1"),
		{Owner.ToString(), Scope.ToString(), Death.ToString(), Policy.ToString().ToLower()});
}
FGuid FShanmenItemBasicSupplyRequest::MakeItemId(const FGuid& Owner, const FGuid& Scope,
	const FGuid& Death, FName Definition)
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.BasicSupply.Item.r1"),
		{Owner.ToString(), Scope.ToString(), Death.ToString(), Definition.ToString().ToLower()});
}

FShanmenItemTransactionReceipt FShanmenItemRepository::ReplenishBasics(const FShanmenItemBasicSupplyRequest& R)
{
	using EError = EShanmenItemTransactionError;
	constexpr auto Operation = EShanmenItemTransactionOperation::ReplenishBasics;
	auto Lines = R.Lines;
	Lines.Sort([](const auto& A, const auto& B) { return A.DefinitionId.LexicalLess(B.DefinitionId); });
	TArray<FString> Parts = {R.Context.OwnerId.ToString(), R.Context.RunId.ToString(), R.DeathRequestId.ToString(),
		R.PolicyId.ToString().ToLower(), R.Context.Content.Version.ToString().ToLower(), R.Context.Content.Digest};
	for (const auto& L : Lines)
	{
		Parts.Add(L.DefinitionId.ToString().ToLower()); Parts.Add(L.DestinationContainerId.ToString()); Parts.Add(FString::FromInt(L.Quantity));
	}
	// CAS is a precondition, not the semantic identity of an accepted command.
	const FGuid FP = FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.BasicSupply.Command.r1"), Parts);
	FShanmenItemTransactionReceipt Replayed;
	if (bInitialized && TryReplay(R.Context.RequestId, FP, Operation, Replayed)) return Replayed;
	auto Reject = [&](EError Error) { return MakeRejected(Operation, R.Context.RequestId, FP, Error); };
	if (!bInitialized) return Reject(EError::NotInitialized);
	if (!R.Context.IsValid() || Lines.IsEmpty() || Lines.Num() > 4 || R.ExpectedAuthorityRevision < 0
		|| R.Context.RequestId != FShanmenItemBasicSupplyRequest::MakeRequestId(R.Context.OwnerId, R.Context.RunId, R.DeathRequestId, R.PolicyId))
		return Reject(EError::InvalidRequest);
	if (!IsSameContent(R.Context.Content, State.Content)) return Reject(EError::ContentMismatch);
	if (R.ExpectedAuthorityRevision != State.AuthorityRevision) return Reject(EError::StaleAuthorityRevision);
	if (State.AuthorityRevision == MAX_int32) return Reject(EError::InvariantViolation);
	const auto* Death = State.ProcessedRequests.Find(R.DeathRequestId);
	if (!Death || !Death->Receipt.IsSuccess() || Death->Receipt.Operation != EShanmenItemTransactionOperation::FinalizePreparedRun
		|| Death->Receipt.PurposeId != FShanmenItemRunLifecyclePurpose::Death() || Death->Receipt.ReservationIds.IsEmpty())
		return Reject(EError::BasicSupplyNotEligible);
	const auto* First = State.Reservations.Find(Death->Receipt.ReservationIds[0]);
	if (!First || First->OwnerId != R.Context.OwnerId || First->RunId != R.Context.RunId) return Reject(EError::ScopeMismatch);
	for (const auto& Pair : State.ProcessedRequests)
	{
		const auto& Receipt = Pair.Value.Receipt;
		if (!Receipt.IsSuccess()) continue;
		if (Receipt.Operation == Operation && Receipt.ReservationId == R.DeathRequestId) return Reject(EError::BasicSupplyAlreadyUsed);
		if (Receipt.Operation != EShanmenItemTransactionOperation::StartPreparedRun
			&& Receipt.Operation != EShanmenItemTransactionOperation::ClaimPreparedRun
			&& Receipt.Operation != EShanmenItemTransactionOperation::FinalizePreparedRun) continue;
		const auto* Reservation = Receipt.ReservationIds.IsEmpty() ? nullptr : State.Reservations.Find(Receipt.ReservationIds[0]);
		if (Reservation && Reservation->OwnerId == R.Context.OwnerId && Receipt.AuthorityRevision > Death->Receipt.AuthorityRevision)
			return Reject(EError::BasicSupplyNotEligible); // Old deaths cannot sponsor a later preparation/Run.
	}
	for (const auto& Pair : State.Reservations)
	{
		if (Pair.Value.OwnerId == R.Context.OwnerId && Pair.Value.State == EShanmenItemReservationState::Reserved)
			return Reject(EError::BasicSupplyNotEligible);
	}
	FState Candidate = State;
	TSet<FName> Definitions, Roles;
	TArray<FGuid> Granted;
	for (const auto& L : Lines)
	{
		const auto* D = State.Definitions.Find(L.DefinitionId);
		const auto* F = State.Grid.Footprints.FindByPredicate([&](const auto& Value) { return Value.DefinitionId == L.DefinitionId; });
		const auto* C = State.Containers.Find(L.DestinationContainerId);
		const auto* Layout = State.Grid.Layouts.FindByPredicate([&](const auto& Value) { return Value.ContainerId == L.DestinationContainerId; });
		if (!D || !F || !C || !Layout || Definitions.Contains(L.DefinitionId) || L.Quantity < 1 || L.Quantity > 2
			|| L.Quantity > D->MaxStack || C->OwnerId != R.Context.OwnerId || C->RunId != R.Context.RunId)
			return Reject(EError::InvalidRequest);
		Definitions.Add(L.DefinitionId);
		const bool Equipment = !F->EquipmentRole.IsNone();
		if (Equipment ? (L.Quantity != 1 || Layout->Kind != EShanmenItemGridKind::Equipment || Layout->EquipmentRole != F->EquipmentRole
			|| Roles.Contains(F->EquipmentRole) || !D->Supports(EShanmenItemResourceKind::DeploymentLock))
			: (Layout->Kind != EShanmenItemGridKind::Stash || !F->bSecureAllowed || !D->Supports(EShanmenItemResourceKind::Quantity)))
			return Reject(EError::GridPolicyViolation);
		if (Equipment) Roles.Add(F->EquipmentRole);
		bool Owned = false;
		// Check the whole owner's stock, not only the equipped slot or baseline definition.
		for (const auto& Pair : State.Items)
		{
			const auto& I = Pair.Value;
			if (I.OwnerId != R.Context.OwnerId || I.Quantity < 1 || I.State == EShanmenItemInstanceState::Destroyed
				|| I.State == EShanmenItemInstanceState::Depleted) continue;
			const auto* IF = State.Grid.Footprints.FindByPredicate([&](const auto& Value) { return Value.DefinitionId == I.DefinitionId; });
			if (Equipment ? (IF && IF->EquipmentRole == F->EquipmentRole) : I.DefinitionId == L.DefinitionId) { Owned = true; break; }
		}
		if (Owned) continue;
		FShanmenItemInstance I;
		I.ItemInstanceId = FShanmenItemBasicSupplyRequest::MakeItemId(R.Context.OwnerId, R.Context.RunId, R.DeathRequestId, L.DefinitionId);
		I.DefinitionId = L.DefinitionId; I.OwnerId = R.Context.OwnerId; I.RunId = R.Context.RunId;
		I.Quantity = L.Quantity; I.Durability = D->MaxDurability; I.Charges = D->MaxCharges;
		if (Candidate.Items.Contains(I.ItemInstanceId) || Candidate.Containers.Contains(I.ItemInstanceId)
			|| Candidate.Reservations.Contains(I.ItemInstanceId) || Candidate.ProcessedRequests.Contains(I.ItemInstanceId))
			return Reject(EError::InvariantViolation);
		Candidate.Items.Add(I.ItemInstanceId, I);
		FShanmenItemRepository Preview; Preview.State = Candidate; Preview.bInitialized = true;
		const auto S = Preview.CaptureSnapshot();
		int32 Anchor = INDEX_NONE;
		for (int32 Y = 0; Y < Layout->Height && Anchor == INDEX_NONE; ++Y)
			for (int32 X = 0; X < Layout->Width; ++X)
				if (FShanmenItemGridPolicy::CanPlace(S, I.ItemInstanceId, L.DestinationContainerId, X, Y, false) == EError::None)
					{ Anchor = Y * Layout->Width + X; break; }
		if (Anchor == INDEX_NONE) return Reject(EError::GridNoSpace);
		auto& Added = Candidate.Items.FindChecked(I.ItemInstanceId); Added.ParentContainerId = L.DestinationContainerId; Added.SlotIndex = Anchor;
		Candidate.Containers.FindChecked(L.DestinationContainerId).Slots[Anchor] = I.ItemInstanceId;
		Granted.Add(I.ItemInstanceId);
	}
	if (Granted.IsEmpty()) return Reject(EError::BasicSupplyNotNeeded);
	FShanmenItemRepository Preview; Preview.State = Candidate; Preview.bInitialized = true;
	auto S = Preview.CaptureSnapshot();
	const auto GridError = FShanmenItemGridPolicy::ReconcileStorage(S);
	if (GridError != EError::None) return Reject(GridError);
	Candidate.Grid = MoveTemp(S.Grid);
	for (const auto& C : S.Containers) Candidate.Containers.Add(C.ContainerId, C);
	for (const auto& I : S.Items) Candidate.Items.Add(I.ItemInstanceId, I);
	++Candidate.AuthorityRevision;
	FShanmenItemTransactionReceipt Receipt;
	Receipt.bSuccess = true; Receipt.Operation = Operation; Receipt.Phase = EShanmenItemTransactionPhase::Committed;
	Receipt.RequestId = R.Context.RequestId; Receipt.ReservationId = R.DeathRequestId; Receipt.ItemInstanceId = Granted[0];
	Receipt.ReservationIds = Granted; Receipt.Amount = Granted.Num(); Receipt.AuthorityRevision = Candidate.AuthorityRevision;
	Receipt.PurposeId = R.PolicyId; Receipt.ReceiptId = MakeReceiptId(R.Context.RequestId, FP, Receipt.Phase, Receipt.Error);
	RecordProcessed(Candidate, R.Context.RequestId, FP, Receipt);
	EError Error;
	if (!Receipt.IsValid() || !ValidateState(Candidate, &Error)) return Reject(EError::InvariantViolation);
	State = MoveTemp(Candidate); return Receipt;
}
