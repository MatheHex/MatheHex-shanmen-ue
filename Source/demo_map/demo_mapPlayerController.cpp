#include "demo_mapPlayerController.h"
#include "demo_map.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapTrainingTarget.h"
#include "demo_mapCombatTargeting.h"
#include "demo_mapGameMode.h"
#include "demo_mapSkillComponent.h"
#include "demo_mapPlayerCombat.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapSearchContainerActor.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapInputContextTypes.h"
#include "demo_mapInputConsumptionTrace.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapShanmenThrownWeaponArcEditingInteractionComposition.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/DateTime.h"

namespace
{
	constexpr float PlayerCharacterMovementTickInterval = 0.001f;
	using FArcEditingInteractionReadResult =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult;
	using FArcEditingInteractionRequest =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest;
	using FArcEditingInteractionRequestResult =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult;
	using FComposeArcEditingInteractionRequest = TFunctionRef<bool(
		const FArcEditingInteractionReadResult&,
		FArcEditingInteractionRequest&)>;

	int32 CountControllerPrerequisites(
		const UCharacterMovementComponent* Movement,
		const Ademo_mapPlayerController* Controller)
	{
		if (!Movement || !Controller)
		{
			return 0;
		}
		const FTickPrerequisite Expected(
			const_cast<Ademo_mapPlayerController*>(Controller),
			const_cast<Ademo_mapPlayerController*>(Controller)
				->PrimaryActorTick);
		int32 Count = 0;
		for (const FTickPrerequisite& Candidate :
			Movement->PrimaryComponentTick.GetPrerequisites())
		{
			if (Candidate == Expected)
			{
				++Count;
			}
		}
		return Count;
	}

	FArcEditingInteractionRequestResult RouteArcEditingInteraction(
		Ademo_mapPlayerController& Controller,
		FComposeArcEditingInteractionRequest ComposeRequest)
	{
		const FArcEditingInteractionReadResult Read =
			Controller.ReadThrownWeaponInputChoiceInteraction();
		FArcEditingInteractionRequest Request;
		if (!ComposeRequest(Read, Request))
		{
			Request = FArcEditingInteractionRequest();
		}
		return Controller.RouteThrownWeaponInputChoiceInteractionRequest(
			Request);
	}
}

Ademo_mapPlayerController::Ademo_mapPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	BasicAttackParams.Cooldown = BasicAttackCooldown;
	BasicAttackParams.VerticalTolerance = BasicAttackVerticalOffset * 2.0f;
}

#if !UE_BUILD_SHIPPING
void Ademo_mapPlayerController::RecordInputRestoreTraceEvent(
	Edemo_mapInputRestoreTraceEvent Event) const
{
	if (!Isdemo_mapInputRestoreTraceRuntimeActive())
	{
		return;
	}
	if (const Ademo_mapGameMode* Mode =
			GetWorld()
				? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
				: nullptr)
	{
		if (Ademo_mapV3ProgressionManager* V3 =
			Mode->GetV3ProgressionManager())
		{
			V3->RecordInputRestoreTraceEvent(
				Event,
				-1.0f,
				-1.0,
				Edemo_mapInputRestoreTraceCaller::Controller);
		}
	}
}

bool Ademo_mapPlayerController::DispatchAutomationKey(const FKey& Key)
{
	if (InputComponent == nullptr || !Key.IsValid())
	{
		return false;
	}
	InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0f));
	TickPlayerInput(1.0f / 60.0f, false);
	InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.0f));
	TickPlayerInput(1.0f / 60.0f, false);
	return true;
}

bool Ademo_mapPlayerController::DispatchAutomationKeyPressed(const FKey& Key)
{
	if (InputComponent == nullptr || !Key.IsValid())
	{
		return false;
	}
	InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0f));
	TickPlayerInput(1.0f / 60.0f, false);
	return true;
}

bool Ademo_mapPlayerController::DispatchAutomationKeyReleased(const FKey& Key)
{
	if (InputComponent == nullptr || !Key.IsValid())
	{
		return false;
	}
	InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.0f));
	TickPlayerInput(1.0f / 60.0f, false);
	return true;
}

bool Ademo_mapPlayerController::HasMovementOrderingForAutomation(
	const UCharacterMovementComponent* Movement) const
{
	return OrderedCharacterMovement.Get() == Movement
		&& CountControllerPrerequisites(Movement, this) == 1;
}

int32 Ademo_mapPlayerController::
	GetMovementOrderingPrerequisiteCountForAutomation(
		const UCharacterMovementComponent* Movement) const
{
	return CountControllerPrerequisites(Movement, this);
}
#endif

void Ademo_mapPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent == nullptr)
	{
		return;
	}

	Fdemo_mapInputBindingSettings::Get().Load();
	BindProductInputActions();
	UE_LOG(Logdemo_map, Log, TEXT("P7: unified registry bindings active, including Runtime Inventory and Back."));
}

void Ademo_mapPlayerController::BindProductInputActions()
{
	if (!InputComponent) return;
	ProductInputBindingComponent = InputComponent;
	ProductInputBindingStartIndex = InputComponent->KeyBindings.Num();
	const Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveForward), IE_Pressed, this, &Ademo_mapPlayerController::StartMoveForward);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveForward), IE_Released, this, &Ademo_mapPlayerController::StopMoveForward);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveBackward), IE_Pressed, this, &Ademo_mapPlayerController::StartMoveBackward);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveBackward), IE_Released, this, &Ademo_mapPlayerController::StopMoveBackward);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveRight), IE_Pressed, this, &Ademo_mapPlayerController::StartMoveRight);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveRight), IE_Released, this, &Ademo_mapPlayerController::StopMoveRight);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveLeft), IE_Pressed, this, &Ademo_mapPlayerController::StartMoveLeft);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::MoveLeft), IE_Released, this, &Ademo_mapPlayerController::StopMoveLeft);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::ResetRun), IE_Pressed, this, &Ademo_mapPlayerController::ReloadCurrentLevel);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::PrimaryAttack), IE_Pressed, this, &Ademo_mapPlayerController::StartBasicAttack);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::SkillGroundCircle), IE_Pressed, this, &Ademo_mapPlayerController::ToggleGroundCircle);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::SkillSelfSector), IE_Pressed, this, &Ademo_mapPlayerController::CastSelfSector);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::SkillStraightProjectile), IE_Pressed, this, &Ademo_mapPlayerController::FireStraightProjectile);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle), IE_Pressed, this, &Ademo_mapPlayerController::ToggleThrownWeaponTrajectory);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet), IE_Pressed, this, &Ademo_mapPlayerController::SetThrownWeaponArcTargetFromPointerAim);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease), IE_Pressed, this, &Ademo_mapPlayerController::IncreaseThrownWeaponArcApex);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease), IE_Pressed, this, &Ademo_mapPlayerController::DecreaseThrownWeaponArcApex);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear), IE_Pressed, this, &Ademo_mapPlayerController::ClearThrownWeaponArcTarget);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::SpiritEvasion), IE_Pressed, this, &Ademo_mapPlayerController::StartSpiritEvasion);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::WeaponGuard), IE_Pressed, this, &Ademo_mapPlayerController::StartWeaponGuard);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::WeaponGuard), IE_Released, this, &Ademo_mapPlayerController::StopWeaponGuard);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Interact), IE_Pressed, this, &Ademo_mapPlayerController::BeginInteractV3);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Interact), IE_Released, this, &Ademo_mapPlayerController::EndInteractV3);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar1), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot1);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar2), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot2);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar3), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot3);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar4), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot4);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar5), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot5);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar6), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot6);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar7), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot7);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar8), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot8);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Hotbar9), IE_Pressed, this, &Ademo_mapPlayerController::UseHotbarSlot9);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Inventory), IE_Pressed, this, &Ademo_mapPlayerController::ToggleV3Inventory);
	InputComponent->BindKey(Settings.GetKey(Fdemo_mapInputActionIds::Back), IE_Pressed, this, &Ademo_mapPlayerController::HandleBackAction);
	// Escape is intentionally reserved from rebinding and unwinds the active
	// item surface from the top: search first, then the runtime inventory.
	// Backspace keeps its broader legacy "Back" role.
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &Ademo_mapPlayerController::HandleSearchEscapeAction);
	ProductInputBindingCount =
		InputComponent->KeyBindings.Num() - ProductInputBindingStartIndex;
}

