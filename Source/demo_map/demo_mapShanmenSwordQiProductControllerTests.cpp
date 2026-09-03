#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiProductController.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordQiProjectile.h"

namespace
{
	constexpr EAutomationTestFlags SwordQiControllerFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordQiControllerRunA(0xD3840001, 0, 0, 1);
	const FGuid SwordQiControllerRunB(0xD3840002, 0, 0, 1);
	const FGuid SwordQiIntentA(0xD3840003, 0, 0, 1);
	const FGuid SwordQiIntentB(0xD3840004, 0, 0, 1);
	const FGuid SwordQiOtherOwner(0xD3840005, 0, 0, 1);

	struct FSwordQiControllerFixture
	{
		UWorld* World = nullptr;
		APawn* Pawn = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Fdemo_mapItemAuthority Items;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenSwordQiProductController Controller;
		FString Diagnostic;
		int32 AuthorizationCount = 0;
		bool bReady = false;

		FSwordQiControllerFixture()
		{
			if (!GEngine)
			{
				return;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
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
			Pawn = World->SpawnActor<APawn>();
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P184SwordQiHealth"))
				: nullptr;
			Attributes = Pawn
				? NewObject<Udemo_mapAttributeComponent>(
					Pawn, TEXT("P184SwordQiAttributes"))
				: nullptr;
			bReady = Pawn && Health && Attributes
				&& Attributes->SetBaseValue(
					Fdemo_mapAttributeIds::Primary01, 6.0f)
				&& Coordinator.TryBeginRun(
					SwordQiControllerRunA,
					Pawn,
					Health,
					Diagnostic)
				&& Controller.TryBegin(
					SwordQiControllerRunA,
					Diagnostic);
		}

		~FSwordQiControllerFixture()
		{
			Controller.Reset();
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		FGuid Equip(
			FAutomationTestBase& Test,
			FName DefinitionId)
		{
			TArray<FGuid> AddedIds;
			const Fdemo_mapItemOperationResult Added =
				Items.AddDefinition(DefinitionId, 1, &AddedIds);
			const bool bAdded = Added.bSuccess && AddedIds.Num() == 1;
			Test.TestTrue(TEXT("fixture sword added"), bAdded);
			const bool bEquipped = bAdded
				&& Items.Equip(
					AddedIds[0],
					Fdemo_mapItemIds::WeaponSlot).bSuccess;
			Test.TestTrue(TEXT("fixture sword equipped"), bEquipped);
			return AddedIds.Num() == 1 ? AddedIds[0] : FGuid();
		}

		Fdemo_mapShanmenSwordQiIntent Intent(
			const FGuid& IntentId,
			const FGuid& RunId = SwordQiControllerRunA,
			const FVector& Origin = FVector(20.0, 30.0, 60.0))
		{
			Fdemo_mapShanmenSwordQiIntent Result;
			check(Fdemo_mapShanmenSwordQiIntent::TryCapture(
				IntentId,
				RunId,
				Origin,
				FVector::ForwardVector,
				Result));
			return Result;
		}

		Fdemo_mapShanmenPlayerActionGateResult Authorize(
			const Fdemo_mapShanmenPlayerActionOccupancySnapshot& Occupancy =
				Fdemo_mapShanmenPlayerActionOccupancySnapshot())
		{
			++AuthorizationCount;
			return Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
				Coordinator.TryAuthorizePlayerAction(
					Edemo_mapShanmenPlayerActionKind::SwordQi,
					Occupancy));
		}

		Fdemo_mapShanmenSwordQiControllerResult Submit(
			const Fdemo_mapShanmenSwordQiIntent& RequestedIntent)
		{
			return Controller.TrySubmit(
				World,
				Ademo_mapShanmenSwordQiProjectile::StaticClass(),
				Items,
				*Attributes,
				Coordinator,
				Pawn,
				RequestedIntent,
				[this]() { return Authorize(); });
		}
	};

