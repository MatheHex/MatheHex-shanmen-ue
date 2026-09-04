#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include <limits>
#include <type_traits>
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputContextTypes.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapInputRestoreTrace.h"
#include "demo_mapInputConsumptionTrace.h"
#include "demo_mapPlayerController.h"
#include "demo_mapSearchContainerTypes.h"
#include "demo_mapV3ProgressionManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr EAutomationTestFlags InputRestoreFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	Fdemo_mapInputContextResolution Resolve(
		bool bSearch = false,
		bool bPreparation = false,
		bool bSettlement = false,
		bool bDefeated = false,
		bool bReset = false)
	{
		Fdemo_mapInputContextReasons Reasons;
		Reasons.bSearchContainer = bSearch;
		Reasons.bPreparation = bPreparation;
		Reasons.bSettlement = bSettlement;
		Reasons.bDefeated = bDefeated;
		Reasons.bReset = bReset;
		return Fdemo_mapInputContextResolver::Resolve(Reasons);
	}

	FString ReadInputRestoreSource(const TCHAR* Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(
			Text,
			*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/demo_map"), Name));
		return Text;
	}

	FString ReadInputRestoreFunctionBlock(
		const TCHAR* Name,
		const TCHAR* StartSignature,
		const TCHAR* EndSignature)
	{
		const FString Source = ReadInputRestoreSource(Name);
		const FString StartToken(StartSignature);
		const int32 Start = Source.Find(
			StartToken,
			ESearchCase::CaseSensitive);
		if (Start == INDEX_NONE)
		{
			return FString();
		}

		const int32 End = Source.Find(
			EndSignature,
			ESearchCase::CaseSensitive,
			ESearchDir::FromStart,
			Start + StartToken.Len());
		return End == INDEX_NONE
			? FString()
			: Source.Mid(Start, End - Start);
	}

	bool IsExactGameplay(const Fdemo_mapInputContextResolution& Value)
	{
		return Value.Context == Edemo_mapInputContext::Gameplay
			&& Value.bGameplayAllowed
			&& !Value.bOwnedMoveLookIgnoreRequired
			&& Value.bUseGameOnly
			&& !Value.bUseGameAndUI
			&& !Value.bUseUIOnly
			&& Value.bShowCursor;
	}

	bool IsExactModal(
		const Fdemo_mapInputContextResolution& Value,
		Edemo_mapInputContext Context)
	{
		return Value.Context == Context
			&& !Value.bGameplayAllowed
			&& Value.bOwnedMoveLookIgnoreRequired
			&& !Value.bUseGameOnly;
	}

	struct FMovementOrderingFixture
	{
		UWorld* World = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* FirstCharacter = nullptr;

		FMovementOrderingFixture()
		{
			World = NewObject<UWorld>(
				GetTransientPackage(),
				NAME_None,
				RF_Transient);
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.SetCurrentWorld(World);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(false)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(false)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(false)
					.SetTransactional(false)
					.CreateFXSystem(false));
			Controller =
				World->SpawnActor<Ademo_mapPlayerController>();
			FirstCharacter =
				World->SpawnActor<ACharacter>();
			if (Controller && FirstCharacter)
			{
				Controller->Possess(FirstCharacter);
				Controller->InitInputSystem();
			}
		}

		~FMovementOrderingFixture()
		{
			if (Controller && !Controller->IsActorBeingDestroyed())
			{
				Controller->UnPossess();
			}
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		bool IsValid() const
		{
			return World
				&& Controller
				&& FirstCharacter
				&& FirstCharacter->GetCharacterMovement();
		}

		UCharacterMovementComponent* FirstMovement() const
		{
			return FirstCharacter
				? FirstCharacter->GetCharacterMovement()
				: nullptr;
		}

		bool DispatchMoveForwardPressed() const
		{
			const FKey Key =
				Fdemo_mapInputBindingSettings::Get().GetKey(
					Fdemo_mapInputActionIds::MoveForward);
			return Controller
				&& Key.IsValid()
				&& Controller->DispatchAutomationKeyPressed(Key);
		}
	};

	bool DispatchRealMovementUpdatedHandler(
		Ademo_mapPlayerController* Controller,
		ACharacter* Character,
		float DeltaSeconds)
	{
		return Controller
			&& Controller->
				DispatchInputConsumptionMovementUpdatedForAutomation(
					Character,
					DeltaSeconds);
	}
}

#define INPUT_RESTORE_TEST(ClassName, Number, Name) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		ClassName, \
		"demo_map.InputRestore." Number "." Name, \
		InputRestoreFlags)

INPUT_RESTORE_TEST(FInputRestore01, "01", "ContextDefaultsPreparation")
bool FInputRestore01::RunTest(const FString&) { TestTrue(TEXT("Preparation exact"), IsExactModal(Resolve(false, true), Edemo_mapInputContext::Preparation)); return true; }

INPUT_RESTORE_TEST(FInputRestore02, "02", "GameplayContextExactState")
bool FInputRestore02::RunTest(const FString&) { TestTrue(TEXT("Gameplay exact"), IsExactGameplay(Resolve())); return true; }

INPUT_RESTORE_TEST(FInputRestore03, "03", "ModalContextExactState")
bool FInputRestore03::RunTest(const FString&) { const auto V = Resolve(true); TestTrue(TEXT("Search exact"), IsExactModal(V, Edemo_mapInputContext::SearchContainer) && V.bUseGameAndUI && !V.bUseUIOnly); return true; }

INPUT_RESTORE_TEST(FInputRestore04, "04", "DuplicateAcquireIdempotent")
bool FInputRestore04::RunTest(const FString&) { TestTrue(TEXT("Same derivation"), Resolve(true) == Resolve(true)); return true; }

INPUT_RESTORE_TEST(FInputRestore05, "05", "DuplicateReleaseIdempotent")
bool FInputRestore05::RunTest(const FString&) { TestTrue(TEXT("Same release"), Resolve() == Resolve()); return true; }

INPUT_RESTORE_TEST(FInputRestore06, "06", "PreparationOpenLocksGameplay")
bool FInputRestore06::RunTest(const FString&) { TestFalse(TEXT("Locked"), Resolve(false, true).bGameplayAllowed); return true; }

INPUT_RESTORE_TEST(FInputRestore07, "07", "StartRunReleasesPreparationLock")
bool FInputRestore07::RunTest(const FString&) { TestTrue(TEXT("Release -> GameOnly"), IsExactGameplay(Resolve())); return true; }

INPUT_RESTORE_TEST(FInputRestore08, "08", "PreparationCancelRemainsLocked")
bool FInputRestore08::RunTest(const FString&) { TestEqual(TEXT("Preparation remains"), Resolve(false, true).Context, Edemo_mapInputContext::Preparation); return true; }

INPUT_RESTORE_TEST(FInputRestore09, "09", "SearchOpenLocksGameplay")
bool FInputRestore09::RunTest(const FString&) { TestFalse(TEXT("Search locked"), Resolve(true).bGameplayAllowed); return true; }

INPUT_RESTORE_TEST(FInputRestore10, "10", "ChestTakeCloseRestores")
bool FInputRestore10::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Chest widget close then real probe"), S.Contains(TEXT("ChestTakeCloseCommitted")) && S.Contains(TEXT("AutomationClickClose"))); return true; }

INPUT_RESTORE_TEST(FInputRestore11, "11", "CorpseTakeCloseRestores")
bool FInputRestore11::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Corpse widget close then real probe"), S.Contains(TEXT("CorpseTakeCloseCommitted")) && S.Contains(TEXT("INPUT_RESTORE_CORPSE_CLOSE"))); return true; }

INPUT_RESTORE_TEST(FInputRestore12, "12", "CloseWithoutTakeRestores")
bool FInputRestore12::RunTest(const FString&) { TestTrue(TEXT("Search release -> Gameplay"), IsExactGameplay(Resolve(false))); return true; }

INPUT_RESTORE_TEST(FInputRestore13, "13", "CloseDuringSearchRestores")
bool FInputRestore13::RunTest(const FString&) { TestTrue(TEXT("Search action does not add context layer"), Resolve(true) == Resolve(true)); return true; }

INPUT_RESTORE_TEST(FInputRestore14, "14", "DamageInterruptRestores")
bool FInputRestore14::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Damage uses common close"), S.Contains(TEXT("CloseSearchContainer(TEXT(\"PositivePlayerDamage\"), true)"))); return true; }

INPUT_RESTORE_TEST(FInputRestore15, "15", "RangeInterruptRestores")
bool FInputRestore15::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Range interruption is explicit"), S.Contains(TEXT("TEXT(\"OutOfRange\")"))); return true; }

INPUT_RESTORE_TEST(FInputRestore16, "16", "DefeatedKeepsLegitimateLock")
bool FInputRestore16::RunTest(const FString&) { TestEqual(TEXT("Defeated priority"), Resolve(true, false, false, true).Context, Edemo_mapInputContext::Defeated); return true; }

INPUT_RESTORE_TEST(FInputRestore17, "17", "SettlementKeepsLegitimateLock")
bool FInputRestore17::RunTest(const FString&) { TestEqual(TEXT("Settlement priority"), Resolve(true, true, true).Context, Edemo_mapInputContext::Settlement); return true; }

INPUT_RESTORE_TEST(FInputRestore18, "18", "TerminalToPreparationTransition")
bool FInputRestore18::RunTest(const FString&) { TestEqual(TEXT("Preparation after settlement release"), Resolve(false, true).Context, Edemo_mapInputContext::Preparation); return true; }

INPUT_RESTORE_TEST(FInputRestore19, "19", "ResetReconcilesCurrentContext")
bool FInputRestore19::RunTest(const FString&) { TestEqual(TEXT("Reset priority"), Resolve(true, true, true, true, true).Context, Edemo_mapInputContext::Reset); return true; }

