#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenWeaponGuardProductHost.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid HostRunId(
		0xDF100001, 0xDF100002, 0xDF100003, 0xDF100004);
	const FGuid HostOwnerId(
		0xDF110001, 0xDF110002, 0xDF110003, 0xDF110004);
	const FGuid HostDefenderId(
		0xDF120001, 0xDF120002, 0xDF120003, 0xDF120004);
	const FGuid HostWeaponId(
		0xDF130001, 0xDF130002, 0xDF130003, 0xDF130004);
	const FGuid HostTimelineId(
		0xDF140001, 0xDF140002, 0xDF140003, 0xDF140004);
	const FGuid HostThreatId(
		0xDF150001, 0xDF150002, 0xDF150003, 0xDF150004);
	const FGuid HostArmorLayerId(
		0xDF160001, 0xDF160002, 0xDF160003, 0xDF160004);

	FName ThreatActionDefinitionId()
	{
		return TEXT("Combat.Action.Test.P11_5Threat");
	}

	FShanmenCombatActionSnapshot MakeGuardAction(
		uint64 ActivationSequence = 1150)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = HostRunId;
		Capture.OwnerId = HostOwnerId;
		Capture.SourceEntityId = HostDefenderId;
		Capture.SourceItemInstanceId = HostWeaponId;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.5");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.5-GUARD");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenCombatActionSnapshot MakeForeignAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = HostRunId;
		Capture.OwnerId = HostOwnerId;
		Capture.SourceEntityId = HostDefenderId;
		Capture.SourceItemInstanceId = HostWeaponId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.NotGuard");
		Capture.Content.Version = TEXT("0.0.10.P11.5");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.5-NOT-GUARD");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1151);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenCombatActionSnapshot MakeThreatAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = HostRunId;
		Capture.OwnerId = HostThreatId;
		Capture.SourceEntityId = HostThreatId;
		Capture.ActionDefinitionId = ThreatActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.5");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.5-THREAT");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1152);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenWeaponGuardDefinition MakeDefinition(float Fraction = 0.25f)
	{
		FShanmenWeaponGuardDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Sword.WeaponGuard01");
		Capture.GuardFraction = Fraction;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(Capture, Definition));
		return Definition;
	}

	FShanmenHitCandidate MakeCandidate(
		const FGuid& SourceEntityId = HostThreatId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			HostRunId,
			SourceEntityId,
			ThreatActionDefinitionId(),
			1152);
		Candidate.SourceEntityId = SourceEntityId;
		Candidate.TargetEntityId = HostDefenderId;
		Candidate.DetectorId = TEXT("Detector.Test.P11_5Threat");
		Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Candidate.HitLocation = FVector(100.0, 0.0, 5.0);
		Candidate.HitNormal = FVector::BackwardVector;
		Candidate.HitOrdinal = 0;
		check(Candidate.IsValid());
		return Candidate;
	}

	FShanmenDefenseSnapshot MakeBaseDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenDefenseLayer& Armor = Defense.Layers.AddDefaulted_GetRef();
		Armor.LayerId = HostArmorLayerId;
		Armor.RuleId = TEXT("Defense.Test.P11_5Armor");
		Armor.Operation = EShanmenDefenseOperation::AbsorbPoints;
		Armor.Order = FShanmenDefenseOrder::Resistance;
		Armor.Magnitude = 10.0f;
		Armor.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseArmor());
		Armor.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		check(Defense.IsValid());
		return Defense;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseSnapshot& Defense,
		float RawDamage = 100.0f)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeThreatAction();
		Request.Candidate = MakeCandidate();
		Request.Damage.FormulaId = TEXT("Damage.Test.P11_5Physical");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 8;
		Request.Defense = Defense;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			HostRunId,
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	struct FGuardHostWorld
	{
		UWorld* World = nullptr;

		bool Start()
		{
			if (!GEngine)
			{
				return false;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return false;
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
			return true;
		}

		AActor* Spawn(const FVector& Location) const
		{
			if (!World)
			{
				return nullptr;
			}
			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(
				AActor::StaticClass(), FTransform::Identity, Parameters);
			if (!Actor)
			{
				return nullptr;
			}
			USceneComponent* Root = NewObject<USceneComponent>(
				Actor, NAME_None, RF_Transient);
			if (!Root)
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			Actor->SetRootComponent(Root);
			Root->SetWorldLocation(Location);
			return Actor;
		}

		void Stop()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		~FGuardHostWorld()
		{
			Stop();
		}
	};

	Fdemo_mapShanmenWeaponGuardHostStartResult StartHost(
		Fdemo_mapShanmenWeaponGuardProductHost& OutHost,
		double MinimumFacingDot = 0.5,
		uint64 ActivationSequence = 1150)
	{
		return Fdemo_mapShanmenWeaponGuardProductHost::TryStart(
			MakeGuardAction(ActivationSequence),
			MakeDefinition(),
			HostTimelineId,
			10,
			15,
			TEXT("Defense.Sword.PerfectGuard01"),
			TEXT("Defense.Sword.WeaponGuardArc01"),
			MinimumFacingDot,
			OutHost);
	}

	struct FGuardHostFixture
	{
		FGuardHostWorld World;
		FShanmenWorldEntityRegistry Registry;
		Fdemo_mapShanmenWeaponGuardProductHost Host;
		AActor* Defender = nullptr;
		AActor* Threat = nullptr;

		bool Start(double MinimumFacingDot = 0.5)
		{
			if (!World.Start()
				|| !Registry.TryBeginRun(HostRunId)
				|| !StartHost(Host, MinimumFacingDot).IsSuccess())
			{
				return false;
			}
			Defender = World.Spawn(FVector::ZeroVector);
			Threat = World.Spawn(FVector(100.0, 0.0, 0.0));
			return Defender
				&& Threat
				&& Registry.BindObject(
					HostRunId, Defender, HostDefenderId)
					== EShanmenWorldBindingResult::Bound
				&& Registry.BindObject(HostRunId, Threat, HostThreatId)
					== EShanmenWorldBindingResult::Bound;
		}

		Fdemo_mapShanmenWeaponGuardHostDefenseResult Compose(
			int64 ObservedTick,
			const FShanmenDefenseSnapshot& BaseDefense,
			const FShanmenHitCandidate& Candidate = MakeCandidate(),
			UWorld* ExplicitWorld = nullptr)
		{
			return Host.TryComposeDefense(
				ExplicitWorld ? ExplicitWorld : World.World,
				Registry,
				Defender,
				Threat,
				ObservedTick,
				Candidate,
				BaseDefense);
		}

		Fdemo_mapShanmenWeaponGuardHostDefenseResult Compose(
			int64 ObservedTick)
		{
			return Compose(ObservedTick, MakeBaseDefense());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardHostPerfectReplayTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductHost.PerfectReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardHostPerfectReplayTest::RunTest(const FString&)
{
	FGuardHostFixture Fixture;
	if (!Fixture.Start())
	{
		AddError(TEXT("Could not start P11.5 perfect host fixture."));
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult First =
		Fixture.Compose(10);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Replay =
		Fixture.Compose(10);
	const FShanmenImpactResult Impact = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(First.Composition.Defense));

	TestTrue(TEXT("Host starts active with one exact committed window"),
		Fixture.Host.IsValid()
			&& Fixture.Host.IsActive()
			&& Fixture.Host.GetWindow().IsActiveFor(
				Fixture.Host.GetActionRuntime())
			&& Fixture.Host.GetLastObservedTick() == 10);
	TestTrue(TEXT("Perfect defense is bound to the host receipt"),
		First.IsSuccess()
			&& First.HasGuardLayer()
			&& First.HostId == Fixture.Host.GetHostId()
			&& First.Composition.Defense.Layers.Num() == 2
			&& First.Composition.Defense.Layers.Last().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard()));
	TestTrue(TEXT("Exact same impact observation replays deterministically"),
		Replay.IsSuccess()
			&& Replay.ReceiptId == First.ReceiptId
			&& Replay.Composition.ReceiptId == First.Composition.ReceiptId);
	TestTrue(TEXT("Host output enters the existing pure resolver"),
		Impact.IsConserved()
			&& Impact.Outcome == EShanmenDefenseOutcome::PerfectGuarded
			&& FMath::IsNearlyZero(Impact.FinalDamage));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardHostOrdinaryOutsideTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductHost.OrdinaryOutside",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardHostOrdinaryOutsideTest::RunTest(const FString&)
{
	FGuardHostFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Ordinary =
		Fixture.Compose(15);
	const FShanmenImpactResult OrdinaryImpact = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Ordinary.Composition.Defense));
	Fixture.Threat->SetActorLocation(
		FVector(-100.0, 0.0, 0.0),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Outside =
		Fixture.Compose(16);
	const FShanmenImpactResult OutsideImpact = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Outside.Composition.Defense));

	TestTrue(TEXT("Ordinary guard remains before existing armor"),
		Ordinary.HasGuardLayer()
			&& Ordinary.Composition.Defense.Layers.Last().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard())
			&& FMath::IsNearlyEqual(OrdinaryImpact.PreventedDamage, 35.0f)
			&& FMath::IsNearlyEqual(OrdinaryImpact.FinalDamage, 65.0f));
	TestTrue(TEXT("Outside arc is successful but preserves base defense"),
		Outside.IsSuccess()
			&& !Outside.HasGuardLayer()
			&& Outside.Status
				== Edemo_mapShanmenWeaponGuardHostDefenseStatus::
				ComposedOutsideArc
			&& Outside.Composition.Defense.Layers.Num() == 1
			&& FMath::IsNearlyEqual(OutsideImpact.FinalDamage, 90.0f)
			&& Fixture.Host.GetLastObservedTick() == 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardHostMonotonicAuthorityTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductHost.MonotonicAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardHostMonotonicAuthorityTest::RunTest(const FString&)
{
	FGuardHostFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult First =
		Fixture.Compose(12);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Stale =
		Fixture.Compose(11);
	const FGuid ForeignThreat(
		0xDF170001, 0xDF170002, 0xDF170003, 0xDF170004);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Foreign =
		Fixture.Compose(13, MakeBaseDefense(), MakeCandidate(ForeignThreat));
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Recovered =
		Fixture.Compose(13);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult NullWorld =
		Fixture.Host.TryComposeDefense(
			nullptr,
			Fixture.Registry,
			Fixture.Defender,
			Fixture.Threat,
			14,
			MakeCandidate(),
			MakeBaseDefense());

	TestTrue(TEXT("First monotonic observation is accepted"),
		First.IsSuccess());
	TestEqual(TEXT("Older tick is rejected before P11.4"),
		Stale.Error,
		Edemo_mapShanmenWeaponGuardHostDefenseError::
		NonMonotonicObservation);
	TestTrue(TEXT("Composition failure preserves nested authority evidence"),
		Foreign.Error
			== Edemo_mapShanmenWeaponGuardHostDefenseError::
			CompositionRejected
			&& Foreign.Composition.Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::
				WorldEvaluationRejected
			&& Foreign.Composition.WorldEvaluation.Status
				== Edemo_mapShanmenWeaponGuardWorldStatus::
				ThreatIdentityMismatch);
	TestTrue(TEXT("Rejected samples do not advance the monotonic fence"),
		Recovered.IsSuccess()
			&& Fixture.Host.GetLastObservedTick() == 13);
	TestTrue(TEXT("Null World remains a nested P11.3 rejection"),
		NullWorld.Error
			== Edemo_mapShanmenWeaponGuardHostDefenseError::
			CompositionRejected
			&& NullWorld.Composition.WorldEvaluation.Status
				== Edemo_mapShanmenWeaponGuardWorldStatus::WorldInvalid
			&& Fixture.Host.GetLastObservedTick() == 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardHostLifecycleTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductHost.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardHostLifecycleTest::RunTest(const FString&)
{
	FGuardHostFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardHostTransitionResult Recovery =
		Fixture.Host.TryEnterRecovery();
	const bool bEnteredRecovery = Fixture.Host.IsRecovery();
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Closed =
		Fixture.Compose(10);
	const Fdemo_mapShanmenWeaponGuardHostTransitionResult Completed =
		Fixture.Host.TryComplete();
	const Fdemo_mapShanmenWeaponGuardHostTransitionResult CompletionReplay =
		Fixture.Host.TryComplete();

	TestTrue(TEXT("Active host enters Recovery through action authority"),
		Recovery.IsSuccess()
			&& bEnteredRecovery);
	TestEqual(TEXT("Recovery closes per-impact composition"),
		Closed.Error,
		Edemo_mapShanmenWeaponGuardHostDefenseError::HostNotActive);
	TestTrue(TEXT("Completed transition is terminal and idempotent"),
		Completed.Status
			== Edemo_mapShanmenWeaponGuardHostTransitionStatus::Completed
			&& CompletionReplay.IsSuccess()
			&& CompletionReplay.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::
				AlreadyTerminal);

	Fdemo_mapShanmenWeaponGuardProductHost InterruptedHost;
	check(StartHost(InterruptedHost).IsSuccess());
	const Fdemo_mapShanmenWeaponGuardHostTransitionResult Interrupted =
		InterruptedHost.TryInterrupt();
	const Fdemo_mapShanmenWeaponGuardHostTransitionResult InterruptReplay =
		InterruptedHost.TryInterrupt();
	TestTrue(TEXT("Committed guard terminates only as interruption"),
		Interrupted.IsSuccess()
			&& Interrupted.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::Interrupted
			&& InterruptedHost.IsTerminal()
			&& InterruptReplay.Status
				== Edemo_mapShanmenWeaponGuardHostTransitionStatus::
				AlreadyTerminal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardHostStartFencesTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductHost.StartFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardHostStartFencesTest::RunTest(const FString&)
{
	Fdemo_mapShanmenWeaponGuardProductHost First;
	Fdemo_mapShanmenWeaponGuardProductHost Replay;
	Fdemo_mapShanmenWeaponGuardProductHost DifferentArc;
	const Fdemo_mapShanmenWeaponGuardHostStartResult FirstStart =
		StartHost(First);
	const Fdemo_mapShanmenWeaponGuardHostStartResult ReplayStart =
		StartHost(Replay);
	const Fdemo_mapShanmenWeaponGuardHostStartResult DifferentStart =
		StartHost(DifferentArc, 0.75);

	Fdemo_mapShanmenWeaponGuardProductHost Rejected;
	const Fdemo_mapShanmenWeaponGuardHostStartResult InvalidTimeline =
		Fdemo_mapShanmenWeaponGuardProductHost::TryStart(
			MakeGuardAction(),
			MakeDefinition(),
			FGuid(),
			10,
			15,
			TEXT("Defense.Sword.PerfectGuard01"),
			TEXT("Defense.Sword.WeaponGuardArc01"),
			0.5,
			Rejected);
	const Fdemo_mapShanmenWeaponGuardHostStartResult InvalidWindow =
		Fdemo_mapShanmenWeaponGuardProductHost::TryStart(
			MakeGuardAction(),
			MakeDefinition(),
			HostTimelineId,
			10,
			10,
			TEXT("Defense.Sword.PerfectGuard01"),
			TEXT("Defense.Sword.WeaponGuardArc01"),
			0.5,
			Rejected);
	const Fdemo_mapShanmenWeaponGuardHostStartResult ForeignAction =
		Fdemo_mapShanmenWeaponGuardProductHost::TryStart(
			MakeForeignAction(),
			MakeDefinition(),
			HostTimelineId,
			10,
			15,
			TEXT("Defense.Sword.PerfectGuard01"),
			TEXT("Defense.Sword.WeaponGuardArc01"),
			0.5,
			Rejected);

	TestTrue(TEXT("Exact authored inputs replay the same host identity"),
		FirstStart.IsSuccess()
			&& ReplayStart.IsSuccess()
			&& First.GetHostId() == Replay.GetHostId());
	TestNotEqual(TEXT("Authored arc policy participates in host identity"),
		First.GetHostId(), DifferentArc.GetHostId());
	TestTrue(TEXT("Invalid timeline and interval fail before publication"),
		InvalidTimeline.Error
				== Edemo_mapShanmenWeaponGuardHostStartError::InvalidInput
			&& InvalidWindow.Error
				== Edemo_mapShanmenWeaponGuardHostStartError::InvalidInput
			&& !Rejected.IsValid());
	TestEqual(TEXT("Non-guard action cannot open the product host"),
		ForeignAction.Error,
		Edemo_mapShanmenWeaponGuardHostStartError::InvalidInput);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardHostConflictIsolationTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductHost.ConflictIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardHostConflictIsolationTest::RunTest(const FString&)
{
	FGuardHostFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult First =
		Fixture.Compose(10, Base);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Conflict =
		Fixture.Compose(10, First.Composition.Defense);
	const Fdemo_mapShanmenWeaponGuardHostDefenseResult Recovery =
		Fixture.Compose(11, Base);

	TestEqual(TEXT("Caller-owned base snapshot remains untouched"),
		Base.Layers.Num(), 1);
	TestTrue(TEXT("Re-appending the same selected layer fails closed"),
		Conflict.Error
			== Edemo_mapShanmenWeaponGuardHostDefenseError::
			CompositionRejected
			&& Conflict.Composition.Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::LayerConflict
			&& !Conflict.ReceiptId.IsValid());
	TestTrue(TEXT("Conflict does not consume the observation tick"),
		Recovery.IsSuccess()
			&& Fixture.Host.GetLastObservedTick() == 11);
	return true;
}

#endif
