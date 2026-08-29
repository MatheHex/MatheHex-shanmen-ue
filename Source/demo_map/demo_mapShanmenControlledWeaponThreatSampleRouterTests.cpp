#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponThreatSampleRouter.h"

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
	const FGuid SampleRunId(0xD3820001, 0, 0, 1);
	const FGuid SampleOtherRunId(0xD3820002, 0, 0, 1);
	const FGuid SampleLowItemId(0xD3820010, 0, 0, 1);
	const FGuid SampleHighItemId(0xD3820011, 0, 0, 1);
	const FGuid SampleOwnerId(0xD3820020, 0, 0, 1);
	const FGuid SampleIntent0(0xD3820100, 0, 0, 1);
	const FGuid SampleIntent1(0xD3820101, 0, 0, 1);
	const FGuid SampleIntent2(0xD3820102, 0, 0, 1);
	const FGuid SampleConflictIntent(0xD382010F, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindSampleEnemyDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeSampleEncounterIdentity(
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

	struct FThreatSampleRouterFixture
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

		explicit FThreatSampleRouterFixture(const FGuid& RunId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P622PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P622PlayerHealth"))
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
							TEXT("P622WeaponRoot%d"), Index)))
					: nullptr;
				if (Weapon && Root)
				{
					Weapon->SetRootComponent(Root);
				}
				Weapons.Add(Weapon);
				WeaponRoots.Add(Root);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindSampleEnemyDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P622EnemyIdentity"))
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
					MakeSampleEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					RunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	FShanmenContentStamp MakeSampleContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.22");
		Content.Digest = TEXT("P6.22.ControlledWeaponThreatSampleRouter.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakeSamplePrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence)
	{
		const uint32 SequenceBits = static_cast<uint32>(ActivationSequence);
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId =
			FGuid(0xD3820200 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = SampleOwnerId;
		Prepared.Evidence.ItemInstanceId = ItemInstanceId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.22");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3820300 + SequenceBits, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 22;
		Prepared.Evidence.ItemRevision = 6;
		Prepared.Evidence.Content = MakeSampleContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = SampleOwnerId;
		ActionCapture.SourceEntityId = Coordinator.GetPlayerEntityId();
		ActionCapture.SourceItemInstanceId = ItemInstanceId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeSampleContent();
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
			TEXT("Detector.ControlledWeapon.P6.22.%lld"),
			ActivationSequence));
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.22.SampleRouterTest");
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

	Fdemo_mapShanmenControlledWeaponMotionCapture MakeSampleMotion()
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

	bool AttachSampleWeapon(
		FThreatSampleRouterFixture& Fixture,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const FGuid& ItemInstanceId,
		int64 ActivationSequence,
		int32 WeaponIndex)
	{
		return Host.TryAttach(
			MakeSamplePrepared(
				Fixture.Coordinator,
				ItemInstanceId,
				ActivationSequence),
			Fixture.Coordinator,
			Fixture.Pawn,
			Fixture.Weapons[WeaponIndex],
			Fixture.WeaponRoots[WeaponIndex],
			MakeSampleMotion()).IsAttached();
	}

	Fdemo_mapShanmenControlledWeaponOrbitThreatContact MakeSampleContact(
		const FThreatSampleRouterFixture& Fixture,
		const FVector& Location = FVector(80.0, 20.0, 30.0))
	{
		Fdemo_mapShanmenControlledWeaponOrbitThreatContact Contact;
		Contact.Overlap.OverlapObjectHandle =
			FActorInstanceHandle(Fixture.Enemy);
		Contact.Overlap.Component = Fixture.GetEnemyRoot();
		Contact.Overlap.ItemIndex = 0;
		Contact.ContactLocation = Location;
		Contact.ContactNormal = FVector::BackwardVector;
		return Contact;
	}

	Fdemo_mapShanmenControlledWeaponThreatSampleRequest MakeSampleRequest(
		const FGuid& ItemInstanceId,
		const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>&
			Contacts)
	{
		Fdemo_mapShanmenControlledWeaponThreatSampleRequest Request;
		Request.ItemInstanceId = ItemInstanceId;
		Request.Contacts = Contacts;
		return Request;
	}

	Fdemo_mapShanmenControlledWeaponThreatSampleIntent MakeSampleIntent(
		const FGuid& IntentId,
		const FGuid& RunId,
		int64 SampleSequence,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>&
			Requests)
	{
		Fdemo_mapShanmenControlledWeaponThreatSampleIntent Intent;
		check(Fdemo_mapShanmenControlledWeaponThreatSampleIntent::TryCapture(
			IntentId,
			RunId,
			SampleSequence,
			Coordinator,
			Requests,
			Intent));
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponThreatSampleStableReplayTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatSampleRouter.StableReplayAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponThreatSampleStableReplayTest::RunTest(
	const FString&)
{
	FThreatSampleRouterFixture Fixture(SampleRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	Fdemo_mapShanmenControlledWeaponThreatSampleRouter Router;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachSampleWeapon(
			Fixture, Host, SampleHighItemId, 2, 1)
		|| !AttachSampleWeapon(
			Fixture, Host, SampleLowItemId, 1, 0))
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.22 stable fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponOrbitThreatContact Contact =
		MakeSampleContact(Fixture);
	const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>
		ReverseRequests = {
			MakeSampleRequest(SampleHighItemId, { Contact }),
			MakeSampleRequest(SampleLowItemId, { Contact })
		};
	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent Intent =
		MakeSampleIntent(
			SampleIntent0,
			SampleRunId,
			0,
			Fixture.Coordinator,
			ReverseRequests);
	TestTrue(TEXT("Capture canonicalizes the explicit item subset"),
		Intent.IsValid()
		&& Intent.GetItemIdentities().Num() == 2
		&& Intent.GetItemIdentities()[0].ItemInstanceId == SampleLowItemId
		&& Intent.GetItemIdentities()[1].ItemInstanceId == SampleHighItemId);

	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Applied =
		Router.TryRoute(Host, Fixture.Coordinator, Intent);
	TestTrue(TEXT("Sequence zero commits one stable whole-Host sample"),
		Applied.IsAccepted()
		&& !Applied.IsReplay()
		&& Applied.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::Applied
		&& Applied.Batch.Entries.Num() == 2
		&& Applied.Batch.Entries[0].ItemInstanceId == SampleLowItemId
		&& Applied.Batch.Entries[1].ItemInstanceId == SampleHighItemId
		&& Applied.Batch.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& Applied.Batch.Entries[1].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& Host.NumConsumedThreatPresenceIntents() == 2
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2
		&& Router.IsValid()
		&& Router.GetRunId() == SampleRunId
		&& Router.GetNextSampleSequence() == 1
		&& Router.NumAcceptedSamples() == 1);

	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent ReplayIntent =
		MakeSampleIntent(
			SampleIntent0,
			SampleRunId,
			0,
			Fixture.Coordinator,
			ReverseRequests);
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Replayed =
		Router.TryRoute(Host, Fixture.Coordinator, ReplayIntent);
	TestTrue(TEXT("Exact latest replay returns receipts without Host mutation"),
		Replayed.IsAccepted()
		&& Replayed.IsReplay()
		&& Replayed.Batch.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& Host.NumConsumedThreatPresenceIntents() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 2
		&& Router.GetNextSampleSequence() == 1);

	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent ConflictIntent =
		MakeSampleIntent(
			SampleIntent0,
			SampleRunId,
			0,
			Fixture.Coordinator,
			{
				MakeSampleRequest(SampleHighItemId, {}),
				MakeSampleRequest(SampleLowItemId, {})
			});
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Conflict =
		Router.TryRoute(Host, Fixture.Coordinator, ConflictIntent);
	TestTrue(TEXT("Latest sequence identity cannot change its payload"),
		Conflict.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::
				IntentConflict
		&& !Conflict.IsAccepted()
		&& Host.NumConsumedThreatPresenceIntents() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Router.GetNextSampleSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponThreatSampleAtomicRetryTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatSampleRouter.AtomicFailureAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponThreatSampleAtomicRetryTest::RunTest(
	const FString&)
{
	FThreatSampleRouterFixture Fixture(SampleRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	Fdemo_mapShanmenControlledWeaponThreatSampleRouter Router;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot()
		|| !AttachSampleWeapon(
			Fixture, Host, SampleHighItemId, 2, 1)
		|| !AttachSampleWeapon(
			Fixture, Host, SampleLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.22 rollback fixture."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponOrbitThreatContact Contact =
		MakeSampleContact(Fixture);
	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent RejectedIntent =
		MakeSampleIntent(
			SampleIntent0,
			SampleRunId,
			0,
			Fixture.Coordinator,
			{
				MakeSampleRequest(SampleHighItemId, { Contact, Contact }),
				MakeSampleRequest(SampleLowItemId, {})
			});
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Rejected =
		Router.TryRoute(Host, Fixture.Coordinator, RejectedIntent);
	TestTrue(TEXT("Later duplicate evidence rejects the complete staged pulse"),
		Rejected.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::
				BatchRejected
		&& !Rejected.IsAccepted()
		&& Router.IsEmpty()
		&& Router.IsValid()
		&& Router.GetNextSampleSequence() == 0
		&& !Router.GetRunId().IsValid()
		&& Host.NumConsumedThreatPresenceIntents() == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 0
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 0);

	const Fdemo_mapShanmenControlledWeaponThreatSampleIntent CorrectedIntent =
		MakeSampleIntent(
			SampleIntent0,
			SampleRunId,
			0,
			Fixture.Coordinator,
			{
				MakeSampleRequest(SampleHighItemId, {}),
				MakeSampleRequest(SampleLowItemId, {})
			});
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Corrected =
		Router.TryRoute(Host, Fixture.Coordinator, CorrectedIntent);
	TestTrue(TEXT("Corrected retry reuses the unspent sequence and IntentId"),
		Corrected.IsAccepted()
		&& Corrected.Batch.Entries.Num() == 2
		&& Corrected.Batch.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& Corrected.Batch.Entries[1].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& Corrected.Batch.Entries[0].Finalization.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Corrected.Batch.Entries[1].Finalization.GetConsumption().GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& Host.NumConsumedThreatPresenceIntents() == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2
		&& Host.GetThreatPresenceAuthority().GetAuthorityRevision() == 0
		&& Router.GetNextSampleSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponThreatSampleSequenceFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatSampleRouter.SequenceRunAndResetFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponThreatSampleSequenceFenceTest::RunTest(
	const FString&)
{
	FThreatSampleRouterFixture Fixture(SampleRunId);
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	Fdemo_mapShanmenControlledWeaponThreatSampleRouter Router;
	if (!Fixture.bReady
		|| !AttachSampleWeapon(
			Fixture, Host, SampleLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.22 sequence fixture."));
		return false;
	}

	const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>
		EmptyRequest = { MakeSampleRequest(SampleLowItemId, {}) };
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Exhausted =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeSampleIntent(
				SampleIntent2,
				SampleRunId,
				MAX_int64,
				Fixture.Coordinator,
				EmptyRequest));
	TestTrue(TEXT("An unadvanceable sequence fails before Host mutation"),
		Exhausted.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::
				SequenceExhausted
		&& Router.IsEmpty()
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 0);
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Gap =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeSampleIntent(
				SampleIntent1,
				SampleRunId,
				1,
				Fixture.Coordinator,
				EmptyRequest));
	TestTrue(TEXT("A future first pulse cannot create a sequence gap"),
		Gap.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::SequenceGap
		&& Router.IsEmpty()
		&& Router.GetNextSampleSequence() == 0
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 0);

	const Fdemo_mapShanmenControlledWeaponThreatSampleResult First =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeSampleIntent(
				SampleIntent0,
				SampleRunId,
				0,
				Fixture.Coordinator,
				EmptyRequest));
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Second =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeSampleIntent(
				SampleIntent1,
				SampleRunId,
				1,
				Fixture.Coordinator,
				EmptyRequest));
	TestTrue(TEXT("Contiguous explicit pulses own deterministic sample ordinals"),
		First.IsAccepted()
		&& Second.IsAccepted()
		&& First.Batch.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 0
		&& Second.Batch.Entries[0].Finalization.GetPresence().GetPolicy()
			.GetEmission().GetContext().GetHitOrdinal() == 1
		&& Router.GetNextSampleSequence() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2);

	const Fdemo_mapShanmenControlledWeaponThreatSampleResult Stale =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeSampleIntent(
				SampleConflictIntent,
				SampleRunId,
				0,
				Fixture.Coordinator,
				EmptyRequest));
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult ReusedId =
		Router.TryRoute(
			Host,
			Fixture.Coordinator,
			MakeSampleIntent(
				SampleIntent1,
				SampleRunId,
				2,
				Fixture.Coordinator,
				EmptyRequest));
	TestTrue(TEXT("Stale history and latest IntentId reuse both fail closed"),
		Stale.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::SequenceStale
		&& ReusedId.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::
				IntentConflict
		&& Router.GetNextSampleSequence() == 2
		&& Host.GetThreatPresenceAuthority().GetSampleCheckpointRevision()
			== 2);

	FThreatSampleRouterFixture OtherFixture(SampleOtherRunId);
	Fdemo_mapShanmenControlledWeaponRunHost OtherHost;
	if (!OtherFixture.bReady
		|| !AttachSampleWeapon(
			OtherFixture, OtherHost, SampleLowItemId, 1, 0))
	{
		AddError(TEXT("Could not prepare P6.22 cross-Run fixture."));
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponThreatSampleResult CrossRun =
		Router.TryRoute(
			OtherHost,
			OtherFixture.Coordinator,
			MakeSampleIntent(
				SampleIntent2,
				SampleOtherRunId,
				2,
				OtherFixture.Coordinator,
				EmptyRequest));
	TestTrue(TEXT("One Router cannot cross an active Run boundary"),
		CrossRun.Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::
				RouterRunMismatch
		&& Router.GetRunId() == SampleRunId
		&& Router.GetNextSampleSequence() == 2
		&& OtherHost.GetThreatPresenceAuthority()
			.GetSampleCheckpointRevision() == 0);

	Fdemo_mapShanmenControlledWeaponThreatSampleIntent Invalid;
	Fdemo_mapShanmenControlledWeaponThreatSampleRequest Unresolved =
		MakeSampleRequest(SampleLowItemId, {});
	Fdemo_mapShanmenControlledWeaponOrbitThreatContact MissingIdentity;
	MissingIdentity.ContactLocation = FVector(1.0, 2.0, 3.0);
	MissingIdentity.ContactNormal = FVector::UpVector;
	Unresolved.Contacts.Add(MissingIdentity);
	TestFalse(TEXT("Capture rejects geometry without stable registry identity"),
		Fdemo_mapShanmenControlledWeaponThreatSampleIntent::TryCapture(
			SampleIntent2,
			SampleRunId,
			2,
			Fixture.Coordinator,
			{ Unresolved },
			Invalid));

	Router.Reset();
	TestTrue(TEXT("Run release reset returns the bounded owner to empty"),
		Router.IsEmpty()
		&& Router.IsValid()
		&& !Router.GetRunId().IsValid()
		&& Router.GetNextSampleSequence() == 0
		&& Router.NumAcceptedSamples() == 0);
	return true;
}

#endif