bool Ademo_mapPlayerController::RemoveProductInputActions()
{
	UInputComponent* BoundComponent = ProductInputBindingComponent.Get();
	if (!BoundComponent || ProductInputBindingCount == 0)
	{
		ProductInputBindingComponent.Reset();
		ProductInputBindingStartIndex = INDEX_NONE;
		ProductInputBindingCount = 0;
		return true;
	}
	if (ProductInputBindingStartIndex < 0
		|| ProductInputBindingStartIndex + ProductInputBindingCount
			> BoundComponent->KeyBindings.Num())
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("Input remap rejected: owned BindKey range is no longer valid."));
		return false;
	}
	for (int32 Index = ProductInputBindingStartIndex;
		Index < ProductInputBindingStartIndex + ProductInputBindingCount;
		++Index)
	{
		if (!BoundComponent->KeyBindings[Index].KeyDelegate.IsBoundToObject(this))
		{
			UE_LOG(
				Logdemo_map,
				Error,
				TEXT("Input remap rejected: owned BindKey range changed ownership."));
			return false;
		}
	}
	// UE 5.8 ClearBindingsForObject only invalidates its cached action map;
	// it does not remove raw BindKey entries. This controller records and
	// verifies the exact contiguous range it owns so a live remap cannot leave
	// the old physical key active or remove unrelated component bindings.
	BoundComponent->KeyBindings.RemoveAt(
		ProductInputBindingStartIndex,
		ProductInputBindingCount,
		EAllowShrinking::No);
	ProductInputBindingComponent.Reset();
	ProductInputBindingStartIndex = INDEX_NONE;
	ProductInputBindingCount = 0;
	return true;
}

void Ademo_mapPlayerController::RebuildProductInputBindings()
{
	if (!InputComponent) return;
	if (!RemoveProductInputActions()) return;
	InputComponent->ClearBindingsForObject(this);
	BindProductInputActions();
	if (PlayerInput) PlayerInput->FlushPressedKeys();
}

Fdemo_mapInputBindingResult Ademo_mapPlayerController::ApplyInputBindingOverride(FName ActionId, const FKey& Key)
{
	Fdemo_mapInputBindingResult Result;
	UGameInstance* Instance = GetGameInstance();
	const Udemo_mapProfileSessionSubsystem* Session = Instance ? Instance->GetSubsystem<Udemo_mapProfileSessionSubsystem>() : nullptr;
	if (!Session || Session->GetSnapshot().SessionState != Edemo_mapProfileSessionState::ReadyForPreparation)
	{
		Result.Status = Edemo_mapInputBindingStatus::LoadRejected;
		Result.Diagnostic = TEXT("Input overrides are allowed only in ReadyForPreparation controls UI.");
		return Result;
	}
	Result = Fdemo_mapInputBindingSettings::Get().ApplyOverride(ActionId, Key);
	if (Result.IsSuccess()) RebuildProductInputBindings();
	return Result;
}

Fdemo_mapInputBindingResult
Ademo_mapPlayerController::ApplyInputBindingOverrideWithSwap(
	FName ActionId,
	const FKey& Key)
{
	Fdemo_mapInputBindingResult Result;
	UGameInstance* Instance = GetGameInstance();
	const Udemo_mapProfileSessionSubsystem* Session = Instance
		? Instance->GetSubsystem<Udemo_mapProfileSessionSubsystem>()
		: nullptr;
	if (!Session || Session->GetSnapshot().SessionState
		!= Edemo_mapProfileSessionState::ReadyForPreparation)
	{
		Result.Status = Edemo_mapInputBindingStatus::LoadRejected;
		Result.Diagnostic =
			TEXT("Input overrides are allowed only in ReadyForPreparation controls UI.");
		return Result;
	}
	Result = Fdemo_mapInputBindingSettings::Get()
		.ApplyOverrideWithSwap(ActionId, Key);
	if (Result.IsSuccess()) RebuildProductInputBindings();
	return Result;
}

Fdemo_mapInputBindingResult Ademo_mapPlayerController::RestoreDefaultInputBindings()
{
	Fdemo_mapInputBindingResult Result;
	UGameInstance* Instance = GetGameInstance();
	const Udemo_mapProfileSessionSubsystem* Session = Instance ? Instance->GetSubsystem<Udemo_mapProfileSessionSubsystem>() : nullptr;
	if (!Session || Session->GetSnapshot().SessionState != Edemo_mapProfileSessionState::ReadyForPreparation)
	{
		Result.Status = Edemo_mapInputBindingStatus::LoadRejected;
		Result.Diagnostic = TEXT("Default restore is allowed only in ReadyForPreparation controls UI.");
		return Result;
	}
	Result = Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	if (Result.IsSuccess()) RebuildProductInputBindings();
	return Result;
}

void Ademo_mapPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
#if !UE_BUILD_SHIPPING
	Recorddemo_mapInputConsumptionControllerTick(
		GetPawn(),
		this,
		DeltaTime);
#endif
	if (!IsGameplayInputAllowed())
	{
		bMoveForwardPressed = false;
		bMoveBackwardPressed = false;
		bMoveRightPressed = false;
		bMoveLeftPressed = false;
		if (Udemo_mapSkillComponent* Skills = GetSkillComponent())
		{
			Skills->CancelAllSkillState();
		}
		return;
	}
	ApplyCameraRelativeMovement();
	UpdateMouseFacing();
	if (Udemo_mapSkillComponent* Skills = GetSkillComponent(); Skills != nullptr && Skills->IsGroundCircleTargeting() && !Skills->IsPreviewExternallyControlled())
	{
		Skills->UpdateGroundTargetPreview(bCurrentAimPointValid, LastValidAimPoint);
	}
}

void Ademo_mapPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	bInputRestoreDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("V3InputRestoreDiagnostics"));
	InstallMovementOrdering(InPawn);

	ACharacter* ControlledCharacter = Cast<ACharacter>(InPawn);
	if (ControlledCharacter == nullptr)
	{
		return;
	}
#if !UE_BUILD_SHIPPING
	InstallInputConsumptionMovementHook(
		ControlledCharacter,
		false);
#endif

	ControlledCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	ControlledCharacter->bUseControllerRotationYaw = false;

	if (USpringArmComponent* SpringArm = ControlledCharacter->FindComponentByClass<USpringArmComponent>())
	{
		SpringArm->bUsePawnControlRotation = false;
		SpringArm->bInheritYaw = false;
	}

	ReconcileInputContext(TEXT("PawnPossessed"));
	UE_LOG(Logdemo_map, Log, TEXT("T3: mouse-facing controller active; WASD movement retained; camera remains independent."));
}

void Ademo_mapPlayerController::OnUnPossess()
{
	if (Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr)
	{
		Mode->RouteWeaponGuardTerminationIntent(
			Edemo_mapShanmenWeaponGuardTerminationReason::PawnUnpossessed);
	}
	RemoveInputConsumptionMovementHook();
	RemoveMovementOrdering();
	Super::OnUnPossess();
}

void Ademo_mapPlayerController::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr)
	{
		Mode->RouteWeaponGuardTerminationIntent(
			Edemo_mapShanmenWeaponGuardTerminationReason::ControllerEndPlay);
	}
	RemoveInputConsumptionMovementHook();
	RemoveMovementOrdering();
	bSettlementInputLockHeld = false;
	bProfilePreparationInputLockHeld = false;
	bSearchContainerInputLockHeld = false;
	bInventoryInputLockHeld = false;
	ProfilePreparationFocusWidget.Reset();
	SearchContainerFocusWidget.Reset();
	InventoryFocusWidget.Reset();
	if (bOwnedInputIgnoreApplied)
	{
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		bOwnedInputIgnoreApplied = false;
	}
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapPlayerController::RemoveInputConsumptionMovementHook()
{
	if (ACharacter* ObservedCharacter =
		InputConsumptionObservedCharacter.Get())
	{
		ObservedCharacter->OnCharacterMovementUpdated.RemoveDynamic(
			this,
			&Ademo_mapPlayerController::HandleInputConsumptionMovementUpdated);
	}
	InputConsumptionObservedCharacter.Reset();
#if !UE_BUILD_SHIPPING
	bInputConsumptionHandlerAlreadyBound = false;
#endif
}

void Ademo_mapPlayerController::HandleInputConsumptionMovementUpdated(
	float DeltaSeconds,
	FVector OldLocation,
	FVector OldVelocity)
{
#if !UE_BUILD_SHIPPING
	++InputConsumptionActualCallbackCount;
	InputConsumptionLastActualCallbackDeltaSeconds = DeltaSeconds;
	Recorddemo_mapInputConsumptionMovementUpdated(
		GetPawn(),
		this,
		DeltaSeconds,
		OldLocation,
		OldVelocity);
#else
	(void)DeltaSeconds;
	(void)OldLocation;
	(void)OldVelocity;
#endif
}

