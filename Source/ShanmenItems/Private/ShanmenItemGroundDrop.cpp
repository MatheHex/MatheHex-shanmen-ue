#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using EOp=EShanmenItemTransactionOperation;
	using EError=EShanmenItemTransactionError;
	const FShanmenItemTransactionReceipt* Started(const FShanmenItemAuthoritySnapshot& S,const FGuid& Run)
	{
		for (const auto& P:S.ProcessedRequests) if (P.Receipt.IsSuccess() && P.Receipt.ReservationId==Run
			&& (P.Receipt.Operation==EOp::StartPreparedRun || P.Receipt.Operation==EOp::ClaimPreparedRun)) return &P.Receipt;
		return nullptr;
	}
}

FGuid FShanmenItemGroundDropRequest::MakeRequestId(const FGuid& Owner,const FGuid& Scope,const FGuid& Run,const FGuid& Item,int32 Revision)
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.GroundDrop.r1"),
		{Owner.ToString(),Scope.ToString(),Run.ToString(),Item.ToString(),LexToString(Revision)});
}
FGuid FShanmenItemGroundDropRequest::ContainerId() const
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.GroundContainer.r1"),
		{Context.OwnerId.ToString(),Context.RunId.ToString(),ActiveRunId.ToString(),Context.RequestId.ToString()});
}
FGuid FShanmenItemGroundDropRequest::Fingerprint() const
{
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.GroundDrop.Command.r1"),
		{Context.OwnerId.ToString(),Context.RunId.ToString(),Context.RequestId.ToString(),ActiveRunId.ToString(),
		ItemInstanceId.ToString(),Context.Content.Version.ToString(),Context.Content.Digest,
		LexToString(ExpectedAuthorityRevision),LexToString(ExpectedItemRevision),LexToString(Position.X),LexToString(Position.Y),LexToString(Position.Z)});
}
bool FShanmenItemGroundDropPolicy::IsPositionValid(const FIntVector& P)
{
	return P.X>=-1000000 && P.X<=1000000 && P.Y>=-1000000 && P.Y<=1000000 && P.Z>=-1000000 && P.Z<=1000000;
}
FName FShanmenItemGroundDropPolicy::EncodePosition(const FIntVector& P)
{
	return IsPositionValid(P)?FName(*FString::Printf(TEXT("Run.GroundDrop.r1.X%d.Y%d.Z%d"),P.X,P.Y,P.Z)):NAME_None;
}
bool FShanmenItemGroundDropPolicy::DecodePosition(FName Encoded,FIntVector& Out)
{
	Out=FIntVector::ZeroValue; const FString S=Encoded.ToString();
	const FString Prefix=TEXT("Run.GroundDrop.r1.X");
	if (!S.StartsWith(Prefix)) return false;
	const int32 Y=S.Find(TEXT(".Y")),Z=S.Find(TEXT(".Z"));
	FIntVector P;
	if (Y<=Prefix.Len() || Z<=Y+2 || !LexTryParseString(P.X,*S.Mid(Prefix.Len(),Y-Prefix.Len()))
		|| !LexTryParseString(P.Y,*S.Mid(Y+2,Z-Y-2)) || !LexTryParseString(P.Z,*S.Mid(Z+2))
		|| !IsPositionValid(P) || EncodePosition(P).ToString()!=S) return false;
	Out=P; return true;
}
bool FShanmenItemGroundDropPolicy::Read(const FShanmenItemAuthoritySnapshot& S,const FGuid& Owner,const FGuid& Scope,
	const FGuid& Run,TArray<FShanmenItemGroundDropView>& Out)
{
	Out.Reset(); FShanmenItemAuthoritySnapshot Projection;
	if (!FShanmenItemRunGridPolicy::Project(S,Owner,Scope,Run,Projection)) return false;
	for (const auto& P:S.ProcessedRequests)
	{
		const auto& R=P.Receipt;
		if (!R.IsSuccess() || R.Operation!=EOp::DropActiveRunItem || R.ReservationId!=Run) continue;
		FShanmenItemGroundDropView V;
		const auto* I=S.Items.FindByPredicate([&](const auto& Item){return Item.ItemInstanceId==R.ItemInstanceId;});
		if (!I || I->OwnerId!=Owner || I->RunId!=Scope || !R.IsValid() || !DecodePosition(R.PurposeId,V.Position)) { Out.Reset(); return false; }
		V.ContainerId=R.ReservationIds[0];
		const auto* C=S.Containers.FindByPredicate([&](const auto& Container){return Container.ContainerId==V.ContainerId;});
		if (!C || C->OwnerId!=Owner || C->RunId!=Scope || C->ContainerType!=TEXT("GroundDrop")) { Out.Reset(); return false; }
		V.bEmpty=!C->Slots.ContainsByPredicate([](const FGuid& Id){return Id.IsValid();}); Out.Add(V);
	}
	Out.Sort([](const auto& A,const auto& B){return A.ContainerId.ToString()<B.ContainerId.ToString();}); return true;
}

