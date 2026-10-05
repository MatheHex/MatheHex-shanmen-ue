#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using EOp = EShanmenItemTransactionOperation;
	using EError = EShanmenItemTransactionError;
	const FShanmenItemTransactionReceipt* Claim(const FShanmenItemAuthoritySnapshot& S, const FGuid& Run)
	{
		for (const auto& P : S.ProcessedRequests) if (P.Receipt.IsSuccess() && P.Receipt.ReservationId == Run
			&& (P.Receipt.Operation == EOp::StartPreparedRun || P.Receipt.Operation == EOp::ClaimPreparedRun)) return &P.Receipt;
		return nullptr;
	}
	FShanmenItemGeneratedSourceReceipt Source(const FShanmenItemGeneratedSourcePlan& Plan)
	{
		FShanmenItemGeneratedSourceView V; V.OwnerId=Plan.OwnerId; V.RunId=Plan.RunId; V.SourceContent=Plan.Content;
		V.State=EShanmenItemGeneratedSourceRunState::Active; V.AcceptedSequence=Plan.ExpectedSequence; V.PityState=Plan.PityStateBefore;
		return FShanmenItemGeneratedSourceContract::Evaluate(V,Plan).Receipt;
	}
}

FGuid FShanmenItemRunInventoryRequest::MakeRequestId(const FGuid& Owner,const FGuid& Scope,const FGuid& Run)
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Run.Inventory.r1"),
		{Owner.ToString(),Scope.ToString(),Run.ToString()});
}

FGuid FShanmenItemRunInventoryRequest::Fingerprint() const
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Run.Inventory.Command.r1"),
		{Context.OwnerId.ToString(),Context.RunId.ToString(),ActiveRunId.ToString(),Context.Content.Version.ToString(),Context.Content.Digest});
}

bool FShanmenItemRunGridPolicy::IsMaterialized(const FShanmenItemAuthoritySnapshot& S,const FGuid& Run)
{
	return S.ProcessedRequests.ContainsByPredicate([&](const auto& P){return P.Receipt.IsSuccess()
		&& P.Receipt.Operation==EOp::MaterializeRunInventory && P.Receipt.ReservationId==Run;});
}

bool FShanmenItemRunGridPolicy::IsTransferred(const FShanmenItemAuthoritySnapshot& S,const FGuid& Run,const FGuid& Reservation)
{
	return S.ProcessedRequests.ContainsByPredicate([&](const auto& P){return P.Receipt.IsSuccess()
		&& P.Receipt.Operation==EOp::MaterializeRunInventory && P.Receipt.ReservationId==Run
		&& P.Receipt.ReservationIds.Contains(Reservation);});
}

FGuid FShanmenItemSourceMaterializeRequest::MakeRequestId(const FGuid& Owner,const FGuid& Run,FName Role)
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Source.Materialize.r1"),
		{Owner.ToString(),Run.ToString(),Role.ToString().ToLower()});
}

