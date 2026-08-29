#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponWorldAdapter.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
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
	const FGuid WorldRunId(0xD3630001, 0, 0, 1);
	const FGuid WorldOwnerId(0xD3630002, 0, 0, 1);
	const FGuid WorldItemId(0xD3630003, 0, 0, 1);

	const Fdemo_mapM01EnemyDefinition* FindMeleeDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeEncounterIdentity(
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

	struct FControlledWeaponWorldFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FControlledWeaponWorldFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			PlayerRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("P63PlayerRoot"))
				: nullptr;
			PlayerHealth = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("P63PlayerHealth"))
				: nullptr;
			if (Pawn && PlayerRoot)
			{
				Pawn->SetRootComponent(PlayerRoot);
			}

			const Fdemo_mapM01EnemyDefinition* Definition =
				FindMeleeDefinition();
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P63EnemyIdentity"))
				: nullptr;
			if (!Pawn || !PlayerRoot || !PlayerHealth
				|| !Definition || !Enemy || !EnemyIdentity)
			{
				return;
			}

			Enemy->AddInstanceComponent(EnemyIdentity);
			const Fdemo_mapEnemyEncounterIdentity EncounterIdentity =
				MakeEncounterIdentity(*Definition);
			bReady = EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					EncounterIdentity,
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					WorldRunId, Pawn, PlayerHealth, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		UPrimitiveComponent* GetEnemyRoot() const
		{
			return Enemy
				? Cast<UPrimitiveComponent>(Enemy->GetRootComponent())
				: nullptr;
		}
	};

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.3");
		Content.Digest = TEXT("P6.3.ControlledWeaponWorldDelivery.v1");
		return Content;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakePrepared(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& SourceEntityId)
	{
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId = FGuid(0xD3630010, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = Coordinator.GetRunId();
		Prepared.Evidence.OwnerId = WorldOwnerId;
		Prepared.Evidence.ItemInstanceId = WorldItemId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.3");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3630011, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 11;
		Prepared.Evidence.ItemRevision = 5;
		Prepared.Evidence.Content = MakeContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = Coordinator.GetRunId();
		ActionCapture.OwnerId = WorldOwnerId;
		ActionCapture.SourceEntityId = SourceEntityId;
		ActionCapture.SourceItemInstanceId = WorldItemId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeContent();
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
			TEXT("Detector.ControlledWeapon.P6.3.FlyingSword");
		DefinitionCapture.FormulaId =
			TEXT("Formula.ControlledWeapon.P6.3.ProductTest");
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

	bool StartDirectedSession(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& SourceEntityId,
		Fdemo_mapShanmenControlledWeaponSession& OutSession,
		FShanmenWorldHitContext& OutContext)
	{
		FShanmenActionTransitionReceipt Startup;
		FShanmenActionTransitionReceipt Active;
		FShanmenControlledWeaponCommandReceipt Launch;
		return Fdemo_mapShanmenControlledWeaponSession::TryStart(
				MakePrepared(Coordinator, SourceEntityId),
				OutSession,
				Startup,
				Active)
			&& OutSession.TryIssueControl(
				0,
				EShanmenControlledWeaponCommandKind::Launch,
				FVector::ForwardVector,
				Launch)
			&& OutSession.TryBeginContactWindow(OutContext);
	}

	FHitResult MakeSweepHit(const FControlledWeaponWorldFixture& Fixture)
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

	FOverlapResult MakeOverlap(const FControlledWeaponWorldFixture& Fixture)
	{
		FOverlapResult Overlap;
		Overlap.OverlapObjectHandle = FActorInstanceHandle(Fixture.Enemy);
		Overlap.Component = Fixture.GetEnemyRoot();
		Overlap.ItemIndex = 0;
		return Overlap;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponWorldThreatEvidenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery.ThreatEvidenceCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponWorldThreatEvidenceTest::RunTest(
	const FString&)
{
	FControlledWeaponWorldFixture Fixture;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot())
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.13 threat evidence fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult
		InvalidEmission = Fdemo_mapShanmenControlledWeaponWorldAdapter::
			CaptureOrbitThreatTargetEvidence(
				Fixture.Coordinator,
				FShanmenDetectorEmissionReceipt(),
				{ Fixture.Enemy });
	TestTrue(TEXT("Unfinished geometry cannot be treated as target evidence"),
		InvalidEmission.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			EmissionInvalid);

	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;
	FShanmenWorldHitContext Context;
	if (!Fdemo_mapShanmenControlledWeaponSession::TryStart(
			MakePrepared(
				Fixture.Coordinator,
				Fixture.Coordinator.GetPlayerEntityId()),
			Session,
			Startup,
			Active)
		|| !Session.TryBeginOrbitThreatWindow(Context))
	{
		AddError(TEXT("Could not start P6.13 Orbit threat session."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const Fdemo_mapShanmenControlledWeaponOrbitThreatResult Projected =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::
		ProjectOrbitThreatOverlap(
			Session,
			Fixture.Coordinator,
			Context,
			MakeOverlap(Fixture),
			FVector(75.0, 20.0, 30.0),
			FVector::BackwardVector);
	FShanmenDetectorEmissionReceipt Emission;
	if (!Projected.IsProjected()
		|| !Session.TryEndOrbitThreatWindow(Emission))
	{
		AddError(TEXT("Could not close P6.13 canonical geometry receipt."));
		return false;
	}

	AActor* Unregistered = NewObject<AActor>(GetTransientPackage());
	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult NullActor =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::
		CaptureOrbitThreatTargetEvidence(
			Fixture.Coordinator, Emission, { nullptr });
	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult
		UnregisteredActor = Fdemo_mapShanmenControlledWeaponWorldAdapter::
			CaptureOrbitThreatTargetEvidence(
				Fixture.Coordinator, Emission, { Unregistered });
	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult
		OutsideEmission = Fdemo_mapShanmenControlledWeaponWorldAdapter::
			CaptureOrbitThreatTargetEvidence(
				Fixture.Coordinator, Emission, { Fixture.Pawn });
	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Missing =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::
		CaptureOrbitThreatTargetEvidence(
			Fixture.Coordinator, Emission, {});
	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Duplicate =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::
		CaptureOrbitThreatTargetEvidence(
			Fixture.Coordinator,
			Emission,
			{ Fixture.Enemy, Fixture.Enemy });
	TestTrue(TEXT("Null target fails closed explicitly"),
		NullActor.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			TargetActorInvalid);
	TestTrue(TEXT("Unregistered target fails before tag inference"),
		UnregisteredActor.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			TargetNotRegistered);
	TestTrue(TEXT("Registered Actor outside the receipt cannot replace target"),
		OutsideEmission.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			TargetOutsideEmission);
	TestTrue(TEXT("One Actor is required for every canonical candidate"),
		Missing.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			MissingTarget);
	TestTrue(TEXT("Entity aliases cannot duplicate one evidence row"),
		Duplicate.Error
			== Edemo_mapShanmenControlledWeaponThreatEvidenceError::
			DuplicateTarget);

	const Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Captured =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::
		CaptureOrbitThreatTargetEvidence(
			Fixture.Coordinator, Emission, { Fixture.Enemy });
	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Canonical vitality host supplies Living target evidence"),
		Captured.IsCaptured()
		&& Captured.ExpectedTargetCount == 1
		&& Captured.TargetEvidence.Num() == 1
		&& Captured.TargetEvidence[0].GetTargetEntityId()
			== Projected.Candidate.TargetEntityId
		&& Captured.TargetEvidence[0].GetTargetTags().HasTagExact(
			FShanmenCombatNativeTags::TargetLiving()));
	TestTrue(TEXT("Evidence capture cannot mutate vitality or impact state"),
		FMath::IsNearlyEqual(Before.CurrentVitality, After.CurrentVitality)
		&& Session.GetExecution().NumAcceptedImpacts() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponWorldSweepOverlapTest,
	"Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery.SweepOverlapAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponWorldSweepOverlapTest::RunTest(const FString&)
{
	FControlledWeaponWorldFixture Fixture;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot())
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.3 world fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenWorldHitContext FirstContext;
	if (!StartDirectedSession(
			Fixture.Coordinator,
			Fixture.Coordinator.GetPlayerEntityId(),
			Session,
			FirstContext))
	{
		AddError(TEXT("Could not start P6.3 directed session."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const FHitResult Hit = MakeSweepHit(Fixture);
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult First =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveSweepContact(
			Session, Fixture.Coordinator, FirstContext, Hit);
	FShanmenTargetVitalitySnapshot AfterFirst;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterFirst));
	TestTrue(TEXT("Sweep resolves and reaches canonical vitality"),
		First.IsDelivered());
	TestTrue(TEXT("First contact is a new canonical commit"),
		First.Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::Committed);
	TestTrue(TEXT("Impact remains bound to the exact physical item"),
		First.Impact.GetRequest().Action.GetSourceItemInstanceId()
			== WorldItemId);
	TestTrue(TEXT("Frozen controlled formula resolves 0.9 damage"),
		FMath::IsNearlyEqual(First.Impact.GetResult().RawDamage, 0.9f));
	TestTrue(TEXT("Canonical receipt equals the observed vitality delta"),
		FMath::IsNearlyEqual(
			Before.CurrentVitality - AfterFirst.CurrentVitality,
			First.GetNewlyCommittedDamage(), 0.001f));
	TestEqual(TEXT("Successful delivery commits one session impact"),
		Session.GetExecution().NumAcceptedImpacts(), 1);

	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Duplicate =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveSweepContact(
			Session, Fixture.Coordinator, FirstContext, Hit);
	FShanmenTargetVitalitySnapshot AfterDuplicate;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(AfterDuplicate));
	TestTrue(TEXT("Duplicate callback is rejected before a second write"),
		Duplicate.Error
			== Edemo_mapShanmenControlledWeaponWorldDeliveryError::CandidateRejected
		&& FMath::IsNearlyEqual(
			AfterDuplicate.CurrentVitality, AfterFirst.CurrentVitality)
		&& Session.GetExecution().NumAcceptedImpacts() == 1);

	TestTrue(TEXT("First contact window closes"),
		Session.TryEndContactWindow());
	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("Second window owns the next stable ordinal"),
		Session.TryBeginContactWindow(SecondContext)
		&& SecondContext.GetHitOrdinal() == FirstContext.GetHitOrdinal() + 1);
	const FOverlapResult Overlap = MakeOverlap(Fixture);
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Stale =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveOverlapContact(
			Session,
			Fixture.Coordinator,
			FirstContext,
			Overlap,
			FVector(110.0, 20.0, 30.0),
			FVector::BackwardVector);
	TestTrue(TEXT("Stale context cannot consume the new window"),
		Stale.Error
			== Edemo_mapShanmenControlledWeaponWorldDeliveryError::CandidateRejected
		&& Session.GetExecution().NumAcceptedImpacts() == 1);

	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Second =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveOverlapContact(
			Session,
			Fixture.Coordinator,
			SecondContext,
			Overlap,
			FVector(110.0, 20.0, 30.0),
			FVector::BackwardVector);
	TestTrue(TEXT("Overlap uses the same atomic delivery path"),
		Second.IsDelivered()
		&& Second.Impact.GetRequest().Candidate.HitOrdinal == 1
		&& Second.Impact.GetResult().ImpactId
			!= First.Impact.GetResult().ImpactId
		&& Session.GetExecution().NumAcceptedImpacts() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponWorldDeliveryRollbackTest,
	"Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery.DeliveryFailureRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponWorldDeliveryRollbackTest::RunTest(
	const FString&)
{
	FControlledWeaponWorldFixture Fixture;
	if (!Fixture.bReady || !Fixture.GetEnemyRoot())
	{
		AddError(FString::Printf(
			TEXT("Could not prepare P6.3 rollback fixture: %s"),
			*Fixture.Diagnostic));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenWorldHitContext Context;
	const FGuid WrongPlayerEntityId(0xD36300FF, 0, 0, 1);
	if (!StartDirectedSession(
			Fixture.Coordinator,
			WrongPlayerEntityId,
			Session,
			Context))
	{
		AddError(TEXT("Could not start source-mismatch session."));
		return false;
	}

	FShanmenTargetVitalitySnapshot Before;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(Before));
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveSweepContact(
			Session,
			Fixture.Coordinator,
			Context,
			MakeSweepHit(Fixture));
	FShanmenTargetVitalitySnapshot After;
	check(Fixture.Enemy->TryCaptureCombatVitalitySnapshot(After));
	TestTrue(TEXT("Canonical source rejection is surfaced"),
		Result.Error
			== Edemo_mapShanmenControlledWeaponWorldDeliveryError::DeliveryRejected
		&& Result.Impact.IsValid()
		&& Result.Delivery.Error
			== Edemo_mapCombatImpactDeliveryError::SourceMismatch);
	TestTrue(TEXT("Failed delivery rolls back session and vitality together"),
		Session.GetExecution().NumAcceptedImpacts() == 0
		&& FMath::IsNearlyEqual(
			Before.CurrentVitality, After.CurrentVitality)
		&& Fixture.Enemy->NumCommittedCombatImpacts() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponWorldUnregisteredContactTest,
	"Shanmen.0_0_10.Product.ControlledWeaponWorldDelivery.UnregisteredContact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponWorldUnregisteredContactTest::RunTest(
	const FString&)
{
	FControlledWeaponWorldFixture Fixture;
	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenWorldHitContext Context;
	if (!Fixture.bReady
		|| !StartDirectedSession(
			Fixture.Coordinator,
			Fixture.Coordinator.GetPlayerEntityId(),
			Session,
			Context))
	{
		AddError(TEXT("Could not prepare P6.3 contact rejection fixture."));
		return false;
	}

	AActor* Unregistered = NewObject<AActor>(GetTransientPackage());
	UBoxComponent* Root = Unregistered
		? NewObject<UBoxComponent>(Unregistered, TEXT("UnregisteredRoot"))
		: nullptr;
	if (Unregistered && Root)
	{
		Unregistered->SetRootComponent(Root);
	}
	FHitResult Hit(
		Unregistered,
		Root,
		FVector::ZeroVector,
		FVector::UpVector);
	Hit.ImpactPoint = FVector::ZeroVector;
	Hit.ImpactNormal = FVector::UpVector;
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Result =
		Fdemo_mapShanmenControlledWeaponWorldAdapter::ResolveSweepContact(
			Session, Fixture.Coordinator, Context, Hit);
	TestTrue(TEXT("Unregistered transient actors never become candidates"),
		Result.Error
			== Edemo_mapShanmenControlledWeaponWorldDeliveryError::ContactNotResolved
		&& Session.GetExecution().NumAcceptedImpacts() == 0);
	return true;
}

#endif
