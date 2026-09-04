#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"

#include "ShanmenThrownWeaponArcPlanner.generated.h"

/** Operation tiers frozen by the 0.0.10 Hidden Weapon Mastery direction. */
UENUM(BlueprintType)
enum class EShanmenThrownWeaponTechniqueTier : uint8
{
	Beginner,
	Intermediate,
	Master
};

/** Mutable authoring input for one manual ballistic-arc request. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcRequestCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	EShanmenThrownWeaponTechniqueTier TechniqueTier =
		EShanmenThrownWeaponTechniqueTier::Beginner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon")
	FVector Target = FVector::ZeroVector;

	/** Positive world-down acceleration magnitude. The planner itself owns no World. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	double GravityMagnitude = 0.0;

	/** Requested apex height above the higher endpoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	double ApexClearance = 0.0;

	/** Content/authority envelope; this contract does not choose final tuning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	double MaximumLaunchSpeed = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|ThrownWeapon", meta = (ClampMin = "0.0"))
	double MaximumFlightTime = 0.0;
};

/** Immutable, identity-bound request for a manually selected ballistic arc. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcRequest
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenThrownWeaponArcRequestCapture& Capture,
		FShanmenThrownWeaponArcRequest& OutRequest);

	bool IsValid() const;
	bool Matches(const FShanmenThrownWeaponArcRequest& Other) const;
	bool IsArcUnlocked() const
	{
		return IsValid()
			&& TechniqueTier
				!= EShanmenThrownWeaponTechniqueTier::Beginner;
	}

	const FGuid& GetRequestId() const { return RequestId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	EShanmenThrownWeaponTechniqueTier GetTechniqueTier() const
	{
		return TechniqueTier;
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetTarget() const { return Target; }
	double GetGravityMagnitude() const { return GravityMagnitude; }
	double GetApexClearance() const { return ApexClearance; }
	double GetMaximumLaunchSpeed() const { return MaximumLaunchSpeed; }
	double GetMaximumFlightTime() const { return MaximumFlightTime; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FGuid RequestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	EShanmenThrownWeaponTechniqueTier TechniqueTier =
		EShanmenThrownWeaponTechniqueTier::Beginner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector Target = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double GravityMagnitude = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double ApexClearance = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double MaximumLaunchSpeed = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double MaximumFlightTime = 0.0;
};

/** Deterministic ballistic plan. It is geometry only and owns no projectile. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcPlan
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(const FShanmenThrownWeaponArcPlan& Other) const;
	bool TrySamplePosition(double ElapsedSeconds, FVector& OutPosition) const;

	const FGuid& GetPlanId() const { return PlanId; }
	const FShanmenThrownWeaponArcRequest& GetRequest() const
	{
		return Request;
	}
	const FVector& GetInitialVelocity() const { return InitialVelocity; }
	const FVector& GetGravityAcceleration() const
	{
		return GravityAcceleration;
	}
	const FVector& GetApexPosition() const { return ApexPosition; }
	double GetLaunchSpeed() const { return LaunchSpeed; }
	double GetTimeToApexSeconds() const { return TimeToApexSeconds; }
	double GetFlightTimeSeconds() const { return FlightTimeSeconds; }

private:
	friend class FShanmenThrownWeaponArcPlanner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FGuid PlanId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FShanmenThrownWeaponArcRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector InitialVelocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector GravityAcceleration = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	FVector ApexPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double LaunchSpeed = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double TimeToApexSeconds = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|ThrownWeapon", meta = (AllowPrivateAccess = "true"))
	double FlightTimeSeconds = 0.0;
};

enum class EShanmenThrownWeaponArcPlanStatus : uint8
{
	Invalid,
	Planned,
	RequestRejected,
	TechniqueLocked,
	Unreachable
};

/** Typed outcome for one pure planning attempt. */
struct SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcPlanResult
{
	EShanmenThrownWeaponArcPlanStatus Status =
		EShanmenThrownWeaponArcPlanStatus::Invalid;
	FString Diagnostic;
	FShanmenThrownWeaponArcRequest Request;
	FShanmenThrownWeaponArcPlan Plan;

	bool IsValid() const;
	bool IsPlanned() const
	{
		return IsValid()
			&& Status == EShanmenThrownWeaponArcPlanStatus::Planned;
	}
};

/** Pure fixed-apex ballistic solver for the intermediate mastery operation. */
class SHANMENCOMBATRUNTIME_API FShanmenThrownWeaponArcPlanner
{
public:
	static FShanmenThrownWeaponArcPlanResult Plan(
		const FShanmenThrownWeaponArcRequestCapture& Capture);
	static FShanmenThrownWeaponArcPlanResult Plan(
		const FShanmenThrownWeaponArcRequest& Request);
};
