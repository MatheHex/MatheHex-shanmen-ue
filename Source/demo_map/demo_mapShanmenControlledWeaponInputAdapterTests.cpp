#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponInputAdapter.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenControlledWeaponWorldLifecycle.h"

namespace
{
	constexpr EAutomationTestFlags ControlledWeaponInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid InputRunId(0xD3770001, 0, 0, 1);
	const FGuid InputItemId(0xD3770010, 0, 0, 1);
	const FGuid InputOwnerId(0xD3770020, 0, 0, 1);
	const FGuid LaunchIntentId(0xD3770100, 0, 0, 1);
	const FGuid RedirectIntentId(0xD3770101, 0, 0, 1);
	const FGuid RecallIntentId(0xD3770102, 0, 0, 1);

	Fdemo_mapShanmenControlledWeaponRunCommandResult MakeRejectedRoute()
	{
		Fdemo_mapShanmenControlledWeaponRunCommandResult Result;
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunCommandStatus::CommandRejected;
		Result.Diagnostic = TEXT("Synthetic command rejection.");
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponInputReadModel MakeReadModel(
		EShanmenControlledWeaponState State)
	{
		Fdemo_mapShanmenControlledWeaponInputReadModel Result;
		check(Fdemo_mapShanmenControlledWeaponInputReadModel::TryCapture(
			InputRunId, InputItemId, State, Result));
		return Result;
	}

	struct FControlledWeaponInputFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		AActor* Weapon = nullptr;
		UBoxComponent* WeaponRoot = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenControlledWeaponRunHost Host;
		Fdemo_mapShanmenControlledWeaponRunCommandRouter Router;
		FString Diagnostic;
		bool bReady = false;

		FControlledWeaponInputFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P217PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P217PlayerHealth"))
				: nullptr;
			Weapon = NewObject<AActor>(GetTransientPackage());
			WeaponRoot = Weapon
				? NewObject<UBoxComponent>(Weapon, TEXT("P217WeaponRoot"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}
			if (Weapon && WeaponRoot)
			{
				Weapon->SetRootComponent(WeaponRoot);
			}
			bReady = Pawn && PlayerRoot && PlayerHealth && Weapon && WeaponRoot
				&& Coordinator.TryBeginRun(
					InputRunId, Pawn, PlayerHealth, Diagnostic)
				&& Attach();
		}

		bool Attach()
		{
			FShanmenContentStamp Content;
			Content.Version = TEXT("Shanmen.0.0.10.P21.7");
			Content.Digest = TEXT("P21.7.ControlledWeaponInput.v1");

			Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
			Prepared.Status =
				Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
			Prepared.Evidence.CorrelationId =
				FGuid(0xD3770200, 0, 0, 1);
			Prepared.Evidence.ActiveRunId = InputRunId;
			Prepared.Evidence.OwnerId = InputOwnerId;
			Prepared.Evidence.ItemInstanceId = InputItemId;
			Prepared.Evidence.ItemDefinitionId =
				TEXT("Item.Test.FlyingSword.P21.7");
			Prepared.Evidence.DeploymentReservationId =
				FGuid(0xD3770300, 0, 0, 1);
			Prepared.Evidence.AuthorityRevision = 15;
			Prepared.Evidence.ItemRevision = 9;
			Prepared.Evidence.Content = Content;

			FShanmenCombatActionCapture ActionCapture;
			ActionCapture.RunId = InputRunId;
			ActionCapture.OwnerId = InputOwnerId;
			ActionCapture.SourceEntityId = Coordinator.GetPlayerEntityId();
			ActionCapture.SourceItemInstanceId = InputItemId;
			ActionCapture.ActionDefinitionId =
				FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
			ActionCapture.Content = Content;
			ActionCapture.SourceTags.AddTag(
				FShanmenCombatNativeTags::SourcePlayer());
			ActionCapture.SourceTags.AddTag(
				FShanmenItemNativeTags::ItemWeaponFlyingSword());
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					InputRunId,
					ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId,
					1);
			if (!FShanmenCombatActionSnapshot::TryCapture(
				ActionCapture, Prepared.Action))
			{
				return false;
			}

			FShanmenControlledWeaponDefinitionCapture DefinitionCapture;
			DefinitionCapture.ActionDefinitionId =
				FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
			DefinitionCapture.DetectorId =
				TEXT("Detector.ControlledWeapon.P21.7.Input");
			DefinitionCapture.FormulaId =
				TEXT("Formula.ControlledWeapon.P21.7.Input");
			DefinitionCapture.BaseDamage = 0.5f;
			DefinitionCapture.ControlPowerCoefficient = 0.01f;
			DefinitionCapture.DamageTags.AddTag(
				FShanmenCombatNativeTags::DamagePhysicalSlash());
			DefinitionCapture.RequiredTargetTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
			if (!FShanmenControlledWeaponDefinition::TryCapture(
					DefinitionCapture, Prepared.Definition)
				|| !FShanmenControlledWeaponOffenseSnapshot::TryCapture(
					40.0f, Prepared.Offense)
				|| !FShanmenControlledWeaponExecution::TryCreate(
					Prepared.Action,
					Prepared.Definition,
					Prepared.Offense,
					Prepared.Execution))
			{
				return false;
			}

			Fdemo_mapShanmenControlledWeaponMotionCapture Motion;
			Motion.DirectedSpeed = 400.0f;
			Motion.OrbitCenterOffset = FVector(0.0, 0.0, 50.0);
			Motion.OrbitPlaneNormal = FVector::UpVector;
			Motion.OrbitReferenceAxis = FVector::ForwardVector;
			Motion.OrbitRadius = 100.0f;
			Motion.OrbitAngularSpeedRadiansPerSecond = UE_PI * 0.5f;
			Motion.InitialOrbitPhaseRadians = 0.0f;
			Motion.MaximumStepSeconds = 0.5f;
			return Prepared.IsPrepared()
				&& Host.TryAttach(
					Prepared,
					Coordinator,
					Pawn,
					Weapon,
					WeaponRoot,
					Motion).IsAttached();
		}

