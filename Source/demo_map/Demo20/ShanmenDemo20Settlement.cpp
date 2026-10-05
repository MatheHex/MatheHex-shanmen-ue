#include "ShanmenDemo20Settlement.h"
#include "ShanmenDemo20Catalog.h"

FString FShanmenDemo20Settlement::Describe(const FShanmenItemAuthoritySnapshot& S, const FGuid& Run)
{
	const auto* R = S.RunReports.FindByPredicate([&](const auto& V) { return V.ActiveRunId == Run
		&& V.OwnerId == FShanmenDemo20Catalog::OwnerId() && V.ScopeId == FShanmenDemo20Catalog::ScopeId(); });
	if (!R || !R->IsValid() || !R->IsClosed())
		return TEXT("该局没有完整的物品统计记录（旧版本局或尚未确认）。\n不会从当前库存猜测历史；新局将记录明细。\n仓库仍以已保存的实际物品为准。");
	FString Body;
	auto Section = [&](const TCHAR* Heading, int32 FShanmenItemRunReportLine::*Field)
	{
		Body += Heading; Body += TEXT("\n"); bool Any = false;
		for (const auto& L : R->Lines) if (L.*Field > 0)
		{ Body += FString::Printf(TEXT("  %s × %d\n"), *FShanmenDemo20Catalog::ItemName(L.DefinitionId), L.*Field); Any = true; }
		if (!Any) Body += TEXT("  无\n"); Body += TEXT("\n");
	};
	Section(TEXT("搜寻取得 · 累计取出"), &FShanmenItemRunReportLine::Obtained);
	Section(TEXT("本局消耗"), &FShanmenItemRunReportLine::Consumed);
	if (R->TerminalReason == EShanmenItemRunTerminalReason::Extraction)
	{
		Section(TEXT("已带回 · 含出发携带与安全格"), &FShanmenItemRunReportLine::BroughtBack);
		if (R->Lines.ContainsByPredicate([](const auto& L) { return L.Lost > 0; }))
			Section(TEXT("未带回携带 · 历史局声明的未返还余量"), &FShanmenItemRunReportLine::Lost);
	}
	else
	{
		Section(R->TerminalReason == EShanmenItemRunTerminalReason::Death ? TEXT("死亡损失 · 终局时普通携带与装备")
			: TEXT("放弃损失 · 终局时普通携带与装备"), &FShanmenItemRunReportLine::Lost);
		Section(R->TerminalReason == EShanmenItemRunTerminalReason::Death ? TEXT("死亡保留 · 安全格与护命匣")
			: TEXT("放弃保留 · 安全格与护命匣"), &FShanmenItemRunReportLine::Retained);
	}
	Section(TEXT("场景遗留 · 含未领取与丢弃，未带回"), &FShanmenItemRunReportLine::LeftInWorld);
	Body += TEXT("局外仓库未参与本局扣除，保持原有物品。\n取得是搜寻容器取出操作的累计量，非额外奖励；地面取回不重复计。放回搜寻容器再取出会再次计入操作统计。");
	return Body;
}

FString FShanmenDemo20Settlement::DescribeLatestSaved(const FShanmenItemAuthoritySnapshot& S)
{
	const FShanmenItemRunReport* Latest = nullptr;
	for (const auto& R : S.RunReports)
		if (R.OwnerId == FShanmenDemo20Catalog::OwnerId() && R.ScopeId == FShanmenDemo20Catalog::ScopeId()
			&& R.IsValid() && R.IsClosed() && R.EndRevision <= S.AuthorityRevision
			&& (!Latest || R.EndRevision > Latest->EndRevision)) Latest = &R;
	if (!Latest) return {};
	const TCHAR* Result = Latest->TerminalReason == EShanmenItemRunTerminalReason::Extraction ? TEXT("撤离")
		: Latest->TerminalReason == EShanmenItemRunTerminalReason::Death ? TEXT("死亡") : TEXT("放弃");
	return FString::Printf(TEXT("最近一份已保存物品结算 · %s\n只读历史，不重新结算或发放。\n\n"), Result) + Describe(S,Latest->ActiveRunId);
}
