#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponRunHost.h"

#include "Components/BoxComponent.h"
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
	const FGuid HostRunId(0xD3650001, 0, 0, 1);
	const FGuid HostLowItemId(0xD3650002, 0, 0, 1);
	const FGuid HostHighItemId(0xD3650003, 0, 0, 1);
	const FGuid HostThirdItemId(0xD3650004, 0, 0, 1);
	const FGuid HostOwnerId(0xD3650010, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindHostEnemyDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeHostEncounterIdentity(
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

	struct FControlledWeaponHostFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		TArray<AActor*> Weapons;
		TArray<UBoxComponent*> WeaponRoots;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FControlledWeaponHostFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P65PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P65PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}

			for (int32 Index = 0; Index < 3; ++Index)
			{
				AActor* Weapon = NewObject<AActor>(GetTransientPackage());
				UBoxComponent* Root = Weapon
					? NewObject<UBoxComponent>(
						Weapon,
						FName(*FString::Printf(
							TEXT("P65WeaponRoot%d"), Index)))
					: nullptr;
				if (Weapon && Root)
				{
					Weapon->SetRootComponent(Root);
					Weapon->SetActorLocation(
						FVector(10.0, 100.0 * Index, 30.0));
				}
				Weapons.Add(Weapon);
				WeaponRoots.Add(Root);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindHostEnemyDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P65EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| Weapons.Contains(nullptr)
				|| WeaponRoots.Contains(nullptr)
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}

			Enemy->AddInstanceComponent(EnemyIdentity);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeHostEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					HostRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	FShanmenContentStamp MakeHostContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.5");
		Content.Digest = TEXT("P6.5.ControlledWeaponRunHost.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakeHostPrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence)
	{
		const uint32 SequenceBits = static_cast<uint32>(ActivationSequence);
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId =
			FGuid(0xD3650100 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = HostOwnerId;
		Prepared.Evidence.ItemInstanceId = ItemInstanceId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.5");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3650200 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 13;
		Prepared.Evidence.ItemRevision = 7;
		Prepared.Evidence.Content = MakeHostContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = HostOwnerId;
		ActionCapture.SourceEntityId = Coordinator.GetPlayerEntityId();
		ActionCapture.SourceItemInstanceId = ItemInstanceId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeHostContent();
		ActionCapture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		ActionCapture.SourceTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponFlyingSword());
		ActionCapture.ActivationId =
			FShanmenCombatIdFactory::MakeActivationId(
				ActionCapture.RunId,
				ActionCapture.SourceEntityId,
				ActionCapture.ActionDefinitionId,
				ActivationSequence);
		check(FShanmenCombatActionSnapshot::TryCapture(
			ActionCapture, Prepared.Action));

		FShanmenControlledWeaponDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.DetectorId = FName(*FString::Printf(
			TEXT("Detector.ControlledWeapon.P6.5.%lld"),
			ActivationSequence));
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.5.HostTest");
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

	Fdemo_mapShanmenControlledWeaponMotionCapture MakeHostMotion(
		float InitialOrbitPhaseRadians = 0.0f)
	{
		Fdemo_mapShanmenControlledWeaponMotionCapture Motion;
		Motion.DirectedSpeed = 400.0f;
		Motion.OrbitCenterOffset = FVector(0.0, 0.0, 50.0);
		Motion.OrbitPlaneNormal = FVector::UpVector;
		Motion.OrbitReferenceAxis = FVector::ForwardVector;
		Motion.OrbitRadius = 100.0f;
		Motion.OrbitAngularSpeedRadiansPerSecond = UE_PI * 0.5f;
		Motion.InitialOrbitPhaseRadians = InitialOrbitPhaseRadians;
		Motion.MaximumStepSeconds = 0.5f;
		return Motion;
	}

	Fdemo_mapShanmenControlledWeaponHostAttachResult AttachHostWeapon(
		FControlledWeaponHostFixture& Fixture,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence,
		int32 WeaponIndex,
		float InitialOrbitPhaseRadians = 0.0f)
	{
		return Host.TryAttach(
			MakeHostPrepared(
				Fixture.Coordinator,
				ItemInstanceId,
				ActivationSequence),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[WeaponIndex],
			Fixture.WeaponRoots[WeaponIndex],
			MakeHostMotion(InitialOrbitPhaseRadians));
	}

	FHitResult MakeHostSweepHit(
		const FControlledWeaponHostFixture& Fixture)
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

	FOverlapResult MakeHostOverlap(
		const FControlledWeaponHostFixture& Fixture)
	{
		FOverlapResult Overlap;
		Overlap.OverlapObjectHandle = FActorInstanceHandle(Fixture.Enemy);
		Overlap.Component = Fixture.GetEnemyRoot();
		Overlap.ItemIndex = 0;
		return Overlap;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostStableOrderTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.StableIdentityAndOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostStableOrderTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady)
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.5 host fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	TestTrue(TEXT("Reverse attachment accepts two exact physical items"),
		AttachHostWeapon(Fixture, Host, HostHighItemId, 2, 1).IsAttached()
		&& AttachHostWeapon(Fixture, Host, HostLowItemId, 1, 0).IsAttached()
		&& Host.IsValid()
		&& Host.NumBound() == 2
		&& Host.NumActive() == 2);
	const TArray<FGuid> Ordered = Host.GetOrderedItemInstanceIds();
	TestTrue(TEXT("Stable item identity, not attachment order, owns batching"),
		Ordered.Num() == 2
		&& Ordered[0] == HostLowItemId
		&& Ordered[1] == HostHighItemId);

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Each exact item accepts an independent Launch sequence"),
		Host.TryLaunch(HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(HostHighItemId, 0, FVector::RightVector, Command));
	const FVector LowStart = Fixture.Weapons[0]->GetActorLocation();
	const FVector HighStart = Fixture.Weapons[1]->GetActorLocation();
	Fdemo_mapShanmenControlledWeaponHostMovementBatch Batch;
	TestTrue(TEXT("Directed swords advance in stable item order"),
		Host.TryAdvanceDirectedInOrder(0.1f, Batch)
		&& Batch.IsFullyAdvanced()
		&& Batch.Entries.Num() == 2
		&& Batch.Entries[0].ItemInstanceId == HostLowItemId
		&& Batch.Entries[1].ItemInstanceId == HostHighItemId
		&& Fixture.Weapons[0]->GetActorLocation().Equals(
			LowStart + FVector(40.0, 0.0, 0.0))
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighStart + FVector(0.0, 40.0, 0.0)));

	const FVector LowBeforeRejected =
		Fixture.Weapons[0]->GetActorLocation();
	const FVector HighBeforeRejected =
		Fixture.Weapons[1]->GetActorLocation();
	TestFalse(TEXT("Any oversized sample fails host preflight"),
		Host.TryAdvanceDirectedInOrder(0.75f, Batch));
	TestTrue(TEXT("Rejected batch moves none of the physical Actors"),
		Fixture.Weapons[0]->GetActorLocation().Equals(LowBeforeRejected)
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighBeforeRejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostOrbitOrderTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitOrderAndPreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostOrbitOrderTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not prepare P6.8 host orbit fixture."));
		return false;
	}
	Fixture.Pawn->SetActorLocation(FVector(50.0, 60.0, 10.0));
	if (!AttachHostWeapon(
			Fixture,
			Host,
			HostHighItemId,
			2,
			1,
			UE_PI).IsAttached()
		|| !AttachHostWeapon(
			Fixture,
			Host,
			HostLowItemId,
			1,
			0,
			0.0f).IsAttached())
	{
		AddError(TEXT("Could not attach P6.8 host orbit items."));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponHostOrbitBatch Orbit;
	TestTrue(TEXT("Host advances reverse-attached orbit items in stable order"),
		Host.TryAdvanceOrbitingInOrder(0.5f, Orbit)
		&& Orbit.IsFullyAdvanced()
		&& Orbit.Entries.Num() == 2
		&& Orbit.Entries[0].ItemInstanceId == HostLowItemId
		&& Orbit.Entries[1].ItemInstanceId == HostHighItemId
		&& FMath::IsNearlyEqual(
			Orbit.Entries[0].Movement.EndPhaseRadians,
			UE_PI * 0.25f)
		&& FMath::IsNearlyEqual(
			Orbit.Entries[1].Movement.EndPhaseRadians,
			UE_PI * 1.25f));

	const FVector LowBeforeRejected =
		Fixture.Weapons[0]->GetActorLocation();
	const FVector HighBeforeRejected =
		Fixture.Weapons[1]->GetActorLocation();
	const float LowPhaseBeforeRejected =
		Host.FindController(HostLowItemId)
			->GetCurrentOrbitPhaseRadians();
	const float HighPhaseBeforeRejected =
		Host.FindController(HostHighItemId)
			->GetCurrentOrbitPhaseRadians();
	TestFalse(TEXT("Any oversized orbit sample fails host preflight"),
		Host.TryAdvanceOrbitingInOrder(0.75f, Orbit));
	TestTrue(TEXT("Rejected orbit batch changes no transform or phase"),
		Fixture.Weapons[0]->GetActorLocation().Equals(LowBeforeRejected)
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighBeforeRejected)
		&& FMath::IsNearlyEqual(
			Host.FindController(HostLowItemId)
				->GetCurrentOrbitPhaseRadians(),
			LowPhaseBeforeRejected)
		&& FMath::IsNearlyEqual(
			Host.FindController(HostHighItemId)
				->GetCurrentOrbitPhaseRadians(),
			HighPhaseBeforeRejected));

	FShanmenControlledWeaponCommandReceipt Launch;
	TestTrue(TEXT("One exact item can leave Orbiting independently"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	TestTrue(TEXT("Later orbit batch advances only the remaining item"),
		Host.TryAdvanceOrbitingInOrder(0.1f, Orbit)
		&& Orbit.IsFullyAdvanced()
		&& Orbit.AttemptedCount == 1
		&& Orbit.Entries[0].ItemInstanceId == HostHighItemId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostFrameOwnerTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.FrameOwnerResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostFrameOwnerTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady)
	{
		AddError(TEXT("Could not prepare P6.9 frame-owner fixture."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult InvalidDelta =
		Host.AdvanceOrbitingFrame(0.0f);
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Empty =
		Host.AdvanceOrbitingFrame(0.1f);
	TestTrue(TEXT("Frame result separates invalid delta from an empty no-op"),
		InvalidDelta.IsValid()
		&& InvalidDelta.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::DeltaInvalid
		&& Empty.IsNoOp()
		&& !Empty.RunId.IsValid()
		&& Empty.BoundCount == 0);

	if (!AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1, UE_PI).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not attach P6.9 frame-owner items."));
		return false;
	}
	TestEqual(TEXT("Both newly attached items are Orbiting"),
		Host.NumOrbiting(), 2);

	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Advanced =
		Host.AdvanceOrbitingFrame(0.25f);
	TestTrue(TEXT("Owner frame advances all Orbiting items in one batch"),
		Advanced.IsAdvanced()
		&& Advanced.RunId == HostRunId
		&& Advanced.BoundCount == 2
		&& Advanced.OrbitingCount == 2
		&& Advanced.Batch.Entries.Num() == 2
		&& Advanced.Batch.Entries[0].ItemInstanceId == HostLowItemId
		&& Advanced.Batch.Entries[1].ItemInstanceId == HostHighItemId);

	const FVector LowBeforeRejected =
		Fixture.Weapons[0]->GetActorLocation();
	const FVector HighBeforeRejected =
		Fixture.Weapons[1]->GetActorLocation();
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Rejected =
		Host.AdvanceOrbitingFrame(0.75f);
	TestTrue(TEXT("Oversized owner frame is an auditable rejection"),
		Rejected.IsValid()
		&& Rejected.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::MovementRejected
		&& Rejected.RunId == HostRunId
		&& Rejected.BoundCount == 2
		&& Rejected.OrbitingCount == 2
		&& Rejected.Batch.AttemptedCount == 0
		&& Fixture.Weapons[0]->GetActorLocation().Equals(LowBeforeRejected)
		&& Fixture.Weapons[1]->GetActorLocation().Equals(
			HighBeforeRejected));

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Canonical Launch removes both items from Orbit cadence"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(
			HostHighItemId, 0, FVector::RightVector, Command)
		&& Host.NumOrbiting() == 0);
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult DirectedNoOp =
		Host.AdvanceOrbitingFrame(0.1f);
	TestTrue(TEXT("A valid Host with no Orbiting items is a clean no-op"),
		DirectedNoOp.IsNoOp()
		&& DirectedNoOp.RunId == HostRunId
		&& DirectedNoOp.BoundCount == 2);

	Fixture.Weapons[0]->SetRootComponent(nullptr);
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Corrupted =
		Host.AdvanceOrbitingFrame(0.1f);
	TestTrue(TEXT("Corrupted non-empty Host is never reported as idle"),
		Corrupted.IsValid()
		&& Corrupted.Status
			== Edemo_mapShanmenControlledWeaponOrbitFrameStatus::HostInvalid
		&& Corrupted.RunId == HostRunId
		&& Corrupted.BoundCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostIndependentLifecycleTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.IndependentContactAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostIndependentLifecycleTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached()
		|| !AttachHostWeapon(
			Fixture, Host, HostHighItemId, 2, 1).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.5 lifecycle fixture."));
		return false;
	}

	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenWorldHitContext LowContext;
	FShanmenWorldHitContext HighContext;
	TestTrue(TEXT("Both item Sessions own independent contact windows"),
		Host.TryLaunch(HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(HostHighItemId, 0, FVector::RightVector, Command)
		&& Host.TryBeginContactWindow(HostLowItemId, LowContext)
		&& Host.TryBeginContactWindow(HostHighItemId, HighContext));

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Low =
		Host.ResolveSweepContact(
			HostLowItemId,
			Fixture.Coordinator,
			MakeHostSweepHit(Fixture));
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult High =
		Host.ResolveOverlapContact(
			HostHighItemId,
			Fixture.Coordinator,
			MakeHostOverlap(Fixture),
			FVector(110.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Two exact items retain distinct impact identities"),
		Low.IsDelivered()
		&& High.IsDelivered()
		&& Low.Impact.GetRequest().Action.GetSourceItemInstanceId()
			== HostLowItemId
		&& High.Impact.GetRequest().Action.GetSourceItemInstanceId()
			== HostHighItemId
		&& Low.Impact.GetRequest().ImpactId
			!= High.Impact.GetRequest().ImpactId);
	TestTrue(TEXT("Both accepted impacts reach one canonical vitality"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality - After.CurrentVitality,
			Low.GetNewlyCommittedDamage()
				+ High.GetNewlyCommittedDamage(),
			0.001f));

	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Duplicate =
		Host.ResolveSweepContact(
			HostLowItemId,
			Fixture.Coordinator,
			MakeHostSweepHit(Fixture));
	const Fdemo_mapShanmenControlledWeaponProductController* HighController =
		Host.FindController(HostHighItemId);
	TestTrue(TEXT("One item's duplicate callback cannot mutate the other"),
		!Duplicate.IsDelivered()
		&& HighController
		&& HighController->GetSession().GetExecution()
			.NumAcceptedImpacts() == 1);

	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("Low item completes without closing the high item"),
		Host.TryEndContactWindow(HostLowItemId)
		&& Host.TryRecallAndComplete(
			HostLowItemId, 1, Recall, Recovery, Completed)
		&& Host.NumActive() == 1);
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt> Interrupted;
	TestTrue(TEXT("Host-wide interrupt terminates only remaining active items"),
		Host.TryInterruptAll(Interrupted)
		&& Interrupted.Num() == 1
		&& Interrupted[0].ItemInstanceId == HostHighItemId
		&& Host.NumActive() == 0);
	HighController = Host.FindController(HostHighItemId);
	TestTrue(TEXT("Interrupt also closes the high item's contact window"),
		HighController
		&& HighController->GetSession().IsTerminal()
		&& !HighController->HasActiveContactWindow());
	TestTrue(TEXT("Terminal items can be retired independently"),
		Host.TryRemoveTerminal(HostLowItemId)
		&& Host.NumBound() == 1
		&& Host.TryRemoveTerminal(HostHighItemId)
		&& Host.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostOrbitThreatTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.OrbitThreatRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostOrbitThreatTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.10 Host Orbit threat fixture."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	FShanmenWorldHitContext ThreatContext;
	TestTrue(TEXT("Host routes threat window to the exact Orbiting item"),
		Host.TryBeginOrbitThreatWindow(HostLowItemId, ThreatContext));
	const Fdemo_mapShanmenControlledWeaponOrbitThreatResult Projected =
		Host.ProjectOrbitThreatOverlap(
			HostLowItemId,
			Fixture.Coordinator,
			MakeHostOverlap(Fixture),
			FVector(80.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Host projection retains exact item and zero-damage boundary"),
		Projected.IsProjected()
		&& Projected.Context.GetAction().GetSourceItemInstanceId()
			== HostLowItemId
		&& FMath::IsNearlyEqual(
			Before.CurrentVitality, After.CurrentVitality));
	TestFalse(TEXT("Unknown item cannot borrow another item's threat window"),
		Host.ProjectOrbitThreatOverlap(
			HostHighItemId,
			Fixture.Coordinator,
			MakeHostOverlap(Fixture),
			FVector(80.0, 20.0, 30.0),
			FVector::BackwardVector).IsProjected());

	FShanmenControlledWeaponCommandReceipt Launch;
	TestFalse(TEXT("Host Launch is fenced while threat sample is active"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	FShanmenDetectorEmissionReceipt ThreatReceipt;
	TestTrue(TEXT("Host returns exact-item canonical threat evidence"),
		Host.TryEndOrbitThreatWindow(HostLowItemId, ThreatReceipt)
		&& ThreatReceipt.IsValid()
		&& ThreatReceipt.GetCandidates().Num() == 1
		&& ThreatReceipt.GetCandidates()[0].TargetEntityId
			== Projected.Candidate.TargetEntityId
		&& ThreatReceipt.GetContext().GetAction().GetSourceItemInstanceId()
			== HostLowItemId);
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Evidence;
	FShanmenControlledWeaponThreatPolicyReceipt ThreatPolicy;
	TestTrue(TEXT("Host captures world evidence for the exact item"),
		Host.TryEvaluateOrbitThreatActors(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			Evidence,
			ThreatPolicy)
		&& Evidence.IsCaptured()
		&& Evidence.TargetEvidence.Num() == 1
		&& Evidence.TargetEvidence[0].GetTargetEntityId()
			== Projected.Candidate.TargetEntityId
		&& ThreatPolicy.IsValid()
		&& ThreatPolicy.NumAcceptedTargets() == 1);
	FShanmenControlledWeaponThreatPolicyReceipt RejectedPolicy;
	TestFalse(TEXT("Unknown item cannot capture another item's targets"),
		Host.TryEvaluateOrbitThreatActors(
			HostHighItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			Evidence,
			RejectedPolicy));
	FShanmenControlledWeaponThreatPresenceReceipt Presence;
	TestTrue(TEXT("Host emits threat presence for the exact routed item"),
		Host.TryBuildOrbitThreatPresenceIntents(
			HostLowItemId, ThreatPolicy, Presence)
		&& Presence.IsValid()
		&& Presence.GetIntents().Num() == 1
		&& Presence.GetIntents()[0].GetSourceItemInstanceId()
			== HostLowItemId
		&& Presence.GetIntents()[0].GetCandidate().TargetEntityId
			== Projected.Candidate.TargetEntityId);
	FShanmenControlledWeaponThreatPresenceReceipt RejectedPresence;
	TestFalse(TEXT("Unknown item cannot borrow another item's presence policy"),
		Host.TryBuildOrbitThreatPresenceIntents(
			HostHighItemId, ThreatPolicy, RejectedPresence));

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult FirstFinalized;
	TestTrue(TEXT("Run Host atomically finalizes exact-item threat evidence"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			FirstFinalized)
		&& FirstFinalized.IsFinalized()
		&& FirstFinalized.GetItemInstanceId() == HostLowItemId
		&& FirstFinalized.GetEvidence().TargetEvidence.Num() == 1
		&& FirstFinalized.GetPresence().GetIntents().Num() == 1
		&& FirstFinalized.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		&& FirstFinalized.GetConsumption().GetReceipts().Num() == 1
		&& FirstFinalized.GetConsumption().GetReceipts()[0].GetIntent()
			.GetCandidate().TargetEntityId
			== Projected.Candidate.TargetEntityId
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult ReplayFinalized;
	TestTrue(TEXT("Atomic threat finalization is exact-replay idempotent"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			ReplayFinalized)
		&& ReplayFinalized.IsFinalized()
		&& ReplayFinalized.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::AlreadyConsumed
		&& ReplayFinalized.GetConsumption().GetReceipts()[0]
			.GetAuthorityRevision()
			== FirstFinalized.GetConsumption().GetReceipts()[0]
				.GetAuthorityRevision()
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult RejectedFinalized;
	TestFalse(TEXT("Unknown item cannot finalize another item's threat sample"),
		Host.TryFinalizeOrbitThreatSample(
			HostHighItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{ Fixture.Enemy },
			RejectedFinalized));
	TestFalse(TEXT("Incomplete world evidence cannot partially consume"),
		Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			ThreatReceipt,
			{},
			RejectedFinalized));
	TestTrue(TEXT("Rejected finalization preserves the authority ledger"),
		!RejectedFinalized.IsFinalized()
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);

	FShanmenWorldHitContext EmptyThreatContext;
	FShanmenDetectorEmissionReceipt EmptyThreatReceipt;
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult EmptyFinalized;
	TestTrue(TEXT("Explicit empty threat samples finalize as a zero-effect no-op"),
		Host.TryBeginOrbitThreatWindow(HostLowItemId, EmptyThreatContext)
		&& Host.TryEndOrbitThreatWindow(HostLowItemId, EmptyThreatReceipt)
		&& EmptyThreatReceipt.GetCandidates().IsEmpty()
		&& Host.TryFinalizeOrbitThreatSample(
			HostLowItemId,
			Fixture.Coordinator,
			EmptyThreatReceipt,
			{},
			EmptyFinalized)
		&& EmptyFinalized.IsFinalized()
		&& EmptyFinalized.GetEvidence().TargetEvidence.IsEmpty()
		&& EmptyFinalized.GetPresence().GetIntents().IsEmpty()
		&& EmptyFinalized.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 1);
	FShanmenTargetVitalitySnapshot AfterConsume;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterConsume));
	TestTrue(TEXT("Presence consumption mutates no vitality or impact ledger"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality, AfterConsume.CurrentVitality)
		&& Host.FindController(HostLowItemId)
		&& Host.FindController(HostLowItemId)->GetSession().GetExecution()
			.NumAcceptedImpacts() == 0);
	TestTrue(TEXT("Consumed presence leaves the item free to Launch"),
		Host.TryLaunch(
			HostLowItemId, 0, FVector::ForwardVector, Launch));
	FShanmenWorldHitContext DirectedContext;
	TestTrue(TEXT("Host-directed window follows the same detector ordinal"),
		Host.TryBeginContactWindow(HostLowItemId, DirectedContext)
		&& DirectedContext.GetHitOrdinal()
			== EmptyThreatContext.GetHitOrdinal() + 1
		&& Host.TryEndContactWindow(HostLowItemId));
	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("Last-item retirement clears the Run-scoped authority"),
		Host.TryRecallAndComplete(
			HostLowItemId, 1, Recall, Recovery, Completed)
		&& Host.NumConsumedThreatPresenceIntents() == 1
		&& Host.TryRemoveTerminal(HostLowItemId)
		&& Host.IsEmpty()
		&& Host.NumConsumedThreatPresenceIntents() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunHostAtomicFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunHost.AttachFencesAndAtomicInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunHostAtomicFenceTest::RunTest(
	const FString&)
{
	FControlledWeaponHostFixture Fixture;
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachHostWeapon(
			Fixture, Host, HostLowItemId, 1, 0).IsAttached())
	{
		AddError(TEXT("Could not prepare P6.5 fence fixture."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponHostAttachResult DuplicateItem =
		AttachHostWeapon(Fixture, Host, HostLowItemId, 2, 1);
	const Fdemo_mapShanmenControlledWeaponHostAttachResult DuplicateActor =
		Host.TryAttach(
			MakeHostPrepared(Fixture.Coordinator, HostHighItemId, 2),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[0],
			Fixture.WeaponRoots[0],
			MakeHostMotion());
	const Fdemo_mapShanmenControlledWeaponHostAttachResult
		DuplicateActivation = Host.TryAttach(
			MakeHostPrepared(Fixture.Coordinator, HostThirdItemId, 1),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[2],
			Fixture.WeaponRoots[2],
			MakeHostMotion());
	TestTrue(TEXT("Exact item identity cannot be attached twice"),
		DuplicateItem.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::ItemAlreadyBound);
	TestTrue(TEXT("One physical Actor cannot represent two items"),
		DuplicateActor.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::WeaponActorAlreadyBound);
	TestTrue(TEXT("One activation identity cannot represent two items"),
		DuplicateActivation.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::ActivationAlreadyBound);
	TestTrue(TEXT("Rejected attachments leave the original host unchanged"),
		Host.IsValid()
		&& Host.NumBound() == 1
		&& Host.FindController(HostLowItemId) != nullptr);

	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenWorldHitContext Context;
	TestTrue(TEXT("Interrupt fixture owns one live contact window"),
		Host.TryLaunch(HostLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryBeginContactWindow(HostLowItemId, Context));
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt> Interrupted;
	TestTrue(TEXT("Host interrupt is an atomic terminal transition"),
		Host.TryInterruptAll(Interrupted)
		&& Interrupted.Num() == 1
		&& Interrupted[0].ItemInstanceId == HostLowItemId
		&& Host.IsValid()
		&& Host.NumActive() == 0);
	TestFalse(TEXT("A second interrupt has no partial work to commit"),
		Host.TryInterruptAll(Interrupted));
	TestTrue(TEXT("No-op interrupt returns an empty receipt set"),
		Interrupted.IsEmpty());
	TestTrue(TEXT("Terminal retirement resets the empty host binding"),
		Host.TryRemoveTerminal(HostLowItemId)
		&& Host.IsEmpty()
		&& !Host.IsValid());
	return true;
}

#endif
