#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapItemTypes.h"
#include "demo_mapProfileRunTypes.h"
#include "demo_mapShanmenPreparationAdapter.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapItemSubsystem;
class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenRunLifecycleStatus : uint8
{
	Started,
	Resumed,
	Finalized,
	NoChange,
	AuthorityNotReady,
	PreparedLoadoutRejected,
	AuthorityStartRejected,
	RuntimeConflict,
	RuntimeMaterializationRejected,
	SettlementInvalid,
	UnsupportedTerminalReason,
	UnknownSecuredItem,
	AcquiredItemRejected,
	FinalizeRejected
};

struct Fdemo_mapShanmenRunStartResult
{
	Edemo_mapShanmenRunLifecycleStatus Status =
		Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady;
	FString Diagnostic;
	FGuid ActiveRunId;
	Fdemo_mapShanmenRunCorrelation RunCorrelation;
	Fdemo_mapShanmenPreparedLoadoutReceipt PreparedLoadout;
	FShanmenItemDurableCommandResult StartCommand;
	Fdemo_mapPreparedRunRuntimeResult RuntimeResult;

	bool IsStarted() const
	{
		return Status == Edemo_mapShanmenRunLifecycleStatus::Started
			|| Status == Edemo_mapShanmenRunLifecycleStatus::Resumed;
	}
};

struct Fdemo_mapShanmenRunFinalizeResult
{
	Edemo_mapShanmenRunLifecycleStatus Status =
		Edemo_mapShanmenRunLifecycleStatus::SettlementInvalid;
	FString Diagnostic;
	FGuid ActiveRunId;
	FShanmenItemDurableCommandResult FinalizeCommand;

	bool IsFinalized() const
	{
		return Status == Edemo_mapShanmenRunLifecycleStatus::Finalized
			|| Status == Edemo_mapShanmenRunLifecycleStatus::NoChange;
	}
};

/**
 * One-way P1.12 bridge from the durable ShanmenItems prepared receipt into the
 * existing transient Runtime and back through one atomic terminal command. It
 * never writes Profile ActiveRun or Code B.
 */
struct Fdemo_mapShanmenRunLifecycleAdapter
{
	/** Rebuild the complete immutable active-Run correlation without mutation. */
	static bool TryGetActiveRunCorrelation(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapShanmenRunCorrelation& OutCorrelation,
		FString* OutDiagnostic = nullptr);

	/**
	 * Read-only recovery probe for the one durable Start/Claim that has no
	 * matching Finalize receipt.  It never prepares or mutates Runtime state.
	 */
	static bool TryFindRecoverableActiveRun(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		FGuid& OutActiveRunId,
		FString* OutDiagnostic = nullptr);

	static Fdemo_mapShanmenRunStartResult StartPreparedRun(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Udemo_mapItemSubsystem& Runtime);

	static Fdemo_mapShanmenRunFinalizeResult FinalizeSettlement(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapSettlementSummary& Summary);
};