bool FShanmenItemRunGridPolicy::Project(const FShanmenItemAuthoritySnapshot& S,const FGuid& Owner,
	const FGuid& Scope,const FGuid& Run,FShanmenItemAuthoritySnapshot& Out)
{
	Out={}; const auto* Active=Claim(S,Run);
	if (!Active || Active->ReservationIds.IsEmpty()) return false;
	const auto* First=S.Reservations.FindByPredicate([&](const auto& V){return V.ReservationId==Active->ReservationIds[0];});
	if (!First || First->OwnerId!=Owner || First->RunId!=Scope) return false;
	for (const auto& P:S.ProcessedRequests)
	{
		const auto& R=P.Receipt;
		if (R.IsSuccess() && R.Operation==EOp::FinalizePreparedRun && R.ReservationId==Run) return false;
		if (R.IsSuccess() && R.Operation==EOp::PreparePreparedRunResourceIntent && R.ItemInstanceId==Run
			&& !S.ProcessedRequests.ContainsByPredicate([&](const auto& F){return F.Receipt.IsSuccess()
				&& F.Receipt.Operation==EOp::FinalizePreparedRunResourceIntent && F.Receipt.ItemInstanceId==R.RequestId;})) return false;
		if (!R.IsSuccess() || R.Operation!=EOp::PreparePreparedRunQuantityIntent || R.ReservationIds.Num()!=1 || R.ReservationIds[0]!=Run) continue;
		if (!S.ProcessedRequests.ContainsByPredicate([&](const auto& F){return F.Receipt.IsSuccess()
			&& F.Receipt.Operation==EOp::FinalizePreparedRunQuantityIntent && F.Receipt.ReservationIds.Num()==2
			&& F.Receipt.ReservationIds[0]==Run && F.Receipt.ReservationIds[1]==R.RequestId;})) return false;
	}
	if (IsMaterialized(S,Run)) { Out=S; return true; }
	FShanmenItemAuthoritySnapshot P=S;
	for (const auto& Id:Active->ReservationIds)
	{
		const auto* V=S.Reservations.FindByPredicate([&](const auto& R){return R.ReservationId==Id;});
		if (!V || V->OwnerId!=Owner || V->RunId!=Scope) return false;
		if (V->ResourceKind!=EShanmenItemResourceKind::Quantity) continue;
		int32 Remaining=V->Amount;
		for (const auto& Entry:S.ProcessedRequests)
		{
			const auto& R=Entry.Receipt;
			if (!R.IsSuccess() || R.ItemInstanceId!=V->ItemInstanceId) continue;
			if ((R.Operation==EOp::ConsumePreparedRunItem && R.ReservationId==Run)
				|| (R.Operation==EOp::FinalizePreparedRunQuantityIntent && R.Phase==EShanmenItemTransactionPhase::Committed
					&& R.ReservationIds.Num()==2 && R.ReservationIds[0]==Run)) Remaining-=R.Amount;
		}
		if (Remaining<0) return false;
		if (!Remaining) continue;
		FName Purpose; FGuid Container; int32 Slot;
		if (!FShanmenItemReservationPlacement::Decode(V->PurposeId,Purpose,Container,Slot)) return false;
		auto* C=P.Containers.FindByPredicate([&](const auto& R){return R.ContainerId==Container;});
		auto* I=P.Items.FindByPredicate([&](const auto& R){return R.ItemInstanceId==V->ItemInstanceId;});
		if (!C || !I || !C->Slots.IsValidIndex(Slot) || C->Slots[Slot].IsValid()) return false;
		I->Quantity=Remaining; I->State=EShanmenItemInstanceState::Stored; I->ParentContainerId=Container; I->SlotIndex=Slot;
		C->Slots[Slot]=I->ItemInstanceId;
	}
	if (!FShanmenItemGridPolicy::Validate(P)) return false;
	Out=MoveTemp(P); return true;
}

