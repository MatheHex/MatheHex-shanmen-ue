#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponRunLifecycle.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	const FGuid LifecycleRunId(0xD3660001, 0, 0, 1);
	const FGuid LifecycleOtherRunId(0xD3660002, 0, 0, 1);
	const FGuid LifecycleLowItemId(0xD3660010, 0, 0, 1);
	const FGuid LifecycleHighItemId(0xD3660011, 0, 0, 1);
	const FGuid LifecycleOwnerId(0xD3660020, 0, 0, 1);

	struct FControlledWeaponLifecycleFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		TArray<AActor*> Weapons;
		TArray<UBoxComponent*> WeaponRoots;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		explicit FControlledWeaponLifecycleFixture(const FGuid& RunId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P66PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P66PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}
			for (int32 Index = 0; Index < 2; ++Index)
			{
				AActor* Weapon = NewObject<AActor>(GetTransientPackage());
				UBoxComponent* Root = Weapon
					? NewObject<UBoxComponent>(
						Weapon,
						FName(*FString::Printf(
							TEXT("P66WeaponRoot%d"), Index)))
					: nullptr;
				if (Weapon && Root)
				{
					Weapon->SetRootComponent(Root);
				}
				Weapons.Add(Weapon);
				WeaponRoots.Add(Root);
			}
			bReady = Pawn && PlayerRoot && PlayerHealth
				&& !Weapons.Contains(nullptr)
				&& !WeaponRoots.Contains(nullptr)
				&& Coordinator.TryBeginRun(
					RunId, Pawn, PlayerHealth, Diagnostic);
		}
	};

	FShanmenContentStamp MakeLifecycleContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.6");
		Content.Digest = TEXT("P6.6.ControlledWeaponRunLifecycle.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakeLifecyclePrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence)
	{
		const uint32 SequenceBits = static_cast<uint32>(ActivationSequence);
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId =
			FGuid(0xD3660100 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = LifecycleOwnerId;
		Prepared.Evidence.ItemInstanceId = ItemInstanceId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.6");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3660200 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 14;
		Prepared.Evidence.ItemRevision = 8;
		Prepared.Evidence.Content = MakeLifecycleContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = LifecycleOwnerId;
		ActionCapture.SourceEntityId = Coordinator.GetPlayerEntityId();
		ActionCapture.SourceItemInstanceId = ItemInstanceId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeLifecycleContent();
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
			TEXT("Detector.ControlledWeapon.P6.6.%lld"),
			ActivationSequence));
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.6.LifecycleTest");
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

	Fdemo_mapShanmenControlledWeaponMotionCapture MakeLifecycleMotion()
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

	bool AttachLifecycleWeapon(
		FControlledWeaponLifecycleFixture& Fixture,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence,
		int32 WeaponIndex)
	{
		return Host.TryAttach(
			MakeLifecyclePrepared(
				Fixture.Coordinator,
				ItemInstanceId,
				ActivationSequence),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[WeaponIndex],
			Fixture.WeaponRoots[WeaponIndex],
			MakeLifecycleMotion()).IsAttached();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponMixedStateTeardownTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle.MixedStateTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponMixedStateTeardownTest::RunTest(
	const FString&)
{
	FControlledWeaponLifecycleFixture Fixture(LifecycleRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachLifecycleWeapon(
			Fixture, Host, LifecycleLowItemId, 1, 0)
		|| !AttachLifecycleWeapon(
			Fixture, Host, LifecycleHighItemId, 2, 1))
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.6 mixed-state fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Both exact items enter independent Directed states"),
		Host.TryLaunch(
			LifecycleLowItemId, 0, FVector::ForwardVector, Command)
		&& Host.TryLaunch(
			LifecycleHighItemId, 0, FVector::RightVector, Command));
	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("One item can already be terminal before Run teardown"),
		Host.TryRecallAndComplete(
			LifecycleLowItemId, 1, Recall, Recovery, Completed)
		&& Host.NumActive() == 1);

	const Fdemo_mapShanmenControlledWeaponRunEndResult Result =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			Host, Fixture.Coordinator);
	TestTrue(TEXT("Active items interrupt before all terminal items retire"),
		Result.IsEnded()
		&& Result.RunId == LifecycleRunId
		&& Result.BoundItemCount == 2
		&& Result.ActiveItemCount == 1
		&& Result.InterruptedItemCount == 1
		&& Result.RetiredItemCount == 2
		&& Result.InterruptReceipts.Num() == 1
		&& Result.InterruptReceipts[0].ItemInstanceId
			== LifecycleHighItemId);
	TestTrue(TEXT("Host clears before Coordinator identity release completes"),
		Host.IsEmpty()
		&& !Fixture.Coordinator.IsActive()
		&& !Fixture.PlayerHealth->IsCombatEntityBound());

	const Fdemo_mapShanmenControlledWeaponRunEndResult Replay =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			Host, Fixture.Coordinator);
	TestTrue(TEXT("A released Run cannot be ended a second time"),
		Replay.Status
			== Edemo_mapShanmenControlledWeaponRunEndStatus::CoordinatorNotActive
		&& Host.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunMismatchFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle.RunMismatchFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunMismatchFenceTest::RunTest(
	const FString&)
{
	FControlledWeaponLifecycleFixture First(LifecycleRunId);
	FControlledWeaponLifecycleFixture Second(LifecycleOtherRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!First.bReady || !Second.bReady
		|| !AttachLifecycleWeapon(
			First, Host, LifecycleLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.6 mismatch fixture."));
		return false;
	}
	FShanmenControlledWeaponCommandReceipt Command;
	check(Host.TryLaunch(
		LifecycleLowItemId, 0, FVector::ForwardVector, Command));

	const Fdemo_mapShanmenControlledWeaponRunEndResult Result =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			Host, Second.Coordinator);
	TestTrue(TEXT("A different Coordinator Run fails before any mutation"),
		Result.Status
			== Edemo_mapShanmenControlledWeaponRunEndStatus::HostRunMismatch
		&& Host.IsValid()
		&& Host.NumActive() == 1
		&& First.Coordinator.IsActive()
		&& Second.Coordinator.IsActive());
	Host.Reset();
	First.Coordinator.Reset();
	Second.Coordinator.Reset();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponCoordinatorRejectionTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunLifecycle.CoordinatorRejectionPreservesHost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponCoordinatorRejectionTest::RunTest(
	const FString&)
{
	FControlledWeaponLifecycleFixture Fixture(LifecycleRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	if (!Fixture.bReady
		|| !AttachLifecycleWeapon(
			Fixture, Host, LifecycleLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.6 Coordinator rejection fixture."));
		return false;
	}
	FShanmenControlledWeaponCommandReceipt Command;
	check(Host.TryLaunch(
		LifecycleLowItemId, 0, FVector::ForwardVector, Command));

	const FGuid ExpectedPlayerEntityId =
		Fixture.Coordinator.GetPlayerEntityId();
	const FGuid WrongPlayerEntityId(0xD36600FF, 0, 0, 1);
	if (!Fixture.PlayerHealth->TryEndCombatEntityBinding(
			ExpectedPlayerEntityId)
		|| !Fixture.PlayerHealth->TryBindCombatEntity(
			WrongPlayerEntityId))
	{
		AddError(TEXT("Could not inject the P6.6 Coordinator identity mismatch."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponRunEndResult Rejected =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			Host, Fixture.Coordinator);
	TestTrue(TEXT("Coordinator rejection discards the staged Host teardown"),
		Rejected.Status
			== Edemo_mapShanmenControlledWeaponRunEndStatus::CoordinatorEndRejected
		&& Host.IsValid()
		&& Host.NumActive() == 1
		&& Fixture.Coordinator.IsActive());

	TestTrue(TEXT("Repairing identity permits an exact retry"),
		Fixture.PlayerHealth->TryEndCombatEntityBinding(WrongPlayerEntityId)
		&& Fixture.PlayerHealth->TryBindCombatEntity(
			ExpectedPlayerEntityId));
	const Fdemo_mapShanmenControlledWeaponRunEndResult Retried =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			Host, Fixture.Coordinator);
	TestTrue(TEXT("Retry commits one interruption and Run release"),
		Retried.IsEnded()
		&& Retried.InterruptedItemCount == 1
		&& Retried.RetiredItemCount == 1
		&& Host.IsEmpty()
		&& !Fixture.Coordinator.IsActive());
	return true;
}

#endif
