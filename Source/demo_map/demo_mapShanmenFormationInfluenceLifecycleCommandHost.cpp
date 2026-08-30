#include "demo_mapShanmenFormationInfluenceLifecycleCommandHost.h"

namespace
{
	bool LeaseSnapshotsMatch(
		const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& Left,
		const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.LeaseId == Right.LeaseId
			&& Left.ApplyIntentId == Right.ApplyIntentId
			&& Left.Key.Matches(Right.Key)
			&& Left.EvaluationReceipt.Matches(Right.EvaluationReceipt);
	}
}

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
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost Candidate;
	Candidate.Correlation = ProductHost.GetSession().GetCorrelation();
	Candidate.LedgerId = ProductHost.GetInfluenceLedger().GetLedgerId();
	if (!Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
			ProductHost, Candidate.ConsumerRuntime)
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutHost = MoveTemp(Candidate);
	return true;
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

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord Existing;
	if (Router.TryGetRecord(Command.GetCommandId(), Existing))
	{
		return Router.TryRouteWithConsumers(
			World, ProductHost, ConsumerRuntime, Command);
	}

	if (Command.GetKind()
		== Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep)
	{
		Fdemo_mapShanmenFormationInfluenceIntent Pending;
		if (ProductHost.TryPeekNextInfluenceIntent(Pending)
			&& Pending.IntentId
				== Command.GetStepRequest().ExpectedIntentId
			&& Pending.Operation
				== Edemo_mapShanmenFormationInfluenceOperation::Remove)
		{
			Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
			Fdemo_mapShanmenFormationInfluenceLeaseSnapshot Lease;
			const auto& Executor = Router.GetCoordinator().
				GetExecutionService().GetRuntime().GetExecutor();
			if (Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
					Pending, Key)
				&& Executor.TryGetActiveLease(Key, Lease))
			{
				const int32 ActiveConsumers = ConsumerRuntime.GetBridge().
					GetCommandHost().GetActiveApplicationCountForLease(
						Lease.LeaseId);
				if (ActiveConsumers == INDEX_NONE)
				{
					auto Result = Reject(
						Command,
						Edemo_mapShanmenFormationInfluenceLifecycleStatus::
							StateInvalid,
						TEXT("Consumer lease-order query is inconsistent."));
					Result.Lifecycle.bConsumerLeaseOrderChecked = true;
					Result.Lifecycle.ConsumerLeaseId = Lease.LeaseId;
					return Result;
				}
				if (ActiveConsumers > 0)
				{
					auto Result = Reject(
						Command,
						Edemo_mapShanmenFormationInfluenceLifecycleStatus::
							ConsumerDeactivateRequired,
						TEXT("Native consumers must be explicitly removed before their authoritative lease Remove."));
					Result.Lifecycle.bConsumerLeaseOrderChecked = true;
					Result.Lifecycle.ConsumerLeaseId = Lease.LeaseId;
					Result.Lifecycle.ActiveConsumerApplicationCount =
						ActiveConsumers;
					return Result;
				}
			}
		}
	}

	return Router.TryRouteWithConsumers(
		World, ProductHost, ConsumerRuntime, Command);
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::
TryActivateConsumer(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	const FGuid& SubjectEntityId,
	Udemo_mapAttributeComponent* AttributeComponent,
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& ApplyCommand)
{
	if (!IsValid())
	{
		Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult Result;
		Result.Diagnostic =
			TEXT("Influence lifecycle CommandHost is not open or is invalid.");
		return Result;
	}
	if (!ApplyCommand.IsValid()
		|| ApplyCommand.GetOperation()
			!= Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply)
	{
		return ConsumerRuntime.TryActivate(
			ProductHost, SubjectEntityId, AttributeComponent, ApplyCommand);
	}
	if (IsCompletedConsumerCommand(ApplyCommand))
	{
		auto Result = ConsumerRuntime.TryActivate(
			ProductHost, SubjectEntityId, AttributeComponent, ApplyCommand);
		Result.bLeaseAuthorityChecked = true;
		Result.AuthoritativeLease = ApplyCommand.GetProjection().GetLease();
		return Result;
	}
	if (!Router.IsBound())
	{
		return RejectConsumer(
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				LeaseAuthorityUnavailable,
			TEXT("Consumer activation requires this CommandHost's bound lease authority."));
	}
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot AuthoritativeLease;
	if (!TryGetAuthoritativeActiveLease(ApplyCommand, AuthoritativeLease))
	{
		return RejectConsumer(
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				LeaseNotActive,
			TEXT("Consumer activation requires the exact active lease owned by this CommandHost."));
	}
	if (!LeaseSnapshotsMatch(
			AuthoritativeLease, ApplyCommand.GetProjection().GetLease()))
	{
		auto Result = RejectConsumer(
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				LeaseIdentityMismatch,
			TEXT("Consumer activation command does not match authoritative lease evidence."));
		Result.bLeaseAuthorityChecked = true;
		Result.AuthoritativeLease = AuthoritativeLease;
		return Result;
	}
	auto Result = ConsumerRuntime.TryActivate(
		ProductHost, SubjectEntityId, AttributeComponent, ApplyCommand);
	Result.bLeaseAuthorityChecked = true;
	Result.AuthoritativeLease = AuthoritativeLease;
	return Result;
}

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::
TryDeactivateConsumer(
	const Fdemo_mapShanmenFormationProductHost& ProductHost,
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& RemoveCommand)
{
	if (!IsValid())
	{
		Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult Result;
		Result.Diagnostic =
			TEXT("Influence lifecycle CommandHost is not open or is invalid.");
		return Result;
	}
	if (!RemoveCommand.IsValid()
		|| RemoveCommand.GetOperation()
			!= Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove)
	{
		return ConsumerRuntime.TryDeactivate(ProductHost, RemoveCommand);
	}
	if (IsCompletedConsumerCommand(RemoveCommand))
	{
		auto Result = ConsumerRuntime.TryDeactivate(
			ProductHost, RemoveCommand);
		Result.bLeaseAuthorityChecked = true;
		Result.AuthoritativeLease = RemoveCommand.GetProjection().GetLease();
		return Result;
	}
	if (!Router.IsBound())
	{
		return RejectConsumer(
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				LeaseAuthorityUnavailable,
			TEXT("Consumer deactivation requires this CommandHost's bound lease authority."));
	}
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot AuthoritativeLease;
	if (!TryGetAuthoritativeActiveLease(RemoveCommand, AuthoritativeLease))
	{
		return RejectConsumer(
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				LeaseNotActive,
			TEXT("Consumer deactivation must precede authoritative lease removal."));
	}
	if (!LeaseSnapshotsMatch(
			AuthoritativeLease, RemoveCommand.GetProjection().GetLease()))
	{
		auto Result = RejectConsumer(
			Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
				LeaseIdentityMismatch,
			TEXT("Consumer deactivation command does not match authoritative lease evidence."));
		Result.bLeaseAuthorityChecked = true;
		Result.AuthoritativeLease = AuthoritativeLease;
		return Result;
	}
	auto Result = ConsumerRuntime.TryDeactivate(ProductHost, RemoveCommand);
	Result.bLeaseAuthorityChecked = true;
	Result.AuthoritativeLease = AuthoritativeLease;
	return Result;
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
	if (!Correlation.IsValid()
		|| !LedgerId.IsValid()
		|| !ConsumerRuntime.IsValid()
		|| ConsumerRuntime.GetBridge().GetCorrelation() != Correlation
		|| !Router.IsValid())
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

Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::RejectConsumer(
	const Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus Status,
	const TCHAR* Diagnostic) const
{
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntimeResult Result;
	Result.Status = Status;
	Result.RuntimeId = ConsumerRuntime.GetRuntimeId();
	Result.Diagnostic = Diagnostic;
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::
TryGetAuthoritativeActiveLease(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command,
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& OutLease) const
{
	OutLease = Fdemo_mapShanmenFormationInfluenceLeaseSnapshot();
	return IsValid() && Router.IsBound() && Command.IsValid()
		&& Router.GetCoordinator().GetExecutionService().GetRuntime().
			GetExecutor().TryGetActiveLease(
				Command.GetProjection().GetLease().Key, OutLease);
}

bool Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::
IsCompletedConsumerCommand(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command) const
{
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Completed;
	return IsValid()
		&& ConsumerRuntime.GetBridge().GetCommandHost().
			TryGetCompletedTransaction(Command, Completed);
}
