#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"

FGuid FShanmenItemLoadoutStartRequest::MakeRequestId(const FShanmenOperationContext& C, int32 Revision)
{
	if (!C.OwnerId.IsValid() || !C.RunId.IsValid() || Revision < 0 || !C.Content.IsValid()) return {};
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.LoadoutStart.r1"),
		{C.OwnerId.ToString(), C.RunId.ToString(), C.Content.Version.ToString(), C.Content.Digest, FString::FromInt(Revision)});
}
FGuid FShanmenItemLoadoutStartRequest::MakeLineRequestId(const FGuid& Start, const FGuid& Item)
{
	if (!Start.IsValid() || !Item.IsValid()) return {};
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.LoadoutLine.r1"), {Start.ToString(), Item.ToString()});
}

FShanmenItemTransactionReceipt FShanmenItemRepository::StartLoadout(const FShanmenItemLoadoutStartRequest& R)
{
	using EError = EShanmenItemTransactionError;
	constexpr auto Operation = EShanmenItemTransactionOperation::StartPreparedRun;
	FShanmenItemRunStartRequest Start; Start.Context = R.Context;
	auto Lines = R.Lines;
	Lines.Sort([](const auto& A, const auto& B) { return A.ItemInstanceId.ToString() < B.ItemInstanceId.ToString(); });
	for (const auto& L : Lines) Start.ReservationIds.Add(MakeReservationId(L));
	const auto FP = Fingerprint(Start);
	auto Reject = [&](EError E) { return MakeRejected(Operation, R.Context.RequestId, FP, E); };
	if (!bInitialized) return Reject(EError::NotInitialized);
	if (!R.Context.IsValid() || Lines.IsEmpty() || Lines.Num() > 256
		|| R.Context.RequestId != FShanmenItemLoadoutStartRequest::MakeRequestId(R.Context, R.ExpectedAuthorityRevision))
		return Reject(EError::InvalidRequest);
	if (!IsSameContent(R.Context.Content, State.Content)) return Reject(EError::ContentMismatch);
	TSet<FGuid> Items;
	for (const auto& L : Lines)
	{
		if (!L.IsValid() || Items.Contains(L.ItemInstanceId) || L.Context.OwnerId != R.Context.OwnerId
			|| L.Context.RunId != R.Context.RunId || !IsSameContent(L.Context.Content, R.Context.Content)
			|| L.Context.RequestId != FShanmenItemLoadoutStartRequest::MakeLineRequestId(R.Context.RequestId, L.ItemInstanceId))
			return Reject(EError::InvalidRequest);
		Items.Add(L.ItemInstanceId);
	}
	// A batch replay must verify EVERY immutable line, not only the list of derived
	// reservation IDs. A changed quantity/purpose/revision cannot reuse success.
	FShanmenItemTransactionReceipt Replay;
	if (TryReplay(R.Context.RequestId, FP, Operation, Replay))
	{
		if (!Replay.IsSuccess()) return Replay;
		for (const auto& L : Lines)
		{
			FShanmenItemTransactionReceipt LineReplay;
			if (!TryReplay(L.Context.RequestId, Fingerprint(L), EShanmenItemTransactionOperation::Reserve, LineReplay)
				|| !LineReplay.IsSuccess()) return Reject(EError::RequestIdConflict);
		}
		return Replay;
	}
	if (R.ExpectedAuthorityRevision != State.AuthorityRevision) return Reject(EError::StaleAuthorityRevision);
	for (const auto& Pair : State.Reservations)
		if (Pair.Value.OwnerId == R.Context.OwnerId && Pair.Value.State == EShanmenItemReservationState::Reserved)
			return Reject(EError::ActiveRunConflict); // Never strand an unrelated preparation intent behind this Run.
	if (State.AuthorityRevision > MAX_int32 - Lines.Num() - 1) return Reject(EError::InvariantViolation);
	FShanmenItemRepository Candidate; Candidate.State = State; Candidate.bInitialized = true;
	for (const auto& L : Lines)
	{
		// A new bundle never adopts reservations created through a different path.
		if (State.ProcessedRequests.Contains(L.Context.RequestId)) return Reject(EError::RequestIdConflict);
		const auto Reserved = Candidate.Reserve(L);
		if (!Reserved.IsSuccess()) return Reject(Reserved.Error);
	}
	const auto Started = Candidate.StartPreparedRun(Start);
	if (!Started.IsSuccess()) return Reject(Started.Error);
	State = MoveTemp(Candidate.State);
	return Started;
}
