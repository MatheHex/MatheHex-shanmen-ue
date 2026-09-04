#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponRunCommandRouter.h"

class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Input-device-independent choice of one exact item and explicit trajectory. */
class Fdemo_mapShanmenThrownWeaponSelectionIntent
{
public:
	/** Straight compatibility capture. */
	static bool TryCapture(
		const FGuid& SelectionId,
		const FGuid& RunId,
		const FGuid& SourceItemInstanceId,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance,
		Fdemo_mapShanmenThrownWeaponSelectionIntent& OutIntent);
	/** Arc input owns geometry only; product policy and action identity stay external. */
	static bool TryCaptureArc(
		const FGuid& SelectionId,
		const FGuid& RunId,
		const FGuid& SourceItemInstanceId,
		const FVector& Origin,
		const FVector& Target,
		double ApexClearance,
		Fdemo_mapShanmenThrownWeaponSelectionIntent& OutIntent);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponSelectionIntent& Other) const;
	const FGuid& GetSelectionId() const { return SelectionId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	const FVector& GetOrigin() const { return Origin; }
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	const FVector& GetAimDirection() const { return AimDirection; }
	float GetMaximumDistance() const { return MaximumDistance; }
	const FVector& GetTarget() const { return Target; }
	double GetApexClearance() const { return ApexClearance; }

private:
	FGuid SelectionId;
	FGuid RunId;
	FGuid SourceItemInstanceId;
	FVector Origin = FVector::ZeroVector;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	FVector AimDirection = FVector::ZeroVector;
	float MaximumDistance = 0.0f;
	FVector Target = FVector::ZeroVector;
	double ApexClearance = 0.0;
};

/** Product-owned Arc envelope; no input device may choose these values. */
class Fdemo_mapShanmenThrownWeaponArcProductPolicy
{
public:
	static bool TryCapture(
		EShanmenThrownWeaponTechniqueTier TechniqueTier,
		double GravityMagnitude,
		double MaximumFlightTime,
		Fdemo_mapShanmenThrownWeaponArcProductPolicy& OutPolicy);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& Other) const;
	EShanmenThrownWeaponTechniqueTier GetTechniqueTier() const
	{
		return TechniqueTier;
	}
	double GetGravityMagnitude() const { return GravityMagnitude; }
	double GetMaximumFlightTime() const { return MaximumFlightTime; }

private:
	EShanmenThrownWeaponTechniqueTier TechniqueTier =
		EShanmenThrownWeaponTechniqueTier::Beginner;
	double GravityMagnitude = 0.0;
	double MaximumFlightTime = 0.0;
};

/** Product-owned combat values frozen independently from input selection. */
class Fdemo_mapShanmenThrownWeaponProductCapture
{
public:
	/** Straight compatibility capture. */
	static bool TryCapture(
		const FShanmenThrownWeaponDefinitionCapture& Definition,
		float TechniquePower,
		const FGameplayTagContainer& SourceTags,
		Fdemo_mapShanmenThrownWeaponProductCapture& OutCapture);
	static bool TryCaptureArc(
		const FShanmenThrownWeaponDefinitionCapture& Definition,
		float TechniquePower,
		const FGameplayTagContainer& SourceTags,
		EShanmenThrownWeaponTechniqueTier TechniqueTier,
		double GravityMagnitude,
		double MaximumFlightTime,
		Fdemo_mapShanmenThrownWeaponProductCapture& OutCapture);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponProductCapture& Other) const;
	const FShanmenThrownWeaponDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenThrownWeaponOffenseSnapshot& GetOffense() const
	{
		return Offense;
	}
	const FGameplayTagContainer& GetSourceTags() const
	{
		return SourceTags;
	}
	const Fdemo_mapShanmenThrownWeaponArcProductPolicy& GetArcPolicy() const
	{
		return ArcPolicy;
	}

