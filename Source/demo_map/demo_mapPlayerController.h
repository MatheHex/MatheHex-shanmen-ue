#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapShanmenControlledWeaponInputAdapter.h"
#include "demo_mapShanmenSpiritEvasionInputAdapter.h"
#include "demo_mapShanmenThrownWeaponHotbarConfirmationAdapter.h"
#include "demo_mapShanmenThrownWeaponArcLaunchInputAdapter.h"
#include "demo_mapShanmenThrownWeaponInputChoiceControllerAdapter.h"
#include "demo_mapShanmenThrownWeaponInputChoiceIntentAdapter.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator.h"
#include "demo_mapShanmenWeaponGuardInputAdapter.h"
#include "demo_mapPlayerController.generated.h"

class Udemo_mapSkillComponent;
class UCharacterMovementComponent;
class UUserWidget;
class ACharacter;
#if !UE_BUILD_SHIPPING
enum class Edemo_mapInputRestoreTraceEvent : uint8;
#endif

/**
 * Player controller for the base top-down map.
 * Uses camera-relative WASD movement, mouse-facing, and a direct left-click melee attack.
 */
UCLASS()
class Ademo_mapPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	bool bMoveForwardPressed = false;
	bool bMoveBackwardPressed = false;
	bool bMoveRightPressed = false;
	bool bMoveLeftPressed = false;
	bool bSettlementInputLockHeld = false;
	bool bProfilePreparationInputLockHeld = false;
	bool bSearchContainerInputLockHeld = false;
	bool bInventoryInputLockHeld = false;
	bool bInputRestoreDiagnostics = false;
	bool bOwnedInputIgnoreApplied = false;
	mutable bool bLoggedFirstMoveInput = false;
	mutable bool bLoggedFirstAttackInput = false;
	mutable bool bLoggedFirstInteractInput = false;
	FString InputSurfaceState = TEXT("Gameplay");
	FString InputModeState = TEXT("Uninitialized");
	TWeakObjectPtr<UUserWidget> SettlementFocusWidget;
	TWeakObjectPtr<UUserWidget> ProfilePreparationFocusWidget;
	TWeakObjectPtr<UUserWidget> SearchContainerFocusWidget;
	TWeakObjectPtr<UUserWidget> InventoryFocusWidget;
	int32 ProfilePreparationInputApplyCount = 0;
	int32 GameplayInputRestoreCount = 0;
	bool bHasValidAimPoint = false;
	bool bCurrentAimPointValid = false;
	bool bHasValidAimDirection = false;
	FVector LastValidAimPoint = FVector::ZeroVector;
	FVector LastValidAimDirection = FVector::ForwardVector;
	TWeakObjectPtr<UCharacterMovementComponent> OrderedCharacterMovement;
	TWeakObjectPtr<ACharacter> InputConsumptionObservedCharacter;
	TWeakObjectPtr<UInputComponent> ProductInputBindingComponent;
	int32 ProductInputBindingStartIndex = INDEX_NONE;
	int32 ProductInputBindingCount = 0;
	uint64 LastMovementApplicationFrame = MAX_uint64;
	Fdemo_mapShanmenThrownWeaponArcConfirmationOwner
		ThrownWeaponArcConfirmationOwner;
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter
		ThrownWeaponHotbarConfirmationAdapter;
#if !UE_BUILD_SHIPPING
	bool bRecordedMovementAppliedForCurrentPress = false;
	bool bInputConsumptionReflectedFunctionPresent = false;
	bool bInputConsumptionHandlerAlreadyBound = false;
	uint64 InputConsumptionActualCallbackCount = 0;
	float InputConsumptionLastActualCallbackDeltaSeconds = -1.0f;
#endif

	static constexpr float MinimumAimDistance = 25.0f;
	static constexpr float BasicAttackCooldown = 0.45f;
	static constexpr float BasicAttackCoefficient = 1.0f;
	static constexpr float BasicAttackRange = 200.0f;
	static constexpr float BasicAttackRadius = 85.0f;
	static constexpr float BasicAttackStartOffset = 60.0f;
	static constexpr float BasicAttackVerticalOffset = 50.0f;
	static constexpr double ThrownWeaponArcApexAdjustmentStep = 0.25;
	float BasicAttackReadyTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Combat")
	Fdemo_mapCommonSkillParams BasicAttackParams;

