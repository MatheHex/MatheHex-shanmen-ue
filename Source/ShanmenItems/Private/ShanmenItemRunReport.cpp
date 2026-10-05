#include "ShanmenItemRepository.h"

namespace
{
	using EOp = EShanmenItemTransactionOperation;
	using EKind = EShanmenItemGridKind;
	using EReason = EShanmenItemRunTerminalReason;

	FShanmenItemRunReportLine& Line(FShanmenItemRunReport& Report, FName Definition)
	{
		if (auto* Found = Report.Lines.FindByPredicate([&](const auto& L) { return L.DefinitionId == Definition; })) return *Found;
		FShanmenItemRunReportLine New; New.DefinitionId = Definition; Report.Lines.Add(New);
		return Report.Lines.Last();
	}
	bool Add(int32& Total, int32 Amount)
	{
		if (Amount < 0 || Total > MAX_int32 - Amount) return false;
		Total += Amount; return true;
	}
	void Sort(FShanmenItemRunReport& Report)
	{ Report.Lines.Sort([](const auto& A, const auto& B) { return A.DefinitionId.LexicalLess(B.DefinitionId); }); }
}

bool FShanmenItemRunReportLine::operator==(const FShanmenItemRunReportLine& O) const
{
	return DefinitionId == O.DefinitionId && Obtained == O.Obtained && Consumed == O.Consumed
		&& BroughtBack == O.BroughtBack && Lost == O.Lost && Retained == O.Retained && LeftInWorld == O.LeftInWorld;
}

bool FShanmenItemRunReport::IsValid() const
{
	if (!ActiveRunId.IsValid() || !OwnerId.IsValid() || !ScopeId.IsValid() || StartRevision < 1 || Lines.Num() > 65536) return false;
	if (IsClosed())
	{
		if (EndRevision <= StartRevision || !TerminalRequestId.IsValid()
			|| (TerminalReason != EReason::Extraction && TerminalReason != EReason::Death && TerminalReason != EReason::Abandon)) return false;
	}
	else if (TerminalRequestId.IsValid() || TerminalReason != EReason::None) return false;
	FName Previous;
	for (const auto& L : Lines)
	{
		if (L.DefinitionId.IsNone() || (!Previous.IsNone() && !Previous.LexicalLess(L.DefinitionId))
			|| L.Obtained < 0 || L.Consumed < 0 || L.BroughtBack < 0 || L.Lost < 0 || L.Retained < 0 || L.LeftInWorld < 0
			|| !(L.Obtained || L.Consumed || L.BroughtBack || L.Lost || L.Retained || L.LeftInWorld)) return false;
		if (!IsClosed() && (L.Consumed || L.BroughtBack || L.Lost || L.Retained || L.LeftInWorld)) return false;
		if (TerminalReason != EReason::Extraction && L.BroughtBack) return false;
		Previous = L.DefinitionId;
	}
	return true;
}

bool FShanmenItemRunReport::operator==(const FShanmenItemRunReport& O) const
{
	return ActiveRunId == O.ActiveRunId && OwnerId == O.OwnerId && ScopeId == O.ScopeId
		&& StartRevision == O.StartRevision && EndRevision == O.EndRevision && TerminalRequestId == O.TerminalRequestId
		&& TerminalReason == O.TerminalReason && Lines == O.Lines;
}

bool FShanmenItemRepository::BeginRunReport(FState& Candidate, const FShanmenItemTransactionReceipt& Start,
	const FShanmenOperationContext& Context)
{
	if (Candidate.Grid.IsEmpty()) return true; // Legacy slot Runs keep exact historical behavior.
	if (Candidate.RunReports.Num() >= 4096 || Candidate.RunReports.Contains(Start.ReservationId)) return false;
	FShanmenItemRunReport R; R.ActiveRunId = Start.ReservationId; R.OwnerId = Context.OwnerId; R.ScopeId = Context.RunId;
	R.StartRevision = Start.AuthorityRevision; Candidate.RunReports.Add(R.ActiveRunId, R); return R.IsValid();
}

