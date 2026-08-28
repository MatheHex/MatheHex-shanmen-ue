#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"

#include "ShanmenActionOrchestrator.generated.h"

UENUM(BlueprintType)
enum class EShanmenActionTerminalReason : uint8
{
	None,
	Completed,
	Cancelled,
	Interrupted
};

/** Immutable audit record for one accepted action-phase transition. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionTransitionReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetActivationId() const { return ActivationId; }
	int64 GetSequence() const { return Sequence; }
	EShanmenCombatActionPhase GetFromPhase() const { return FromPhase; }
	EShanmenCombatActionPhase GetToPhase() const { return ToPhase; }
	EShanmenActionTerminalReason GetTerminalReason() const { return TerminalReason; }
	bool CrossedCommitPointNow() const { return bCrossedCommitPointNow; }
	bool HasReachedCommitPoint() const { return bCommitPointReached; }

private:
	friend class FShanmenActionOrchestrator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	FGuid ActivationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	int64 Sequence = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	EShanmenCombatActionPhase FromPhase = EShanmenCombatActionPhase::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	EShanmenCombatActionPhase ToPhase = EShanmenCombatActionPhase::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	EShanmenActionTerminalReason TerminalReason = EShanmenActionTerminalReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	bool bCrossedCommitPointNow = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Action", meta = (AllowPrivateAccess = "true"))
	bool bCommitPointReached = false;
};

/**
 * Pure, deterministic lifecycle authority for one frozen combat activation.
 *
 * GAS owns when transitions are requested. This object owns whether they are
 * legal. Startup -> Active is the one commit point; only Active may emit hit
 * candidates. World queries, damage math and item mutations remain external.
 */
class SHANMENCOMBATRUNTIME_API FShanmenActionOrchestrator
{
public:
	static bool TryStart(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutOrchestrator,
		FShanmenActionTransitionReceipt& OutReceipt);

	bool IsValid() const;
	bool IsTerminal() const { return TerminalReason != EShanmenActionTerminalReason::None; }
	bool CanEmitCandidates() const;
	bool HasReachedCommitPoint() const { return bCommitPointReached; }
	EShanmenCombatActionPhase GetPhase() const { return Phase; }
	EShanmenActionTerminalReason GetTerminalReason() const { return TerminalReason; }
	int64 GetNextSequence() const { return NextSequence; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }

	bool TryAdvance(
		EShanmenCombatActionPhase ExpectedPhase,
		FShanmenActionTransitionReceipt& OutReceipt);
	bool TryCancel(
		EShanmenCombatActionPhase ExpectedPhase,
		FShanmenActionTransitionReceipt& OutReceipt);
	bool TryInterrupt(
		EShanmenCombatActionPhase ExpectedPhase,
		FShanmenActionTransitionReceipt& OutReceipt);
	void Reset();

private:
	bool EmitTransition(
		EShanmenCombatActionPhase ToPhase,
		EShanmenActionTerminalReason NewTerminalReason,
		bool bCrossCommitPoint,
		FShanmenActionTransitionReceipt& OutReceipt);

	FShanmenCombatActionSnapshot Action;
	EShanmenCombatActionPhase Phase = EShanmenCombatActionPhase::Idle;
	EShanmenActionTerminalReason TerminalReason = EShanmenActionTerminalReason::None;
	int64 NextSequence = 0;
	bool bStarted = false;
	bool bCommitPointReached = false;
};