public:

	/** Constructor */
	Ademo_mapPlayerController();

	/** Executes the current pawn's forward melee sweep. Returns true when at least one target takes damage. */
	bool TryBasicAttack();
	float GetBasicAttackCooldownRemaining() const;
	static int32 ResolveHotbarSlotForKey(const FKey& Key);
	Fdemo_mapInputBindingResult ApplyInputBindingOverride(FName ActionId, const FKey& Key);
	Fdemo_mapInputBindingResult ApplyInputBindingOverrideWithSwap(FName ActionId, const FKey& Key);
	Fdemo_mapInputBindingResult RestoreDefaultInputBindings();
	void RebuildProductInputBindings();
	/** Adapts one future dedicated input without owning its physical key. */
	Fdemo_mapShanmenSpiritEvasionInputResult RouteSpiritEvasionStartInput();
	/** Routes one canonical flying-sword Launch-or-Recall physical command. */
	Fdemo_mapShanmenControlledWeaponInputResult
	RouteControlledWeaponLaunchRecallInput();
	/** Routes one frozen, device-independent Arc launch command. */
	Fdemo_mapShanmenThrownWeaponArcLaunchInputResult
	RouteThrownWeaponArcLaunchCommand(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command);
	/** Consumes one device-independent logical Arc confirmation event. */
	Fdemo_mapShanmenThrownWeaponArcConfirmationResult
	RouteThrownWeaponArcConfirmation(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent);
	/** Routes the existing hotbar press by the sole current trajectory choice. */
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult
	RouteThrownWeaponHotbarConfirmationInput(int32 HotbarSlotNumber);
	/** Submits one frozen choice edit through the sole GameMode session. */
	Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult
	RouteThrownWeaponInputChoiceCommand(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command);
	/** Captures one logical choice edit against the sole current revision. */
	Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult
	RouteThrownWeaponInputChoiceIntent(
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent);
	/** Reads the current revisionless choice interaction projection. */
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult
	ReadThrownWeaponInputChoiceInteraction();
	/** Revalidates one visible choice request before routing its intent. */
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
	RouteThrownWeaponInputChoiceInteractionRequest(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Request);
	/** Reads, composes, and stale-safely routes one device-independent Arc target. */
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
	RouteThrownWeaponArcTargetInteraction(const FVector2D& RawTargetIntent);
	/** Reads, composes, and stale-safely routes one device-independent Arc apex delta. */
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
	RouteThrownWeaponArcApexAdjustmentInteraction(double RawNormalizedDelta);
	/** Reads, composes, and stale-safely routes one device-independent Arc target clear. */
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
	RouteThrownWeaponArcTargetClearInteraction();
	/** Routes the physical hold start through the Run-owned fixed timeline. */
	Fdemo_mapShanmenWeaponGuardInputResult RouteWeaponGuardStartInput();
	/** Release deliberately bypasses gameplay UI locks to prevent stuck guard. */
	Fdemo_mapShanmenWeaponGuardReleaseInputResult RouteWeaponGuardReleaseInput();
	bool IsGameplayInputAllowed() const;
	Udemo_mapSkillComponent* GetSkillComponent() const;
	FVector GetLastValidAimDirection() const { return bHasValidAimDirection ? LastValidAimDirection : FVector::ZeroVector; }
	void ReloadCurrentLevel();
	/** Owns the one settlement input lock and visible report focus. */
	void BeginSettlementInputLock(UUserWidget* FocusWidget);
	/** Idempotently releases the transient report lock and restores the next valid UI surface. */
	bool EndSettlementInputLock();
	/** Idempotently owns the visible Preparation UI focus, mouse, and gameplay lock. */
	void BeginProfilePreparationInputLock(UUserWidget* FocusWidget);
	/** Owns the non-pausing Runtime Container UI input surface. */
	void BeginSearchContainerInputLock(UUserWidget* FocusWidget);
	/** Owns the non-pausing Runtime inventory input surface. */
	void BeginInventoryInputLock(UUserWidget* FocusWidget);
	/** Idempotently restores the exact normal Gameplay surface after Container close. */
	bool RestoreGameplayControlFromSearchContainer();
	/** Idempotently releases Runtime inventory focus and restores Gameplay now. */
	bool RestoreGameplayControlFromInventory();
	/** Idempotently restores normal top-down controls after a new V3 run becomes active. */
	bool RestoreGameplayControlForNewRun();
	bool IsProfilePreparationInputLocked() const { return bProfilePreparationInputLockHeld; }
	bool IsSearchContainerInputLocked() const { return bSearchContainerInputLockHeld; }
	bool IsInventoryInputLocked() const { return bInventoryInputLockHeld; }
	const FString& GetInputSurfaceState() const { return InputSurfaceState; }
	const FString& GetInputModeState() const { return InputModeState; }
	int32 GetProfilePreparationInputApplyCount() const { return ProfilePreparationInputApplyCount; }
	int32 GetGameplayInputRestoreCount() const { return GameplayInputRestoreCount; }
	void LogInputRestoreDiagnostics(const TCHAR* Phase) const;
	void ReconcileInputContext(const TCHAR* Reason);

