#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.h"

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus
	: uint8
{
	Dispatched,
	Resumed,
	Replayed,
	ProjectionRejected,
	CaptureRejected,
	TransactionIncomplete,
	StateInvalid
};

/** Complete evidence for one frozen or current Product projection dispatch. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
			ProjectionRejected;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
		Projection;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCaptureResult
		TransactionCapture;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionResult
		Transaction;

	bool IsSuccess() const;
};

/**
 * Stateless current ProductSession to P12.21 transaction seam.
 *
 * Every identity, Host and executor remains caller-owned. The route reads one
 * current immutable presentation state, captures one frozen transaction, then
 * invokes it once. The returned request is retained in TransactionCapture so
 * callers can perform historical replay after the source Session advances.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch
{
public:
	/** Dispatch one already-frozen projection without reading ProductSession. */
	static Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult
	TryDispatchProjection(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
			Projection,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture&
			Identity,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);

	static Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchResult
	TryDispatchCurrent(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session,
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture&
			Identity,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
		Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor);
};
