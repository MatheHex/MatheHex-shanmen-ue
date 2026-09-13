#pragma once

#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenFormationScatterResourcePlan.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenFormationScatterResourcePreparationStatus : uint8
{
	Invalid,
	Prepared,
	Replayed,
	AuthorityNotReady,
	SnapshotUnavailable,
	PlanInvalid,
	PlanStale,
	PlanStaleRolledBack,
	EvidenceInvalid,
	AttemptCancelled,
	PrepareRejectedRolledBack,
	RollbackRecoveryRequired,
	PreparationRecoveryRequired,
	ForwardRecoveryRequired
};

/**
 * Narrow serialized authority boundary used by the bounded preparation
 * algorithm. Product code adapts the sole GameInstance item authority;
 * automation may inject the same durable service behind this interface.
 */
class Idemo_mapShanmenFormationScatterResourceAuthority
{
public:
	virtual ~Idemo_mapShanmenFormationScatterResourceAuthority() = default;

	virtual bool IsReady() const = 0;
	virtual bool TryCaptureSnapshot(
		FShanmenItemAuthoritySnapshot& OutSnapshot) const = 0;
	virtual FShanmenItemDurableCommandResult PrepareQuantity(
		const FShanmenItemRunQuantityIntentRequest& Request) = 0;
	virtual FShanmenItemDurableCommandResult FinalizeQuantity(
		const FShanmenItemRunQuantityIntentFinalizeRequest& Request) = 0;
};

/** Complete evidence from one bounded whole-batch preparation pass. */
struct Fdemo_mapShanmenFormationScatterResourcePreparationResult
{
	Edemo_mapShanmenFormationScatterResourcePreparationStatus Status =
		Edemo_mapShanmenFormationScatterResourcePreparationStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationScatterResourcePlan Plan;
	TArray<FShanmenItemDurableCommandResult> PrepareCommands;
	TArray<FShanmenItemDurableCommandResult> RollbackCommands;
	/** One slot per plan reservation; an absent receipt remains invalid. */
	TArray<FShanmenItemTransactionReceipt> PrepareReceipts;
	/** One slot per plan reservation; this stage only creates Cancelled receipts. */
	TArray<FShanmenItemTransactionReceipt> FinalizeReceipts;

	bool IsValid() const;
	bool IsPrepared() const;
	bool IsCancelled() const;
	bool RequiresRecovery() const;
};

/**
 * Durable all-or-none preparation of one immutable P27.18 resource plan.
 *
 * Every physical stack is prepared in canonical plan order. A failed line
 * triggers one bounded reverse-order cancellation pass over all preceding
 * durable prepares. Existing exact receipts are reconstructed before any
 * command, so replay never depends on transient session state. This stage
 * never commits quantity, submits formation anchors, or touches World state.
 */
struct Fdemo_mapShanmenFormationScatterResourcePreparation
{
	/** Product entry point through the sole GameInstance item authority. */
	static Fdemo_mapShanmenFormationScatterResourcePreparationResult Prepare(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);

	/** Testable bounded algorithm behind the product authority adapter. */
	static Fdemo_mapShanmenFormationScatterResourcePreparationResult
	PrepareWithAuthority(
		Idemo_mapShanmenFormationScatterResourceAuthority& Authority,
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan);

	/**
	 * Deterministic terminal identity shared by future commit and current
	 * rollback. The first durable terminal choice therefore wins forever.
	 */
	static bool BuildFinalizeRequest(
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		int32 ReservationIndex,
		bool bCommit,
		FShanmenItemRunQuantityIntentFinalizeRequest& OutRequest);
};