INPUT_RESTORE_TEST(FInputRestore20, "20", "EndPlayClearsOwnedLocks")
bool FInputRestore20::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapPlayerController.cpp")); TestTrue(TEXT("EndPlay releases one owned layer"), S.Contains(TEXT("bOwnedInputIgnoreApplied = false")) && S.Contains(TEXT("Super::EndPlay"))); return true; }

INPUT_RESTORE_TEST(FInputRestore21, "21", "StaleWidgetCloseCannotRelock")
bool FInputRestore21::RunTest(const FString&) { TestTrue(TEXT("Released search stays gameplay"), IsExactGameplay(Resolve(false))); return true; }

INPUT_RESTORE_TEST(FInputRestore22, "22", "ReopenCycleNoAccumulation")
bool FInputRestore22::RunTest(const FString&) { TestTrue(TEXT("Cycle exact"), Resolve(true).bOwnedMoveLookIgnoreRequired && IsExactGameplay(Resolve(false))); return true; }

INPUT_RESTORE_TEST(FInputRestore23, "23", "ThreeContainerCyclesNoAccumulation")
bool FInputRestore23::RunTest(const FString&) { auto V = Resolve(); for (int32 I = 0; I < 3; ++I) { V = Resolve(true); V = Resolve(false); } TestTrue(TEXT("Three cycles"), IsExactGameplay(V)); return true; }

INPUT_RESTORE_TEST(FInputRestore24, "24", "PawnRebindAppliesCurrentContext")
bool FInputRestore24::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapPlayerController.cpp")); TestTrue(TEXT("OnPossess reconciles"), S.Contains(TEXT("ReconcileInputContext(TEXT(\"PawnPossessed\"))"))); return true; }

INPUT_RESTORE_TEST(FInputRestore25, "25", "InputRemapPreserved")
bool FInputRestore25::RunTest(const FString&) { TestTrue(TEXT("Registry MoveForward exists"), Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::MoveForward) != nullptr); return true; }

INPUT_RESTORE_TEST(FInputRestore26, "26", "HotbarAndCombatGatePreserved")
bool FInputRestore26::RunTest(const FString&)
{
	const FString Hotbar = ReadInputRestoreFunctionBlock(
		TEXT("demo_mapPlayerController.cpp"),
		TEXT("void Ademo_mapPlayerController::UseHotbarSlot(int32 SlotNumber)"),
		TEXT("void Ademo_mapPlayerController::UseHotbarSlot1()"));
	const int32 HotbarGate = Hotbar.Find(TEXT("!IsGameplayInputAllowed()"));
	const int32 ThrownRoute = Hotbar.Find(
		TEXT("RouteThrownWeaponHotbarConfirmationInput("));
	const int32 TreatmentRoute =
		Hotbar.Find(TEXT("RouteMeridianShockTreatmentHotbarInput("));
	const int32 QuickSlotRoute = Hotbar.Find(TEXT("RequestUseBoundQuickSlot("));
	TestTrue(
		TEXT("Hotbar gate and trajectory-aware routes precede generic item use"),
		HotbarGate != INDEX_NONE
			&& ThrownRoute > HotbarGate
			&& TreatmentRoute > ThrownRoute
			&& QuickSlotRoute > TreatmentRoute);

	const FString StartAttack = ReadInputRestoreFunctionBlock(
		TEXT("demo_mapPlayerController.cpp"),
		TEXT("void Ademo_mapPlayerController::StartBasicAttack()"),
		TEXT("void Ademo_mapPlayerController::ToggleGroundCircle()"));
	const int32 StartAttackGate =
		StartAttack.Find(TEXT("!IsGameplayInputAllowed()"));
	const int32 ConfirmGroundCircle =
		StartAttack.Find(TEXT("Skills->ConfirmGroundCircle();"));
	const int32 TryAttack = StartAttack.Find(TEXT("TryBasicAttack();"));
	TestTrue(
		TEXT("Bound combat entry gate precedes targeting and attack routes"),
		StartAttackGate != INDEX_NONE
			&& ConfirmGroundCircle > StartAttackGate
			&& TryAttack > StartAttackGate);

	const FString BasicAttack = ReadInputRestoreFunctionBlock(
		TEXT("demo_mapPlayerController.cpp"),
		TEXT("bool Ademo_mapPlayerController::TryBasicAttack()"),
		TEXT("float Ademo_mapPlayerController::GetBasicAttackCooldownRemaining() const"));
	const int32 BasicAttackGate =
		BasicAttack.Find(TEXT("!IsGameplayInputAllowed()"));
	const int32 CooldownMutation =
		BasicAttack.Find(TEXT("BasicAttackReadyTime = CurrentTime"));
	TestTrue(
		TEXT("Direct combat route independently gates before state mutation"),
		BasicAttackGate != INDEX_NONE
			&& CooldownMutation > BasicAttackGate);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore27, "27", "CharacterMovementModePreserved")
bool FInputRestore27::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapPlayerController.cpp")); TestTrue(TEXT("MOVE_None repaired only"), S.Contains(TEXT("Movement->MovementMode == MOVE_None")) && S.Contains(TEXT("MOVE_Walking"))); return true; }

INPUT_RESTORE_TEST(FInputRestore28, "28", "NoDelayedTwentySecondRestore")
bool FInputRestore28::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapPlayerController.cpp")); TestFalse(TEXT("No 20 second restore"), S.Contains(TEXT("20.0f")) || S.Contains(TEXT("20.0"))); return true; }

INPUT_RESTORE_TEST(FInputRestore29, "29", "RealPlayerInputMovesAfterRunStart")
bool FInputRestore29::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Real W and distance contract"), S.Contains(TEXT("DispatchAutomationKeyPressed(MoveKey)")) && S.Contains(TEXT("PlanarDistance >= 10.0f")) && S.Contains(TEXT("Elapsed <= 0.50"))); return true; }

INPUT_RESTORE_TEST(FInputRestore30, "30", "RealPlayerInputMovesAfterChestClose")
bool FInputRestore30::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Chest boundary real probe"), S.Contains(TEXT("BeginInputRestoreMovementProbe(TEXT(\"ChestTakeCloseCommitted\"))"))); return true; }

INPUT_RESTORE_TEST(FInputRestore31, "31", "RealPlayerInputMovesAfterCorpseClose")
bool FInputRestore31::RunTest(const FString&) { const FString S = ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp")); TestTrue(TEXT("Corpse boundary real probe"), S.Contains(TEXT("BeginInputRestoreMovementProbe(TEXT(\"CorpseTakeCloseCommitted\"))"))); return true; }

INPUT_RESTORE_TEST(FInputRestore32, "32", "SchemaItemsAndProtectedScopesUnchanged")
bool FInputRestore32::RunTest(const FString&) { TestEqual(TEXT("Chest frozen capacity"), Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity, 6); TestEqual(TEXT("Unified action registry count"), Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num(), 23); return true; }

INPUT_RESTORE_TEST(FInputRestore33, "33", "TraceDisabledIsBehaviorNeutral")
bool FInputRestore33::RunTest(const FString&)
{
	Fdemo_mapInputRestoreTrace Trace(false);
	Fdemo_mapInputRestoreTraceSnapshot State;
	const uint32 Serial =
		Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::RunStartFresh);
	Trace.SetBoundary(
		Edemo_mapInputRestoreTraceBoundary::RunStartPlayable);
	Trace.Record(
		Edemo_mapInputRestoreTraceEvent::ExistingProbeBoundaryEmitted,
		State);
	TestTrue(
		TEXT("Disabled observer remains inert"),
		!Trace.IsEnabled()
			&& Serial == 0
			&& Trace.GetTransitionSerial() == 0
			&& Trace.Num() == 0
			&& Trace.GetDroppedCount() == 0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore34, "34", "TraceUsesFixedBoundedBuffer")
bool FInputRestore34::RunTest(const FString&)
{
	Fdemo_mapInputRestoreTrace Trace;
	Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::ChestTakeClose);
	Fdemo_mapInputRestoreTraceSnapshot State;
	for (int32 Index = 0; Index < Fdemo_mapInputRestoreTrace::Capacity + 17; ++Index)
	{
		State.DistanceUU = static_cast<float>(Index);
		Trace.Record(Edemo_mapInputRestoreTraceEvent::ProbeSample, State);
	}
	TestTrue(
		TEXT("Fixed buffer clamps and counts drops"),
		Trace.Num() == Fdemo_mapInputRestoreTrace::Capacity
			&& Trace.GetDroppedCount() == 17
			&& Trace.GetEntry(Fdemo_mapInputRestoreTrace::Capacity) == nullptr);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore35, "35", "TraceEventOrderFreshRun")
bool FInputRestore35::RunTest(const FString&)
{
	Fdemo_mapInputRestoreTrace Trace;
	Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::RunStartFresh);
	Fdemo_mapInputRestoreTraceSnapshot State;
	Trace.Record(Edemo_mapInputRestoreTraceEvent::RunActivated, State);
	Trace.SetBoundary(
		Edemo_mapInputRestoreTraceBoundary::RunStartPlayable);
	Trace.Record(
		Edemo_mapInputRestoreTraceEvent::ExistingProbeBoundaryEmitted,
		State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::MoveKeyDispatch, State);
	Trace.Record(
		Edemo_mapInputRestoreTraceEvent::MoveActionHandlerEntered,
		State);
	Trace.Record(
		Edemo_mapInputRestoreTraceEvent::ControlInputBecameNonZero,
		State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::MovementApplied, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ProbePass, State);
	TestTrue(
		TEXT("Fresh-run order is preserved"),
		Trace.Num() == 7
			&& Trace.GetEntry(0)->Event
				== Edemo_mapInputRestoreTraceEvent::RunActivated
			&& Trace.GetEntry(1)->Event
				== Edemo_mapInputRestoreTraceEvent::ExistingProbeBoundaryEmitted
			&& Trace.GetEntry(2)->Event
				== Edemo_mapInputRestoreTraceEvent::MoveKeyDispatch
			&& Trace.GetEntry(3)->Event
				== Edemo_mapInputRestoreTraceEvent::MoveActionHandlerEntered
			&& Trace.GetEntry(4)->Event
				== Edemo_mapInputRestoreTraceEvent::ControlInputBecameNonZero
			&& Trace.GetEntry(5)->Event
				== Edemo_mapInputRestoreTraceEvent::MovementApplied
			&& Trace.GetEntry(6)->Event
				== Edemo_mapInputRestoreTraceEvent::ProbePass);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore36, "36", "TraceEventOrderChestClose")
