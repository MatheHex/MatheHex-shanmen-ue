#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritEvasion.h"

#include "ShanmenSpiritEvasionMovement.generated.h"

/** Mutable input sample captured by a product input adapter. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionMovementIntentCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FShanmenCombatActionSnapshot Action;

	/** Product-owned policy key; distance, duration and trajectory live behind it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FName MovementPolicyId = NAME_None;

	/** Input direction only. Z is deliberately discarded during capture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FVector CandidateDirection = FVector::ZeroVector;
};

/** Immutable, action-bound directional intent with no world or movement values. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionMovementIntent
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenSpiritEvasionMovementIntentCapture& Capture,
		FShanmenSpiritEvasionMovementIntent& OutIntent);

	bool IsValid() const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	FName GetMovementPolicyId() const { return MovementPolicyId; }
	const FVector& GetPlanarDirection() const { return PlanarDirection; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGuid IntentId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FName MovementPolicyId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FVector PlanarDirection = FVector::ZeroVector;
};

/** Immutable delivery request emitted only while the exact evasion window is Active. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionMovementRequest
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetRequestId() const { return RequestId; }
	const FShanmenSpiritEvasionMovementIntent& GetIntent() const { return Intent; }
	const FShanmenSpiritEvasionWindowReceipt& GetWindow() const { return Window; }

private:
	friend struct FShanmenSpiritEvasionMovementPlanner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGuid RequestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritEvasionMovementIntent Intent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritEvasionWindowReceipt Window;
};

/**
 * Pure seam between product input and a future movement-policy adapter.
 *
 * It owns no distance, duration, collision query, timer, resource balance,
 * Actor, World or movement mutation. The P10.0 action window remains the only
 * lifetime authority.
 */
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionMovementPlanner
{
	static bool TryCreateRequest(
		const FShanmenSpiritEvasionMovementIntent& Intent,
		const FShanmenSpiritEvasionWindow& Window,
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenSpiritEvasionMovementRequest& OutRequest);
};

/** Idempotency gate for repeated input/product bridge delivery callbacks. */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionMovementLedger
{
public:
	bool TryAccept(const FShanmenSpiritEvasionMovementRequest& Request);
	bool Contains(const FGuid& RequestId) const;
	int32 Num() const;
	void Reset();

private:
	TSet<FGuid> AcceptedRequestIds;
};
