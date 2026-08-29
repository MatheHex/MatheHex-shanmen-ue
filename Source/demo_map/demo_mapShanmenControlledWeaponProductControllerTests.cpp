#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponProductController.h"

#include "Components/BoxComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	const FGuid ProductRunId(0xD3640001, 0, 0, 1);
	const FGuid ProductOwnerId(0xD3640002, 0, 0, 1);
	const FGuid ProductItemId(0xD3640003, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindProductEnemyDefinition()
	{
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype
				== Edemo_mapM01EnemyArchetype::StandardSkirmisher)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	Fdemo_mapEnemyEncounterIdentity MakeProductEncounterIdentity(
		const Fdemo_mapM01EnemyDefinition& Definition)
	{
		Fdemo_mapEnemyEncounterIdentity Identity;
		Identity.EncounterId = Definition.EncounterId;
		Identity.RouteId = Definition.RouteId;
		Identity.SpawnMarkerId = Definition.SpawnMarkerId;
		Identity.LootTableId = Definition.CorpseIdentity;
		Identity.SkillProfileId = Definition.SkillProfileId;
		return Identity;
	}

	struct FControlledWeaponProductFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		AActor* Weapon = nullptr;
		UBoxComponent* WeaponRoot = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FControlledWeaponProductFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P64PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P64PlayerHealth"))
				: nullptr;
			Weapon = NewObject<AActor>(GetTransientPackage());
			WeaponRoot = Weapon
				? NewObject<UBoxComponent>(Weapon, TEXT("P64WeaponRoot"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}
			if (Weapon && WeaponRoot)
			{
				Weapon->SetRootComponent(WeaponRoot);
				Weapon->SetActorLocation(FVector(10.0, 20.0, 30.0));
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindProductEnemyDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P64EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| !Weapon || !WeaponRoot
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}

			Enemy->AddInstanceComponent(EnemyIdentity);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeProductEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					ProductRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	FShanmenContentStamp MakeProductContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.4");
		Content.Digest = TEXT("P6.4.ControlledWeaponProductController.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakeProductPrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& SourceEntityId)
	{
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId = FGuid(0xD3640010, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = ProductOwnerId;
		Prepared.Evidence.ItemInstanceId = ProductItemId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.4");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3640011, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 12;
		Prepared.Evidence.ItemRevision = 6;
		Prepared.Evidence.Content = MakeProductContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = ProductOwnerId;
		ActionCapture.SourceEntityId = SourceEntityId;
		ActionCapture.SourceItemInstanceId = ProductItemId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeProductContent();
		ActionCapture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		ActionCapture.SourceTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponFlyingSword());
		ActionCapture.ActivationId =
			FShanmenCombatIdFactory::MakeActivationId(
				ActionCapture.RunId,
				ActionCapture.SourceEntityId,
				ActionCapture.ActionDefinitionId,
				1);
		check(FShanmenCombatActionSnapshot::TryCapture(
			ActionCapture, Prepared.Action));

		FShanmenControlledWeaponDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.DetectorId =
			TEXT("Detector.ControlledWeapon.P6.4.FlyingSword");
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.4.ProductTest");
		DefinitionCapture.BaseDamage = 0.5f;
		DefinitionCapture.ControlPowerCoefficient = 0.01f;
		DefinitionCapture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		DefinitionCapture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		check(FShanmenControlledWeaponDefinition::TryCapture(
			DefinitionCapture, Prepared.Definition));
		check(FShanmenControlledWeaponOffenseSnapshot::TryCapture(
			40.0f, Prepared.Offense));
		check(FShanmenControlledWeaponExecution::TryCreate(
			Prepared.Action,
			Prepared.Definition,
			Prepared.Offense,
			Prepared.Execution));
		check(Prepared.IsPrepared());
		return Prepared;
	}

	Fdemo_mapShanmenControlledWeaponMotionCapture MakeProductMotion()
	{
		Fdemo_mapShanmenControlledWeaponMotionCapture Motion;
		Motion.DirectedSpeed = 400.0f;
		Motion.OrbitCenterOffset = FVector(0.0, 0.0, 50.0);
		Motion.OrbitPlaneNormal = FVector::UpVector;
		Motion.OrbitReferenceAxis = FVector::ForwardVector;
		Motion.OrbitRadius = 100.0f;
		Motion.OrbitAngularSpeedRadiansPerSecond = UE_PI * 0.5f;
		Motion.InitialOrbitPhaseRadians = 0.0f;
		Motion.MaximumStepSeconds = 0.5f;
		return Motion;
	}

	Fdemo_mapShanmenControlledWeaponProductStartResult StartProduct(
		FControlledWeaponProductFixture& Fixture,
		Fdemo_mapShanmenControlledWeaponProductController& OutController)
	{
		return Fdemo_mapShanmenControlledWeaponProductController::TryStart(
			MakeProductPrepared(
				Fixture.Coordinator,
				Fixture.Coordinator.GetPlayerEntityId()),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapon,
			Fixture.WeaponRoot,
			MakeProductMotion(),
			OutController);
	}

	FHitResult MakeProductSweepHit(
		const FControlledWeaponProductFixture& Fixture)
	{
		FHitResult Hit(
			Fixture.Enemy,
			Fixture.GetEnemyRoot(),
			FVector(100.0, 20.0, 30.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(100.0, 20.0, 30.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}

	FOverlapResult MakeProductOverlap(
		const FControlledWeaponProductFixture& Fixture)
	{
		FOverlapResult Overlap;
		Overlap.OverlapObjectHandle = FActorInstanceHandle(Fixture.Enemy);
		Overlap.Component = Fixture.GetEnemyRoot();
		Overlap.ItemIndex = 0;
		return Overlap;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponProductBindingMotionTest,
	"Shanmen.0_0_10.Product.ControlledWeaponController.BindingAndMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponProductBindingMotionTest::RunTest(
	const FString&)
{
	FControlledWeaponProductFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.4 product fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Controller;
	const Fdemo_mapShanmenControlledWeaponProductStartResult Start =
		StartProduct(Fixture, Controller);
	TestTrue(TEXT("Exact deployed item binds to one physical weapon Actor"),
		Start.IsStarted()
		&& Controller.IsActive()
		&& Controller.GetSourceActor() == Fixture.Pawn
		&& Controller.GetWeaponActor() == Fixture.Weapon
		&& Controller.GetWeaponCollisionRoot() == Fixture.WeaponRoot
		&& Controller.GetSession().GetEvidence().ItemInstanceId
			== ProductItemId);

	Fdemo_mapShanmenControlledWeaponMovementReceipt Movement;
	FHitResult BlockingHit;
	TestFalse(TEXT("Orbiting state cannot consume a directed movement step"),
		Controller.TryAdvanceDirected(0.25f, Movement, BlockingHit));

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Launch accepts the first product control sample"),
		Controller.TryLaunch(0, FVector(2.0, 0.0, 0.0), Command));
	const FGuid LaunchCommandId = Command.GetCommandId();
	TestTrue(TEXT("Exact Launch replay remains idempotent at product boundary"),
		Controller.TryLaunch(0, FVector::ForwardVector, Command)
		&& Command.GetCommandId() == LaunchCommandId
		&& Controller.GetSession().GetExecution().GetNextCommandSequence()
			== 1);
	const FVector FirstStart = Fixture.Weapon->GetActorLocation();
	TestTrue(TEXT("Frozen speed advances the same Actor with a swept step"),
		Controller.TryAdvanceDirected(0.25f, Movement, BlockingHit)
		&& Movement.IsValid()
		&& Movement.SourceItemInstanceId == ProductItemId
		&& Movement.CommandSequence == 0
		&& Movement.StartLocation.Equals(FirstStart)
		&& Movement.RequestedEndLocation.Equals(
			FirstStart + FVector(100.0, 0.0, 0.0))
		&& Movement.ActualEndLocation.Equals(
			Fixture.Weapon->GetActorLocation()));

	const FVector BeforeRejectedStep = Fixture.Weapon->GetActorLocation();
	TestFalse(TEXT("Oversized DeltaSeconds fails before moving the Actor"),
		Controller.TryAdvanceDirected(0.75f, Movement, BlockingHit));
	TestTrue(TEXT("Rejected step leaves the physical transform unchanged"),
		Fixture.Weapon->GetActorLocation().Equals(BeforeRejectedStep));

	TestTrue(TEXT("Redirect changes direction without replacing the Actor"),
		Controller.TryRedirect(1, FVector(0.0, 3.0, 0.0), Command));
	const FVector RedirectStart = Fixture.Weapon->GetActorLocation();
	TestTrue(TEXT("Later step uses the accepted redirect and sequence"),
		Controller.TryAdvanceDirected(0.1f, Movement, BlockingHit)
		&& Movement.CommandSequence == 1
		&& Movement.Direction.Equals(FVector::RightVector)
		&& Movement.RequestedEndLocation.Equals(
			RedirectStart + FVector(0.0, 40.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponProductOrbitMotionTest,
	"Shanmen.0_0_10.Product.ControlledWeaponController.OrbitMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponProductOrbitMotionTest::RunTest(
	const FString&)
{
	FControlledWeaponProductFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not prepare P6.8 orbit fixture."));
		return false;
	}
	Fixture.Pawn->SetActorLocation(FVector(100.0, 200.0, 10.0));

	Fdemo_mapShanmenControlledWeaponProductController Controller;
	if (!StartProduct(Fixture, Controller).IsStarted())
	{
		AddError(TEXT("Could not start P6.8 orbit controller."));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt Orbit;
	const FVector FirstCenter(100.0, 200.0, 60.0);
	const FVector FirstExpected = FirstCenter + FVector(
		100.0 / FMath::Sqrt(2.0),
		100.0 / FMath::Sqrt(2.0),
		0.0);
	TestTrue(TEXT("Orbiting item consumes its frozen half-second pose sample"),
		Controller.TryAdvanceOrbiting(0.5f, Orbit)
		&& Orbit.IsValid()
		&& Orbit.SourceItemInstanceId == ProductItemId
		&& Orbit.Center.Equals(FirstCenter)
		&& FMath::IsNearlyEqual(
			Orbit.StartPhaseRadians, 0.0f)
		&& FMath::IsNearlyEqual(
			Orbit.EndPhaseRadians, UE_PI * 0.25f)
		&& Orbit.RequestedEndLocation.Equals(
			FirstExpected, 0.001f)
		&& Fixture.Weapon->GetActorLocation().Equals(
			FirstExpected, 0.001f));

	const FVector BeforeRejected = Fixture.Weapon->GetActorLocation();
	const float PhaseBeforeRejected =
		Controller.GetCurrentOrbitPhaseRadians();
	TestFalse(TEXT("Oversized orbit sample fails before placement or phase"),
		Controller.TryAdvanceOrbiting(0.75f, Orbit));
	TestTrue(TEXT("Rejected orbit sample preserves transform and phase"),
		Fixture.Weapon->GetActorLocation().Equals(BeforeRejected)
		&& FMath::IsNearlyEqual(
			Controller.GetCurrentOrbitPhaseRadians(),
			PhaseBeforeRejected));

	Fixture.Pawn->SetActorLocation(FVector(110.0, 230.0, 20.0));
	const FVector SecondExpected(110.0, 330.0, 70.0);
	TestTrue(TEXT("Orbit center follows the current source Actor location"),
		Controller.TryAdvanceOrbiting(0.5f, Orbit)
		&& FMath::IsNearlyEqual(
			Orbit.EndPhaseRadians, UE_PI * 0.5f)
		&& Orbit.RequestedEndLocation.Equals(
			SecondExpected, 0.001f));

	FShanmenControlledWeaponCommandReceipt Launch;
	TestTrue(TEXT("Launch leaves Orbiting through the canonical command"),
		Controller.TryLaunch(0, FVector::ForwardVector, Launch)
		&& Controller.IsDirected());
	const FVector LaunchedLocation = Fixture.Weapon->GetActorLocation();
	TestFalse(TEXT("Directed item cannot consume an orbit pose"),
		Controller.TryAdvanceOrbiting(0.1f, Orbit));
	TestTrue(TEXT("Rejected post-launch orbit leaves Actor unchanged"),
		Fixture.Weapon->GetActorLocation().Equals(LaunchedLocation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponProductContactCompletionTest,
	"Shanmen.0_0_10.Product.ControlledWeaponController.ContactAndCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponProductContactCompletionTest::RunTest(
	const FString&)
{
	FControlledWeaponProductFixture Fixture;
	Fdemo_mapShanmenControlledWeaponProductController Controller;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !StartProduct(Fixture, Controller).IsStarted())
	{
		AddError(TEXT("Could not prepare P6.4 contact fixture."));
		return false;
	}

	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("Directed product opens exactly one owned contact window"),
		Controller.TryLaunch(0, FVector::ForwardVector, Command)
		&& Controller.TryBeginContactWindow(FirstContext)
		&& Controller.HasActiveContactWindow());

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult First =
		Controller.ResolveSweepContact(
			Fixture.Coordinator, MakeProductSweepHit(Fixture));
	FShanmenTargetVitalitySnapshot AfterFirst;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterFirst));
	TestTrue(TEXT("Sweep contact reaches only canonical vitality authority"),
		First.IsDelivered()
		&& First.Impact.GetRequest().Action.GetSourceItemInstanceId()
			== ProductItemId
		&& FMath::IsNearlyEqual(
			Before.CurrentVitality - AfterFirst.CurrentVitality,
			First.GetNewlyCommittedDamage(), 0.001f)
		&& Controller.GetSession().GetExecution().NumAcceptedImpacts() == 1);

	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Duplicate =
		Controller.ResolveSweepContact(
			Fixture.Coordinator, MakeProductSweepHit(Fixture));
	TestTrue(TEXT("Duplicate callback cannot mutate product or vitality twice"),
		!Duplicate.IsDelivered()
		&& Controller.GetSession().GetExecution().NumAcceptedImpacts() == 1);

	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestFalse(TEXT("Recall cannot race the controller-owned contact window"),
		Controller.TryRecallAndComplete(
			1, Recall, Recovery, Completed));
	TestTrue(TEXT("First contact window closes without ending the Session"),
		Controller.TryEndContactWindow()
		&& !Controller.HasActiveContactWindow());

	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("A later product window owns the next stable ordinal"),
		Controller.TryBeginContactWindow(SecondContext)
		&& SecondContext.GetHitOrdinal()
			== FirstContext.GetHitOrdinal() + 1);
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Second =
		Controller.ResolveOverlapContact(
			Fixture.Coordinator,
			MakeProductOverlap(Fixture),
			FVector(110.0, 20.0, 30.0),
			FVector::BackwardVector);
	TestTrue(TEXT("Overlap callback shares the same owned Session and authority"),
		Second.IsDelivered()
		&& Controller.GetSession().GetExecution().NumAcceptedImpacts() == 2);
	TestTrue(TEXT("Closed window permits canonical Recall and completion"),
		Controller.TryEndContactWindow()
		&& Controller.TryRecallAndComplete(
			1, Recall, Recovery, Completed)
		&& Controller.IsValid()
		&& Controller.GetSession().IsTerminal());

	Fdemo_mapShanmenControlledWeaponMovementReceipt Movement;
	FHitResult BlockingHit;
	TestFalse(TEXT("Completed product rejects any further world movement"),
		Controller.TryAdvanceDirected(0.1f, Movement, BlockingHit));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponProductOrbitThreatTest,
	"Shanmen.0_0_10.Product.ControlledWeaponController.OrbitThreatProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponProductOrbitThreatTest::RunTest(
	const FString&)
{
	FControlledWeaponProductFixture Fixture;
	Fdemo_mapShanmenControlledWeaponProductController Controller;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !StartProduct(Fixture, Controller).IsStarted())
	{
		AddError(TEXT("Could not prepare P6.10 Orbit threat fixture."));
		return false;
	}

	FGuid EnemyEntityId;
	if (!Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			Fixture.Coordinator.GetRunId(),
			Fixture.Enemy,
			INDEX_NONE,
			EnemyEntityId))
	{
		AddError(TEXT("P6.10 fixture enemy has no canonical entity identity."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	FShanmenWorldHitContext ThreatContext;
	TestTrue(TEXT("Orbiting item opens a candidate-only threat window"),
		Controller.TryBeginOrbitThreatWindow(ThreatContext)
		&& Controller.HasActiveOrbitThreatWindow()
		&& !Controller.HasActiveDirectedContactWindow()
		&& ThreatContext.GetHitOrdinal() == 0);

	const Fdemo_mapShanmenControlledWeaponOrbitThreatResult First =
		Controller.ProjectOrbitThreatOverlap(
			Fixture.Coordinator,
			MakeProductOverlap(Fixture),
			FVector(90.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenTargetVitalitySnapshot AfterProjection;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterProjection));
	TestTrue(TEXT("Explicit overlap projects one canonical threat candidate"),
		First.IsProjected()
		&& First.Context.GetAction().GetSourceItemInstanceId()
			== ProductItemId
		&& First.Candidate.TargetEntityId == EnemyEntityId
		&& First.Candidate.HitOrdinal == 0);
	TestTrue(TEXT("Threat projection cannot mutate vitality or impact ledger"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality,
			AfterProjection.CurrentVitality)
		&& Controller.GetSession().GetExecution().NumAcceptedImpacts() == 0);

	const Fdemo_mapShanmenControlledWeaponOrbitThreatResult Duplicate =
		Controller.ProjectOrbitThreatOverlap(
			Fixture.Coordinator,
			MakeProductOverlap(Fixture),
			FVector(91.0, 20.0, 30.0),
			FVector::BackwardVector);
	TestTrue(TEXT("One threat sample accepts each target at most once"),
		Duplicate.Error
			== Edemo_mapShanmenControlledWeaponOrbitThreatError::CandidateRejected);

	FShanmenControlledWeaponCommandReceipt Launch;
	Fdemo_mapShanmenControlledWeaponOrbitMovementReceipt Orbit;
	TestFalse(TEXT("Open threat sample fences Launch"),
		Controller.TryLaunch(0, FVector::ForwardVector, Launch));
	TestFalse(TEXT("Open threat sample fences Orbit movement"),
		Controller.TryAdvanceOrbiting(0.1f, Orbit));
	TestFalse(TEXT("Orbit window cannot call the directed damage adapter"),
		Controller.ResolveOverlapContact(
			Fixture.Coordinator,
			MakeProductOverlap(Fixture),
			FVector(90.0, 20.0, 30.0),
			FVector::BackwardVector).IsDelivered());

	FShanmenDetectorEmissionReceipt ThreatReceipt;
	TestTrue(TEXT("Threat window closes with exact canonical evidence"),
		Controller.TryEndOrbitThreatWindow(ThreatReceipt)
		&& ThreatReceipt.IsValid()
		&& ThreatReceipt.GetCandidates().Num() == 1
		&& ThreatReceipt.GetCandidates()[0].TargetEntityId == EnemyEntityId
		&& ThreatReceipt.GetContext().GetAction().GetSourceItemInstanceId()
			== ProductItemId
		&& !Controller.HasActiveContactWindow());
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Evidence;
	FShanmenControlledWeaponThreatPolicyReceipt ThreatPolicy;
	TestFalse(TEXT("Missing sampled Actor fails closed before target policy"),
		Controller.TryEvaluateOrbitThreatActors(
			Fixture.Coordinator,
			ThreatReceipt,
			{},
			Evidence,
			ThreatPolicy));
	TestTrue(TEXT("Missing Actor exposes an explicit evidence error"),
		Evidence.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			MissingTarget);
	TestFalse(TEXT("Duplicate sampled Actor cannot forge two evidence rows"),
		Controller.TryEvaluateOrbitThreatActors(
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy, Fixture.Enemy },
			Evidence,
			ThreatPolicy));
	TestTrue(TEXT("Duplicate entity identity is distinguished from missing"),
		Evidence.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			DuplicateTarget);
	TestFalse(TEXT("Registered source Actor cannot replace sampled target"),
		Controller.TryEvaluateOrbitThreatActors(
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Pawn },
			Evidence,
			ThreatPolicy));
	TestTrue(TEXT("Out-of-receipt entity is rejected explicitly"),
		Evidence.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			TargetOutsideEmission);
	TestTrue(TEXT("Product exposes the frozen target-policy audit without damage"),
		Controller.TryEvaluateOrbitThreatActors(
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			Evidence,
			ThreatPolicy)
		&& Evidence.IsCaptured()
		&& Evidence.ExpectedTargetCount == 1
		&& Evidence.TargetEvidence.Num() == 1
		&& Evidence.TargetEvidence[0].GetTargetEntityId() == EnemyEntityId
		&& Evidence.TargetEvidence[0].GetTargetTags().HasTagExact(
			FShanmenCombatNativeTags::TargetLiving())
		&& ThreatPolicy.IsValid()
		&& ThreatPolicy.NumAcceptedTargets() == 1
		&& ThreatPolicy.GetTargets()[0].GetCandidate().TargetEntityId
			== EnemyEntityId
		&& FMath::IsNearlyEqual(
			AfterProjection.CurrentVitality,
			Before.CurrentVitality)
		&& Controller.GetSession().GetExecution().NumAcceptedImpacts() == 0
		&& Controller.TryLaunch(0, FVector::ForwardVector, Launch));
	FShanmenWorldHitContext DirectedContext;
	TestTrue(TEXT("Directed contact continues the shared detector ordinal"),
		Controller.TryBeginContactWindow(DirectedContext)
		&& DirectedContext.GetHitOrdinal()
			== ThreatContext.GetHitOrdinal() + 1);
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Delivered =
		Controller.ResolveOverlapContact(
			Fixture.Coordinator,
			MakeProductOverlap(Fixture),
			FVector(110.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenTargetVitalitySnapshot AfterDirected;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterDirected));
	TestTrue(TEXT("Only the later Directed candidate reaches damage authority"),
		Delivered.IsDelivered()
		&& Controller.GetSession().GetExecution().NumAcceptedImpacts() == 1
		&& AfterDirected.CurrentVitality
			< AfterProjection.CurrentVitality
		&& Controller.TryEndContactWindow());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponProductAtomicFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponController.AtomicFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponProductAtomicFenceTest::RunTest(
	const FString&)
{
	FControlledWeaponProductFixture Fixture;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not prepare P6.4 atomic-fence fixture."));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponProductController Rejected;
	const FGuid WrongSourceEntityId(0xD36400FF, 0, 0, 1);
	const Fdemo_mapShanmenControlledWeaponProductStartResult SourceMismatch =
		Fdemo_mapShanmenControlledWeaponProductController::TryStart(
			MakeProductPrepared(Fixture.Coordinator, WrongSourceEntityId),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapon,
			Fixture.WeaponRoot,
			MakeProductMotion(),
			Rejected);
	TestTrue(TEXT("Prepared source identity must match the registered player"),
		SourceMismatch.Error
			== Edemo_mapShanmenControlledWeaponProductStartError::SourceMismatch
		&& !Rejected.IsValid());

	const Fdemo_mapShanmenControlledWeaponProductStartResult SameActor =
		Fdemo_mapShanmenControlledWeaponProductController::TryStart(
			MakeProductPrepared(
				Fixture.Coordinator,
				Fixture.Coordinator.GetPlayerEntityId()),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Pawn,
			Fixture.PlayerRoot,
			MakeProductMotion(),
			Rejected);
	TestTrue(TEXT("Source Actor cannot masquerade as the physical weapon"),
		SameActor.Error
			== Edemo_mapShanmenControlledWeaponProductStartError::ActorBindingInvalid
		&& !Rejected.IsValid());

	Fdemo_mapShanmenControlledWeaponMotionCapture InvalidOrbit =
		MakeProductMotion();
	InvalidOrbit.OrbitReferenceAxis = FVector::UpVector;
	const Fdemo_mapShanmenControlledWeaponProductStartResult BadOrbit =
		Fdemo_mapShanmenControlledWeaponProductController::TryStart(
			MakeProductPrepared(
				Fixture.Coordinator,
				Fixture.Coordinator.GetPlayerEntityId()),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapon,
			Fixture.WeaponRoot,
			InvalidOrbit,
			Rejected);
	TestTrue(TEXT("Orbit plane and reference axis must be orthogonal"),
		BadOrbit.Error
			== Edemo_mapShanmenControlledWeaponProductStartError::MotionInvalid
		&& !Rejected.IsValid());

	Fdemo_mapShanmenControlledWeaponProductController Controller;
	if (!StartProduct(Fixture, Controller).IsStarted())
	{
		AddError(TEXT("Valid P6.4 controller did not start."));
		return false;
	}
	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenWorldHitContext Context;
	TestTrue(TEXT("Interrupt fixture owns one open directed window"),
		Controller.TryLaunch(0, FVector::ForwardVector, Command)
		&& Controller.TryBeginContactWindow(Context));

	FShanmenActionTransitionReceipt Interrupted;
	TestTrue(TEXT("Interrupt closes Session and contact window atomically"),
		Controller.TryInterrupt(Interrupted)
		&& Controller.IsValid()
		&& Controller.GetSession().IsTerminal()
		&& !Controller.HasActiveContactWindow());
	TestFalse(TEXT("Interrupted product rejects redirect"),
		Controller.TryRedirect(1, FVector::RightVector, Command));
	TestFalse(TEXT("Interrupted product rejects a new contact window"),
		Controller.TryBeginContactWindow(Context));
	return true;
}

#endif