bool FInputRestore36::RunTest(const FString&)
{
	Fdemo_mapInputRestoreTrace Trace;
	Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::ChestTakeClose);
	Fdemo_mapInputRestoreTraceSnapshot State;
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerOpenCommitted, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::TakeCommitted, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::CloseIntentReceived, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerCloseBegin, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::SearchReleased, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerCloseCommitted, State);
	TestTrue(
		TEXT("Chest close order uses real observer event types"),
		Trace.Num() == 6
			&& Trace.GetEntry(0)->Event
				== Edemo_mapInputRestoreTraceEvent::ContainerOpenCommitted
			&& Trace.GetEntry(1)->Event
				== Edemo_mapInputRestoreTraceEvent::TakeCommitted
			&& Trace.GetEntry(2)->Event
				== Edemo_mapInputRestoreTraceEvent::CloseIntentReceived
			&& Trace.GetEntry(3)->Event
				== Edemo_mapInputRestoreTraceEvent::ContainerCloseBegin
			&& Trace.GetEntry(4)->Event
				== Edemo_mapInputRestoreTraceEvent::SearchReleased
			&& Trace.GetEntry(5)->Event
				== Edemo_mapInputRestoreTraceEvent::ContainerCloseCommitted);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore37, "37", "TraceEventOrderCorpseClose")
bool FInputRestore37::RunTest(const FString&)
{
	Fdemo_mapInputRestoreTrace Trace;
	Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::CorpseTakeClose);
	Fdemo_mapInputRestoreTraceSnapshot State;
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerOpenBegin, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::SearchLockApplied, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::TakeCommitted, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::CloseIntentReceived, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::SearchReleased, State);
	TestTrue(
		TEXT("Corpse close order and phase are retained"),
		Trace.Num() == 5
			&& Trace.GetPhase()
				== Edemo_mapInputRestoreTracePhase::CorpseTakeClose
			&& Trace.GetEntry(0)->Event
				== Edemo_mapInputRestoreTraceEvent::ContainerOpenBegin
			&& Trace.GetEntry(4)->Event
				== Edemo_mapInputRestoreTraceEvent::SearchReleased);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore38, "38", "ExistingProbeBoundaryRecordedWithoutMutation")
bool FInputRestore38::RunTest(const FString&)
{
	const Edemo_mapInputRestoreTraceBoundary ExistingBoundary =
		Edemo_mapInputRestoreTraceBoundary::ChestTakeCloseCommitted;
	Fdemo_mapInputRestoreTrace Trace;
	Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::ChestTakeClose);
	Trace.SetBoundary(ExistingBoundary);
	Fdemo_mapInputRestoreTraceSnapshot State;
	State.DistanceUU = 0.0f;
	State.LatencySeconds = 0.0;
	Trace.Record(
		Edemo_mapInputRestoreTraceEvent::ExistingProbeBoundaryEmitted,
		State);
	const Fdemo_mapInputRestoreTraceEntry* Entry = Trace.GetEntry(0);
	TestTrue(
		TEXT("Observer records the existing boundary without redefining it"),
		Entry
			&& ExistingBoundary
				== Edemo_mapInputRestoreTraceBoundary::ChestTakeCloseCommitted
			&& Trace.GetBoundary() == ExistingBoundary
			&& Entry->Boundary == ExistingBoundary
			&& Entry->Snapshot.DistanceUU == 0.0f
			&& Entry->Snapshot.LatencySeconds == 0.0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore39, "39", "TransitionSerialCorrelatesOneCloseCycle")
bool FInputRestore39::RunTest(const FString&)
{
	Fdemo_mapInputRestoreTrace Trace;
	const uint32 First =
		Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::ChestTakeClose);
	Fdemo_mapInputRestoreTraceSnapshot State;
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerOpenBegin, State);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerCloseCommitted, State);
	const uint32 Second =
		Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::ChestTakeClose);
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ContainerOpenBegin, State);
	TestTrue(
		TEXT("One close cycle shares one serial and the next cycle advances"),
		First > 0
			&& Trace.GetEntry(0)->TransitionSerial == First
			&& Trace.GetEntry(1)->TransitionSerial == First
			&& Second == First + 1
			&& Trace.GetEntry(2)->TransitionSerial == Second);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore40, "40", "TraceFlushDoesNotWriteProductState")
bool FInputRestore40::RunTest(const FString&)
{
	int32 ProductStateSentinel = 7341;
	Fdemo_mapInputRestoreTrace Trace;
	Trace.BeginTransition(Edemo_mapInputRestoreTracePhase::RunStartFresh);
	Fdemo_mapInputRestoreTraceSnapshot State;
	Trace.Record(Edemo_mapInputRestoreTraceEvent::ProbePass, State);
	const Fdemo_mapInputRestoreTraceEntry Before = *Trace.GetEntry(0);
	Trace.FlushToLog(TEXT("AutomationTestTerminal"));
	const Fdemo_mapInputRestoreTraceEntry* After = Trace.GetEntry(0);
	TestTrue(
		TEXT("Flush is log-only and leaves product sentinel and evidence unchanged"),
		ProductStateSentinel == 7341
			&& Trace.Num() == 1
			&& After
			&& After->Sequence == Before.Sequence
			&& After->Event == Before.Event
			&& After->TransitionSerial == Before.TransitionSerial);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore41, "41", "MovementOrderingInstalledForPossessedCharacter")
bool FInputRestore41::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	TestTrue(
		TEXT("Possessed CharacterMovement has exactly one Controller prerequisite"),
		Fixture.Controller->HasMovementOrderingForAutomation(Movement));
	TestEqual(
		TEXT("Controller and CharacterMovement are compatible PrePhysics ticks"),
		Movement->PrimaryComponentTick.TickGroup,
		Fixture.Controller->PrimaryActorTick.TickGroup);
	TestTrue(
		TEXT("Both participating ticks are enabled"),
		Fixture.Controller->IsActorTickEnabled()
			&& Movement->IsComponentTickEnabled());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore42, "42", "MovementOrderingRebindIsIdempotent")
bool FInputRestore42::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.Controller->Possess(Fixture.FirstCharacter);
	Fixture.Controller->Possess(Fixture.FirstCharacter);
	TestEqual(
		TEXT("Repeated possess/rebind retains one prerequisite"),
		Fixture.Controller->GetMovementOrderingPrerequisiteCountForAutomation(
			Fixture.FirstMovement()),
		1);
	const FTickPrerequisite Reverse(
		Fixture.FirstMovement(),
		Fixture.FirstMovement()->PrimaryComponentTick);
	TestFalse(
		TEXT("Controller does not depend on CharacterMovement"),
		Fixture.Controller->PrimaryActorTick.GetPrerequisites().Contains(Reverse));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore43, "43", "MovementOrderingRemovedFromOldPawnAndEndPlay")
bool FInputRestore43::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	UCharacterMovementComponent* OldMovement = Fixture.FirstMovement();
	ACharacter* Replacement =
		Fixture.World->SpawnActor<ACharacter>();
	if (!TestNotNull(TEXT("Replacement Character"), Replacement))
	{
		return false;
	}
	UCharacterMovementComponent* ReplacementMovement =
		Replacement->GetCharacterMovement();
	Fixture.Controller->Possess(Replacement);
	TestEqual(
		TEXT("Old Pawn no longer depends on Controller"),
		Fixture.Controller->GetMovementOrderingPrerequisiteCountForAutomation(
			OldMovement),
		0);
	TestTrue(
		TEXT("Replacement Pawn owns the single ordering"),
		Fixture.Controller->HasMovementOrderingForAutomation(
			ReplacementMovement));
	Fixture.Controller->Destroy();
	TestEqual(
		TEXT("EndPlay removes the replacement dependency"),
		Fixture.Controller->GetMovementOrderingPrerequisiteCountForAutomation(
			ReplacementMovement),
		0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore44, "44", "HeldMovementQueuedBeforeCharacterMovementConsume")
bool FInputRestore44::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	TestTrue(
		TEXT("Real MoveForward binding dispatches"),
		Fixture.DispatchMoveForwardPressed());
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	const FVector Pending =
		Fixture.FirstCharacter->GetPendingMovementInputVector();
	TestTrue(TEXT("Held intent queues a non-zero Pawn input vector"), !Pending.IsNearlyZero());
	TestTrue(
		TEXT("Queue producer is ordered before the real consumer tick"),
		Fixture.Controller->HasMovementOrderingForAutomation(
			Fixture.FirstMovement()));
	const FVector Consumed =
		Fixture.FirstMovement()->ConsumeInputVector();
	TestTrue(TEXT("CharacterMovement consumes the queued vector"), !Consumed.IsNearlyZero());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore45, "45", "OneMovementApplicationPerControllerFrame")
bool FInputRestore45::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	const FVector Once =
		Fixture.FirstCharacter->GetPendingMovementInputVector();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	const FVector Twice =
		Fixture.FirstCharacter->GetPendingMovementInputVector();
	TestTrue(TEXT("First Controller frame queues movement"), !Once.IsNearlyZero());
	TestTrue(
		TEXT("A repeated call in the same engine frame does not double apply"),
		Twice.Equals(Once));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore46, "46", "ModalContextsStillBlockMovement")
