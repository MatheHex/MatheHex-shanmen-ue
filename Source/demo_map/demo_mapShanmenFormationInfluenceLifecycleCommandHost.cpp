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

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery::IsValid()
	const
{
	return LifecycleCommandId.IsValid() && SubjectEntityId.IsValid()
		&& AuthoritativeLease.IsValid() && Definition.IsValid()
		&& Projection.IsValid() && Apply.IsValid() && Remove.IsValid()
		&& SubjectEntityId == AuthoritativeLease.Key.SubjectEntityId
		&& LeaseSnapshotsMatch(
			AuthoritativeLease, Projection.GetLease())
		&& Definition.Matches(Projection.GetDefinition())
		&& Apply.GetOperation()
			== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
		&& Remove.GetOperation()
			== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove
		&& Apply.GetProjection().Matches(Projection)
		&& Remove.GetProjection().Matches(Projection)
		&& Apply.GetHandle() == Remove.GetHandle()
		&& Apply.GetCommandId() != Remove.GetCommandId();
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandDeliveryResult::
IsSuccess() const
{
	if (Status
			!= Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::Prepared
		|| !SourceReceipt.IsValid() || !SourceReceipt.Result.IsSuccess()
		|| SourceReceipt.Command.GetKind()
			!= Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep
		|| !ProjectionAttempt.HasProjection() || !Delivery.IsValid()
		|| SourceReceipt.Command.GetCommandId()
			!= Delivery.LifecycleCommandId
		|| !ProjectionAttempt.Projection.Matches(Delivery.Projection))
	{
		return false;
	}
	const auto& Lifecycle = SourceReceipt.Result.Lifecycle;
	const auto& Execution = Lifecycle.Step.Execution;
	const auto& Intent = Execution.Invocation.Intent;
	return Lifecycle.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepAccepted
		&& Lifecycle.Step.IsSuccess() && Execution.IsSuccess()
		&& Execution.Invocation.IsValid()
		&& Intent.Operation
			== Edemo_mapShanmenFormationInfluenceOperation::Apply
		&& SourceReceipt.Command.GetStepRequest().ExpectedIntentId
			== Intent.IntentId
		&& Delivery.AuthoritativeLease.ApplyIntentId == Intent.IntentId
		&& Delivery.SubjectEntityId == Intent.SubjectEntityId;
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

Fdemo_mapShanmenFormationInfluenceConsumerCommandDeliveryResult
Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::
TryPrepareConsumerCommands(
	const FGuid& AppliedLifecycleCommandId,
	const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Definition)
	const
{
	Fdemo_mapShanmenFormationInfluenceConsumerCommandDeliveryResult Result;
	const auto Reject = [&Result](
		const Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus Status,
		const TCHAR* Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};

	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::HostInvalid,
			TEXT("Consumer command delivery requires one valid lifecycle CommandHost."));
	}
	if (!AppliedLifecycleCommandId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleCommandIdInvalid,
			TEXT("Consumer command delivery requires one valid lifecycle CommandId."));
	}
	if (!Definition.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				DefinitionInvalid,
			TEXT("Consumer command delivery requires one valid authored definition."));
	}
	if (!Router.TryGetRecord(
			AppliedLifecycleCommandId, Result.SourceReceipt))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleReceiptNotFound,
			TEXT("Consumer command delivery requires this Host's durable lifecycle receipt."));
	}
	if (!Result.SourceReceipt.IsValid()
		|| !Result.SourceReceipt.Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleReceiptRejected,
			TEXT("Consumer command delivery requires one successful lifecycle receipt."));
	}
	if (Result.SourceReceipt.Command.GetKind()
		!= Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::ExecuteStep)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleOperationMismatch,
			TEXT("Consumer command delivery requires one successful Apply step receipt."));
	}

	const auto& Lifecycle = Result.SourceReceipt.Result.Lifecycle;
	const auto& Execution = Lifecycle.Step.Execution;
	if (Lifecycle.Status
			!= Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepAccepted
		|| !Lifecycle.Step.IsSuccess() || !Execution.IsSuccess()
		|| !Execution.Invocation.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleReceiptRejected,
			TEXT("Lifecycle receipt does not contain one accepted execution step."));
	}

	const auto& ApplyIntent = Execution.Invocation.Intent;
	if (ApplyIntent.Operation
		!= Edemo_mapShanmenFormationInfluenceOperation::Apply)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleOperationMismatch,
			TEXT("Consumer command delivery cannot be derived from a Remove intent."));
	}
	Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
	if (Result.SourceReceipt.Command.GetStepRequest().ExpectedIntentId
			!= ApplyIntent.IntentId
		|| !Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
			ApplyIntent, Key))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::StateInvalid,
			TEXT("Apply receipt and frozen lifecycle command identities disagree."));
	}

	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot AuthoritativeLease;
	const auto& Executor = Router.GetCoordinator().GetExecutionService().
		GetRuntime().GetExecutor();
	if (!Executor.TryGetActiveLease(Key, AuthoritativeLease))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LeaseNotActive,
			TEXT("The Apply receipt no longer owns one active authoritative lease."));
	}
	if (!AuthoritativeLease.IsValid()
		|| AuthoritativeLease.ApplyIntentId != ApplyIntent.IntentId
		|| !AuthoritativeLease.Key.Matches(Key))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::StateInvalid,
			TEXT("Active authoritative lease does not match its Apply receipt."));
	}

	Result.ProjectionAttempt =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			AuthoritativeLease, Definition);
	if (!Result.ProjectionAttempt.HasProjection())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				ProjectionRejected,
			TEXT("Active lease could not produce one native consumer projection."));
	}

	Result.Delivery.LifecycleCommandId = AppliedLifecycleCommandId;
	Result.Delivery.SubjectEntityId = ApplyIntent.SubjectEntityId;
	Result.Delivery.AuthoritativeLease = AuthoritativeLease;
	Result.Delivery.Definition = Definition;
	Result.Delivery.Projection = Result.ProjectionAttempt.Projection;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerProjector::TryBuildCommand(
			Result.Delivery.Projection,
			Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply,
			Result.Delivery.Apply)
		|| !Fdemo_mapShanmenFormationInfluenceConsumerProjector::TryBuildCommand(
			Result.Delivery.Projection,
			Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove,
			Result.Delivery.Remove))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				CommandBuildRejected,
			TEXT("Consumer projection failed to build reversible commands."));
	}

	Result.Status =
		Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::Prepared;
	Result.Diagnostic =
		TEXT("Prepared one immutable consumer command delivery from the active lease.");
	if (!Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::StateInvalid,
			TEXT("Prepared consumer command delivery violated invariants."));
	}
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
