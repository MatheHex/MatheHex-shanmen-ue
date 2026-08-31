#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenWeaponGuardDefenseCoordinator.h"

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
	const FGuid GuardDefenseRunId(
		0xDE200001, 0xDE200002, 0xDE200003, 0xDE200004);
	const FGuid GuardDefenseOwnerId(
		0xDE210001, 0xDE210002, 0xDE210003, 0xDE210004);
	const FGuid GuardDefenseDefenderId(
		0xDE220001, 0xDE220002, 0xDE220003, 0xDE220004);
	const FGuid GuardDefenseWeaponId(
		0xDE230001, 0xDE230002, 0xDE230003, 0xDE230004);
	const FGuid GuardDefenseTimelineId(
		0xDE240001, 0xDE240002, 0xDE240003, 0xDE240004);
	const FGuid GuardDefenseThreatId(
		0xDE250001, 0xDE250002, 0xDE250003, 0xDE250004);
	const FGuid GuardDefenseArmorLayerId(
		0xDE260001, 0xDE260002, 0xDE260003, 0xDE260004);
	const FGuid GuardDefenseExtraLayerId(
		0xDE270001, 0xDE270002, 0xDE270003, 0xDE270004);

	FName ThreatActionDefinitionId()
	{
		return TEXT("Combat.Action.Test.P11_4Threat");
	}

	FShanmenCombatActionSnapshot MakeGuardAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = GuardDefenseRunId;
		Capture.OwnerId = GuardDefenseOwnerId;
		Capture.SourceEntityId = GuardDefenseDefenderId;
		Capture.SourceItemInstanceId = GuardDefenseWeaponId;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.4");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.4-GUARD");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1140);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenCombatActionSnapshot MakeThreatAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = GuardDefenseRunId;
		Capture.OwnerId = GuardDefenseThreatId;
		Capture.SourceEntityId = GuardDefenseThreatId;
		Capture.ActionDefinitionId = ThreatActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.4");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.4-THREAT");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1141);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenWeaponGuardDefinition MakeGuardDefinition()
	{
		FShanmenWeaponGuardDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Sword.WeaponGuard01");
		Capture.GuardFraction = 0.25f;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(Capture, Definition));
		return Definition;
	}

	FShanmenHitCandidate MakeCandidate(
		const FGuid& SourceEntityId = GuardDefenseThreatId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			GuardDefenseRunId,
			SourceEntityId,
			ThreatActionDefinitionId(),
			1141);
		Candidate.SourceEntityId = SourceEntityId;
		Candidate.TargetEntityId = GuardDefenseDefenderId;
		Candidate.DetectorId = TEXT("Detector.Test.P11_4Threat");
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
		Armor.LayerId = GuardDefenseArmorLayerId;
		Armor.RuleId = TEXT("Defense.Test.P11_4Armor");
		Armor.Operation = EShanmenDefenseOperation::AbsorbPoints;
		Armor.Order = FShanmenDefenseOrder::Resistance;
		Armor.Magnitude = 10.0f;
		Armor.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseArmor());
		Armor.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		check(Defense.IsValid());
		return Defense;
	}

	FShanmenWeaponGuardTimelineObservation MakeObservation(
		int64 ObservedTick,
		const FGuid& TimelineId = GuardDefenseTimelineId)
	{
		FShanmenWeaponGuardTimelineObservation Observation;
		check(FShanmenWeaponGuardTimelineObservation::TryCapture(
			TimelineId, ObservedTick, Observation));
		return Observation;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseSnapshot& Defense,
		float RawDamage = 100.0f)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeThreatAction();
		Request.Candidate = MakeCandidate();
		Request.Damage.FormulaId = TEXT("Damage.Test.P11_4Physical");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 7;
		Request.Defense = Defense;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			GuardDefenseRunId,
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	struct FGuardDefenseChain
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenWeaponGuardWindow Window;
		FShanmenWeaponGuardWindowReceipt Open;
		FShanmenWeaponPerfectGuardPolicy TimingPolicy;
		FShanmenWeaponGuardArcPolicy ArcPolicy;

		bool Start(double MinimumFacingDot = 0.5)
		{
			FShanmenActionTransitionReceipt Commit;
			const FShanmenCombatActionSnapshot Action = MakeGuardAction();
			return FShanmenActionOrchestrator::TryStart(
					Action, Runtime, Commit)
				&& Runtime.TryAdvance(
					EShanmenCombatActionPhase::Startup, Commit)
				&& FShanmenWeaponGuardWindow::TryOpen(
					Action,
					MakeGuardDefinition(),
					Commit,
					Runtime,
					Window,
					Open)
				&& FShanmenWeaponPerfectGuardPolicy::TryCapture(
					Open,
					GuardDefenseTimelineId,
					10,
					15,
					TEXT("Defense.Sword.PerfectGuard01"),
					TimingPolicy)
				&& FShanmenWeaponGuardArcPolicy::TryCapture(
					Open,
					TEXT("Defense.Sword.WeaponGuardArc01"),
					MinimumFacingDot,
					ArcPolicy);
		}
	};

	struct FGuardDefenseWorld
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

		~FGuardDefenseWorld()
		{
			Stop();
		}
	};

	struct FGuardDefenseFixture
	{
		FGuardDefenseWorld World;
		FShanmenWorldEntityRegistry Registry;
		FGuardDefenseChain Chain;
		AActor* Defender = nullptr;
		AActor* Threat = nullptr;

		bool Start()
		{
			if (!World.Start()
				|| !Registry.TryBeginRun(GuardDefenseRunId)
				|| !Chain.Start())
			{
				return false;
			}
			Defender = World.Spawn(FVector::ZeroVector);
			Threat = World.Spawn(FVector(100.0, 0.0, 0.0));
			return Defender
				&& Threat
				&& Registry.BindObject(
					GuardDefenseRunId,
					Defender,
					GuardDefenseDefenderId)
					== EShanmenWorldBindingResult::Bound
				&& Registry.BindObject(
					GuardDefenseRunId,
					Threat,
					GuardDefenseThreatId)
					== EShanmenWorldBindingResult::Bound;
		}

		Fdemo_mapShanmenWeaponGuardDefenseResult Compose(
			int64 ObservedTick,
			const FShanmenDefenseSnapshot& BaseDefense,
			const FShanmenHitCandidate& Candidate = MakeCandidate())
		{
			return Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose(
				World.World,
				Registry,
				Defender,
				Threat,
				Chain.Window,
				Chain.Runtime,
				Chain.TimingPolicy,
				MakeObservation(ObservedTick),
				Chain.ArcPolicy,
				Candidate,
				BaseDefense);
		}

		Fdemo_mapShanmenWeaponGuardDefenseResult Compose(int64 ObservedTick)
		{
			return Compose(ObservedTick, MakeBaseDefense());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardDefensePerfectImpactTest,
	"Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator.PerfectImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardDefensePerfectImpactTest::RunTest(const FString&)
{
	FGuardDefenseFixture Fixture;
	if (!Fixture.Start())
	{
		AddError(TEXT("Could not start P11.4 perfect fixture."));
		return false;
	}
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();
	const Fdemo_mapShanmenWeaponGuardDefenseResult Result =
		Fixture.Compose(10, Base);
	const FShanmenImpactResult Impact = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Result.Defense));

	TestTrue(TEXT("Perfect guard appends exactly one selected layer"),
		Result.IsSuccess()
			&& Result.HasGuardLayer()
			&& Result.Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedQualified
			&& Result.BaseDefense.Layers.Num() == 1
			&& Result.Defense.Layers.Num() == 2
			&& Result.Defense.Layers.Last().LayerId
				== Result.WorldEvaluation.Evaluation.GetLayer().LayerId
			&& Result.Defense.Layers.Last().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard()));
	TestEqual(TEXT("Caller base snapshot remains untouched"),
		Base.Layers.Num(), 1);
	TestTrue(TEXT("Composed snapshot enters the existing pure resolver"),
		Impact.IsConserved()
			&& Impact.Outcome == EShanmenDefenseOutcome::PerfectGuarded
			&& FMath::IsNearlyEqual(Impact.PreventedDamage, 100.0f)
			&& FMath::IsNearlyZero(Impact.FinalDamage));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardDefenseOrdinaryAndOutsideTest,
	"Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator.OrdinaryAndOutside",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardDefenseOrdinaryAndOutsideTest::RunTest(
	const FString&)
{
	FGuardDefenseFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardDefenseResult Ordinary =
		Fixture.Compose(15);
	const FShanmenImpactResult OrdinaryImpact = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Ordinary.Defense));
	Fixture.Threat->SetActorLocation(
		FVector(-100.0, 0.0, 0.0),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	const Fdemo_mapShanmenWeaponGuardDefenseResult Outside =
		Fixture.Compose(15);
	const FShanmenImpactResult OutsideImpact = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(Outside.Defense));

	TestTrue(TEXT("Ordinary guard composes before existing armor"),
		Ordinary.HasGuardLayer()
			&& Ordinary.Defense.Layers.Last().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard())
			&& OrdinaryImpact.IsConserved()
			&& FMath::IsNearlyEqual(OrdinaryImpact.PreventedDamage, 35.0f)
			&& FMath::IsNearlyEqual(OrdinaryImpact.FinalDamage, 65.0f));
	TestTrue(TEXT("Outside arc preserves base defense without guard layer"),
		Outside.IsSuccess()
			&& !Outside.HasGuardLayer()
			&& Outside.Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedOutsideArc
			&& Outside.Defense.Layers.Num() == 1
			&& Outside.Defense.Layers[0].LayerId == GuardDefenseArmorLayerId
			&& OutsideImpact.IsConserved()
			&& FMath::IsNearlyEqual(OutsideImpact.PreventedDamage, 10.0f)
			&& FMath::IsNearlyEqual(OutsideImpact.FinalDamage, 90.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardDefenseConflictTest,
	"Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator.Conflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardDefenseConflictTest::RunTest(const FString&)
{
	FGuardDefenseFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardDefenseResult First =
		Fixture.Compose(10);
	const Fdemo_mapShanmenWeaponGuardDefenseResult Reappend =
		Fixture.Compose(10, First.Defense);
	FShanmenDefenseSnapshot Invalid = MakeBaseDefense();
	const FShanmenDefenseLayer Duplicate = Invalid.Layers[0];
	Invalid.Layers.Add(Duplicate);
	const Fdemo_mapShanmenWeaponGuardDefenseResult InvalidBase =
		Fixture.Compose(10, Invalid);

	TestEqual(TEXT("Already-present guard identity fails closed"),
		Reappend.Status,
		Edemo_mapShanmenWeaponGuardDefenseStatus::LayerConflict);
	TestFalse(TEXT("Conflict publishes no coordinator receipt"),
		Reappend.ReceiptId.IsValid()
			|| !Reappend.BaseDefense.Layers.IsEmpty()
			|| !Reappend.BaseDefense.TargetTags.IsEmpty()
			|| !Reappend.Defense.Layers.IsEmpty()
			|| !Reappend.Defense.TargetTags.IsEmpty());
	TestEqual(TEXT("Duplicate base layer identity rejected before projection"),
		InvalidBase.Status,
		Edemo_mapShanmenWeaponGuardDefenseStatus::BaseDefenseInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardDefenseAuthorityFailureTest,
	"Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator.AuthorityFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardDefenseAuthorityFailureTest::RunTest(const FString&)
{
	FGuardDefenseFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const FGuid ForeignTimeline(
		0xDE280001, 0xDE280002, 0xDE280003, 0xDE280004);
	const Fdemo_mapShanmenWeaponGuardDefenseResult TimingFailure =
		Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose(
			Fixture.World.World,
			Fixture.Registry,
			Fixture.Defender,
			Fixture.Threat,
			Fixture.Chain.Window,
			Fixture.Chain.Runtime,
			Fixture.Chain.TimingPolicy,
			MakeObservation(10, ForeignTimeline),
			Fixture.Chain.ArcPolicy,
			MakeCandidate(),
			MakeBaseDefense());
	const Fdemo_mapShanmenWeaponGuardDefenseResult WorldFailure =
		Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose(
			nullptr,
			Fixture.Registry,
			Fixture.Defender,
			Fixture.Threat,
			Fixture.Chain.Window,
			Fixture.Chain.Runtime,
			Fixture.Chain.TimingPolicy,
			MakeObservation(10),
			Fixture.Chain.ArcPolicy,
			MakeCandidate(),
			MakeBaseDefense());
	const FGuid ForeignThreat(
		0xDE290001, 0xDE290002, 0xDE290003, 0xDE290004);
	const Fdemo_mapShanmenWeaponGuardDefenseResult IdentityFailure =
		Fixture.Compose(10, MakeBaseDefense(), MakeCandidate(ForeignThreat));

	TestEqual(TEXT("Foreign timeline is rejected by P11.1"),
		TimingFailure.Status,
		Edemo_mapShanmenWeaponGuardDefenseStatus::TimingProjectionRejected);
	TestEqual(TEXT("Null World is reported through P11.3"),
		WorldFailure.Status,
		Edemo_mapShanmenWeaponGuardDefenseStatus::WorldEvaluationRejected);
	TestEqual(TEXT("Nested World status remains inspectable"),
		WorldFailure.WorldEvaluation.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::WorldInvalid);
	TestEqual(TEXT("Foreign threat identity is rejected by P11.3"),
		IdentityFailure.WorldEvaluation.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::ThreatIdentityMismatch);
	TestFalse(TEXT("Authority failures publish no coordinator receipt"),
		TimingFailure.ReceiptId.IsValid()
			|| WorldFailure.ReceiptId.IsValid()
			|| IdentityFailure.ReceiptId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardDefenseReplayLifecycleTest,
	"Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator.ReplayLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardDefenseReplayLifecycleTest::RunTest(const FString&)
{
	FGuardDefenseFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardDefenseResult First =
		Fixture.Compose(10);
	const Fdemo_mapShanmenWeaponGuardDefenseResult Replay =
		Fixture.Compose(10);
	FShanmenDefenseSnapshot Extended = MakeBaseDefense();
	FShanmenDefenseLayer& Extra = Extended.Layers.AddDefaulted_GetRef();
	Extra.LayerId = GuardDefenseExtraLayerId;
	Extra.RuleId = TEXT("Defense.Test.P11_4Extra");
	Extra.Operation = EShanmenDefenseOperation::AbsorbPoints;
	Extra.Order = FShanmenDefenseOrder::Shield;
	Extra.Magnitude = 5.0f;
	Extra.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
	check(Extended.IsValid());
	const Fdemo_mapShanmenWeaponGuardDefenseResult DifferentBase =
		Fixture.Compose(10, Extended);
	Fixture.Threat->SetActorLocation(
		FVector(200.0, 0.0, 0.0),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	const Fdemo_mapShanmenWeaponGuardDefenseResult Moved =
		Fixture.Compose(10);

	TestTrue(TEXT("Exact inputs replay deterministic coordinator receipt"),
		First.IsSuccess()
			&& Replay.IsSuccess()
			&& First.ReceiptId == Replay.ReceiptId);
	TestNotEqual(TEXT("Base snapshot evidence participates in receipt identity"),
		First.ReceiptId, DifferentBase.ReceiptId);
	TestNotEqual(TEXT("P11.3 live position evidence participates in receipt"),
		First.ReceiptId, Moved.ReceiptId);

	FShanmenActionTransitionReceipt Recovery;
	check(Fixture.Chain.Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	const Fdemo_mapShanmenWeaponGuardDefenseResult Closed =
		Fixture.Compose(10);
	TestEqual(TEXT("Recovery closes composition through P11.1"),
		Closed.Status,
		Edemo_mapShanmenWeaponGuardDefenseStatus::TimingProjectionRejected);
	TestFalse(TEXT("Closed lifecycle publishes no coordinator receipt"),
		Closed.ReceiptId.IsValid());
	return true;
}

#endif
