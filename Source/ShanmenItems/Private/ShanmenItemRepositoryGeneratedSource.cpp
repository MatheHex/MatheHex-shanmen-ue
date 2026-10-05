#include "ShanmenItemRepository.h"
#include "ShanmenItemGeneratedSourceCodec.h"
#include "ShanmenDeterministicId.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	using FPlan = FShanmenItemGeneratedSourcePlan;
	using FSource = FShanmenItemGeneratedSourceReceipt;
	using FContract = FShanmenItemGeneratedSourceContract;
	using EOperation = EShanmenItemTransactionOperation;
	FSource Rebuild(const FPlan& Plan)
	{
		FShanmenItemGeneratedSourceView View;
		View.OwnerId = Plan.OwnerId; View.RunId = Plan.RunId; View.SourceContent = Plan.Content;
		View.State = EShanmenItemGeneratedSourceRunState::Active;
		View.AcceptedSequence = Plan.ExpectedSequence; View.PityState = Plan.PityStateBefore;
		return FContract::Evaluate(View, Plan).Receipt;
	}
	const FShanmenItemTransactionReceipt* FindRunReceipt(
		const TMap<FGuid, FShanmenItemProcessedRequestSnapshot>& Requests, const FGuid& RunId, bool bTerminal)
	{
		for (const auto& Pair : Requests)
		{
			const auto& R = Pair.Value.Receipt;
			if (R.IsSuccess() && R.ReservationId == RunId && (bTerminal
				? R.Operation == EOperation::FinalizePreparedRun
				: (R.Operation == EOperation::StartPreparedRun || R.Operation == EOperation::ClaimPreparedRun)))
			{
				return &R;
			}
		}
		return nullptr;
	}
	FShanmenItemTransactionReceipt Transaction(const FSource& Source, int32 Revision, const FGuid& ReceiptId)
	{
		FShanmenItemTransactionReceipt R;
		R.bSuccess = true; R.Operation = EOperation::AcceptGeneratedSource;
		R.Phase = EShanmenItemTransactionPhase::Committed;
		R.RequestId = Source.GetSourceId(); R.ReceiptId = ReceiptId;
		R.ReservationId = Source.GetPlan().RunId;
		R.ItemInstanceId = Source.GetContainerId();
		R.Amount = Source.GetPlan().Entries.Num();
		R.AuthorityRevision = Revision; R.PurposeId = Source.GetPlan().SourceRoleId;
		return R;
	}
}

FGuid FShanmenItemRepository::Fingerprint(const FShanmenItemGeneratedSourceRequest& Request)
{
	TSharedPtr<FJsonObject> Object;
	if (!Request.ItemContent.IsValid() || !FShanmenItemGeneratedSourceCodec::Encode(Request.Plan, Object)) { return FGuid(); }
	FString Text;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
	if (!FJsonSerializer::Serialize(Object.ToSharedRef(), Writer)) { return FGuid(); }
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.AcceptGeneratedSource.v1"),
		{ Request.ItemContent.Version.ToString().ToLower(), Request.ItemContent.Digest, Text });
}