bool Ademo_mapPlayerController::InstallInputConsumptionMovementHook(
	ACharacter* InCharacter,
	bool bForceForAutomation)
{
#if !UE_BUILD_SHIPPING
	const bool bGateRequested =
		bForceForAutomation
		|| (FParse::Param(
				FCommandLine::Get(),
				TEXT("InputRestoreAutomation"))
			&& FParse::Param(
				FCommandLine::Get(),
				TEXT("InputConsumptionMicroTrace"))
			&& FParse::Param(
				FCommandLine::Get(),
				TEXT("InputMovementGateTrace")));
	if (!bGateRequested || !InCharacter)
	{
		RemoveInputConsumptionMovementHook();
		return false;
	}

	if (InputConsumptionObservedCharacter.Get() != InCharacter)
	{
		RemoveInputConsumptionMovementHook();
	}

	const FName HandlerName =
		GET_FUNCTION_NAME_CHECKED(
			Ademo_mapPlayerController,
			HandleInputConsumptionMovementUpdated);
	bInputConsumptionReflectedFunctionPresent =
		FindFunction(HandlerName) != nullptr;
	if (!bInputConsumptionReflectedFunctionPresent)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("Unable to bind delegate: reflected MovementUpdated handler is absent."));
		return false;
	}

	InCharacter->OnCharacterMovementUpdated.AddUniqueDynamic(
		this,
		&Ademo_mapPlayerController::HandleInputConsumptionMovementUpdated);
	InputConsumptionObservedCharacter = InCharacter;
	bInputConsumptionHandlerAlreadyBound =
		InCharacter->OnCharacterMovementUpdated.IsAlreadyBound(
			this,
			&Ademo_mapPlayerController::HandleInputConsumptionMovementUpdated);
	if (!bInputConsumptionHandlerAlreadyBound)
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("Unable to bind delegate: specific MovementUpdated handler pair is absent."));
		InputConsumptionObservedCharacter.Reset();
	}
	return bInputConsumptionHandlerAlreadyBound;
#else
	(void)Character;
	(void)bForceForAutomation;
	return false;
#endif
}

#if !UE_BUILD_SHIPPING
void Ademo_mapPlayerController::RefreshInputConsumptionMovementHookForGate(
	ACharacter* InCharacter)
{
	InstallInputConsumptionMovementHook(InCharacter, false);
}

bool Ademo_mapPlayerController::
	InstallInputConsumptionMovementHookForAutomation(
		ACharacter* InCharacter)
{
	return InstallInputConsumptionMovementHook(InCharacter, true);
}

bool Ademo_mapPlayerController::
	DispatchInputConsumptionMovementUpdatedForAutomation(
		ACharacter* InCharacter,
		float DeltaSeconds)
{
	if (!InCharacter
		|| InputConsumptionObservedCharacter.Get() != InCharacter
		|| !IsInputConsumptionHandlerAlreadyBoundForAutomation())
	{
		return false;
	}
	const UCharacterMovementComponent* Movement =
		InCharacter->GetCharacterMovement();
	const FVector OldLocation =
		Movement && Movement->UpdatedComponent
			? Movement->UpdatedComponent->GetComponentLocation()
			: InCharacter->GetActorLocation();
	const FVector OldVelocity =
		Movement ? Movement->Velocity : InCharacter->GetVelocity();
	const uint64 Before = InputConsumptionActualCallbackCount;
	HandleInputConsumptionMovementUpdated(
		DeltaSeconds,
		OldLocation,
		OldVelocity);
	return InputConsumptionActualCallbackCount > Before;
}

FName Ademo_mapPlayerController::
	GetInputConsumptionMovementHandlerNameForAutomation() const
{
	return GET_FUNCTION_NAME_CHECKED(
		Ademo_mapPlayerController,
		HandleInputConsumptionMovementUpdated);
}

bool Ademo_mapPlayerController::
	IsInputConsumptionHandlerAlreadyBoundForAutomation()
{
	ACharacter* BoundCharacter =
		InputConsumptionObservedCharacter.Get();
	return BoundCharacter
		&& BoundCharacter->OnCharacterMovementUpdated.IsAlreadyBound(
			this,
			&Ademo_mapPlayerController::HandleInputConsumptionMovementUpdated);
}
#endif

void Ademo_mapPlayerController::InstallMovementOrdering(APawn* InPawn)
{
	UCharacterMovementComponent* Movement =
		Cast<ACharacter>(InPawn)
			? CastChecked<ACharacter>(InPawn)->GetCharacterMovement()
			: nullptr;
	if (OrderedCharacterMovement.Get() != Movement)
	{
		RemoveMovementOrdering();
	}
	if (!Movement
		|| !PrimaryActorTick.bCanEverTick
		|| !Movement->PrimaryComponentTick.bCanEverTick)
	{
		return;
	}

	// UE 5.8 UPrimitiveComponent::MoveComponentImpl discards swept movement
	// shorter than 4 * UE_KINDA_SMALL_NUMBER. At an uncapped frame rate the
	// first walking displacement can stay below that floor and PhysWalking
	// then derives zero velocity from the unchanged transform. A 1 ms product
	// cadence keeps ordinary frame rates unchanged while making the first
	// MaxAcceleration integration large enough to clear the engine floor.
	Movement->SetComponentTickInterval(
		PlayerCharacterMovementTickInterval);

	const FTickPrerequisite ReverseDependency(
		Movement,
		Movement->PrimaryComponentTick);
	if (PrimaryActorTick.GetPrerequisites().Contains(ReverseDependency))
	{
		UE_LOG(
			Logdemo_map,
			Error,
			TEXT("P8.7: movement ordering rejected a Controller/CharacterMovement tick cycle for %s."),
			*GetNameSafe(InPawn));
		return;
	}

	Movement->PrimaryComponentTick.AddPrerequisite(
		this,
		PrimaryActorTick);
	if (CountControllerPrerequisites(Movement, this) == 1)
	{
		OrderedCharacterMovement = Movement;
		UE_LOG(
			Logdemo_map,
			Verbose,
			TEXT("P8.7: Controller-before-CharacterMovement ordering installed for %s."),
			*GetNameSafe(InPawn));
	}
}

void Ademo_mapPlayerController::RemoveMovementOrdering()
{
	if (UCharacterMovementComponent* Movement =
		OrderedCharacterMovement.Get())
	{
		Movement->PrimaryComponentTick.RemovePrerequisite(
			this,
			PrimaryActorTick);
	}
	OrderedCharacterMovement.Reset();
}

void Ademo_mapPlayerController::StartMoveForward()
{
#if !UE_BUILD_SHIPPING
	if (!bMoveForwardPressed)
	{
		bRecordedMovementAppliedForCurrentPress = false;
	}
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::MoveActionHandlerEntered);
	Recorddemo_mapInputConsumptionTraceStage(
		Edemo_mapInputConsumptionStage::MoveActionHandlerEntered,
		GetPawn(),
		this);
#endif
	if (bInputRestoreDiagnostics && !bLoggedFirstMoveInput)
	{
		bLoggedFirstMoveInput = true;
		LogInputRestoreDiagnostics(TEXT("L.FirstW"));
	}
	if (!IsGameplayInputAllowed())
	{
#if !UE_BUILD_SHIPPING
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::MoveHandlerRejected);
#endif
		return;
	}
	bMoveForwardPressed = true;
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ControlInputBecameNonZero);
#endif
}

void Ademo_mapPlayerController::StopMoveForward()
{
	bMoveForwardPressed = false;
#if !UE_BUILD_SHIPPING
	bRecordedMovementAppliedForCurrentPress = false;
#endif
}

void Ademo_mapPlayerController::StartMoveBackward()
{
	if (!IsGameplayInputAllowed()) return;
	bMoveBackwardPressed = true;
}

void Ademo_mapPlayerController::StopMoveBackward()
{
	bMoveBackwardPressed = false;
}

void Ademo_mapPlayerController::StartMoveRight()
{
	if (!IsGameplayInputAllowed()) return;
	bMoveRightPressed = true;
}

void Ademo_mapPlayerController::StopMoveRight()
{
	bMoveRightPressed = false;
}

void Ademo_mapPlayerController::StartMoveLeft()
{
	if (!IsGameplayInputAllowed()) return;
	bMoveLeftPressed = true;
}

void Ademo_mapPlayerController::StopMoveLeft()
{
	bMoveLeftPressed = false;
}

void Ademo_mapPlayerController::ApplyCameraRelativeMovement()
{
	APawn* ControlledPawn = GetPawn();
	const float ForwardAxis = (bMoveForwardPressed ? 1.0f : 0.0f) - (bMoveBackwardPressed ? 1.0f : 0.0f);
	const float RightAxis = (bMoveRightPressed ? 1.0f : 0.0f) - (bMoveLeftPressed ? 1.0f : 0.0f);
	if (ControlledPawn == nullptr || (ForwardAxis == 0.0f && RightAxis == 0.0f))
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FRotationMatrix CameraYawMatrix(FRotator(0.0f, ViewRotation.Yaw, 0.0f));
	const FVector Forward = CameraYawMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = CameraYawMatrix.GetUnitAxis(EAxis::Y);
	const FVector MovementDirection = (Forward * ForwardAxis + Right * RightAxis).GetSafeNormal();
	if (LastMovementApplicationFrame == GFrameCounter)
	{
		return;
	}
	LastMovementApplicationFrame = GFrameCounter;

	ControlledPawn->AddMovementInput(MovementDirection, 1.0f, false);
#if !UE_BUILD_SHIPPING
	Recorddemo_mapInputConsumptionTraceStage(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		ControlledPawn,
		this,
		-1.0f,
		-1.0,
		MovementDirection);
	Recorddemo_mapInputConsumptionTraceStage(
		Edemo_mapInputConsumptionStage::PreCharacterMovement,
		ControlledPawn,
		this,
		-1.0f,
		-1.0,
		MovementDirection);
	if (!bRecordedMovementAppliedForCurrentPress)
	{
		bRecordedMovementAppliedForCurrentPress = true;
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::MovementApplied);
	}