	Fdemo_mapShanmenPlayerActionOccupancySnapshot BlockingOccupancy()
	{
		Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
		Occupancy.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			SwordQiOtherOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::None);
		return Occupancy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiControllerRouteAndEndTest,
	"Shanmen.0_0_10.Product.SwordQiProductController.RouteAndRunEnd",
	SwordQiControllerFlags)

bool Fdemo_mapSwordQiControllerRouteAndEndTest::RunTest(const FString&)
{
	FSwordQiControllerFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.4 controller fixture."));
		return false;
	}
	const FGuid SwordId =
		Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapShanmenSwordQiControllerResult Applied =
		Fixture.Submit(Fixture.Intent(SwordQiIntentA));
	TestTrue(TEXT("Run owner resolves equipment and launches one command"),
		SwordId.IsValid()
			&& Applied.IsAccepted()
			&& !Applied.bReusedIntent
			&& Applied.Item.Authorization.GetSourceItemInstanceId()
				== SwordId
			&& Applied.AttackPower == 7.0f
			&& Fixture.AuthorizationCount == 1
			&& Fixture.Controller.NumCapturedIntents() == 1
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 2);

	Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
	TestTrue(TEXT("controller projects active Sword Qi occupancy"),
		Fixture.Controller.TryAppendOccupancy(Occupancy)
			&& Occupancy.NumOccupiedProducts() == 1
			&& Occupancy.GetSoleClaim()
			&& Occupancy.GetSoleClaim()->OwningAction
				== Edemo_mapShanmenPlayerActionKind::SwordQi);

