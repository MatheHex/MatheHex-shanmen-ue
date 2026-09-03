#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiInputAdapter.h"

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
	constexpr EAutomationTestFlags SwordQiInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordQiInputRunA(0xD3850001, 0, 0, 1);
	const FGuid SwordQiInputRunB(0xD3850002, 0, 0, 1);
	const FGuid SwordQiInputEventA(0xD3850003, 0, 0, 1);
	const FGuid SwordQiInputEventB(0xD3850004, 0, 0, 1);

	Fdemo_mapShanmenSwordQiInputSample SpatialSample(
		const FVector& Origin = FVector(20.0, 30.0, 60.0),
		const FVector& AimDirection = FVector::ForwardVector)
	{
		Fdemo_mapShanmenSwordQiInputSample Sample;
		check(Fdemo_mapShanmenSwordQiInputSample::TryCapture(
			Origin,
			AimDirection,
			Sample));
		return Sample;
	}

	struct FSwordQiInputFixture
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

		FSwordQiInputFixture()
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
					Pawn, TEXT("P185SwordQiHealth"))
				: nullptr;
			Attributes = Pawn
				? NewObject<Udemo_mapAttributeComponent>(
					Pawn, TEXT("P185SwordQiAttributes"))
				: nullptr;
			bReady = Pawn && Health && Attributes
				&& Attributes->SetBaseValue(
					Fdemo_mapAttributeIds::Primary01, 6.0f)
				&& Coordinator.TryBeginRun(
					SwordQiInputRunA,
					Pawn,
					Health,
					Diagnostic)
				&& Controller.TryBegin(
					SwordQiInputRunA,
					Diagnostic);
		}

		~FSwordQiInputFixture()
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

		Fdemo_mapShanmenSwordQiControllerResult Route(
			const Fdemo_mapShanmenSwordQiIntent& Intent)
		{
			return Controller.TrySubmit(
				World,
				Ademo_mapShanmenSwordQiProjectile::StaticClass(),
				Items,
				*Attributes,
				Coordinator,
				Pawn,
				Intent,
				[this]()
				{
					++AuthorizationCount;
					return Fdemo_mapShanmenPlayerActionGateResult::
						FromArbitration(
							Coordinator.TryAuthorizePlayerAction(
								Edemo_mapShanmenPlayerActionKind::SwordQi,
								Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
				});
		}

		Fdemo_mapShanmenSwordQiInputResult Input(
			const FGuid& EventId,
			const FVector& Origin,
			int32& SampleCount,
			int32& RouteCount)
		{
			return Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
				true,
				bReady,
				SwordQiInputRunA,
				EventId,
				[&SampleCount, Origin]()
				{
					++SampleCount;
					return SpatialSample(Origin, FVector(10.0, 3.0, 2.0));
				},
				[this, &RouteCount](
					const Fdemo_mapShanmenSwordQiIntent& Intent)
				{
					++RouteCount;
					return Route(Intent);
				});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiInputDeterministicIdentityTest,
	"Shanmen.0_0_10.Product.SwordQiInputAdapter.DeterministicIdentity",
	SwordQiInputFlags)

bool Fdemo_mapSwordQiInputDeterministicIdentityTest::RunTest(const FString&)
{
	const FGuid First = Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
		SwordQiInputRunA,
		SwordQiInputEventA);
	TestTrue(TEXT("one Run/event pair has one stable intent identity"),
		First.IsValid()
			&& First
				== Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
					SwordQiInputRunA,
					SwordQiInputEventA));
	TestTrue(TEXT("event identity and Run namespace both participate"),
		First
			!= Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
				SwordQiInputRunA,
				SwordQiInputEventB)
			&& First
				!= Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
					SwordQiInputRunB,
					SwordQiInputEventA));
	TestFalse(TEXT("invalid identity fails closed"),
		Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
			FGuid(), SwordQiInputEventA).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiInputGatesBeforeSamplingTest,
	"Shanmen.0_0_10.Product.SwordQiInputAdapter.GatesBeforeSampling",
	SwordQiInputFlags)