#endif
}

void Ademo_mapPlayerController::UpdateMouseFacing()
{
	bCurrentAimPointValid = false;
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, true, Hit) && Hit.bBlockingHit)
	{
		LastValidAimPoint = Hit.Location;
		bHasValidAimPoint = true;
		bCurrentAimPointValid = true;
	}

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr || !bHasValidAimPoint)
	{
		return;
	}

	FVector AimDirection = LastValidAimPoint - ControlledPawn->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.SizeSquared() < FMath::Square(MinimumAimDistance))
	{
		return;
	}
	LastValidAimDirection = AimDirection.GetSafeNormal();
	bHasValidAimDirection = true;

	const float DesiredYaw = AimDirection.Rotation().Yaw;
	ControlledPawn->SetActorRotation(FRotator(0.0f, DesiredYaw, 0.0f));
}

void Ademo_mapPlayerController::ReloadCurrentLevel()
{
	if (!IsGameplayInputAllowed()) return;
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		if (Udemo_mapSkillComponent* Skills = GetSkillComponent())
		{
			Skills->CancelAllSkillState();
		}
		if (Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode()))
		{
			if (Ademo_mapV3ProgressionManager* V3 = Mode->GetV3ProgressionManager())
			{
				V3->RequestSettlementAndReload(Edemo_mapRunEndReason::Abandon);
				return;
			}
		}
		UGameplayStatics::OpenLevel(this, FName(*World->GetMapName()), true);
	}
}

void Ademo_mapPlayerController::StartBasicAttack()
{
	if (bInputRestoreDiagnostics && !bLoggedFirstAttackInput)
	{
		bLoggedFirstAttackInput = true;
		LogInputRestoreDiagnostics(TEXT("L.FirstLMB"));
	}
	if (!IsGameplayInputAllowed()) return;
	if (Udemo_mapSkillComponent* Skills = GetSkillComponent(); Skills != nullptr && Skills->IsGroundCircleTargeting())
	{
		Skills->ConfirmGroundCircle();
		return;
	}
	TryBasicAttack();
}

void Ademo_mapPlayerController::ToggleGroundCircle()
{
	if (!IsGameplayInputAllowed()) return;
	if (Udemo_mapSkillComponent* Skills = GetSkillComponent())
	{
		Skills->ToggleGroundCircleTargeting();
		if (Skills->IsGroundCircleTargeting()) Skills->UpdateGroundTargetPreview(bCurrentAimPointValid, LastValidAimPoint);
	}
}

void Ademo_mapPlayerController::CancelGroundCircle()
{
	if (Udemo_mapSkillComponent* Skills = GetSkillComponent()) Skills->CancelGroundCircleTargeting();
}

void Ademo_mapPlayerController::CastSelfSector()
{
	if (!IsGameplayInputAllowed()) return;
	if (Udemo_mapSkillComponent* Skills = GetSkillComponent())
	{
		Skills->CancelGroundCircleTargeting();
		Skills->TryCastSelfSector(GetLastValidAimDirection());
	}
}

void Ademo_mapPlayerController::FireStraightProjectile()
{
	if (!IsGameplayInputAllowed()) return;
	if (Udemo_mapSkillComponent* Skills = GetSkillComponent())
	{
		Skills->CancelGroundCircleTargeting();
		Skills->TryFireStraightProjectile(GetLastValidAimDirection());
	}
}

void Ademo_mapPlayerController::ToggleThrownWeaponTrajectory()
{
#if !UE_BUILD_SHIPPING
	++ThrownWeaponTrajectoryToggleInvocationCount;
	LastThrownWeaponTrajectoryToggleRead =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult();
	LastThrownWeaponTrajectoryToggleRequest =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest();
	LastThrownWeaponTrajectoryToggleResult =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult();
#endif

	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult Read =
		ReadThrownWeaponInputChoiceInteraction();
#if !UE_BUILD_SHIPPING
	LastThrownWeaponTrajectoryToggleRead = Read;
#endif
	if (!Read.IsProjected())
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("Thrown-weapon trajectory toggle rejected before capture: %s"),
			*Read.GetDiagnostic());
		return;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
		TryCaptureTrajectoryToggle(Read.GetReadModel(), Request))
	{
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("Thrown-weapon trajectory toggle could not capture a current request."));
		return;
	}
#if !UE_BUILD_SHIPPING
	LastThrownWeaponTrajectoryToggleRequest = Request;
#endif

	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult Result =
		RouteThrownWeaponInputChoiceInteractionRequest(Request);
#if !UE_BUILD_SHIPPING
	LastThrownWeaponTrajectoryToggleResult = Result;
#endif
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("Thrown-weapon trajectory toggle %s: %s"),
		Result.IsAccepted() ? TEXT("accepted") : TEXT("rejected"),
		*Result.GetDiagnostic());
}

void Ademo_mapPlayerController::SetThrownWeaponArcTargetFromPointerAim()
{
	const FVector AimDirection = GetLastValidAimDirection();
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult Result =
		RouteThrownWeaponArcTargetInteraction(
			FVector2D(AimDirection.X, AimDirection.Y));
#if !UE_BUILD_SHIPPING
	++ThrownWeaponArcEditingInputInvocationCount;
	LastThrownWeaponArcEditingInputResult = Result;
#endif
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("Thrown-weapon Arc pointer target %s: %s"),
		Result.IsAccepted() ? TEXT("accepted") : TEXT("rejected"),
		*Result.GetDiagnostic());
}

void Ademo_mapPlayerController::IncreaseThrownWeaponArcApex()
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult Result =
		RouteThrownWeaponArcApexAdjustmentInteraction(
			ThrownWeaponArcApexAdjustmentStep);
#if !UE_BUILD_SHIPPING
	++ThrownWeaponArcEditingInputInvocationCount;
	LastThrownWeaponArcEditingInputResult = Result;
#endif
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("Thrown-weapon Arc apex increase %s: %s"),
		Result.IsAccepted() ? TEXT("accepted") : TEXT("rejected"),
		*Result.GetDiagnostic());
}

void Ademo_mapPlayerController::DecreaseThrownWeaponArcApex()
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult Result =
		RouteThrownWeaponArcApexAdjustmentInteraction(
			-ThrownWeaponArcApexAdjustmentStep);
#if !UE_BUILD_SHIPPING
	++ThrownWeaponArcEditingInputInvocationCount;
	LastThrownWeaponArcEditingInputResult = Result;
#endif
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("Thrown-weapon Arc apex decrease %s: %s"),
		Result.IsAccepted() ? TEXT("accepted") : TEXT("rejected"),
		*Result.GetDiagnostic());
}

void Ademo_mapPlayerController::ClearThrownWeaponArcTarget()
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult Result =
		RouteThrownWeaponArcTargetClearInteraction();
#if !UE_BUILD_SHIPPING
	++ThrownWeaponArcEditingInputInvocationCount;
	LastThrownWeaponArcEditingInputResult = Result;
#endif
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("Thrown-weapon Arc target clear %s: %s"),
		Result.IsAccepted() ? TEXT("accepted") : TEXT("rejected"),
		*Result.GetDiagnostic());
}

void Ademo_mapPlayerController::StartSpiritEvasion()
{
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		RouteSpiritEvasionStartInput();
#if !UE_BUILD_SHIPPING
	++SpiritEvasionInputInvocationCount;
	LastSpiritEvasionInputResult = Result;
#else
	(void)Result;
#endif
}

Fdemo_mapShanmenSpiritEvasionInputResult
Ademo_mapPlayerController::RouteSpiritEvasionStartInput()
{
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	return Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
		IsGameplayInputAllowed(),
		Mode != nullptr,
		[this]() { return GetLastValidAimDirection(); },
		[Mode](const FVector& Direction)
		{
			return Mode
				? Mode->RouteSpiritEvasionStartIntent(Direction)
				: Fdemo_mapShanmenSpiritEvasionProductRouteResult();
		});
}

