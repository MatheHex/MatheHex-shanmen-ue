#include "ShanmenDemo20Medicine.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FShanmenOperationContext Context(const FShanmenItemAuthoritySnapshot& S,const FGuid& Intent,const TCHAR* Step)
	{
		FShanmenOperationContext C; C.OwnerId=FShanmenDemo20Catalog::OwnerId(); C.RunId=FShanmenDemo20Catalog::ScopeId(); C.Content=S.Content;
		C.RequestId=FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Medicine.Command.r1"),{Intent.ToString(),Step}); return C;
	}
}

bool FShanmenDemo20Medicine::Capture(const FShanmenItemAuthoritySnapshot& S,const FGuid& Run,
	TArray<FShanmenDemo20MedicineLine>& Out,FString& Reason)
{
	Out.Reset(); FShanmenDemo20ActiveLoadout Active;
	if (!FShanmenDemo20Loadout::InspectActive(S,Active,Reason) || Active.RunId!=Run) return false;
	for (const auto& L:Active.RemainingOriginals)
	{
		const auto* Item=S.Items.FindByPredicate([&](const auto& I){return I.ItemInstanceId==L.ItemInstanceId;});
		if (!Item || Item->DefinitionId!=TEXT("Heal.Pill")) continue;
		Out.Add({Item->ItemInstanceId,L.RemainingQuantity,Item->Revision,EShanmenDemo20MedicineOrigin::PreparedCarry});
	}
	const bool HasSecureEquipment=S.Items.ContainsByPredicate([](const auto& I){return I.OwnerId==FShanmenDemo20Catalog::OwnerId()
		&& I.RunId==FShanmenDemo20Catalog::ScopeId() && I.State==EShanmenItemInstanceState::Stored && I.Quantity==1
		&& I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox"));});
	if (HasSecureEquipment) for (const auto& I:S.Items)
	{
		if (I.DefinitionId==TEXT("Heal.Pill"))
			if (I.OwnerId==FShanmenDemo20Catalog::OwnerId() && I.RunId==FShanmenDemo20Catalog::ScopeId()
				&& I.State==EShanmenItemInstanceState::Stored && I.Quantity>0 && I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Secure")))
				Out.Add({I.ItemInstanceId,I.Quantity,I.Revision,EShanmenDemo20MedicineOrigin::Secure});
	}
	Out.Sort([](const auto& A,const auto& B){return A.Origin==B.Origin ? A.ItemId.ToString()<B.ItemId.ToString() : A.Origin<B.Origin;});
	return true;
}

FGuid FShanmenDemo20Medicine::IntentId(const FShanmenDemo20WorldCheckpoint& C)
{
	if (!C.IsValid() || !C.Medicine.IsSet()) return {};
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Medicine.Intent.r1"),{C.Combat.RunId.ToString(),
		LexToString(C.Combat.Sequence+1),C.Medicine.ItemId.ToString(),LexToString(C.Medicine.ExpectedQuantity),
		LexToString(C.Medicine.ExpectedItemRevision),LexToString(static_cast<uint8>(C.Medicine.Origin))});
}

bool FShanmenDemo20Medicine::BuildIntent(const FShanmenDemo20WorldCheckpoint& C,const FShanmenItemAuthoritySnapshot& S,
	FShanmenDemo20WorldCheckpoint& Out,FString& Reason)
{
	if (!C.IsValid() || C.Medicine.IsSet()) { Reason=TEXT("治疗记录正在确认，不会另开用药请求。"); return false; }
	FShanmenDemo20Session Candidate;
	if (!Candidate.RestoreExpedition(C.Combat) || !Candidate.TryUseMedicine(Reason)) return false;
	TArray<FShanmenDemo20MedicineLine> Lines;
	if (!Capture(S,C.Combat.RunId,Lines,Reason)) return false;
	if (Lines.IsEmpty()) { Reason=TEXT("没有可用回春丹：仓库物品不能在探索中使用，请出发前携带。"); return false; }
	Out=C; Out.Medicine.ItemId=Lines[0].ItemId; Out.Medicine.ExpectedQuantity=Lines[0].Quantity;
	Out.Medicine.ExpectedItemRevision=Lines[0].ItemRevision; Out.Medicine.Origin=Lines[0].Origin;
	return Out.IsValid();
}

bool FShanmenDemo20Medicine::Recover(const FString& Root,FShanmenDemo20WorldCheckpoint& C,
	const FShanmenDemo20MedicinePorts& P,FString& Reason)
{
	if (!C.Medicine.IsSet()) return true;
	auto Fail=[&](){Reason=TEXT("治疗尚未确认，世界已暂停。请重试确认；需要物品档恢复时重启游戏。不会免费治疗或重复扣药。");return false;};
	if (!C.IsValid() || C.Generation<1 || !P.Capture || !P.PrepareRun || !P.FinalizeRun || !P.Reserve || !P.Commit) return Fail();
	FShanmenItemAuthoritySnapshot S; FShanmenDemo20ActiveLoadout Identity;
	if (!P.Capture(S) || !FShanmenDemo20Loadout::InspectActiveIdentity(S,Identity,Reason) || Identity.RunId!=C.Combat.RunId) return Fail();
	const auto* Item=S.Items.FindByPredicate([&](const auto& I){return I.ItemInstanceId==C.Medicine.ItemId;});
	if (!Item || Item->DefinitionId!=TEXT("Heal.Pill") || Item->OwnerId!=FShanmenDemo20Catalog::OwnerId()
		|| Item->RunId!=FShanmenDemo20Catalog::ScopeId()) return Fail();
	const FGuid Id=IntentId(C); const auto PrepareContext=Context(S,Id,TEXT("Prepare"));
	if (C.Medicine.Origin==EShanmenDemo20MedicineOrigin::PreparedCarry)
	{
		FShanmenItemRunQuantityIntentRequest R; R.Context=PrepareContext; R.ActiveRunId=C.Combat.RunId; R.IntentId=Id;
		R.ItemInstanceId=C.Medicine.ItemId; R.Amount=1; R.ExpectedQuantityBefore=C.Medicine.ExpectedQuantity; R.PurposeId=TEXT("Demo20.Medicine.r1");
		if (!P.PrepareRun(R).IsCommandSuccess()) return Fail();
		FShanmenItemRunQuantityIntentFinalizeRequest F; F.Context=Context(S,Id,TEXT("Commit")); F.ActiveRunId=R.ActiveRunId;
		F.PrepareRequestId=R.Context.RequestId; F.IntentId=Id; F.ItemInstanceId=R.ItemInstanceId; F.bCommit=true;
		if (!P.FinalizeRun(F).IsCommandSuccess()) return Fail();
	}
	else
	{
		// Consuming the last secure pill clears placement. Only its exact committed
		// reservation can authorize tombstone recovery; never select a replacement.
		const bool ExactDepleted=S.Reservations.ContainsByPredicate([&](const auto& V){return V.ReserveRequestId==PrepareContext.RequestId
			&& V.ItemInstanceId==Item->ItemInstanceId && V.State==EShanmenItemReservationState::Committed
			&& V.Amount==1 && V.ResourceKind==EShanmenItemResourceKind::Quantity && V.PurposeId==TEXT("Demo20.Medicine.Secure.r1")
			&& V.ItemRevisionAtReserve==C.Medicine.ExpectedItemRevision && V.OwnerId==Item->OwnerId && V.RunId==Item->RunId;});
		if (Item->ParentContainerId!=FShanmenDemo20Catalog::ContainerId(TEXT("Secure"))
			&& !(ExactDepleted && Item->Quantity==0 && Item->State==EShanmenItemInstanceState::Depleted)) return Fail();
		FShanmenItemReserveRequest R; R.Context=PrepareContext; R.ItemInstanceId=Item->ItemInstanceId;
		R.ExpectedItemRevision=C.Medicine.ExpectedItemRevision; R.Amount=1; R.ResourceKind=EShanmenItemResourceKind::Quantity;
		R.PurposeId=TEXT("Demo20.Medicine.Secure.r1");
		const auto Reserved=P.Reserve(R); if (!Reserved.IsCommandSuccess()) return Fail();
		FShanmenItemReservationActionRequest F; F.Context=Context(S,Id,TEXT("Commit")); F.ReservationId=Reserved.Receipt.ReservationId;
		if (!P.Commit(F).IsCommandSuccess()) return Fail();
	}
	FShanmenDemo20Session Healed;
	if (!Healed.RestoreExpedition(C.Combat) || !Healed.TryUseMedicine(Reason)) return Fail();
	auto After=C; After.Medicine={}; if (!Healed.CaptureExpedition(After.Combat)) return Fail();
	if (!FShanmenDemo20WorldCheckpointStore::Save(Root,C,After,Reason)) return Fail();
	Reason.Reset(); return true;
}