FShanmenItemTransactionReceipt FShanmenItemRepository::MaterializeRunInventory(const FShanmenItemRunInventoryRequest& R)
{
	const auto FP=R.Fingerprint(); FShanmenItemTransactionReceipt Replayed;
	if (bInitialized && TryReplay(R.Context.RequestId,FP,EOp::MaterializeRunInventory,Replayed)) return Replayed;
	auto Reject=[&](EError E){return MakeRejected(EOp::MaterializeRunInventory,R.Context.RequestId,FP,E);};
	if (!bInitialized) return Reject(EError::NotInitialized);
	if (!R.Context.IsValid() || !R.ActiveRunId.IsValid()
		|| R.Context.RequestId!=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId)) return Reject(EError::InvalidRequest);
	if (!IsSameContent(R.Context.Content,State.Content)) return Reject(EError::ContentMismatch);
	const auto S=CaptureSnapshot(); FShanmenItemAuthoritySnapshot P;
	if (!FShanmenItemRunGridPolicy::Project(S,R.Context.OwnerId,R.Context.RunId,R.ActiveRunId,P)) return Reject(EError::RunItemIntentConflict);
	const auto* Active=Claim(S,R.ActiveRunId);
	if (!Active || State.AuthorityRevision==MAX_int32) return Reject(EError::InvariantViolation);
	FState Candidate=State; TArray<FGuid> TransferredIds; int64 Total=0;
	for (const auto& Id:Active->ReservationIds)
	{
		const auto* V=State.Reservations.Find(Id);
		if (!V || V->State!=EShanmenItemReservationState::Committed) return Reject(EError::RunNotFound);
		FName Purpose; FGuid ContainerId; int32 Slot;
		if (!FShanmenItemReservationPlacement::Decode(V->PurposeId,Purpose,ContainerId,Slot)) return Reject(EError::GridPolicyViolation);
		const auto* L=Candidate.Grid.Layouts.FindByPredicate([&](const auto& Layout){return Layout.ContainerId==ContainerId;});
		if (!L) return Reject(EError::GridPolicyViolation);
		if (V->ResourceKind==EShanmenItemResourceKind::DeploymentLock && L->Kind!=EShanmenItemGridKind::Carry) continue;
		if (L->Kind!=EShanmenItemGridKind::Carry) return Reject(EError::GridPolicyViolation);
		TransferredIds.Add(Id);
		auto* I=Candidate.Items.Find(V->ItemInstanceId);
		const auto* Projected=P.Items.FindByPredicate([&](const auto& Item){return Item.ItemInstanceId==V->ItemInstanceId;});
		if (V->ResourceKind==EShanmenItemResourceKind::DeploymentLock)
		{
			if (!I || I->State!=EShanmenItemInstanceState::Deployed || I->DeploymentReservationId!=Id
				|| I->Revision==MAX_int32 || I->ChildContainerId.IsValid() || I->ParentContainerId!=ContainerId) return Reject(EError::GridPolicyViolation);
			I->State=EShanmenItemInstanceState::Stored; I->DeploymentReservationId.Invalidate(); ++I->Revision;
			Total+=I->Quantity; if (Total>MAX_int32) return Reject(EError::InvariantViolation);
			continue;
		}
		if (V->ResourceKind!=EShanmenItemResourceKind::Quantity) return Reject(EError::GridPolicyViolation);
		if (!I || !Projected || I->State!=EShanmenItemInstanceState::Depleted || I->Quantity!=0
			|| I->Revision==MAX_int32 || I->ChildContainerId.IsValid()) return Reject(EError::GridPolicyViolation);
		if (!Projected->Quantity) continue;
		if (Projected->ParentContainerId!=ContainerId) return Reject(EError::GridPolicyViolation);
		Total+=Projected->Quantity; if (Total>MAX_int32) return Reject(EError::InvariantViolation);
		I->Quantity=Projected->Quantity; I->State=EShanmenItemInstanceState::Stored;
		I->ParentContainerId=Projected->ParentContainerId; I->SlotIndex=Projected->SlotIndex; ++I->Revision;
		Candidate.Containers.FindChecked(I->ParentContainerId).Slots[I->SlotIndex]=I->ItemInstanceId;
	}
	++Candidate.AuthorityRevision;
	FShanmenItemTransactionReceipt Result; Result.bSuccess=true; Result.Operation=EOp::MaterializeRunInventory;
	Result.Phase=EShanmenItemTransactionPhase::Committed; Result.RequestId=R.Context.RequestId; Result.ReservationId=R.ActiveRunId;
	Result.ItemInstanceId=Active->RequestId; Result.ReservationIds=TransferredIds; Result.Amount=TransferredIds.Num();
	Result.ResourceBefore=Result.ResourceAfter=Result.AvailableAfter=static_cast<int32>(Total);
	Result.PurposeId=TEXT("Run.Inventory.r1"); Result.AuthorityRevision=Candidate.AuthorityRevision;
	Result.ReceiptId=MakeReceiptId(Result.RequestId,FP,Result.Phase,Result.Error);
	RecordProcessed(Candidate,Result.RequestId,FP,Result);
	EError Error; if (!Result.IsValid() || !ValidateState(Candidate,&Error)) return Reject(EError::InvariantViolation);
	State=MoveTemp(Candidate); return Result;
}