Fdemo_mapShanmenThrownWeaponArcLaunchInputResult
Ademo_mapPlayerController::RouteThrownWeaponArcLaunchCommand(
	const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
{
	Ademo_mapGameMode* Mode = nullptr;
	return Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter::Route(
		Command,
		IsGameplayInputAllowed(),
		InputSurfaceState == TEXT("Gameplay"),
		InputModeState == TEXT("GameOnly"),
		[this, &Mode]()
		{
			UWorld* World = GetWorld();
			Mode = World
				? Cast<Ademo_mapGameMode>(World->GetAuthGameMode())
				: nullptr;
			return Mode != nullptr;
		},
		[&Mode]()
		{
			return Mode
				? Mode->GetThrownWeaponInputChoiceState()
				: Fdemo_mapShanmenThrownWeaponInputChoiceState();
		},
		[this, &Mode](
			const int32 HotbarSlotNumber,
			const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy)
		{
			return Mode
				? Mode->RouteThrownWeaponArcChoiceFromSourceHotbarInput(
					HotbarSlotNumber, GetPawn(), Policy)
				: Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult();
		});
}

Fdemo_mapShanmenThrownWeaponArcConfirmationResult
Ademo_mapPlayerController::RouteThrownWeaponArcConfirmation(
	const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent)
{
	return ThrownWeaponArcConfirmationOwner.Confirm(
		Intent,
		IsGameplayInputAllowed()
			&& InputSurfaceState == TEXT("Gameplay")
			&& InputModeState == TEXT("GameOnly"),
		[this]()
		{
			UWorld* World = GetWorld();
			Ademo_mapGameMode* Mode = World
				? Cast<Ademo_mapGameMode>(World->GetAuthGameMode())
				: nullptr;
			return Mode
				? Mode->GetThrownWeaponInputChoiceState()
				: Fdemo_mapShanmenThrownWeaponInputChoiceState();
		},
		[this](
			const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
		{
			return RouteThrownWeaponArcLaunchCommand(Command);
		});
}

Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult
Ademo_mapPlayerController::RouteThrownWeaponHotbarConfirmationInput(
	const int32 HotbarSlotNumber)
{
	const bool bInputAllowed = IsGameplayInputAllowed()
		&& InputSurfaceState == TEXT("Gameplay")
		&& InputModeState == TEXT("GameOnly");
	Ademo_mapGameMode* Mode = nullptr;
	if (bInputAllowed)
	{
		UWorld* World = GetWorld();
		Mode = World
			? Cast<Ademo_mapGameMode>(World->GetAuthGameMode())
			: nullptr;
	}

	return ThrownWeaponHotbarConfirmationAdapter.Route(
		HotbarSlotNumber,
		bInputAllowed,
		Mode != nullptr,
		[&Mode]()
		{
			return Mode
				? Mode->GetThrownWeaponInputChoiceState()
				: Fdemo_mapShanmenThrownWeaponInputChoiceState();
		},
		[]()
		{
			return Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::
				GetCanonical();
		},
		[this, &Mode](const int32 SlotNumber)
		{
			return Mode
				? Mode->RouteThrownWeaponHotbarInput(
					SlotNumber,
					GetPawn(),
					[this]() { return GetLastValidAimDirection(); })
				: Fdemo_mapShanmenThrownWeaponInputResult();
		},
		[this](
			const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent)
		{
			return RouteThrownWeaponArcConfirmation(Intent);
		});
}

Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult
Ademo_mapPlayerController::RouteThrownWeaponInputChoiceCommand(
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
{
	Ademo_mapGameMode* Mode = nullptr;
	return Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter::Route(
		Command,
		IsGameplayInputAllowed(),
		InputSurfaceState == TEXT("Gameplay"),
		InputModeState == TEXT("GameOnly"),
		[this, &Mode]()
		{
			UWorld* World = GetWorld();
			Mode = World
				? Cast<Ademo_mapGameMode>(World->GetAuthGameMode())
				: nullptr;
			return Mode != nullptr;
		},
		[&Mode](
			const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&
				ChoiceCommand)
		{
			return Mode
				? Mode->SubmitThrownWeaponInputChoiceCommand(ChoiceCommand)
				: Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult();
		});
}

Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult
Ademo_mapPlayerController::RouteThrownWeaponInputChoiceIntent(
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent)
{
	Ademo_mapGameMode* Mode = nullptr;
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult Result =
		Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter::Route(
		Intent,
		[this, &Mode]()
		{
			UWorld* World = GetWorld();
			Mode = World
				? Cast<Ademo_mapGameMode>(World->GetAuthGameMode())
				: nullptr;
			return Mode != nullptr;
		},
		[&Mode]()
		{
			return Mode
				? Mode->GetThrownWeaponInputChoiceState()
				: Fdemo_mapShanmenThrownWeaponInputChoiceState();
		},
		[this](
			const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
		{
			return RouteThrownWeaponInputChoiceCommand(Command);
		});
	if (Result.IsAccepted() && Mode != nullptr)
	{
		FString PreviewDiagnostic;
		if (!Mode->RefreshThrownWeaponArcPreLaunchPreview(
				GetPawn(), Intent.GetKind(), PreviewDiagnostic))
		{
			UE_LOG(
				Logdemo_map,
				Warning,
				TEXT("0_0_10_THROWN_ARC_PREVIEW Event=LiveEditRefreshRejected Kind=%d Diagnostic=%s"),
				static_cast<int32>(Intent.GetKind()),
				*PreviewDiagnostic);
		}
	}
	return Result;
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult
Ademo_mapPlayerController::ReadThrownWeaponInputChoiceInteraction()
{
	Ademo_mapGameMode* Mode = nullptr;
	return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
		[this, &Mode]()
		{
			UWorld* World = GetWorld();
			Mode = World
				? Cast<Ademo_mapGameMode>(World->GetAuthGameMode())
				: nullptr;
			return Mode != nullptr;
		},
		[&Mode]()
		{
			return Mode
				? Mode->GetThrownWeaponInputChoiceState()
				: Fdemo_mapShanmenThrownWeaponInputChoiceState();
		});
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
Ademo_mapPlayerController::RouteThrownWeaponInputChoiceInteractionRequest(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Request)
{
	return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
		Execute(
			Request,
			[this]()
			{
				return ReadThrownWeaponInputChoiceInteraction();
			},
			[this](
				const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent)
			{
				return RouteThrownWeaponInputChoiceIntent(Intent);
			});
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
Ademo_mapPlayerController::RouteThrownWeaponArcTargetInteraction(
	const FVector2D& RawTargetIntent)
{
	return RouteArcEditingInteraction(
		*this,
		[&RawTargetIntent](
			const FArcEditingInteractionReadResult& Read,
			FArcEditingInteractionRequest& OutRequest)
		{
			return Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition::
				TryComposeTargetRequest(
					Read, RawTargetIntent, OutRequest);
		});
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
Ademo_mapPlayerController::RouteThrownWeaponArcApexAdjustmentInteraction(
	const double RawNormalizedDelta)
{
	return RouteArcEditingInteraction(
		*this,
		[RawNormalizedDelta](
			const FArcEditingInteractionReadResult& Read,
			FArcEditingInteractionRequest& OutRequest)
		{
			return Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition::
				TryComposeApexAdjustmentRequest(
					Read, RawNormalizedDelta, OutRequest);
		});
}

Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
Ademo_mapPlayerController::RouteThrownWeaponArcTargetClearInteraction()
{
	return RouteArcEditingInteraction(
		*this,
		[](
			const FArcEditingInteractionReadResult& Read,
			FArcEditingInteractionRequest& OutRequest)
		{
			return Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition::
				TryComposeTargetClearRequest(Read, OutRequest);
		});
}

void Ademo_mapPlayerController::StartWeaponGuard()
{
	const Fdemo_mapShanmenWeaponGuardInputResult Result =
		RouteWeaponGuardStartInput();
#if !UE_BUILD_SHIPPING
	++WeaponGuardPressInvocationCount;
	LastWeaponGuardInputResult = Result;
#else
	(void)Result;
#endif
}

void Ademo_mapPlayerController::StopWeaponGuard()
{
	const Fdemo_mapShanmenWeaponGuardReleaseInputResult Result =
		RouteWeaponGuardReleaseInput();
#if !UE_BUILD_SHIPPING
	++WeaponGuardReleaseInvocationCount;
	LastWeaponGuardReleaseInputResult = Result;
#else
	(void)Result;
#endif
}

Fdemo_mapShanmenWeaponGuardInputResult
Ademo_mapPlayerController::RouteWeaponGuardStartInput()
{
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	Fdemo_mapShanmenWeaponGuardSessionStartResult SessionStart;
	Fdemo_mapShanmenWeaponGuardInputResult Result =
		Fdemo_mapShanmenWeaponGuardInputAdapter::RouteStartInput(
			IsGameplayInputAllowed(),
			Mode != nullptr,
			[Mode]()
			{
				return Mode
					? Mode->CaptureWeaponGuardInputTimeline()
					: Fdemo_mapShanmenWeaponGuardInputTimelineSample();
			},
			[Mode, &SessionStart](
				const FGuid& TimelineId,
				int64 ActiveStartTick)
			{
				if (!Mode)
				{
					return Fdemo_mapShanmenWeaponGuardProductRouteResult();
				}
				SessionStart = Mode->RouteWeaponGuardStartIntent(
					TimelineId,
					ActiveStartTick);
				return SessionStart.IsStarted()
					? SessionStart.Route
					: Fdemo_mapShanmenWeaponGuardProductRouteResult();
			});
	if (Mode && !SessionStart.IsStarted()
		&& !SessionStart.Diagnostic.IsEmpty())
	{
		Result.Status =
			Edemo_mapShanmenWeaponGuardInputStatus::ProductRejected;
		Result.Diagnostic = SessionStart.Diagnostic;
	}
	return Result;
}

Fdemo_mapShanmenWeaponGuardReleaseInputResult
Ademo_mapPlayerController::RouteWeaponGuardReleaseInput()
{
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	return Fdemo_mapShanmenWeaponGuardInputAdapter::RouteReleaseInput(
		Mode != nullptr,
		[Mode]()
		{
			return Mode
				? Mode->RouteWeaponGuardReleaseIntent()
				: Fdemo_mapShanmenWeaponGuardSessionTransitionResult();
		});
}

int32 Ademo_mapPlayerController::ResolveHotbarSlotForKey(
	const FKey& Key)
{
	const Fdemo_mapInputBindingSettings& Settings = Fdemo_mapInputBindingSettings::Get();
	for (int32 Slot = 1; Slot <= 9; ++Slot)
	{
		if (Settings.GetKey(Fdemo_mapInputActionRegistry::HotbarActionId(Slot)) == Key) return Slot;
	}
	return INDEX_NONE;
}

void Ademo_mapPlayerController::UseHotbarSlot(int32 SlotNumber)
{
#if !UE_BUILD_SHIPPING
	LastHotbarSlotForwardedForAutomation = SlotNumber;
#endif
	if (SlotNumber < 1 || SlotNumber > FCodeBHotbarBindings::SlotCount || !IsGameplayInputAllowed())
	{
		return;
	}
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	bool bRouteExistingThrownConfirmation = true;
	bool bCompleteArcPreLaunchConfirmation = false;
	if (Mode != nullptr)
	{
		const Fdemo_mapShanmenThrownWeaponArcPreLaunchHotbarResult
			PreLaunch = Mode->RouteThrownWeaponArcPreLaunchHotbarInput(
				SlotNumber, GetPawn());
		if (!PreLaunch.IsValid())
		{
			UE_LOG(
				Logdemo_map,
				Warning,
				TEXT("0_0_10_THROWN_ARC_PREVIEW Event=PreLaunchProtocolRejected Slot=%d Diagnostic=%s"),
				SlotNumber,
				*PreLaunch.GetDiagnostic());
			return;
		}
		if (PreLaunch.ShouldPassThrough())
		{
			bRouteExistingThrownConfirmation = false;
		}
		else if (PreLaunch.ShouldRouteExistingConfirmation())
		{
			bCompleteArcPreLaunchConfirmation =
				PreLaunch.IsArcConfirmationRequested();
		}
		else
		{
			return;
		}
	}
	if (bRouteExistingThrownConfirmation)
	{
		const Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult
			ThrownRoute = RouteThrownWeaponHotbarConfirmationInput(SlotNumber);
		if (Mode != nullptr && bCompleteArcPreLaunchConfirmation)
		{
			FString CompletionDiagnostic;
			if (!Mode->CompleteThrownWeaponArcPreLaunchConfirmation(
					SlotNumber,
					ThrownRoute.IsAccepted(),
					CompletionDiagnostic))
			{
				UE_LOG(
					Logdemo_map,
					Warning,
					TEXT("0_0_10_THROWN_ARC_PREVIEW Event=ConfirmationCompletionRejected Slot=%d LaunchAccepted=%d Diagnostic=%s"),
					SlotNumber,
					ThrownRoute.IsAccepted() ? 1 : 0,
					*CompletionDiagnostic);
			}
		}
		if (!ThrownRoute.ShouldPassThrough())
		{
			return;
		}
	}
	if (Mode)
	{
		const Fdemo_mapShanmenMeridianShockTreatmentInputResult
			TreatmentRoute =
				Mode->RouteMeridianShockTreatmentHotbarInput(SlotNumber);
		if (!TreatmentRoute.ShouldPassThrough())
		{
			return;
		}
	}
	if (Ademo_mapV3ProgressionManager* Manager = Mode ? Mode->GetV3ProgressionManager() : nullptr)
	{
		Manager->RequestUseBoundQuickSlot(SlotNumber);
	}
}

void Ademo_mapPlayerController::UseHotbarSlot1() { UseHotbarSlot(1); }
void Ademo_mapPlayerController::UseHotbarSlot2() { UseHotbarSlot(2); }
void Ademo_mapPlayerController::UseHotbarSlot3() { UseHotbarSlot(3); }
void Ademo_mapPlayerController::UseHotbarSlot4() { UseHotbarSlot(4); }
void Ademo_mapPlayerController::UseHotbarSlot5() { UseHotbarSlot(5); }
void Ademo_mapPlayerController::UseHotbarSlot6() { UseHotbarSlot(6); }
void Ademo_mapPlayerController::UseHotbarSlot7() { UseHotbarSlot(7); }
void Ademo_mapPlayerController::UseHotbarSlot8() { UseHotbarSlot(8); }
void Ademo_mapPlayerController::UseHotbarSlot9() { UseHotbarSlot(9); }

bool Ademo_mapPlayerController::TryBasicAttack()
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (ControlledPawn == nullptr || World == nullptr || !IsGameplayInputAllowed())
	{
		UE_LOG(Logdemo_map, Warning, TEXT("T4: basic attack rejected because the pawn is unavailable or defeated."));
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime < BasicAttackReadyTime)
	{
		UE_LOG(Logdemo_map, Verbose, TEXT("P5: basic attack rejected by cooldown. Remaining=%.3f"), BasicAttackReadyTime - CurrentTime);
		return false;
	}

	FVector Forward = ControlledPawn->GetActorForwardVector();
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		UE_LOG(Logdemo_map, Warning, TEXT("T4: basic attack rejected because the pawn forward vector is invalid."));
		return false;
	}

	BasicAttackReadyTime = CurrentTime
		+ Fdemo_mapPlayerCombat::CaptureEffectiveCooldown(
			ControlledPawn,
			BasicAttackCooldown);

	const FVector Start = ControlledPawn->GetActorLocation() + (FVector::UpVector * BasicAttackVerticalOffset) + (Forward * BasicAttackStartOffset);
	const FVector End = Start + (Forward * BasicAttackRange);
	const FCollisionShape AttackSphere = FCollisionShape::MakeSphere(BasicAttackRadius);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(T4BasicAttack), false, ControlledPawn);
	QueryParams.AddIgnoredActor(ControlledPawn);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectQueryParams, AttackSphere, QueryParams);

	if (Ademo_mapGameMode* Mode = Cast<Ademo_mapGameMode>(
		World->GetAuthGameMode());
		Mode && Mode->ShouldUseM01BasicSwordProductPath())
	{
		const Fdemo_mapBasicSwordProductExecutionResult ProductResult =
			Mode->ExecuteM01PlayerBasicSwordSweep(
				Fdemo_mapPlayerCombat::CaptureAttackPower(ControlledPawn),
				Hits);
		// M01 is an atomic route switch. A rejected product action must not
		// fall through to the retained ApplyDamage compatibility writer.
		return ProductResult.AppliedDamage();
	}

	TSet<AActor*> DamagedActors;
	bool bAppliedDamage = false;
	const float DamageSnapshot = Fdemo_mapPlayerCombat::CaptureOutgoingDamage(ControlledPawn, BasicAttackCoefficient);
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor == nullptr || DamagedActors.Contains(HitActor) || !HitActor->CanBeDamaged())
		{
			continue;
		}
		if (!Fdemo_mapCombatTargeting::CanAffect(ControlledPawn, HitActor, BasicAttackParams.TargetFilter))
		{
			continue;
		}

		DamagedActors.Add(HitActor);
		const float AppliedDamage = UGameplayStatics::ApplyDamage(HitActor, DamageSnapshot, this, ControlledPawn, nullptr);
		bAppliedDamage |= AppliedDamage > 0.0f;
	}

	UE_LOG(Logdemo_map, Log, TEXT("T4: basic attack sweep start=(%.1f, %.1f, %.1f) end=(%.1f, %.1f, %.1f) hits=%d damaged=%d"),
		Start.X, Start.Y, Start.Z, End.X, End.Y, End.Z, Hits.Num(), DamagedActors.Num());
	return bAppliedDamage;
}

float Ademo_mapPlayerController::GetBasicAttackCooldownRemaining() const
{
	return GetWorld()
		? FMath::Max(
			0.0f,
			BasicAttackReadyTime - GetWorld()->GetTimeSeconds())
		: 0.0f;
}

bool Ademo_mapPlayerController::IsGameplayInputAllowed() const
{
	const APawn* ControlledPawn = GetPawn();
	const Udemo_mapPlayerHealthComponent* Health = ControlledPawn != nullptr ? ControlledPawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	const Ademo_mapGameMode* Mode = GetWorld() != nullptr ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	const Ademo_mapV3ProgressionManager* V3 = Mode ? Mode->GetV3ProgressionManager() : nullptr;
	return !bSettlementInputLockHeld
		&& !bProfilePreparationInputLockHeld
		&& !bSearchContainerInputLockHeld
		&& !bInventoryInputLockHeld
		&& (Health == nullptr || !Health->IsDefeated())
		&& (Mode == nullptr || !Mode->IsResetPending())
		&& (V3 == nullptr || (!V3->IsInventoryOpen() && !V3->IsSettlementPending() && !V3->IsSearchContainerOpen()));
}

Udemo_mapSkillComponent* Ademo_mapPlayerController::GetSkillComponent() const
{
	return GetPawn() != nullptr ? GetPawn()->FindComponentByClass<Udemo_mapSkillComponent>() : nullptr;
}

void Ademo_mapPlayerController::BeginInteractV3()
{
	if (bInputRestoreDiagnostics && !bLoggedFirstInteractInput)
	{
		bLoggedFirstInteractInput = true;
		LogInputRestoreDiagnostics(TEXT("L.FirstG"));
	}
	if (!IsGameplayInputAllowed()) return;
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (Ademo_mapV3ProgressionManager* V3 = Mode->GetV3ProgressionManager()) V3->RequestInteractFocused();
	}
}

