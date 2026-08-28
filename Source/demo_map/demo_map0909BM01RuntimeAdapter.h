#pragma once

#include "CoreMinimal.h"
#include "demo_map0909BFrameworkTypes.h"

class Ademo_mapGameMode;
class Ademo_mapPlayerController;

/**
 * Narrow adapter around the authored M01 world.  It owns map/readiness checks
 * and has no authority to commit a Profile or a Code B terminal result.
 */
class Fdemo_map0909BM01RuntimeAdapter
{
public:
	/** The sole authored M01 descriptor accepted by the current 0.0.9B shell. */
	static const TCHAR* ExpectedM01MapDescriptor();
	static bool ValidateM01DescriptorConfiguration(FString& OutDiagnostic);

	void Initialize(
		Ademo_mapGameMode* InGameMode,
		Ademo_mapPlayerController* InController);

	/**
	 * Accepts exactly one Coordinator-owned attempt and requests the existing
	 * authored M01 materialization seam. It reports no RuntimeReady claim.
	 */
	bool BeginActivation(
		const FGuid& StartAttemptId,
		Fdemo_map0909BM01RuntimeReceipt& OutReceipt);

	/**
	 * Formal readiness callback point. It rechecks the currently live M01
	 * World, GameMode/WorldSettings, controller, pawn and restored input for
	 * the exact pending attempt; stale or duplicate signals are ignored.
	 */
	bool ConfirmRuntimeReady(
		const FGuid& StartAttemptId,
		Fdemo_map0909BM01RuntimeReceipt& OutReceipt);

	/** Idempotently releases only the matching transient attempt. */
	bool CancelAttempt(const FGuid& StartAttemptId, FString& OutDiagnostic);
	bool IsAttemptPending(const FGuid& StartAttemptId) const;

private:
	bool CollectWorldFacts(
		const FGuid& StartAttemptId,
		const FGuid& ExpectedOwnerId,
		const FGuid& ExpectedRunId,
		bool bRequireRestoredInput,
		Fdemo_map0909BM01RuntimeReceipt& OutReceipt) const;
	void StartReceipt(
		const FGuid& StartAttemptId,
		Fdemo_map0909BM01RuntimeReceipt& OutReceipt);
	void MarkTechnicalFailure(
		Fdemo_map0909BM01RuntimeReceipt& InOutReceipt,
		const FString& FailureClass,
		const FString& Detail) const;

	TWeakObjectPtr<Ademo_mapGameMode> GameMode;
	TWeakObjectPtr<Ademo_mapPlayerController> Controller;
	TOptional<Fdemo_map0909BM01RuntimeReceipt> ActiveAttempt;
	int64 NextReceiptSequence = 0;
};
