#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

/**
 * Immutable caller-supplied plane used to project an Arc choice.
 * Capture normalizes both axes and rejects skew/parallel input.
 */
class Fdemo_mapShanmenThrownWeaponArcChoiceBasis
{
public:
	static bool TryCapture(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& Right,
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis& OutBasis);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& Other) const;
	const FGuid& GetBasisId() const { return BasisId; }
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetForward() const { return Forward; }
	const FVector& GetRight() const { return Right; }

private:
	FGuid BasisId;
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ZeroVector;
	FVector Right = FVector::ZeroVector;
};

/**
 * Frozen content policy for mapping normalized target and apex values.
 *
 * Target X is signed lateral intent. Target Y maps [-1, 1] onto the
 * minimum/maximum forward range. Accumulated apex adjustment maps [-1, 1]
 * onto the minimum/maximum positive apex clearance.
 */
class Fdemo_mapShanmenThrownWeaponArcChoicePolicy
{
public:
	static bool TryCapture(
		double MinimumForwardDistance,
		double MaximumForwardDistance,
		double MaximumLateralOffset,
		double MinimumApexClearance,
		double MaximumApexClearance,
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy& OutPolicy);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Other) const;
	const FGuid& GetPolicyId() const { return PolicyId; }
	double GetMinimumForwardDistance() const
	{
		return MinimumForwardDistance;
	}
	double GetMaximumForwardDistance() const
	{
		return MaximumForwardDistance;
	}
	double GetMaximumLateralOffset() const
	{
		return MaximumLateralOffset;
	}
	double GetMinimumApexClearance() const
	{
		return MinimumApexClearance;
	}
	double GetMaximumApexClearance() const
	{
		return MaximumApexClearance;
	}

private:
	FGuid PolicyId;
	double MinimumForwardDistance = 0.0;
	double MaximumForwardDistance = 0.0;
	double MaximumLateralOffset = 0.0;
	double MinimumApexClearance = 0.0;
	double MaximumApexClearance = 0.0;
};

enum class Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus : uint8
{
	Invalid,
	Projected,
	ChoiceStateInvalid,
	TrajectoryNotArc,
	TargetIntentMissing,
	BasisInvalid,
	PolicyInvalid,
	OutputRejected
};

/** Immutable pure-value projection ready for an Arc input adapter caller. */
class Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult
{
public:
	bool IsValid() const;
	bool IsProjected() const { return IsValid(); }
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult& Other) const;
	Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetProjectionId() const { return ProjectionId; }
	const FGuid& GetChoiceStateId() const { return ChoiceStateId; }
	uint64 GetChoiceRevision() const { return ChoiceRevision; }
	const FGuid& GetBasisId() const { return BasisId; }
	const FGuid& GetPolicyId() const { return PolicyId; }
	const FVector& GetTarget() const { return Target; }
	double GetApexClearance() const { return ApexClearance; }
	double GetForwardDistance() const { return ForwardDistance; }
	double GetLateralOffset() const { return LateralOffset; }

private:
	friend class Fdemo_mapShanmenThrownWeaponArcChoiceProjector;
	static Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Reject(
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus Status,
		const TCHAR* Diagnostic);

	Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus Status =
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Invalid;
	FString Diagnostic;
	FGuid ProjectionId;
	FGuid ChoiceStateId;
	uint64 ChoiceRevision = MAX_uint64;
	FGuid BasisId;
	FGuid PolicyId;
	FVector Target = FVector::ZeroVector;
	double ApexClearance = 0.0;
	double ForwardDistance = 0.0;
	double LateralOffset = 0.0;
};

/** Pure projector. It never queries World, traces, input devices, or UI. */
class Fdemo_mapShanmenThrownWeaponArcChoiceProjector
{
public:
	static Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Project(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& Basis,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy);
};
