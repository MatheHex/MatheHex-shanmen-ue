#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.h"

/** Caller-owned identities for one explicit Create -> ProcessNext -> End pass. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture
{
	FGuid HostId;
	int64 CreateSequence = 0;
	int64 ProcessSequence = 1;
	int64 EndSequence = 2;
	FGuid CreateCommandId;
	FGuid ProcessCommandId;
	FGuid EndCommandId;
	FGuid VisualConsumerId;
	FGuid AudioConsumerId;
	FGuid VisualAttemptId;
	FGuid AudioAttemptId;

	bool IsValid() const;
};

/**
 * Frozen single-projection transaction request.
 *
 * Every transport and execution identity is captured before dispatch. The
 * request owns no ProductSession, Host, executor, sequence or retry state.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest&
			Other) const;

	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
	GetProjection() const
	{
		return Projection;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest&
	GetCreateRequest() const
	{
		return CreateRequest;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
	GetProcessEnvelope() const
	{
		return ProcessEnvelope;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
	GetEndEnvelope() const
	{
		return EndEnvelope;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory;

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection Projection;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest
		CreateRequest;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope ProcessEnvelope;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope EndEnvelope;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus
	: uint8
{
	Captured,
	IdentityInvalid,
	ProjectionInvalid,
	CreateRejected,
	ProcessRejected,
	EndRejected,
	RequestRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
				IdentityInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest
		Request;

	bool IsCaptured() const
	{
		return Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureStatus::
					Captured
			&& Request.IsValid();
	}
};

/** Stateless capture of all three commands before any Host mutation. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureResult
	Capture(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
			Projection,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture&
			Identity);
};

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus
	: uint8
{
	Completed,
	Resumed,
	Replayed,
	RequestInvalid,
	CreateRejected,
	ProcessRetryPending,
	ProcessRejected,
	EndRejected,
	StateInvalid
};

/** Complete evidence for one bounded caller-driven transaction attempt. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
			RequestInvalid;
	FGuid RunId;
	FGuid ConfigId;
	FGuid CuePolicyId;
	FGuid EventId;
	FGuid HostId;
	FGuid BatchId;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteResult Create;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Process;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult End;
	int64 FinalNextSequence = 0;
	int32 FinalRecordCount = 0;
	bool bFinalTerminal = false;
	bool bAnyReplay = false;
	bool bAllReplay = false;
	FString Diagnostic;

	bool IsCompleted() const;
	bool HasDurableProgress() const;
};

/**
 * Bounded synchronous product transaction.
 *
 * One call dispatches each frozen envelope at most once. It stops after any
 * retry/reject and never owns executors, Host state, retry policy or a loop.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult
	TryExecute(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest&
			Request,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
};