bool FShanmenItemRepository::AccumulateRunPickup(FState& Candidate, const FGuid& Run,
	const FShanmenItemInstance& Source, const FGuid& Destination, int32 Amount)
{
	auto* Report = Candidate.RunReports.Find(Run);
	if (!Report) return true; // Old active Runs have incomplete history, never invent a total.
	if (Report->IsClosed()) return false;
	const auto* To = Candidate.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == Destination; });
	if (!To || (To->Kind != EKind::Carry && To->Kind != EKind::Secure)) return true;
	bool FromSearch = false;
	for (const auto& P : Candidate.ProcessedRequests)
	{
		const auto& R = P.Value.Receipt;
		if (R.IsSuccess() && R.Operation == EOp::MaterializeGeneratedSource && R.ReservationId == Run
			&& R.ItemInstanceId == Source.ParentContainerId) { FromSearch = true; break; }
	}
	if (!FromSearch) return true; // Ground reclaim and Carry <-> Secure are not new acquisitions.
	if (Source.OwnerId != Report->OwnerId || Source.RunId != Report->ScopeId || Amount < 1) return false;
	if (!Add(Line(*Report, Source.DefinitionId).Obtained, Amount)) return false;
	Sort(*Report); return Report->IsValid();
}

bool FShanmenItemRepository::CloseRunReport(FState& Candidate, const FShanmenItemAuthoritySnapshot& Before,
	const FShanmenItemRunFinalizeRequest& Request, const FShanmenItemTransactionReceipt& Terminal)
{
	auto* Report = Candidate.RunReports.Find(Request.ActiveRunId);
	if (!Report) return true; // Schema <= 6 Runs explicitly have no retrospective settlement facts.
	if (Report->IsClosed()) return false;
	FShanmenItemAuthoritySnapshot Projected;
	if (!FShanmenItemRunGridPolicy::Project(Before, Report->OwnerId, Report->ScopeId, Report->ActiveRunId, Projected)) return false;
	TSet<FGuid> CurrentWorldContainers;
	for (const auto& P : Before.ProcessedRequests)
	{
		const auto& R = P.Receipt;
		if (!R.IsSuccess() || R.ReservationId != Report->ActiveRunId) continue;
		if (R.Operation == EOp::MaterializeGeneratedSource) CurrentWorldContainers.Add(R.ItemInstanceId);
		if (R.Operation == EOp::DropActiveRunItem) for (const auto& Id : R.ReservationIds) CurrentWorldContainers.Add(Id);
	}
	for (const auto& I : Projected.Items)
	{
		if (I.OwnerId != Report->OwnerId || I.RunId != Report->ScopeId || I.Quantity < 1) continue;
		const auto* Layout = Projected.Grid.Layouts.FindByPredicate([&](const auto& L) { return L.ContainerId == I.ParentContainerId; });
		if (!Layout || Layout->Kind == EKind::Stash) continue; // Warehouse is untouched, not brought-back reward.
		const auto* After = Candidate.Items.Find(I.ItemInstanceId); if (!After) return false;
		if (Layout->Kind == EKind::World)
		{
			if (CurrentWorldContainers.Contains(I.ParentContainerId)
				&& !Add(Line(*Report, I.DefinitionId).LeftInWorld, I.Quantity)) return false;
			continue; // Unrelated/older World containers are not this Run's scene remainder.
		}
		auto& L = Line(*Report, I.DefinitionId);
		if (After->State == EShanmenItemInstanceState::Destroyed)
		{
			if (After->Quantity != 0 || !Add(L.Lost, I.Quantity)) return false;
		}
		else if (Request.TerminalReason == EReason::Extraction)
		{
			// Legacy, non-materialized grid Runs may explicitly return less than the
			// prepared remainder. Preserve that existing contract, and record the
			// unreturned amount separately; modern graph Runs return their full balance.
			if (After->Quantity < 0 || After->Quantity > I.Quantity
				|| !Add(L.BroughtBack, After->Quantity) || !Add(L.Lost, I.Quantity - After->Quantity)) return false;
		}
		else if (After->Quantity != I.Quantity || !Add(L.Retained, After->Quantity)) return false;
	}
	// Only committed quantity uses after this Run's claim. Start itself debits
	// prepared stock but is NOT consumption; reserves/cancels/replays count zero.
	for (const auto& P : Before.ProcessedRequests)
	{
		const auto& R = P.Receipt;
		if (!R.IsSuccess() || R.AuthorityRevision <= Report->StartRevision || R.AuthorityRevision >= Terminal.AuthorityRevision) continue;
		const bool Direct = R.Operation == EOp::ConsumePreparedRunItem && R.ReservationId == Report->ActiveRunId;
		const bool Intent = R.Operation == EOp::FinalizePreparedRunQuantityIntent && R.Phase == EShanmenItemTransactionPhase::Committed
			&& R.ReservationIds.Num() == 2 && R.ReservationIds[0] == Report->ActiveRunId;
		const bool Commit = R.Operation == EOp::Commit && R.ResourceKind == EShanmenItemResourceKind::Quantity;
		if (!Direct && !Intent && !Commit) continue;
		const auto* I = Candidate.Items.Find(R.ItemInstanceId);
		if (!I) return false;
		if (I->OwnerId != Report->OwnerId || I->RunId != Report->ScopeId)
		{ if (Commit) continue; return false; } // Concurrent scopes' generic commits belong to their own audit.
		if (!Add(Line(*Report, I->DefinitionId).Consumed, R.Amount)) return false;
	}
	Report->EndRevision = Terminal.AuthorityRevision; Report->TerminalRequestId = Terminal.RequestId;
	Report->TerminalReason = Request.TerminalReason; Sort(*Report); return Report->IsValid();
}

