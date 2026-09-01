#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FCommand =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand;
	using FReceipt =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt;
	using FResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandResult;
	using ECommandStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus;
	using EStepStatus =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString Int64String(const int64 Value)
	{
		return FString::Printf(TEXT("%lld"), static_cast<long long>(Value));
	}

	FGuid DeriveReceiptId(
		const FCommand& Command,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult&
			Step)
	{
		if (!Command.IsValid())
		{
			return FGuid();
		}
		const auto& Projection = Command.GetPreparedDispatch().GetProjection();
		const auto& PlanSeed = Command.GetPreparedDispatch().GetPlan().GetSeed();
		const auto& Request = Command.GetRequest();
		const FString RetrySeed = Step.Decision.IsDecided()
			? GuidDigits(Step.Decision.Decision.GetRetrySeed().RetrySeed)
			: TEXT("none");
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionProductRetryStepCommandReceipt.r1"),
			{
				GuidDigits(Command.GetCommandId()),
				GuidDigits(Request.Policy.PolicySeed),
				FString::FromInt(Request.Policy.MaxRenewals),
				FString::FromInt(Request.RenewalsUsed),
				GuidDigits(PlanSeed.DispatchSeed),
				GuidDigits(PlanSeed.VisualConsumerScopeId),
				GuidDigits(PlanSeed.AudioConsumerScopeId),
				GuidDigits(Projection.GetRunId()),
				GuidDigits(Projection.GetEvent().GetEventId()),
				FString::FromInt(Projection.GetObservationRevision()),
				GuidDigits(Command.GetExpectedHostId()),
				GuidDigits(Command.GetExpectedRunId()),
				GuidDigits(Command.GetExpectedBatchId()),
				Int64String(Command.GetExpectedNextSequence()),
				FString::FromInt(Command.GetExpectedRecordCount()),
				Command.ExpectsTerminalHost() ? TEXT("terminal") : TEXT("open"),
				FString::FromInt(static_cast<int32>(Step.Status)),
				FString::FromInt(static_cast<int32>(Step.Decision.Status)),
				FString::FromInt(static_cast<int32>(Step.Execution.Status)),
				RetrySeed,
				FString::FromInt(Step.NextRenewalsUsed),
				Int64String(Step.FinalNextSequence),
				FString::FromInt(Step.FinalRecordCount),
				Step.bFinalTerminal ? TEXT("terminal") : TEXT("open")
			});
	}

	bool IsRecordedStep(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult&
			Step)
	{
		return Step.IsHandled()
			|| (Step.Status == EStepStatus::DecisionRejected
				&& !Step.Decision.IsDecided()
				&& !Step.DidExecuteRetry())
			|| (Step.Status == EStepStatus::ExecutionRejected
				&& Step.Decision.IsDecided()
				&& Step.DidExecuteRetry());
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand::
TryCapture(
	const FGuid& RequestedCommandId,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
		RequestedPreparedDispatch,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
		RequestedRequest,
	FCommand& OutCommand)
{
	OutCommand = FCommand();
	if (!RequestedCommandId.IsValid()
		|| !RequestedPreparedDispatch.IsValid()
		|| !RequestedRequest.IsValid()
		|| !Host.IsValid()
		|| !Host.IsBound()
		|| Host.GetNextSequence() <= 0
		|| Host.GetRecordCount() <= 0
		|| Host.GetNextSequence() != Host.GetRecordCount())
	{
		return false;
	}

	OutCommand.CommandId = RequestedCommandId;
	OutCommand.PreparedDispatch = RequestedPreparedDispatch;
	OutCommand.Request = RequestedRequest;
	OutCommand.ExpectedHostId = Host.GetHostId();
	OutCommand.ExpectedRunId = Host.GetRunId();
	OutCommand.ExpectedBatchId = Host.GetBatchId();
	OutCommand.ExpectedNextSequence = Host.GetNextSequence();
	OutCommand.ExpectedRecordCount = Host.GetRecordCount();
	OutCommand.bExpectedTerminal = Host.IsTerminal();
	if (!OutCommand.IsValid() || !OutCommand.MatchesHostCursor(Host))
	{
		OutCommand = FCommand();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand::
IsValid() const
{
	return CommandId.IsValid()
		&& PreparedDispatch.IsValid()
		&& Request.IsValid()
		&& ExpectedHostId.IsValid()
		&& ExpectedRunId.IsValid()
		&& ExpectedBatchId.IsValid()
		&& ExpectedNextSequence > 0
		&& ExpectedRecordCount > 0
		&& ExpectedNextSequence == ExpectedRecordCount;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand::
Matches(const FCommand& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& CommandId == Other.CommandId
		&& PreparedDispatch.Matches(Other.PreparedDispatch)
		&& Request.Matches(Other.Request)
		&& ExpectedHostId == Other.ExpectedHostId
		&& ExpectedRunId == Other.ExpectedRunId
		&& ExpectedBatchId == Other.ExpectedBatchId
		&& ExpectedNextSequence == Other.ExpectedNextSequence
		&& ExpectedRecordCount == Other.ExpectedRecordCount
		&& bExpectedTerminal == Other.bExpectedTerminal;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand::
MatchesHostCursor(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host) const
{
	return IsValid()
		&& Host.IsValid()
		&& Host.IsBound()
		&& Host.GetHostId() == ExpectedHostId
		&& Host.GetRunId() == ExpectedRunId
		&& Host.GetBatchId() == ExpectedBatchId
		&& Host.GetNextSequence() == ExpectedNextSequence
		&& Host.GetRecordCount() == ExpectedRecordCount
		&& Host.IsTerminal() == bExpectedTerminal;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt::
IsValid() const
{
	if (!ReceiptId.IsValid()
		|| !Command.IsValid()
		|| !IsRecordedStep(Step)
		|| ReceiptId != DeriveReceiptId(Command, Step))
	{
		return false;
	}

	if (!Step.Decision.IsDecided())
	{
		return Step.Status == EStepStatus::DecisionRejected
			&& Step.NextRenewalsUsed == Command.GetRequest().RenewalsUsed
			&& Step.FinalNextSequence == Command.GetExpectedNextSequence()
			&& Step.FinalRecordCount == Command.GetExpectedRecordCount()
			&& Step.bFinalTerminal == Command.ExpectsTerminalHost();
	}

	return Step.Decision.Decision.GetPreparedDispatch().Matches(
			Command.GetPreparedDispatch())
		&& Step.Decision.Decision.GetRequest().Matches(Command.GetRequest())
		&& Step.NextRenewalsUsed
			== Step.Decision.Decision.GetNextRenewalsUsed();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandResult::
HasReceipt() const
{
	const bool bRecorded = Status == ECommandStatus::RecordedHandled
		|| Status == ECommandStatus::RecordedDecisionRejected
		|| Status == ECommandStatus::RecordedExecutionRejected;
	return bRecorded && Receipt.IsValid();
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandAdapter::
TryExecute(
	const FCommand& Command,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	FResult Result;
	if (!Command.IsValid())
	{
		Result.Diagnostic = TEXT(
			"Retry-step adapter requires one valid immutable command.");
		return Result;
	}
	if (!Host.IsValid() || !Host.IsBound())
	{
		Result.Status = ECommandStatus::HostInvalid;
		Result.Diagnostic = TEXT(
			"Retry-step command requires one valid bound command Host.");
		return Result;
	}
	if (!Command.MatchesHostCursor(Host))
	{
		Result.Status = ECommandStatus::HostCursorMismatch;
		Result.Diagnostic = TEXT(
			"Retry-step command is stale or belongs to another Host cursor.");
		return Result;
	}

	Result.Receipt.Command = Command;
	Result.Receipt.Step =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepService::
			TryRunStep(
				Command.GetPreparedDispatch(),
				Host,
				VisualExecutor,
				AudioExecutor,
				Command.GetRequest());
	if (Result.Receipt.Step.IsHandled())
	{
		Result.Status = ECommandStatus::RecordedHandled;
	}
	else if (Result.Receipt.Step.Status == EStepStatus::DecisionRejected)
	{
		Result.Status = ECommandStatus::RecordedDecisionRejected;
	}
	else if (Result.Receipt.Step.Status == EStepStatus::ExecutionRejected
		&& Result.Receipt.Step.DidExecuteRetry())
	{
		Result.Status = ECommandStatus::RecordedExecutionRejected;
	}
	else
	{
		Result.Status = ECommandStatus::StepStateInvalid;
		Result.Diagnostic = Result.Receipt.Step.Diagnostic;
		Result.Receipt = FReceipt();
		return Result;
	}

	Result.Receipt.ReceiptId =
		DeriveReceiptId(Result.Receipt.Command, Result.Receipt.Step);
	if (!Result.Receipt.IsValid())
	{
		Result.Status = ECommandStatus::ReceiptInvalid;
		Result.Diagnostic = TEXT(
			"Retry-step command produced inconsistent immutable receipt evidence.");
		Result.Receipt = FReceipt();
		return Result;
	}
	Result.Diagnostic = Result.Receipt.Step.Diagnostic;
	return Result;
}