bool FShanmenItemRepository::ValidateRunInventory(const FState& Candidate)
{
	for (const auto& Pair:Candidate.ProcessedRequests)
	{
		const auto& M=Pair.Value.Receipt;
		if (!M.IsSuccess() || M.Operation!=EOp::MaterializeRunInventory) continue;
		const FShanmenItemTransactionReceipt* Active=nullptr; const FShanmenItemTransactionReceipt* Terminal=nullptr;
		for (const auto& Entry:Candidate.ProcessedRequests)
		{
			const auto& E=Entry.Value.Receipt; if (!E.IsSuccess() || E.ReservationId!=M.ReservationId) continue;
			if (E.Operation==EOp::StartPreparedRun || E.Operation==EOp::ClaimPreparedRun) Active=&E;
			if (E.Operation==EOp::FinalizePreparedRun) Terminal=&E;
		}
		if (!Active || Active->ReservationIds.IsEmpty() || Active->RequestId!=M.ItemInstanceId
			|| M.AuthorityRevision<=Active->AuthorityRevision || (Terminal && M.AuthorityRevision>=Terminal->AuthorityRevision)) return false;
		const auto* First=Candidate.Reservations.Find(Active->ReservationIds[0]); if (!First) return false;
		FShanmenItemRunInventoryRequest R; R.Context.OwnerId=First->OwnerId; R.Context.RunId=First->RunId;
		R.Context.Content=Candidate.Content; R.ActiveRunId=M.ReservationId;
		R.Context.RequestId=R.MakeRequestId(First->OwnerId,First->RunId,R.ActiveRunId);
		const auto FP=R.Fingerprint();
		if (M.RequestId!=R.Context.RequestId || Pair.Value.Fingerprint!=FP
			|| M.ReceiptId!=MakeReceiptId(M.RequestId,FP,M.Phase,M.Error)) return false;
		for (const auto& Entry:Candidate.ProcessedRequests)
		{
			const auto& E=Entry.Value.Receipt;
			if (!E.IsSuccess() || E.Operation!=EOp::PreparePreparedRunResourceIntent
				|| E.ItemInstanceId!=R.ActiveRunId || E.AuthorityRevision>=M.AuthorityRevision) continue;
			bool Closed=false;
			for (const auto& Final:Candidate.ProcessedRequests) if (Final.Value.Receipt.IsSuccess()
				&& Final.Value.Receipt.Operation==EOp::FinalizePreparedRunResourceIntent
				&& Final.Value.Receipt.ItemInstanceId==E.RequestId
				&& Final.Value.Receipt.AuthorityRevision<M.AuthorityRevision) Closed=true;
			if (!Closed) return false;
		}
		TArray<FGuid> Ids; int64 Total=0;
		for (const auto& Id:Active->ReservationIds)
		{
			const auto* V=Candidate.Reservations.Find(Id); if (!V) return false;
			FName Purpose; FGuid ContainerId; int32 Slot;
			if (!FShanmenItemReservationPlacement::Decode(V->PurposeId,Purpose,ContainerId,Slot)) return false;
			const auto* L=Candidate.Grid.Layouts.FindByPredicate([&](const auto& Layout){return Layout.ContainerId==ContainerId;});
			if (!L) return false;
			if (V->ResourceKind==EShanmenItemResourceKind::DeploymentLock && L->Kind!=EShanmenItemGridKind::Carry) continue;
			if (L->Kind!=EShanmenItemGridKind::Carry) return false;
			Ids.Add(Id); int64 Remaining=V->Amount;
			if (V->ResourceKind==EShanmenItemResourceKind::DeploymentLock) { Total+=Remaining; continue; }
			if (V->ResourceKind!=EShanmenItemResourceKind::Quantity) return false;
			for (const auto& Entry:Candidate.ProcessedRequests)
			{
				const auto& E=Entry.Value.Receipt; if (!E.IsSuccess()) continue;
				const bool Direct=E.Operation==EOp::ConsumePreparedRunItem && E.ReservationId==R.ActiveRunId;
				const bool Intent=(E.Operation==EOp::PreparePreparedRunQuantityIntent || E.Operation==EOp::FinalizePreparedRunQuantityIntent)
					&& !E.ReservationIds.IsEmpty() && E.ReservationIds[0]==R.ActiveRunId;
				if ((Direct || Intent) && E.AuthorityRevision>=M.AuthorityRevision) return false;
				if (E.ItemInstanceId==V->ItemInstanceId && (Direct || (Intent && E.Operation==EOp::FinalizePreparedRunQuantityIntent
					&& E.Phase==EShanmenItemTransactionPhase::Committed))) Remaining-=E.Amount;
				if (E.Operation==EOp::PreparePreparedRunQuantityIntent && Intent)
				{
					bool Closed=false;
					for (const auto& Final:Candidate.ProcessedRequests) if (Final.Value.Receipt.IsSuccess()
						&& Final.Value.Receipt.Operation==EOp::FinalizePreparedRunQuantityIntent
						&& Final.Value.Receipt.ReservationIds==TArray<FGuid>{R.ActiveRunId,E.RequestId}
						&& Final.Value.Receipt.AuthorityRevision<M.AuthorityRevision) Closed=true;
					if (!Closed) return false;
				}
			}
			if (Remaining<0) return false; Total+=Remaining;
		}
		if (Ids!=M.ReservationIds || Total!=M.ResourceBefore) return false;
	}
	return true;
}

