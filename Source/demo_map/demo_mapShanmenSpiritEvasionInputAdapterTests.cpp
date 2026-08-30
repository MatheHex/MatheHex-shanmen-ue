#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSpiritEvasionInputAdapter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	const EAutomationTestFlags InputAdapterFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid InputAdapterRun(
		0xE0500001, 0xE0500002, 0xE0500003, 0xE0500004);
	constexpr double InputAdapterStartTime = 600.0;

	class FInputAdapterPreflightPort final
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

	struct FInputAdapterFixture
	{
		UWorld* World = nullptr;
		ACharacter* Character = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapShanmenSpiritEvasionComponent* Component = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FInputAdapterFixture()
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
			Health = Character
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Character,
					TEXT("SpiritEvasionInputAdapterHealth"))
				: nullptr;
			if (Character && Health)
			{
				Character->AddInstanceComponent(Health);
				Health->RegisterComponent();
			}
			const Fdemo_mapShanmenSpiritEvasionInstallationResult Installation =
				Fdemo_mapShanmenSpiritEvasionCommandRouter::EnsureInstalled(
					Character);
			Component = Installation.Component;
			bReady = Character
				&& Health
				&& Health->IsRegistered()
				&& Installation.IsSuccess()
				&& Coordinator.TryBeginRun(
					InputAdapterRun,
					Character,
					Health,
					Diagnostic);
		}

		~FInputAdapterFixture()
		{
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		Fdemo_mapShanmenSpiritEvasionProductRouteResult Route(
			const FVector& Direction,
			FInputAdapterPreflightPort& Preflight)
		{
			return Fdemo_mapShanmenSpiritEvasionProductRoute::
				TryRouteAtForAutomation(
					Component,
					Coordinator,
					Character,
					Direction,
					InputAdapterStartTime,
					Preflight);
		}
	};

	Fdemo_mapShanmenSpiritEvasionProductRouteResult MakeRejectedRoute()
	{
		Fdemo_mapShanmenSpiritEvasionProductRouteResult Result;
		Result.Status =
			Edemo_mapShanmenSpiritEvasionProductRouteStatus::ProductRejected;
		Result.Diagnostic = TEXT("Synthetic product rejection.");
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInputGameplayFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionInputAdapter.GameplayFence",
	InputAdapterFlags)

bool Fdemo_mapSpiritEvasionInputGameplayFenceTest::RunTest(
	const FString& Parameters)
{
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
			false,
			true,
			[&SampleCount]()
			{
				++SampleCount;
				return FVector::ForwardVector;
			},
			[&RouteCount](const FVector&)
			{
				++RouteCount;
				return MakeRejectedRoute();
			});
	TestEqual(
		TEXT("locked gameplay is classified"),
		Result.Status,
		Edemo_mapShanmenSpiritEvasionInputStatus::GameplayBlocked);
	TestFalse(TEXT("locked gameplay does not sample"), Result.bDirectionSampled);
	TestFalse(TEXT("locked gameplay does not route"), Result.bProductRouteInvoked);
	TestEqual(TEXT("sampler stays untouched"), SampleCount, 0);
	TestEqual(TEXT("product route stays untouched"), RouteCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInputRouteFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionInputAdapter.RouteFence",
	InputAdapterFlags)

bool Fdemo_mapSpiritEvasionInputRouteFenceTest::RunTest(
	const FString& Parameters)
{
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
			true,
			false,
			[&SampleCount]()
			{
				++SampleCount;
				return FVector::ForwardVector;
			},
			[&RouteCount](const FVector&)
			{
				++RouteCount;
				return MakeRejectedRoute();
			});
	TestEqual(
		TEXT("missing GameMode route is classified"),
		Result.Status,
		Edemo_mapShanmenSpiritEvasionInputStatus::ProductRouteUnavailable);
	TestEqual(TEXT("missing route prevents sampling"), SampleCount, 0);
	TestEqual(TEXT("missing route prevents invocation"), RouteCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInputSingleSampleTest,
	"Shanmen.0_0_10.Product.SpiritEvasionInputAdapter.SingleSample",
	InputAdapterFlags)

