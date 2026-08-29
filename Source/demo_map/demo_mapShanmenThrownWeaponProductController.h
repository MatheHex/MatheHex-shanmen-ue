#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponRunCommandRouter.h"

class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Input-device-independent choice of one exact item and straight trajectory. */
class Fdemo_mapShanmenThrownWeaponSelectionIntent
{
public:
	static bool TryCapture(
		const FGuid& SelectionId,
		const FGuid& RunId,
		const FGuid& SourceItemInstanceId,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance,
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
	const FVector& GetAimDirection() const { return AimDirection; }
	float GetMaximumDistance() const { return MaximumDistance; }

private:
	FGuid SelectionId;
	FGuid RunId;
	FGuid SourceItemInstanceId;
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ZeroVector;
	float MaximumDistance = 0.0f;
};

/** Product-owned combat values frozen independently from input selection. */
class Fdemo_mapShanmenThrownWeaponProductCapture
{
public:
	static bool TryCapture(
		const FShanmenThrownWeaponDefinitionCapture& Definition,
		float TechniquePower,
		const FGameplayTagContainer& SourceTags,
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

private:
	FShanmenThrownWeaponDefinition Definition;
	FShanmenThrownWeaponOffenseSnapshot Offense;
	FGameplayTagContainer SourceTags;
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
	SequenceReservationRejected,
	ActionCaptureRejected,
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
 * Run-scoped facade between future selection input and the P7.4 Router.
 *
 * Input owns only SelectionIntent. Product definition/offense are captured by
 * the product boundary, exact item tags/content are read from ShanmenItems,
 * and action sequence is reserved by CombatRunCoordinator. A SelectionId is
 * permanently mapped to one frozen P7.4 intent, so transient retries reuse the
 * same action while payload conflicts consume no new sequence.
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

	bool IsEmpty() const { return CapturedSelections.IsEmpty(); }
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	int32 NumCapturedSelections() const { return CapturedSelections.Num(); }
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

	FGuid RunId;
	TMap<FGuid, FCapturedSelection> CapturedSelections;
};
