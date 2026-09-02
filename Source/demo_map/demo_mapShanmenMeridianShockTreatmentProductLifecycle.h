#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenMeridianShockTreatmentProductSession.h"

class Udemo_mapShanmenCombatConditionComponent;
class Udemo_mapShanmenItemAuthoritySubsystem;

/**
 * Product-lifetime binding between the active durable Run and its treatment
 * session. It derives the condition recovery partition from the bound item
 * owner, storage root and active Run. It owns no UI, key binding, timer, Tick,
 * inventory or condition truth, and refuses teardown while recovery remains.
 */
class Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle
{
public:
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle() = default;
	~Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle() = default;

	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle(
		const Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle&) = delete;
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle& operator=(
		const Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle&) = delete;
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle(
		Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle&&) = delete;
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle& operator=(
		Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle&&) = delete;

	bool TryBegin(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Udemo_mapShanmenCombatConditionComponent* ConditionComponent,
		FString& OutDiagnostic);

	Fdemo_mapShanmenMeridianShockTreatmentRouteResult TrySubmitHotbar(
		const FGuid& RequestId,
		const FGuid& ItemInstanceId,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample);

	bool TryRecoverPending(FString& OutDiagnostic);
	bool TryEnd(FString& OutDiagnostic);

	bool IsActive() const { return Session.IsActive(); }
	bool IsValid() const;
	bool IsEmpty() const { return !IsActive() && IsValid(); }
	bool HasUnresolvedRecovery() const
	{
		return Session.HasUnresolvedRecovery();
	}
	const FGuid& GetRunId() const { return Session.GetRunId(); }
	int32 NumCapturedRequests() const
	{
		return Session.NumCapturedRequests();
	}
	int32 NumPendingRecovery() const
	{
		return Session.NumPendingRecovery();
	}

#if WITH_DEV_AUTOMATION_TESTS
	void SetInterruptAfterIntentPersistenceForAutomation(bool bEnabled)
	{
		Session.SetInterruptAfterIntentPersistenceForAutomation(bEnabled);
	}
	void SetInterruptAfterConditionMutationForAutomation(bool bEnabled)
	{
		Session.SetInterruptAfterConditionMutationForAutomation(bEnabled);
	}
	void SetInterruptAfterProofPersistenceForAutomation(bool bEnabled)
	{
		Session.SetInterruptAfterProofPersistenceForAutomation(bEnabled);
	}
	void SetRecoveryStoreFailureForAutomation(
		Edemo_mapShanmenTreatmentRecoveryStoreFailureStage Stage)
	{
		Session.SetRecoveryStoreFailureForAutomation(Stage);
	}
#endif

private:
	Fdemo_mapShanmenMeridianShockTreatmentRouteResult RejectUnavailable(
		const FGuid& RequestId,
		const TCHAR* Diagnostic) const;

	TWeakObjectPtr<Udemo_mapShanmenItemAuthoritySubsystem> BoundAuthority;
	Fdemo_mapShanmenMeridianShockTreatmentProductSession Session;
};
