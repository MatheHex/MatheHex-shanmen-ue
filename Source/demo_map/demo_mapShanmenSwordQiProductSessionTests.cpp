#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiProductSession.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	constexpr EAutomationTestFlags SwordQiProductFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordQiProductRunA(0xD3830001, 0, 0, 1);
	const FGuid SwordQiProductRunB(0xD3830002, 0, 0, 1);
	const FGuid SwordQiProductItem(0xD3830003, 0, 0, 1);
	const FGuid SwordQiProductOtherOwner(0xD3830004, 0, 0, 1);

	struct FSwordQiProductFixture
	{
		UWorld* World = nullptr;
		APawn* Pawn = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FSwordQiProductFixture()
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
					Pawn, TEXT("P183SwordQiHealth"))
				: nullptr;
			bReady = Pawn && Health
				&& Coordinator.TryBeginRun(
					SwordQiProductRunA, Pawn, Health, Diagnostic);
		}

		~FSwordQiProductFixture()
		{
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		Fdemo_mapShanmenSwordQiProductStartResult Prepare(
			const FVector& Origin = FVector(20.0, 30.0, 60.0),
			const FVector& Direction = FVector::ForwardVector)
		{
			return Fdemo_mapShanmenSwordQiProductAuthority::PrepareLaunch(
				Coordinator,
				SwordQiProductItem,
				20.0f,
				Origin,
				Direction);
		}

		Fdemo_mapShanmenPlayerActionGateResult Authorize(
			const Fdemo_mapShanmenPlayerActionOccupancySnapshot& Occupancy =
				Fdemo_mapShanmenPlayerActionOccupancySnapshot())
		{
			return Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
				Coordinator.TryAuthorizePlayerAction(
					Edemo_mapShanmenPlayerActionKind::SwordQi,
					Occupancy));
		}
	};

	Fdemo_mapShanmenPlayerActionOccupancySnapshot ThrownOccupancy()
	{
		Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
		Occupancy.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			SwordQiProductOtherOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::None);
		return Occupancy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiProductAuthorityTest,
	"Shanmen.0_0_10.Product.SwordQiProductSession.CanonicalAuthority",
	SwordQiProductFlags)

bool Fdemo_mapSwordQiProductAuthorityTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordQiProductConfig First;
	Fdemo_mapShanmenSwordQiProductConfig Replay;
	TestTrue(TEXT("canonical Sword Qi product policy is available"),
		Fdemo_mapShanmenSwordQiProductConfig::TryCreateCanonical(First)
			&& First.IsValid());
	TestTrue(TEXT("canonical policy identity is replay stable"),
		Fdemo_mapShanmenSwordQiProductConfig::TryCreateCanonical(Replay)
			&& Replay.GetConfigId() == First.GetConfigId());
	TestEqual(TEXT("P-stage speed is owned by one config"),
		First.GetDefinition().GetFlightSpeed(), 900.0f);
	TestEqual(TEXT("P-stage range is owned by one config"),
		First.GetDefinition().GetMaximumRange(), 1400.0f);

	FSwordQiProductFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.3 authority fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiProductStartResult Invalid =
		Fdemo_mapShanmenSwordQiProductAuthority::PrepareLaunch(
			Fixture.Coordinator,
			SwordQiProductItem,
			20.0f,
			FVector::ZeroVector,
			FVector::ZeroVector);
	TestTrue(TEXT("invalid sampled direction fails before reservation"),
		!Invalid.IsReady()
			&& Invalid.Status
				== Edemo_mapShanmenSwordQiProductStartStatus::InvalidDirection
			&& Fixture.Coordinator.GetNextPlayerSwordQiActivationSequence() == 1);

	const Fdemo_mapShanmenSwordQiProductStartResult Ready = Fixture.Prepare();
	TestTrue(TEXT("valid product request freezes one command"), Ready.IsReady());
	TestTrue(TEXT("Run owns sequence-one identity and exact sword"),
		Ready.Reservation.GetActivationSequence() == 1
			&& Ready.Reservation.GetAction().GetRunId()
				== Fixture.Coordinator.GetRunId()
			&& Ready.Reservation.GetAction().GetSourceEntityId()
				== Fixture.Coordinator.GetPlayerEntityId()
			&& Ready.Reservation.GetAction().GetSourceItemInstanceId()
				== SwordQiProductItem
			&& Ready.Command.GetCommandId()
				== Ready.Reservation.GetActivationId()
			&& Fixture.Coordinator.GetNextPlayerSwordQiActivationSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiProductRouteReplayTest,
	"Shanmen.0_0_10.Product.SwordQiProductSession.RouteReplayAndOccupancy",
	SwordQiProductFlags)

