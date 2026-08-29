#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponRunCommandRouter.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	const FGuid RouterRunId(0xD3670001, 0, 0, 1);
	const FGuid RouterOtherRunId(0xD3670002, 0, 0, 1);
	const FGuid RouterLowItemId(0xD3670010, 0, 0, 1);
	const FGuid RouterHighItemId(0xD3670011, 0, 0, 1);
	const FGuid RouterUnknownItemId(0xD367001F, 0, 0, 1);
	const FGuid RouterOwnerId(0xD3670020, 0, 0, 1);
	const FGuid RouterLaunchIntentId(0xD3670100, 0, 0, 1);
	const FGuid RouterRedirectIntentId(0xD3670101, 0, 0, 1);
	const FGuid RouterRecallIntentId(0xD3670102, 0, 0, 1);

	struct FControlledWeaponRouterFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		TArray<AActor*> Weapons;
		TArray<UBoxComponent*> WeaponRoots;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		explicit FControlledWeaponRouterFixture(const FGuid& RunId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P67PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P67PlayerHealth"))
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
							TEXT("P67WeaponRoot%d"), Index)))
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

	FShanmenContentStamp MakeRouterContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.7");
		Content.Digest = TEXT("P6.7.ControlledWeaponRunCommandRouter.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakeRouterPrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence)
	{
		const uint32 SequenceBits = static_cast<uint32>(ActivationSequence);
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId =
			FGuid(0xD3670200 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = RouterOwnerId;
		Prepared.Evidence.ItemInstanceId = ItemInstanceId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.7");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3670300 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 15;
		Prepared.Evidence.ItemRevision = 9;
		Prepared.Evidence.Content = MakeRouterContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = RouterOwnerId;
		ActionCapture.SourceEntityId = Coordinator.GetPlayerEntityId();
		ActionCapture.SourceItemInstanceId = ItemInstanceId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeRouterContent();
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
			TEXT("Detector.ControlledWeapon.P6.7.%lld"),
			ActivationSequence));
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.7.RouterTest");
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

	Fdemo_mapShanmenControlledWeaponMotionCapture MakeRouterMotion()
	{
		Fdemo_mapShanmenControlledWeaponMotionCapture Motion;
		Motion.DirectedSpeed = 400.0f;
		Motion.MaximumStepSeconds = 0.5f;
		return Motion;
	}

	bool AttachRouterWeapon(
		FControlledWeaponRouterFixture& Fixture,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence,
		int32 WeaponIndex)
	{
		return Host.TryAttach(
			MakeRouterPrepared(
				Fixture.Coordinator,
				ItemInstanceId,
				ActivationSequence),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[WeaponIndex],
			Fixture.WeaponRoots[WeaponIndex],
			MakeRouterMotion()).IsAttached();
	}

	Fdemo_mapShanmenControlledWeaponRunCommandIntent MakeRouterIntent(
		const FGuid& IntentId,
		const FGuid& RunId,
		EShanmenControlledWeaponCommandKind Kind,
		const TArray<FGuid>& Items,
		const FVector& Direction)
	{
		Fdemo_mapShanmenControlledWeaponRunCommandIntent Intent;
		check(Fdemo_mapShanmenControlledWeaponRunCommandIntent::TryCapture(
			IntentId, RunId, Kind, Items, Direction, Intent));
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunCommandStableBatchTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter.StableAtomicBatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunCommandStableBatchTest::RunTest(
	const FString&)
{
	FControlledWeaponRouterFixture Fixture(RouterRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	Fdemo_mapShanmenControlledWeaponRunCommandRouter Router;
	if (!Fixture.bReady
		|| !AttachRouterWeapon(
			Fixture, Host, RouterHighItemId, 2, 1)
		|| !AttachRouterWeapon(
			Fixture, Host, RouterLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.7 stable batch fixture."));
		return false;
	}

	const TArray<FGuid> ReverseTargets = {
		RouterHighItemId, RouterLowItemId };
	const Fdemo_mapShanmenControlledWeaponRunCommandResult Launch =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeRouterIntent(
				RouterLaunchIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Launch,
				ReverseTargets,
				FVector(2.0, 0.0, 0.0)));
	TestTrue(TEXT("Reverse target input commits in stable exact-item order"),
		Launch.IsAccepted()
		&& !Launch.IsReplay()
		&& Launch.Entries.Num() == 2
		&& Launch.Entries[0].ItemInstanceId == RouterLowItemId
		&& Launch.Entries[1].ItemInstanceId == RouterHighItemId
		&& Launch.Entries[0].ExpectedSequence == 0
		&& Launch.Entries[1].ExpectedSequence == 0
		&& Launch.Entries[0].Command.GetDirectionAfter()
			.Equals(FVector::ForwardVector)
		&& Launch.Entries[1].Command.GetDirectionAfter()
			.Equals(FVector::ForwardVector));

	const Fdemo_mapShanmenControlledWeaponRunCommandResult Redirect =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeRouterIntent(
				RouterRedirectIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Redirect,
				ReverseTargets,
				FVector::RightVector));
	TestTrue(TEXT("Router derives each next command sequence internally"),
		Redirect.IsAccepted()
		&& Redirect.Entries.Num() == 2
		&& Redirect.Entries[0].ExpectedSequence == 1
		&& Redirect.Entries[1].ExpectedSequence == 1);

	const Fdemo_mapShanmenControlledWeaponRunCommandResult Recall =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeRouterIntent(
				RouterRecallIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Recall,
				ReverseTargets,
				FVector::ZeroVector));
	TestTrue(TEXT("Atomic group recall closes every selected action"),
		Recall.IsAccepted()
		&& Recall.Entries.Num() == 2
		&& Recall.Entries[0].ExpectedSequence == 2
		&& Recall.Entries[1].ExpectedSequence == 2
		&& Host.NumActive() == 0
		&& Router.IsValid()
		&& Router.NumProcessedIntents() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunCommandReplayTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter.ReplayAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunCommandReplayTest::RunTest(
	const FString&)
{
	FControlledWeaponRouterFixture Fixture(RouterRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	Fdemo_mapShanmenControlledWeaponRunCommandRouter Router;
	if (!Fixture.bReady
		|| !AttachRouterWeapon(
			Fixture, Host, RouterLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.7 replay fixture."));
		return false;
	}

	const TArray<FGuid> Target = { RouterLowItemId };
	const Fdemo_mapShanmenControlledWeaponRunCommandIntent FirstIntent =
		MakeRouterIntent(
			RouterLaunchIntentId,
			RouterRunId,
			EShanmenControlledWeaponCommandKind::Launch,
			Target,
			FVector::ForwardVector);
	const Fdemo_mapShanmenControlledWeaponRunCommandResult First =
		Router.TryRoute(Host, Fixture.Coordinator, FirstIntent);
	const Fdemo_mapShanmenControlledWeaponRunCommandIntent Equivalent =
		MakeRouterIntent(
			RouterLaunchIntentId,
			RouterRunId,
			EShanmenControlledWeaponCommandKind::Launch,
			Target,
			FVector(3.0, 0.0, 0.0));
	const Fdemo_mapShanmenControlledWeaponRunCommandResult Replay =
		Router.TryRoute(Host, Fixture.Coordinator, Equivalent);
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Host.FindController(RouterLowItemId);
	TestTrue(TEXT("Equivalent normalized intent replays exact receipts"),
		First.IsAccepted()
		&& Replay.IsAccepted()
		&& Replay.IsReplay()
		&& Replay.Entries[0].Command.GetCommandId()
			== First.Entries[0].Command.GetCommandId()
		&& Router.NumProcessedIntents() == 1
		&& Controller
		&& Controller->GetSession().GetExecution()
			.GetNextCommandSequence() == 1);

	const Fdemo_mapShanmenControlledWeaponRunCommandIntent ConflictIntent =
		MakeRouterIntent(
			RouterLaunchIntentId,
			RouterRunId,
			EShanmenControlledWeaponCommandKind::Redirect,
			Target,
			FVector::RightVector);
	const Fdemo_mapShanmenControlledWeaponRunCommandResult Conflict =
		Router.TryRoute(Host, Fixture.Coordinator, ConflictIntent);
	Controller = Host.FindController(RouterLowItemId);
	TestTrue(TEXT("Conflicting IntentId cannot consume another sequence"),
		Conflict.Status
			== Edemo_mapShanmenControlledWeaponRunCommandStatus::IntentIdConflict
		&& Router.NumProcessedIntents() == 1
		&& Controller
		&& Controller->GetSession().GetExecution()
			.GetNextCommandSequence() == 1);

	const Fdemo_mapShanmenControlledWeaponRunCommandResult Next =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeRouterIntent(
				RouterRedirectIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Redirect,
				Target,
				FVector::RightVector));
	TestTrue(TEXT("A new intent receives the true next item sequence"),
		Next.IsAccepted()
		&& Next.Entries[0].ExpectedSequence == 1
		&& Router.NumProcessedIntents() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunCommandRollbackTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter.AtomicRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunCommandRollbackTest::RunTest(
	const FString&)
{
	FControlledWeaponRouterFixture Fixture(RouterRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	Fdemo_mapShanmenControlledWeaponRunCommandRouter Router;
	if (!Fixture.bReady
		|| !AttachRouterWeapon(
			Fixture, Host, RouterLowItemId, 1, 0)
		|| !AttachRouterWeapon(
			Fixture, Host, RouterHighItemId, 2, 1))
	{
		AddError(TEXT("Could not prepare P6.7 rollback fixture."));
		return false;
	}
	FShanmenControlledWeaponCommandReceipt DirectLaunch;
	check(Host.TryLaunch(
		RouterHighItemId, 0, FVector::RightVector, DirectLaunch));

	const TArray<FGuid> Targets = {
		RouterHighItemId, RouterLowItemId };
	const Fdemo_mapShanmenControlledWeaponRunCommandResult Rejected =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeRouterIntent(
				RouterLaunchIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Launch,
				Targets,
				FVector::ForwardVector));
	const Fdemo_mapShanmenControlledWeaponProductController* Low =
		Host.FindController(RouterLowItemId);
	const Fdemo_mapShanmenControlledWeaponProductController* High =
		Host.FindController(RouterHighItemId);
	TestTrue(TEXT("Later item rejection discards earlier staged mutation"),
		Rejected.Status
			== Edemo_mapShanmenControlledWeaponRunCommandStatus::CommandRejected
		&& Rejected.Entries.IsEmpty()
		&& Router.IsEmpty()
		&& Low
		&& Low->GetSession().GetExecution().GetState()
			== EShanmenControlledWeaponState::Orbiting
		&& Low->GetSession().GetExecution().GetNextCommandSequence() == 0
		&& High
		&& High->GetSession().GetExecution().GetState()
			== EShanmenControlledWeaponState::Directed
		&& High->GetSession().GetExecution().GetNextCommandSequence() == 1);

	const TArray<FGuid> UnknownTarget = { RouterUnknownItemId };
	const Fdemo_mapShanmenControlledWeaponRunCommandResult Unknown =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeRouterIntent(
				RouterRedirectIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Launch,
				UnknownTarget,
				FVector::ForwardVector));
	TestTrue(TEXT("Unknown exact item fails before Host mutation"),
		Unknown.Status
			== Edemo_mapShanmenControlledWeaponRunCommandStatus::TargetNotBound
		&& Router.IsEmpty()
		&& Low->GetSession().GetExecution().GetNextCommandSequence() == 0);

	Fdemo_mapShanmenControlledWeaponRunCommandIntent Duplicate;
	const TArray<FGuid> DuplicateTargets = {
		RouterLowItemId, RouterLowItemId };
	TestFalse(TEXT("Duplicate exact targets cannot be canonicalized away"),
		Fdemo_mapShanmenControlledWeaponRunCommandIntent::TryCapture(
			RouterRecallIntentId,
			RouterRunId,
			EShanmenControlledWeaponCommandKind::Recall,
			DuplicateTargets,
			FVector::ZeroVector,
			Duplicate));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRunCommandIdentityTest,
	"Shanmen.0_0_10.Product.ControlledWeaponRunCommandRouter.RunIdentityFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponRunCommandIdentityTest::RunTest(
	const FString&)
{
	FControlledWeaponRouterFixture First(RouterRunId);
	FControlledWeaponRouterFixture Second(RouterOtherRunId);
	Fdemo_mapShanmenControlledWeaponRunHost FirstHost;
	Fdemo_mapShanmenControlledWeaponRunHost SecondHost;
	Fdemo_mapShanmenControlledWeaponRunCommandRouter Router;
	if (!First.bReady || !Second.bReady
		|| !AttachRouterWeapon(
			First, FirstHost, RouterLowItemId, 1, 0)
		|| !AttachRouterWeapon(
			Second, SecondHost, RouterHighItemId, 2, 0))
	{
		AddError(TEXT("Could not prepare P6.7 identity fixture."));
		return false;
	}

	const TArray<FGuid> FirstTarget = { RouterLowItemId };
	const Fdemo_mapShanmenControlledWeaponRunCommandResult Mixed =
		Router.TryRoute(
			FirstHost,
			Second.Coordinator,
			MakeRouterIntent(
				RouterLaunchIntentId,
				RouterOtherRunId,
				EShanmenControlledWeaponCommandKind::Launch,
				FirstTarget,
				FVector::ForwardVector));
	TestTrue(TEXT("Intent, Host, and Coordinator must name one Run"),
		Mixed.Status
			== Edemo_mapShanmenControlledWeaponRunCommandStatus::RunMismatch
		&& Router.IsEmpty()
		&& FirstHost.FindController(RouterLowItemId)
			->GetSession().GetExecution().GetNextCommandSequence() == 0);

	const Fdemo_mapShanmenControlledWeaponRunCommandResult FirstAccepted =
		Router.TryRoute(
			FirstHost,
			First.Coordinator,
			MakeRouterIntent(
				RouterLaunchIntentId,
				RouterRunId,
				EShanmenControlledWeaponCommandKind::Launch,
				FirstTarget,
				FVector::ForwardVector));
	const TArray<FGuid> SecondTarget = { RouterHighItemId };
	const Fdemo_mapShanmenControlledWeaponRunCommandResult CrossRun =
		Router.TryRoute(
			SecondHost,
			Second.Coordinator,
			MakeRouterIntent(
				RouterRedirectIntentId,
				RouterOtherRunId,
				EShanmenControlledWeaponCommandKind::Launch,
				SecondTarget,
				FVector::RightVector));
	TestTrue(TEXT("A bound Router cannot cross into another Run"),
		FirstAccepted.IsAccepted()
		&& CrossRun.Status
			== Edemo_mapShanmenControlledWeaponRunCommandStatus::RouterRunMismatch
		&& Router.IsValid()
		&& Router.GetRunId() == RouterRunId
		&& SecondHost.FindController(RouterHighItemId)
			->GetSession().GetExecution().GetNextCommandSequence() == 0);
	Router.Reset();
	TestTrue(TEXT("Explicit Run boundary clears replay identity"),
		Router.IsEmpty() && Router.IsValid());
	FirstHost.Reset();
	SecondHost.Reset();
	First.Coordinator.Reset();
	Second.Coordinator.Reset();
	return true;
}

#endif
