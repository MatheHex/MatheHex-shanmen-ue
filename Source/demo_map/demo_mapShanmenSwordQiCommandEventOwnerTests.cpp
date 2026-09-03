#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiCommandEventOwner.h"

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
	constexpr EAutomationTestFlags SwordQiCommandEventFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordQiCommandRunA(0xD3860001, 0, 0, 1);
	const FGuid SwordQiCommandRunB(0xD3860002, 0, 0, 1);

	Fdemo_mapShanmenSwordQiInputSample CommandSpatialSample(
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

	struct FSwordQiCommandEventFixture
	{
		UWorld* World = nullptr;
		APawn* Pawn = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Fdemo_mapItemAuthority Items;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenSwordQiProductController Controller;
		Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
		FString Diagnostic;
		int32 AuthorizationCount = 0;
		bool bReady = false;

		FSwordQiCommandEventFixture()
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
					Pawn, TEXT("P186SwordQiHealth"))
				: nullptr;
			Attributes = Pawn
				? NewObject<Udemo_mapAttributeComponent>(
					Pawn, TEXT("P186SwordQiAttributes"))
				: nullptr;
			bReady = Pawn && Health && Attributes
				&& Attributes->SetBaseValue(
					Fdemo_mapAttributeIds::Primary01, 6.0f)
				&& Coordinator.TryBeginRun(
					SwordQiCommandRunA,
					Pawn,
					Health,
					Diagnostic)
				&& Controller.TryBegin(
					SwordQiCommandRunA,
					Diagnostic)
				&& Owner.TryBegin(
					SwordQiCommandRunA,
					Diagnostic);
		}

		~FSwordQiCommandEventFixture()
		{
			Owner.Reset();
			Controller.Reset();
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		FGuid Equip(FAutomationTestBase& Test, const FName DefinitionId)
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
				SwordQiCommandRunA,
				EventId,
				[&SampleCount, Origin]()
				{
					++SampleCount;
					return CommandSpatialSample(
						Origin, FVector(10.0, 3.0, 2.0));
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
	Fdemo_mapSwordQiCommandEventLifecycleTest,
	"Shanmen.0_0_10.Product.SwordQiCommandEventOwner.LifecycleIdentity",
	SwordQiCommandEventFlags)

bool Fdemo_mapSwordQiCommandEventLifecycleTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	const FGuid First =
		Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
			SwordQiCommandRunA,
			1);
	TestTrue(TEXT("Run and sequence derive one stable logical event"),
		First.IsValid()
			&& First
				== Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
					SwordQiCommandRunA,
					1));
	TestTrue(TEXT("Run and sequence both participate in identity"),
		First
			!= Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
				SwordQiCommandRunA,
				2)
			&& First
				!= Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
					SwordQiCommandRunB,
					1));
	TestFalse(TEXT("invalid identity fails closed"),
		Fdemo_mapShanmenSwordQiCommandEventOwner::MakeInputEventId(
			FGuid(), 1).IsValid());
	TestTrue(TEXT("default owner is valid and empty"),
		Owner.IsValid() && Owner.IsEmpty());

	FString Diagnostic;
	TestTrue(TEXT("owner binds exact Run"),
		Owner.TryBegin(SwordQiCommandRunA, Diagnostic)
			&& Owner.IsActive()
			&& Owner.IsValid()
			&& Owner.GetNextEventSequence() == 1);
	TestTrue(TEXT("same Run binding is idempotent"),
		Owner.TryBegin(SwordQiCommandRunA, Diagnostic));
	TestFalse(TEXT("active owner cannot change Run"),
		Owner.TryBegin(SwordQiCommandRunB, Diagnostic));

	Fdemo_mapShanmenSwordQiCommandEventEndSummary Summary;
	TestFalse(TEXT("teardown requires exact Run"),
		Owner.TryEnd(SwordQiCommandRunB, Summary, Diagnostic));
	TestTrue(TEXT("exact teardown reports and clears empty event ledger"),
		Owner.TryEnd(SwordQiCommandRunA, Summary, Diagnostic)
			&& Summary.IsValid()
			&& Summary.RunId == SwordQiCommandRunA
			&& Summary.CommittedEventCount == 0
			&& Owner.IsValid()
			&& Owner.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiCommandEventPreRouteFenceTest,
	"Shanmen.0_0_10.Product.SwordQiCommandEventOwner.PreRouteFences",
	SwordQiCommandEventFlags)

