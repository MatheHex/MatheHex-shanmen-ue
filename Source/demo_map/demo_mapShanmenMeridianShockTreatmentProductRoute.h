#pragma once

#include "CoreMinimal.h"

#include "demo_mapShanmenCombatRunFixedTimeline.h"
#include "demo_mapShanmenMeridianShockTreatmentAdapter.h"
#include "demo_mapShanmenMeridianShockTreatmentRecoveryProof.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenCombatConditionComponent;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Immutable player request captured before either treatment authority mutates. */
struct Fdemo_mapShanmenMeridianShockTreatmentCommand
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenMeridianShockTreatmentCommand& Other) const;

	const FGuid& GetRequestId() const { return RequestId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	int32 GetQuantity() const { return Quantity; }
	const Fdemo_mapShanmenRunCorrelation& GetCorrelation() const
	{
		return Correlation;
	}
	const Fdemo_mapShanmenCombatConditionStatusSnapshot&
	GetConditionStatus() const
	{
		return ConditionStatus;
	}
	const Fdemo_mapShanmenCombatRunTimelineSample& GetTimelineSample() const
	{
		return TimelineSample;
	}

private:
	friend class Fdemo_mapShanmenMeridianShockTreatmentProductRoute;

	static bool TryCapture(
		const FGuid& RequestId,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenCombatConditionStatusSnapshot& ConditionStatus,
		const FGuid& ItemInstanceId,
		int32 Quantity,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		Fdemo_mapShanmenMeridianShockTreatmentCommand& OutCommand);

	FGuid RequestId;
	Fdemo_mapShanmenRunCorrelation Correlation;
	Fdemo_mapShanmenCombatConditionStatusSnapshot ConditionStatus;
	FGuid ItemInstanceId;
	int32 Quantity = 0;
	Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
};

enum class Edemo_mapShanmenMeridianShockTreatmentRouteStatus : uint8
{
	Rejected,
	Committed,
	Replayed,
	Cancelled,
	RecoveryRequired
};

enum class Edemo_mapShanmenMeridianShockTreatmentRouteError : uint8
{
	None,
	RouteInactive,
	RouteInvalid,
	RequestInvalid,
	RequestIdConflict,
	RunMismatch,
	ConditionUnavailable,
	PrepareRejected,
	TreatmentRejected,
	CancellationRejected,
	CommitRejected,
	InterruptedAfterTreatment
};

/** Complete ordered proof returned by the sole treatment product route. */
struct Fdemo_mapShanmenMeridianShockTreatmentRouteResult
{
	Edemo_mapShanmenMeridianShockTreatmentRouteStatus Status =
		Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Rejected;
	Edemo_mapShanmenMeridianShockTreatmentRouteError Error =
		Edemo_mapShanmenMeridianShockTreatmentRouteError::RouteInactive;
	bool bReusedRequest = false;
	FGuid RequestId;
	FGuid TreatmentId;
	Fdemo_mapShanmenMeridianShockTreatmentItemResult Item;
	Fdemo_mapShanmenCombatConditionTreatmentResult Treatment;
	FString Diagnostic;

	bool IsCommitted() const
	{
		return (Status
				== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Committed
			|| Status
				== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Replayed)
			&& Error == Edemo_mapShanmenMeridianShockTreatmentRouteError::None
			&& RequestId.IsValid()
			&& TreatmentId.IsValid()
			&& Treatment.IsSuccess()
			&& Item.IsFinalized();
	}

	bool IsCancelled() const
	{
		return Status
				== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::Cancelled
			&& RequestId.IsValid()
			&& TreatmentId.IsValid()
			&& Item.IsFinalized();
	}

	bool RequiresRecovery() const
	{
		return Status
			== Edemo_mapShanmenMeridianShockTreatmentRouteStatus::
				RecoveryRequired;
	}
};

/**
 * Sole Game-Thread owner of prepare -> treat -> commit for Meridian Shock.
 *
 * The route journals immutable commands for active-runtime conflict handling.
 * If that transient journal is lost, it can reconstruct one pending prepare
 * from ShanmenItems only when the condition authority independently proves the
 * same TreatmentId. ShanmenItems remains inventory truth and the condition
 * component remains condition truth. Once treatment succeeds, every retry is
 * commit-only: cancellation can never be selected afterward.
 */
class Fdemo_mapShanmenMeridianShockTreatmentProductRoute
{
public:
	bool TryBegin(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		Udemo_mapShanmenCombatConditionComponent* ConditionComponent,
		FString& OutDiagnostic);

	/** Captures the exact live condition revision before any item mutation. */
	bool TryCaptureCommand(
		const FGuid& RequestId,
		const FGuid& ItemInstanceId,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		Fdemo_mapShanmenMeridianShockTreatmentCommand& OutCommand,
		FString& OutDiagnostic,
		int32 Quantity = 1) const;

	/** Executes or resumes the exact immutable command. */
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult TryExecute(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenMeridianShockTreatmentCommand& Command);

	/**
	 * Recovers one durable treatment prepare whose transient command journal was
	 * lost. Recovery is commit-only once condition proof exists and never uses a
	 * missing proof as permission to cancel inventory.
	 */
	bool TryRecoverDurablePreparation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		int32& OutRecoveredCount,
		FString& OutDiagnostic);
	/**
	 * Recovery overload for proofs decoded from the condition domain's trusted
	 * persistence source. Foreign proofs are ignored; corrupt or conflicting
	 * proof sets fail closed before either authority mutates.
	 */
	bool TryRecoverDurablePreparation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		TConstArrayView<
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof> Proofs,
		int32& OutRecoveredCount,
		FString& OutDiagnostic);

	/** Refuses to forget a prepared or treated command that still needs work. */
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);

	bool IsValid() const;
	bool IsEmpty() const;
	bool HasUnresolvedRecovery() const;
	const FGuid& GetRunId() const { return Correlation.ActiveRunId; }
	int32 NumJournaledCommands() const { return Journal.Num(); }

#if WITH_DEV_AUTOMATION_TESTS
	/** Simulates transient route/session loss at the unsafe handoff boundary. */
	void SetInterruptAfterTreatmentForAutomation(bool bEnabled)
	{
		bInterruptAfterTreatmentForAutomation = bEnabled;
	}
#endif

private:
	struct FJournalEntry
	{
		Fdemo_mapShanmenMeridianShockTreatmentCommand Command;
		Fdemo_mapShanmenMeridianShockTreatmentItemResult Preparation;
		Fdemo_mapShanmenCombatConditionTreatmentResult Treatment;
		Fdemo_mapShanmenMeridianShockTreatmentItemResult Finalization;
		bool bPrepared = false;
		bool bTreated = false;
		bool bCommitted = false;
		bool bCancelled = false;

		bool IsValid() const;
		bool IsResolved() const { return bCommitted || bCancelled; }
	};

	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Reject(
		Edemo_mapShanmenMeridianShockTreatmentRouteError Error,
		const Fdemo_mapShanmenMeridianShockTreatmentCommand* Command,
		const TCHAR* Diagnostic,
		bool bReusedRequest = false) const;
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Resume(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		FJournalEntry& Entry,
		bool bReusedRequest);
	void Clear();

	Fdemo_mapShanmenRunCorrelation Correlation;
	TWeakObjectPtr<Udemo_mapShanmenCombatConditionComponent>
		ConditionComponent;
	TMap<FGuid, FJournalEntry> Journal;
	bool bActive = false;
#if WITH_DEV_AUTOMATION_TESTS
	bool bInterruptAfterTreatmentForAutomation = false;
#endif
};