void Ademo_mapPlayerController::EndInteractV3()
{
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (Ademo_mapV3ProgressionManager* V3 = Mode->GetV3ProgressionManager())
		{
			V3->ReleaseInteractFocused();
		}
	}
}

void Ademo_mapPlayerController::ToggleV3Inventory()
{
	if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (Ademo_mapV3ProgressionManager* V3 = Mode->GetV3ProgressionManager())
		{
			// Closing the already-open inventory is the one legal Tab action while the
			// inventory itself owns the input surface. Preparation still cannot open it.
			if (V3->IsInventoryOpen() || IsGameplayInputAllowed()) V3->ToggleInventory();
		}
	}
}

void Ademo_mapPlayerController::HandleBackAction()
{
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	if (Ademo_mapV3ProgressionManager* V3 =
		Mode ? Mode->GetV3ProgressionManager() : nullptr)
	{
		if (V3->IsSearchContainerOpen())
		{
			V3->CloseSearchContainer(TEXT("InputBack"), true);
			return;
		}
		if (V3->IsInventoryOpen())
		{
			V3->CloseInventory();
			return;
		}
		if (bProfilePreparationInputLockHeld
			&& ProfilePreparationFocusWidget.Get()
				== V3->GetProfilePreparationWidget())
		{
			V3->ReturnToSectNavigation();
		}
	}
}