private:
	FShanmenThrownWeaponDefinition Definition;
	FShanmenThrownWeaponOffenseSnapshot Offense;
	FGameplayTagContainer SourceTags;
	Fdemo_mapShanmenThrownWeaponArcProductPolicy ArcPolicy;
};

enum class Edemo_mapShanmenThrownWeaponProductStatus : uint8
{
	Applied,
	RecoveryApplied,
	CoordinatorNotReady,
	SelectionInvalid,
	CaptureInvalid,
	RunMismatch,
	ControllerInvalid,
	ControllerRunMismatch,
	AuthorityNotReady,
	SnapshotUnavailable,
	SnapshotStale,
	ItemNotFound,
	DefinitionNotThrownWeapon,
	TrajectoryMismatch,
	SequenceReservationRejected,
	ActionCaptureRejected,
	ArcPlanRejected,
	CommandCaptureRejected,
	SelectionIdConflict,
	SelectionNotFound,
	RouterRejected
};

/** Product-level audit joining one selection to its Run-owned action identity. */
struct Fdemo_mapShanmenThrownWeaponProductResult
{
	Edemo_mapShanmenThrownWeaponProductStatus Status =
		Edemo_mapShanmenThrownWeaponProductStatus::CoordinatorNotReady;
	bool bReusedSelection = false;
	FGuid SelectionId;
	FGuid RunId;
	FGuid SourceItemInstanceId;
	uint64 ActivationSequence = 0;
	FGuid ActivationId;
	Fdemo_mapShanmenThrownWeaponRunCommandResult Command;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsRecoveryApplied() const;
	bool HasCapturedAction() const;
};

/**
 * Run-scoped facade between trajectory selection and the P7.4 Router.
 *
 * Input owns only SelectionIntent. Product definition/offense are captured by
 * the product boundary, exact item tags/content are read from ShanmenItems,
 * and action sequence is reserved by CombatRunCoordinator. Arc plans are
 * derived only after that Run-owned identity exists. A SelectionId is
 * permanently mapped to one frozen command or planning rejection, so retries
 * reuse the same action while payload conflicts consume no new sequence.
 */
class Fdemo_mapShanmenThrownWeaponProductController
{
public:
	Fdemo_mapShanmenThrownWeaponProductResult TrySubmit(
		Fdemo_mapShanmenThrownWeaponRunHost& Host,
		Fdemo_mapShanmenThrownWeaponRunCommandRouter& Router,
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection,
		const Fdemo_mapShanmenThrownWeaponProductCapture& Product);

	/** Selection-only recovery delegates to P7.4 and can never relaunch. */
	Fdemo_mapShanmenThrownWeaponProductResult TryRecoverCancellation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapShanmenThrownWeaponRunCommandRouter& Router,
		const Fdemo_mapShanmenThrownWeaponSelectionIntent& Selection);

	bool IsEmpty() const
	{
		return CapturedSelections.IsEmpty()
			&& RejectedArcSelections.IsEmpty();
	}
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	int32 NumCapturedSelections() const
	{
		return CapturedSelections.Num() + RejectedArcSelections.Num();
	}
	/** Read-only audit access; callers still submit only SelectionIntent. */
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* FindCapturedCommand(
		const FGuid& SelectionId) const;
	void Reset();

private:
	struct FCapturedSelection
	{
		Fdemo_mapShanmenThrownWeaponSelectionIntent Selection;
		Fdemo_mapShanmenThrownWeaponProductCapture Product;
		uint64 ActivationSequence = 0;
		Fdemo_mapShanmenThrownWeaponRunCommandIntent Command;
	};
	struct FRejectedArcSelection
	{
		Fdemo_mapShanmenThrownWeaponSelectionIntent Selection;
		Fdemo_mapShanmenThrownWeaponProductCapture Product;
		uint64 ActivationSequence = 0;
		FShanmenCombatActionSnapshot Action;
		FString Diagnostic;
	};

	FGuid RunId;
	TMap<FGuid, FCapturedSelection> CapturedSelections;
	TMap<FGuid, FRejectedArcSelection> RejectedArcSelections;
};