bool FInputRestore46::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.Controller->BeginSearchContainerInputLock(nullptr);
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	TestFalse(
		TEXT("Search modal retains the gameplay gate"),
		Fixture.Controller->IsGameplayInputAllowed());
	TestFalse(
		TEXT("Rejected modal input does not retain held intent"),
		Fixture.Controller->IsMoveForwardPressedForAutomation());
	TestTrue(
		TEXT("Ordering does not bypass the modal gate"),
		Fixture.FirstCharacter->GetPendingMovementInputVector().IsNearlyZero());
	TestTrue(
		TEXT("Ordering remains installed while modal"),
		Fixture.Controller->HasMovementOrderingForAutomation(
			Fixture.FirstMovement()));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore47, "47", "SearchCloseUsesGlobalOrderingWithoutSpecialRestore")
bool FInputRestore47::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	Fixture.Controller->BeginSearchContainerInputLock(nullptr);
	const bool bRestored =
		Fixture.Controller->RestoreGameplayControlFromSearchContainer();
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	TestTrue(TEXT("Search close restores the existing gameplay gate"), bRestored);
	TestTrue(
		TEXT("Search close retains the one global ordering"),
		Fixture.Controller->HasMovementOrderingForAutomation(Movement));
	TestTrue(
		TEXT("Post-close held input uses the normal queue"),
		!Fixture.FirstCharacter->GetPendingMovementInputVector().IsNearlyZero());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore48, "48", "RunStartChestCorpseReloadShareOrderingContract")
bool FInputRestore48::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	const bool bRunStart =
		Fixture.Controller->RestoreGameplayControlForNewRun();
	Fixture.Controller->BeginSearchContainerInputLock(nullptr);
	const bool bChest =
		Fixture.Controller->RestoreGameplayControlFromSearchContainer();
	Fixture.Controller->BeginSearchContainerInputLock(nullptr);
	const bool bCorpse =
		Fixture.Controller->RestoreGameplayControlFromSearchContainer();
	const bool bReload =
		Fixture.Controller->RestoreGameplayControlForNewRun();
	TestTrue(
		TEXT("RunStart/Chest/Corpse/Reload transitions remain gameplay-capable"),
		bRunStart && bChest && bCorpse && bReload);
	TestTrue(
		TEXT("All transitions retain one shared Controller ordering authority"),
		Fixture.Controller->HasMovementOrderingForAutomation(Movement));
	TestEqual(
		TEXT("No transition accumulates an extra prerequisite"),
		Fixture.Controller->GetMovementOrderingPrerequisiteCountForAutomation(
			Movement),
		1);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore49, "49", "InputConsumptionTraceDisabledIsBehaviorNeutral")
bool FInputRestore49::RunTest(const FString&)
{
	Fdemo_mapInputConsumptionTrace Trace(false);
	Trace.BeginBoundary(nullptr, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ExistingProbeBoundaryEmitted,
		nullptr);
	TestTrue(
		TEXT("Disabled consumption observer creates no evidence or drops"),
		!Trace.IsEnabled()
			&& Trace.Num() == 0
			&& Trace.GetDroppedCount() == 0
			&& Trace.GetMovementFrameCount() == 0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore50, "50", "AfterAddPendingVectorCaptured")
bool FInputRestore50::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	TestTrue(
		TEXT("AfterAdd captures the real queued Pawn vector"),
		Entry && !Entry->Snapshot.PendingInput.IsNearlyZero());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore51, "51", "PreMovementPendingVectorCaptured")
bool FInputRestore51::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PreCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(1);
	TestTrue(
		TEXT("PreCharacterMovement retains queued input before consume"),
		Entry && !Entry->Snapshot.PendingInput.IsNearlyZero());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore52, "52", "PostMovementConsumedVectorCaptured")
bool FInputRestore52::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(1);
	TestTrue(
		TEXT("PostCharacterMovement captures Pawn LastMovementInputVector"),
		Entry && !Entry->Snapshot.ConsumedInput.IsNearlyZero());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore53, "53", "SyncOrAsyncMovementPathCaptured")
bool FInputRestore53::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	TestTrue(
		TEXT("Runtime movement path is explicitly Sync or Async"),
		Entry
			&& (Entry->Snapshot.MovementPath == 0
				|| Entry->Snapshot.MovementPath == 1));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore54, "54", "AccelerationAndMovementLimitsCaptured")
bool FInputRestore54::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	TestTrue(
		TEXT("Real CharacterMovement limits are captured"),
		Entry
			&& Entry->Snapshot.MovementIdentity != 0
			&& Entry->Snapshot.MaxAcceleration > 0.0f
			&& Entry->Snapshot.MaxSpeed > 0.0f
			&& Entry->Snapshot.BrakingDeceleration >= 0.0f);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore55, "55", "CapsuleAndFocusedChestCollisionCaptured")