bool Fdemo_mapSwordQiInputGatesBeforeSamplingTest::RunTest(const FString&)
{
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	auto Sample = [&SampleCount]()
	{
		++SampleCount;
		return SpatialSample();
	};
	auto Route = [&RouteCount](const Fdemo_mapShanmenSwordQiIntent&)
	{
		++RouteCount;
		return Fdemo_mapShanmenSwordQiControllerResult();
	};

	const Fdemo_mapShanmenSwordQiInputResult GameplayBlocked =
		Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
			false,
			true,
			SwordQiInputRunA,
			SwordQiInputEventA,
			Sample,
			Route);
	const Fdemo_mapShanmenSwordQiInputResult RouteUnavailable =
		Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
			true,
			false,
			SwordQiInputRunA,
			SwordQiInputEventA,
			Sample,
			Route);
	const Fdemo_mapShanmenSwordQiInputResult RunUnavailable =
		Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
			true,
			true,
			FGuid(),
			SwordQiInputEventA,
			Sample,
			Route);
	const Fdemo_mapShanmenSwordQiInputResult EventInvalid =
		Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
			true,
			true,
			SwordQiInputRunA,
			FGuid(),
			Sample,
			Route);

	TestTrue(TEXT("all availability fences classify before sampling"),
		GameplayBlocked.Status
			== Edemo_mapShanmenSwordQiInputStatus::GameplayBlocked
			&& RouteUnavailable.Status
				== Edemo_mapShanmenSwordQiInputStatus::
					ProductRouteUnavailable
			&& RunUnavailable.Status
				== Edemo_mapShanmenSwordQiInputStatus::RunUnavailable
			&& EventInvalid.Status
				== Edemo_mapShanmenSwordQiInputStatus::EventIdentityInvalid
			&& !GameplayBlocked.bSpatialSampled
			&& !RouteUnavailable.bSpatialSampled
			&& !RunUnavailable.bSpatialSampled
			&& !EventInvalid.bSpatialSampled
			&& SampleCount == 0
			&& RouteCount == 0);
	const Fdemo_mapShanmenSwordQiInputResult InvalidSpatial =
		Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
			true,
			true,
			SwordQiInputRunA,
			SwordQiInputEventA,
			[&SampleCount]()
			{
				++SampleCount;
				return Fdemo_mapShanmenSwordQiInputSample();
			},
			Route);
	TestTrue(TEXT("invalid spatial input samples once and never routes"),
		InvalidSpatial.Status
			== Edemo_mapShanmenSwordQiInputStatus::SpatialSampleRejected
			&& InvalidSpatial.bSpatialSampled
			&& !InvalidSpatial.bProductRouteInvoked
			&& SampleCount == 1
			&& RouteCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiInputAppliedReplayAndConflictTest,
	"Shanmen.0_0_10.Product.SwordQiInputAdapter.AppliedReplayAndConflict",
	SwordQiInputFlags)

