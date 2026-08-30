#include "demo_mapShanmenFormationInfluenceLifecycleCommandHost.h"

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost& OutHost)
{
	OutHost = Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost();
	if (!ProductHost.IsValid()
		|| !ProductHost.HasInfluenceAuthority()
		|| !ProductHost.GetSession().GetCorrelation().IsValid()
		|| !ProductHost.GetInfluenceLedger().GetLedgerId().IsValid())
	{
		return false;
	}
	OutHost.Correlation = ProductHost.GetSession().GetCorrelation();
	OutHost.LedgerId = ProductHost.GetInfluenceLedger().GetLedgerId();
	return OutHost.IsValid();
}

Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TrySubmit(
	UWorld* World,
	Fdemo_mapShanmenFormationProductHost& ProductHost,
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command)
{
	if (!IsValid())
	{
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Result;
		Result.CommandId = Command.GetCommandId();
		Result.Kind = Command.GetKind();
		Result.Status =
			Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				RouterInvalid;
		Result.Diagnostic =
			TEXT("Influence lifecycle CommandHost is not open or is invalid.");
		return Result;
	}
	if (!Command.IsValid())
	{
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Result;
		Result.CommandId = Command.GetCommandId();
		Result.Kind = Command.GetKind();
		Result.Diagnostic =
			TEXT("Influence lifecycle submission requires one valid command.");
		return Result;
	}
	if (!ProductHost.IsValid())
	{
		return Reject(
			Command,
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::HostInvalid,
			TEXT("Influence lifecycle submission requires one valid ProductHost."));
	}
	if (ProductHost.GetSession().GetCorrelation() != Correlation
		|| Command.GetCorrelation() != Correlation)
	{
		return Reject(
			Command,
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::
				CorrelationMismatch,
			TEXT("Influence lifecycle submission rejected foreign Run identity."));
	}
	if (!ProductHost.HasInfluenceAuthority())
	{
		return Reject(
			Command,
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::LedgerUnavailable,
			TEXT("Influence lifecycle submission requires Host ledger authority."));
	}
	if (ProductHost.GetInfluenceLedger().GetLedgerId() != LedgerId)
	{
		return Reject(
			Command,
			Edemo_mapShanmenFormationInfluenceLifecycleStatus::BindingConflict,
			TEXT("Influence lifecycle submission rejected foreign ledger identity."));
	}

	return Router.TryRoute(World, ProductHost, Command);
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryGetReceipt(
	const FGuid& CommandId,
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord& OutReceipt) const
{
	OutReceipt = Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord();
	return IsValid() && Router.TryGetRecord(CommandId, OutReceipt);
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::IsValid() const
{
	if (!Correlation.IsValid() || !LedgerId.IsValid() || !Router.IsValid())
	{
		return false;
	}
	return !Router.IsBound()
		|| (Router.GetCoordinator().GetBoundCorrelation() == Correlation
			&& Router.GetCoordinator().GetBoundLedgerId() == LedgerId);
}

Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::Reject(
	const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command,
	Edemo_mapShanmenFormationInfluenceLifecycleStatus LifecycleStatus,
	const TCHAR* Diagnostic) const
{
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Result;
	Result.CommandId = Command.GetCommandId();
	Result.Kind = Command.GetKind();
	Result.Status =
		Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
			LifecycleRejected;
	Result.Lifecycle.Status = LifecycleStatus;
	Result.Diagnostic = Diagnostic;
	Result.Lifecycle.Diagnostic = Diagnostic;
	return Result;
}
