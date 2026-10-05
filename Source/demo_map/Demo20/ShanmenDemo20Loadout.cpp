#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"

namespace
{
	const FName OrdinaryPurpose(TEXT("Demo20.Expedition.Ordinary.r1"));
	bool Finished(const FShanmenItemAuthoritySnapshot& S, FGuid Run)
	{
		return S.ProcessedRequests.ContainsByPredicate([&](const auto& P) { return P.Receipt.IsSuccess()
			&& P.Receipt.Operation == EShanmenItemTransactionOperation::FinalizePreparedRun && P.Receipt.ReservationId == Run; });
	}
	bool HasActive(const FShanmenItemAuthoritySnapshot& S)
	{
		return S.ProcessedRequests.ContainsByPredicate([&](const auto& P) { return P.Receipt.IsSuccess()
			&& (P.Receipt.Operation == EShanmenItemTransactionOperation::StartPreparedRun
				|| P.Receipt.Operation == EShanmenItemTransactionOperation::ClaimPreparedRun) && !Finished(S, P.Receipt.ReservationId); });
	}
}

bool FShanmenDemo20Loadout::Build(const FShanmenItemAuthoritySnapshot& S, FShanmenItemLoadoutStartRequest& Out, FString& Reason)
{
	Out = {}; Reason.Reset();
	FShanmenItemRepository Check;
	if (!Check.TryLoadSnapshot(S)) { Reason = TEXT("物品权威不可用，请先恢复存档。"); return false; }
	if (HasActive(S)) { Reason = TEXT("有未结算探索：整备已锁定，不会覆盖原局。请返回入口继续原局。"); return false; }
	for (FName Role : {FName(TEXT("Weapon")), FName(TEXT("Armor")), FName(TEXT("Backpack"))})
	{
		if (!S.Items.ContainsByPredicate([&](const auto& I) { return I.OwnerId == FShanmenDemo20Catalog::OwnerId()
			&& I.RunId == FShanmenDemo20Catalog::ScopeId() && I.State == EShanmenItemInstanceState::Stored
			&& I.Quantity == 1 && I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(Role); }))
		{ Reason = TEXT("出发条件：请先装备") + FShanmenDemo20Catalog::ContainerName(Role) + TEXT("。"); return false; }
	}
	FShanmenItemLoadoutStartRequest R;
	R.Context.OwnerId = FShanmenDemo20Catalog::OwnerId(); R.Context.RunId = FShanmenDemo20Catalog::ScopeId(); R.Context.Content = S.Content;
	R.ExpectedAuthorityRevision = S.AuthorityRevision;
	R.Context.RequestId = FShanmenItemLoadoutStartRequest::MakeRequestId(R.Context, R.ExpectedAuthorityRevision);
	for (const auto& I : S.Items)
	{
		if (I.OwnerId != R.Context.OwnerId || I.RunId != R.Context.RunId || I.State != EShanmenItemInstanceState::Stored) continue;
		const bool Equipped = I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Weapon"))
			|| I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Armor"))
			|| I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Backpack"));
		if (!Equipped && I.ParentContainerId != FShanmenDemo20Catalog::ContainerId(TEXT("Carry"))) continue;
		const auto* D = S.Definitions.FindByPredicate([&](const auto& V) { return V.DefinitionId == I.DefinitionId; });
		if (!D || I.Quantity < 1) { Reason = TEXT("携带物品定义不完整，出发已拒绝。"); return false; }
		FShanmenItemReserveRequest L; L.Context = R.Context;
		L.Context.RequestId = FShanmenItemLoadoutStartRequest::MakeLineRequestId(R.Context.RequestId, I.ItemInstanceId);
		L.ItemInstanceId = I.ItemInstanceId; L.ExpectedItemRevision = I.Revision;
		L.ResourceKind = D->Supports(EShanmenItemResourceKind::DeploymentLock)
			? EShanmenItemResourceKind::DeploymentLock : EShanmenItemResourceKind::Quantity;
		L.Amount = L.ResourceKind == EShanmenItemResourceKind::DeploymentLock ? 1 : I.Quantity;
		L.PurposeId = FShanmenItemReservationPlacement::Encode(OrdinaryPurpose, I.ParentContainerId, I.SlotIndex);
		R.Lines.Add(L);
	}
	R.Lines.Sort([](const auto& A, const auto& B) { return A.ItemInstanceId.ToString() < B.ItemInstanceId.ToString(); });
	Out = MoveTemp(R); return true;
}

bool FShanmenDemo20Loadout::InspectActiveIdentity(const FShanmenItemAuthoritySnapshot& S, FShanmenDemo20ActiveLoadout& Out, FString& Reason)
{
	Out = {}; Reason.Reset(); FShanmenItemRepository Check;
	if (!Check.TryLoadSnapshot(S)) { Reason = TEXT("存档权威校验失败。"); return false; }
	const FShanmenItemTransactionReceipt* Active = nullptr;
	for (const auto& P : S.ProcessedRequests)
	{
		if (!P.Receipt.IsSuccess() || P.Receipt.Operation != EShanmenItemTransactionOperation::StartPreparedRun
			|| Finished(S, P.Receipt.ReservationId)) continue;
		if (Active) { Reason = TEXT("探索身份冲突，需要恢复。"); return false; }
		Active = &P.Receipt;
	}
	if (!Active) { Reason = TEXT("没有未结算的正式探索。"); return false; }
	Out.RunId=Active->ReservationId; Out.StartRequestId=Active->RequestId;
	const FGuid Seed=FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Expedition.RunSeed.r1"),{Out.RunId.ToString()});
	Out.RunSeed=(static_cast<uint64>(Seed.A)<<32)|Seed.B; return true;
}