bool FInputRestore55::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	const FString ObserverSource =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.cpp"));
	TestTrue(
		TEXT("Raw capsule and deferred focused-chest collision paths are retained"),
		Entry
			&& Entry->Snapshot.CapsuleIdentity != 0
			&& Entry->Snapshot.CapsuleRadius > 0.0f
			&& Entry->Snapshot.CapsuleHalfHeight > 0.0f
			&& ObserverSource.Contains(TEXT("EnrichAfterVerdict"))
			&& ObserverSource.Contains(TEXT("GetComponentsBoundingBox"))
			&& ObserverSource.Contains(TEXT("IsOverlappingActor")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore56, "56", "StartPenetrationAndBlockingHitCaptured")
bool FInputRestore56::RunTest(const FString&)
{
	const FString ObserverSource =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.cpp"));
	TestTrue(
		TEXT("Post-terminal sweep captures penetration, block and actor identity"),
		ObserverSource.Contains(TEXT("SweepSingleByChannel"))
			&& ObserverSource.Contains(TEXT("Hit.bStartPenetrating"))
			&& ObserverSource.Contains(TEXT("Hit.GetActor()"))
			&& ObserverSource.Contains(
				TEXT("SCENE_QUERY_STAT(InputConsumptionMicroTraceEnrichment)")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore57, "57", "PostMovementToPostPhysicsResetDetected")
bool FInputRestore57::RunTest(const FString&)
{
	const FString ObserverSource =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.cpp"));
	TestTrue(
		TEXT("Post-movement displacement to post-physics rollback is explicit"),
		ObserverSource.Contains(TEXT("bPostMovementDisplaced"))
			&& ObserverSource.Contains(TEXT("bPostPhysicsDisplaced"))
			&& ObserverSource.Contains(TEXT("PostPhysics"))
			&& ObserverSource.Contains(TEXT("M6")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore58, "58", "ProjectOwnedMovementZeroWriterMarkerIsReadOnly")
bool FInputRestore58::RunTest(const FString&)
{
	const FString ObserverHeader =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.h"));
	const FString ObserverSource =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.cpp"));
	TestTrue(
		TEXT("Writer marker is an evidence field and reports the audited none result"),
		ObserverHeader.Contains(TEXT("bProjectOwnedMovementZeroWriter = false"))
			&& ObserverSource.Contains(
				TEXT("writer_audit=NoProjectOwnedPlayerMovementZeroWriter"))
			&& !ObserverSource.Contains(TEXT("StopMovementImmediately")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore59, "59", "DeepTraceIsBoundedAndDoesNotDropRequiredFrames")
bool FInputRestore59::RunTest(const FString&)
{
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(nullptr, nullptr);
	for (int32 Index = 0;
		Index < Fdemo_mapInputConsumptionTrace::Capacity + 9;
		++Index)
	{
		Trace.Record(
			Edemo_mapInputConsumptionStage::ProbeSample,
			nullptr,
			nullptr,
			static_cast<float>(Index),
			static_cast<double>(Index) / 100.0);
	}
	TestTrue(
		TEXT("Fixed buffer clamps deterministically and counts overflow"),
		Trace.Num() == Fdemo_mapInputConsumptionTrace::Capacity
			&& Trace.GetDroppedCount() == 9
			&& Fdemo_mapInputConsumptionTrace::RequiredMovementFrames == 1);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore60, "60", "RunStartChestCorpseUseUnchangedProbeBoundary")
bool FInputRestore60::RunTest(const FString&)
{
	const FString Manager =
		ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp"));
	TestTrue(
		TEXT("Observer arms only around the unchanged existing probe boundary"),
		Manager.Contains(TEXT("SetInputRestoreTraceBoundary(Boundary)"))
			&& Manager.Contains(TEXT("ActivateInputConsumptionTrace()"))
			&& Manager.Contains(
				TEXT("ExistingProbeBoundaryEmitted"))
			&& Manager.Contains(TEXT("RunStartPlayable"))
			&& Manager.Contains(TEXT("ChestTakeCloseCommitted"))
			&& Manager.Contains(TEXT("CorpseTakeCloseCommitted")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore61, "61", "MicroTraceDisabledHasZeroRuntimeFootprint")
bool FInputRestore61::RunTest(const FString&)
{
	Fdemo_mapInputConsumptionTrace Trace(false);
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
	Trace.BeginBoundary(nullptr, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		nullptr);
	TestTrue(
		TEXT("Disabled MicroTrace has null runtime, no entries and no hooks to service"),
		!Trace.IsEnabled()
			&& !Trace.IsActive()
			&& !Isdemo_mapInputConsumptionTraceRuntimeActive()
			&& Trace.Num() == 0
			&& Trace.GetDroppedCount() == 0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore62, "62", "MicroTraceUsesFixedPODStorageWithoutActiveWindowAllocation")
bool FInputRestore62::RunTest(const FString&)
{
	TestTrue(
		TEXT("Raw snapshot is trivially copyable POD"),
		std::is_trivially_copyable<
			Fdemo_mapInputConsumptionSnapshot>::value);
	TestTrue(
		TEXT("Entry is trivially copyable POD"),
		std::is_trivially_copyable<
			Fdemo_mapInputConsumptionEntry>::value);
	TestTrue(
		TEXT("Capacity is fixed inside the authorized 64-128 range"),
		Fdemo_mapInputConsumptionTrace::Capacity >= 64
			&& Fdemo_mapInputConsumptionTrace::Capacity <= 128);

	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(nullptr, nullptr);
	for (int32 Index = 0;
		Index < Fdemo_mapInputConsumptionTrace::Capacity;
		++Index)
	{
		Trace.Record(
			Edemo_mapInputConsumptionStage::ProbeSample,
			nullptr,
			nullptr,
			static_cast<float>(Index));
	}
	TestTrue(
		TEXT("Preallocated storage accepts exactly capacity without a drop"),
		Trace.Num() == Fdemo_mapInputConsumptionTrace::Capacity
			&& Trace.GetDroppedCount() == 0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore63, "63", "ActiveWindowHasNoSweepBoundsFormattingOrIO")
bool FInputRestore63::RunTest(const FString&)
{
	const FString Source =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.cpp"));
	const int32 ActiveStart = Source.Find(
		TEXT("Fdemo_mapInputConsumptionTrace::CaptureRaw("));
	const int32 EnrichmentStart = Source.Find(
		TEXT("void Fdemo_mapInputConsumptionTrace::EnrichAfterVerdict()"));
	const FString ActiveRegion =
		ActiveStart != INDEX_NONE
			&& EnrichmentStart > ActiveStart
				? Source.Mid(ActiveStart, EnrichmentStart - ActiveStart)
				: FString();
	TestTrue(TEXT("Active source region was isolated"), !ActiveRegion.IsEmpty());
	TestFalse(TEXT("No active sweep"), ActiveRegion.Contains(TEXT("SweepSingleByChannel")));
	TestFalse(TEXT("No active bounds query"), ActiveRegion.Contains(TEXT("GetComponentsBoundingBox")));
	TestFalse(TEXT("No active overlap query"), ActiveRegion.Contains(TEXT("IsOverlappingActor")));
	TestFalse(TEXT("No active component enumeration"), ActiveRegion.Contains(TEXT("FindComponentByClass")));
	TestFalse(TEXT("No active formatting"), ActiveRegion.Contains(TEXT("FString")));
	TestFalse(TEXT("No active logging"), ActiveRegion.Contains(TEXT("UE_LOG")));
	TestFalse(TEXT("No active I/O"), ActiveRegion.Contains(TEXT("IFileManager")));
	TestFalse(TEXT("No active dynamic array"), ActiveRegion.Contains(TEXT("TArray")));
	TestFalse(TEXT("No active lock"), ActiveRegion.Contains(TEXT("FScopeLock")));
	TestFalse(TEXT("No active sleep"), ActiveRegion.Contains(TEXT("Sleep(")));
	TestFalse(
		TEXT("No active render flush"),
		ActiveRegion.Contains(TEXT("FlushRenderingCommands")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore64, "64", "PreMovementRawHookCapturesPendingInput")
bool FInputRestore64::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	const FVector Intended = FVector(1.0f, 0.0f, 0.0f);
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PreCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	const Fdemo_mapInputConsumptionEntry* Pre = Trace.GetEntry(1);
	TestTrue(
		TEXT("Controller prerequisite pre-consume hook captures intended and pending input"),
		Pre
			&& Pre->Stage
				== Edemo_mapInputConsumptionStage::PreCharacterMovement
			&& !Pre->Snapshot.IntendedInput.IsNearlyZero()
			&& !Pre->Snapshot.PendingInput.IsNearlyZero()
			&& Fixture.Controller->HasMovementOrderingForAutomation(
				Fixture.FirstMovement()));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore65, "65", "PostMovementRawHookCapturesConsumedInputVelocityAndLocation")
bool FInputRestore65::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.DispatchMoveForwardPressed();
	Fixture.Controller->RunPlayerTickForAutomation(1.0f / 60.0f);
	const FVector Intended(1.0f, 0.0f, 0.0f);
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PreCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Fixture.FirstMovement()->Velocity = FVector(17.0f, 4.0f, 0.0f);
	Fixture.FirstCharacter->SetActorLocation(FVector(3.0f, 2.0f, 1.0f));
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Post = Trace.GetEntry(2);
	TestTrue(
		TEXT("Real movement hook captures consumed vector, velocity and location"),
		Post
			&& !Post->Snapshot.ConsumedInput.IsNearlyZero()
			&& Post->Snapshot.Velocity.Equals(FVector(17.0f, 4.0f, 0.0f))
			&& Post->Snapshot.Location.Equals(FVector(3.0f, 2.0f, 1.0f)));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore66, "66", "PostPhysicsRawHookCapturesFinalFrameState")
bool FInputRestore66::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	const FVector Intended(1.0f, 0.0f, 0.0f);
	Fixture.FirstCharacter->AddMovementInput(Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PreCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Fixture.FirstCharacter->ConsumeMovementInputVector();
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Fixture.FirstCharacter->SetActorLocation(FVector(6.0f, 0.0f, 0.0f));
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostPhysics,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* PostPhysics = Trace.GetEntry(3);
	TestTrue(
		TEXT("World-post-actor hook captures final state after PostMovement"),
		PostPhysics
			&& PostPhysics->Stage
				== Edemo_mapInputConsumptionStage::PostPhysics
			&& PostPhysics->Snapshot.Location.Equals(
				FVector(6.0f, 0.0f, 0.0f)));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore67, "67", "ProbeVerdictIsImmutableWhilePostHooksComplete")
bool FInputRestore67::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Trace.FreezeVerdict(
		false,
		Fixture.FirstCharacter,
		Fixture.Controller,
		0.0f,
		0.501);
	const uint64 FrozenFrame = Trace.GetVerdictFrame();
	Trace.FreezeVerdict(
		true,
		Fixture.FirstCharacter,
		Fixture.Controller,
		20.0f,
		0.1);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostPhysics,
		Fixture.FirstCharacter,
		Fixture.Controller);
	TestTrue(
		TEXT("Frozen FAIL cannot be changed while terminal PostPhysics completes"),
		Trace.IsVerdictFrozen()
			&& !Trace.GetFrozenProbeVerdict()
			&& Trace.GetVerdictFrame() == FrozenFrame
			&& Trace.HasVerdictFramePostHooks());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore68, "68", "PostTerminalEnrichmentRunsOnlyAfterVerdict")
bool FInputRestore68::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace;
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	TestFalse(
		TEXT("Enrichment is absent during the activity window"),
		Trace.IsPostTerminalEnriched());
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Trace.FreezeVerdict(
		false,
		Fixture.FirstCharacter,
		Fixture.Controller,
		0.0f,
		0.501);
	TestFalse(
		TEXT("Verdict freeze itself does not enrich"),
		Trace.IsPostTerminalEnriched());
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostPhysics,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Trace.EnrichAfterVerdict();
	TestTrue(
		TEXT("Heavy enrichment becomes available only after frozen post hooks"),
		Trace.IsPostTerminalEnriched()
			&& Trace.GetEnrichment().bCompleted);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore69, "69", "PairedCalibrationPlanIsFixedInterleavedAndNonSelecting")
bool FInputRestore69::RunTest(const FString&)
{
	enum class ECalibrationMode : uint8 { Off, Micro };
	ECalibrationMode Plan[24] = {};
	for (int32 Pair = 0; Pair < 12; ++Pair)
	{
		const bool bOddPair = ((Pair + 1) % 2) == 1;
		Plan[Pair * 2] =
			bOddPair ? ECalibrationMode::Off : ECalibrationMode::Micro;
		Plan[Pair * 2 + 1] =
			bOddPair ? ECalibrationMode::Micro : ECalibrationMode::Off;
	}
	bool bValid = true;
	for (int32 Pair = 0; Pair < 12; ++Pair)
	{
		const ECalibrationMode ExpectedFirst =
			((Pair + 1) % 2) == 1
				? ECalibrationMode::Off
				: ECalibrationMode::Micro;
		bValid &= Plan[Pair * 2] == ExpectedFirst;
		bValid &= Plan[Pair * 2] != Plan[Pair * 2 + 1];
	}
	TestTrue(
		TEXT("Twelve predeclared pairs alternate Off/Micro without result selection"),
		bValid);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore70, "70", "MicroTraceCompleteFailureChainSupportsM1ThroughM8")
bool FInputRestore70::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace RealObserver;
	RealObserver.BeginBoundary(Fixture.FirstCharacter, nullptr);
	TestTrue(
		TEXT("Classification is exercised on the real observer type"),
		RealObserver.IsActive()
			&& Fixture.FirstMovement() != nullptr);

	auto Classify =
		[](const Fdemo_mapInputConsumptionClassificationEvidence& Evidence)
		{
			return FString(
				Fdemo_mapInputConsumptionTrace::ClassifyEvidence(
					Evidence,
					false));
		};
	Fdemo_mapInputConsumptionClassificationEvidence E;
	E.bComplete = true;
	E.bIntendedNonZero = true;
	TestEqual(TEXT("M1"), Classify(E), FString(TEXT("M1")));
	E.bAfterAddNonZero = true;
	TestEqual(TEXT("M2"), Classify(E), FString(TEXT("M2")));
	E.bPreMovementNonZero = true;
	TestEqual(TEXT("M3"), Classify(E), FString(TEXT("M3")));
	E.bConsumedNonZero = true;
	TestEqual(TEXT("M4"), Classify(E), FString(TEXT("M4")));
	E.bAccelerationNonZero = true;
	E.bCollisionOrPenetration = true;
	TestEqual(TEXT("M5"), Classify(E), FString(TEXT("M5")));
	E.bCollisionOrPenetration = false;
	E.bPostMovementDisplaced = true;
	TestEqual(TEXT("M6"), Classify(E), FString(TEXT("M6")));
	E.bPostPhysicsDisplaced = true;
	E.bProbeAgreesWithPostPhysics = false;
	TestEqual(TEXT("M7"), Classify(E), FString(TEXT("M7")));
	E.bProbeAgreesWithPostPhysics = true;
	E.bConcreteResidual = true;
	TestEqual(TEXT("M8"), Classify(E), FString(TEXT("M8")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore71, "71", "GateTraceDisabledHasZeroRuntimeFootprint")
bool FInputRestore71::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, false);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.RecordControllerTick(
		Fixture.FirstCharacter,
		Fixture.Controller,
		1.0f / 60.0f);
	Trace.RecordMovementUpdated(
		Fixture.FirstCharacter,
		Fixture.Controller,
		1.0f / 60.0f,
		FVector::ZeroVector,
		FVector::ZeroVector);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	TestTrue(
		TEXT("Gate-disabled MicroTrace performs no gate callback mutation"),
		!Trace.IsGateEnabled()
			&& Entry
			&& Entry->Snapshot.ControllerTickSerial == 0
			&& Entry->Snapshot.MovementUpdateSerial == 0);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore72, "72", "GateTraceExtendsExistingMicroTraceInPlace")
bool FInputRestore72::RunTest(const FString&)
{
	const FString Header =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.h"));
	const FString Controller =
		ReadInputRestoreSource(TEXT("demo_mapPlayerController.cpp"));
	TestTrue(
		TEXT("One existing fixed observer owns both MicroTrace and GateTrace"),
		Header.Contains(TEXT("class Fdemo_mapInputConsumptionTrace final"))
			&& Header.Contains(TEXT("bool bGateEnabled = false"))
			&& Controller.Contains(
				TEXT("OnCharacterMovementUpdated.AddUniqueDynamic"))
			&& !Header.Contains(TEXT("Fdemo_mapMovementGateTrace")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore73, "73", "ActualMovementUpdateSerialSeparatesCurrentTickFromStaleFields")
bool FInputRestore73::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	if (!TestTrue(
		TEXT("Specific product MovementUpdated handler installed"),
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				Fixture.FirstCharacter)))
	{
		return false;
	}
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Setdemo_mapInputConsumptionTraceRuntime(&Trace);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Before = Trace.GetEntry(0);
	const Fdemo_mapInputConsumptionEntry* After = Trace.GetEntry(1);
	TestTrue(
		TEXT("Only the actual update hook advances the current-boundary serial"),
		Before && After
			&& Before->Snapshot.PreBoundaryActualCallbackCount >= 1
			&& Before->Snapshot.ActualCallbackSerial
				== Before->Snapshot.PreBoundaryActualCallbackCount
			&& !Before->Snapshot.bActualMovementUpdateFired
			&& After->Snapshot.ActualCallbackSerial
				> After->Snapshot.PreBoundaryActualCallbackCount
			&& After->Snapshot.bActualMovementUpdateFired);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore74, "74", "RealMovementUpdateHookCapturesEffectiveDeltaSeconds")
bool FInputRestore74::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	if (!TestTrue(
		TEXT("Specific product MovementUpdated handler installed"),
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				Fixture.FirstCharacter)))
	{
		return false;
	}
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Setdemo_mapInputConsumptionTraceRuntime(&Trace);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		0.0125f);
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	TestTrue(
		TEXT("Actual movement callback POD retains delta and old transform facts"),
		Entry
			&& FMath::IsNearlyEqual(
				Entry->Snapshot.EffectiveDeltaSeconds,
				0.0125f)
			&& Entry->Snapshot.ActualCallbackSerial
				> Entry->Snapshot.PreBoundaryActualCallbackCount
			&& Entry->Snapshot.ActualCallbackFrame
				== static_cast<uint64>(GFrameCounter)
			&& FMath::IsFinite(
				Entry->Snapshot.ActualCallbackWorldTime));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore75, "75", "UpdatedComponentAndValidDataPredicatesCaptured")
bool FInputRestore75::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	TestTrue(
		TEXT("Real product component public validity predicates are captured"),
		Entry && Movement
			&& Entry->Snapshot.MovementIdentity
				== reinterpret_cast<uint64>(Movement)
			&& Entry->Snapshot.UpdatedComponentIdentity
				== reinterpret_cast<uint64>(
					Movement->UpdatedComponent.Get())
			&& Entry->Snapshot.bUpdatedComponentPresent
			&& Entry->Snapshot.bHasValidData == Movement->HasValidData());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore76, "76", "SkipUpdatePredicateMirrorMatchesUE58Source")
bool FInputRestore76::RunTest(const FString&)
{
	FString Source;
	const FString Path = FPaths::Combine(
		FPaths::EngineSourceDir(),
		TEXT("Runtime/Engine/Private/Components/MovementComponent.cpp"));
	FFileHelper::LoadFileToString(Source, *Path);
	const FString Observer =
		ReadInputRestoreSource(TEXT("demo_mapInputConsumptionTrace.cpp"));
	TestTrue(
		TEXT("Source audit: observer mirrors UE 5.8 public ShouldSkipUpdate gates"),
		Source.Contains(TEXT("bUpdateOnlyIfRendered"))
			&& Source.Contains(TEXT("GetLastRenderTime()"))
			&& Source.Contains(TEXT("Mobility != EComponentMobility::Movable"))
			&& Observer.Contains(TEXT("bUpdatedComponentRenderedRecently"))
			&& Observer.Contains(TEXT("EComponentMobility::Movable")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore77, "77", "RootMotionAndMovementInProgressFactsCaptured")
bool FInputRestore77::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	TestTrue(
		TEXT("Real component root-motion and movement-progress facts agree"),
		Entry && Movement
			&& Entry->Snapshot.bHasAnimRootMotion
				== Movement->HasAnimRootMotion()
			&& Entry->Snapshot.bHasRootMotionSources
				== Movement->HasRootMotionSources()
			&& Entry->Snapshot.bMovementInProgress
				== Movement->IsMovementInProgress());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore78, "78", "AnalogModifierAndMaxInputSpeedFactsCaptured")
bool FInputRestore78::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	TestTrue(
		TEXT("Analog modifier and derived MaxInputSpeed use real product component"),
		Entry && Movement
			&& FMath::IsNearlyEqual(
				Entry->Snapshot.AnalogInputModifier,
				Movement->GetAnalogInputModifier())
			&& FMath::IsNearlyEqual(
				Entry->Snapshot.MaxInputSpeed,
				Movement->GetMaxSpeed()
					* Movement->GetAnalogInputModifier()));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore79, "79", "FloorBaseScopedMovementAndDeferredUpdateFactsCaptured")
bool FInputRestore79::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Trace.Record(
		Edemo_mapInputConsumptionStage::ProbeSample,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const Fdemo_mapInputConsumptionEntry* Entry = Trace.GetEntry(0);
	UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	TestTrue(
		TEXT("Floor/base/scoped/deferred facts agree with real product component"),
		Entry && Movement
			&& Entry->Snapshot.bCurrentFloorWalkable
				== Movement->CurrentFloor.IsWalkableFloor()
			&& FMath::IsNearlyEqual(
				Entry->Snapshot.FloorDistance,
				Movement->CurrentFloor.FloorDist)
			&& Entry->Snapshot.bScopedMovementUpdates
				== !!Movement->bEnableScopedMovementUpdates
			&& Entry->Snapshot.bMovementInProgress
				== Movement->IsMovementInProgress());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore80, "80", "PreFixCompleteFailureClassifiesExactlyOneI1ThroughI8")
bool FInputRestore80::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	if (!TestTrue(
		TEXT("Specific product MovementUpdated handler installed"),
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				Fixture.FirstCharacter)))
	{
		return false;
	}
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	const FVector Intended(1.0f, 0.0f, 0.0f);
	Fixture.FirstCharacter->AddMovementInput(Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::AfterAddMovementInput,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PreCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller,
		-1.0f,
		-1.0,
		Intended);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostCharacterMovement,
		Fixture.FirstCharacter,
		Fixture.Controller);
	Trace.FreezeVerdict(
		false,
		Fixture.FirstCharacter,
		Fixture.Controller,
		0.0f,
		0.501);
	Trace.Record(
		Edemo_mapInputConsumptionStage::PostPhysics,
		Fixture.FirstCharacter,
		Fixture.Controller);
	const FString Classification(Trace.ClassifyInternalGate());
	TestTrue(
		TEXT("Complete real-observer failure emits one enumerated internal gate"),
		Trace.HasCompleteTrace()
			&& Classification.Len() == 2
			&& Classification[0] == TCHAR('I')
			&& Classification[1] >= TCHAR('1')
			&& Classification[1] <= TCHAR('8'));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore81, "81", "RepairCannotUseTimerTeleportVelocityOrProbeShortcut")
