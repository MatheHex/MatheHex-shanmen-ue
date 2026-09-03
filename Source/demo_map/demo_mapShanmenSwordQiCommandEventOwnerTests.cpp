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

		Fdemo_mapShanmenSwordQiInputResult FrozenInput(
			const FGuid& EventId,
			const Fdemo_mapShanmenSwordQiInputSample& FrozenSample,
			int32& FrozenReadCount,
			int32& RouteCount)
		{
			return Fdemo_mapShanmenSwordQiInputAdapter::RouteStartInput(
				true,
				bReady,
				SwordQiCommandRunA,
				EventId,
				[&FrozenSample, &FrozenReadCount]()
				{
					++FrozenReadCount;
					return FrozenSample;
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
			&& !Summary.HadPendingRetry()
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
		AddError(TEXT("Could not bind the P18.8 fence fixture."));
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
	TestTrue(TEXT("valid sample freezes request despite product rejection"),
		ProductRejected.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& ProductRejected.bEventCommitted
			&& ProductRejected.CanReplay()
			&& ProductRejected.Request.IsValid()
			&& ProductRejected.Request.GetEvent().GetInputEventId()
				== ProductRejected.Event.GetInputEventId()
			&& ProductRejected.Request.GetSample().GetOrigin()
				== ProductRejected.Input.Sample.GetOrigin()
			&& ProductRejected.Event.GetEventSequence() == 1
			&& !ProductRejected.bPendingRetryStored
			&& !Owner.HasPendingRetry()
			&& SampleCount == 2
			&& RouteCount == 1
			&& Owner.GetNextEventSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiCommandFrozenBeforeProductRouteTest,
	"Shanmen.0_0_10.Product.SwordQiCommandEventOwner.FrozenBeforeProductRoute",
	SwordQiCommandEventFlags)

bool Fdemo_mapSwordQiCommandFrozenBeforeProductRouteTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	FString Diagnostic;
	if (!Owner.TryBegin(SwordQiCommandRunA, Diagnostic))
	{
		AddError(TEXT("Could not bind the P18.8 frozen-request fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiInputSample FrozenSample =
		CommandSpatialSample(
			FVector(17.0, 23.0, 61.0),
			FVector(8.0, 5.0, 3.0));
	int32 SourceCaptureCount = 0;
	const Fdemo_mapShanmenSwordQiCommandEventResult Captured =
		Owner.TryIssue(
			[&](const FGuid& EventId)
			{
				++SourceCaptureCount;
				Fdemo_mapShanmenSwordQiInputResult Input;
				Input.Status =
					Edemo_mapShanmenSwordQiInputStatus::IntentCaptureRejected;
				Input.bSpatialSampled = true;
				Input.InputEventId = EventId;
				Input.RunId = SwordQiCommandRunA;
				Input.Sample = FrozenSample;
				Input.Diagnostic =
					TEXT("Synthetic pre-product capture rejection.");
				return Input;
			});
	TestTrue(TEXT("first valid sample commits a replayable request"),
		Captured.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& Captured.bNewEvent
			&& Captured.bEventCommitted
			&& Captured.CanReplay()
			&& Captured.Request.IsValid()
			&& !Captured.Input.bProductRouteInvoked
			&& !Captured.bPendingRetryStored
			&& !Owner.HasPendingRetry()
			&& SourceCaptureCount == 1
			&& Owner.GetNextEventSequence() == 2);

	int32 FrozenRouteCount = 0;
	const Fdemo_mapShanmenSwordQiCommandEventResult Replayed =
		Owner.TryReplay(
			Captured.Request,
			[&](
				const FGuid& EventId,
				const Fdemo_mapShanmenSwordQiInputSample& RequestSample)
			{
				++FrozenRouteCount;
				Fdemo_mapShanmenSwordQiInputResult Input;
				Input.Status =
					Edemo_mapShanmenSwordQiInputStatus::IntentCaptureRejected;
				Input.bSpatialSampled = true;
				Input.InputEventId = EventId;
				Input.RunId = SwordQiCommandRunA;
				Input.Sample = RequestSample;
				Input.Diagnostic =
					TEXT("Synthetic frozen-request replay rejection.");
				return Input;
			});
	TestTrue(TEXT("replay receives only the frozen sample and never reallocates"),
		Replayed.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& !Replayed.bNewEvent
			&& Replayed.CanReplay()
			&& Replayed.Request.GetSample().GetOrigin()
				== FrozenSample.GetOrigin()
			&& Replayed.Request.GetSample().GetAimDirection().Equals(
				FrozenSample.GetAimDirection())
			&& !Replayed.bPendingRetryAttempt
			&& !Replayed.bPendingRetryStored
			&& !Owner.HasPendingRetry()
			&& SourceCaptureCount == 1
			&& FrozenRouteCount == 1
			&& Owner.GetNextEventSequence() == 2);

	Fdemo_mapShanmenSwordQiCommandEventOwner OtherOwner;
	TestTrue(TEXT("second owner binds another Run"),
		OtherOwner.TryBegin(SwordQiCommandRunB, Diagnostic));
	int32 ForeignRouteCount = 0;
	const Fdemo_mapShanmenSwordQiCommandEventResult Foreign =
		OtherOwner.TryReplay(
			Captured.Request,
			[&](
				const FGuid&,
				const Fdemo_mapShanmenSwordQiInputSample&)
			{
				++ForeignRouteCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			});
	TestTrue(TEXT("frozen request cannot cross its Run owner"),
		Foreign.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::RequestNotOwned
			&& ForeignRouteCount == 0
			&& !Foreign.CanReplay());
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
		AddError(TEXT("Could not build the P18.8 applied fixture."));
		return false;
	}
	const FGuid FirstSword =
		Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	int32 SampleCount = 0;
	int32 FrozenReadCount = 0;
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
		AddError(TEXT("Could not launch the P18.8 applied fixture."));
		return false;
	}
	const FGuid FrozenCommandId =
		First.Input.Product.Start.Command.GetCommandId();
	TestTrue(TEXT("new logical command commits exactly one event"),
		FirstSword.IsValid()
			&& First.bNewEvent
			&& First.bEventCommitted
			&& First.Request.IsValid()
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
			First.Request,
			[&](
				const FGuid& EventId,
				const Fdemo_mapShanmenSwordQiInputSample& FrozenSample)
			{
				return Fixture.FrozenInput(
					EventId, FrozenSample, FrozenReadCount, RouteCount);
			});
	TestTrue(TEXT("explicit replay reuses identity and frozen sample"),
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
			&& !Replay.bPendingRetryAttempt
			&& !Replay.bPendingRetryStored
			&& !Fixture.Owner.HasPendingRetry()
			&& SampleCount == 1
			&& FrozenReadCount == 1
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
		AddError(TEXT("Could not build the P18.8 retry fixture."));
		return false;
	}
	Fixture.Equip(*this, Fdemo_mapItemIds::TrainingBlade);
	int32 SampleCount = 0;
	int32 FrozenReadCount = 0;
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
			&& Busy.bPendingRetryStored
			&& Fixture.Owner.HasPendingRetry()
			&& Fixture.Owner.GetPendingRetryRequest()
			&& Fixture.Owner.GetPendingRetryRequest()->Matches(Busy.Request)
			&& Fixture.Owner.GetNextEventSequence() == 3
			&& Fixture.Controller.NumCapturedIntents() == 2);

	int32 BlockedIssueRouteCount = 0;
	const Fdemo_mapShanmenSwordQiCommandEventResult BlockedIssue =
		Fixture.Owner.TryIssue(
			[&BlockedIssueRouteCount](const FGuid&)
			{
				++BlockedIssueRouteCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			});
	int32 BlockedReplayRouteCount = 0;
	const Fdemo_mapShanmenSwordQiCommandEventResult BlockedReplay =
		Fixture.Owner.TryReplay(
			Busy.Request,
			[&BlockedReplayRouteCount](
				const FGuid&,
				const Fdemo_mapShanmenSwordQiInputSample&)
			{
				++BlockedReplayRouteCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			});
	TestTrue(TEXT("capacity-one slot blocks replacement and generic replay"),
		BlockedIssue.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::
				PendingRetryOccupied
			&& BlockedReplay.Status
				== Edemo_mapShanmenSwordQiCommandEventStatus::
					PendingRetryOccupied
			&& BlockedIssueRouteCount == 0
			&& BlockedReplayRouteCount == 0
			&& Fixture.Owner.GetNextEventSequence() == 3);

	const Fdemo_mapShanmenSwordQiCommandEventResult StillBusy =
		Fixture.Owner.TryRetryPending(
			[&](
				const FGuid& EventId,
				const Fdemo_mapShanmenSwordQiInputSample& FrozenSample)
			{
				return Fixture.FrozenInput(
					EventId, FrozenSample, FrozenReadCount, RouteCount);
			});
	TestTrue(TEXT("explicit retry retains the same request while Host is busy"),
		StillBusy.Status
			== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& StillBusy.bPendingRetryAttempt
			&& StillBusy.bPendingRetryStored
			&& StillBusy.Request.Matches(Busy.Request)
			&& StillBusy.Input.Product.Route.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy
			&& Fixture.Owner.HasPendingRetry()
			&& SampleCount == 2
			&& FrozenReadCount == 1
			&& RouteCount == 3
			&& Fixture.AuthorizationCount == 1
			&& Fixture.Owner.GetNextEventSequence() == 3);

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
		Fixture.Owner.TryRetryPending(
			[&](
				const FGuid& EventId,
				const Fdemo_mapShanmenSwordQiInputSample& FrozenSample)
			{
				return Fixture.FrozenInput(
					EventId, FrozenSample, FrozenReadCount, RouteCount);
			});
	TestTrue(TEXT("retry launches frozen command without new event"),
		Retried.IsAccepted()
			&& !Retried.bNewEvent
			&& Retried.bPendingRetryAttempt
			&& !Retried.bPendingRetryStored
			&& Retried.Input.Product.bReusedIntent
			&& !Retried.Input.Product.Route.IsReplay()
			&& Retried.Input.Product.AttackPower == 7.0f
			&& Retried.Input.Product.Start.Command.GetCommandId()
				== Busy.Input.Product.Start.Command.GetCommandId()
			&& !Fixture.Owner.HasPendingRetry()
			&& SampleCount == 2
			&& FrozenReadCount == 2
			&& RouteCount == 4
			&& Fixture.AuthorizationCount == 2
			&& Fixture.Owner.GetNextEventSequence() == 3);

	Fdemo_mapShanmenSwordQiCommandEventEndSummary Summary;
	TestTrue(TEXT("teardown reports exactly two committed events"),
		Fixture.Owner.TryEnd(
			SwordQiCommandRunA, Summary, Fixture.Diagnostic)
			&& Summary.IsValid()
			&& Summary.CommittedEventCount == 2
			&& !Summary.HadPendingRetry()
			&& Fixture.Owner.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiCommandPendingCancelAndTeardownTest,
	"Shanmen.0_0_10.Product.SwordQiCommandEventOwner.PendingCancelAndTeardown",
	SwordQiCommandEventFlags)

bool Fdemo_mapSwordQiCommandPendingCancelAndTeardownTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	FString Diagnostic;
	if (!Owner.TryBegin(SwordQiCommandRunA, Diagnostic))
	{
		AddError(TEXT("Could not bind the P18.8 cancellation fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiInputSample FrozenSample =
		CommandSpatialSample(FVector(70.0, 11.0, 62.0));
	const auto HostBusyInput = [&](const FGuid& EventId)
	{
		Fdemo_mapShanmenSwordQiInputResult Input;
		Input.Status = Edemo_mapShanmenSwordQiInputStatus::ProductRejected;
		Input.bSpatialSampled = true;
		Input.bProductRouteInvoked = true;
		Input.InputEventId = EventId;
		Input.RunId = SwordQiCommandRunA;
		Input.IntentId = Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
			SwordQiCommandRunA,
			EventId);
		Input.Sample = FrozenSample;
		Input.Product.Status =
			Edemo_mapShanmenSwordQiControllerStatus::RouteRejected;
		Input.Product.Route.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy;
		Input.Diagnostic = TEXT("Synthetic HostBusy rejection.");
		return Input;
	};

	const Fdemo_mapShanmenSwordQiCommandEventResult FirstPending =
		Owner.TryIssue(HostBusyInput);
	TestTrue(TEXT("HostBusy is the sole retryable rejection"),
		FirstPending.bPendingRetryStored
			&& Owner.HasPendingRetry());

	Fdemo_mapShanmenSwordQiPendingRetryCancellation Cancellation;
	TestTrue(TEXT("explicit cancellation returns immutable request evidence"),
		Owner.TryCancelPending(Cancellation, Diagnostic)
			&& Cancellation.IsValid()
			&& Cancellation.RunId == SwordQiCommandRunA
			&& Cancellation.Request.Matches(FirstPending.Request)
			&& !Owner.HasPendingRetry()
			&& Owner.GetNextEventSequence() == 2);
	Fdemo_mapShanmenSwordQiPendingRetryCancellation MissingCancellation;
	TestFalse(TEXT("empty slot cannot be cancelled twice"),
		Owner.TryCancelPending(MissingCancellation, Diagnostic));

	const Fdemo_mapShanmenSwordQiCommandEventResult SecondPending =
		Owner.TryIssue(HostBusyInput);
	Fdemo_mapShanmenSwordQiCommandEventEndSummary Summary;
	TestTrue(TEXT("Run teardown audits and clears an unconsumed pending request"),
		SecondPending.bPendingRetryStored
			&& SecondPending.Event.GetEventSequence() == 2
			&& Owner.TryEnd(SwordQiCommandRunA, Summary, Diagnostic)
			&& Summary.IsValid()
			&& Summary.CommittedEventCount == 2
			&& Summary.HadPendingRetry()
			&& Summary.PendingRetryAtTeardown.Matches(SecondPending.Request)
			&& Owner.IsValid()
			&& Owner.IsEmpty());
	return true;
}

#endif
