#pragma once

#include "CoreMinimal.h"
#include "demo_map0909BFrameworkTypes.h"
#include "demo_map0909BM01RuntimeAdapter.h"

class Ademo_mapGameMode;
class Ademo_mapPlayerController;

/**
 * Audited deployment coordinator. It is the only 0.0.9B product path allowed
 * to advance AtSect -> PreparingStart -> ActivatingWorld -> InRun.
 */
class Fdemo_map0909BRunStartCoordinator
{
public:
	void Initialize(
		Ademo_mapGameMode* InGameMode,
		Ademo_mapPlayerController* InController);

	bool StartM01Run(FString& OutPlayerFeedback);
	bool IsAtSect() const { return State == Edemo_map0909BTopState::AtSect; }
	Edemo_map0909BTopState GetState() const { return State; }
	const Fdemo_map0909BStartDiagnostic& GetLastDiagnostic() const { return LastDiagnostic; }

private:
	void BeginAttempt();
	void Transition(Edemo_map0909BTopState NewState, const FString& SequenceStep);
	void RecordRuntimeReceipt(const Fdemo_map0909BM01RuntimeReceipt& Receipt);
	bool ValidateRuntimeReady(
		const Fdemo_map0909BM01RuntimeReceipt& Receipt,
		FString& OutDiagnostic) const;
	bool ReturnToSectAfterTechnicalFailure(
		const FString& FailureClass,
		const FString& Detail,
		FString& OutPlayerFeedback);

	TWeakObjectPtr<Ademo_mapGameMode> GameMode;
	TWeakObjectPtr<Ademo_mapPlayerController> Controller;
	TUniquePtr<Fdemo_map0909BM01RuntimeAdapter> M01Adapter;
	Edemo_map0909BTopState State = Edemo_map0909BTopState::AtSect;
	Fdemo_map0909BStartDiagnostic LastDiagnostic;
	Fdemo_mapShanmenRunCorrelation AttemptRunCorrelation;
	int64 NextAttemptSequence = 0;
};
