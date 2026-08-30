#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCoordinator.h"

class UWorld;

enum class Edemo_mapShanmenFormationInfluenceLifecycleCommandKind : uint8
{
	Invalid,
	ExecuteStep,
	PrepareTerminal,
	SealAndEnd
};

/** Frozen caller intent for exactly one explicit lifecycle operation. */
class Fdemo_mapShanmenFormationInfluenceLifecycleCommand
{
public:
	static bool TryCaptureStep(
		const FGuid& CommandId,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand);
	static bool TryCapturePrepareTerminal(
		const FGuid& CommandId,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand);
	static bool TryCaptureSealAndEnd(
		const FGuid& CommandId,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Other) const;
	const FGuid& GetCommandId() const { return CommandId; }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	Edemo_mapShanmenFormationInfluenceLifecycleCommandKind GetKind() const
	{
		return Kind;
	}
	const Fdemo_mapShanmenFormationInfluenceExecutionRequest&
	GetStepRequest() const
	{
		return StepRequest;
	}

private:
	static bool TryCaptureWithoutStep(
		const FGuid& CommandId,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind Kind,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand& OutCommand);

	FGuid CommandId;
	Fdemo_mapShanmenRunCorrelation Correlation;
	Edemo_mapShanmenFormationInfluenceLifecycleCommandKind Kind =
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::Invalid;
	Fdemo_mapShanmenFormationInfluenceExecutionRequest StepRequest;
};

enum class Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus : uint8
{
	Applied,
	LifecycleRejected,
	CommandInvalid,
	RouterInvalid,
	CommandIdConflict,
	OperationIdentityConflict,
	StateInvalid
};

/** Auditable outer command receipt around one P8.20 lifecycle result. */
struct Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult
{
	Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus Status =
		Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::CommandInvalid;
	FGuid CommandId;
	Edemo_mapShanmenFormationInfluenceLifecycleCommandKind Kind =
		Edemo_mapShanmenFormationInfluenceLifecycleCommandKind::Invalid;
	bool bReplay = false;
	bool bRecoveryAttempted = false;
	bool bRouterStateCommitted = false;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceLifecycleResult Lifecycle;

	bool IsSuccess() const;
	bool IsDurableRecord() const;
	bool IsReplay() const { return bReplay; }
};

/**
 * Typed, caller-driven command boundary around the P8.20 Coordinator.
 *
 * One call routes exactly one frozen command. Accepted or forward-mutating
 * results lock CommandId payload identity. Exact success replay returns stored
 * receipts without re-entry. Only an exact SealAndEnd whose prior result was
 * EndRejected may explicitly re-enter for forward World-teardown recovery.
 * The Router never discovers requests, loops, retries in the background, owns
 * Host/World, or interprets input devices.
 */
class Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter
{
public:
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult TryRoute(
		UWorld* World,
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command);

	bool IsValid() const;
	bool IsEmpty() const { return Records.IsEmpty(); }
	bool IsBound() const { return Coordinator.IsBound(); }
	int32 GetRecordCount() const { return Records.Num(); }
	const Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator&
	GetCoordinator() const
	{
		return Coordinator;
	}

private:
	struct FRecord
	{
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand Command;
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult Result;
	};

	static Fdemo_mapShanmenFormationInfluenceLifecycleResult Execute(
		Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator& Coordinator,
		UWorld* World,
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandResult TryRecoverEnd(
		UWorld* World,
		Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommand& Command,
		int32 RecordIndex);

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Coordinator;
	TArray<FRecord> Records;
};