bool FShanmenItemRepository::ValidateGeneratedSources(const FState& Candidate)
{
	if (Candidate.GeneratedSources.Num() > MaxGeneratedSources) { return false; }
	TMap<FGuid, TArray<const FPlan*>> ByRun;
	TSet<FGuid> IdentitySet;
	int32 EntryCount = 0;
	for (const auto& Pair : Candidate.ProcessedRequests)
	{
		if (Pair.Value.Receipt.IsSuccess() && Pair.Value.Receipt.Operation == EOperation::AcceptGeneratedSource
			&& !Candidate.GeneratedSources.Contains(Pair.Key)) { return false; }
		if (Pair.Value.Receipt.IsSuccess() && Pair.Value.Receipt.Operation == EOperation::MaterializeGeneratedSource)
		{
			const auto& M=Pair.Value.Receipt;
			bool Found=false;
			for (const auto& SourcePair:Candidate.GeneratedSources) if (SourcePair.Value.RunId==M.ReservationId
				&& SourcePair.Value.SourceRoleId==M.PurposeId && FShanmenItemSourceMaterializeRequest::MakeRequestId(
					SourcePair.Value.OwnerId,M.ReservationId,M.PurposeId)==M.RequestId) { Found=true; break; }
			if (!Found) return false;
		}
		IdentitySet.Add(Pair.Value.Receipt.ReceiptId);
	}
	for (const auto& Pair : Candidate.GeneratedSources)
	{
		const FPlan& Plan = Pair.Value;
		if (!Plan.IsValid() || Plan.Entries.Num() > MaxGeneratedEntries - EntryCount) { return false; }
		EntryCount += Plan.Entries.Num();
		const FSource Source = Rebuild(Plan);
		const auto* Processed = Candidate.ProcessedRequests.Find(Pair.Key);
		const auto* Claim = FindRunReceipt(Candidate.ProcessedRequests, Plan.RunId, false);
		const auto* Terminal = FindRunReceipt(Candidate.ProcessedRequests, Plan.RunId, true);
		const auto* First = Claim && !Claim->ReservationIds.IsEmpty() ? Candidate.Reservations.Find(Claim->ReservationIds[0]) : nullptr;
		if (!Source.IsValid() || Source.GetSourceId() != Pair.Key || !Processed || !First || First->OwnerId != Plan.OwnerId
			|| Processed->Receipt.AuthorityRevision <= Claim->AuthorityRevision
			|| Processed->Receipt.AuthorityRevision > Candidate.AuthorityRevision
			|| (Terminal && Processed->Receipt.AuthorityRevision >= Terminal->AuthorityRevision)) { return false; }
		FShanmenItemGeneratedSourceRequest Request { Candidate.Content, Plan };
		const FGuid FP = Fingerprint(Request);
		const FGuid ReceiptId = MakeReceiptId(Pair.Key, FP, EShanmenItemTransactionPhase::Committed, EShanmenItemTransactionError::None);
		if (Processed->Fingerprint != FP || !(Processed->Receipt == Transaction(Source, Processed->Receipt.AuthorityRevision, ReceiptId))) { return false; }
		const auto MaterializeId=FShanmenItemSourceMaterializeRequest::MakeRequestId(Plan.OwnerId,Plan.RunId,Plan.SourceRoleId);
		const auto* Materialized=Candidate.ProcessedRequests.Find(MaterializeId);
		if (Materialized)
		{
			const auto& M=Materialized->Receipt;
			const auto MaterializeFP=FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Source.Materialize.Command.r1"),
				{Plan.OwnerId.ToString(),First->RunId.ToString(),Plan.RunId.ToString(),Plan.SourceRoleId.ToString().ToLower(),
				Candidate.Content.Version.ToString(),Candidate.Content.Digest});
			const auto* C=Candidate.Containers.Find(Source.GetContainerId());
			const auto* L=Candidate.Grid.Layouts.FindByPredicate([&](const auto& V){return V.ContainerId==Source.GetContainerId();});
			if (!M.IsSuccess() || M.Operation!=EOperation::MaterializeGeneratedSource || M.ReservationId!=Plan.RunId
				|| M.ItemInstanceId!=Source.GetContainerId() || M.ReservationIds!=Source.GetItemIds() || M.PurposeId!=Plan.SourceRoleId
				|| M.Amount!=Plan.Entries.Num() || M.AuthorityRevision<=Processed->Receipt.AuthorityRevision
				|| (Terminal && M.AuthorityRevision>=Terminal->AuthorityRevision) || Materialized->Fingerprint!=MaterializeFP
				|| M.ReceiptId!=MakeReceiptId(MaterializeId,MaterializeFP,M.Phase,M.Error)
				|| !C || C->OwnerId!=Plan.OwnerId || C->RunId!=First->RunId || C->ContainerType!=TEXT("GeneratedSource")
				|| !L || L->Kind!=EShanmenItemGridKind::World || L->Width!=8 || L->Height!=8) return false;
			for (int32 N=0;N<Plan.Entries.Num();++N)
			{
				const auto* I=Candidate.Items.Find(Source.GetItemIds()[N]); const auto& E=Plan.Entries[N];
				if (!I || I->OwnerId!=Plan.OwnerId || I->RunId!=First->RunId || I->DefinitionId!=E.Definition.DefinitionId
					|| !(I->RewardMetadata==E.RewardMetadata)) return false;
			}
		}
		TArray<FGuid> Ids = Source.GetItemIds();
		Ids.Add(Source.GetSourceId()); Ids.Add(Source.GetContainerId());
		for (const FGuid& Id : Ids)
		{
			// Unacquired source identities are reserved, not inserted into the item graph.
			const bool ItemId=Source.GetItemIds().Contains(Id), ContainerId=Id==Source.GetContainerId();
			if (IdentitySet.Contains(Id) || (Candidate.Items.Contains(Id) && !(Materialized && ItemId))
				|| (Candidate.Containers.Contains(Id) && !(Materialized && ContainerId))
				|| Candidate.Reservations.Contains(Id) || (Candidate.ProcessedRequests.Contains(Id) && Id != Pair.Key)) { return false; }
			IdentitySet.Add(Id);
		}
		for (const auto& Entry : Plan.Entries)
		{
			const auto* Definition = Candidate.Definitions.Find(Entry.Definition.DefinitionId);
			if (!Definition || !(*Definition == Entry.Definition)) { return false; }
		}
		ByRun.FindOrAdd(Plan.RunId).Add(&Plan);
	}
	for (auto& Pair : ByRun)
	{
		auto& Plans = Pair.Value;
		Plans.Sort([](const FPlan& A, const FPlan& B) { return A.ExpectedSequence < B.ExpectedSequence; });
		int64 Sequence = 0;
		int32 Pity = 0;
		int32 PreviousRevision = INDEX_NONE;
		for (const FPlan* Plan : Plans)
		{
			const FGuid Id = FContract::MakeSourceId(Plan->OwnerId, Plan->RunId, Plan->SourceRoleId);
			const int32 Revision = Candidate.ProcessedRequests.FindChecked(Id).Receipt.AuthorityRevision;
			if (Plan->ExpectedSequence != Sequence || Plan->PityStateBefore != Pity || Revision <= PreviousRevision
				|| !IsSameContent(Plan->Content, Plans[0]->Content)) { return false; }
			++Sequence; Pity = Plan->PityStateAfter; PreviousRevision = Revision;
		}
	}
	return true;
}