bool Fdemo_mapSpiritEvasionInputSingleSampleTest::RunTest(
	const FString& Parameters)
{
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	FVector RoutedDirection = FVector::ZeroVector;
	const FVector RawDirection(3.0f, 4.0f, 7.0f);
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
			true,
			true,
			[&SampleCount, &RawDirection]()
			{
				++SampleCount;
				return RawDirection;
			},
			[&RouteCount, &RoutedDirection](const FVector& Direction)
			{
				++RouteCount;
				RoutedDirection = Direction;
				return MakeRejectedRoute();
			});
	TestEqual(TEXT("direction is sampled exactly once"), SampleCount, 1);
	TestEqual(TEXT("product route is invoked exactly once"), RouteCount, 1);
	TestTrue(TEXT("sample proof is retained"), Result.bDirectionSampled);
	TestTrue(TEXT("route proof is retained"), Result.bProductRouteInvoked);
	TestEqual(TEXT("adapter preserves the raw sample"), Result.SampledDirection, RawDirection);
	TestEqual(TEXT("route receives the exact raw sample"), RoutedDirection, RawDirection);
	TestEqual(
		TEXT("rejected route is not retried or accepted"),
		Result.Status,
		Edemo_mapShanmenSpiritEvasionInputStatus::ProductRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInputInvalidIntentTest,
	"Shanmen.0_0_10.Product.SpiritEvasionInputAdapter.InvalidIntent",
	InputAdapterFlags)

bool Fdemo_mapSpiritEvasionInputInvalidIntentTest::RunTest(
	const FString& Parameters)
{
	FInputAdapterFixture Fixture;
	FInputAdapterPreflightPort Preflight;
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
			true,
			true,
			[&SampleCount]()
			{
				++SampleCount;
				return FVector::ZeroVector;
			},
			[&Fixture, &Preflight, &RouteCount](const FVector& Direction)
			{
				++RouteCount;
				return Fixture.Route(Direction, Preflight);
			});
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestEqual(TEXT("invalid input samples once"), SampleCount, 1);
	TestEqual(TEXT("invalid input delegates once"), RouteCount, 1);
	TestEqual(
		TEXT("product authority classifies invalid direction"),
		Result.ProductRoute.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::ProductRejected);
	TestEqual(
		TEXT("invalid input does not consume activation sequence"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));
	TestEqual(TEXT("invalid input performs no preflight"), Preflight.EvaluationCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInputAppliedTest,
	"Shanmen.0_0_10.Product.SpiritEvasionInputAdapter.AppliedProof",
	InputAdapterFlags)

bool Fdemo_mapSpiritEvasionInputAppliedTest::RunTest(
	const FString& Parameters)
{
	FInputAdapterFixture Fixture;
	FInputAdapterPreflightPort Preflight;
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
			true,
			true,
			[&SampleCount]()
			{
				++SampleCount;
				return FVector(3.0f, 4.0f, 7.0f);
			},
			[&Fixture, &Preflight, &RouteCount](const FVector& Direction)
			{
				++RouteCount;
				return Fixture.Route(Direction, Preflight);
			});
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestTrue(TEXT("input proof is accepted"), Result.IsAccepted());
	TestEqual(TEXT("accepted input samples once"), SampleCount, 1);
	TestEqual(TEXT("accepted input routes once"), RouteCount, 1);
	TestEqual(
		TEXT("product authority owns planar normalization"),
		Result.ProductRoute.ProductStart.Command.GetCandidateDirection(),
		FVector(0.6f, 0.8f, 0.0f));
	TestEqual(TEXT("accepted input preflights once"), Preflight.EvaluationCount, 1);
	TestEqual(
		TEXT("accepted input consumes one activation"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInputRejectedExecutionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionInputAdapter.RejectedExecution",
	InputAdapterFlags)

bool Fdemo_mapSpiritEvasionInputRejectedExecutionTest::RunTest(
	const FString& Parameters)
{
	FInputAdapterFixture Fixture;
	FInputAdapterPreflightPort Preflight;
	Preflight.bAllow = false;
	int32 SampleCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenSpiritEvasionInputResult Result =
		Fdemo_mapShanmenSpiritEvasionInputAdapter::RouteStartInput(
			true,
			true,
			[&SampleCount]()
			{
				++SampleCount;
				return FVector::ForwardVector;
			},
			[&Fixture, &Preflight, &RouteCount](const FVector& Direction)
			{
				++RouteCount;
				return Fixture.Route(Direction, Preflight);
			});
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestFalse(TEXT("execution rejection is not accepted"), Result.IsAccepted());
	TestEqual(TEXT("rejected execution samples once"), SampleCount, 1);
	TestEqual(TEXT("rejected execution routes once"), RouteCount, 1);
	TestEqual(
		TEXT("execution rejection remains a product rejection"),
		Result.Status,
		Edemo_mapShanmenSpiritEvasionInputStatus::ProductRejected);
	TestEqual(
		TEXT("route retains the downstream rejection"),
		Result.ProductRoute.Status,
		Edemo_mapShanmenSpiritEvasionProductRouteStatus::CommandRouteRejected);
	TestEqual(
		TEXT("published reservation is consumed without adapter retry"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(2));
	TestEqual(TEXT("rejected execution preflights once"), Preflight.EvaluationCount, 1);
	return true;
}

#endif