FShanmenItemTransactionReceipt FShanmenItemRepository::MaterializeGeneratedSource(const FShanmenItemSourceMaterializeRequest& R)
{
	const auto Id=FShanmenItemSourceMaterializeRequest::MakeRequestId(R.Context.OwnerId,R.ActiveRunId,R.SourceRoleId);
	const auto FP=FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Source.Materialize.Command.r1"),
		{R.Context.OwnerId.ToString(),R.Context.RunId.ToString(),R.ActiveRunId.ToString(),R.SourceRoleId.ToString().ToLower(),
		R.Context.Content.Version.ToString(),R.Context.Content.Digest});
	FShanmenItemTransactionReceipt Replayed;
	if (bInitialized && TryReplay(R.Context.RequestId,FP,EOp::MaterializeGeneratedSource,Replayed)) return Replayed;
	auto Reject=[&](EError E){return MakeRejected(EOp::MaterializeGeneratedSource,R.Context.RequestId,FP,E);};
	if (!bInitialized) return Reject(EError::NotInitialized);
	if (!R.Context.IsValid() || !R.ActiveRunId.IsValid() || R.SourceRoleId.IsNone() || R.Context.RequestId!=Id) return Reject(EError::InvalidRequest);
	if (!IsSameContent(R.Context.Content,State.Content)) return Reject(EError::ContentMismatch);
	const auto Read=ReadGeneratedSource(R.Context.OwnerId,R.ActiveRunId,R.SourceRoleId);
	if (Read.Status!=EShanmenItemGeneratedSourceReadStatus::Accepted || Read.RunState!=EShanmenItemGeneratedSourceRunState::Active) return Reject(EError::RunNotFound);
	FShanmenItemAuthoritySnapshot Projection;
	if (!FShanmenItemRunGridPolicy::Project(CaptureSnapshot(),R.Context.OwnerId,R.Context.RunId,R.ActiveRunId,Projection)) return Reject(EError::RunItemIntentConflict);
	if (State.AuthorityRevision==MAX_int32) return Reject(EError::InvariantViolation);
	const auto& Receipt=Read.Receipt; const auto& Plan=Receipt.GetPlan(); FState Candidate=State;
	if (Candidate.Containers.Contains(Receipt.GetContainerId())) return Reject(EError::InvariantViolation);
	FShanmenItemContainer Container; Container.ContainerId=Receipt.GetContainerId(); Container.OwnerId=R.Context.OwnerId;
	Container.RunId=R.Context.RunId; Container.ContainerType=TEXT("GeneratedSource"); Container.Slots.SetNum(64);
	Candidate.Containers.Add(Container.ContainerId,Container);
	FShanmenItemGridLayout Layout; Layout.ContainerId=Container.ContainerId; Layout.Kind=EShanmenItemGridKind::World; Layout.Width=Layout.Height=8;
	Candidate.Grid.Layouts.Add(Layout);
	for (int32 N=0;N<Plan.Entries.Num();++N)
	{
		const auto& E=Plan.Entries[N]; const auto ItemId=Receipt.GetItemIds()[N];
		if (Candidate.Items.Contains(ItemId)) return Reject(EError::InvariantViolation);
		FShanmenItemInstance Item; Item.ItemInstanceId=ItemId; Item.DefinitionId=E.Definition.DefinitionId;
		Item.OwnerId=R.Context.OwnerId; Item.RunId=R.Context.RunId; Item.Quantity=E.Quantity;
		Item.Durability=E.Definition.MaxDurability; Item.Charges=E.Definition.MaxCharges; Item.RewardMetadata=E.RewardMetadata;
		Item.ParentContainerId=Container.ContainerId; Item.SlotIndex=0; Item.State=EShanmenItemInstanceState::Stored;
		Candidate.Items.Add(ItemId,Item);
		FShanmenItemRepository Preview; Preview.State=Candidate; Preview.bInitialized=true; const auto Geometry=Preview.CaptureSnapshot();
		bool Placed=false;
		for (int32 Y=0;Y<8 && !Placed;++Y) for (int32 X=0;X<8 && !Placed;++X)
			if (FShanmenItemGridPolicy::CanPlace(Geometry,ItemId,Container.ContainerId,X,Y,false)==EError::None)
			{
				auto& PlacedItem=Candidate.Items.FindChecked(ItemId); PlacedItem.SlotIndex=Y*8+X;
				Candidate.Containers.FindChecked(Container.ContainerId).Slots[PlacedItem.SlotIndex]=ItemId; Placed=true;
			}
		if (!Placed) return Reject(EError::GridNoSpace);
	}
	Candidate.Grid.Canonicalize(); ++Candidate.AuthorityRevision;
	FShanmenItemTransactionReceipt Result; Result.bSuccess=true; Result.Operation=EOp::MaterializeGeneratedSource;
	Result.Phase=EShanmenItemTransactionPhase::Committed; Result.RequestId=R.Context.RequestId; Result.ReservationId=R.ActiveRunId;
	Result.ItemInstanceId=Container.ContainerId; Result.ReservationIds=Receipt.GetItemIds(); Result.Amount=Plan.Entries.Num();
	Result.PurposeId=R.SourceRoleId; Result.AuthorityRevision=Candidate.AuthorityRevision;
	Result.ReceiptId=MakeReceiptId(Result.RequestId,FP,Result.Phase,Result.Error); RecordProcessed(Candidate,Result.RequestId,FP,Result);
	EError Error; if (!Result.IsValid() || !ValidateState(Candidate,&Error)) return Reject(EError::InvariantViolation);
	State=MoveTemp(Candidate); return Result;
}

