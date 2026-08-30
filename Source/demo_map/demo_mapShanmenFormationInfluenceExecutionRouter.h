#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceExecutorAdapter.h"

/** Caller-owned identity and expected Host intent for one routing decision. */
struct Fdemo_mapShanmenFormationInfluenceExecutionRequest
{
	FGuid RequestId;
	FGuid ExpectedIntentId;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Other) const;
};

enum class Edemo_mapShanmenFormationInfluenceRouteStatus : uint8
{
	Routed,
	RequestReplayed,
	HostEvidenceRecovered,
	RouterInvalid,
	HostInvalid,
	CorrelationMismatch,
	RequestInvalid,
	LedgerUnavailable,
	BindingConflict,
	IntentUnavailable,
	IntentOutOfOrder,
	RequestConflict,
	AttemptCollision,
	StateInvalid
};

/** Immutable command selected for a new request, replay, or Host recovery. */
struct Fdemo_mapShanmenFormationInfluenceRouteResult
{
	Edemo_mapShanmenFormationInfluenceRouteStatus Status =
		Edemo_mapShanmenFormationInfluenceRouteStatus::RequestInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceExecutionCommand Command;

	bool IsSuccess() const;
};

/**
 * Pure-value request boundary in front of the concrete product runtime.
 *
 * The Router reads but never owns or mutates ProductHost. It derives one
 * replay-stable AttemptId from LedgerId + RequestId + ExpectedIntentId, fences
 * request-payload reuse, and records only successful routing decisions. Host
 * attempt evidence permits a fresh Router to reconstruct an already-issued
 * command after the canonical pending intent has advanced or the ledger seals.
 */
class Fdemo_mapShanmenFormationInfluenceExecutionRouter
{
public:
	Fdemo_mapShanmenFormationInfluenceRouteResult TryRoute(
		const Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request);

	bool IsValid() const;
	bool IsBound() const { return BoundLedgerId.IsValid(); }
	const FGuid& GetBoundLedgerId() const { return BoundLedgerId; }
	const Fdemo_mapShanmenRunCorrelation& GetBoundCorrelation() const
	{
		return BoundCorrelation;
	}
	int32 GetRecordCount() const { return Records.Num(); }
	bool TryGetCommand(
		const FGuid& RequestId,
		Fdemo_mapShanmenFormationInfluenceExecutionCommand& OutCommand) const;

private:
	struct FRecord
	{
		Fdemo_mapShanmenFormationInfluenceExecutionRequest Request;
		Fdemo_mapShanmenFormationInfluenceExecutionCommand Command;
	};

	Fdemo_mapShanmenRunCorrelation BoundCorrelation;
	FGuid BoundLedgerId;
	TArray<FRecord> Records;
};