bool Fdemo_mapSwordQiProductRouteReplayTest::RunTest(const FString&)
{
	FSwordQiProductFixture Fixture;
	const Fdemo_mapShanmenSwordQiProductStartResult Start = Fixture.Prepare();
	if (!Fixture.bReady || !Start.IsReady())
	{
		AddError(TEXT("Could not build the P18.3 route fixture."));
		return false;
	}

	Fdemo_mapShanmenSwordQiProductSession Session;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenSwordQiProductRouteResult Applied =
		Session.TryRoute(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			Start.Command,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize();
			});
	Ademo_mapShanmenSwordQiProjectile* FirstProjectile =
		Session.GetHost().GetProjectile();
	TestTrue(TEXT("authorized command crosses Active and owns one flight"),
		Applied.IsAccepted()
			&& !Applied.IsReplay()
			&& AuthorizationCount == 1
			&& Session.IsValid()
			&& Session.IsInFlight()
			&& FirstProjectile != nullptr
			&& Session.GetOccupancyOwnerId() == Start.Command.GetCommandId());

	Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
	TestTrue(TEXT("active flight projects one non-preemptible Sword Qi owner"),
		Session.TryAppendOccupancy(Occupancy)
			&& Occupancy.IsValid()
			&& Occupancy.NumOccupiedProducts() == 1
			&& Occupancy.GetSoleClaim()
			&& Occupancy.GetSoleClaim()->OwningAction
				== Edemo_mapShanmenPlayerActionKind::SwordQi
			&& Occupancy.GetSoleClaim()->OwnerId
				== Start.Command.GetCommandId());
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt BlockedSword =
		Fixture.Coordinator.TryAuthorizePlayerAction(
			Edemo_mapShanmenPlayerActionKind::BasicSword,
			Occupancy);
	TestTrue(TEXT("in-flight Sword Qi blocks another sword action"),
		BlockedSword.IsValid()
			&& !BlockedSword.IsAuthorized()
			&& BlockedSword.OccupyingAction
				== Edemo_mapShanmenPlayerActionKind::SwordQi);

	const Fdemo_mapShanmenSwordQiProductRouteResult Replay =
		Session.TryRoute(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			Start.Command,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize();
			});
	TestTrue(TEXT("exact replay performs no authorization or second spawn"),
		Replay.IsAccepted()
			&& Replay.IsReplay()
			&& AuthorizationCount == 1
			&& Session.GetHost().GetProjectile() == FirstProjectile
			&& Session.NumProcessedCommands() == 1);

	Fdemo_mapShanmenSwordQiTerminalReceipt Terminal;
	TestTrue(TEXT("owner interruption becomes terminal and can be retired"),
		Session.TryInterrupt()
			&& Session.IsTerminal()
			&& Session.TryRetireTerminal(Terminal)
			&& Terminal.IsValid()
			&& Terminal.Kind
				== Edemo_mapShanmenSwordQiTerminalKind::Interrupted
			&& Session.IsEmpty()
			&& Session.IsValid());
	Fdemo_mapShanmenPlayerActionOccupancySnapshot EmptyAfterTerminal;
	TestTrue(TEXT("retired terminal flight releases the action lane"),
		Session.TryAppendOccupancy(EmptyAfterTerminal)
			&& EmptyAfterTerminal.NumOccupiedProducts() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiProductConflictTest,
	"Shanmen.0_0_10.Product.SwordQiProductSession.ConflictAndPayloadReplay",
	SwordQiProductFlags)