FShanmenItemTransactionReceipt FShanmenItemRepository::DropActiveRunItem(const FShanmenItemGroundDropRequest& R)
{
	const auto FP=R.Fingerprint(); FShanmenItemTransactionReceipt Replay;
	if (bInitialized && TryReplay(R.Context.RequestId,FP,EOp::DropActiveRunItem,Replay)) return Replay;
	auto Reject=[&](EError E){return MakeRejected(EOp::DropActiveRunItem,R.Context.RequestId,FP,E);};
	if (!bInitialized) return Reject(EError::NotInitialized);
	if (!R.Context.IsValid() || !R.ActiveRunId.IsValid() || !R.ItemInstanceId.IsValid()
		|| R.ExpectedAuthorityRevision<0 || R.ExpectedItemRevision<0 || !FShanmenItemGroundDropPolicy::IsPositionValid(R.Position)
		|| R.Context.RequestId!=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId,R.ItemInstanceId,R.ExpectedItemRevision)) return Reject(EError::InvalidRequest);
	if (!IsSameContent(R.Context.Content,State.Content)) return Reject(EError::ContentMismatch);
	if (R.ExpectedAuthorityRevision!=State.AuthorityRevision) return Reject(EError::StaleAuthorityRevision);
	const auto S=CaptureSnapshot(); FShanmenItemAuthoritySnapshot Projection;
	if (!FShanmenItemRunGridPolicy::IsMaterialized(S,R.ActiveRunId)
		|| !FShanmenItemRunGridPolicy::Project(S,R.Context.OwnerId,R.Context.RunId,R.ActiveRunId,Projection)) return Reject(EError::RunItemIntentConflict);
	const auto* I=State.Items.Find(R.ItemInstanceId);
	if (!I) return Reject(EError::ItemNotFound);
	if (I->OwnerId!=R.Context.OwnerId || I->RunId!=R.Context.RunId) return Reject(EError::ScopeMismatch);
	if (I->Revision!=R.ExpectedItemRevision) return Reject(EError::StaleItemRevision);
	const auto* L=State.Grid.Layouts.FindByPredicate([&](const auto& Layout){return Layout.ContainerId==I->ParentContainerId;});
	const auto* Footprint=State.Grid.Footprints.FindByPredicate([&](const auto& F){return F.DefinitionId==I->DefinitionId;});
	if (!L || (L->Kind!=EShanmenItemGridKind::Carry && L->Kind!=EShanmenItemGridKind::Secure) || !Footprint
		|| Footprint->bStorageEquipment || I->ChildContainerId.IsValid() || I->State!=EShanmenItemInstanceState::Stored
		|| I->Revision==MAX_int32 || State.AuthorityRevision==MAX_int32 || Footprint->Width>8 || Footprint->Height>8) return Reject(EError::GridPolicyViolation);
	for (const auto& V:State.Reservations) if (V.Value.ItemInstanceId==I->ItemInstanceId && V.Value.State==EShanmenItemReservationState::Reserved)
		return Reject(EError::RunItemIntentConflict);
	int32 Count=0; for (const auto& P:State.ProcessedRequests) if (P.Value.Receipt.IsSuccess()
		&& P.Value.Receipt.Operation==EOp::DropActiveRunItem && P.Value.Receipt.ReservationId==R.ActiveRunId) ++Count;
	if (Count>=MaxGeneratedSources || State.Containers.Contains(R.ContainerId())) return Reject(EError::InvariantViolation);
	FState Candidate=State;
	FShanmenItemContainer C; C.ContainerId=R.ContainerId(); C.OwnerId=R.Context.OwnerId; C.RunId=R.Context.RunId;
	C.ContainerType=TEXT("GroundDrop"); C.Slots.SetNum(64); C.Slots[0]=I->ItemInstanceId; Candidate.Containers.Add(C.ContainerId,C);
	FShanmenItemGridLayout G; G.ContainerId=C.ContainerId; G.Kind=EShanmenItemGridKind::World; G.Width=G.Height=8; Candidate.Grid.Layouts.Add(G);
	Candidate.Containers.FindChecked(I->ParentContainerId).Slots[I->SlotIndex].Invalidate();
	auto& Moved=Candidate.Items.FindChecked(I->ItemInstanceId); Moved.ParentContainerId=C.ContainerId; Moved.SlotIndex=0; ++Moved.Revision;
	Candidate.Grid.Canonicalize(); ++Candidate.AuthorityRevision;
	FShanmenItemTransactionReceipt Result; Result.bSuccess=true; Result.Operation=EOp::DropActiveRunItem;
	Result.Phase=EShanmenItemTransactionPhase::Committed; Result.RequestId=R.Context.RequestId; Result.ReservationId=R.ActiveRunId;
	Result.ItemInstanceId=I->ItemInstanceId; Result.ReservationIds={C.ContainerId}; Result.Amount=1;
	Result.ResourceBefore=Result.ResourceAfter=Result.AvailableAfter=I->Quantity; Result.ItemRevision=Moved.Revision;
	Result.PurposeId=FShanmenItemGroundDropPolicy::EncodePosition(R.Position); Result.AuthorityRevision=Candidate.AuthorityRevision;
	Result.ReceiptId=MakeReceiptId(Result.RequestId,FP,Result.Phase,Result.Error); RecordProcessed(Candidate,Result.RequestId,FP,Result);
	EError Error; if (!Result.IsValid() || !ValidateState(Candidate,&Error)) return Reject(EError::InvariantViolation);
	State=MoveTemp(Candidate); return Result;
}