bool FShanmenItemRepository::ValidateRunReports(const FState& Candidate)
{
	if (Candidate.RunReports.Num() > 4096) return false;
	for (const auto& P : Candidate.RunReports)
	{
		const auto& R = P.Value;
		if (P.Key != R.ActiveRunId || !R.IsValid() || R.StartRevision > Candidate.AuthorityRevision
			|| (R.IsClosed() && R.EndRevision > Candidate.AuthorityRevision)) return false;
		const FShanmenItemTransactionReceipt* Start = nullptr; const FShanmenItemTransactionReceipt* End = nullptr;
		for (const auto& E : Candidate.ProcessedRequests)
		{
			const auto& Receipt = E.Value.Receipt; if (!Receipt.IsSuccess() || Receipt.ReservationId != R.ActiveRunId) continue;
			if (Receipt.Operation == EOp::StartPreparedRun || Receipt.Operation == EOp::ClaimPreparedRun) Start = &Receipt;
			if (Receipt.Operation == EOp::FinalizePreparedRun) End = &Receipt;
		}
		const auto* First = Start && !Start->ReservationIds.IsEmpty() ? Candidate.Reservations.Find(Start->ReservationIds[0]) : nullptr;
		if (!First || First->OwnerId != R.OwnerId || First->RunId != R.ScopeId || Start->AuthorityRevision != R.StartRevision
			|| R.IsClosed() != (End != nullptr)) return false;
		if (End)
		{
			const FName Purpose = R.TerminalReason == EReason::Extraction ? FShanmenItemRunLifecyclePurpose::Extraction()
				: R.TerminalReason == EReason::Death ? FShanmenItemRunLifecyclePurpose::Death() : FShanmenItemRunLifecyclePurpose::Abandon();
			if (End->RequestId != R.TerminalRequestId || End->AuthorityRevision != R.EndRevision || End->PurposeId != Purpose) return false;
			TMap<FName, int64> Consumed;
			for (const auto& E : Candidate.ProcessedRequests)
			{
				const auto& Use = E.Value.Receipt;
				if (!Use.IsSuccess() || Use.AuthorityRevision <= R.StartRevision || Use.AuthorityRevision >= R.EndRevision) continue;
				const bool Direct = Use.Operation == EOp::ConsumePreparedRunItem && Use.ReservationId == R.ActiveRunId;
				const bool Intent = Use.Operation == EOp::FinalizePreparedRunQuantityIntent && Use.Phase == EShanmenItemTransactionPhase::Committed
					&& Use.ReservationIds.Num() == 2 && Use.ReservationIds[0] == R.ActiveRunId;
				const bool Commit = Use.Operation == EOp::Commit && Use.ResourceKind == EShanmenItemResourceKind::Quantity;
				if (!Direct && !Intent && !Commit) continue;
				const auto* I = Candidate.Items.Find(Use.ItemInstanceId);
				if (!I) return false;
				if (I->OwnerId != R.OwnerId || I->RunId != R.ScopeId) { if (Commit) continue; return false; }
				Consumed.FindOrAdd(I->DefinitionId) += Use.Amount;
			}
			for (const auto& L : R.Lines)
			{
				if (L.Consumed != Consumed.FindRef(L.DefinitionId)) return false;
				Consumed.Remove(L.DefinitionId);
			}
			if (!Consumed.IsEmpty()) return false;
		}
		for (const auto& L : R.Lines) if (!Candidate.Definitions.Contains(L.DefinitionId)) return false;
	}
	return true;
}