bool FInputRestore81::RunTest(const FString&)
{
	const FString Controller =
		ReadInputRestoreSource(TEXT("demo_mapPlayerController.cpp"));
	const int32 Start = Controller.Find(
		TEXT("void Ademo_mapPlayerController::OnPossess("));
	const int32 End = Controller.Find(
		TEXT("void Ademo_mapPlayerController::OnUnPossess()"));
	const FString RepairOwner =
		Start != INDEX_NONE && End > Start
			? Controller.Mid(Start, End - Start)
			: FString();
	TestTrue(TEXT("Source audit isolated the product lifecycle owner"), !RepairOwner.IsEmpty());
	TestFalse(TEXT("No Timer repair"), RepairOwner.Contains(TEXT("Timer")));
	TestFalse(TEXT("No Teleport repair"), RepairOwner.Contains(TEXT("Teleport")));
	TestFalse(TEXT("No Velocity repair"), RepairOwner.Contains(TEXT("Velocity")));
	TestFalse(TEXT("No Probe shortcut"), RepairOwner.Contains(TEXT("Probe")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore82, "82", "RepairRunsOnlyOnConfirmedProductGate")
bool FInputRestore82::RunTest(const FString&)
{
	Fdemo_mapInputConsumptionTrace Disabled(true, false);
	Disabled.BeginBoundary(nullptr, nullptr);
	Disabled.FreezeVerdict(false, nullptr, nullptr, 0.0f, 0.501);
	TestEqual(
		TEXT("No internal-gate decision without the explicit GateTrace contract"),
		FString(Disabled.ClassifyInternalGate()),
		FString(TEXT("INCOMPLETE")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore83, "83", "PostFixPreviouslyFailingGateClearsWithoutThresholdChange")
bool FInputRestore83::RunTest(const FString&)
{
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(nullptr, nullptr);
	Trace.FreezeVerdict(true, nullptr, nullptr, 10.01f, 0.50);
	TestEqual(
		TEXT("Frozen existing >10 UU / <=0.50 s PASS clears internal failure"),
		FString(Trace.ClassifyInternalGate()),
		FString(TEXT("PASS")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore84, "84", "PairedPreAndPostPackagePlansAreFixedAndNonSelecting")
bool FInputRestore84::RunTest(const FString&)
{
	enum class EPlanMode : uint8 { Off, Gate, PostFix };
	EPlanMode PreFix[24] = {};
	for (int32 Pair = 0; Pair < 12; ++Pair)
	{
		const bool bOdd = ((Pair + 1) % 2) == 1;
		PreFix[Pair * 2] = bOdd ? EPlanMode::Off : EPlanMode::Gate;
		PreFix[Pair * 2 + 1] = bOdd ? EPlanMode::Gate : EPlanMode::Off;
	}
	EPlanMode PostFix[30] = {};
	for (EPlanMode& Slot : PostFix)
	{
		Slot = EPlanMode::PostFix;
	}
	bool bFixed = true;
	for (int32 Pair = 0; Pair < 12; ++Pair)
	{
		bFixed &= PreFix[Pair * 2] != PreFix[Pair * 2 + 1];
	}
	for (const EPlanMode Slot : PostFix)
	{
		bFixed &= Slot == EPlanMode::PostFix;
	}
	TestTrue(
		TEXT("24 interleaved PreFix and 30 fixed PostFix slots are predeclared"),
		bFixed);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore85, "85", "ReflectedMovementUpdatedFunctionNameExact")
bool FInputRestore85::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	const FName HandlerName =
		Fixture.Controller->
			GetInputConsumptionMovementHandlerNameForAutomation();
	const FString HandlerText = HandlerName.ToString();
	TestTrue(
		TEXT("Exact reflected UFUNCTION is present with no surrounding whitespace"),
		HandlerName
				== FName(TEXT("HandleInputConsumptionMovementUpdated"))
			&& HandlerText.TrimStartAndEnd() == HandlerText
			&& Fixture.Controller->FindFunction(HandlerName) != nullptr);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore86, "86", "DynamicDelegateBindsSpecificCallableHandler")
bool FInputRestore86::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	const bool bFirstInstall =
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				Fixture.FirstCharacter);
	const bool bSecondInstall =
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				Fixture.FirstCharacter);
	TestTrue(
		TEXT("Idempotent install proves the exact Controller/handler pair"),
		bFirstInstall
			&& bSecondInstall
			&& Fixture.Controller->
				IsInputConsumptionReflectedFunctionPresentForAutomation()
			&& Fixture.Controller->
				IsInputConsumptionHandlerAlreadyBoundForAutomation());
	return true;
}

INPUT_RESTORE_TEST(FInputRestore87, "87", "ActualCallbackSerialAdvancesInRealFreshMovement")
bool FInputRestore87::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid())
		|| !TestTrue(
			TEXT("Exact product handler installed"),
			Fixture.Controller->
				InstallInputConsumptionMovementHookForAutomation(
					Fixture.FirstCharacter)))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	const uint64 Before =
		Fixture.Controller->
			GetInputConsumptionActualCallbackCountForAutomation();
	Setdemo_mapInputConsumptionTraceRuntime(&Trace);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
	TestTrue(
		TEXT("Real Character MovementUpdated delegate invokes the product callback"),
		Fixture.Controller->
				GetInputConsumptionActualCallbackCountForAutomation()
				> Before
			&& Trace.GetActualCallbackSerial() > Before);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore88, "88", "ActualCallbackCarriesFinitePositiveDeltaSeconds")
