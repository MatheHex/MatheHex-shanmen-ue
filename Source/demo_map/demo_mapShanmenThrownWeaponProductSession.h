#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponProductController.h"

class UWorld;
class Udemo_mapShanmenItemAuthoritySubsystem;

/** Device-independent request selecting one frozen active-Run hotbar slot. */
class Fdemo_mapShanmenThrownWeaponHotbarIntent
{
public:
	static bool TryCapture(
		const FGuid& SelectionId,
		int32 HotbarSlotNumber,
		const FVector& Origin,
		const FVector& AimDirection,
		float MaximumDistance,
		Fdemo_mapShanmenThrownWeaponHotbarIntent& OutIntent);

	bool IsValid() const;
	bool Matches(const Fdemo_mapShanmenThrownWeaponHotbarIntent& Other) const;
	const FGuid& GetSelectionId() const { return SelectionId; }
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetAimDirection() const { return AimDirection; }
	float GetMaximumDistance() const { return MaximumDistance; }

private:
	FGuid SelectionId;
	int32 HotbarSlotNumber = INDEX_NONE;
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ZeroVector;
	float MaximumDistance = 0.0f;
};

/** Immutable content policy owned by one thrown-weapon product session. */
class Fdemo_mapShanmenThrownWeaponSessionConfig
{
public:
	static bool TryCapture(
		const FShanmenThrownWeaponDefinitionCapture& Definition,
		const FGameplayTagContainer& SourceTags,
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponSessionConfig& Other) const;
	const FShanmenThrownWeaponDefinitionCapture& GetDefinition() const
	{
		return Definition;
	}
	const FGameplayTagContainer& GetSourceTags() const { return SourceTags; }

private:
	FShanmenThrownWeaponDefinitionCapture Definition;
	FGameplayTagContainer SourceTags;
};

enum class Edemo_mapShanmenThrownWeaponSessionStatus : uint8
{
	Applied,
	RecoveryApplied,
	SessionInactive,
	SessionInvalid,
	RunMismatch,
	RequestInvalid,
	SelectionIdConflict,
	HotbarSlotEmpty,
	SourceUnavailable,
	ProductCaptureRejected,
	HostResetRejected,
	SelectionNotFound,
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
	Fdemo_mapShanmenThrownWeaponProductResult Product;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsRecoveryApplied() const;
	bool RequiresRecovery() const { return Product.Command.RequiresRecovery(); }
};

/**
 * Active-Run owner of hotbar resolution, action stat capture, and P7.3-P7.5
 * state. The frozen RunCorrelation is the only hotbar source; this class never
 * reads or writes the legacy item subsystem, input bindings, UI, or inventory.
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
	int32 NumCapturedSelections() const { return CapturedSelections.Num(); }
	Edemo_mapShanmenThrownWeaponHostState GetHostState() const
	{
		return Host.GetState();
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
