#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapItemTypes.h"
#include "demo_mapProfileRunTypes.h"
#include "demo_mapShanmenPreparationAdapter.h"

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
	ClaimRejected,
	RuntimeConflict,
	RuntimeMaterializationRejected,
	SettlementInvalid,
	UnsupportedTerminalReason,
	UnknownSecuredItem,
	FinalizeRejected
};

struct Fdemo_mapShanmenRunStartResult
{
	Edemo_mapShanmenRunLifecycleStatus Status =
		Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady;
	FString Diagnostic;
	FGuid ActiveRunId;
	Fdemo_mapShanmenPreparedLoadoutReceipt PreparedLoadout;
	FShanmenItemDurableCommandResult ClaimCommand;
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
 * One-way P1.9 bridge from the durable ShanmenItems prepared receipt into the
 * existing transient Runtime. It never writes Profile ActiveRun or Code B.
 */
struct Fdemo_mapShanmenRunLifecycleAdapter
{
	static Fdemo_mapShanmenRunStartResult StartPreparedRun(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Udemo_mapItemSubsystem& Runtime);

	static Fdemo_mapShanmenRunFinalizeResult FinalizeSettlement(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapSettlementSummary& Summary);
};