	Fdemo_mapShanmenSwordQiControllerEndSummary Summary;
	TestTrue(TEXT("Run teardown interrupts, proves and clears active flight"),
		Fixture.Controller.TryEnd(
			SwordQiControllerRunA,
			Summary,
			Fixture.Diagnostic)
			&& Summary.IsValid()
			&& Summary.CapturedIntentCount == 1
			&& Summary.ProcessedCommandCount == 1
			&& Summary.bInterruptedFlight
			&& Summary.TerminalReceipt.IsValid()
			&& Summary.TerminalReceipt.Kind
				== Edemo_mapShanmenSwordQiTerminalKind::Interrupted
			&& Fixture.Controller.IsEmpty()
			&& Fixture.Controller.IsValid());
	TestTrue(TEXT("controller releases before the shared coordinator"),
		Fixture.Coordinator.TryEndRun(
			SwordQiControllerRunA,
			Fixture.Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiControllerFrozenReplayTest,
	"Shanmen.0_0_10.Product.SwordQiProductController.FrozenReplay",
	SwordQiControllerFlags)

bool Fdemo_mapSwordQiControllerFrozenReplayTest::RunTest(const FString&)
{
	FSwordQiControllerFixture Fixture;
	const FGuid FirstSword =
		Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapShanmenSwordQiIntent Intent =
		Fixture.Intent(SwordQiIntentA);
	const Fdemo_mapShanmenSwordQiControllerResult First =
		Fixture.Submit(Intent);
	if (!Fixture.bReady || !First.IsAccepted())
	{
		AddError(TEXT("Could not launch the P18.4 replay fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiLaunchCommand* Frozen =
		Fixture.Controller.FindCapturedCommand(SwordQiIntentA);
	const FGuid FrozenCommandId =
		Frozen ? Frozen->GetCommandId() : FGuid();
	TestTrue(TEXT("post-capture attribute and equipment changes succeed"),
		Fixture.Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01, 98.0f)
			&& Fixture.Equip(*this, Fdemo_mapItemIds::WeaponLevel1)
				.IsValid());
	const Fdemo_mapShanmenSwordQiControllerResult Replay =
		Fixture.Submit(Intent);
	TestTrue(TEXT("exact intent replay never re-samples or re-reserves"),
		Replay.IsAccepted()
			&& Replay.bReusedIntent
			&& Replay.Route.IsReplay()
			&& Replay.AttackPower == 7.0f
			&& Replay.Item.Authorization.GetSourceItemInstanceId()
				== FirstSword
			&& Replay.Start.Command.GetCommandId() == FrozenCommandId
			&& Fixture.AuthorizationCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiControllerBusyRetryTest,
	"Shanmen.0_0_10.Product.SwordQiProductController.BusyRetry",
	SwordQiControllerFlags)

bool Fdemo_mapSwordQiControllerBusyRetryTest::RunTest(const FString&)
{
	FSwordQiControllerFixture Fixture;
	Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapShanmenSwordQiControllerResult First =
		Fixture.Submit(Fixture.Intent(SwordQiIntentA));
	const Fdemo_mapShanmenSwordQiIntent SecondIntent =
		Fixture.Intent(
			SwordQiIntentB,
			SwordQiControllerRunA,
			FVector(40.0, 30.0, 60.0));
	const Fdemo_mapShanmenSwordQiControllerResult Busy =
		Fixture.Submit(SecondIntent);
	TestTrue(TEXT("busy Host retains a frozen second intent"),
		Fixture.bReady
			&& First.IsAccepted()
			&& Busy.Status
				== Edemo_mapShanmenSwordQiControllerStatus::RouteRejected
			&& Busy.Route.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy
			&& Busy.Start.IsReady()
			&& Fixture.Controller.NumCapturedIntents() == 2
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 3);
	Fdemo_mapShanmenSwordQiTerminalReceipt FirstTerminal;
	TestTrue(TEXT("first flight can make room for the captured retry"),
		Fixture.Controller.TryInterrupt()
			&& Fixture.Controller.TryRetireTerminal(FirstTerminal)
			&& FirstTerminal.IsValid());
	TestTrue(TEXT("later authority changes cannot rewrite captured request"),
		Fixture.Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01, 12.0f)
			&& Fixture.Equip(*this, Fdemo_mapItemIds::WeaponLevel2)
				.IsValid());
	const Fdemo_mapShanmenSwordQiControllerResult Retried =
		Fixture.Submit(SecondIntent);
	TestTrue(TEXT("transient busy retry launches the same reserved command"),
		Retried.IsAccepted()
			&& Retried.bReusedIntent
			&& !Retried.Route.IsReplay()
			&& Retried.AttackPower == 7.0f
			&& Retried.Start.Command.GetCommandId()
				== Busy.Start.Command.GetCommandId()
			&& Fixture.AuthorizationCount == 2
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiControllerFencesTest,
	"Shanmen.0_0_10.Product.SwordQiProductController.Fences",
	SwordQiControllerFlags)

bool Fdemo_mapSwordQiControllerFencesTest::RunTest(const FString&)
{
	FSwordQiControllerFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.4 fence fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiIntent Valid =
		Fixture.Intent(SwordQiIntentA);
	const Fdemo_mapShanmenSwordQiControllerResult NoEquipment =
		Fixture.Submit(Valid);
	TestTrue(TEXT("missing equipped sword fails before sequence reservation"),
		NoEquipment.Status
			== Edemo_mapShanmenSwordQiControllerStatus::
				ItemAuthorizationRejected
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 1);
	Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapShanmenSwordQiControllerResult WrongRun =
		Fixture.Submit(Fixture.Intent(SwordQiIntentA, SwordQiControllerRunB));
	TestTrue(TEXT("foreign Run fails before sequence reservation"),
		WrongRun.Status
			== Edemo_mapShanmenSwordQiControllerStatus::RunMismatch
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 1);

	const Fdemo_mapShanmenPlayerActionOccupancySnapshot Blocked =
		BlockingOccupancy();
	const Fdemo_mapShanmenSwordQiControllerResult GateRejected =
		Fixture.Controller.TrySubmit(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Items,
			*Fixture.Attributes,
			Fixture.Coordinator,
			Fixture.Pawn,
			Valid,
			[&Fixture, &Blocked]()
			{
				return Fixture.Authorize(Blocked);
			});
	TestTrue(TEXT("persistent occupied lane records one durable rejection"),
		GateRejected.Status
			== Edemo_mapShanmenSwordQiControllerStatus::RouteRejected
			&& GateRejected.Route.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::ActionGateRejected
			&& GateRejected.Route.IsTerminal()
			&& Fixture.AuthorizationCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 2);

	const Fdemo_mapShanmenSwordQiControllerResult Conflict =
		Fixture.Submit(Fixture.Intent(
			SwordQiIntentA,
			SwordQiControllerRunA,
			FVector(21.0, 30.0, 60.0)));
	TestTrue(TEXT("one IntentId cannot alias another trajectory"),
		Conflict.Status
			== Edemo_mapShanmenSwordQiControllerStatus::IntentIdConflict
			&& Fixture.AuthorizationCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 2);

	Fdemo_mapShanmenSwordQiControllerEndSummary Summary;
	TestTrue(TEXT("rejected-only Run controller still closes cleanly"),
		Fixture.Controller.TryEnd(
			SwordQiControllerRunA,
			Summary,
			Fixture.Diagnostic)
			&& Summary.IsValid()
			&& Summary.CapturedIntentCount == 1
			&& Summary.ProcessedCommandCount == 1
			&& !Summary.bInterruptedFlight
			&& !Summary.TerminalReceipt.IsValid());
	return true;
}

#endif