FShanmenItemTransactionReceipt FShanmenItemRepository::AcceptGeneratedSource(const FShanmenItemGeneratedSourceRequest& Request)
{
	const FPlan& Plan = Request.Plan;
	const FGuid Id = FContract::MakeSourceId(Plan.OwnerId, Plan.RunId, Plan.SourceRoleId);
	const FGuid FP = Fingerprint(Request);
	auto Reject = [&](EShanmenItemTransactionError Error) { return MakeRejected(EOperation::AcceptGeneratedSource, Id, FP, Error); };
	if (!bInitialized) { return Reject(EShanmenItemTransactionError::NotInitialized); }
	if (!Plan.IsValid() || !FP.IsValid()) { return Reject(EShanmenItemTransactionError::InvalidRequest); }
	if (!IsSameContent(Request.ItemContent, State.Content)) { return Reject(EShanmenItemTransactionError::ContentMismatch); }
	if (const auto* Existing = State.GeneratedSources.Find(Id))
	{
		return *Existing == Plan ? State.ProcessedRequests.FindChecked(Id).Receipt : Reject(EShanmenItemTransactionError::RequestIdConflict);
	}
	if (State.ProcessedRequests.Contains(Id)) { return Reject(EShanmenItemTransactionError::RequestIdConflict); }
	const auto* Claim = FindRunReceipt(State.ProcessedRequests, Plan.RunId, false);
	const auto* First = Claim && !Claim->ReservationIds.IsEmpty() ? State.Reservations.Find(Claim->ReservationIds[0]) : nullptr;
	if (!First || First->OwnerId != Plan.OwnerId) { return Reject(EShanmenItemTransactionError::RunNotFound); }
	if (FindRunReceipt(State.ProcessedRequests, Plan.RunId, true)) { return Reject(EShanmenItemTransactionError::RunAlreadyFinalized); }
	if (State.AuthorityRevision == MAX_int32 || State.GeneratedSources.Num() >= MaxGeneratedSources) { return Reject(EShanmenItemTransactionError::InvalidRequest); }
	const FSource Source = Rebuild(Plan);
	if (!Source.IsValid()) { return Reject(EShanmenItemTransactionError::InvalidRequest); }
	FState Candidate = State;
	// A migrated catalog contains only previously owned definitions. Admit the
	// plan's complete definitions in the SAME candidate as its immutable source.
	// As with terminal acquired-item import, existing definitions are immutable;
	// this is catalog consistency, not authorization of a product source/manifest.
	for (const auto& Entry : Plan.Entries)
	{
		const auto* ExistingDefinition = Candidate.Definitions.Find(Entry.Definition.DefinitionId);
		if (ExistingDefinition)
		{
			if (!(*ExistingDefinition == Entry.Definition)) { return Reject(EShanmenItemTransactionError::InvariantViolation); }
		}
		else { Candidate.Definitions.Add(Entry.Definition.DefinitionId, Entry.Definition); }
	}
	Candidate.GeneratedSources.Add(Id, Plan);
	++Candidate.AuthorityRevision;
	const auto Receipt = Transaction(Source, Candidate.AuthorityRevision,
		MakeReceiptId(Id, FP, EShanmenItemTransactionPhase::Committed, EShanmenItemTransactionError::None));
	RecordProcessed(Candidate, Id, FP, Receipt);
	EShanmenItemTransactionError Error;
	if (!ValidateState(Candidate, &Error)) { return Reject(EShanmenItemTransactionError::InvariantViolation); }
	State = MoveTemp(Candidate);
	return Receipt;
}