bool Fdemo_mapSwordQiProductConflictTest::RunTest(const FString&)
{
	FSwordQiProductFixture Fixture;
	const Fdemo_mapShanmenSwordQiProductStartResult Start = Fixture.Prepare();
	if (!Fixture.bReady || !Start.IsReady())
	{
		AddError(TEXT("Could not build the P18.3 conflict fixture."));
		return false;
	}

	Fdemo_mapShanmenSwordQiProductSession Session;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenSwordQiProductRouteResult Conflict =
		Session.TryRoute(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			Start.Command,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize(ThrownOccupancy());
			});
	TestTrue(TEXT("existing persistent action rejects and records the command"),
		Conflict.Status
			== Edemo_mapShanmenSwordQiProductRouteStatus::ActionGateRejected
			&& Conflict.IsTerminal()
			&& AuthorizationCount == 1
			&& Session.IsEmpty()
			&& Session.NumProcessedCommands() == 1);

	const Fdemo_mapShanmenSwordQiProductRouteResult Replay =
		Session.TryRoute(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			Start.Command,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize();
			});
	TestTrue(TEXT("rejected command cannot be reauthorized on replay"),
		Replay.IsReplay()
			&& Replay.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::ActionGateRejected
			&& AuthorizationCount == 1);

	FShanmenSwordQiOffenseSnapshot Offense;
	Fdemo_mapShanmenSwordQiLaunchCommand ConflictingPayload;
	check(FShanmenSwordQiOffenseSnapshot::TryCapture(20.0f, Offense));
	check(Fdemo_mapShanmenSwordQiLaunchCommand::TryCapture(
		Start.Reservation,
		Start.Config,
		Offense,
		FVector(21.0, 30.0, 60.0),
		FVector::ForwardVector,
		ConflictingPayload));
	const Fdemo_mapShanmenSwordQiProductRouteResult PayloadConflict =
		Session.TryRoute(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			ConflictingPayload,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize();
			});
	TestTrue(TEXT("one ActivationId cannot alias another launch payload"),
		PayloadConflict.Status
			== Edemo_mapShanmenSwordQiProductRouteStatus::CommandIdConflict
			&& !PayloadConflict.IsTerminal()
			&& AuthorizationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiProductFailureResetTest,
	"Shanmen.0_0_10.Product.SwordQiProductSession.SpawnFailureAndRunReset",
	SwordQiProductFlags)

bool Fdemo_mapSwordQiProductFailureResetTest::RunTest(const FString&)
{
	FSwordQiProductFixture Fixture;
	const Fdemo_mapShanmenSwordQiProductStartResult First = Fixture.Prepare();
	if (!Fixture.bReady || !First.IsReady())
	{
		AddError(TEXT("Could not build the P18.3 failure fixture."));
		return false;
	}

	Fdemo_mapShanmenSwordQiProductSession Session;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenSwordQiProductRouteResult Failed =
		Session.TryRoute(
			nullptr,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			First.Command,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize();
			});
	TestTrue(TEXT("missing World is a durable no-carrier rejection"),
		Failed.Status
			== Edemo_mapShanmenSwordQiProductRouteStatus::CarrierSpawnRejected
			&& Failed.IsTerminal()
			&& AuthorizationCount == 1
			&& Session.IsEmpty()
			&& Session.IsValid());
	const Fdemo_mapShanmenSwordQiProductRouteResult Replay =
		Session.TryRoute(
			Fixture.World,
			Ademo_mapShanmenSwordQiProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.Pawn,
			First.Command,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				return Fixture.Authorize();
			});
	TestTrue(TEXT("spawn failure replay cannot silently become a launch"),
		Replay.IsReplay()
			&& Replay.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::CarrierSpawnRejected
			&& AuthorizationCount == 1
			&& Session.IsEmpty());

	TestTrue(TEXT("inactive Session and Run can close cleanly"),
		Session.Reset()
			&& Fixture.Coordinator.TryEndRun(
				SwordQiProductRunA, Fixture.Diagnostic));
	TestEqual(TEXT("Run close resets the Sword Qi sequence"),
		Fixture.Coordinator.GetNextPlayerSwordQiActivationSequence(),
		static_cast<uint64>(1));
	TestTrue(TEXT("same player can open a distinct next Run"),
		Fixture.Coordinator.TryBeginRun(
			SwordQiProductRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	const Fdemo_mapShanmenSwordQiProductStartResult Next = Fixture.Prepare();
	TestTrue(TEXT("next Run starts at sequence one with different identity"),
		Next.IsReady()
			&& Next.Reservation.GetActivationSequence() == 1
			&& Next.Reservation.GetActivationId()
				!= First.Reservation.GetActivationId());
	return true;
}

#endif