bool FShanmenDemo20Loadout::InspectActive(const FShanmenItemAuthoritySnapshot& S, FShanmenDemo20ActiveLoadout& Out, FString& Reason)
{
	Out={};
	FShanmenDemo20ActiveLoadout Identity;
	if (!InspectActiveIdentity(S,Identity,Reason)) { Out={}; return false; }
	const auto* ActiveProcessed=S.ProcessedRequests.FindByPredicate([&](const auto& P){return P.Receipt.RequestId==Identity.StartRequestId;});
	const auto* Active=&ActiveProcessed->Receipt;
	for (const auto& P : S.ProcessedRequests)
	{
		const auto& Prepare = P.Receipt;
		const bool Quantity = Prepare.Operation == EShanmenItemTransactionOperation::PreparePreparedRunQuantityIntent
			&& Prepare.ReservationIds.Num() == 1 && Prepare.ReservationIds[0] == Active->ReservationId;
		const bool Resource = Prepare.Operation == EShanmenItemTransactionOperation::PreparePreparedRunResourceIntent
			&& Prepare.ItemInstanceId == Active->ReservationId;
		if (!Prepare.IsSuccess() || (!Quantity && !Resource)) continue;
		const bool Resolved = S.ProcessedRequests.ContainsByPredicate([&](const auto& V)
		{
			const auto& Final = V.Receipt;
			return Final.IsSuccess() && (Quantity
				? Final.Operation == EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
					&& Final.ReservationIds.Num() == 2 && Final.ReservationIds[0] == Active->ReservationId && Final.ReservationIds[1] == Prepare.RequestId
				: Final.Operation == EShanmenItemTransactionOperation::FinalizePreparedRunResourceIntent && Final.ItemInstanceId == Prepare.RequestId);
		});
		if (!Resolved) { Reason = TEXT("物品使用结果尚待恢复，不会返还未确认的消耗。"); return false; }
	}
	FShanmenDemo20ActiveLoadout R; R.RunId = Active->ReservationId; R.StartRequestId = Active->RequestId;
	const FGuid Seed = FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Expedition.RunSeed.r1"), {R.RunId.ToString()});
	R.RunSeed = (static_cast<uint64>(Seed.A) << 32) | Seed.B;
	for (const auto& Id : Active->ReservationIds)
	{
		const auto* V = S.Reservations.FindByPredicate([&](const auto& Value) { return Value.ReservationId == Id; });
		FName Purpose; FGuid Container; int32 Slot = INDEX_NONE;
		if (!V || V->OwnerId != FShanmenDemo20Catalog::OwnerId() || V->RunId != FShanmenDemo20Catalog::ScopeId()
			|| !FShanmenItemReservationPlacement::Decode(V->PurposeId, Purpose, Container, Slot) || Purpose != OrdinaryPurpose)
		{ Reason = TEXT("正式探索携带来源不匹配，禁止生成替代清单。"); return false; }
		int32 Remaining = V->Amount;
		if (FShanmenItemRunGridPolicy::IsTransferred(S,R.RunId,Id)) continue;
		if (V->ResourceKind == EShanmenItemResourceKind::Quantity)
			for (const auto& P : S.ProcessedRequests)
			{
				const auto& Receipt = P.Receipt;
				if (!Receipt.IsSuccess() || Receipt.ItemInstanceId != V->ItemInstanceId) continue;
				const bool Direct = Receipt.Operation == EShanmenItemTransactionOperation::ConsumePreparedRunItem && Receipt.ReservationId == R.RunId;
				const bool Intent = Receipt.Operation == EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
					&& Receipt.Phase == EShanmenItemTransactionPhase::Committed && Receipt.ReservationIds.Num() == 2 && Receipt.ReservationIds[0] == R.RunId;
				if (Direct || Intent) Remaining -= Receipt.Amount;
			}
		if (Remaining < 0) { Reason = TEXT("携带余额冲突，需要恢复。"); return false; }
		if (Remaining > 0) { FShanmenItemRunSecuredOriginal L; L.ItemInstanceId = V->ItemInstanceId; L.RemainingQuantity = Remaining; R.RemainingOriginals.Add(L); }
	}
	Out = MoveTemp(R); return true;
}

FString FShanmenDemo20Loadout::Summary(const FShanmenItemAuthoritySnapshot& S)
{
	FShanmenItemLoadoutStartRequest R; FString Reason;
	if (!Build(S, R, Reason)) return Reason;
	int32 Equipment = 0, Ordinary = 0, Secure = 0;
	for (const auto& L : R.Lines) { if (L.ResourceKind == EShanmenItemResourceKind::DeploymentLock) Equipment += L.Amount; else Ordinary += L.Amount; }
	for (const auto& I : S.Items) if (I.OwnerId == R.Context.OwnerId && I.RunId == R.Context.RunId
		&& I.State == EShanmenItemInstanceState::Stored && I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Secure"))) Secure += I.Quantity;
	return FString::Printf(TEXT("携带确认：%d 件普通装备 / %d 件物资\n安全格 %d 件保留；仓库与灵石留在局外。"), Equipment, Ordinary, Secure);
}
