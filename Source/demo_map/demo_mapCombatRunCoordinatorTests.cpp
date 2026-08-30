#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapM01BossCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "demo_mapShanmenFormationInfluenceConsumerWorldResolution.h"

#include <limits>

namespace
{
	const FGuid CoordinatorRunA(0x54300001, 0, 0, 1);
	const FGuid CoordinatorRunB(0x54300002, 0, 0, 1);
	const FGuid CoordinatorOwnerId(0x54300003, 0, 0, 1);

	struct FCombatRunCoordinatorFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FCombatRunCoordinatorFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("PlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("PlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					CoordinatorRunA,
					Pawn,
					Health,
					Diagnostic);
		}
	};

	const Fdemo_mapM01EnemyDefinition* FindM01MeleeDefinition(
		bool bEnhanced = false)
	{
		const Edemo_mapM01EnemyArchetype DesiredArchetype = bEnhanced
			? Edemo_mapM01EnemyArchetype::EliteStalker
			: Edemo_mapM01EnemyArchetype::StandardSkirmisher;
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype == DesiredArchetype)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	const Fdemo_mapM01EnemyDefinition* FindM01RangedDefinition()
	{
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype
				== Edemo_mapM01EnemyArchetype::StandardRanged)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	const Fdemo_mapM01EnemyDefinition* FindM01HeavyDefinition(
		bool bElite = false)
	{
		const Edemo_mapM01EnemyArchetype DesiredArchetype = bElite
			? Edemo_mapM01EnemyArchetype::EliteBulwark
			: Edemo_mapM01EnemyArchetype::StandardBruiser;
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype == DesiredArchetype)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	const Fdemo_mapM01EnemyDefinition* FindM01BossDefinition()
	{
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype == Edemo_mapM01EnemyArchetype::BossMain)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	AActor* NewM01ProductActor(
		const Fdemo_mapM01EnemyDefinition& Definition)
	{
		switch (Definition.Archetype)
		{
		case Edemo_mapM01EnemyArchetype::StandardSkirmisher:
		case Edemo_mapM01EnemyArchetype::EliteStalker:
			return NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
		case Edemo_mapM01EnemyArchetype::StandardRanged:
			return NewObject<Ademo_mapRangedEnemyCharacter>(
				GetTransientPackage());
		case Edemo_mapM01EnemyArchetype::StandardBruiser:
		case Edemo_mapM01EnemyArchetype::EliteBulwark:
			return NewObject<Ademo_mapHeavyEnemyCharacter>(
				GetTransientPackage());
		case Edemo_mapM01EnemyArchetype::BossMain:
			return NewObject<Ademo_mapM01BossCharacter>(GetTransientPackage());
		default:
			return nullptr;
		}
	}

	Fdemo_mapEnemyEncounterIdentity MakeLegacyEncounterIdentity(
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

	bool ConfigureM01ProductActor(
		const Fdemo_mapM01EnemyDefinition& Definition,
		AActor* EnemyActor)
	{
		const Fdemo_mapEnemyEncounterIdentity LegacyIdentity =
			MakeLegacyEncounterIdentity(Definition);
		switch (Definition.Archetype)
		{
		case Edemo_mapM01EnemyArchetype::StandardSkirmisher:
		case Edemo_mapM01EnemyArchetype::EliteStalker:
			return CastChecked<Ademo_mapEnemyCharacter>(EnemyActor)
				->ConfigureEncounter(
					LegacyIdentity,
					Definition.Tuning,
					Definition.IsElite());
		case Edemo_mapM01EnemyArchetype::StandardRanged:
			return CastChecked<Ademo_mapRangedEnemyCharacter>(EnemyActor)
				->ConfigureEncounter(
					LegacyIdentity,
					Definition.Tuning,
					Definition.IsElite());
		case Edemo_mapM01EnemyArchetype::StandardBruiser:
		case Edemo_mapM01EnemyArchetype::EliteBulwark:
			return CastChecked<Ademo_mapHeavyEnemyCharacter>(EnemyActor)
				->ConfigureEncounter(LegacyIdentity, Definition.Tuning);
		case Edemo_mapM01EnemyArchetype::BossMain:
			return CastChecked<Ademo_mapM01BossCharacter>(EnemyActor)
				->ConfigureBoss(Definition);
		default:
			return false;
		}
	}

	struct FM01MeleeEnemyFixture
	{
		const Fdemo_mapM01EnemyDefinition* Definition = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* Identity = nullptr;
		bool bReady = false;

		explicit FM01MeleeEnemyFixture(bool bEnhanced = false)
		{
			Definition = FindM01MeleeDefinition(bEnhanced);
			Enemy = NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
			Identity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy,
					TEXT("M01AuthoredIdentity"))
				: nullptr;
			if (!Definition || !Enemy || !Identity)
			{
				return;
			}
			Enemy->AddInstanceComponent(Identity);
			const Fdemo_mapEnemyEncounterIdentity LegacyIdentity =
				MakeLegacyEncounterIdentity(*Definition);
			bReady = Identity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					LegacyIdentity,
					Definition->Tuning,
					Definition->IsElite());
		}
	};

	struct FM01RangedEnemyFixture
	{
		const Fdemo_mapM01EnemyDefinition* Definition = nullptr;
		Ademo_mapRangedEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* Identity = nullptr;
		bool bReady = false;

		FM01RangedEnemyFixture()
		{
			Definition = FindM01RangedDefinition();
			Enemy = NewObject<Ademo_mapRangedEnemyCharacter>(
				GetTransientPackage());
			Identity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy,
					TEXT("M01AuthoredRangedIdentity"))
				: nullptr;
			if (!Definition || !Enemy || !Identity)
			{
				return;
			}
			Enemy->AddInstanceComponent(Identity);
			const Fdemo_mapEnemyEncounterIdentity LegacyIdentity =
				MakeLegacyEncounterIdentity(*Definition);
			bReady = Identity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					LegacyIdentity,
					Definition->Tuning,
					Definition->IsElite());
		}
	};

	struct FM01HeavyEnemyFixture
	{
		const Fdemo_mapM01EnemyDefinition* Definition = nullptr;
		Ademo_mapHeavyEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* Identity = nullptr;
		bool bReady = false;

		explicit FM01HeavyEnemyFixture(bool bElite = false)
		{
			Definition = FindM01HeavyDefinition(bElite);
			Enemy = NewObject<Ademo_mapHeavyEnemyCharacter>(
				GetTransientPackage());
			Identity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy,
					bElite
						? TEXT("M01AuthoredEliteHeavyIdentity")
						: TEXT("M01AuthoredHeavyIdentity"))
				: nullptr;
			if (!Definition || !Enemy || !Identity)
			{
				return;
			}
			Enemy->AddInstanceComponent(Identity);
			const Fdemo_mapEnemyEncounterIdentity LegacyIdentity =
				MakeLegacyEncounterIdentity(*Definition);
			bReady = Identity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					LegacyIdentity,
					Definition->Tuning);
		}
	};

	struct FM01BossEnemyFixture
	{
		const Fdemo_mapM01EnemyDefinition* Definition = nullptr;
		Ademo_mapM01BossCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* Identity = nullptr;
		bool bReady = false;

		FM01BossEnemyFixture()
		{
			Definition = FindM01BossDefinition();
			Enemy = NewObject<Ademo_mapM01BossCharacter>(
				GetTransientPackage());
			Identity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy,
					TEXT("M01AuthoredBossIdentity"))
				: nullptr;
			if (!Definition || !Enemy || !Identity)
			{
				return;
			}
			Enemy->AddInstanceComponent(Identity);
			bReady = Identity->Configure(*Definition)
				&& Enemy->ConfigureBoss(*Definition);
		}
	};

	FShanmenCombatActionSnapshot MakeCoordinatorAction(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		uint64 ActivationSequence = 1)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = CoordinatorOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.SourceItemInstanceId = FGuid(0x54300004, 0, 0, 1);
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P4.4");
		Capture.Content.Digest =
			TEXT("TEST-DIGEST-P4.4-COMBAT-RUN-COORDINATOR");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenBasicSwordDefinition MakeCoordinatorSwordDefinition(
		float BaseDamage = 2.0f)
	{
		FShanmenBasicSwordDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Weapon.Main");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Basic01.r1");
		Capture.BaseDamage = BaseDamage;
		Capture.AttackPowerCoefficient = 0.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		FShanmenBasicSwordDefinition Definition;
		check(FShanmenBasicSwordDefinition::TryCapture(Capture, Definition));
		return Definition;
	}

	bool TryResolveSwordImpact(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenCombatActionSnapshot& Action,
		AActor* TargetActor,
		UPrimitiveComponent* TargetRoot,
		const FShanmenTargetVitalitySnapshot& Vitality,
		FShanmenBasicSwordImpactReceipt& OutImpact,
		float BaseDamage = 2.0f)
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Phase;
		if (!FShanmenActionOrchestrator::TryStart(Action, Runtime, Phase)
			|| !Runtime.TryAdvance(EShanmenCombatActionPhase::Startup, Phase))
		{
			return false;
		}

		FShanmenBasicSwordOffenseSnapshot Offense;
		if (!FShanmenBasicSwordOffenseSnapshot::TryCapture(1.0f, Offense))
		{
			return false;
		}
		FShanmenBasicSwordExecution Sword;
		if (!FShanmenBasicSwordExecution::TryCreate(
			Action,
			MakeCoordinatorSwordDefinition(BaseDamage),
			Offense,
			Sword))
		{
			return false;
		}
		FShanmenWorldHitContext Context;
		if (!Sword.TryBeginEmission(Runtime, Context))
		{
			return false;
		}

		FHitResult WorldHit(
			TargetActor,
			TargetRoot,
			FVector(20.0, 0.0, 50.0),
			FVector::BackwardVector);
		WorldHit.ImpactPoint = FVector(20.0, 0.0, 50.0);
		WorldHit.ImpactNormal = FVector::BackwardVector;
		WorldHit.Item = 0;
		FShanmenHitCandidate Candidate;
		if (!FShanmenWorldHitAdapter::TryFromSweep(
			Context,
			WorldHit,
			Coordinator.GetEntityRegistry(),
			Candidate))
		{
			return false;
		}

		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Sword.TryResolveCandidate(
			Runtime,
			Candidate,
			Vitality,
			Defense,
			OutImpact);
	}

	FHitResult MakeProductSwordHit(AActor* TargetActor)
	{
		UPrimitiveComponent* TargetRoot = TargetActor
			? Cast<UPrimitiveComponent>(TargetActor->GetRootComponent())
			: nullptr;
		FHitResult Hit(
			TargetActor,
			TargetRoot,
			FVector(40.0, 0.0, 50.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(40.0, 0.0, 50.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}

	FOverlapResult MakeProductShapeOverlap(AActor* TargetActor)
	{
		FOverlapResult Overlap;
		Overlap.OverlapObjectHandle = FActorInstanceHandle(TargetActor);
		Overlap.Component = TargetActor
			? Cast<UPrimitiveComponent>(TargetActor->GetRootComponent())
			: nullptr;
		Overlap.ItemIndex = 0;
		return Overlap;
	}

	FHitResult MakeProductProjectileHit(AActor* TargetActor)
	{
		UPrimitiveComponent* TargetRoot = TargetActor
			? Cast<UPrimitiveComponent>(TargetActor->GetRootComponent())
			: nullptr;
		FHitResult Hit(
			TargetActor,
			TargetRoot,
			FVector(600.0, 0.0, 55.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(600.0, 0.0, 55.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorLifecycleTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.RunLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorLifecycleTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("Product combat Run fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid ExpectedFirstId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunA,
			Fdemo_mapCombatRunCoordinator::PlayerSpawnSourceId(),
			0);
	TestTrue(TEXT("Player identity uses only Run and fixed spawn tuple"),
		Fixture.Coordinator.GetPlayerEntityId() == ExpectedFirstId);
	TestTrue(TEXT("Coordinator and vitality share one player identity"),
		Fixture.Coordinator.IsReady()
			&& Fixture.Health->GetCombatEntityId() == ExpectedFirstId);

	FGuid Resolved;
	TestTrue(TEXT("Registry resolves the product Pawn alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			Fixture.Pawn,
			INDEX_NONE,
			Resolved)
			&& Resolved == ExpectedFirstId);
	TestTrue(TEXT("Registry resolves the collision-root alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			Fixture.CollisionRoot,
			INDEX_NONE,
			Resolved)
			&& Resolved == ExpectedFirstId);
	TestTrue(TEXT("Same product Run bind is idempotent"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunA,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	TestFalse(TEXT("Live coordinator rejects silent cross-Run switch"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	TestFalse(TEXT("Foreign Run cannot release the live identity"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunB,
			Fixture.Diagnostic));
	TestTrue(TEXT("Exact Run release clears registry and vitality binding"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic)
			&& !Fixture.Coordinator.IsActive()
			&& !Fixture.Health->IsCombatEntityBound());

	TestTrue(TEXT("Persistent Pawn may enter a later authority Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic));
	const FGuid ExpectedSecondId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunB,
			Fdemo_mapCombatRunCoordinator::PlayerSpawnSourceId(),
			0);
	TestTrue(TEXT("New Run deterministically produces a distinct player identity"),
		ExpectedSecondId.IsValid()
			&& ExpectedSecondId != ExpectedFirstId
			&& Fixture.Coordinator.GetPlayerEntityId() == ExpectedSecondId
			&& Fixture.Health->GetCombatEntityId() == ExpectedSecondId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorEntityAliasBindingTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.EntityAliasBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorEntityAliasBindingTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	Fdemo_mapCombatRunCoordinator InactiveCoordinator;
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* BodyAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* UnboundAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	TestTrue(TEXT("Entity alias fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady && Attributes
			&& BodyAttributes && UnboundAttributes);
	if (!Fixture.bReady || !EnemyFixture.bReady || !Attributes
		|| !BodyAttributes || !UnboundAttributes)
	{
		return false;
	}

	const auto NotReady = InactiveCoordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, Attributes);
	const auto MissingSource = Fixture.Coordinator.TryBindEntityAlias(
		nullptr, INDEX_NONE, Attributes);
	const auto MissingAlias = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, nullptr);
	const auto InvalidBody = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE - 1, Attributes);
	const auto SourceNotFound = Fixture.Coordinator.TryBindEntityAlias(
		UnboundAttributes, INDEX_NONE, Attributes);
	TestTrue(TEXT("Invalid alias requests fail before registry mutation"),
		NotReady.Status
			== Edemo_mapCombatRunEntityAliasStatus::CoordinatorNotReady
			&& MissingSource.Status
				== Edemo_mapCombatRunEntityAliasStatus::
					RegisteredObjectUnavailable
			&& MissingAlias.Status
				== Edemo_mapCombatRunEntityAliasStatus::AliasObjectUnavailable
			&& InvalidBody.Status
				== Edemo_mapCombatRunEntityAliasStatus::BodyIndexInvalid
			&& SourceNotFound.Status
				== Edemo_mapCombatRunEntityAliasStatus::
					RegisteredObjectNotFound
			&& Fixture.Coordinator.GetEntityRegistry().NumObjectBindings() == 3);

	const auto Bound = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, Attributes);
	const auto Replay = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, Attributes);
	const auto ExactBody = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, BodyAttributes, 7);
	const auto ExactBodyReplay = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, BodyAttributes, 7);
	const auto WorldResolution =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Fixture.Coordinator.GetEntityRegistry(),
			CoordinatorRunA,
			Attributes);
	const auto WrongBodyResolution =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Fixture.Coordinator.GetEntityRegistry(),
			CoordinatorRunA,
			BodyAttributes,
			8);
	const auto ExactBodyResolution =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Fixture.Coordinator.GetEntityRegistry(),
			CoordinatorRunA,
			BodyAttributes,
			7);

	TestTrue(TEXT("Registered source derives one pointer-free alias receipt"),
		Bound.IsSuccess()
			&& Bound.Status == Edemo_mapCombatRunEntityAliasStatus::Bound
			&& Bound.EntityId == Fixture.Coordinator.GetPlayerEntityId()
			&& Bound.RegisteredObjectUniqueId == Fixture.Pawn->GetUniqueID()
			&& Bound.AliasObjectUniqueId == Attributes->GetUniqueID()
			&& Bound.BindingCountAfter == Bound.BindingCountBefore + 1);
	TestTrue(TEXT("Exact alias replay is read-only"),
		Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapCombatRunEntityAliasStatus::AlreadyBound
			&& !Replay.bRegistryUpdated
			&& Replay.BindingCountAfter == Replay.BindingCountBefore);
	TestTrue(TEXT("Exact body alias preserves body-specific lookup"),
		ExactBody.IsSuccess() && ExactBody.AliasBodyIndex == 7
			&& ExactBodyReplay.IsSuccess()
			&& ExactBodyReplay.Status
				== Edemo_mapCombatRunEntityAliasStatus::AlreadyBound
			&& WrongBodyResolution.Status
				== Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
					EntityNotFound
			&& ExactBodyResolution.IsSuccess());
	TestTrue(TEXT("Combat Run alias feeds formation consumer resolution"),
		WorldResolution.IsSuccess()
			&& WorldResolution.ResolvedEntityId
				== Fixture.Coordinator.GetPlayerEntityId()
			&& WorldResolution.Resolution.AttributeComponentUniqueId
				== Attributes->GetUniqueID());

	TestTrue(TEXT("M01 fixture enters the same Run registry"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy, Fixture.Diagnostic));
	const int32 CountBeforeConflict =
		Fixture.Coordinator.GetEntityRegistry().NumObjectBindings();
	const auto Conflict = Fixture.Coordinator.TryBindEntityAlias(
		Fixture.Pawn, INDEX_NONE, EnemyFixture.Enemy);
	TestTrue(TEXT("Existing foreign entity alias fails without mutation"),
		Conflict.Status == Edemo_mapCombatRunEntityAliasStatus::AliasConflict
			&& Conflict.bRegisteredObjectResolved
			&& !Conflict.bAliasVerified
			&& !Conflict.bRegistryUpdated
			&& Fixture.Coordinator.GetEntityRegistry().NumObjectBindings()
				== CountBeforeConflict);

	TestTrue(TEXT("Run end clears derived aliases with the registry"),
		Fixture.Coordinator.TryEndRun(CoordinatorRunA, Fixture.Diagnostic));
	FGuid ResolvedAfterEnd;
	TestFalse(TEXT("Ended Run cannot resolve the attribute alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			Attributes,
			INDEX_NONE,
			ResolvedAfterEnd));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorPlayerDeliveryTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.PlayerDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorPlayerDeliveryTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("Product combat Run fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid SourceEntityId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			CoordinatorRunA,
			TEXT("Spawn.Combat.TestSource"),
			0);
	const FShanmenCombatActionSnapshot Action =
		MakeCoordinatorAction(CoordinatorRunA, SourceEntityId);
	FShanmenTargetVitalitySnapshot Vitality;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Vitality));
	FShanmenBasicSwordImpactReceipt Impact;
	TestTrue(TEXT("Registered player world hit resolves one Impact"),
		TryResolveSwordImpact(
			Fixture.Coordinator,
			Action,
			Fixture.Pawn,
			Fixture.CollisionRoot,
			Vitality,
			Impact));

	const Fdemo_mapCombatImpactDeliveryResult First =
		Fixture.Coordinator.DeliverBasicSwordImpactToPlayer(Impact);
	TestTrue(TEXT("Coordinator commits resolved Impact to the player host"),
		First.IsSuccess()
			&& First.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation()
				== 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverBasicSwordImpactToPlayer(Impact);
	TestTrue(TEXT("Player receipt replay cannot double-commit"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation()
				== 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01IdentityTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01AuthoredIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01IdentityTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Player and M01 melee fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady);
	if (!Fixture.bReady || !EnemyFixture.bReady)
	{
		return false;
	}

	TestTrue(TEXT("Authored M01 enemy enters the shared Run registry"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic));
	const FGuid ExpectedEnemyId =
		Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
			CoordinatorRunA,
			*EnemyFixture.Definition);
	TestTrue(TEXT("Enemy identity uses Run plus authored SpawnMarkerId only"),
		ExpectedEnemyId.IsValid()
			&& ExpectedEnemyId
				== FShanmenWorldEntityIdFactory::MakeEntityId(
					CoordinatorRunA,
					EnemyFixture.Definition->SpawnMarkerId,
					0)
			&& EnemyFixture.Enemy->GetCombatEntityId() == ExpectedEnemyId);

	FGuid Resolved;
	UPrimitiveComponent* EnemyRoot = Cast<UPrimitiveComponent>(
		EnemyFixture.Enemy->GetRootComponent());
	TestTrue(TEXT("Registry resolves the M01 actor alias"),
		Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
			CoordinatorRunA,
			EnemyFixture.Enemy,
			INDEX_NONE,
			Resolved)
			&& Resolved == ExpectedEnemyId);
	TestTrue(TEXT("Registry resolves the M01 collision-root alias"),
		EnemyRoot
			&& Fixture.Coordinator.GetEntityRegistry().TryResolveObject(
				CoordinatorRunA,
				EnemyRoot,
				INDEX_NONE,
				Resolved)
			&& Resolved == ExpectedEnemyId);
	TestTrue(TEXT("Same authored actor registration is idempotent"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic));
	FM01MeleeEnemyFixture ConflictingFixture;
	TestTrue(TEXT("Second fixture with same authored identity initializes"),
		ConflictingFixture.bReady);
	TestFalse(TEXT("Duplicate authored spawn cannot claim another actor"),
		Fixture.Coordinator.TryRegisterM01Enemy(
			ConflictingFixture.Enemy,
			Fixture.Diagnostic));

	TSet<FGuid> AuthoredEntityIds;
	AuthoredEntityIds.Add(ExpectedEnemyId);
	TArray<AActor*> OtherAuthoredActors;
	for (const Fdemo_mapM01EnemyDefinition& Definition :
		Fdemo_mapM01EnemyConfig::GetDefinitions())
	{
		if (Definition.SpawnMarkerId
			== EnemyFixture.Definition->SpawnMarkerId)
		{
			continue;
		}
		AActor* AuthoredActor = NewM01ProductActor(Definition);
		Udemo_mapM01EnemyIdentityComponent* AuthoredIdentity = AuthoredActor
			? NewObject<Udemo_mapM01EnemyIdentityComponent>(AuthoredActor)
			: nullptr;
		if (AuthoredActor && AuthoredIdentity)
		{
			AuthoredActor->AddInstanceComponent(AuthoredIdentity);
		}
		const FGuid AuthoredEntityId =
			Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
				CoordinatorRunA,
				Definition);
		const bool bRegistered = AuthoredActor
			&& AuthoredIdentity
			&& AuthoredIdentity->Configure(Definition)
			&& ConfigureM01ProductActor(Definition, AuthoredActor)
			&& !AuthoredEntityIds.Contains(AuthoredEntityId)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				AuthoredActor,
				Fixture.Diagnostic);
		TestTrue(
			FString::Printf(
				TEXT("Authored M01 spawn registers uniquely: %s"),
				*Definition.SpawnMarkerId.ToString()),
			bRegistered);
		if (bRegistered)
		{
			AuthoredEntityIds.Add(AuthoredEntityId);
			OtherAuthoredActors.Add(AuthoredActor);
		}
	}
	TestTrue(TEXT("All 14 authored enemies share one Run registry"),
		Fixture.Coordinator.NumRegisteredM01Enemies() == 14
			&& AuthoredEntityIds.Num() == 14);
	TestTrue(TEXT("Every authored M01 actor owns one vitality ledger"),
		Fixture.Coordinator.NumVitalityBoundM01Enemies() == 14);
	bool bAllEnemyBindingsReleased = Fixture.Coordinator.TryEndRun(
		CoordinatorRunA,
		Fixture.Diagnostic);
	for (AActor* AuthoredActor : OtherAuthoredActors)
	{
		const Idemo_mapCombatVitalityHost* VitalityHost =
			Cast<Idemo_mapCombatVitalityHost>(AuthoredActor);
		bAllEnemyBindingsReleased = bAllEnemyBindingsReleased
			&& VitalityHost
			&& !VitalityHost->IsCombatEntityBound();
	}
	TestTrue(TEXT("Exact Run release clears player and all enemy ledgers"),
		bAllEnemyBindingsReleased
			&& !Fixture.Health->IsCombatEntityBound()
			&& !EnemyFixture.Enemy->IsCombatEntityBound());

	TestTrue(TEXT("Same product actors enter a later Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	const FGuid SecondEnemyId =
		Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
			CoordinatorRunB,
			*EnemyFixture.Definition);
	TestTrue(TEXT("New Run changes enemy identity without random inputs"),
		SecondEnemyId.IsValid()
			&& SecondEnemyId != ExpectedEnemyId
			&& EnemyFixture.Enemy->GetCombatEntityId() == SecondEnemyId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01DeliveryTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01BasicSwordDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01DeliveryTest::RunTest(const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Player and M01 melee fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady);
	if (!Fixture.bReady || !EnemyFixture.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FShanmenCombatActionSnapshot Action = MakeCoordinatorAction(
		CoordinatorRunA,
		Fixture.Coordinator.GetPlayerEntityId());
	FShanmenTargetVitalitySnapshot Vitality;
	check(EnemyFixture.Enemy->TryCaptureCombatVitalitySnapshot(Vitality));
	FShanmenBasicSwordImpactReceipt Impact;
	TestTrue(TEXT("M01 collision root resolves the authored target Impact"),
		TryResolveSwordImpact(
			Fixture.Coordinator,
			Action,
			EnemyFixture.Enemy,
			Cast<UPrimitiveComponent>(EnemyFixture.Enemy->GetRootComponent()),
			Vitality,
			Impact));

	const float VitalityBefore = EnemyFixture.Enemy->GetCurrentVitality();
	const Fdemo_mapCombatImpactDeliveryResult First =
		Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
			Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("First resolved sword receipt commits once to M01 vitality"),
		First.IsSuccess()
			&& First.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 2.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 1
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
			Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("M01 receipt replay is visible but cannot double-apply"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 2.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 1
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);

	TestTrue(TEXT("Exact first Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Persistent hosts bind the next Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
			Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("Delayed old-Run receipt fails before enemy mutation"),
		DelayedOldRun.Error
				== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldRun.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 2.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 0
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 0
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01EnemyBasicMeleeProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyBasicMeleeProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01EnemyBasicMeleeProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Enemy-to-player product fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady);
	if (!Fixture.bReady || !EnemyFixture.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			EnemyFixture.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(
			Fixture.Pawn,
			TEXT("P47PlayerAttributes"));
	Fdemo_mapModifierSpec FlatReduction;
	FlatReduction.SourceId = TEXT("P4.7.Test.FlatReduction");
	FlatReduction.AttributeId =
		Fdemo_mapAttributeIds::FlatDamageReduction;
	FlatReduction.Operation = Edemo_mapModifierOperation::Add;
	FlatReduction.Value = 1.0f;
	Fdemo_mapModifierHandle FlatReductionHandle;
	TestTrue(TEXT("Player canonical defense fixture binds"),
		Attributes
			&& Fixture.Health->BindAttributeComponent(Attributes, false)
			&& Attributes->AddModifier(
				FlatReduction,
				FlatReductionHandle));

	AActor* UnregisteredSource =
		NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidSource =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			UnregisteredSource,
			Fixture.Pawn,
			3.0f);
	TestTrue(TEXT("Unregistered enemy fails before action identity is consumed"),
		InvalidSource.Error
			== Edemo_mapM01EnemyAttackExecutionError::SourceNotRegistered
			&& !InvalidSource.ActivationId.IsValid()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				5.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 0);

	const FGuid SourceEntityId = EnemyFixture.Enemy->GetCombatEntityId();
	const Fdemo_mapM01EnemyAttackExecutionResult First =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			EnemyFixture.Enemy,
			Fixture.Pawn,
			3.0f);
	const FShanmenImpactResult& FirstResolution =
		First.Impact.GetResult();
	const FGuid ExpectedFirstActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			SourceEntityId,
			First.Impact.GetRequest().Action.GetActionDefinitionId(),
			1);
	TestTrue(TEXT("First real enemy melee strike resolves player armor and commits"),
		First.IsExecuted()
			&& First.ActivationId == ExpectedFirstActivation
			&& First.Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FirstResolution.Outcome
				== EShanmenDefenseOutcome::Mitigated
			&& FMath::IsNearlyEqual(FirstResolution.RawDamage, 3.0f)
			&& FMath::IsNearlyEqual(
				FirstResolution.PreventedDamage,
				1.0f)
			&& FMath::IsNearlyEqual(FirstResolution.FinalDamage, 2.0f)
			&& FirstResolution.TriggeredLayers.Num() == 1
			&& FirstResolution.TriggeredLayers[0].Operation
				== EShanmenDefenseOperation::AbsorbPoints
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapCombatImpactDeliveryResult FirstReplay =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			First.Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("Enemy melee receipt replay cannot double-apply"),
		FirstReplay.IsSuccess()
			&& FirstReplay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	Fdemo_mapModifierSpec HalfDodge;
	HalfDodge.SourceId = TEXT("P4.7.Test.HalfDodge");
	HalfDodge.AttributeId = Fdemo_mapAttributeIds::DodgeChance;
	HalfDodge.Operation = Edemo_mapModifierOperation::Add;
	HalfDodge.Value = 0.5f;
	Fdemo_mapModifierHandle HalfDodgeHandle;
	TestTrue(TEXT("Half-dodge modifier prepares deterministic sampling"),
		Attributes->AddModifier(HalfDodge, HalfDodgeHandle));
	const FGuid ExpectedSecondActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			SourceEntityId,
			First.Impact.GetRequest().Action.GetActionDefinitionId(),
			2);
	const FGuid ExpectedSecondImpact =
		FShanmenCombatIdFactory::MakeImpactId(
			CoordinatorRunA,
			ExpectedSecondActivation,
			First.Impact.GetRequest().Candidate.DetectorId,
			Fixture.Coordinator.GetPlayerEntityId(),
			0);
	FShanmenDefenseSnapshot DefenseA;
	FShanmenDefenseSnapshot DefenseB;
	bool bDefenseReplayStable =
		Fixture.Health->TryCaptureCombatDefenseSnapshot(
			ExpectedSecondImpact,
			DefenseA)
		&& Fixture.Health->TryCaptureCombatDefenseSnapshot(
			ExpectedSecondImpact,
			DefenseB)
		&& DefenseA.Layers.Num() == DefenseB.Layers.Num();
	for (int32 Index = 0;
		bDefenseReplayStable && Index < DefenseA.Layers.Num();
		++Index)
	{
		bDefenseReplayStable =
			DefenseA.Layers[Index].LayerId
				== DefenseB.Layers[Index].LayerId
			&& DefenseA.Layers[Index].RuleId
				== DefenseB.Layers[Index].RuleId
			&& DefenseA.Layers[Index].Operation
				== DefenseB.Layers[Index].Operation
			&& DefenseA.Layers[Index].Order
				== DefenseB.Layers[Index].Order
			&& FMath::IsNearlyEqual(
				DefenseA.Layers[Index].Magnitude,
				DefenseB.Layers[Index].Magnitude);
	}
	TestTrue(TEXT("Same ImpactId captures the same ordered player defense"),
		bDefenseReplayStable);

	Fdemo_mapModifierSpec RemainingDodge = HalfDodge;
	RemainingDodge.SourceId = TEXT("P4.7.Test.RemainingDodge");
	Fdemo_mapModifierHandle RemainingDodgeHandle;
	TestTrue(TEXT("Full dodge prepares a guaranteed canonical avoidance layer"),
		Attributes->AddModifier(RemainingDodge, RemainingDodgeHandle));
	const Fdemo_mapM01EnemyAttackExecutionResult Evaded =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			EnemyFixture.Enemy,
			Fixture.Pawn,
			3.0f);
	TestTrue(TEXT("Second strike records full avoidance without a damage broadcast"),
		Evaded.IsExecuted()
			&& Evaded.ActivationId == ExpectedSecondActivation
			&& Evaded.Impact.GetResult().Outcome
				== EShanmenDefenseOutcome::Evaded
			&& FMath::IsNearlyEqual(
				Evaded.Impact.GetResult().PreventedDamage,
				3.0f)
			&& FMath::IsNearlyEqual(
				Evaded.Impact.GetResult().FinalDamage,
				0.0f)
			&& Evaded.Impact.GetResult().TriggeredLayers.Num() == 1
			&& Evaded.Impact.GetResult().TriggeredLayers[0].Operation
				== EShanmenDefenseOperation::PreventAll
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 2
			&& Fixture.Health->NumCommittedCombatImpacts() == 2
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	TestTrue(TEXT("Exact first Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Persistent combatants bind the next Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			First.Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("Old-Run enemy receipt is rejected before player mutation"),
		DelayedOldRun.Error
			== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldRun.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const FGuid SecondRunSourceId = EnemyFixture.Enemy->GetCombatEntityId();
	const Fdemo_mapM01EnemyAttackExecutionResult SecondRunFirst =
		Fixture.Coordinator.ExecuteM01EnemyBasicMeleeStrike(
			EnemyFixture.Enemy,
			Fixture.Pawn,
			3.0f);
	const FGuid ExpectedSecondRunFirstActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunB,
			SecondRunSourceId,
			SecondRunFirst.Impact.GetRequest().Action
				.GetActionDefinitionId(),
			1);
	TestTrue(TEXT("New Run restarts per-enemy sequence under a distinct RunId"),
		SecondRunFirst.IsExecuted()
			&& SecondRunFirst.ActivationId
				== ExpectedSecondRunFirstActivation
			&& SecondRunFirst.ActivationId != First.ActivationId
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01EnemyMeleeDashProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyMeleeDashProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01EnemyMeleeDashProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture StandardEnemy;
	FM01MeleeEnemyFixture EnhancedEnemy(true);
	TestTrue(TEXT("Standard and enhanced dash fixtures initialize"),
		Fixture.bReady && StandardEnemy.bReady && EnhancedEnemy.bReady);
	if (!Fixture.bReady || !StandardEnemy.bReady || !EnhancedEnemy.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			StandardEnemy.Enemy,
			Fixture.Diagnostic)
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			EnhancedEnemy.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(
			Fixture.Pawn,
			TEXT("P48PlayerAttributes"));
	Fdemo_mapModifierSpec FlatReduction;
	FlatReduction.SourceId = TEXT("P4.8.Test.FlatReduction");
	FlatReduction.AttributeId =
		Fdemo_mapAttributeIds::FlatDamageReduction;
	FlatReduction.Operation = Edemo_mapModifierOperation::Add;
	FlatReduction.Value = 0.25f;
	Fdemo_mapModifierHandle FlatReductionHandle;
	TestTrue(TEXT("Dash target defense fixture binds"),
		Attributes
			&& Fixture.Health->BindAttributeComponent(Attributes, false)
			&& Attributes->AddModifier(
				FlatReduction,
				FlatReductionHandle));

	const Fdemo_mapM01EnemyAttackExecutionResult InvalidProfile =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			StandardEnemy.Enemy,
			Fixture.Pawn,
			TEXT("P4.8.Invalid.MeleeDash"),
			1,
			1.0f);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidSerial =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			StandardEnemy.Enemy,
			Fixture.Pawn,
			StandardEnemy.Definition->SkillProfileId,
			0,
			1.0f);
	const Fdemo_mapM01EnemyAttackExecutionResult MismatchedProfile =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			StandardEnemy.Enemy,
			Fixture.Pawn,
			EnhancedEnemy.Definition->SkillProfileId,
			1,
			1.0f);
	TestTrue(TEXT("Dash identity rejects invalid serial and authored-profile mismatch"),
		InvalidProfile.Error
			== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& InvalidSerial.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence
			&& MismatchedProfile.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& !InvalidProfile.ActivationId.IsValid()
			&& !InvalidSerial.ActivationId.IsValid()
			&& !MismatchedProfile.ActivationId.IsValid()
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult StandardFirst =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			StandardEnemy.Enemy,
			Fixture.Pawn,
			StandardEnemy.Definition->SkillProfileId,
			1,
			1.0f);
	const FGuid StandardSourceId =
		StandardEnemy.Enemy->GetCombatEntityId();
	const FGuid ExpectedStandardActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			StandardSourceId,
			TEXT("Combat.Action.Enemy.Melee.Dash.Standard"),
			1);
	TestTrue(TEXT("Standard dash resolves fractional defense and commits once"),
		StandardFirst.IsExecuted()
			&& StandardFirst.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::StandardMeleeDash
			&& StandardFirst.ActivationId == ExpectedStandardActivation
			&& StandardFirst.Impact.GetRequest().Action
				.GetActionDefinitionId()
				== TEXT("Combat.Action.Enemy.Melee.Dash.Standard")
			&& StandardFirst.Impact.GetRequest().Candidate.DetectorId
				== TEXT("Detector.Enemy.Melee.DashContact")
			&& StandardFirst.Impact.GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Enemy.Melee.Dash.r1")
			&& StandardFirst.Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				StandardFirst.Impact.GetResult().PreventedDamage,
				0.25f)
			&& FMath::IsNearlyEqual(
				StandardFirst.Impact.GetResult().FinalDamage,
				0.75f)
			&& FMath::IsNearlyEqual(
				StandardFirst.GetNewlyCommittedDamage(),
				0.75f)
			&& ShouldRequestEnemySkillKnockback(
				StandardFirst.GetNewlyCommittedDamage(),
				Fixture.Health->IsDefeated())
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapCombatImpactDeliveryResult StandardReceiptReplay =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			StandardFirst.Impact,
			StandardEnemy.Enemy);
	TestTrue(TEXT("Exact dash receipt replay is idempotent"),
		StandardReceiptReplay.IsSuccess()
			&& StandardReceiptReplay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapM01EnemyAttackExecutionResult StandardReconstruction =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			StandardEnemy.Enemy,
			Fixture.Pawn,
			StandardEnemy.Definition->SkillProfileId,
			1,
			1.0f);
	TestTrue(TEXT("Same dash serial cannot reconstruct a different snapshot"),
		!StandardReconstruction.IsExecuted()
			&& StandardReconstruction.Error
				== Edemo_mapM01EnemyAttackExecutionError::DeliveryRejected
			&& StandardReconstruction.ActivationId
				== StandardFirst.ActivationId
			&& StandardReconstruction.Impact.GetRequest().ImpactId
				== StandardFirst.Impact.GetRequest().ImpactId
			&& StandardReconstruction.Delivery.Error
				== Edemo_mapCombatImpactDeliveryError::CommitRejected
			&& FMath::IsNearlyZero(
				StandardReconstruction.GetNewlyCommittedDamage())
			&& !ShouldRequestEnemySkillKnockback(
				StandardReconstruction.GetNewlyCommittedDamage(),
				Fixture.Health->IsDefeated())
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapM01EnemyAttackExecutionResult EnhancedFirst =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			EnhancedEnemy.Enemy,
			Fixture.Pawn,
			EnhancedEnemy.Definition->SkillProfileId,
			1,
			1.0f);
	const FGuid ExpectedEnhancedActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			EnhancedEnemy.Enemy->GetCombatEntityId(),
			TEXT("Combat.Action.Enemy.Melee.Dash.Enhanced"),
			1);
	TestTrue(TEXT("Enhanced authored profile owns a distinct frozen dash family"),
		EnhancedFirst.IsExecuted()
			&& EnhancedFirst.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::EnhancedMeleeDash
			&& EnhancedFirst.ActivationId == ExpectedEnhancedActivation
			&& EnhancedFirst.ActivationId != StandardFirst.ActivationId
			&& EnhancedFirst.Impact.GetRequest().Action
				.GetActionDefinitionId()
				== TEXT("Combat.Action.Enemy.Melee.Dash.Enhanced")
			&& FMath::IsNearlyEqual(
				EnhancedFirst.GetNewlyCommittedDamage(),
				0.75f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 2
			&& Fixture.Health->NumCommittedCombatImpacts() == 2
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 2);

	TestTrue(TEXT("Exact dash Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Standard dash source binds a fresh Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				StandardEnemy.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldDash =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			StandardFirst.Impact,
			StandardEnemy.Enemy);
	TestTrue(TEXT("Old-Run dash receipt cannot mutate the rebound player"),
		DelayedOldDash.Error
			== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldDash.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult NewRunDash =
		Fixture.Coordinator.ExecuteM01EnemyMeleeDashContact(
			StandardEnemy.Enemy,
			Fixture.Pawn,
			StandardEnemy.Definition->SkillProfileId,
			1,
			10.0f);
	const FGuid ExpectedNewRunActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunB,
			StandardEnemy.Enemy->GetCombatEntityId(),
			TEXT("Combat.Action.Enemy.Melee.Dash.Standard"),
			1);
	TestTrue(TEXT("Run-reset defeating dash has new identity and no knockback"),
		NewRunDash.IsExecuted()
			&& NewRunDash.ActivationId == ExpectedNewRunActivation
			&& NewRunDash.ActivationId != StandardFirst.ActivationId
			&& FMath::IsNearlyEqual(
				NewRunDash.GetNewlyCommittedDamage(),
				3.5f)
			&& NewRunDash.DidNewCommitDefeatTarget()
			&& !ShouldRequestEnemySkillKnockback(
				NewRunDash.GetNewlyCommittedDamage(),
				NewRunDash.DidNewCommitDefeatTarget())
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				0.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01EnemyRangedProjectileProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyRangedProjectileProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01EnemyRangedProjectileProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01RangedEnemyFixture RangedEnemy;
	FM01MeleeEnemyFixture MeleeEnemy;
	TestTrue(TEXT("Ranged projectile fixtures initialize"),
		Fixture.bReady && RangedEnemy.bReady && MeleeEnemy.bReady);
	if (!Fixture.bReady || !RangedEnemy.bReady || !MeleeEnemy.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			RangedEnemy.Enemy,
			Fixture.Diagnostic)
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			MeleeEnemy.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(
			Fixture.Pawn,
			TEXT("P49PlayerAttributes"));
	Fdemo_mapModifierSpec FlatReduction;
	FlatReduction.SourceId = TEXT("P4.9.Test.FlatReduction");
	FlatReduction.AttributeId =
		Fdemo_mapAttributeIds::FlatDamageReduction;
	FlatReduction.Operation = Edemo_mapModifierOperation::Add;
	FlatReduction.Value = 0.25f;
	Fdemo_mapModifierHandle FlatReductionHandle;
	TestTrue(TEXT("Projectile target defense fixture binds"),
		Attributes
			&& Fixture.Health->BindAttributeComponent(Attributes, false)
			&& Attributes->AddModifier(
				FlatReduction,
				FlatReductionHandle));

	const FVector ImpactLocation(100.0, 25.0, 60.0);
	const FVector ImpactNormal(-1.0, 0.0, 0.0);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidProfile =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep,
			1,
			1.0f,
			ImpactLocation,
			ImpactNormal);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidSequence =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			0,
			1.0f,
			ImpactLocation,
			ImpactNormal);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidContact =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			1,
			1.0f,
			FVector(
				std::numeric_limits<double>::quiet_NaN(),
				0.0,
				0.0),
			ImpactNormal);
	const Fdemo_mapM01EnemyAttackExecutionResult WrongSourceFamily =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			MeleeEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			1,
			1.0f,
			ImpactLocation,
			ImpactNormal);
	TestTrue(TEXT("Projectile invalid identity and contact fail before mutation"),
		InvalidProfile.Error
			== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& InvalidSequence.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence
			&& InvalidContact.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidContact
			&& WrongSourceFamily.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& !InvalidProfile.ActivationId.IsValid()
			&& !InvalidSequence.ActivationId.IsValid()
			&& !InvalidContact.ActivationId.IsValid()
			&& !WrongSourceFamily.ActivationId.IsValid()
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult First =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			1,
			1.0f,
			ImpactLocation,
			ImpactNormal);
	const FGuid SourceEntityId = RangedEnemy.Enemy->GetCombatEntityId();
	const FGuid ExpectedActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			SourceEntityId,
			TEXT("Combat.Action.Enemy.Projectile.StandardRanged"),
			1);
	TestTrue(TEXT("Standard ranged projectile resolves and commits fractionally"),
		First.IsExecuted()
			&& First.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::StandardRangedProjectile
			&& First.ActivationId == ExpectedActivation
			&& First.Impact.GetRequest().Action.GetContent().Version
				== TEXT("0.0.10.P4.9")
			&& First.Impact.GetRequest().Candidate.DetectorId
				== TEXT("Detector.Enemy.Projectile.Contact")
			&& First.Impact.GetRequest().Candidate.DetectorKind
				== EShanmenHitDetectorKind::Projectile
			&& First.Impact.GetRequest().Candidate.HitLocation
				== ImpactLocation
			&& First.Impact.GetRequest().Candidate.HitNormal
				== ImpactNormal
			&& First.Impact.GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Enemy.Projectile.StandardRanged.r1")
			&& First.Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				First.Impact.GetResult().PreventedDamage,
				0.25f)
			&& FMath::IsNearlyEqual(
				First.GetNewlyCommittedDamage(),
				0.75f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			First.Impact,
			RangedEnemy.Enemy);
	TestTrue(TEXT("Exact projectile receipt replay is idempotent"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapM01EnemyAttackExecutionResult Reconstruction =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			1,
			1.0f,
			ImpactLocation,
			ImpactNormal);
	TestTrue(TEXT("Same projectile sequence cannot reconstruct a new snapshot"),
		!Reconstruction.IsExecuted()
			&& Reconstruction.Error
				== Edemo_mapM01EnemyAttackExecutionError::DeliveryRejected
			&& Reconstruction.ActivationId == First.ActivationId
			&& Reconstruction.Impact.GetRequest().ImpactId
				== First.Impact.GetRequest().ImpactId
			&& Reconstruction.Delivery.Error
				== Edemo_mapCombatImpactDeliveryError::CommitRejected
			&& FMath::IsNearlyZero(
				Reconstruction.GetNewlyCommittedDamage())
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1);

	const Fdemo_mapM01EnemyAttackExecutionResult Second =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			2,
			1.0f,
			ImpactLocation + FVector(25.0, 0.0, 0.0),
			ImpactNormal);
	TestTrue(TEXT("Next projectile sequence owns a distinct canonical action"),
		Second.IsExecuted()
			&& Second.ActivationId != First.ActivationId
			&& FMath::IsNearlyEqual(
				Second.GetNewlyCommittedDamage(),
				0.75f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 2
			&& Fixture.Health->NumCommittedCombatImpacts() == 2
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 2);

	TestTrue(TEXT("Projectile Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Ranged source binds a fresh Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				RangedEnemy.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			First.Impact,
			RangedEnemy.Enemy);
	TestTrue(TEXT("Old-Run projectile receipt cannot mutate rebound player"),
		DelayedOldRun.Error
			== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldRun.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult NewRunFirst =
		Fixture.Coordinator.ExecuteM01EnemyRangedProjectileImpact(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			RangedEnemy.Definition->SkillProfileId,
			1,
			10.0f,
			ImpactLocation,
			ImpactNormal);
	const FGuid ExpectedNewRunActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunB,
			RangedEnemy.Enemy->GetCombatEntityId(),
			TEXT("Combat.Action.Enemy.Projectile.StandardRanged"),
			1);
	TestTrue(TEXT("Run-reset projectile sequence derives a new lethal identity"),
		NewRunFirst.IsExecuted()
			&& NewRunFirst.ActivationId == ExpectedNewRunActivation
			&& NewRunFirst.ActivationId != First.ActivationId
			&& FMath::IsNearlyEqual(
				NewRunFirst.GetNewlyCommittedDamage(),
				3.5f)
			&& NewRunFirst.DidNewCommitDefeatTarget()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				0.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01EnemyHeavySectorProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyHeavySectorProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01EnemyHeavySectorProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01HeavyEnemyFixture StandardHeavy;
	FM01HeavyEnemyFixture EliteHeavy(true);
	FM01RangedEnemyFixture RangedEnemy;
	TestTrue(TEXT("Heavy-sector fixtures initialize"),
		Fixture.bReady
			&& StandardHeavy.bReady
			&& EliteHeavy.bReady
			&& RangedEnemy.bReady);
	if (!Fixture.bReady
		|| !StandardHeavy.bReady
		|| !EliteHeavy.bReady
		|| !RangedEnemy.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			StandardHeavy.Enemy,
			Fixture.Diagnostic)
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			EliteHeavy.Enemy,
			Fixture.Diagnostic)
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			RangedEnemy.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	TestTrue(TEXT("Heavy registration resets the Run-local action sequence"),
		StandardHeavy.Enemy->GetNextAttackSequence() == 1
			&& StandardHeavy.Enemy->GetActiveAttackSequence() == 0
			&& EliteHeavy.Enemy->GetNextAttackSequence() == 1
			&& EliteHeavy.Enemy->GetActiveAttackSequence() == 0);

	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(
			Fixture.Pawn,
			TEXT("P410PlayerAttributes"));
	Fdemo_mapModifierSpec FlatReduction;
	FlatReduction.SourceId = TEXT("P4.10.Test.FlatReduction");
	FlatReduction.AttributeId =
		Fdemo_mapAttributeIds::FlatDamageReduction;
	FlatReduction.Operation = Edemo_mapModifierOperation::Add;
	FlatReduction.Value = 0.25f;
	Fdemo_mapModifierHandle FlatReductionHandle;
	TestTrue(TEXT("Heavy target defense fixture binds"),
		Attributes
			&& Fixture.Health->BindAttributeComponent(Attributes, false)
			&& Attributes->AddModifier(
				FlatReduction,
				FlatReductionHandle));

	const Fdemo_mapM01EnemyAttackExecutionResult InvalidSequence =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			StandardHeavy.Enemy,
			Fixture.Pawn,
			0,
			2.0f);
	const Fdemo_mapM01EnemyAttackExecutionResult ExhaustedSequence =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			StandardHeavy.Enemy,
			Fixture.Pawn,
			MAX_uint64,
			2.0f);
	const Fdemo_mapM01EnemyAttackExecutionResult WrongSourceFamily =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			RangedEnemy.Enemy,
			Fixture.Pawn,
			1,
			2.0f);
	TestTrue(TEXT("Heavy invalid identity fails before vitality mutation"),
		InvalidSequence.Error
			== Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence
			&& ExhaustedSequence.Error
				== Edemo_mapM01EnemyAttackExecutionError::SequenceExhausted
			&& WrongSourceFamily.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& !InvalidSequence.ActivationId.IsValid()
			&& !ExhaustedSequence.ActivationId.IsValid()
			&& !WrongSourceFamily.ActivationId.IsValid()
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult First =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			StandardHeavy.Enemy,
			Fixture.Pawn,
			1,
			2.0f);
	const FGuid FirstSourceEntityId =
		StandardHeavy.Enemy->GetCombatEntityId();
	const FGuid ExpectedActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			FirstSourceEntityId,
			TEXT("Combat.Action.Enemy.Heavy.Sector"),
			1);
	TestTrue(TEXT("Standard heavy sector resolves through canonical defense"),
		First.IsExecuted()
			&& First.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::HeavySector
			&& First.ActivationId == ExpectedActivation
			&& First.Impact.GetRequest().Action.GetContent().Version
				== TEXT("0.0.10.P4.10")
			&& First.Impact.GetRequest().Candidate.DetectorId
				== TEXT("Detector.Enemy.Heavy.Sector")
			&& First.Impact.GetRequest().Candidate.DetectorKind
				== EShanmenHitDetectorKind::Shape
			&& First.Impact.GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Enemy.Heavy.Sector.r1")
			&& First.Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				First.Impact.GetResult().PreventedDamage,
				0.25f)
			&& FMath::IsNearlyEqual(
				First.GetNewlyCommittedDamage(),
				1.75f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			First.Impact,
			StandardHeavy.Enemy);
	const Fdemo_mapM01EnemyAttackExecutionResult Reconstruction =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			StandardHeavy.Enemy,
			Fixture.Pawn,
			1,
			2.0f);
	TestTrue(TEXT("Heavy receipt replay is idempotent and reconstruction fails"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& !Reconstruction.IsExecuted()
			&& Reconstruction.Error
				== Edemo_mapM01EnemyAttackExecutionError::DeliveryRejected
			&& Reconstruction.ActivationId == First.ActivationId
			&& Reconstruction.Impact.GetRequest().ImpactId
				== First.Impact.GetRequest().ImpactId
			&& Reconstruction.Delivery.Error
				== Edemo_mapCombatImpactDeliveryError::CommitRejected
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				3.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 1);

	const Fdemo_mapM01EnemyAttackExecutionResult Elite =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			EliteHeavy.Enemy,
			Fixture.Pawn,
			1,
			3.0f);
	TestTrue(TEXT("Elite bulwark shares formula but keeps source identity"),
		Elite.IsExecuted()
			&& Elite.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::HeavySector
			&& Elite.ActivationId != First.ActivationId
			&& Elite.Impact.GetRequest().Action.GetSourceEntityId()
				== EliteHeavy.Enemy->GetCombatEntityId()
			&& FMath::IsNearlyEqual(
				Elite.GetNewlyCommittedDamage(),
				2.75f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				0.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 2
			&& Fixture.Health->NumCommittedCombatImpacts() == 2
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 2);

	const Fdemo_mapM01EnemyAttackExecutionResult Second =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			StandardHeavy.Enemy,
			Fixture.Pawn,
			2,
			0.5f);
	TestTrue(TEXT("Next heavy sequence owns a distinct canonical action"),
		Second.IsExecuted()
			&& Second.ActivationId != First.ActivationId
			&& FMath::IsNearlyEqual(
				Second.GetNewlyCommittedDamage(),
				0.25f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				0.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 3
			&& Fixture.Health->NumCommittedCombatImpacts() == 3
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 3);

	TestTrue(TEXT("Heavy-sector Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Heavy source binds a fresh Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				StandardHeavy.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			First.Impact,
			StandardHeavy.Enemy);
	TestTrue(TEXT("Old-Run heavy receipt cannot mutate rebound player"),
		DelayedOldRun.Error
			== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& !DelayedOldRun.CommitResult.IsValid()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				0.25f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0
			&& StandardHeavy.Enemy->GetNextAttackSequence() == 1
			&& StandardHeavy.Enemy->GetActiveAttackSequence() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult NewRunFirst =
		Fixture.Coordinator.ExecuteM01EnemyHeavySectorAttack(
			StandardHeavy.Enemy,
			Fixture.Pawn,
			1,
			10.0f);
	const FGuid ExpectedNewRunActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunB,
			StandardHeavy.Enemy->GetCombatEntityId(),
			TEXT("Combat.Action.Enemy.Heavy.Sector"),
			1);
	TestTrue(TEXT("Run-reset heavy sequence derives a new lethal identity"),
		NewRunFirst.IsExecuted()
			&& NewRunFirst.ActivationId == ExpectedNewRunActivation
			&& NewRunFirst.ActivationId != First.ActivationId
			&& FMath::IsNearlyEqual(
				NewRunFirst.GetNewlyCommittedDamage(),
				0.25f)
			&& NewRunFirst.DidNewCommitDefeatTarget()
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				0.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorM01BossAttackProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.M01BossAttackProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorM01BossAttackProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01BossEnemyFixture Boss;
	FM01HeavyEnemyFixture Heavy;
	TestTrue(TEXT("Boss attack fixtures initialize"),
		Fixture.bReady && Boss.bReady && Heavy.bReady);
	if (!Fixture.bReady || !Boss.bReady || !Heavy.bReady
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			Boss.Enemy,
			Fixture.Diagnostic)
		|| !Fixture.Coordinator.TryRegisterM01Enemy(
			Heavy.Enemy,
			Fixture.Diagnostic))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	TestTrue(TEXT("Boss registration resets the Run-local action sequence"),
		Boss.Enemy->GetNextAttackSequence() == 1
			&& Boss.Enemy->GetActiveAttackSequence() == 0);

	const FVector ImpactLocation(240.0, 35.0, 70.0);
	const FVector ImpactNormal(-1.0, 0.0, 0.0);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidShapeSequence =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Boss.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Sweep,
			0,
			0.5f);
	const Fdemo_mapM01EnemyAttackExecutionResult ExhaustedShapeSequence =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Boss.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Sweep,
			MAX_uint64,
			0.5f);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidShapeFamily =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Boss.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Volley,
			1,
			0.5f);
	const Fdemo_mapM01EnemyAttackExecutionResult WrongBossSource =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Heavy.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Sweep,
			1,
			0.5f);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidVolleySequence =
		Fixture.Coordinator.ExecuteM01BossVolleyProjectileImpact(
			Boss.Enemy,
			Fixture.Pawn,
			0,
			0,
			0.5f,
			ImpactLocation,
			ImpactNormal);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidLowOrdinal =
		Fixture.Coordinator.ExecuteM01BossVolleyProjectileImpact(
			Boss.Enemy,
			Fixture.Pawn,
			1,
			-1,
			0.5f,
			ImpactLocation,
			ImpactNormal);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidHighOrdinal =
		Fixture.Coordinator.ExecuteM01BossVolleyProjectileImpact(
			Boss.Enemy,
			Fixture.Pawn,
			1,
			3,
			0.5f,
			ImpactLocation,
			ImpactNormal);
	const Fdemo_mapM01EnemyAttackExecutionResult InvalidVolleyContact =
		Fixture.Coordinator.ExecuteM01BossVolleyProjectileImpact(
			Boss.Enemy,
			Fixture.Pawn,
			1,
			0,
			0.5f,
			FVector(
				std::numeric_limits<double>::quiet_NaN(),
				0.0,
				0.0),
			ImpactNormal);
	TestTrue(TEXT("Boss malformed identity and contact fail before mutation"),
		InvalidShapeSequence.Error
			== Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence
			&& ExhaustedShapeSequence.Error
				== Edemo_mapM01EnemyAttackExecutionError::SequenceExhausted
			&& InvalidShapeFamily.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& WrongBossSource.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile
			&& InvalidVolleySequence.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence
			&& InvalidLowOrdinal.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidHitOrdinal
			&& InvalidHighOrdinal.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidHitOrdinal
			&& InvalidVolleyContact.Error
				== Edemo_mapM01EnemyAttackExecutionError::InvalidContact
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult Sweep =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Boss.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Sweep,
			1,
			0.5f);
	const FGuid SourceEntityId = Boss.Enemy->GetCombatEntityId();
	const FGuid ExpectedSweepActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			SourceEntityId,
			TEXT("Combat.Action.Enemy.Boss.Sweep"),
			1);
	TestTrue(TEXT("Boss sweep uses the frozen canonical shape contract"),
		Sweep.IsExecuted()
			&& Sweep.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::BossSweep
			&& Sweep.ActivationId == ExpectedSweepActivation
			&& Sweep.Impact.GetRequest().Action.GetContent().Version
				== TEXT("0.0.10.P4.11")
			&& Sweep.Impact.GetRequest().Candidate.DetectorId
				== TEXT("Detector.Enemy.Boss.Sweep")
			&& Sweep.Impact.GetRequest().Candidate.DetectorKind
				== EShanmenHitDetectorKind::Shape
			&& Sweep.Impact.GetRequest().Candidate.HitOrdinal == 0
			&& Sweep.Impact.GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Enemy.Boss.Sweep.r1")
			&& FMath::IsNearlyEqual(Sweep.GetNewlyCommittedDamage(), 0.5f)
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.5f));

	const Fdemo_mapCombatImpactDeliveryResult SweepReplay =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			Sweep.Impact,
			Boss.Enemy);
	TestTrue(TEXT("Exact Boss sweep receipt replay is idempotent"),
		SweepReplay.IsSuccess()
			&& SweepReplay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& Fixture.Health->GetCombatAuthorityRevision() == 1);

	const Fdemo_mapM01EnemyAttackExecutionResult Charge =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Boss.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Charge,
			2,
			0.5f);
	TestTrue(TEXT("Boss charge owns a distinct frozen action identity"),
		Charge.IsExecuted()
			&& Charge.Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::BossCharge
			&& Charge.ActivationId != Sweep.ActivationId
			&& Charge.Impact.GetRequest().Candidate.DetectorId
				== TEXT("Detector.Enemy.Boss.Charge")
			&& Charge.Impact.GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Enemy.Boss.Charge.r1")
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				4.0f));

	Fdemo_mapM01EnemyAttackExecutionResult Volley[3];
	for (int32 Ordinal = 0; Ordinal < 3; ++Ordinal)
	{
		Volley[Ordinal] =
			Fixture.Coordinator.ExecuteM01BossVolleyProjectileImpact(
				Boss.Enemy,
				Fixture.Pawn,
				3,
				Ordinal,
				0.5f,
				ImpactLocation + FVector(25.0 * Ordinal, 0.0, 0.0),
				ImpactNormal);
	}
	const FGuid ExpectedVolleyActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			SourceEntityId,
			TEXT("Combat.Action.Enemy.Boss.Volley"),
			3);
	TestTrue(TEXT("Boss volley shares one action and owns three impacts"),
		Volley[0].IsExecuted()
			&& Volley[1].IsExecuted()
			&& Volley[2].IsExecuted()
			&& Volley[0].Impact.GetFamily()
				== Edemo_mapM01EnemyAttackFamily::BossVolleyProjectile
			&& Volley[0].ActivationId == ExpectedVolleyActivation
			&& Volley[1].ActivationId == ExpectedVolleyActivation
			&& Volley[2].ActivationId == ExpectedVolleyActivation
			&& Volley[0].Impact.GetRequest().Candidate.HitOrdinal == 0
			&& Volley[1].Impact.GetRequest().Candidate.HitOrdinal == 1
			&& Volley[2].Impact.GetRequest().Candidate.HitOrdinal == 2
			&& Volley[0].Impact.GetRequest().ImpactId
				!= Volley[1].Impact.GetRequest().ImpactId
			&& Volley[1].Impact.GetRequest().ImpactId
				!= Volley[2].Impact.GetRequest().ImpactId
			&& Volley[0].Impact.GetRequest().Candidate.DetectorKind
				== EShanmenHitDetectorKind::Projectile
			&& Volley[0].Impact.GetRequest().Candidate.HitLocation
				== ImpactLocation
			&& Volley[2].Impact.GetRequest().Candidate.HitLocation
				== ImpactLocation + FVector(50.0, 0.0, 0.0)
			&& Volley[0].Impact.GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Enemy.Boss.Volley.r1")
			&& FMath::IsNearlyEqual(
				Fixture.Health->GetCurrentVitality(),
				2.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 5
			&& Fixture.Health->NumCommittedCombatImpacts() == 5
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 5);

	const Fdemo_mapM01EnemyAttackExecutionResult VolleyReconstruction =
		Fixture.Coordinator.ExecuteM01BossVolleyProjectileImpact(
			Boss.Enemy,
			Fixture.Pawn,
			3,
			1,
			0.5f,
			ImpactLocation + FVector(25.0, 0.0, 0.0),
			ImpactNormal);
	TestTrue(TEXT("Same Boss volley ordinal cannot reconstruct a snapshot"),
		!VolleyReconstruction.IsExecuted()
			&& VolleyReconstruction.Error
				== Edemo_mapM01EnemyAttackExecutionError::DeliveryRejected
			&& VolleyReconstruction.Impact.GetRequest().ImpactId
				== Volley[1].Impact.GetRequest().ImpactId
			&& Fixture.Health->GetCombatAuthorityRevision() == 5);

	TestTrue(TEXT("Boss attack Run release succeeds"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic));
	TestTrue(TEXT("Boss source binds a fresh Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				Boss.Enemy,
				Fixture.Diagnostic)
			&& Boss.Enemy->GetNextAttackSequence() == 1
			&& Boss.Enemy->GetActiveAttackSequence() == 0);
	const Fdemo_mapCombatImpactDeliveryResult DelayedOldRun =
		Fixture.Coordinator.DeliverM01EnemyAttackImpactToPlayer(
			Sweep.Impact,
			Boss.Enemy);
	TestTrue(TEXT("Old-Run Boss receipt cannot mutate rebound player"),
		DelayedOldRun.Error
			== Edemo_mapCombatImpactDeliveryError::RunMismatch
			&& Fixture.Health->GetCombatAuthorityRevision() == 0
			&& Fixture.Health->NumCommittedCombatImpacts() == 0);

	const Fdemo_mapM01EnemyAttackExecutionResult NewRunSweep =
		Fixture.Coordinator.ExecuteM01BossShapeAttack(
			Boss.Enemy,
			Fixture.Pawn,
			Edemo_mapM01BossAttack::Sweep,
			1,
			10.0f);
	const FGuid ExpectedNewRunActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunB,
			Boss.Enemy->GetCombatEntityId(),
			TEXT("Combat.Action.Enemy.Boss.Sweep"),
			1);
	TestTrue(TEXT("Run-reset Boss sequence derives a new lethal identity"),
		NewRunSweep.IsExecuted()
			&& NewRunSweep.ActivationId == ExpectedNewRunActivation
			&& NewRunSweep.ActivationId != Sweep.ActivationId
			&& FMath::IsNearlyEqual(
				NewRunSweep.GetNewlyCommittedDamage(),
				2.5f)
			&& NewRunSweep.DidNewCommitDefeatTarget()
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health
				->GetPositiveDamageBroadcastCountForAutomation() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorAllM01VitalityHostsTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.AllM01VitalityHosts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorAllM01VitalityHostsTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("All-host fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FShanmenCombatActionSnapshot Action = MakeCoordinatorAction(
		CoordinatorRunA,
		Fixture.Coordinator.GetPlayerEntityId(),
		99);
	TArray<AActor*> AuthoredActors;
	int32 VerifiedHosts = 0;
	for (const Fdemo_mapM01EnemyDefinition& Definition :
		Fdemo_mapM01EnemyConfig::GetDefinitions())
	{
		AActor* EnemyActor = NewM01ProductActor(Definition);
		Udemo_mapM01EnemyIdentityComponent* Identity = EnemyActor
			? NewObject<Udemo_mapM01EnemyIdentityComponent>(EnemyActor)
			: nullptr;
		if (EnemyActor && Identity)
		{
			EnemyActor->AddInstanceComponent(Identity);
		}
		Idemo_mapCombatVitalityHost* VitalityHost = EnemyActor
			? Cast<Idemo_mapCombatVitalityHost>(EnemyActor)
			: nullptr;
		const bool bPrepared = EnemyActor
			&& Identity
			&& Identity->Configure(Definition)
			&& ConfigureM01ProductActor(Definition, EnemyActor)
			&& VitalityHost
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyActor,
				Fixture.Diagnostic);
		TestTrue(
			FString::Printf(TEXT("Host prepares: %s"),
				*Definition.SpawnMarkerId.ToString()),
			bPrepared);
		if (!bPrepared)
		{
			continue;
		}
		AuthoredActors.Add(EnemyActor);

		FShanmenTargetVitalitySnapshot Before;
		FShanmenBasicSwordImpactReceipt Impact;
		const bool bResolved =
			VitalityHost->TryCaptureCombatVitalitySnapshot(Before)
			&& TryResolveSwordImpact(
				Fixture.Coordinator,
				Action,
				EnemyActor,
				Cast<UPrimitiveComponent>(EnemyActor->GetRootComponent()),
				Before,
				Impact,
				0.5f);
		TestTrue(
			FString::Printf(TEXT("Fractional Impact resolves: %s"),
				*Definition.SpawnMarkerId.ToString()),
			bResolved);
		if (!bResolved)
		{
			continue;
		}

		const Fdemo_mapCombatImpactDeliveryResult First =
			Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
				Impact,
				EnemyActor);
		const Fdemo_mapCombatImpactDeliveryResult Replay =
			Fixture.Coordinator.DeliverBasicSwordImpactToM01Enemy(
				Impact,
				EnemyActor);
		FShanmenTargetVitalitySnapshot AfterCanonical;
		const bool bCanonical = First.IsSuccess()
			&& First.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& VitalityHost->TryCaptureCombatVitalitySnapshot(AfterCanonical)
			&& FMath::IsNearlyEqual(
				AfterCanonical.CurrentVitality,
				Before.CurrentVitality - 0.5f)
			&& AfterCanonical.AuthorityRevision == 1
			&& VitalityHost->NumCommittedCombatImpacts() == 1
			&& VitalityHost->GetPositiveCombatDamageCountForAutomation() == 1;
		TestTrue(
			FString::Printf(TEXT("Canonical commit is fractional and idempotent: %s"),
				*Definition.SpawnMarkerId.ToString()),
			bCanonical);

		FDamageEvent LegacyDamageEvent;
		const float LegacyApplied = EnemyActor->TakeDamage(
			1.0f,
			LegacyDamageEvent,
			nullptr,
			nullptr);
		FShanmenTargetVitalitySnapshot AfterLegacy;
		const bool bLegacySynchronized = FMath::IsNearlyEqual(
				LegacyApplied,
				1.0f)
			&& VitalityHost->TryCaptureCombatVitalitySnapshot(AfterLegacy)
			&& FMath::IsNearlyEqual(
				AfterLegacy.CurrentVitality,
				AfterCanonical.CurrentVitality - 1.0f)
			&& AfterLegacy.AuthorityRevision == 2
			&& VitalityHost->NumCommittedCombatImpacts() == 1
			&& VitalityHost->GetPositiveCombatDamageCountForAutomation() == 2;
		TestTrue(
			FString::Printf(TEXT("Retained legacy mutation advances the shared revision: %s"),
				*Definition.SpawnMarkerId.ToString()),
			bLegacySynchronized);
		if (bCanonical && bLegacySynchronized)
		{
			++VerifiedHosts;
		}
	}

	TestTrue(TEXT("All 14 authored host instances pass the same contract"),
		AuthoredActors.Num() == 14
			&& VerifiedHosts == 14
			&& Fixture.Coordinator.NumVitalityBoundM01Enemies() == 14);

	TArray<FHitResult> ProductHits;
	TArray<float> VitalityBeforeProduct;
	for (AActor* EnemyActor : AuthoredActors)
	{
		Idemo_mapCombatVitalityHost* VitalityHost =
			Cast<Idemo_mapCombatVitalityHost>(EnemyActor);
		FShanmenTargetVitalitySnapshot Snapshot;
		if (!VitalityHost
			|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Snapshot))
		{
			continue;
		}
		ProductHits.Add(MakeProductSwordHit(EnemyActor));
		VitalityBeforeProduct.Add(Snapshot.CurrentVitality);
	}
	const Fdemo_mapBasicSwordProductExecutionResult ProductSweep =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			FGuid(0x54360001, 0, 0, 1),
			1.0f,
			ProductHits);
	bool bAllProductHostsCommitted = ProductHits.Num() == 14
		&& VitalityBeforeProduct.Num() == 14
		&& ProductSweep.IsExecuted()
		&& ProductSweep.WorldContactCount == 14
		&& ProductSweep.ResolvedCandidateCount == 14
		&& ProductSweep.DeliveredImpactCount == 14
		&& ProductSweep.CommittedImpactCount == 14
		&& ProductSweep.AlreadyCommittedImpactCount == 0;
	for (int32 Index = 0;
		bAllProductHostsCommitted && Index < AuthoredActors.Num();
		++Index)
	{
		Idemo_mapCombatVitalityHost* VitalityHost =
			Cast<Idemo_mapCombatVitalityHost>(AuthoredActors[Index]);
		FShanmenTargetVitalitySnapshot Snapshot;
		bAllProductHostsCommitted = VitalityHost
			&& VitalityHost->TryCaptureCombatVitalitySnapshot(Snapshot)
			&& FMath::IsNearlyEqual(
				Snapshot.CurrentVitality,
				VitalityBeforeProduct[Index] - 1.0f)
			&& Snapshot.AuthorityRevision == 3
			&& VitalityHost->NumCommittedCombatImpacts() == 2
			&& VitalityHost->GetPositiveCombatDamageCountForAutomation() == 3;
	}
	TestTrue(TEXT("One product sweep commits all 14 authored M01 host classes"),
		bAllProductHostsCommitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorProductSwordSweepTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.ProductBasicSwordSweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorProductSwordSweepTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Product sweep fixtures initialize"),
		Fixture.bReady && EnemyFixture.bReady
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	if (!Fixture.bReady || !EnemyFixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid WeaponInstanceId(0x54350001, 0, 0, 1);
	const FGuid ExpectedActivation = FShanmenCombatIdFactory::MakeActivationId(
		CoordinatorRunA,
		Fixture.Coordinator.GetPlayerEntityId(),
		FShanmenBasicSwordDefinition::CanonicalActionDefinitionId(),
		1);
	const float VitalityBefore = EnemyFixture.Enemy->GetCurrentVitality();
	const FHitResult Hit = MakeProductSwordHit(EnemyFixture.Enemy);
	const Fdemo_mapBasicSwordProductExecutionResult First =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			WeaponInstanceId,
			1.0f,
			{ Hit, Hit });
	TestTrue(TEXT("One real trajectory action has deterministic identity"),
		First.IsExecuted()
			&& First.ActivationId == ExpectedActivation
			&& Fixture.Coordinator
				.GetNextPlayerBasicSwordActivationSequence() == 2);
	TestTrue(TEXT("Duplicate geometry contacts commit the target exactly once"),
		First.WorldContactCount == 2
			&& First.ResolvedCandidateCount == 2
			&& First.DeliveredImpactCount == 1
			&& First.CommittedImpactCount == 1
			&& First.AlreadyCommittedImpactCount == 0
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 1.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1
			&& EnemyFixture.Enemy->NumCommittedCombatImpacts() == 1
			&& EnemyFixture.Enemy
				->GetPositiveCombatDamageCountForAutomation() == 1);

	const Fdemo_mapBasicSwordProductExecutionResult Miss =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			WeaponInstanceId,
			1.0f,
			{});
	TestTrue(TEXT("A legal miss closes normally and consumes a new activation"),
		Miss.IsExecuted()
			&& !Miss.AppliedDamage()
			&& Miss.ActivationId != First.ActivationId
			&& Miss.WorldContactCount == 0
			&& Miss.DeliveredImpactCount == 0
			&& Fixture.Coordinator
				.GetNextPlayerBasicSwordActivationSequence() == 3
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 1.0f));

	TestTrue(TEXT("Exact Run release resets the product activation sequence"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic)
			&& Fixture.Coordinator
				.GetNextPlayerBasicSwordActivationSequence() == 1);
	TestTrue(TEXT("Persistent hosts bind a second product Run"),
		Fixture.Coordinator.TryBeginRun(
			CoordinatorRunB,
			Fixture.Pawn,
			Fixture.Health,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	const Fdemo_mapBasicSwordProductExecutionResult NextRun =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			WeaponInstanceId,
			1.0f,
			{});
	TestTrue(TEXT("Sequence one in a new Run derives a different activation"),
		NextRun.IsExecuted()
			&& NextRun.ActivationId != First.ActivationId
			&& NextRun.ActivationId
				== FShanmenCombatIdFactory::MakeActivationId(
					CoordinatorRunB,
					Fixture.Coordinator.GetPlayerEntityId(),
					FShanmenBasicSwordDefinition::
						CanonicalActionDefinitionId(),
					1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorProductSwordFailClosedTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.ProductBasicSwordFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorProductSwordFailClosedTest::RunTest(
	const FString&)
{
	Fdemo_mapCombatRunCoordinator Empty;
	const FGuid WeaponInstanceId(0x54350002, 0, 0, 1);
	const Fdemo_mapBasicSwordProductExecutionResult NotReady =
		Empty.ExecutePlayerBasicSwordSweep(WeaponInstanceId, 1.0f, {});
	TestTrue(TEXT("Inactive coordinator rejects before action creation"),
		NotReady.Error
			== Edemo_mapBasicSwordProductExecutionError::CoordinatorNotReady
			&& !NotReady.ActivationId.IsValid());

	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("Fail-closed fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	const Fdemo_mapBasicSwordProductExecutionResult MissingWeapon =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(FGuid(), 1.0f, {});
	const Fdemo_mapBasicSwordProductExecutionResult InvalidOffense =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			WeaponInstanceId,
			std::numeric_limits<float>::quiet_NaN(),
			{});
	TestTrue(TEXT("Missing item and invalid offense fail without consuming sequence"),
		MissingWeapon.Error
				== Edemo_mapBasicSwordProductExecutionError::InvalidSourceItem
			&& InvalidOffense.Error
				== Edemo_mapBasicSwordProductExecutionError::InvalidOffense
			&& Fixture.Coordinator
				.GetNextPlayerBasicSwordActivationSequence() == 1);

	Ademo_mapEnemyCharacter* Unregistered =
		NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
	const float UnregisteredVitality = Unregistered->GetCurrentVitality();
	const Fdemo_mapBasicSwordProductExecutionResult UnregisteredHit =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			WeaponInstanceId,
			1.0f,
			{ MakeProductSwordHit(Unregistered) });
	TestTrue(TEXT("Unregistered world contact is ignored without legacy mutation"),
		UnregisteredHit.IsExecuted()
			&& !UnregisteredHit.AppliedDamage()
			&& UnregisteredHit.ResolvedCandidateCount == 0
			&& FMath::IsNearlyEqual(
				Unregistered->GetCurrentVitality(),
				UnregisteredVitality)
			&& !Unregistered->IsCombatEntityBound());

	FCombatRunCoordinatorFixture ReplayFixture;
	const Fdemo_mapBasicSwordProductExecutionResult Replay =
		ReplayFixture.Coordinator.ExecutePlayerBasicSwordSweep(
			WeaponInstanceId,
			1.0f,
			{});
	TestTrue(TEXT("Identical Run inputs replay the same first activation ID"),
		ReplayFixture.bReady
			&& Replay.IsExecuted()
			&& Replay.ActivationId == UnregisteredHit.ActivationId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorPlayerShapeSkillsProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.PlayerShapeSkillsProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorPlayerShapeSkillsProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture StandardEnemy(false);
	FM01MeleeEnemyFixture EnhancedEnemy(true);
	TestTrue(TEXT("Player shape-skill fixtures initialize and register"),
		Fixture.bReady
			&& StandardEnemy.bReady
			&& EnhancedEnemy.bReady
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				StandardEnemy.Enemy,
				Fixture.Diagnostic)
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnhancedEnemy.Enemy,
				Fixture.Diagnostic));
	if (!Fixture.bReady || !StandardEnemy.bReady || !EnhancedEnemy.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const FGuid StandardId = StandardEnemy.Enemy->GetCombatEntityId();
	const FGuid EnhancedId = EnhancedEnemy.Enemy->GetCombatEntityId();
	const float StandardVitalityBefore =
		StandardEnemy.Enemy->GetCurrentVitality();
	const float EnhancedVitalityBefore =
		EnhancedEnemy.Enemy->GetCurrentVitality();
	TestTrue(TEXT("Shape-skill vitality baselines are non-zero"),
		StandardVitalityBefore > 0.0f && EnhancedVitalityBefore > 0.0f);
	TArray<FGuid> ExpectedOrder = { StandardId, EnhancedId };
	ExpectedOrder.Sort(
		[](const FGuid& Left, const FGuid& Right)
		{
			return Left.ToString(EGuidFormats::Digits)
				< Right.ToString(EGuidFormats::Digits);
		});
	const TArray<FOverlapResult> ReverseDuplicateContacts = {
		MakeProductShapeOverlap(EnhancedEnemy.Enemy),
		MakeProductShapeOverlap(StandardEnemy.Enemy),
		MakeProductShapeOverlap(EnhancedEnemy.Enemy)
	};
	const FGuid ExpectedCircleActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			Fixture.Coordinator.GetPlayerEntityId(),
			TEXT("Combat.Action.Player.Skill.GroundCircle"),
			1);
	const Fdemo_mapPlayerShapeSkillExecutionResult Circle =
		Fixture.Coordinator.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::GroundCircle,
			1.0f,
			ReverseDuplicateContacts,
			FVector::ZeroVector);
	TestTrue(TEXT("Ground Circle owns one deterministic Run-local action"),
		Circle.IsExecuted()
			&& Circle.ActivationId == ExpectedCircleActivation
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::GroundCircle) == 2
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::SelfSector) == 1);
	TestTrue(TEXT("Duplicate overlap components collapse into stable target order"),
		Circle.WorldContactCount == 3
			&& Circle.ResolvedCandidateCount == 2
			&& Circle.DeliveredImpactCount == 2
			&& Circle.CommittedImpactCount == 2
			&& Circle.AlreadyCommittedImpactCount == 0
			&& Circle.OrderedTargetEntityIds == ExpectedOrder
			&& Circle.Impacts.Num() == 2);

	bool bCanonicalCircleReceipts = Circle.Impacts.Num() == 2;
	for (int32 Index = 0;
		bCanonicalCircleReceipts && Index < Circle.Impacts.Num();
		++Index)
	{
		const Fdemo_mapPlayerShapeSkillImpactReceipt& Impact =
			Circle.Impacts[Index];
		const FShanmenImpactRequest& Request = Impact.GetRequest();
		bCanonicalCircleReceipts = Impact.IsValid()
			&& Impact.GetFamily()
				== Edemo_mapPlayerShapeSkillFamily::GroundCircle
			&& Request.Action.GetActionDefinitionId()
				== TEXT("Combat.Action.Player.Skill.GroundCircle")
			&& Request.Action.GetContent().Version == TEXT("0.0.10.P4.12")
			&& Request.Candidate.DetectorId
				== TEXT("Detector.Player.Skill.GroundCircle")
			&& Request.Candidate.DetectorKind
				== EShanmenHitDetectorKind::Shape
			&& Request.Candidate.HitOrdinal == 0
			&& Request.Candidate.TargetEntityId == ExpectedOrder[Index]
			&& Request.Damage.FormulaId
				== TEXT("Combat.Formula.Player.Skill.GroundCircle.r1")
			&& Request.Damage.DamageTags.HasTag(
				FShanmenCombatNativeTags::DamagePhysical())
			&& FMath::IsNearlyEqual(
				Impact.GetResult().FinalDamage,
				1.0f)
			&& Impact.GetResult().IsConserved();
	}
	TestTrue(TEXT("Ground Circle receipts freeze the P4.12 shape contract"),
		bCanonicalCircleReceipts);
	TestTrue(TEXT("Both M01 vitality hosts commit Ground Circle exactly once"),
		FMath::IsNearlyEqual(
			StandardEnemy.Enemy->GetCurrentVitality(),
			StandardVitalityBefore - 1.0f)
			&& FMath::IsNearlyEqual(
				EnhancedEnemy.Enemy->GetCurrentVitality(),
				EnhancedVitalityBefore - 1.0f)
			&& StandardEnemy.Enemy->GetCombatAuthorityRevision() == 1
			&& EnhancedEnemy.Enemy->GetCombatAuthorityRevision() == 1);

	AActor* ReplayTarget = Circle.Impacts[0].GetRequest().Candidate.TargetEntityId
		== StandardId
		? static_cast<AActor*>(StandardEnemy.Enemy)
		: static_cast<AActor*>(EnhancedEnemy.Enemy);
	Idemo_mapCombatVitalityHost* ReplayHost =
		Cast<Idemo_mapCombatVitalityHost>(ReplayTarget);
	const int64 ReplayRevision = ReplayHost
		? ReplayHost->GetCombatAuthorityRevision()
		: INDEX_NONE;
	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverPlayerShapeSkillImpactToM01Enemy(
			Circle.Impacts[0],
			ReplayTarget);
	TestTrue(TEXT("Shape receipt replay is visible and cannot double-write"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& ReplayHost
			&& ReplayHost->GetCombatAuthorityRevision() == ReplayRevision);

	const FGuid ExpectedSectorActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			Fixture.Coordinator.GetPlayerEntityId(),
			TEXT("Combat.Action.Player.Skill.SelfSector"),
			1);
	const Fdemo_mapPlayerShapeSkillExecutionResult Sector =
		Fixture.Coordinator.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::SelfSector,
			1.0f,
			{ MakeProductShapeOverlap(StandardEnemy.Enemy) },
			FVector::ZeroVector);
	TestTrue(TEXT("Self Sector owns an independent deterministic sequence"),
		Sector.IsExecuted()
			&& Sector.ActivationId == ExpectedSectorActivation
			&& Sector.DeliveredImpactCount == 1
			&& Sector.CommittedImpactCount == 1
			&& Sector.Impacts.Num() == 1
			&& Sector.Impacts[0].GetFamily()
				== Edemo_mapPlayerShapeSkillFamily::SelfSector
			&& Sector.Impacts[0].GetRequest().Candidate.DetectorId
				== TEXT("Detector.Player.Skill.SelfSector")
			&& Sector.Impacts[0].GetRequest().Damage.FormulaId
				== TEXT("Combat.Formula.Player.Skill.SelfSector.r1")
			&& FMath::IsNearlyEqual(
				StandardEnemy.Enemy->GetCurrentVitality(),
				StandardVitalityBefore - 2.0f)
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::SelfSector) == 2);

	TestTrue(TEXT("Exact Run release resets both player skill sequences"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic)
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::GroundCircle) == 1
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::SelfSector) == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorPlayerShapeSkillsFailClosedTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.PlayerShapeSkillsFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorPlayerShapeSkillsFailClosedTest::RunTest(
	const FString&)
{
	Fdemo_mapCombatRunCoordinator Empty;
	const Fdemo_mapPlayerShapeSkillExecutionResult NotReady =
		Empty.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::GroundCircle,
			1.0f,
			{},
			FVector::ZeroVector);
	TestTrue(TEXT("Inactive coordinator rejects before action identity"),
		NotReady.Error
			== Edemo_mapPlayerShapeSkillExecutionError::CoordinatorNotReady
			&& !NotReady.ActivationId.IsValid());

	FCombatRunCoordinatorFixture Fixture;
	TestTrue(TEXT("Player shape fail-closed fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	const Fdemo_mapPlayerShapeSkillExecutionResult InvalidFamily =
		Fixture.Coordinator.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::None,
			1.0f,
			{},
			FVector::ZeroVector);
	const Fdemo_mapPlayerShapeSkillExecutionResult InvalidDamage =
		Fixture.Coordinator.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::GroundCircle,
			std::numeric_limits<float>::quiet_NaN(),
			{},
			FVector::ZeroVector);
	TestTrue(TEXT("Invalid family and damage do not consume either sequence"),
		InvalidFamily.Error
				== Edemo_mapPlayerShapeSkillExecutionError::InvalidFamily
			&& InvalidDamage.Error
				== Edemo_mapPlayerShapeSkillExecutionError::InvalidDamage
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::GroundCircle) == 1
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::SelfSector) == 1);

	Ademo_mapEnemyCharacter* Unregistered =
		NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
	const float VitalityBefore = Unregistered->GetCurrentVitality();
	const Fdemo_mapPlayerShapeSkillExecutionResult IgnoredContact =
		Fixture.Coordinator.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::GroundCircle,
			1.0f,
			{ MakeProductShapeOverlap(Unregistered) },
			FVector::ZeroVector);
	TestTrue(TEXT("Unregistered overlap closes without legacy mutation"),
		IgnoredContact.IsExecuted()
			&& IgnoredContact.WorldContactCount == 1
			&& IgnoredContact.ResolvedCandidateCount == 0
			&& IgnoredContact.DeliveredImpactCount == 0
			&& !IgnoredContact.AppliedDamage()
			&& FMath::IsNearlyEqual(
				Unregistered->GetCurrentVitality(),
				VitalityBefore)
			&& !Unregistered->IsCombatEntityBound());

	FCombatRunCoordinatorFixture ReplayFixture;
	const Fdemo_mapPlayerShapeSkillExecutionResult DeterministicReplay =
		ReplayFixture.Coordinator.ExecutePlayerShapeSkill(
			Edemo_mapPlayerShapeSkillFamily::GroundCircle,
			1.0f,
			{},
			FVector::ZeroVector);
	TestTrue(TEXT("Identical first shape action replays the activation ID"),
		ReplayFixture.bReady
			&& DeterministicReplay.IsExecuted()
			&& DeterministicReplay.ActivationId
				== IgnoredContact.ActivationId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorPlayerStraightProjectileProductTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.PlayerStraightProjectileProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorPlayerStraightProjectileProductTest::RunTest(
	const FString&)
{
	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Player projectile fixtures initialize and register"),
		Fixture.bReady
			&& EnemyFixture.bReady
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	if (!Fixture.bReady || !EnemyFixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const float VitalityBefore = EnemyFixture.Enemy->GetCurrentVitality();
	TestTrue(TEXT("Projectile target baseline is non-zero"),
		VitalityBefore > 0.0f);
	const FGuid ExpectedActivation =
		FShanmenCombatIdFactory::MakeActivationId(
			CoordinatorRunA,
			Fixture.Coordinator.GetPlayerEntityId(),
			TEXT("Combat.Action.Player.Skill.StraightProjectile"),
			1);
	const Fdemo_mapPlayerProjectileLaunchResult Launch =
		Fixture.Coordinator.PreparePlayerStraightProjectile(
			Fixture.Pawn,
			1.0f);
	TestTrue(TEXT("Launch reserves one deterministic projectile action"),
		Launch.IsPrepared()
			&& Launch.ActivationSequence == 1
			&& Launch.ActivationId == ExpectedActivation
			&& FMath::IsNearlyEqual(Launch.RawDamage, 1.0f)
			&& Fixture.Coordinator
				.GetNextPlayerStraightProjectileActivationSequence() == 2
			&& Fixture.Coordinator.GetNextPlayerShapeSkillActivationSequence(
				Edemo_mapPlayerShapeSkillFamily::GroundCircle) == 1);

	const FHitResult WorldHit = MakeProductProjectileHit(EnemyFixture.Enemy);
	const Fdemo_mapPlayerProjectileImpactResult Impact =
		Fixture.Coordinator.ExecutePlayerStraightProjectileImpact(
			Fixture.Pawn,
			EnemyFixture.Enemy,
			WorldHit.GetComponent(),
			Launch.ActivationSequence,
			Launch.ActivationId,
			Launch.RawDamage,
			WorldHit.ImpactPoint,
			WorldHit.ImpactNormal);
	const FShanmenImpactRequest& Request = Impact.Impact.GetRequest();
	const FGuid ExpectedImpact = FShanmenCombatIdFactory::MakeImpactId(
		CoordinatorRunA,
		ExpectedActivation,
		TEXT("Detector.Player.Skill.StraightProjectile.Projectile"),
		EnemyFixture.Enemy->GetCombatEntityId(),
		0);
	TestTrue(TEXT("Hostile contact resolves the frozen P4.13 contract"),
		Impact.IsExecuted()
			&& Impact.ActivationSequence == 1
			&& Impact.ActivationId == ExpectedActivation
			&& Request.Action.GetActionDefinitionId()
				== TEXT("Combat.Action.Player.Skill.StraightProjectile")
			&& Request.Action.GetContent().Version == TEXT("0.0.10.P4.13")
			&& Request.Action.GetContent().Digest
				== TEXT("Shanmen.Player.Skill.StraightProjectile.Projectile.r1")
			&& Request.Action.GetSourceTags().HasTag(
				FShanmenCombatNativeTags::SourcePlayer())
			&& Request.Candidate.DetectorId
				== TEXT("Detector.Player.Skill.StraightProjectile.Projectile")
			&& Request.Candidate.DetectorKind
				== EShanmenHitDetectorKind::Projectile
			&& Request.Candidate.HitOrdinal == 0
			&& Request.Candidate.TargetEntityId
				== EnemyFixture.Enemy->GetCombatEntityId()
			&& Request.Damage.FormulaId
				== TEXT("Combat.Formula.Player.Skill.StraightProjectile.r1")
			&& Request.Damage.DamageTags.HasTag(
				FShanmenCombatNativeTags::DamagePhysical())
			&& Request.ImpactId == ExpectedImpact
			&& Impact.Impact.GetResult().IsConserved());
	TestTrue(TEXT("Projectile contact commits target vitality exactly once"),
		Impact.Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				Impact.GetNewlyCommittedDamage(),
				1.0f)
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 1.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1);

	const Fdemo_mapCombatImpactDeliveryResult Replay =
		Fixture.Coordinator.DeliverPlayerProjectileImpactToM01Enemy(
			Impact.Impact,
			EnemyFixture.Enemy);
	TestTrue(TEXT("Exact projectile receipt replay cannot double-write"),
		Replay.IsSuccess()
			&& Replay.CommitResult.Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore - 1.0f)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 1);

	const Fdemo_mapPlayerProjectileLaunchResult SecondLaunch =
		Fixture.Coordinator.PreparePlayerStraightProjectile(
			Fixture.Pawn,
			2.0f);
	TestTrue(TEXT("Each fired projectile owns the next action identity"),
		SecondLaunch.IsPrepared()
			&& SecondLaunch.ActivationSequence == 2
			&& SecondLaunch.ActivationId
				!= Launch.ActivationId
			&& Fixture.Coordinator
				.GetNextPlayerStraightProjectileActivationSequence() == 3);
	TestTrue(TEXT("Exact Run release resets projectile sequence"),
		Fixture.Coordinator.TryEndRun(
			CoordinatorRunA,
			Fixture.Diagnostic)
			&& Fixture.Coordinator
				.GetNextPlayerStraightProjectileActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatRunCoordinatorPlayerStraightProjectileFailClosedTest,
	"Shanmen.0_0_10.Product.CombatRunCoordinator.PlayerStraightProjectileFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatRunCoordinatorPlayerStraightProjectileFailClosedTest::
RunTest(const FString&)
{
	Fdemo_mapCombatRunCoordinator Empty;
	const Fdemo_mapPlayerProjectileLaunchResult NotReady =
		Empty.PreparePlayerStraightProjectile(nullptr, 1.0f);
	TestTrue(TEXT("Inactive coordinator rejects launch before identity"),
		NotReady.Error
			== Edemo_mapPlayerProjectileLaunchError::CoordinatorNotReady
			&& !NotReady.ActivationId.IsValid());

	FCombatRunCoordinatorFixture Fixture;
	FM01MeleeEnemyFixture EnemyFixture;
	TestTrue(TEXT("Player projectile fail-closed fixtures initialize"),
		Fixture.bReady
			&& EnemyFixture.bReady
			&& Fixture.Coordinator.TryRegisterM01Enemy(
				EnemyFixture.Enemy,
				Fixture.Diagnostic));
	if (!Fixture.bReady || !EnemyFixture.bReady)
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const Fdemo_mapPlayerProjectileLaunchResult WrongSource =
		Fixture.Coordinator.PreparePlayerStraightProjectile(
			EnemyFixture.Enemy,
			1.0f);
	const Fdemo_mapPlayerProjectileLaunchResult ZeroDamage =
		Fixture.Coordinator.PreparePlayerStraightProjectile(
			Fixture.Pawn,
			0.0f);
	const Fdemo_mapPlayerProjectileLaunchResult NaNDamage =
		Fixture.Coordinator.PreparePlayerStraightProjectile(
			Fixture.Pawn,
			std::numeric_limits<float>::quiet_NaN());
	TestTrue(TEXT("Invalid launch inputs do not consume sequence"),
		WrongSource.Error
			== Edemo_mapPlayerProjectileLaunchError::SourceMismatch
			&& ZeroDamage.Error
				== Edemo_mapPlayerProjectileLaunchError::InvalidDamage
			&& NaNDamage.Error
				== Edemo_mapPlayerProjectileLaunchError::InvalidDamage
			&& Fixture.Coordinator
				.GetNextPlayerStraightProjectileActivationSequence() == 1);

	const Fdemo_mapPlayerProjectileLaunchResult Launch =
		Fixture.Coordinator.PreparePlayerStraightProjectile(
			Fixture.Pawn,
			1.0f);
	const FHitResult WorldHit = MakeProductProjectileHit(EnemyFixture.Enemy);
	const float VitalityBefore = EnemyFixture.Enemy->GetCurrentVitality();
	const Fdemo_mapPlayerProjectileImpactResult WrongIdentity =
		Fixture.Coordinator.ExecutePlayerStraightProjectileImpact(
			Fixture.Pawn,
			EnemyFixture.Enemy,
			WorldHit.GetComponent(),
			Launch.ActivationSequence,
			FGuid(0x54370001, 0, 0, 1),
			Launch.RawDamage,
			WorldHit.ImpactPoint,
			WorldHit.ImpactNormal);
	const Fdemo_mapPlayerProjectileImpactResult InvalidContact =
		Fixture.Coordinator.ExecutePlayerStraightProjectileImpact(
			Fixture.Pawn,
			EnemyFixture.Enemy,
			WorldHit.GetComponent(),
			Launch.ActivationSequence,
			Launch.ActivationId,
			Launch.RawDamage,
			FVector(
				std::numeric_limits<double>::quiet_NaN(),
				0.0,
				0.0),
			WorldHit.ImpactNormal);
	TestTrue(TEXT("Identity and contact corruption fail before mutation"),
		Launch.IsPrepared()
			&& WrongIdentity.Error
				== Edemo_mapPlayerProjectileImpactError::InvalidLaunchIdentity
			&& InvalidContact.Error
				== Edemo_mapPlayerProjectileImpactError::InvalidContact
			&& FMath::IsNearlyEqual(
				EnemyFixture.Enemy->GetCurrentVitality(),
				VitalityBefore)
			&& EnemyFixture.Enemy->GetCombatAuthorityRevision() == 0);

	Ademo_mapEnemyCharacter* Unregistered =
		NewObject<Ademo_mapEnemyCharacter>(GetTransientPackage());
	const float UnregisteredVitality = Unregistered->GetCurrentVitality();
	const FHitResult UnregisteredHit = MakeProductProjectileHit(Unregistered);
	const Fdemo_mapPlayerProjectileImpactResult UnregisteredContact =
		Fixture.Coordinator.ExecutePlayerStraightProjectileImpact(
			Fixture.Pawn,
			Unregistered,
			UnregisteredHit.GetComponent(),
			Launch.ActivationSequence,
			Launch.ActivationId,
			Launch.RawDamage,
			UnregisteredHit.ImpactPoint,
			UnregisteredHit.ImpactNormal);
	TestTrue(TEXT("Unregistered hostile contact cannot reach legacy mutation"),
		UnregisteredContact.Error
			== Edemo_mapPlayerProjectileImpactError::TargetNotRegistered
			&& FMath::IsNearlyEqual(
				Unregistered->GetCurrentVitality(),
				UnregisteredVitality)
			&& !Unregistered->IsCombatEntityBound());

	FCombatRunCoordinatorFixture ReplayFixture;
	const Fdemo_mapPlayerProjectileLaunchResult DeterministicLaunch =
		ReplayFixture.Coordinator.PreparePlayerStraightProjectile(
			ReplayFixture.Pawn,
			1.0f);
	TestTrue(TEXT("Identical first launch replays deterministic identity"),
		ReplayFixture.bReady
			&& DeterministicLaunch.IsPrepared()
			&& DeterministicLaunch.ActivationId == Launch.ActivationId);
	return true;
}

#endif
