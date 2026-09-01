#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.h"

/**
 * Immutable caller command for one P12.28 retry step.
 *
 * The command binds its explicit policy/count to one frozen prepared root and
 * one exact Host cursor. A mutating step therefore makes the command stale;
 * callers must capture a new command from the returned durable state.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand
{
public:
	static bool TryCapture(
		const FGuid& CommandId,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
			PreparedDispatch,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
			Request,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand&
			OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand&
			Other) const;
	bool MatchesHostCursor(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host)
		const;

	const FGuid& GetCommandId() const { return CommandId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch&
	GetPreparedDispatch() const
	{
		return PreparedDispatch;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest&
	GetRequest() const
	{
		return Request;
	}
	const FGuid& GetExpectedHostId() const { return ExpectedHostId; }
	const FGuid& GetExpectedRunId() const { return ExpectedRunId; }
	const FGuid& GetExpectedBatchId() const { return ExpectedBatchId; }
	int64 GetExpectedNextSequence() const { return ExpectedNextSequence; }
	int32 GetExpectedRecordCount() const { return ExpectedRecordCount; }
	bool ExpectsTerminalHost() const { return bExpectedTerminal; }

private:
	FGuid CommandId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch
		PreparedDispatch;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest
		Request;
	FGuid ExpectedHostId;
	FGuid ExpectedRunId;
	FGuid ExpectedBatchId;
	int64 ExpectedNextSequence = 0;
	int32 ExpectedRecordCount = 0;
	bool bExpectedTerminal = false;
};

/** Immutable audit receipt for one exact retry-step command attempt. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt
{
public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand&
	GetCommand() const
	{
		return Command;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult&
	GetStep() const
	{
		return Step;
	}
	bool IsHandled() const { return IsValid() && Step.IsHandled(); }
	bool DidExecuteRetry() const
	{
		return IsValid() && Step.DidExecuteRetry();
	}
	bool CanCaptureNextCommand() const
	{
		return IsValid() && Step.CanRequestAnotherStep();
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandAdapter;

	FGuid ReceiptId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand
		Command;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepResult Step;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus
	: uint8
{
	RecordedHandled,
	RecordedDecisionRejected,
	RecordedExecutionRejected,
	CommandInvalid,
	HostInvalid,
	HostCursorMismatch,
	StepStateInvalid,
	ReceiptInvalid
};

/** Result of routing exactly one immutable retry-step command. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandStatus::
				CommandInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepReceipt
		Receipt;

	bool HasReceipt() const;
};

/** Stateless command-to-P12.28 adapter; owns no replay ledger or retry loop. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandAdapter
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandResult
	TryExecute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand&
			Command,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
};
