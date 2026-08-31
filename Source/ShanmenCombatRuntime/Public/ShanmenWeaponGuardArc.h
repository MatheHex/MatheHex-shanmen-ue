#pragma once

#include "CoreMinimal.h"
#include "ShanmenWeaponPerfectGuard.h"

#include "ShanmenWeaponGuardArc.generated.h"

UENUM(BlueprintType)
enum class EShanmenWeaponGuardArcStatus : uint8
{
	Invalid,
	Qualified,
	OutsideArc
};

/** Immutable content-owned facing threshold for one exact guard window. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardArcPolicy
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenWeaponGuardWindowReceipt& Window,
		FName ArcRuleId,
		double MinimumFacingDot,
		FShanmenWeaponGuardArcPolicy& OutPolicy);

	bool IsValid() const;
	const FGuid& GetPolicyId() const { return PolicyId; }
	const FShanmenWeaponGuardWindowReceipt& GetWindow() const { return Window; }
	FName GetArcRuleId() const { return ArcRuleId; }
	double GetMinimumFacingDot() const { return MinimumFacingDot; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid PolicyId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardWindowReceipt Window;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FName ArcRuleId = NAME_None;

	/** Inclusive dot threshold in [-1,1]; authored balance remains external. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	double MinimumFacingDot = 0.0;
};

/**
 * Immutable contact-bound direction sample.
 * DirectionToThreat points from the defending entity toward the threat.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardThreatSample
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenHitCandidate& Candidate,
		const FVector& GuardFacing,
		const FVector& DirectionToThreat,
		FShanmenWeaponGuardThreatSample& OutSample);

	bool IsValid() const;
	const FGuid& GetSampleId() const { return SampleId; }
	const FShanmenHitCandidate& GetCandidate() const { return Candidate; }
	const FVector& GetGuardFacing() const { return GuardFacing; }
	const FVector& GetDirectionToThreat() const { return DirectionToThreat; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid SampleId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenHitCandidate Candidate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FVector GuardFacing = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FVector DirectionToThreat = FVector::ZeroVector;
};

/** Valid result of one directional qualification, including outside-arc proof. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardArcEvaluation
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool IsQualified() const
	{
		return IsValid()
			&& Status == EShanmenWeaponGuardArcStatus::Qualified;
	}
	bool HasLayer() const { return IsQualified() && Layer.IsValid(); }
	const FGuid& GetEvaluationId() const { return EvaluationId; }
	EShanmenWeaponGuardArcStatus GetStatus() const { return Status; }
	const FShanmenWeaponGuardArcPolicy& GetPolicy() const { return Policy; }
	const FShanmenWeaponGuardTimingProjectionReceipt& GetTimingProjection() const
	{
		return TimingProjection;
	}
	const FShanmenWeaponGuardThreatSample& GetSample() const { return Sample; }
	double GetAlignmentDot() const { return AlignmentDot; }
	const FShanmenDefenseLayer& GetLayer() const { return Layer; }

private:
	friend class FShanmenWeaponGuardArcEvaluator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid EvaluationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	EShanmenWeaponGuardArcStatus Status = EShanmenWeaponGuardArcStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardArcPolicy Policy;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardTimingProjectionReceipt TimingProjection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardThreatSample Sample;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	double AlignmentDot = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenDefenseLayer Layer;
};

/**
 * Stateless direction gate over P11.1's already-selected timing layer.
 * It performs no World query, transform lookup, input read or layer mutation.
 */
class SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardArcEvaluator
{
public:
	static bool TryEvaluate(
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenWeaponGuardArcPolicy& Policy,
		const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection,
		const FShanmenWeaponGuardThreatSample& Sample,
		FShanmenWeaponGuardArcEvaluation& OutEvaluation);
};
