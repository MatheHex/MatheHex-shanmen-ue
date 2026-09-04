#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenPlayerActionArbitration.h"
#include "demo_mapShanmenThrownWeaponProductController.h"

class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Device-independent request selecting one frozen active-Run hotbar slot. */
class Fdemo_mapShanmenThrownWeaponHotbarIntent
{
public:
	/** Straight compatibility capture. */
	static bool TryCapture(
		const FGuid& SelectionId,
		int32 HotbarSlotNumber,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance,
		Fdemo_mapShanmenThrownWeaponHotbarIntent& OutIntent);
	/** Arc input owns geometry only; product policy stays in SessionConfig. */
	static bool TryCaptureArc(
		const FGuid& SelectionId,
		int32 HotbarSlotNumber,
		const FVector& Origin,
		const FVector& Target,
		double ApexClearance,
		Fdemo_mapShanmenThrownWeaponHotbarIntent& OutIntent);

	bool IsValid() const;
	bool Matches(const Fdemo_mapShanmenThrownWeaponHotbarIntent& Other) const;
	const FGuid& GetSelectionId() const { return SelectionId; }
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	const FVector& GetOrigin() const { return Origin; }
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	const FVector& GetAimDirection() const { return AimDirection; }
	float GetMaximumDistance() const { return MaximumDistance; }
	const FVector& GetTarget() const { return Target; }
	double GetApexClearance() const { return ApexClearance; }

private:
	FGuid SelectionId;
	int32 HotbarSlotNumber = INDEX_NONE;
	FVector Origin = FVector::ZeroVector;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	FVector AimDirection = FVector::ZeroVector;
	float MaximumDistance = 0.0f;
	FVector Target = FVector::ZeroVector;
	double ApexClearance = 0.0;
};

/** Immutable content policy owned by one thrown-weapon product session. */
class Fdemo_mapShanmenThrownWeaponSessionConfig
{
public:
	static bool TryCapture(
		const FShanmenThrownWeaponDefinitionCapture& Definition,
		const FGameplayTagContainer& SourceTags,
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig);
	static bool TryCaptureArc(
		const FShanmenThrownWeaponDefinitionCapture& Definition,
		const FGameplayTagContainer& SourceTags,
		EShanmenThrownWeaponTechniqueTier TechniqueTier,
		double GravityMagnitude,
		double MaximumFlightTime,
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponSessionConfig& Other) const;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	const FShanmenThrownWeaponDefinitionCapture& GetDefinition() const
	{
		return Definition;
	}
	const FGameplayTagContainer& GetSourceTags() const { return SourceTags; }
	const Fdemo_mapShanmenThrownWeaponArcProductPolicy& GetArcPolicy() const
	{
		return ArcPolicy;
	}

private:
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	FShanmenThrownWeaponDefinitionCapture Definition;
	FGameplayTagContainer SourceTags;
	Fdemo_mapShanmenThrownWeaponArcProductPolicy ArcPolicy;
};

enum class Edemo_mapShanmenThrownWeaponSessionStatus : uint8
{
	Applied,
	RecoveryApplied,
	SessionInactive,
	SessionInvalid,
	RunMismatch,
	RequestInvalid,
	TrajectoryMismatch,
	SelectionIdConflict,
	HotbarSlotEmpty,
	SourceUnavailable,
	ProductCaptureRejected,
	HostResetRejected,
	SelectionNotFound,
	ActionConflict,
	ProductRejected
};

/** Product audit joining one hotbar request to its exact item and action. */
struct Fdemo_mapShanmenThrownWeaponSessionResult
{
	Edemo_mapShanmenThrownWeaponSessionStatus Status =
		Edemo_mapShanmenThrownWeaponSessionStatus::SessionInactive;
	bool bReusedSelection = false;
	FGuid SelectionId;
	FGuid RunId;
	int32 HotbarSlotNumber = INDEX_NONE;
	FGuid ItemInstanceId;
	float TechniquePower = 0.0f;
	Fdemo_mapShanmenPlayerActionGateResult ActionGate;
	Fdemo_mapShanmenThrownWeaponProductResult Product;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsRecoveryApplied() const;
	bool RequiresRecovery() const { return Product.Command.RequiresRecovery(); }
};

/**
 * Active-Run owner of typed hotbar resolution, action stat capture, and the
 * P7.3-P7.5 state. The frozen RunCorrelation is the only hotbar source; this
 * class never reads or writes the legacy item subsystem, input bindings, UI,
 * or inventory.
 */
class Fdemo_mapShanmenThrownWeaponProductSession
{
public:
	Fdemo_mapShanmenThrownWeaponProductSession() = default;
	~Fdemo_mapShanmenThrownWeaponProductSession() = default;

	Fdemo_mapShanmenThrownWeaponProductSession(
		const Fdemo_mapShanmenThrownWeaponProductSession&) = delete;
	Fdemo_mapShanmenThrownWeaponProductSession& operator=(
		const Fdemo_mapShanmenThrownWeaponProductSession&) = delete;
	Fdemo_mapShanmenThrownWeaponProductSession(
		Fdemo_mapShanmenThrownWeaponProductSession&&) = delete;
	Fdemo_mapShanmenThrownWeaponProductSession& operator=(
		Fdemo_mapShanmenThrownWeaponProductSession&&) = delete;

	bool TryBegin(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		AActor& SourceActor,
		const Fdemo_mapShanmenThrownWeaponSessionConfig& Config,
		FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponSessionResult TrySubmitHotbar(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent);

	/** Selection-only durable cancellation recovery; never launches. */
	Fdemo_mapShanmenThrownWeaponSessionResult TryRecoverCancellation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent);

	bool TryInterruptFlight();
	bool TryExpireRange();
	/** Refuses in-flight or unresolved-recovery teardown. */
	bool TryEnd(FString& OutDiagnostic);

	bool IsActive() const { return bActive; }
	bool IsValid() const;
	const FGuid& GetRunId() const { return Correlation.ActiveRunId; }
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const
	{
		return bActive
			? Config.GetTrajectoryKind()
			: Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	}
	int32 NumCapturedSelections() const { return CapturedSelections.Num(); }
	Edemo_mapShanmenThrownWeaponHostState GetHostState() const
	{
		return Host.GetState();
	}
	/** Stable frozen action identity while the Run Host owns an active flight. */
	const FGuid& GetOccupancyOwnerId() const
	{
		return Host.GetActionRuntime().GetAction().GetActivationId();
	}
	const Fdemo_mapShanmenThrownWeaponTerminalReceipt& GetTerminalReceipt() const
	{
		return Host.GetTerminalReceipt();
	}
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* FindCapturedCommand(
		const FGuid& SelectionId) const;

private:
	struct FCapturedSelection
	{
		Fdemo_mapShanmenThrownWeaponHotbarIntent HotbarIntent;
		Fdemo_mapShanmenThrownWeaponSelectionIntent ProductSelection;
		Fdemo_mapShanmenThrownWeaponProductCapture Product;
		Fdemo_mapShanmenThrownWeaponProductResult LastProductResult;
	};

	Fdemo_mapShanmenThrownWeaponSessionResult RouteCaptured(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		FCapturedSelection& Captured,
		bool bReusedSelection);
	void ClearBinding();

	bool bActive = false;
	Fdemo_mapShanmenRunCorrelation Correlation;
	TWeakObjectPtr<AActor> SourceActor;
	Fdemo_mapShanmenThrownWeaponSessionConfig Config;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	Fdemo_mapShanmenThrownWeaponProductController Controller;
	TMap<FGuid, FCapturedSelection> CapturedSelections;
};