void Ademo_mapPlayerController::HandleSearchEscapeAction()
{
	Ademo_mapGameMode* Mode = GetWorld()
		? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode())
		: nullptr;
	if (Ademo_mapV3ProgressionManager* V3 =
		Mode ? Mode->GetV3ProgressionManager() : nullptr)
	{
		if (V3->IsSearchContainerOpen())
		{
			V3->CloseSearchContainer(TEXT("InputEscape"), true);
			return;
		}
		if (V3->IsInventoryOpen())
		{
			V3->CloseInventory();
		}
	}
}

void Ademo_mapPlayerController::BeginSettlementInputLock(
	UUserWidget* FocusWidget)
{
	if (bSettlementInputLockHeld
		&& SettlementFocusWidget.Get() == FocusWidget)
	{
		ReconcileInputContext(TEXT("SettlementDuplicateAcquire"));
		return;
	}
	bSettlementInputLockHeld = true;
	SettlementFocusWidget = FocusWidget;
	ReconcileInputContext(TEXT("B.SettlementLockApplied"));
}

bool Ademo_mapPlayerController::EndSettlementInputLock()
{
	if (!bSettlementInputLockHeld)
	{
		SettlementFocusWidget.Reset();
		ReconcileInputContext(TEXT("SettlementDuplicateRelease"));
		return bProfilePreparationInputLockHeld || IsGameplayInputAllowed();
	}
	bSettlementInputLockHeld = false;
	SettlementFocusWidget.Reset();
	ReconcileInputContext(TEXT("SettlementReleased"));
	return bProfilePreparationInputLockHeld
		? InputSurfaceState == TEXT("ProfilePreparation")
		: IsGameplayInputAllowed();
}

void Ademo_mapPlayerController::BeginProfilePreparationInputLock(UUserWidget* FocusWidget)
{
	if (bProfilePreparationInputLockHeld && ProfilePreparationFocusWidget.Get() == FocusWidget)
	{
		ReconcileInputContext(TEXT("PreparationDuplicateAcquire"));
		return;
	}
	bProfilePreparationInputLockHeld = true;
	ProfilePreparationFocusWidget = FocusWidget;
	++ProfilePreparationInputApplyCount;
	ReconcileInputContext(TEXT("P.ProfilePreparationLockApplied"));
}

void Ademo_mapPlayerController::BeginSearchContainerInputLock(UUserWidget* FocusWidget)
{
	if (bSearchContainerInputLockHeld && SearchContainerFocusWidget.Get() == FocusWidget)
	{
		ReconcileInputContext(TEXT("SearchDuplicateAcquire"));
		return;
	}
	bSearchContainerInputLockHeld = true;
	SearchContainerFocusWidget = FocusWidget;
	ReconcileInputContext(TEXT("SearchContainerLockApplied"));
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::SearchLockApplied);
#endif
}

void Ademo_mapPlayerController::BeginInventoryInputLock(
	UUserWidget* FocusWidget)
{
	if (bInventoryInputLockHeld
		&& InventoryFocusWidget.Get() == FocusWidget)
	{
		ReconcileInputContext(TEXT("InventoryDuplicateAcquire"));
		return;
	}
	bInventoryInputLockHeld = true;
	InventoryFocusWidget = FocusWidget;
	ReconcileInputContext(TEXT("InventoryLockApplied"));
}

bool Ademo_mapPlayerController::RestoreGameplayControlFromSearchContainer()
{
	if (!bSearchContainerInputLockHeld)
	{
		ReconcileInputContext(TEXT("SearchDuplicateRelease"));
		return !bSettlementInputLockHeld && !bProfilePreparationInputLockHeld;
	}
	bSearchContainerInputLockHeld = false;
	SearchContainerFocusWidget.Reset();
	ReconcileInputContext(TEXT("SearchContainerReleased"));
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::SearchReleased);
#endif
	return IsGameplayInputAllowed() && !IsMoveInputIgnored() && !IsLookInputIgnored();
}

bool Ademo_mapPlayerController::RestoreGameplayControlFromInventory()
{
	if (!bInventoryInputLockHeld)
	{
		ReconcileInputContext(TEXT("InventoryDuplicateRelease"));
		return !bSettlementInputLockHeld
			&& !bProfilePreparationInputLockHeld
			&& !bSearchContainerInputLockHeld;
	}
	bInventoryInputLockHeld = false;
	InventoryFocusWidget.Reset();
	ReconcileInputContext(TEXT("InventoryReleased"));
	return IsGameplayInputAllowed()
		&& !IsMoveInputIgnored()
		&& !IsLookInputIgnored();
}

bool Ademo_mapPlayerController::RestoreGameplayControlForNewRun()
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.3.4.1 INPUT_RESTORE_FAIL: no controlled pawn is available."));
		return false;
	}
	InstallMovementOrdering(ControlledPawn);

	LogInputRestoreDiagnostics(TEXT("A/B.BeforeRestore"));
	const bool bCountRestore = bSettlementInputLockHeld
		|| bProfilePreparationInputLockHeld
		|| bSearchContainerInputLockHeld
		|| bInventoryInputLockHeld
		|| InputSurfaceState != TEXT("Gameplay");
	bSettlementInputLockHeld = false;
	bProfilePreparationInputLockHeld = false;
	bSearchContainerInputLockHeld = false;
	bInventoryInputLockHeld = false;
	// The former sect shell could acquire an engine input-ignore stack before
	// its widget had a valid UMG parent. A single matching release then left
	// movement and look ignored even though the controller reported GameOnly.
	// A verified new deployment has no legitimate UI lock, so clear the stack
	// atomically before applying the new gameplay surface.
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	bOwnedInputIgnoreApplied = false;
	SettlementFocusWidget.Reset();
	ProfilePreparationFocusWidget.Reset();
	SearchContainerFocusWidget.Reset();
	InventoryFocusWidget.Reset();
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			Movement->Activate(true);
			if (Movement->MovementMode == MOVE_None)
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
		}
	}
	ReconcileInputContext(TEXT("NewRunLocksReleased"));
	if (bCountRestore)
	{
		++GameplayInputRestoreCount;
	}
	LogInputRestoreDiagnostics(TEXT("F.AfterRestore"));

	const bool bRestored = IsGameplayInputAllowed() && !IsMoveInputIgnored() && !IsLookInputIgnored();
	if (bRestored)
	{
		UE_LOG(Logdemo_map, Log, TEXT("0.3.4.1 INPUT_RESTORE_PASS pawn=%s move_ignored=%d look_ignored=%d input=%s"), *GetNameSafe(ControlledPawn), IsMoveInputIgnored(), IsLookInputIgnored(), *InputSurfaceState);
	}
	else
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.3.4.1 INPUT_RESTORE_FAIL pawn=%s move_ignored=%d look_ignored=%d input=%s"), *GetNameSafe(ControlledPawn), IsMoveInputIgnored(), IsLookInputIgnored(), *InputSurfaceState);
	}
	return bRestored;
}