#if !UE_BUILD_SHIPPING
	/** Sends a synthetic key through PlayerInput and the normal InputComponent binding stack. */
	bool DispatchAutomationKey(const FKey& Key);
	bool DispatchAutomationKeyPressed(const FKey& Key);
	bool DispatchAutomationKeyReleased(const FKey& Key);
	uint64 GetSpiritEvasionInputInvocationCountForAutomation() const
	{
		return SpiritEvasionInputInvocationCount;
	}
	const Fdemo_mapShanmenSpiritEvasionInputResult&
	GetLastSpiritEvasionInputResultForAutomation() const
	{
		return LastSpiritEvasionInputResult;
	}
	uint64 GetControlledWeaponInputInvocationCountForAutomation() const
	{
		return ControlledWeaponInputInvocationCount;
	}
	const Fdemo_mapShanmenControlledWeaponInputResult&
	GetLastControlledWeaponInputResultForAutomation() const
	{
		return LastControlledWeaponInputResult;
	}
	uint64 GetThrownWeaponTrajectoryToggleInvocationCountForAutomation() const
	{
		return ThrownWeaponTrajectoryToggleInvocationCount;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult&
	GetLastThrownWeaponTrajectoryToggleReadForAutomation() const
	{
		return LastThrownWeaponTrajectoryToggleRead;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest&
	GetLastThrownWeaponTrajectoryToggleRequestForAutomation() const
	{
		return LastThrownWeaponTrajectoryToggleRequest;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult&
	GetLastThrownWeaponTrajectoryToggleResultForAutomation() const
	{
		return LastThrownWeaponTrajectoryToggleResult;
	}
	uint64 GetThrownWeaponArcEditingInputInvocationCountForAutomation() const
	{
		return ThrownWeaponArcEditingInputInvocationCount;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult&
	GetLastThrownWeaponArcEditingInputResultForAutomation() const
	{
		return LastThrownWeaponArcEditingInputResult;
	}
	uint64 GetWeaponGuardPressInvocationCountForAutomation() const
	{
		return WeaponGuardPressInvocationCount;
	}
	uint64 GetWeaponGuardReleaseInvocationCountForAutomation() const
	{
		return WeaponGuardReleaseInvocationCount;
	}
	const Fdemo_mapShanmenWeaponGuardInputResult&
	GetLastWeaponGuardInputResultForAutomation() const
	{
		return LastWeaponGuardInputResult;
	}
	const Fdemo_mapShanmenWeaponGuardReleaseInputResult&
	GetLastWeaponGuardReleaseInputResultForAutomation() const
	{
		return LastWeaponGuardReleaseInputResult;
	}
	int32 GetLastHotbarSlotForwardedForAutomation() const { return LastHotbarSlotForwardedForAutomation; }
	void SetAutomationAimDirection(const FVector& Direction) { LastValidAimDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal(); bHasValidAimDirection = !LastValidAimDirection.IsNearlyZero(); }
	bool IsOwnedInputIgnoreAppliedForAutomation() const { return bOwnedInputIgnoreApplied; }
	bool IsSettlementInputLockedForAutomation() const { return bSettlementInputLockHeld; }
	bool IsMoveForwardPressedForAutomation() const { return bMoveForwardPressed; }
	bool HasPlayerInputForAutomation() const { return PlayerInput != nullptr; }
	bool HasInputComponentForAutomation() const { return InputComponent != nullptr; }
	bool HasMovementOrderingForAutomation(
		const UCharacterMovementComponent* Movement) const;
	int32 GetMovementOrderingPrerequisiteCountForAutomation(
		const UCharacterMovementComponent* Movement) const;
	void RunPlayerTickForAutomation(float DeltaTime) { PlayerTick(DeltaTime); }
	void RefreshInputConsumptionMovementHookForGate(
		ACharacter* InCharacter);
	bool InstallInputConsumptionMovementHookForAutomation(
		ACharacter* InCharacter);
	bool DispatchInputConsumptionMovementUpdatedForAutomation(
		ACharacter* InCharacter,
		float DeltaSeconds);
	void RemoveInputConsumptionMovementHookForAutomation()
	{
		RemoveInputConsumptionMovementHook();
	}
	FName GetInputConsumptionMovementHandlerNameForAutomation() const;
	bool IsInputConsumptionReflectedFunctionPresentForAutomation() const
	{
		return bInputConsumptionReflectedFunctionPresent;
	}
	bool IsInputConsumptionHandlerAlreadyBoundForAutomation();
	uint64 GetInputConsumptionActualCallbackCountForAutomation() const
	{
		return InputConsumptionActualCallbackCount;
	}
	float GetInputConsumptionLastActualCallbackDeltaSecondsForAutomation() const
	{
		return InputConsumptionLastActualCallbackDeltaSeconds;
	}
#endif

protected:

	/** Initialize WASD and level-reload bindings. */
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InstallMovementOrdering(APawn* InPawn);
	void RemoveMovementOrdering();
	bool InstallInputConsumptionMovementHook(
		ACharacter* InCharacter,
		bool bForceForAutomation);
	void RemoveInputConsumptionMovementHook();
	UFUNCTION()
	void HandleInputConsumptionMovementUpdated(
		float DeltaSeconds,
		FVector OldLocation,
		FVector OldVelocity);
	void StartMoveForward();
	void StopMoveForward();
	void StartMoveBackward();
	void StopMoveBackward();
	void StartMoveRight();
	void StopMoveRight();
	void StartMoveLeft();
	void StopMoveLeft();
	void ApplyCameraRelativeMovement();
	void UpdateMouseFacing();
	void StartBasicAttack();
	void ToggleGroundCircle();
	void CancelGroundCircle();
	void CastSelfSector();
	void FireStraightProjectile();
	void ToggleControlledWeaponLaunchRecall();
	void ToggleThrownWeaponTrajectory();
	void SetThrownWeaponArcTargetFromPointerAim();
	void IncreaseThrownWeaponArcApex();
	void DecreaseThrownWeaponArcApex();
	void ClearThrownWeaponArcTarget();
	void StartSpiritEvasion();
	void StartWeaponGuard();
	void StopWeaponGuard();
	void UseHotbarSlot(int32 SlotNumber);
	void UseHotbarSlot1();
	void UseHotbarSlot2();
	void UseHotbarSlot3();
	void UseHotbarSlot4();
	void UseHotbarSlot5();
	void UseHotbarSlot6();
	void UseHotbarSlot7();
	void UseHotbarSlot8();
	void UseHotbarSlot9();
	void BeginInteractV3();
	void EndInteractV3();
	void ToggleV3Inventory();
	void HandleBackAction();
	void HandleSearchEscapeAction();
	void BindProductInputActions();
	bool RemoveProductInputActions();

#if !UE_BUILD_SHIPPING
	void RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent Event) const;
	int32 LastHotbarSlotForwardedForAutomation = INDEX_NONE;
	uint64 SpiritEvasionInputInvocationCount = 0;
	Fdemo_mapShanmenSpiritEvasionInputResult
		LastSpiritEvasionInputResult;
	uint64 ControlledWeaponInputInvocationCount = 0;
	Fdemo_mapShanmenControlledWeaponInputResult
		LastControlledWeaponInputResult;
	uint64 ThrownWeaponTrajectoryToggleInvocationCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult
		LastThrownWeaponTrajectoryToggleRead;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest
		LastThrownWeaponTrajectoryToggleRequest;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
		LastThrownWeaponTrajectoryToggleResult;
	uint64 ThrownWeaponArcEditingInputInvocationCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
		LastThrownWeaponArcEditingInputResult;
	uint64 WeaponGuardPressInvocationCount = 0;
	uint64 WeaponGuardReleaseInvocationCount = 0;
	Fdemo_mapShanmenWeaponGuardInputResult LastWeaponGuardInputResult;
	Fdemo_mapShanmenWeaponGuardReleaseInputResult
		LastWeaponGuardReleaseInputResult;
#endif
};


