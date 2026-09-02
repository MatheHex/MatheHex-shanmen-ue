#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenMeridianShockTreatmentProductRoute.h"

class Udemo_mapShanmenCombatConditionComponent;
class Udemo_mapShanmenItemAuthoritySubsystem;

/**
 * Active-Run session for physical Meridian Shock treatment requests.
 *
 * It keeps only immutable commands needed for exact replay and asks the route
 * to reconcile ShanmenItems' durable prepare ledger before new work or Run
 * teardown. The route remains the sole prepare -> treat -> commit owner;
 * ShanmenItems and the condition component remain the two authorities.
 */
class Fdemo_mapShanmenMeridianShockTreatmentProductSession
{
public:
	Fdemo_mapShanmenMeridianShockTreatmentProductSession() = default;
	~Fdemo_mapShanmenMeridianShockTreatmentProductSession() = default;

	Fdemo_mapShanmenMeridianShockTreatmentProductSession(
		const Fdemo_mapShanmenMeridianShockTreatmentProductSession&) = delete;
	Fdemo_mapShanmenMeridianShockTreatmentProductSession& operator=(
		const Fdemo_mapShanmenMeridianShockTreatmentProductSession&) = delete;
	Fdemo_mapShanmenMeridianShockTreatmentProductSession(
		Fdemo_mapShanmenMeridianShockTreatmentProductSession&&) = delete;
	Fdemo_mapShanmenMeridianShockTreatmentProductSession& operator=(
		Fdemo_mapShanmenMeridianShockTreatmentProductSession&&) = delete;

	bool TryBegin(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		Udemo_mapShanmenCombatConditionComponent* ConditionComponent,
		FString& OutDiagnostic);

	/** Captures and executes one exact hotbar request, or replays its command. */
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult TrySubmitHotbar(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FGuid& RequestId,
		const FGuid& ItemInstanceId,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample);

	/** Reconciles the durable item ledger, then retries runtime commands. */
	bool TryRecoverPending(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		FString& OutDiagnostic);
	bool TryRecoverPending(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		TConstArrayView<
			Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof> Proofs,
		FString& OutDiagnostic);

	/** Recovers pending commits before allowing the active Run to be forgotten. */
	bool TryEnd(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FGuid& ExpectedRunId,
		FString& OutDiagnostic);

	bool IsActive() const { return !Route.IsEmpty(); }
	bool IsValid() const;
	bool IsEmpty() const { return Route.IsEmpty() && Requests.IsEmpty(); }
	bool HasUnresolvedRecovery() const
	{
		return Route.HasUnresolvedRecovery();
	}
	const FGuid& GetRunId() const { return Route.GetRunId(); }
	int32 NumCapturedRequests() const { return Requests.Num(); }
	int32 NumPendingRecovery() const;

#if WITH_DEV_AUTOMATION_TESTS
	void SetInterruptAfterTreatmentForAutomation(bool bEnabled)
	{
		Route.SetInterruptAfterTreatmentForAutomation(bEnabled);
	}
#endif

private:
	struct FCapturedRequest
	{
		Fdemo_mapShanmenMeridianShockTreatmentCommand Command;
		bool bRequiresRecovery = false;
	};

	Fdemo_mapShanmenMeridianShockTreatmentRouteResult Reject(
		Edemo_mapShanmenMeridianShockTreatmentRouteError Error,
		const FGuid& RequestId,
		const TCHAR* Diagnostic) const;

	Fdemo_mapShanmenMeridianShockTreatmentProductRoute Route;
	TMap<FGuid, FCapturedRequest> Requests;
};