bool FShanmenItemRepository::TryGetGeneratedSource(const FGuid& OwnerId, const FGuid& RunId, FName SourceRoleId, FSource& OutReceipt) const
{
	OutReceipt = FSource();
	if (!bInitialized) { return false; }
	const auto* Plan = State.GeneratedSources.Find(FContract::MakeSourceId(OwnerId, RunId, SourceRoleId));
	if (!Plan || Plan->OwnerId != OwnerId || Plan->RunId != RunId || Plan->SourceRoleId != SourceRoleId) { return false; }
	OutReceipt = Rebuild(*Plan);
	return OutReceipt.IsValid();
}

FShanmenItemGeneratedSourceReadResult FShanmenItemRepository::ReadGeneratedSource(
	const FGuid& OwnerId, const FGuid& RunId, FName SourceRoleId) const
{
	FShanmenItemGeneratedSourceReadResult Result;
	if (!bInitialized || !OwnerId.IsValid() || !RunId.IsValid() || SourceRoleId.IsNone()) { return Result; }
	const auto* Claim = FindRunReceipt(State.ProcessedRequests, RunId, false);
	const auto* First = Claim && !Claim->ReservationIds.IsEmpty() ? State.Reservations.Find(Claim->ReservationIds[0]) : nullptr;
	if (!First || First->OwnerId != OwnerId) { return Result; }
	Result.OwnerId = OwnerId; Result.RunId = RunId; Result.SourceRoleId = SourceRoleId;
	Result.ItemContent = State.Content; Result.AuthorityRevision = State.AuthorityRevision;
	Result.RunState = FindRunReceipt(State.ProcessedRequests, RunId, true)
		? EShanmenItemGeneratedSourceRunState::Finalized : EShanmenItemGeneratedSourceRunState::Active;
	// State is validated at every install/commit: per-Run sequence is contiguous,
	// manifest is constant, and each prior pity output equals the next input.
	const FPlan* Latest = nullptr;
	for (const auto& Pair : State.GeneratedSources)
	{
		const auto& Plan = Pair.Value;
		if (Plan.RunId == RunId && Plan.OwnerId == OwnerId
			&& (!Latest || Plan.ExpectedSequence > Latest->ExpectedSequence)) { Latest = &Plan; }
	}
	if (Latest)
	{
		Result.SourceContent = Latest->Content;
		Result.AcceptedSequence = Latest->ExpectedSequence + 1;
		Result.PityState = Latest->PityStateAfter;
	}
	const auto* Existing = State.GeneratedSources.Find(FContract::MakeSourceId(OwnerId, RunId, SourceRoleId));
	if (Existing)
	{
		Result.Receipt = Rebuild(*Existing);
		if (!Result.Receipt.IsValid()) { return FShanmenItemGeneratedSourceReadResult(); }
		Result.Status = EShanmenItemGeneratedSourceReadStatus::Accepted;
	}
	else { Result.Status = EShanmenItemGeneratedSourceReadStatus::Absent; }
	return Result;
}