bool FShanmenItemRepository::ValidateGroundDrops(const FState& Candidate)
{
	FShanmenItemAuthoritySnapshot S; Candidate.Reservations.GenerateValueArray(S.Reservations); Candidate.ProcessedRequests.GenerateValueArray(S.ProcessedRequests);
	TSet<FGuid> Containers;
	for (const auto& P:Candidate.ProcessedRequests)
	{
		const auto& D=P.Value.Receipt; if (!D.IsSuccess() || D.Operation!=EOp::DropActiveRunItem) continue;
		if (!D.IsValid()) return false;
		const auto* I=Candidate.Items.Find(D.ItemInstanceId); const auto* A=Started(S,D.ReservationId);
		const auto* First=A && !A->ReservationIds.IsEmpty()?Candidate.Reservations.Find(A->ReservationIds[0]):nullptr;
		if (!I || !First || I->OwnerId!=First->OwnerId || I->RunId!=First->RunId || I->Revision<D.ItemRevision || D.AuthorityRevision<=A->AuthorityRevision) return false;
		bool Unified=false;
		for (const auto& Entry:Candidate.ProcessedRequests)
		{
			const auto& R=Entry.Value.Receipt; if (!R.IsSuccess() || R.ReservationId!=D.ReservationId) continue;
			if (R.Operation==EOp::FinalizePreparedRun && R.AuthorityRevision<=D.AuthorityRevision) return false;
			if (R.Operation==EOp::MaterializeRunInventory && R.AuthorityRevision<D.AuthorityRevision) Unified=true;
		}
		if (!Unified) return false;
		FShanmenItemGroundDropRequest R; R.Context.OwnerId=First->OwnerId; R.Context.RunId=First->RunId; R.Context.Content=Candidate.Content;
		R.Context.RequestId=D.RequestId; R.ActiveRunId=D.ReservationId; R.ItemInstanceId=D.ItemInstanceId;
		R.ExpectedAuthorityRevision=D.AuthorityRevision-1; R.ExpectedItemRevision=D.ItemRevision-1;
		if (!FShanmenItemGroundDropPolicy::DecodePosition(D.PurposeId,R.Position)) return false;
		const auto FP=R.Fingerprint(); const FGuid Id=R.ContainerId(); const auto* C=Candidate.Containers.Find(Id);
		const auto* L=Candidate.Grid.Layouts.FindByPredicate([&](const auto& Layout){return Layout.ContainerId==Id;});
		if (D.RequestId!=R.MakeRequestId(First->OwnerId,First->RunId,R.ActiveRunId,R.ItemInstanceId,R.ExpectedItemRevision)
			|| FP!=P.Value.Fingerprint || D.ReceiptId!=MakeReceiptId(D.RequestId,FP,D.Phase,D.Error)
			|| D.ReservationIds[0]!=Id || Containers.Contains(Id) || !C || C->OwnerId!=First->OwnerId || C->RunId!=First->RunId
			|| C->ContainerType!=TEXT("GroundDrop") || C->Slots.Num()!=64 || !L || L->Kind!=EShanmenItemGridKind::World || L->Width!=8 || L->Height!=8) return false;
		if (I->Revision==D.ItemRevision && (I->ParentContainerId!=Id || I->SlotIndex!=0 || I->Quantity!=D.ResourceAfter || I->State!=EShanmenItemInstanceState::Stored)) return false;
		Containers.Add(Id);
	}
	for (const auto& C:Candidate.Containers) if (C.Value.ContainerType==TEXT("GroundDrop") && !Containers.Contains(C.Key)) return false;
	return true;
}