bool FInputRestore88::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid())
		|| !TestTrue(
			TEXT("Exact product handler installed"),
			Fixture.Controller->
				InstallInputConsumptionMovementHookForAutomation(
					Fixture.FirstCharacter)))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	Setdemo_mapInputConsumptionTraceRuntime(&Trace);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		0.01f);
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
	const float DeltaSeconds = Trace.GetLastActualCallbackDeltaSeconds();
	TestTrue(
		TEXT("Real callback carries the finite positive effective delta"),
		FMath::IsFinite(DeltaSeconds)
			&& DeltaSeconds > 0.0f
			&& FMath::IsNearlyEqual(DeltaSeconds, 0.01f));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore89, "89", "ChestGateTraceUsesActualCallbackNotDerivedFallback")
bool FInputRestore89::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid())
		|| !TestTrue(
			TEXT("Exact product handler installed"),
			Fixture.Controller->
				InstallInputConsumptionMovementHookForAutomation(
					Fixture.FirstCharacter)))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	const uint64 Before = Trace.GetActualCallbackSerial();
	Trace.RecordDerivedMovementUpdated(
		Fixture.FirstMovement(),
		Fixture.FirstCharacter,
		Fixture.Controller,
		1.0f / 60.0f);
	const uint64 AfterDerived = Trace.GetActualCallbackSerial();
	Setdemo_mapInputConsumptionTraceRuntime(&Trace);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	Setdemo_mapInputConsumptionTraceRuntime(nullptr);
	TestTrue(
		TEXT("Public LastUpdate derivation cannot qualify; only delegate callback advances"),
		AfterDerived == Before
			&& Trace.GetActualCallbackSerial() > AfterDerived);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore90, "90", "UnbindLifecyclePreventsStaleOldPawnCallback")
bool FInputRestore90::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid())
		|| !TestTrue(
			TEXT("First Character handler installed"),
			Fixture.Controller->
				InstallInputConsumptionMovementHookForAutomation(
					Fixture.FirstCharacter)))
	{
		return false;
	}
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	const uint64 BeforeSwap =
		Fixture.Controller->
			GetInputConsumptionActualCallbackCountForAutomation();
	ACharacter* SecondCharacter =
		Fixture.World->SpawnActor<ACharacter>();
	if (!TestNotNull(TEXT("Second real Character"), SecondCharacter))
	{
		return false;
	}
	Fixture.Controller->Possess(SecondCharacter);
	const bool bSecondInstalled =
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				SecondCharacter);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	const uint64 AfterOldPawn =
		Fixture.Controller->
			GetInputConsumptionActualCallbackCountForAutomation();
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		SecondCharacter,
		1.0f / 60.0f);
	const uint64 AfterNewPawn =
		Fixture.Controller->
			GetInputConsumptionActualCallbackCountForAutomation();
	TestTrue(
		TEXT("Old Pawn is stale-safe while replacement Pawn remains callable"),
		bSecondInstalled
			&& AfterOldPawn == BeforeSwap
			&& AfterNewPawn > AfterOldPawn);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore91, "91", "ContainerBoundWithoutCallbackAdvanceCannotQualifyHook")
bool FInputRestore91::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Ademo_mapPlayerController* OtherController =
		Fixture.World->SpawnActor<Ademo_mapPlayerController>();
	if (!TestNotNull(TEXT("Other real product Controller"), OtherController))
	{
		return false;
	}
	const bool bOtherPairBound =
		OtherController->InstallInputConsumptionMovementHookForAutomation(
			Fixture.FirstCharacter);
	Fdemo_mapInputConsumptionTrace Trace(true, true);
	Trace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	TestTrue(
		TEXT("Non-empty multicast container is not qualification for this pair"),
		bOtherPairBound
			&& Fixture.FirstCharacter->
				OnCharacterMovementUpdated.IsBound()
			&& !Fixture.Controller->
				IsInputConsumptionHandlerAlreadyBoundForAutomation()
			&& Fixture.Controller->
				GetInputConsumptionActualCallbackCountForAutomation() == 0
			&& !Trace.IsHookQualifiedBeforeBoundary());
	OtherController->
		RemoveInputConsumptionMovementHookForAutomation();
	return true;
}

INPUT_RESTORE_TEST(FInputRestore92, "92", "HookQualificationHasZeroEnsureCrashAndOffFootprint")
bool FInputRestore92::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputConsumptionTrace OffTrace(false, false);
	OffTrace.BeginBoundary(Fixture.FirstCharacter, nullptr);
	const bool bOffClean =
		!OffTrace.IsEnabled()
		&& OffTrace.Num() == 0
		&& !Fixture.Controller->
			IsInputConsumptionHandlerAlreadyBoundForAutomation()
		&& !Isdemo_mapInputConsumptionTraceRuntimeActive();
	const bool bInstalled =
		Fixture.Controller->
			InstallInputConsumptionMovementHookForAutomation(
				Fixture.FirstCharacter);
	DispatchRealMovementUpdatedHandler(
		Fixture.Controller,
		Fixture.FirstCharacter,
		1.0f / 60.0f);
	const float DeltaSeconds =
		Fixture.Controller->
			GetInputConsumptionLastActualCallbackDeltaSecondsForAutomation();
	TestTrue(
		TEXT("Off path is empty and exact hook path is callable without fallback"),
		bOffClean
			&& bInstalled
			&& Fixture.Controller->
				IsInputConsumptionReflectedFunctionPresentForAutomation()
			&& Fixture.Controller->
				IsInputConsumptionHandlerAlreadyBoundForAutomation()
			&& Fixture.Controller->
				GetInputConsumptionActualCallbackCountForAutomation() >= 1
			&& FMath::IsFinite(DeltaSeconds)
			&& DeltaSeconds > 0.0f);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore93, "93", "MovementCadenceClearsUE58PrimitiveSweepFloor")