bool Fdemo_mapSwordQiInputAppliedReplayAndConflictTest::RunTest(
	const FString&)
{
	FSwordQiInputFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.5 input fixture."));
		return false;
	}
	const FGuid FirstSword =
		Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const FVector FirstOrigin(20.0, 30.0, 60.0);
	const Fdemo_mapShanmenSwordQiInputResult First = Fixture.Input(
		SwordQiInputEventA,
		FirstOrigin,
		SampleCount,
		RouteCount);
	if (!First.IsAccepted())
	{
		AddError(TEXT("Could not launch the P18.5 input fixture."));
		return false;
	}
	const FGuid FrozenCommandId = First.Product.Start.Command.GetCommandId();
	TestTrue(TEXT("one event samples and invokes the sole product route once"),
		FirstSword.IsValid()
			&& SampleCount == 1
			&& RouteCount == 1
			&& First.IntentId
				== Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
					SwordQiInputRunA,
					SwordQiInputEventA)
			&& First.Sample.GetOrigin() == FirstOrigin
			&& First.Sample.GetAimDirection().Equals(
				FVector(10.0, 3.0, 2.0).GetSafeNormal())
			&& First.Product.AttackPower == 7.0f);

	TestTrue(TEXT("authority changes after capture succeed"),
		Fixture.Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01, 98.0f)
			&& Fixture.Equip(*this, Fdemo_mapItemIds::WeaponLevel1)
				.IsValid());
	const Fdemo_mapShanmenSwordQiInputResult Replay = Fixture.Input(
		SwordQiInputEventA,
		FirstOrigin,
		SampleCount,
		RouteCount);
	TestTrue(TEXT("exact event replay keeps the P18.4 frozen command"),
		Replay.IsAccepted()
			&& Replay.Product.bReusedIntent
			&& Replay.Product.Route.IsReplay()
			&& Replay.Product.AttackPower == 7.0f
			&& Replay.Product.Item.Authorization
				.GetSourceItemInstanceId() == FirstSword
			&& Replay.Product.Start.Command.GetCommandId()
				== FrozenCommandId
			&& SampleCount == 2
			&& RouteCount == 2
			&& Fixture.AuthorizationCount == 1);

	const Fdemo_mapShanmenSwordQiInputResult Conflict = Fixture.Input(
		SwordQiInputEventA,
		FVector(21.0, 30.0, 60.0),
		SampleCount,
		RouteCount);
	TestTrue(TEXT("same event identity cannot alias another trajectory"),
		Conflict.Status
			== Edemo_mapShanmenSwordQiInputStatus::ProductRejected
			&& Conflict.Product.Status
				== Edemo_mapShanmenSwordQiControllerStatus::IntentIdConflict
			&& Conflict.IntentId == First.IntentId
			&& SampleCount == 3
			&& RouteCount == 3
			&& Fixture.AuthorizationCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiInputBusyRetryTest,
	"Shanmen.0_0_10.Product.SwordQiInputAdapter.BusyRetry",
	SwordQiInputFlags)

bool Fdemo_mapSwordQiInputBusyRetryTest::RunTest(const FString&)
{
	FSwordQiInputFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.5 retry fixture."));
		return false;
	}
	Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenSwordQiInputResult First = Fixture.Input(
		SwordQiInputEventA,
		FVector(20.0, 30.0, 60.0),
		SampleCount,
		RouteCount);
	const FVector SecondOrigin(40.0, 30.0, 60.0);
	const Fdemo_mapShanmenSwordQiInputResult Busy = Fixture.Input(
		SwordQiInputEventB,
		SecondOrigin,
		SampleCount,
		RouteCount);
	TestTrue(TEXT("busy Host preserves the second event's frozen request"),
		First.IsAccepted()
			&& Busy.Status
				== Edemo_mapShanmenSwordQiInputStatus::ProductRejected
			&& Busy.Product.Status
				== Edemo_mapShanmenSwordQiControllerStatus::RouteRejected
			&& Busy.Product.Route.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy
			&& Busy.Product.Start.IsReady()
			&& SampleCount == 2
			&& RouteCount == 2
			&& Fixture.Controller.NumCapturedIntents() == 2);

	Fdemo_mapShanmenSwordQiTerminalReceipt FirstTerminal;
	TestTrue(TEXT("first flight can retire before an explicit retry"),
		Fixture.Controller.TryInterrupt()
			&& Fixture.Controller.TryRetireTerminal(FirstTerminal)
			&& FirstTerminal.IsValid());
	TestTrue(TEXT("post-capture product authority can move"),
		Fixture.Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01, 12.0f)
			&& Fixture.Equip(*this, Fdemo_mapItemIds::WeaponLevel2)
				.IsValid());
	const Fdemo_mapShanmenSwordQiInputResult Retried = Fixture.Input(
		SwordQiInputEventB,
		SecondOrigin,
		SampleCount,
		RouteCount);
	TestTrue(TEXT("exact retry launches the already-captured command"),
		Retried.IsAccepted()
			&& Retried.Product.bReusedIntent
			&& !Retried.Product.Route.IsReplay()
			&& Retried.Product.AttackPower == 7.0f
			&& Retried.Product.Start.Command.GetCommandId()
				== Busy.Product.Start.Command.GetCommandId()
			&& SampleCount == 3
			&& RouteCount == 3
			&& Fixture.AuthorizationCount == 2
			&& Fixture.Coordinator.
				GetNextPlayerSwordQiActivationSequence() == 3);
	return true;
}

#endif
