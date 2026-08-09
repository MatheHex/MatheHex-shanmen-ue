#pragma once

#include "CoreMinimal.h"
#include "demo_mapM01Extraction.generated.h"

/** Stable M01 identities. Future map content must reference these names, never array indices. */
struct Fdemo_mapM01Ids
{
	static const FName Map;
	static const FName RiskLow;
	static const FName RiskMid;
	static const FName RiskHigh;
	static const FName ResourceTier1;
	static const FName ResourceTier2;
	static const FName ResourceTier3;
	static const FName ExitDiscardSpatial;
	static const FName ExitRegular;
	static const FName ExitBoss;
	static const FName MainBoss;

	static bool Validate(FString* OutError = nullptr);
};

UENUM(BlueprintType)
enum class Edemo_mapM01Risk : uint8
{
	Low,
	Mid,
	High
};

UENUM(BlueprintType)
enum class Edemo_mapM01ResourceTier : uint8
{
	Tier1,
	Tier2,
	Tier3
};

UENUM(BlueprintType)
enum class Edemo_mapM01ExitType : uint8
{
	DiscardSpatial,
	Regular,
	Boss
};

enum class Edemo_mapM01ExtractionState : uint8
{
	Unavailable,
	Locked,
	Available,
	CountingDown,
	Cancelled,
	Completed
};

enum class Edemo_mapM01ExtractionCancelReason : uint8
{
	None,
	LeftRange,
	Damaged,
	PlayerDefeated,
	RunTerminal,
	ConditionInvalidated
};

struct Fdemo_mapM01ExtractionSnapshot
{
	Edemo_mapM01ExitType ExitType = Edemo_mapM01ExitType::Regular;
	FName ExitId = NAME_None;
	Edemo_mapM01ExtractionState State = Edemo_mapM01ExtractionState::Unavailable;
	Edemo_mapM01ExtractionCancelReason CancelReason = Edemo_mapM01ExtractionCancelReason::None;
	float RemainingSeconds = 0.0f;
	bool bInRange = false;
	bool bConditionSatisfied = false;

	bool CanBegin() const
	{
		return bInRange
			&& bConditionSatisfied
			&& (State == Edemo_mapM01ExtractionState::Available
				|| State == Edemo_mapM01ExtractionState::Cancelled);
	}
};

/**
 * Run-scoped authority for all three exits. It stores no Profile state and emits
 * at most one completion token per Run; GameMode remains the Settlement owner.
 */
class Fdemo_mapM01ExtractionAuthority
{
public:
	static constexpr float CountdownSeconds = 3.0f;

	Fdemo_mapM01ExtractionAuthority();

	void ResetForNewRun(bool bSpatialItemEquipped);
	bool NotifyBossDefeated(FName BossId);
	void SetSpatialItemEquipped(bool bEquipped);
	void SetInRange(Edemo_mapM01ExitType ExitType, bool bInRange);
	bool BeginInteraction(Edemo_mapM01ExitType ExitType, FString* OutDiagnostic = nullptr);
	void NotifyEffectiveDamage();
	void NotifyPlayerDefeated();
	void NotifyRunTerminal();
	bool Advance(float DeltaSeconds, Edemo_mapM01ExitType& OutCompletedExit);

	Fdemo_mapM01ExtractionSnapshot GetSnapshot(Edemo_mapM01ExitType ExitType) const;
	FString FormatStatus(Edemo_mapM01ExitType ExitType) const;
	bool IsBossUnlocked() const { return bBossDefeated; }
	bool IsRunTerminal() const { return bRunTerminal; }
	bool HasIssuedCompletion() const { return bCompletionIssued; }

	static FName ExitId(Edemo_mapM01ExitType ExitType);
	static FString ExitLabel(Edemo_mapM01ExitType ExitType);

private:
	struct FRuntime
	{
		Edemo_mapM01ExtractionState State = Edemo_mapM01ExtractionState::Unavailable;
		Edemo_mapM01ExtractionCancelReason CancelReason = Edemo_mapM01ExtractionCancelReason::None;
		float RemainingSeconds = 0.0f;
		bool bInRange = false;
	};

	bool IsConditionSatisfied(Edemo_mapM01ExitType ExitType) const;
	void CancelCounting(Edemo_mapM01ExtractionCancelReason Reason);
	FRuntime& Runtime(Edemo_mapM01ExitType ExitType);
	const FRuntime& Runtime(Edemo_mapM01ExitType ExitType) const;

	TMap<Edemo_mapM01ExitType, FRuntime> Exits;
	TOptional<Edemo_mapM01ExitType> CountingExit;
	bool bSpatialItemEquipped = false;
	bool bBossDefeated = false;
	bool bRunTerminal = false;
	bool bCompletionIssued = false;
};
