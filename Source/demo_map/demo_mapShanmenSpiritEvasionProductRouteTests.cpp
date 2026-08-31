#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSpiritEvasionProductRoute.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#include <limits>

namespace
{
	const EAutomationTestFlags ProductRouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ProductRouteRun(
		0xE0400001, 0xE0400002, 0xE0400003, 0xE0400004);
	constexpr double ProductRouteStartTime = 500.0;

	class FProductRoutePreflightPort final
		: public Idemo_mapShanmenSpiritEvasionPreflightPort
	{
	public:
		virtual Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Evaluate(
			const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan) override
		{
			++EvaluationCount;
			Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Result;
			Result.PlanId = Plan.GetPlanId();
			Result.Displacement.RequestedDistance =
				Plan.GetPolicy().GetRequestedDistance();
			if (!bAllow)
			{
				Result.Status =
					Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
						InsufficientResolvedDistance;
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready;
			Result.Displacement.ResolvedDistance =
				Plan.GetPolicy().GetRequestedDistance();
			return Result;
		}

		bool bAllow = true;
		int32 EvaluationCount = 0;
	};

	struct FProductRouteFixture
	{
		UWorld* World = nullptr;
		ACharacter* Character = nullptr;
		ACharacter* ForeignCharacter = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapShanmenSpiritEvasionComponent* Component = nullptr;
		Udemo_mapShanmenSpiritEvasionComponent* ForeignComponent = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FProductRouteFixture()
		{
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World || !GEngine)
			{
				return;
			}
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
			Character = World->SpawnActor<ACharacter>();
			ForeignCharacter = World->SpawnActor<ACharacter>();
			Health = Character
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Character,
					TEXT("SpiritEvasionProductRouteHealth"))
				: nullptr;
			if (Character && Health)
			{
				Character->AddInstanceComponent(Health);
				Health->RegisterComponent();
			}
			const Fdemo_mapShanmenSpiritEvasionInstallationResult Installation =
				Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
					Character);
			const Fdemo_mapShanmenSpiritEvasionInstallationResult
				ForeignInstallation =
					Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
						ForeignCharacter);
			Component = Installation.Component;
			ForeignComponent = ForeignInstallation.Component;
			bReady = Character
				&& ForeignCharacter
				&& Health
				&& Health->IsRegistered()
				&& Installation.IsSuccess()
				&& ForeignInstallation.IsSuccess()
				&& Coordinator.TryBeginRun(
					ProductRouteRun,
					Character,
					Health,
					Diagnostic);
		}

		~FProductRouteFixture()
		{
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	Fdemo_mapShanmenSpiritEvasionProductRouteResult RouteForTest(
		FProductRouteFixture& Fixture,
		FProductRoutePreflightPort& Preflight,
		const FVector& Direction = FVector::ForwardVector,
		double StartTime = ProductRouteStartTime)
	{
		return Fdemo_mapShanmenSpiritEvasionProductRoute::
			TryRouteAtForAutomation(
				Fixture.Component,
				Fixture.Coordinator,
				Fixture.Character,
				Direction,
				StartTime,
				Preflight);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteEntryFencesTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.EntryFences",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteEntryFencesTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);

	const Fdemo_mapShanmenSpiritEvasionProductRouteResult NoOwner =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
			Fixture.Component,
			Fixture.Coordinator,
			nullptr,
			FVector::ForwardVector,
			ProductRouteStartTime,
			Preflight);
	TestEqual(
		TEXT("missing owner is classified before reservation"),
		NoOwner.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::OwnerUnavailable);

	const Fdemo_mapShanmenSpiritEvasionProductRouteResult NoComponent =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
			nullptr,
			Fixture.Coordinator,
			Fixture.Character,
			FVector::ForwardVector,
			ProductRouteStartTime,
			Preflight);
	TestEqual(
		TEXT("missing component is classified before reservation"),
		NoComponent.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::ComponentUnavailable);

	const Fdemo_mapShanmenSpiritEvasionProductRouteResult OwnerMismatch =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.ForeignCharacter,
			FVector::ForwardVector,
			ProductRouteStartTime,
			Preflight);
	TestEqual(
		TEXT("foreign component owner is rejected"),
		OwnerMismatch.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::
			ComponentOwnerMismatch);

	const Fdemo_mapShanmenSpiritEvasionProductRouteResult UnregisteredOwner =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
			Fixture.ForeignComponent,
			Fixture.Coordinator,
			Fixture.ForeignCharacter,
			FVector::ForwardVector,
			ProductRouteStartTime,
			Preflight);
	TestEqual(
		TEXT("owner outside the active Run registry is rejected"),
		UnregisteredOwner.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::OwnerNotRegistered);

	Fdemo_mapCombatRunCoordinator Unready;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult NoRun =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
			Fixture.Component,
			Unready,
			Fixture.Character,
			FVector::ForwardVector,
			ProductRouteStartTime,
			Preflight);
	TestEqual(
		TEXT("unready Run is rejected before product reservation"),
		NoRun.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::
			CoordinatorUnavailable);
	TestEqual(
		TEXT("all entry failures preserve sequence one"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));
	TestEqual(TEXT("entry failures perform no preflight"), Preflight.EvaluationCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteIntentFencesTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.IntentFences",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteIntentFencesTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const TArray<FVector> InvalidDirections = {
		FVector::ZeroVector,
		FVector::UpVector,
		FVector(NaN, 1.0f, 0.0f)
	};
	for (const FVector& Direction : InvalidDirections)
	{
		const Fdemo_mapShanmenSpiritEvasionProductRouteResult Rejected =
			RouteForTest(Fixture, Preflight, Direction);
		TestEqual(
			TEXT("invalid direction is a product rejection"),
			Rejected.Status,
			Edemo_mapShanmenSpiritEvasionProductRouteStatus::ProductRejected);
		TestFalse(TEXT("rejected intent has no accepted proof"), Rejected.IsAccepted());
	}
	TestEqual(
		TEXT("invalid intents do not consume sequence"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));
	TestEqual(TEXT("invalid intents perform no preflight"), Preflight.EvaluationCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteAppliedTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.AppliedProof",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteAppliedTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult Applied =
		RouteForTest(Fixture, Preflight, FVector(3.0f, 4.0f, 7.0f));
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestTrue(TEXT("complete route proof is accepted"), Applied.IsAccepted());
	TestEqual(
		TEXT("route status is applied"),
		Applied.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::Applied);
	TestEqual(
		TEXT("product direction is planar and normalized"),
		Applied.ProductStart.Command.GetCandidateDirection(),
		FVector(0.6f, 0.8f, 0.0f));
	TestEqual(
		TEXT("router receipt retains reserved activation"),
		Applied.CommandRoute.ActivationId,
		Applied.ProductStart.Reservation.GetActivationId());
	TestTrue(TEXT("component owns the started host"), Fixture.Component->IsActive());
	TestEqual(TEXT("exactly one preflight is performed"), Preflight.EvaluationCount, 1);
	TestEqual(
		TEXT("accepted route consumes sequence one"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteBusyFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.BusyFence",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteBusyFenceTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult First =
		RouteForTest(Fixture, Preflight);
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult Busy =
		RouteForTest(
			Fixture,
			Preflight,
			FVector::RightVector,
			ProductRouteStartTime + 1.0);
	TestTrue(TEXT("first route starts"), First.IsAccepted());
	TestEqual(
		TEXT("nonterminal component rejects before reservation"),
		Busy.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::ComponentBusy);
	TestFalse(TEXT("busy rejection has no product reservation"), Busy.ProductStart.IsReady());
	TestEqual(
		TEXT("busy rejection does not consume sequence two"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(2));
	TestEqual(TEXT("busy rejection performs no second preflight"), Preflight.EvaluationCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteRejectedExecutionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.RejectedExecution",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteRejectedExecutionTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	Preflight.bAllow = false;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult Rejected =
		RouteForTest(Fixture, Preflight);
	TestEqual(
		TEXT("failed preflight is a command-route rejection"),
		Rejected.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::CommandRouteRejected);
	TestTrue(
		TEXT("execution rejection retains the accepted reservation"),
		Rejected.ProductStart.IsReady());
	TestFalse(TEXT("execution rejection is not an accepted route"), Rejected.IsAccepted());
	TestTrue(TEXT("failed start leaves component startable"), Fixture.Component->CanStart());
	TestEqual(
		TEXT("published reservation remains consumed"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(2));

	Preflight.bAllow = true;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult Retry =
		RouteForTest(
			Fixture,
			Preflight,
			FVector::RightVector,
			ProductRouteStartTime + 1.0);
	TestTrue(TEXT("later attempt receives a fresh accepted identity"), Retry.IsAccepted());
	TestEqual(
		TEXT("retry uses sequence two"),
		Retry.ProductStart.Reservation.GetActivationSequence(),
		static_cast<uint64>(2));
	TestNotEqual(
		TEXT("rejected and retried attempts never share identity"),
		Rejected.ProductStart.Reservation.GetActivationId(),
		Retry.ProductStart.Reservation.GetActivationId());
	TestEqual(
		TEXT("retry advances to sequence three"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteTerminalReuseTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.TerminalReuse",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteTerminalReuseTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult First =
		RouteForTest(Fixture, Preflight);
	const Fdemo_mapShanmenSpiritEvasionCommandResult Cancelled =
		Fdemo_mapShanmenSpiritEvasionCommandRouter::TryRoute(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			Fdemo_mapShanmenSpiritEvasionCommand::MakeCancel());
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult Second =
		RouteForTest(
			Fixture,
			Preflight,
			FVector::RightVector,
			ProductRouteStartTime + 1.0);
	TestTrue(TEXT("first route starts"), First.IsAccepted());
	TestTrue(TEXT("typed cancel reaches terminal state"), Cancelled.IsAccepted());
	TestTrue(TEXT("terminal component accepts a new route"), Second.IsAccepted());
	TestEqual(
		TEXT("both routes use the one canonical config"),
		First.ProductStart.Config.GetConfigId(),
		Second.ProductStart.Config.GetConfigId());
	TestNotEqual(
		TEXT("terminal reuse still receives a distinct activation"),
		First.CommandRoute.ActivationId,
		Second.CommandRoute.ActivationId);
	TestEqual(
		TEXT("two accepted starts consume two sequences"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionProductRouteActionConflictTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductRoute.ActionConflict",
	ProductRouteFlags)

bool Fdemo_mapSpiritEvasionProductRouteActionConflictTest::RunTest(
	const FString& Parameters)
{
	FProductRouteFixture Fixture;
	FProductRoutePreflightPort Preflight;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenSpiritEvasionProductRouteResult Rejected =
		Fdemo_mapShanmenSpiritEvasionProductRoute::TryRouteAtForAutomation(
			Fixture.Component,
			Fixture.Coordinator,
			Fixture.Character,
			FVector::ForwardVector,
			ProductRouteStartTime,
			Preflight,
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
				Occupancy.bThrownWeaponInFlight = true;
				return Fdemo_mapShanmenPlayerActionGateResult::
					FromArbitration(
						Fixture.Coordinator.TryAuthorizePlayerAction(
							Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
							Occupancy));
			});
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestTrue(TEXT("conflict returns typed rejected gate"),
		Rejected.Status
			== Edemo_mapShanmenSpiritEvasionProductRouteStatus::ActionConflict
			&& Rejected.ActionGate.IsValid()
			&& !Rejected.ActionGate.IsAuthorized());
	TestTrue(TEXT("authorization runs once after product capture"),
		AuthorizationCount == 1 && Rejected.ProductStart.IsReady());
	TestTrue(TEXT("conflict does not dispatch movement or preflight"),
		Preflight.EvaluationCount == 0
			&& Fixture.Component->CanStart()
			&& !Rejected.CommandRoute.IsAccepted());
	return true;
}

#endif