bool Fdemo_mapSwordQiCommandEventPreRouteFenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	FString Diagnostic;
	if (!Owner.TryBegin(SwordQiCommandRunA, Diagnostic))
	{
		AddError(TEXT("Could not bind the P18.6 fence fixture."));
		return false;
	}
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const auto ProductRoute = [&RouteCount](
		const Fdemo_mapShanmenSwordQiIntent&)
	{
		++RouteCount;
		return Fdemo_mapShanmenSwordQiControllerResult();
	};

	const Fdemo_mapShanmenSwordQiCommandEventResult Blocked =
		Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				return Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
					false,
					true,
					SwordQiCommandRunA,
					EventId,
					[&SampleCount]()
					{
						++SampleCount;
						return CommandSpatialSample();
					},
					ProductRoute);
			});
	TestTrue(TEXT("gameplay gate does not consume event identity"),
		Blocked.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& Blocked.bNewEvent
			&& !Blocked.bEventCommitted
			&& !Blocked.CanReplay()
			&& SampleCount == 0
			&& RouteCount == 0
			&& Owner.GetNextEventSequence() == 1);

	const Fdemo_mapShanmenSwordQiCommandEventResult InvalidSpatial =
		Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				return Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
					true,
					true,
					SwordQiCommandRunA,
					EventId,
					[&SampleCount]()
					{
						++SampleCount;
						return Fdemo_mapShanmenSwordQiInputSample();
					},
					ProductRoute);
			});
	TestTrue(TEXT("invalid spatial sample preserves the same candidate"),
		InvalidSpatial.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& !InvalidSpatial.bEventCommitted
			&& InvalidSpatial.Event.GetInputEventId()
				== Blocked.Event.GetInputEventId()
			&& SampleCount == 1
			&& RouteCount == 0
			&& Owner.GetNextEventSequence() == 1);

	const Fdemo_mapShanmenSwordQiCommandEventResult ProductRejected =
		Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				return Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
					true,
					true,
					SwordQiCommandRunA,
					EventId,
					[&SampleCount]()
					{
						++SampleCount;
						return CommandSpatialSample();
					},
					ProductRoute);
			});
	TestTrue(TEXT("product invocation commits identity despite rejection"),
		ProductRejected.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& ProductRejected.bEventCommitted
			&& ProductRejected.CanReplay()
			&& ProductRejected.Event.GetEventSequence() == 1
			&& SampleCount == 2
			&& RouteCount == 1
			&& Owner.GetNextEventSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiCommandEventAppliedReplayTest,
	"Shanmen.0_0_10.Product.SwordQiCommandEventOwner.AppliedReplay",
	SwordQiCommandEventFlags)

bool Fdemo_mapSwordQiCommandEventAppliedReplayTest::RunTest(
	const FString&)
{
	FSwordQiCommandEventFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.6 applied fixture."));
		return false;
	}
	const FGuid FirstSword =
		Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const FVector FirstOrigin(20.0, 30.0, 60.0);
	const Fdemo_mapShanmenSwordQiCommandEventResult First =
		Fixture.Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				return Fixture.Input(
					EventId, FirstOrigin, SampleCount, RouteCount);
			});
	if (!First.IsAccepted())
	{
		AddError(TEXT("Could not launch the P18.6 applied fixture."));
		return false;
	}
	const FGuid FrozenCommandId =
		First.Input.Product.Start.Command.GetCommandId();
	TestTrue(TEXT("new logical command commits exactly one event"),
		FirstSword.IsValid()
			&& First.bNewEvent
			&& First.bEventCommitted
			&& First.Event.GetEventSequence() == 1
			&& First.Input.InputEventId == First.Event.GetInputEventId()
			&& SampleCount == 1
			&& RouteCount == 1
			&& Fixture.Owner.GetNextEventSequence() == 2);

	TestTrue(TEXT("post-capture product authority can move"),
		Fixture.Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01, 98.0f)
			&& Fixture.Equip(*this, Fdemo_mapItemIds::WeaponLevel1)
				.IsValid());
	const Fdemo_mapShanmenSwordQiCommandEventResult Replay =
		Fixture.Owner.TryReplay(
			First.Event,
			[&](const FGuid& EventId)
			{
				return Fixture.Input(
					EventId, FirstOrigin, SampleCount, RouteCount);
			});
	TestTrue(TEXT("explicit replay reuses identity without allocating"),
		Replay.IsAccepted()
			&& !Replay.bNewEvent
			&& Replay.bEventCommitted
			&& Replay.Event.GetInputEventId()
				== First.Event.GetInputEventId()
			&& Replay.Input.Product.bReusedIntent
			&& Replay.Input.Product.Route.IsReplay()
			&& Replay.Input.Product.AttackPower == 7.0f
			&& Replay.Input.Product.Item.Authorization
				.GetSourceItemInstanceId() == FirstSword
			&& Replay.Input.Product.Start.Command.GetCommandId()
				== FrozenCommandId
			&& SampleCount == 2
			&& RouteCount == 2
			&& Fixture.Owner.GetNextEventSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiCommandEventBusyRetryTest,
	"Shanmen.0_0_10.Product.SwordQiCommandEventOwner.BusyRetry",
	SwordQiCommandEventFlags)