bool FShanmenItemRepository::FinalizeActiveRunGrid(FState& Candidate,const FShanmenItemRunFinalizeRequest& R)
{
	bool HasRunGrid=false;
	for (const auto& P:Candidate.ProcessedRequests) if (P.Value.Receipt.IsSuccess()
		&& (P.Value.Receipt.Operation==EOp::MaterializeGeneratedSource || P.Value.Receipt.Operation==EOp::EditActiveRunGrid
			|| P.Value.Receipt.Operation==EOp::MaterializeRunInventory)
		&& P.Value.Receipt.ReservationId==R.ActiveRunId) { HasRunGrid=true; break; }
	if (!HasRunGrid) return true;
	TSet<FGuid> WorldContainers;
	for (const auto& P:Candidate.GeneratedSources) if (P.Value.RunId==R.ActiveRunId && P.Value.OwnerId==R.Context.OwnerId)
		WorldContainers.Add(Source(P.Value).GetContainerId());
	for (auto& P:Candidate.Items)
	{
		auto& I=P.Value; if (I.State!=EShanmenItemInstanceState::Stored || I.OwnerId!=R.Context.OwnerId || I.RunId!=R.Context.RunId) continue;
		const auto* L=Candidate.Grid.Layouts.FindByPredicate([&](const auto& V){return V.ContainerId==I.ParentContainerId;});
		const bool Lost=WorldContainers.Contains(I.ParentContainerId)
			|| (R.TerminalReason!=EShanmenItemRunTerminalReason::Extraction && L && L->Kind==EShanmenItemGridKind::Carry);
		if (!Lost) continue;
		if (I.Revision==MAX_int32 || I.ChildContainerId.IsValid()) return false;
		auto* C=Candidate.Containers.Find(I.ParentContainerId);
		if (!C || !C->Slots.IsValidIndex(I.SlotIndex) || C->Slots[I.SlotIndex]!=I.ItemInstanceId) return false;
		C->Slots[I.SlotIndex].Invalidate(); I.ParentContainerId.Invalidate(); I.SlotIndex=INDEX_NONE;
		I.Quantity=I.Durability=I.Charges=0; I.State=EShanmenItemInstanceState::Destroyed; ++I.Revision;
		Candidate.Grid.RotatedItems.Remove(I.ItemInstanceId);
	}
	return true;
}