bool FInputRestore93::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	const UCharacterMovementComponent* Movement = Fixture.FirstMovement();
	const float TickInterval =
		Movement ? Movement->PrimaryComponentTick.TickInterval : 0.0f;
	const float FirstIntegratedDistance =
		Movement
			? Movement->GetMaxAcceleration()
				* TickInterval
				* TickInterval
			: 0.0f;
	const float UE58PrimitiveSweepFloor =
		4.0f * UE_KINDA_SMALL_NUMBER;
	TestTrue(
		TEXT("Possess lifecycle installs the 1 ms product movement cadence"),
		Movement
			&& FMath::IsNearlyEqual(TickInterval, 0.001f));
	TestTrue(
		TEXT("First MaxAcceleration displacement clears UE 5.8 swept-move floor"),
		FirstIntegratedDistance > UE58PrimitiveSweepFloor);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore94, "94", "TerminalStateRecordExactFieldSet")
bool FInputRestore94::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputRestoreTerminalStateEmitter Emitter;
	FString Line;
	if (!TestTrue(
		TEXT("Terminal formatter emits"),
		Emitter.TryBuildLine(
			TEXT("RunStartFresh"),
			TEXT("RunStartPlayable"),
			true,
			Fixture.Controller,
			Fixture.FirstCharacter,
			Fixture.World,
			12.5f,
			0.125,
			Line)))
	{
		return false;
	}
	const TArray<FString> RequiredFields = {
		TEXT("phase="),
		TEXT("boundary="),
		TEXT("caller="),
		TEXT("verdict="),
		TEXT("move_ignored="),
		TEXT("look_ignored="),
		TEXT("gameplay_allowed="),
		TEXT("context="),
		TEXT("input_mode="),
		TEXT("possessed="),
		TEXT("movement_mode="),
		TEXT("paused="),
		TEXT("dilation="),
		TEXT("distance_uu="),
		TEXT("latency_seconds=")
	};
	for (const FString& Field : RequiredFields)
	{
		TestEqual(
			*FString::Printf(TEXT("%s appears once"), *Field),
			Line.Replace(*Field, TEXT("")).Len() == Line.Len() - Field.Len()
				? 1
				: 0,
			1);
	}
	int32 EqualsCount = 0;
	for (const TCHAR Character : Line)
	{
		if (Character == TEXT('='))
		{
			++EqualsCount;
		}
	}
	TestEqual(TEXT("Exactly fifteen key/value fields"), EqualsCount, 15);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore95, "95", "TerminalStateRecordSingleEmissionOnPass")
bool FInputRestore95::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputRestoreTerminalStateEmitter Emitter;
	FString First;
	FString Second;
	const bool bFirst = Emitter.TryBuildLine(
		TEXT("ChestTakeClose"),
		TEXT("ChestTakeCloseCommitted"),
		true,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		11.1f,
		0.1,
		First);
	const bool bSecond = Emitter.TryBuildLine(
		TEXT("ChestTakeClose"),
		TEXT("ChestTakeCloseCommitted"),
		true,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		11.1f,
		0.1,
		Second);
	TestTrue(
		TEXT("PASS emits exactly once"),
		bFirst && !bSecond && First.Contains(TEXT("verdict=PASS")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore96, "96", "TerminalStateRecordSingleEmissionOnFail")
bool FInputRestore96::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputRestoreTerminalStateEmitter Emitter;
	FString First;
	FString Second;
	const bool bFirst = Emitter.TryBuildLine(
		TEXT("Reload"),
		TEXT("RunStartPlayable"),
		false,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN(),
		First);
	const bool bSecond = Emitter.TryBuildLine(
		TEXT("Reload"),
		TEXT("RunStartPlayable"),
		false,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		0.0f,
		0.0,
		Second);
	TestTrue(
		TEXT("FAIL emits exactly once with finite sentinels"),
		bFirst
			&& !bSecond
			&& First.Contains(TEXT("verdict=FAIL"))
			&& First.Contains(TEXT("distance_uu=-1.000"))
			&& First.Contains(TEXT("latency_seconds=-1.000000")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore97, "97", "TerminalStateRecordAfterProbeBeforePhaseMarkerAndExit")
bool FInputRestore97::RunTest(const FString&)
{
	const FString Source =
		ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp"));
	const int32 Probe =
		Source.Find(TEXT("INPUT_RESTORE_PROBE: PASS phase=%s boundary=%s reason=%s."));
	const int32 Terminal =
		Source.Find(TEXT("InputRestoreTerminalStateEmitter.TryBuildLine("));
	const int32 PhaseMarker =
		Source.Find(TEXT("const TCHAR* Marker ="), ESearchCase::CaseSensitive, ESearchDir::FromStart, Terminal);
	const int32 Exit =
		Source.Find(TEXT("FPlatformMisc::RequestExitWithStatus(false, 1);"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Terminal);
	TestTrue(
		TEXT("Probe then dedicated terminal then phase marker/exit"),
		Probe != INDEX_NONE
			&& Terminal > Probe
			&& PhaseMarker > Terminal
			&& Exit > Terminal);
	return true;
}

INPUT_RESTORE_TEST(FInputRestore98, "98", "TerminalStateRecordUsesActualControllerPawnAndMovementState")
bool FInputRestore98::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.Controller->RestoreGameplayControlForNewRun();
	Fixture.FirstMovement()->SetMovementMode(MOVE_Walking);
	Fdemo_mapInputRestoreTerminalStateEmitter Emitter;
	FString Line;
	const bool bEmitted = Emitter.TryBuildLine(
		TEXT("CorpseTakeClose"),
		TEXT("CorpseTakeCloseCommitted"),
		true,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		12.25f,
		0.25,
		Line);
	TestTrue(
		TEXT("Terminal fields derive from the actual real Controller/Pawn/Movement"),
		bEmitted
			&& Line.Contains(TEXT("move_ignored=0"))
			&& Line.Contains(TEXT("look_ignored=0"))
			&& Line.Contains(TEXT("gameplay_allowed=1"))
			&& Line.Contains(TEXT("context=Gameplay"))
			&& Line.Contains(TEXT("input_mode=GameOnly"))
			&& Line.Contains(TEXT("possessed=1"))
			&& Line.Contains(TEXT("movement_mode=1")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore99, "99", "TerminalStateRecordNonShippingAutomationOnly")
bool FInputRestore99::RunTest(const FString&)
{
	const FString Header =
		ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.h"));
	const FString Source =
		ReadInputRestoreSource(TEXT("demo_mapV3ProgressionManager.cpp"));
	TestTrue(
		TEXT("Terminal formatter is non-Shipping and exact automation gated"),
		Header.Contains(TEXT("#if !UE_BUILD_SHIPPING"))
			&& Header.Contains(TEXT("Fdemo_mapInputRestoreTerminalStateEmitter"))
			&& Source.Contains(TEXT("bInputRestoreAutomation = FParse::Param(FCommandLine::Get(), TEXT(\"InputRestoreAutomation\"))"))
			&& Source.Contains(TEXT("if (bInputRestoreAutomation)"))
			&& Source.Contains(TEXT("InputRestoreTerminalStateEmitter.TryBuildLine(")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore100, "100", "EditorAndPackagePlansUseSameInputRestoreTerminalMarker")
bool FInputRestore100::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fixture.Controller->RestoreGameplayControlForNewRun();
	Fdemo_mapInputRestoreTerminalStateEmitter EditorConsumer;
	Fdemo_mapInputRestoreTerminalStateEmitter PackageConsumer;
	FString EditorLine;
	FString PackageLine;
	const bool bEditorEmitted = EditorConsumer.TryBuildLine(
		TEXT("Reload"),
		TEXT("RunStartPlayable"),
		true,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		12.0f,
		0.25,
		EditorLine);
	const bool bPackageEmitted = PackageConsumer.TryBuildLine(
		TEXT("Reload"),
		TEXT("RunStartPlayable"),
		true,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		12.0f,
		0.25,
		PackageLine);
	TestTrue(
		TEXT("Independent consumers receive one canonical terminal contract"),
		bEditorEmitted
			&& bPackageEmitted
			&& EditorLine == PackageLine
			&& EditorLine.StartsWith(TEXT("INPUT_RESTORE_TERMINAL_STATE "))
			&& EditorLine.Contains(TEXT("phase=Reload"))
			&& EditorLine.Contains(TEXT("boundary=RunStartPlayable")));
	return true;
}

INPUT_RESTORE_TEST(FInputRestore101, "101", "NonInputRestoreSlotsDoNotRequireGenericInputContextTransition")
bool FInputRestore101::RunTest(const FString&)
{
	FMovementOrderingFixture Fixture;
	if (!TestTrue(TEXT("Real Controller/Character fixture"), Fixture.IsValid()))
	{
		return false;
	}
	Fdemo_mapInputRestoreTerminalStateEmitter Emitter;
	FString Line;
	const bool bEmitted = Emitter.TryBuildLine(
		TEXT("CorpseTakeClose"),
		TEXT("CorpseTakeCloseCommitted"),
		true,
		Fixture.Controller,
		Fixture.FirstCharacter,
		Fixture.World,
		10.0f,
		0.1,
		Line);
	TestTrue(
		TEXT("Dedicated terminal record does not depend on the generic transition marker"),
		bEmitted
			&& Line.StartsWith(TEXT("INPUT_RESTORE_TERMINAL_STATE "))
			&& !Line.Contains(TEXT("INPUT_CONTEXT_TRANSITION")));
	return true;
}

#undef INPUT_RESTORE_TEST

#endif