bool Fdemo_mapSwordQiCommandEventBusyRetryTest::RunTest(const FString&)
{
	FSwordQiCommandEventFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not build the P18.6 retry fixture."));
		return false;
	}
	Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const FVector FirstOrigin(20.0, 30.0, 60.0);
	const Fdemo_mapShanmenSwordQiCommandEventResult First =
		Fixture.Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				return Fixture.Input(
					EventId, FirstOrigin, SampleCount, RouteCount);
			});
	const FVector SecondOrigin(40.0, 30.0, 60.0);
	const Fdemo_mapShanmenSwordQiCommandEventResult Busy =
		Fixture.Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				return Fixture.Input(
					EventId, SecondOrigin, SampleCount, RouteCount);
			});
	TestTrue(TEXT("busy Host still commits the second logical event"),
		First.IsAccepted()
			&& Busy.Status
				== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& Busy.bNewEvent
			&& Busy.bEventCommitted
			&& Busy.CanReplay()
			&& Busy.Event.GetEventSequence() == 2
			&& Busy.Input.Product.Route.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy
			&& Fixture.Owner.GetNextEventSequence() == 3
			&& Fixture.Controller.NumCapturedIntents() == 2);

	Fdemo_mapShanmenSwordQiTerminalReceipt FirstTerminal;
	TestTrue(TEXT("first flight retires before explicit retry"),
		Fixture.Controller.TryInterrupt()
			&& Fixture.Controller.TryRetireTerminal(FirstTerminal)
			&& FirstTerminal.IsValid());
	TestTrue(TEXT("authority may move after rejected command capture"),
		Fixture.Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01, 12.0f)
			&& Fixture.Equip(*this, Fdemo_mapItemIds::WeaponLevel2)
				.IsValid());
	const Fdemo_mapShanmenSwordQiCommandEventResult Retried =
		Fixture.Owner.TryReplay(
			Busy.Event,
			[&](const FGuid& EventId)
			{
				return Fixture.Input(
					EventId, SecondOrigin, SampleCount, RouteCount);
			});
	TestTrue(TEXT("retry launches frozen command without new event"),
		Retried.IsAccepted()
			&& !Retried.bNewEvent
			&& Retried.Input.Product.bReusedIntent
			&& !Retried.Input.Product.Route.IsReplay()
			&& Retried.Input.Product.AttackPower == 7.0f
			&& Retried.Input.Product.Start.Command.GetCommandId()
				== Busy.Input.Product.Start.Command.GetCommandId()
			&& SampleCount == 3
			&& RouteCount == 3
			&& Fixture.AuthorizationCount == 2
			&& Fixture.Owner.GetNextEventSequence() == 3);

	Fdemo_mapShanmenSwordQiCommandEventEndSummary Summary;
	TestTrue(TEXT("teardown reports exactly two committed events"),
		Fixture.Owner.TryEnd(
			SwordQiCommandRunA, Summary, Fixture.Diagnostic)
			&& Summary.IsValid()
			&& Summary.CommittedEventCount == 2
			&& Fixture.Owner.IsEmpty());
	return true;
}

#endif