		Fdemo_mapShanmenControlledWeaponInputResult Route(
			const FGuid& IntentId,
			EShanmenControlledWeaponState State,
			TFunctionRef<FVector()> SampleDirection)
		{
			return Fdemo_mapShanmenControlledWeaponInputAdapter::RouteToggleInput(
				true,
				true,
				[State](Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
				{
					return Fdemo_mapShanmenControlledWeaponInputReadModel::TryCapture(
						InputRunId, InputItemId, State, Out);
				},
				[IntentId]() { return IntentId; },
				SampleDirection,
				[this](
					const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Intent)
				{
					return Router.TryRoute(Host, Coordinator, Intent);
				});
		}

		Fdemo_mapShanmenControlledWeaponInputResult RouteRedirect(
			const FGuid& IntentId,
			EShanmenControlledWeaponState State,
			TFunctionRef<FVector()> SampleDirection)
		{
			return Fdemo_mapShanmenControlledWeaponInputAdapter::
				RouteRedirectInput(
					true,
					true,
					[State](
						Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
					{
						return Fdemo_mapShanmenControlledWeaponInputReadModel::
							TryCapture(InputRunId, InputItemId, State, Out);
					},
					[IntentId]() { return IntentId; },
					SampleDirection,
					[this](
						const Fdemo_mapShanmenControlledWeaponRunCommandIntent&
							Intent)
					{
						return Router.TryRoute(Host, Coordinator, Intent);
					});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponInputGameplayFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponInputAdapter.GameplayFence",
	ControlledWeaponInputFlags)

bool Fdemo_mapControlledWeaponInputGameplayFenceTest::RunTest(const FString&)
{
	int32 ReadCount = 0;
	int32 IdCount = 0;
	int32 DirectionCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenControlledWeaponInputResult Result =
		Fdemo_mapShanmenControlledWeaponInputAdapter::RouteToggleInput(
			false,
			true,
			[&ReadCount](Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
			{
				++ReadCount;
				Out = MakeReadModel(EShanmenControlledWeaponState::Orbiting);
				return true;
			},
			[&IdCount]() { ++IdCount; return LaunchIntentId; },
			[&DirectionCount]()
			{
				++DirectionCount;
				return FVector::ForwardVector;
			},
			[&RouteCount](
				const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)
			{
				++RouteCount;
				return MakeRejectedRoute();
			});
	TestTrue(TEXT("gameplay lock fails before every external dependency"),
		Result.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::GameplayBlocked
		&& ReadCount == 0 && IdCount == 0 && DirectionCount == 0
		&& RouteCount == 0
		&& !Result.bCanonicalReadInvoked
		&& !Result.bProductRouteInvoked);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponInputRouteFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponInputAdapter.RouteFence",
	ControlledWeaponInputFlags)

bool Fdemo_mapControlledWeaponInputRouteFenceTest::RunTest(const FString&)
{
	int32 ReadCount = 0;
	const Fdemo_mapShanmenControlledWeaponInputResult Result =
		Fdemo_mapShanmenControlledWeaponInputAdapter::RouteToggleInput(
			true,
			false,
			[&ReadCount](Fdemo_mapShanmenControlledWeaponInputReadModel&)
			{
				++ReadCount;
				return false;
			},
			[]() { return LaunchIntentId; },
			[]() { return FVector::ForwardVector; },
			[](const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)
			{
				return MakeRejectedRoute();
			});
	TestTrue(TEXT("missing route fails before canonical state is observed"),
		Result.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::ProductRouteUnavailable
		&& ReadCount == 0
		&& !Result.bCanonicalReadInvoked);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponInputIdentityFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponInputAdapter.IdentityFences",
	ControlledWeaponInputFlags)

bool Fdemo_mapControlledWeaponInputIdentityFenceTest::RunTest(const FString&)
{
	Fdemo_mapShanmenControlledWeaponInputReadModel Reused =
		MakeReadModel(EShanmenControlledWeaponState::Orbiting);
	Fdemo_mapShanmenControlledWeaponWorldLifecycle EmptyLifecycle;
	Fdemo_mapShanmenControlledWeaponRunHost EmptyHost;
	TestTrue(TEXT("empty canonical owners fail and clear reused output"),
		!Fdemo_mapShanmenControlledWeaponInputAdapter::TryReadCanonical(
			EmptyLifecycle, EmptyHost, Reused)
		&& !Reused.IsValid());

	int32 DirectionCount = 0;
	int32 RouteCount = 0;
	const Fdemo_mapShanmenControlledWeaponInputResult InvalidId =
		Fdemo_mapShanmenControlledWeaponInputAdapter::RouteToggleInput(
			true,
			true,
			[](Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
			{
				Out = MakeReadModel(EShanmenControlledWeaponState::Orbiting);
				return true;
			},
			[]() { return FGuid(); },
			[&DirectionCount]()
			{
				++DirectionCount;
				return FVector::ForwardVector;
			},
			[&RouteCount](
				const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)
			{
				++RouteCount;
				return MakeRejectedRoute();
			});
	TestTrue(TEXT("invalid intent identity consumes neither aim nor route"),
		InvalidId.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::IntentIdInvalid
		&& InvalidId.bCanonicalReadInvoked
		&& InvalidId.bIntentIdCreated
		&& DirectionCount == 0
		&& RouteCount == 0);

	const Fdemo_mapShanmenControlledWeaponInputResult NoAim =
		Fdemo_mapShanmenControlledWeaponInputAdapter::RouteToggleInput(
			true,
			true,
			[](Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
			{
				Out = MakeReadModel(EShanmenControlledWeaponState::Orbiting);
				return true;
			},
			[]() { return LaunchIntentId; },
			[]() { return FVector::ZeroVector; },
			[&RouteCount](
				const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)
			{
				++RouteCount;
				return MakeRejectedRoute();
			});
	TestTrue(TEXT("invalid launch aim fails before product mutation"),
		NoAim.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::IntentCaptureRejected
		&& NoAim.bDirectionSampled
			&& !NoAim.bIntentCaptured
			&& RouteCount == 0);

	int32 RedirectIdCount = 0;
	int32 RedirectAimCount = 0;
	int32 RedirectRouteCount = 0;
	const Fdemo_mapShanmenControlledWeaponInputResult OrbitRedirect =
		Fdemo_mapShanmenControlledWeaponInputAdapter::RouteRedirectInput(
			true,
			true,
			[](Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
			{
				Out = MakeReadModel(
					EShanmenControlledWeaponState::Orbiting);
				return true;
			},
			[&RedirectIdCount]()
			{
				++RedirectIdCount;
				return RedirectIntentId;
			},
			[&RedirectAimCount]()
			{
				++RedirectAimCount;
				return FVector::RightVector;
			},
			[&RedirectRouteCount](
				const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)
			{
				++RedirectRouteCount;
				return MakeRejectedRoute();
			});
	TestTrue(TEXT("redirect before launch fails before identity, aim, and route"),
		OrbitRedirect.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::CommandUnavailable
		&& OrbitRedirect.bCanonicalReadInvoked
		&& RedirectIdCount == 0
		&& RedirectAimCount == 0
		&& RedirectRouteCount == 0);

	const Fdemo_mapShanmenControlledWeaponInputResult InvalidRedirectAim =
		Fdemo_mapShanmenControlledWeaponInputAdapter::RouteRedirectInput(
			true,
			true,
			[](Fdemo_mapShanmenControlledWeaponInputReadModel& Out)
			{
				Out = MakeReadModel(
					EShanmenControlledWeaponState::Directed);
				return true;
			},
			[]() { return RedirectIntentId; },
			[]() { return FVector::ZeroVector; },
			[&RedirectRouteCount](
				const Fdemo_mapShanmenControlledWeaponRunCommandIntent&)
			{
				++RedirectRouteCount;
				return MakeRejectedRoute();
			});
	TestTrue(TEXT("invalid redirect aim fails before product mutation"),
		InvalidRedirectAim.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::
				IntentCaptureRejected
		&& InvalidRedirectAim.bDirectionSampled
		&& !InvalidRedirectAim.bIntentCaptured
		&& RedirectRouteCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponInputLaunchRecallTest,
	"Shanmen.0_0_10.Product.ControlledWeaponInputAdapter.LaunchRecallExactItem",
	ControlledWeaponInputFlags)

bool Fdemo_mapControlledWeaponInputLaunchRecallTest::RunTest(const FString&)
{
	FControlledWeaponInputFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P21.7 input fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	int32 LaunchAimCount = 0;
	const FVector RawAim(3.0f, 4.0f, 7.0f);
	const Fdemo_mapShanmenControlledWeaponInputResult Launch = Fixture.Route(
		LaunchIntentId,
		EShanmenControlledWeaponState::Orbiting,
		[&LaunchAimCount, &RawAim]()
		{
			++LaunchAimCount;
			return RawAim;
		});
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Fixture.Host.FindController(InputItemId);
	TestTrue(TEXT("one press launches only the exact canonical item"),
		Launch.IsAccepted()
		&& LaunchAimCount == 1
		&& Launch.SampledDirection == RawAim
		&& Launch.Intent.GetTargetItemInstanceIds()
			== TArray<FGuid>({ InputItemId })
		&& Launch.Intent.GetDesiredDirection().Equals(RawAim.GetSafeNormal())
		&& Controller
		&& Controller->IsDirected()
		&& Fixture.Router.NumProcessedIntents() == 1);

	int32 RedirectAimCount = 0;
	const FVector RawRedirect(0.0f, -8.0f, 3.0f);
	const Fdemo_mapShanmenControlledWeaponInputResult Redirect =
		Fixture.RouteRedirect(
			RedirectIntentId,
			EShanmenControlledWeaponState::Directed,
			[&RedirectAimCount, &RawRedirect]()
			{
				++RedirectAimCount;
				return RawRedirect;
			});
	const Fdemo_mapShanmenControlledWeaponProductController*
		ControllerAfterRedirect = Fixture.Host.FindController(InputItemId);
	TestTrue(TEXT("separate redirect press retargets the same directed item"),
		Redirect.IsAccepted()
		&& RedirectAimCount == 1
		&& Redirect.SampledDirection == RawRedirect
		&& Redirect.Intent.GetKind()
			== EShanmenControlledWeaponCommandKind::Redirect
		&& Redirect.Intent.GetTargetItemInstanceIds()
			== TArray<FGuid>({ InputItemId })
		&& Redirect.Intent.GetDesiredDirection().Equals(
			RawRedirect.GetSafeNormal())
		&& Redirect.ProductRoute.Entries.Num() == 1
		&& Redirect.ProductRoute.Entries[0].Command.GetKind()
			== EShanmenControlledWeaponCommandKind::Redirect
		&& Redirect.ProductRoute.Entries[0].Command.GetDirectionAfter().Equals(
			RawRedirect.GetSafeNormal())
		&& ControllerAfterRedirect
		&& ControllerAfterRedirect->IsDirected()
		&& Fixture.Router.NumProcessedIntents() == 2);

	int32 RecallAimCount = 0;
	const Fdemo_mapShanmenControlledWeaponInputResult Recall = Fixture.Route(
		RecallIntentId,
		EShanmenControlledWeaponState::Directed,
		[&RecallAimCount]()
		{
			++RecallAimCount;
			return FVector::RightVector;
		});
	TestTrue(TEXT("next press recalls that same item without sampling aim"),
		Recall.IsAccepted()
		&& RecallAimCount == 0
		&& !Recall.bDirectionSampled
		&& Recall.Intent.GetKind()
			== EShanmenControlledWeaponCommandKind::Recall
		&& Recall.Intent.GetTargetItemInstanceIds()
			== TArray<FGuid>({ InputItemId })
		&& Fixture.Host.NumActive() == 0
		&& Fixture.Router.NumProcessedIntents() == 3);
	return true;
}

#endif