void Ademo_mapPlayerController::ReconcileInputContext(const TCHAR* Reason)
{
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ReconcileBegin);
#endif
	Fdemo_mapInputContextReasons Reasons;
	Reasons.bSearchContainer = bSearchContainerInputLockHeld;
	Reasons.bInventory = bInventoryInputLockHeld;
	Reasons.bPreparation = bProfilePreparationInputLockHeld;
	Reasons.bSettlement = bSettlementInputLockHeld;
	const Fdemo_mapInputContextResolution Resolution =
		Fdemo_mapInputContextResolver::Resolve(Reasons);

	if (Resolution.bOwnedMoveLookIgnoreRequired
		&& !bOwnedInputIgnoreApplied)
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		bOwnedInputIgnoreApplied = true;
	}
	else if (!Resolution.bOwnedMoveLookIgnoreRequired
		&& bOwnedInputIgnoreApplied)
	{
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		bOwnedInputIgnoreApplied = false;
	}

	if (Resolution.bGameplayAllowed)
	{
		if (FSlateApplication::IsInitialized())
		{
#if !UE_BUILD_SHIPPING
			RecordInputRestoreTraceEvent(
				Edemo_mapInputRestoreTraceEvent::KeyboardFocusClearRequested);
#endif
			FSlateApplication::Get().ClearKeyboardFocus(
				EFocusCause::SetDirectly);
		}
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		SetInputMode(Mode);
		InputSurfaceState = TEXT("Gameplay");
		InputModeState = TEXT("GameOnly");
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
#if !UE_BUILD_SHIPPING
		RecordInputRestoreTraceEvent(
			Edemo_mapInputRestoreTraceEvent::GameOnlyApplied);
#endif
	}
	else if (Resolution.bUseGameAndUI)
	{
		FInputModeGameAndUI Mode;
		UUserWidget* FocusWidget =
			bSearchContainerInputLockHeld
				? SearchContainerFocusWidget.Get()
				: InventoryFocusWidget.Get();
		if (FocusWidget)
		{
			Mode.SetWidgetToFocus(
				FocusWidget->TakeWidget());
		}
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(
			EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		InputSurfaceState = bSearchContainerInputLockHeld
			? TEXT("SearchContainer")
			: TEXT("Inventory");
		InputModeState = TEXT("GameAndUI");
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
	}
	else
	{
		FInputModeUIOnly Mode;
		UUserWidget* FocusWidget =
			bSettlementInputLockHeld
				? SettlementFocusWidget.Get()
				: bProfilePreparationInputLockHeld
					? ProfilePreparationFocusWidget.Get()
					: nullptr;
		if (FocusWidget)
		{
			Mode.SetWidgetToFocus(FocusWidget->TakeWidget());
		}
		SetInputMode(Mode);
		InputSurfaceState =
			bSettlementInputLockHeld
				? TEXT("Settlement")
				: TEXT("ProfilePreparation");
		InputModeState = TEXT("UIOnly");
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
	}
	bShowMouseCursor = Resolution.bShowCursor;
	if (Resolution.bOwnedMoveLookIgnoreRequired)
	{
		bMoveForwardPressed = false;
		bMoveBackwardPressed = false;
		bMoveRightPressed = false;
		bMoveLeftPressed = false;
	}
	LogInputRestoreDiagnostics(Reason);
#if !UE_BUILD_SHIPPING
	RecordInputRestoreTraceEvent(
		Edemo_mapInputRestoreTraceEvent::ReconcileEnd);
#endif
}

void Ademo_mapPlayerController::LogInputRestoreDiagnostics(const TCHAR* Phase) const
{
	if (!bInputRestoreDiagnostics)
	{
		return;
	}
	const APawn* ControlledPawn = GetPawn();
	const ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn);
	const UCharacterMovementComponent* Movement =
		ControlledCharacter ? ControlledCharacter->GetCharacterMovement() : nullptr;
	const UWorld* World = GetWorld();
	const Ademo_mapGameMode* Mode =
		World ? Cast<Ademo_mapGameMode>(World->GetAuthGameMode()) : nullptr;
	const Ademo_mapV3ProgressionManager* V3 =
		Mode ? Mode->GetV3ProgressionManager() : nullptr;
	const Udemo_mapItemSubsystem* ItemSubsystem =
		GetGameInstance()
			? GetGameInstance()->GetSubsystem<Udemo_mapItemSubsystem>()
			: nullptr;
	const FVector Location =
		ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	const FVector Velocity =
		ControlledPawn ? ControlledPawn->GetVelocity() : FVector::ZeroVector;
	const FVector PendingInput =
		ControlledPawn
			? ControlledPawn->GetPendingMovementInputVector()
			: FVector::ZeroVector;
	const bool bOrderingInstalled =
		Movement
			&& OrderedCharacterMovement.Get() == Movement
			&& CountControllerPrerequisites(Movement, this) == 1;
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_CONTEXT_TRANSITION utc=%s world=%.6f frame=%llu caller=%s controller=%s pawn=%s run=%s preparation=%d search_open=%d search_container=%s settlement=%d lock_settlement=%d lock_preparation=%d lock_search=%d owned_ignore=%d move_ignored=%d look_ignored=%d gameplay_allowed=%d context=%s input_mode=%s cursor=%d preparation_focus=%d search_focus=%d possessed=%d movement_mode=%d velocity=(%.3f,%.3f,%.3f) location=(%.3f,%.3f,%.3f) pending_input=(%.3f,%.3f,%.3f) movement_ordering=%d controller_tick_group=%d controller_tick_interval=%.6f controller_tick_enabled=%d movement_tick_group=%d movement_tick_interval=%.6f movement_tick_enabled=%d paused=%d dilation=%.3f"),
		*FDateTime::UtcNow().ToIso8601(),
		World ? World->GetTimeSeconds() : -1.0,
		static_cast<uint64>(GFrameCounter),
		Phase,
		*GetNameSafe(this),
		*GetNameSafe(ControlledPawn),
		ItemSubsystem ? *ItemSubsystem->GetActiveRunId().ToString(EGuidFormats::DigitsWithHyphens) : TEXT("none"),
		bProfilePreparationInputLockHeld,
		V3 ? V3->IsSearchContainerOpen() : false,
		V3 ? *GetNameSafe(V3->GetActiveSearchContainer()) : TEXT("none"),
		V3 ? V3->IsSettlementPending() : false,
		bSettlementInputLockHeld,
		bProfilePreparationInputLockHeld,
		bSearchContainerInputLockHeld,
		bOwnedInputIgnoreApplied,
		IsMoveInputIgnored(),
		IsLookInputIgnored(),
		IsGameplayInputAllowed(),
		*InputSurfaceState,
		*InputModeState,
		bShowMouseCursor,
		ProfilePreparationFocusWidget.IsValid(),
		SearchContainerFocusWidget.IsValid(),
		ControlledPawn && ControlledPawn->GetController() == this,
		Movement ? static_cast<int32>(Movement->MovementMode) : INDEX_NONE,
		Velocity.X,
		Velocity.Y,
		Velocity.Z,
		Location.X,
		Location.Y,
		Location.Z,
		PendingInput.X,
		PendingInput.Y,
		PendingInput.Z,
		bOrderingInstalled,
		static_cast<int32>(PrimaryActorTick.TickGroup),
		PrimaryActorTick.TickInterval,
		IsActorTickEnabled(),
		Movement
			? static_cast<int32>(Movement->PrimaryComponentTick.TickGroup)
			: INDEX_NONE,
		Movement ? Movement->PrimaryComponentTick.TickInterval : -1.0f,
		Movement ? Movement->IsComponentTickEnabled() : false,
		World ? World->IsPaused() : false,
		World ? World->GetWorldSettings()->GetEffectiveTimeDilation() : 1.0f);
}
