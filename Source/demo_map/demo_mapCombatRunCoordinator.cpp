#include "demo_mapCombatRunCoordinator.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenControlledWeaponExecution.h"
#include "ShanmenThrownWeaponExecution.h"
#include "ShanmenWorldHitAdapter.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapM01BossCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "demo_mapShanmenDefenseResourceAdapter.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	bool IsSuccessfulBinding(EShanmenWorldBindingResult Result)
	{
		return Result == EShanmenWorldBindingResult::Bound
			|| Result == EShanmenWorldBindingResult::AlreadyBound;
	}

	bool IsExpectedM01ProductActor(
		const Fdemo_mapM01EnemyDefinition& Definition,
		const AActor* EnemyActor)
	{
		switch (Definition.Archetype)
		{
		case Edemo_mapM01EnemyArchetype::StandardSkirmisher:
		case Edemo_mapM01EnemyArchetype::EliteStalker:
			return EnemyActor->IsA<Ademo_mapEnemyCharacter>();
		case Edemo_mapM01EnemyArchetype::StandardRanged:
			return EnemyActor->IsA<Ademo_mapRangedEnemyCharacter>();
		case Edemo_mapM01EnemyArchetype::StandardBruiser:
		case Edemo_mapM01EnemyArchetype::EliteBulwark:
			return EnemyActor->IsA<Ademo_mapHeavyEnemyCharacter>();
		case Edemo_mapM01EnemyArchetype::BossMain:
			return EnemyActor->IsA<Ademo_mapM01BossCharacter>();
		default:
			return false;
		}
	}

	Idemo_mapCombatVitalityHost* ResolveM01VitalityHost(AActor* EnemyActor)
	{
		return EnemyActor
			? Cast<Idemo_mapCombatVitalityHost>(EnemyActor)
			: nullptr;
	}

	struct FM01EnemyAttackSpec
	{
		Edemo_mapM01EnemyAttackFamily Family =
			Edemo_mapM01EnemyAttackFamily::None;
		FName ActionDefinitionId = NAME_None;
		FName DetectorId = NAME_None;
		EShanmenHitDetectorKind DetectorKind =
			EShanmenHitDetectorKind::Shape;
		FName FormulaId = NAME_None;
		FName ContentVersion = NAME_None;
		FString ContentDigest;
		int32 MaxHitOrdinal = 0;

		bool IsValid() const
		{
			return Family != Edemo_mapM01EnemyAttackFamily::None
				&& !ActionDefinitionId.IsNone()
				&& !DetectorId.IsNone()
				&& !FormulaId.IsNone()
				&& !ContentVersion.IsNone()
				&& !ContentDigest.IsEmpty()
				&& MaxHitOrdinal >= 0;
		}
	};

	bool TryGetM01EnemyAttackSpec(
		Edemo_mapM01EnemyAttackFamily Family,
		FM01EnemyAttackSpec& OutSpec)
	{
		OutSpec = FM01EnemyAttackSpec();
		OutSpec.Family = Family;

		switch (Family)
		{
		case Edemo_mapM01EnemyAttackFamily::BasicMelee:
			OutSpec.DetectorId = TEXT("Detector.Enemy.Melee.Contact");
			OutSpec.ContentVersion = TEXT("0.0.10.P4.7");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Melee.Basic01");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Melee.Basic01.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.M01Enemy.BasicMelee.Contact.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::StandardMeleeDash:
			OutSpec.DetectorId =
				TEXT("Detector.Enemy.Melee.DashContact");
			OutSpec.ContentVersion = TEXT("0.0.10.P4.8");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Melee.Dash.Standard");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Melee.Dash.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.M01Enemy.MeleeDash.Standard.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::EnhancedMeleeDash:
			OutSpec.DetectorId =
				TEXT("Detector.Enemy.Melee.DashContact");
			OutSpec.ContentVersion = TEXT("0.0.10.P4.8");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Melee.Dash.Enhanced");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Melee.Dash.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.M01Enemy.MeleeDash.Enhanced.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::StandardRangedProjectile:
			OutSpec.DetectorId =
				TEXT("Detector.Enemy.Projectile.Contact");
			OutSpec.DetectorKind = EShanmenHitDetectorKind::Projectile;
			OutSpec.ContentVersion = TEXT("0.0.10.P4.9");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Projectile.StandardRanged");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Projectile.StandardRanged.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.M01Enemy.StandardRanged.Projectile.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::HeavySector:
			OutSpec.DetectorId = TEXT("Detector.Enemy.Heavy.Sector");
			OutSpec.ContentVersion = TEXT("0.0.10.P4.10");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Heavy.Sector");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Heavy.Sector.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.M01Enemy.Heavy.Sector.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::BossSweep:
			OutSpec.DetectorId = TEXT("Detector.Enemy.Boss.Sweep");
			OutSpec.ContentVersion = TEXT("0.0.10.P4.11");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Boss.Sweep");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Boss.Sweep.r1");
			OutSpec.ContentDigest = TEXT("Shanmen.M01Boss.Sweep.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::BossCharge:
			OutSpec.DetectorId = TEXT("Detector.Enemy.Boss.Charge");
			OutSpec.ContentVersion = TEXT("0.0.10.P4.11");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Boss.Charge");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Boss.Charge.r1");
			OutSpec.ContentDigest = TEXT("Shanmen.M01Boss.Charge.r1");
			break;
		case Edemo_mapM01EnemyAttackFamily::BossVolleyProjectile:
			OutSpec.DetectorId =
				TEXT("Detector.Enemy.Boss.Volley.Projectile");
			OutSpec.DetectorKind = EShanmenHitDetectorKind::Projectile;
			OutSpec.ContentVersion = TEXT("0.0.10.P4.11");
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Enemy.Boss.Volley");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Enemy.Boss.Volley.r1");
			OutSpec.ContentDigest = TEXT("Shanmen.M01Boss.Volley.r1");
			OutSpec.MaxHitOrdinal = 2;
			break;
		default:
			return false;
		}
		return OutSpec.IsValid();
	}

	struct FPlayerShapeSkillSpec
	{
		Edemo_mapPlayerShapeSkillFamily Family =
			Edemo_mapPlayerShapeSkillFamily::None;
		FName ActionDefinitionId = NAME_None;
		FName DetectorId = NAME_None;
		FName FormulaId = NAME_None;
		FName ContentVersion = NAME_None;
		FString ContentDigest;

		bool IsValid() const
		{
			return Family != Edemo_mapPlayerShapeSkillFamily::None
				&& !ActionDefinitionId.IsNone()
				&& !DetectorId.IsNone()
				&& !FormulaId.IsNone()
				&& !ContentVersion.IsNone()
				&& !ContentDigest.IsEmpty();
		}
	};

	bool TryGetPlayerShapeSkillSpec(
		Edemo_mapPlayerShapeSkillFamily Family,
		FPlayerShapeSkillSpec& OutSpec)
	{
		OutSpec = FPlayerShapeSkillSpec();
		OutSpec.Family = Family;
		OutSpec.ContentVersion = TEXT("0.0.10.P4.12");
		switch (Family)
		{
		case Edemo_mapPlayerShapeSkillFamily::GroundCircle:
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Player.Skill.GroundCircle");
			OutSpec.DetectorId =
				TEXT("Detector.Player.Skill.GroundCircle");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Player.Skill.GroundCircle.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.Player.Skill.GroundCircle.Shape.r1");
			break;
		case Edemo_mapPlayerShapeSkillFamily::SelfSector:
			OutSpec.ActionDefinitionId =
				TEXT("Combat.Action.Player.Skill.SelfSector");
			OutSpec.DetectorId =
				TEXT("Detector.Player.Skill.SelfSector");
			OutSpec.FormulaId =
				TEXT("Combat.Formula.Player.Skill.SelfSector.r1");
			OutSpec.ContentDigest =
				TEXT("Shanmen.Player.Skill.SelfSector.Shape.r1");
			break;
		default:
			return false;
		}
		return OutSpec.IsValid();
	}

	bool TryBuildPlayerShapeSkillAction(
		const FPlayerShapeSkillSpec& Spec,
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		uint64 ActivationSequence,
		FShanmenCombatActionSnapshot& OutAction)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = PlayerEntityId;
		Capture.SourceEntityId = PlayerEntityId;
		Capture.ActionDefinitionId = Spec.ActionDefinitionId;
		Capture.Content.Version = Spec.ContentVersion;
		Capture.Content.Digest = Spec.ContentDigest;
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			RunId,
			PlayerEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		return FShanmenCombatActionSnapshot::TryCapture(Capture, OutAction);
	}

	struct FPlayerStraightProjectileSpec
	{
		FName ActionDefinitionId =
			TEXT("Combat.Action.Player.Skill.StraightProjectile");
		FName DetectorId =
			TEXT("Detector.Player.Skill.StraightProjectile.Projectile");
		FName FormulaId =
			TEXT("Combat.Formula.Player.Skill.StraightProjectile.r1");
		FName ContentVersion = TEXT("0.0.10.P4.13");
		FString ContentDigest =
			TEXT("Shanmen.Player.Skill.StraightProjectile.Projectile.r1");

		bool IsValid() const
		{
			return !ActionDefinitionId.IsNone()
				&& !DetectorId.IsNone()
				&& !FormulaId.IsNone()
				&& !ContentVersion.IsNone()
				&& !ContentDigest.IsEmpty();
		}
	};

	bool TryBuildPlayerStraightProjectileAction(
		const FPlayerStraightProjectileSpec& Spec,
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		uint64 ActivationSequence,
		FShanmenCombatActionSnapshot& OutAction)
	{
		if (!Spec.IsValid() || ActivationSequence == 0)
		{
			return false;
		}
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = PlayerEntityId;
		Capture.SourceEntityId = PlayerEntityId;
		Capture.ActionDefinitionId = Spec.ActionDefinitionId;
		Capture.Content.Version = Spec.ContentVersion;
		Capture.Content.Digest = Spec.ContentDigest;
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			RunId,
			PlayerEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		return FShanmenCombatActionSnapshot::TryCapture(Capture, OutAction);
	}

	bool TryResolveM01MeleeDashFamily(
		FName SkillProfileId,
		Edemo_mapM01EnemyAttackFamily& OutFamily)
	{
		OutFamily = Edemo_mapM01EnemyAttackFamily::None;
		if (SkillProfileId
			== Fdemo_mapEnemySkillProfileIds::StandardMeleeDash)
		{
			OutFamily =
				Edemo_mapM01EnemyAttackFamily::StandardMeleeDash;
			return true;
		}
		if (SkillProfileId
			== Fdemo_mapEnemySkillProfileIds::EnhancedMeleeDash)
		{
			OutFamily =
				Edemo_mapM01EnemyAttackFamily::EnhancedMeleeDash;
			return true;
		}
		return false;
	}

	bool TryResolveM01RangedProjectileFamily(
		FName SkillProfileId,
		Edemo_mapM01EnemyAttackFamily& OutFamily)
	{
		OutFamily = Edemo_mapM01EnemyAttackFamily::None;
		if (SkillProfileId
			!= Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep)
		{
			return false;
		}
		OutFamily =
			Edemo_mapM01EnemyAttackFamily::StandardRangedProjectile;
		return true;
	}

	bool TryBuildM01EnemyAttackAction(
		const FM01EnemyAttackSpec& Spec,
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		uint64 ActivationSequence,
		FShanmenCombatActionSnapshot& OutAction)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = SourceEntityId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId = Spec.ActionDefinitionId;
		Capture.Content.Version = Spec.ContentVersion;
		Capture.Content.Digest = Spec.ContentDigest;
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			RunId,
			SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		return FShanmenCombatActionSnapshot::TryCapture(Capture, OutAction);
	}

	bool TryBuildProductBasicSwordDefinition(
		FShanmenBasicSwordDefinition& OutDefinition)
	{
		FShanmenBasicSwordDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Weapon.Main");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Basic01.Product.r1");
		Capture.BaseDamage = 0.0f;
		Capture.AttackPowerCoefficient = 1.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		return FShanmenBasicSwordDefinition::TryCapture(
			Capture,
			OutDefinition);
	}

	bool TryBuildProductBasicSwordAction(
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		const FGuid& SourceItemInstanceId,
		uint64 ActivationSequence,
		FShanmenCombatActionSnapshot& OutAction)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		// The local player entity is also the action owner for this product
		// slice. Persistent profile identity remains outside CombatCore.
		Capture.OwnerId = PlayerEntityId;
		Capture.SourceEntityId = PlayerEntityId;
		Capture.SourceItemInstanceId = SourceItemInstanceId;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P4.5");
		Capture.Content.Digest =
			TEXT("Shanmen.BasicSword.ProductTrajectory.r1");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			RunId,
			PlayerEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		return FShanmenCombatActionSnapshot::TryCapture(Capture, OutAction);
	}
}

bool Fdemo_mapM01EnemyAttackImpactReceipt::IsValid() const
{
	FM01EnemyAttackSpec Spec;
	return TryGetM01EnemyAttackSpec(Family, Spec)
		&& Request.IsValid()
		&& Request.Action.GetActionDefinitionId()
			== Spec.ActionDefinitionId
		&& Request.Action.GetContent().Version == Spec.ContentVersion
		&& Request.Action.GetContent().Digest == Spec.ContentDigest
		&& Request.Action.GetOwnerId()
			== Request.Action.GetSourceEntityId()
		&& Request.Candidate.DetectorId == Spec.DetectorId
		&& Request.Candidate.DetectorKind == Spec.DetectorKind
		&& Request.Candidate.HitOrdinal >= 0
		&& Request.Candidate.HitOrdinal <= Spec.MaxHitOrdinal
		&& Request.Candidate.SourceEntityId
			!= Request.Candidate.TargetEntityId
		&& Request.Damage.FormulaId == Spec.FormulaId
		&& Request.Damage.DamageTags.HasTag(
			FShanmenCombatNativeTags::DamagePhysical())
		&& Request.Defense.TargetTags.HasTag(
			FShanmenCombatNativeTags::TargetLiving())
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool Fdemo_mapPlayerShapeSkillImpactReceipt::IsValid() const
{
	FPlayerShapeSkillSpec Spec;
	return TryGetPlayerShapeSkillSpec(Family, Spec)
		&& Request.IsValid()
		&& Request.Action.GetActionDefinitionId()
			== Spec.ActionDefinitionId
		&& Request.Action.GetContent().Version == Spec.ContentVersion
		&& Request.Action.GetContent().Digest == Spec.ContentDigest
		&& Request.Action.GetOwnerId()
			== Request.Action.GetSourceEntityId()
		&& Request.Action.GetSourceTags().HasTag(
			FShanmenCombatNativeTags::SourcePlayer())
		&& Request.Candidate.DetectorId == Spec.DetectorId
		&& Request.Candidate.DetectorKind
			== EShanmenHitDetectorKind::Shape
		&& Request.Candidate.HitOrdinal == 0
		&& Request.Candidate.SourceEntityId
			!= Request.Candidate.TargetEntityId
		&& Request.Damage.FormulaId == Spec.FormulaId
		&& Request.Damage.DamageTags.HasTag(
			FShanmenCombatNativeTags::DamagePhysical())
		&& Request.Defense.TargetTags.HasTag(
			FShanmenCombatNativeTags::TargetLiving())
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool Fdemo_mapPlayerProjectileImpactReceipt::IsValid() const
{
	const FPlayerStraightProjectileSpec Spec;
	return Spec.IsValid()
		&& Request.IsValid()
		&& Request.Action.GetActionDefinitionId()
			== Spec.ActionDefinitionId
		&& Request.Action.GetContent().Version == Spec.ContentVersion
		&& Request.Action.GetContent().Digest == Spec.ContentDigest
		&& Request.Action.GetOwnerId()
			== Request.Action.GetSourceEntityId()
		&& Request.Action.GetSourceTags().HasTag(
			FShanmenCombatNativeTags::SourcePlayer())
		&& Request.Candidate.DetectorId == Spec.DetectorId
		&& Request.Candidate.DetectorKind
			== EShanmenHitDetectorKind::Projectile
		&& Request.Candidate.HitOrdinal == 0
		&& Request.Candidate.SourceEntityId
			!= Request.Candidate.TargetEntityId
		&& Request.Damage.FormulaId == Spec.FormulaId
		&& Request.Damage.DamageTags.HasTag(
			FShanmenCombatNativeTags::DamagePhysical())
		&& Request.Defense.TargetTags.HasTag(
			FShanmenCombatNativeTags::TargetLiving())
		&& Result.bAccepted
		&& Result.ImpactId == Request.ImpactId
		&& Result.IsConserved();
}

bool Fdemo_mapPlayerThrownWeaponActionReservation::IsValid() const
{
	return ActivationSequence > 0
		&& ActivationId.IsValid()
		&& RunId.IsValid()
		&& SourceEntityId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& ActivationId == FShanmenCombatIdFactory::MakeActivationId(
			RunId,
			SourceEntityId,
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId(),
			ActivationSequence);
}

FName Fdemo_mapCombatRunCoordinator::PlayerSpawnSourceId()
{
	return TEXT("Spawn.Player.Primary");
}

FGuid Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
	const FGuid& RunId,
	const Fdemo_mapM01EnemyDefinition& Definition)
{
	if (!Definition.IsValid())
	{
		return FGuid();
	}
	return FShanmenWorldEntityIdFactory::MakeEntityId(
		RunId,
		Definition.SpawnMarkerId,
		0);
}

bool Fdemo_mapCombatRunCoordinator::TryBeginRun(
	const FGuid& RunId,
	APawn* PlayerPawn,
	Udemo_mapPlayerHealthComponent* PlayerHealth,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!RunId.IsValid() || !PlayerPawn || !PlayerHealth
		|| PlayerHealth->GetOwner() != PlayerPawn)
	{
		OutDiagnostic =
			TEXT("Combat Run binding requires one valid Run, Pawn, and Pawn-owned health component.");
		return false;
	}

	const FGuid ExpectedEntityId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			RunId,
			PlayerSpawnSourceId(),
			0);
	if (!ExpectedEntityId.IsValid())
	{
		OutDiagnostic = TEXT("Player EntityId derivation failed closed.");
		return false;
	}
	if (PlayerHealth->IsCombatEntityBound()
		&& PlayerHealth->GetCombatEntityId() != ExpectedEntityId)
	{
		OutDiagnostic =
			TEXT("Player health is still bound to a different Run identity.");
		return false;
	}

	if (IsActive())
	{
		if (GetRunId() != RunId
			|| PlayerEntityId != ExpectedEntityId
			|| BoundPlayerPawn.Get() != PlayerPawn
			|| BoundPlayerHealth.Get() != PlayerHealth)
		{
			OutDiagnostic =
				TEXT("A live combat Run coordinator cannot switch Run or player host.");
			return false;
		}
		if (!IsReady())
		{
			OutDiagnostic =
				TEXT("The existing combat Run binding is no longer internally consistent.");
			return false;
		}
		OutDiagnostic = TEXT("Combat Run binding already active.");
		return true;
	}

	FShanmenWorldEntityRegistry PreparedRegistry;
	if (!PreparedRegistry.TryBeginRun(RunId)
		|| !IsSuccessfulBinding(PreparedRegistry.BindObject(
			RunId,
			PlayerPawn,
			ExpectedEntityId))
		|| !IsSuccessfulBinding(PreparedRegistry.BindObject(
			RunId,
			PlayerHealth,
			ExpectedEntityId)))
	{
		OutDiagnostic = TEXT("World Entity Registry rejected the player aliases.");
		return false;
	}

	UPrimitiveComponent* PlayerRoot =
		Cast<UPrimitiveComponent>(PlayerPawn->GetRootComponent());
	if (PlayerRoot
		&& !IsSuccessfulBinding(PreparedRegistry.BindObject(
			RunId,
			PlayerRoot,
			ExpectedEntityId)))
	{
		OutDiagnostic =
			TEXT("World Entity Registry rejected the player collision-root alias.");
		return false;
	}
	if (!PlayerHealth->TryBindCombatEntity(ExpectedEntityId))
	{
		OutDiagnostic =
			TEXT("Player vitality host rejected the stable World EntityId.");
		return false;
	}
	bool bPreparedPlayerRequiresResourceDefenseAuthority = false;
	if (UGameInstance* GameInstance = PlayerPawn->GetGameInstance())
	{
		if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
			GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
			Authority
			&& Authority->GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
		{
			const Fdemo_mapShanmenDefenseResourceCoordinationResult Recovery =
				Fdemo_mapShanmenDefenseResourceAdapter::RecoverPendingIntent(
					*Authority, *PlayerHealth);
			if (!Recovery.IsSuccess())
			{
				PlayerHealth->TryEndCombatEntityBinding(ExpectedEntityId);
				OutDiagnostic = Recovery.Diagnostic.IsEmpty()
					? TEXT("Combat Run binding found an ambiguous pending defense resource intent.")
					: Recovery.Diagnostic;
				return false;
			}
			const Fdemo_mapShanmenDefenseOrphanRecoveryResult OrphanRecovery =
				Fdemo_mapShanmenDefenseResourceAdapter::
					RecoverOrphanedDefenseReservations(*Authority);
			if (!OrphanRecovery.bSuccess)
			{
				PlayerHealth->TryEndCombatEntityBinding(ExpectedEntityId);
				OutDiagnostic = OrphanRecovery.Diagnostic.IsEmpty()
					? TEXT("Combat Run binding could not recover a pre-intent defense reservation.")
					: OrphanRecovery.Diagnostic;
				return false;
			}
			Fdemo_mapShanmenRunCorrelation Correlation;
			FShanmenItemAuthoritySnapshot Snapshot;
			FString CorrelationDiagnostic;
			if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
					*Authority, Correlation, &CorrelationDiagnostic)
				|| !Authority->TryCaptureSnapshot(Snapshot))
			{
				PlayerHealth->TryEndCombatEntityBinding(ExpectedEntityId);
				OutDiagnostic = CorrelationDiagnostic.IsEmpty()
					? TEXT("Combat Run binding could not inspect prepared armor authority.")
					: CorrelationDiagnostic;
				return false;
			}
			if (Correlation.ArmorItemInstanceId.IsValid())
			{
				const FShanmenItemInstance* Armor =
					Snapshot.Items.FindByPredicate(
						[&Correlation](const FShanmenItemInstance& Item)
						{
							return Item.ItemInstanceId
								== Correlation.ArmorItemInstanceId;
						});
				if (!Armor)
				{
					PlayerHealth->TryEndCombatEntityBinding(ExpectedEntityId);
					OutDiagnostic =
						TEXT("Combat Run prepared armor identity is absent from item authority.");
					return false;
				}
				bPreparedPlayerRequiresResourceDefenseAuthority =
					Armor->DefinitionId == Fdemo_mapItemIds::SpiritGuardRobe;
			}
		}
	}

	EntityRegistry = MoveTemp(PreparedRegistry);
	PlayerEntityId = ExpectedEntityId;
	BoundPlayerPawn = PlayerPawn;
	BoundPlayerHealth = PlayerHealth;
	BoundPlayerRoot = PlayerRoot;
	bPlayerRequiresResourceDefenseAuthority =
		bPreparedPlayerRequiresResourceDefenseAuthority;
	OutDiagnostic = FString::Printf(
		TEXT("Combat Run bound: RunId=%s PlayerEntityId=%s."),
		*RunId.ToString(EGuidFormats::DigitsWithHyphens),
		*PlayerEntityId.ToString(EGuidFormats::DigitsWithHyphens));
	return IsReady();
}

bool Fdemo_mapCombatRunCoordinator::TryRegisterM01Enemy(
	AActor* EnemyActor,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsReady() || !EnemyActor)
	{
		OutDiagnostic =
			TEXT("M01 enemy registration requires an active combat Run and actor.");
		return false;
	}

	const Udemo_mapM01EnemyIdentityComponent* Identity =
		EnemyActor->FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();
	if (!Identity || !Identity->IsConfigured())
	{
		OutDiagnostic =
			TEXT("M01 enemy registration requires one configured authored identity component.");
		return false;
	}
	const Fdemo_mapM01EnemyDefinition& Definition = Identity->GetDefinition();
	if (!IsExpectedM01ProductActor(Definition, EnemyActor))
	{
		OutDiagnostic =
			TEXT("M01 authored archetype does not match the spawned product actor class.");
		return false;
	}
	const FGuid EntityId = MakeM01EnemyEntityId(GetRunId(), Definition);
	if (!EntityId.IsValid())
	{
		OutDiagnostic = TEXT("M01 authored EntityId derivation failed closed.");
		return false;
	}

	if (const FM01EnemyBinding* Existing = M01EnemyBindings.Find(EntityId))
	{
		Idemo_mapCombatVitalityHost* ExpectedVitalityHost =
			ResolveM01VitalityHost(EnemyActor);
		if (Existing->Actor.Get() != EnemyActor
			|| Existing->SpawnMarkerId != Definition.SpawnMarkerId
			|| Existing->SkillProfileId != Definition.SkillProfileId
			|| !ExpectedVitalityHost)
		{
			OutDiagnostic =
				TEXT("M01 authored EntityId is already owned by a different product actor.");
			return false;
		}
		FGuid ResolvedEntityId;
		if (!EntityRegistry.TryResolveObject(
			GetRunId(), EnemyActor, INDEX_NONE, ResolvedEntityId)
			|| ResolvedEntityId != EntityId
			|| (ExpectedVitalityHost
				&& (!ExpectedVitalityHost->IsCombatEntityBound()
					|| ExpectedVitalityHost->GetCombatEntityId() != EntityId)))
		{
			OutDiagnostic =
				TEXT("Existing M01 enemy binding is no longer internally consistent.");
			return false;
		}
		OutDiagnostic = TEXT("M01 enemy identity already registered.");
		return true;
	}

	FShanmenWorldEntityRegistry PreparedRegistry = EntityRegistry;
	if (!IsSuccessfulBinding(PreparedRegistry.BindObject(
		GetRunId(), EnemyActor, EntityId)))
	{
		OutDiagnostic =
			TEXT("World Entity Registry rejected the M01 enemy actor alias.");
		return false;
	}
	UPrimitiveComponent* CollisionRoot =
		Cast<UPrimitiveComponent>(EnemyActor->GetRootComponent());
	if (CollisionRoot
		&& !IsSuccessfulBinding(PreparedRegistry.BindObject(
			GetRunId(), CollisionRoot, EntityId)))
	{
		OutDiagnostic =
			TEXT("World Entity Registry rejected the M01 collision-root alias.");
		return false;
	}

	Idemo_mapCombatVitalityHost* VitalityHost =
		ResolveM01VitalityHost(EnemyActor);
	if (!VitalityHost || !VitalityHost->TryBindCombatEntity(EntityId))
	{
		OutDiagnostic =
			TEXT("M01 vitality host rejected its authored World EntityId.");
		return false;
	}
	if (Ademo_mapHeavyEnemyCharacter* HeavyEnemy =
		Cast<Ademo_mapHeavyEnemyCharacter>(EnemyActor))
	{
		HeavyEnemy->ResetHeavyAttackForNewRun();
	}
	if (Ademo_mapM01BossCharacter* BossEnemy =
		Cast<Ademo_mapM01BossCharacter>(EnemyActor))
	{
		BossEnemy->ResetBossAttackForNewRun();
	}

	EntityRegistry = MoveTemp(PreparedRegistry);
	FM01EnemyBinding& Added = M01EnemyBindings.Add(EntityId);
	Added.SpawnMarkerId = Definition.SpawnMarkerId;
	Added.SkillProfileId = Definition.SkillProfileId;
	Added.Actor = EnemyActor;
	Added.CollisionRoot = CollisionRoot;
	OutDiagnostic = FString::Printf(
		TEXT("M01 enemy registered: SpawnMarkerId=%s EntityId=%s Vitality=%d."),
		*Definition.SpawnMarkerId.ToString(),
		*EntityId.ToString(EGuidFormats::DigitsWithHyphens),
		1);
	return true;
}

bool Fdemo_mapCombatRunCoordinator::TryEndRun(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ExpectedRunId.IsValid() || !IsActive()
		|| ExpectedRunId != GetRunId())
	{
		OutDiagnostic =
			TEXT("Combat Run end rejected a missing or mismatched RunId.");
		return false;
	}

	if (BoundPlayerHealth.IsValid()
		&& BoundPlayerHealth->IsCombatEntityBound()
		&& BoundPlayerHealth->GetCombatEntityId() != PlayerEntityId)
	{
		OutDiagnostic =
			TEXT("Player vitality host no longer owns the expected Run identity.");
		return false;
	}
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get());
		if (VitalityHost
			&& VitalityHost->IsCombatEntityBound()
			&& VitalityHost->GetCombatEntityId() != Pair.Key)
		{
			OutDiagnostic =
				TEXT("An M01 vitality host no longer owns its authored Run identity.");
			return false;
		}
	}

	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get());
		if (VitalityHost
			&& !VitalityHost->TryEndCombatEntityBinding(Pair.Key))
		{
			OutDiagnostic =
				TEXT("An M01 vitality host rejected the exact Run identity release.");
			return false;
		}
	}
	if (BoundPlayerHealth.IsValid()
		&& !BoundPlayerHealth->TryEndCombatEntityBinding(PlayerEntityId))
	{
		OutDiagnostic =
			TEXT("Player vitality host rejected the exact Run identity release.");
		return false;
	}
	if (!EntityRegistry.TryEndRun(ExpectedRunId))
	{
		OutDiagnostic = TEXT("World Entity Registry rejected the exact Run end.");
		return false;
	}

	M01EnemyBindings.Reset();
	NextM01EnemyBasicMeleeActivationSequences.Reset();
	PlayerEntityId.Invalidate();
	BoundPlayerPawn.Reset();
	BoundPlayerHealth.Reset();
	BoundPlayerRoot.Reset();
	bPlayerRequiresResourceDefenseAuthority = false;
	NextPlayerBasicSwordActivationSequence = 1;
	NextPlayerGroundCircleActivationSequence = 1;
	NextPlayerSelfSectorActivationSequence = 1;
	NextPlayerStraightProjectileActivationSequence = 1;
	NextPlayerThrownWeaponActivationSequence = 1;
	OutDiagnostic = TEXT("Combat Run identities released.");
	return true;
}

void Fdemo_mapCombatRunCoordinator::Reset()
{
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		if (Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get()))
		{
			VitalityHost->TryEndCombatEntityBinding(Pair.Key);
		}
	}
	if (BoundPlayerHealth.IsValid() && PlayerEntityId.IsValid())
	{
		BoundPlayerHealth->TryEndCombatEntityBinding(PlayerEntityId);
	}
	EntityRegistry.Reset();
	M01EnemyBindings.Reset();
	NextM01EnemyBasicMeleeActivationSequences.Reset();
	PlayerEntityId.Invalidate();
	BoundPlayerPawn.Reset();
	BoundPlayerHealth.Reset();
	BoundPlayerRoot.Reset();
	bPlayerRequiresResourceDefenseAuthority = false;
	NextPlayerBasicSwordActivationSequence = 1;
	NextPlayerGroundCircleActivationSequence = 1;
	NextPlayerSelfSectorActivationSequence = 1;
	NextPlayerStraightProjectileActivationSequence = 1;
	NextPlayerThrownWeaponActivationSequence = 1;
}

bool Fdemo_mapCombatRunCoordinator::IsReady() const
{
	if (!IsActive() || !PlayerEntityId.IsValid()
		|| !BoundPlayerPawn.IsValid() || !BoundPlayerHealth.IsValid()
		|| !BoundPlayerHealth->IsCombatEntityBound()
		|| BoundPlayerHealth->GetCombatEntityId() != PlayerEntityId)
	{
		return false;
	}

	FGuid ResolvedEntityId;
	if (!EntityRegistry.TryResolveObject(
		GetRunId(), BoundPlayerPawn.Get(), INDEX_NONE, ResolvedEntityId)
		|| ResolvedEntityId != PlayerEntityId
		|| !EntityRegistry.TryResolveObject(
			GetRunId(), BoundPlayerHealth.Get(), INDEX_NONE, ResolvedEntityId)
		|| ResolvedEntityId != PlayerEntityId)
	{
		return false;
	}

	return !BoundPlayerRoot.IsValid()
		|| (EntityRegistry.TryResolveObject(
			GetRunId(), BoundPlayerRoot.Get(), INDEX_NONE, ResolvedEntityId)
			&& ResolvedEntityId == PlayerEntityId);
}

int32 Fdemo_mapCombatRunCoordinator::NumVitalityBoundM01Enemies() const
{
	int32 Count = 0;
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get());
		if (VitalityHost
			&& VitalityHost->IsCombatEntityBound()
			&& VitalityHost->GetCombatEntityId() == Pair.Key)
		{
			++Count;
		}
	}
	return Count;
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverBasicSwordImpactToPlayer(
	const FShanmenBasicSwordImpactReceipt& Impact)
{
	Fdemo_mapCombatImpactDeliveryResult Delivery;
	if (!IsReady())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
		return Delivery;
	}
	if (!Impact.IsValid())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::InvalidImpactReceipt;
		return Delivery;
	}
	if (Impact.GetRequest().Action.GetRunId() != GetRunId())
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::RunMismatch;
		return Delivery;
	}
	if (Impact.GetRequest().Candidate.TargetEntityId != PlayerEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::TargetMismatch;
		return Delivery;
	}

	FShanmenVitalityCommitCommand Command;
	if (!FShanmenVitalityCommitCommand::TryCreate(
		Impact.GetRequest(), Impact.GetResult(), Command))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CommandConstructionFailed;
		return Delivery;
	}

	Delivery.CommitResult = BoundPlayerHealth->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverBasicSwordImpactToM01Enemy(
	const FShanmenBasicSwordImpactReceipt& Impact,
	AActor* TargetEnemy)
{
	Fdemo_mapCombatImpactDeliveryResult Delivery;
	if (!IsReady())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
		return Delivery;
	}
	if (!Impact.IsValid())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::InvalidImpactReceipt;
		return Delivery;
	}
	if (Impact.GetRequest().Action.GetRunId() != GetRunId())
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::RunMismatch;
		return Delivery;
	}
	if (Impact.GetRequest().Action.GetSourceEntityId() != PlayerEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::SourceMismatch;
		return Delivery;
	}
	if (!TargetEnemy)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}

	FGuid TargetEntityId;
	if (!EntityRegistry.TryResolveObject(
		GetRunId(), TargetEnemy, INDEX_NONE, TargetEntityId))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}
	if (Impact.GetRequest().Candidate.TargetEntityId != TargetEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::TargetMismatch;
		return Delivery;
	}
	const FM01EnemyBinding* Binding = M01EnemyBindings.Find(TargetEntityId);
	if (!Binding || Binding->Actor.Get() != TargetEnemy)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}
	Idemo_mapCombatVitalityHost* VitalityHost =
		ResolveM01VitalityHost(TargetEnemy);
	if (!VitalityHost
		|| !VitalityHost->IsCombatEntityBound()
		|| VitalityHost->GetCombatEntityId() != TargetEntityId)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotVitalityBound;
		return Delivery;
	}

	FShanmenVitalityCommitCommand Command;
	if (!FShanmenVitalityCommitCommand::TryCreate(
		Impact.GetRequest(), Impact.GetResult(), Command))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CommandConstructionFailed;
		return Delivery;
	}

	Delivery.CommitResult = VitalityHost->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverPlayerShapeSkillImpactToM01Enemy(
	const Fdemo_mapPlayerShapeSkillImpactReceipt& Impact,
	AActor* TargetEnemy)
{
	return DeliverResolvedPlayerImpactToM01Enemy(
		Impact.IsValid(),
		Impact.GetRequest(),
		Impact.GetResult(),
		TargetEnemy);
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverPlayerProjectileImpactToM01Enemy(
	const Fdemo_mapPlayerProjectileImpactReceipt& Impact,
	AActor* TargetEnemy)
{
	return DeliverResolvedPlayerImpactToM01Enemy(
		Impact.IsValid(),
		Impact.GetRequest(),
		Impact.GetResult(),
		TargetEnemy);
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverControlledWeaponImpactToM01Enemy(
	const FShanmenControlledWeaponImpactReceipt& Impact,
	AActor* TargetEnemy)
{
	return DeliverResolvedPlayerImpactToM01Enemy(
		Impact.IsValid(),
		Impact.GetRequest(),
		Impact.GetResult(),
		TargetEnemy);
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverThrownWeaponImpactToM01Enemy(
	const FShanmenThrownWeaponImpactReceipt& Impact,
	AActor* TargetEnemy)
{
	return DeliverResolvedPlayerImpactToM01Enemy(
		Impact.IsValid(),
		Impact.GetRequest(),
		Impact.GetResult(),
		TargetEnemy);
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverResolvedPlayerImpactToM01Enemy(
	bool bImpactValid,
	const FShanmenImpactRequest& Request,
	const FShanmenImpactResult& Result,
	AActor* TargetEnemy)
{
	Fdemo_mapCombatImpactDeliveryResult Delivery;
	if (!IsReady())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
		return Delivery;
	}
	if (!bImpactValid)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::InvalidImpactReceipt;
		return Delivery;
	}
	if (Request.Action.GetRunId() != GetRunId())
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::RunMismatch;
		return Delivery;
	}
	if (Request.Action.GetSourceEntityId() != PlayerEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::SourceMismatch;
		return Delivery;
	}
	if (!TargetEnemy)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}

	FGuid TargetEntityId;
	if (!EntityRegistry.TryResolveObject(
		GetRunId(), TargetEnemy, INDEX_NONE, TargetEntityId))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}
	if (Request.Candidate.TargetEntityId != TargetEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::TargetMismatch;
		return Delivery;
	}
	const FM01EnemyBinding* Binding = M01EnemyBindings.Find(TargetEntityId);
	Idemo_mapCombatVitalityHost* VitalityHost =
		ResolveM01VitalityHost(TargetEnemy);
	if (!Binding || Binding->Actor.Get() != TargetEnemy
		|| !VitalityHost
		|| !VitalityHost->IsCombatEntityBound()
		|| VitalityHost->GetCombatEntityId() != TargetEntityId)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotVitalityBound;
		return Delivery;
	}

	FShanmenVitalityCommitCommand Command;
	if (!FShanmenVitalityCommitCommand::TryCreate(
		Request, Result, Command))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CommandConstructionFailed;
		return Delivery;
	}

	Delivery.CommitResult = VitalityHost->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverM01EnemyAttackImpactToPlayer(
	const Fdemo_mapM01EnemyAttackImpactReceipt& Impact,
	AActor* SourceEnemy)
{
	Fdemo_mapCombatImpactDeliveryResult Delivery;
	if (!IsReady())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
		return Delivery;
	}
	if (!Impact.IsValid())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::InvalidImpactReceipt;
		return Delivery;
	}
	const FShanmenImpactRequest& Request = Impact.GetRequest();
	if (Request.Action.GetRunId() != GetRunId())
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::RunMismatch;
		return Delivery;
	}
	if (!SourceEnemy)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::SourceNotRegistered;
		return Delivery;
	}

	FGuid SourceEntityId;
	if (!EntityRegistry.TryResolveObject(
		GetRunId(), SourceEnemy, INDEX_NONE, SourceEntityId))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::SourceNotRegistered;
		return Delivery;
	}
	const FM01EnemyBinding* Binding = M01EnemyBindings.Find(SourceEntityId);
	Idemo_mapCombatVitalityHost* SourceVitalityHost =
		ResolveM01VitalityHost(SourceEnemy);
	if (!Binding || Binding->Actor.Get() != SourceEnemy
		|| !SourceVitalityHost
		|| !SourceVitalityHost->IsCombatEntityBound()
		|| SourceVitalityHost->GetCombatEntityId() != SourceEntityId)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::SourceNotRegistered;
		return Delivery;
	}
	if (Request.Action.GetSourceEntityId() != SourceEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::SourceMismatch;
		return Delivery;
	}
	if (Request.Candidate.TargetEntityId != PlayerEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::TargetMismatch;
		return Delivery;
	}

	const FShanmenImpactResult& ImpactResult = Impact.GetResult();
	const bool bRequiresResourceCoordination =
		Request.Defense.Layers.ContainsByPredicate(
			[](const FShanmenDefenseLayer& Layer)
			{
				return Layer.bRequiresCommitOnTrigger;
			});
	if (bRequiresResourceCoordination)
	{
		UGameInstance* GameInstance = BoundPlayerPawn.IsValid()
			? BoundPlayerPawn->GetGameInstance() : nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = GameInstance
			? GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>()
			: nullptr;
		if (!Authority)
		{
			Delivery.Error =
				Edemo_mapCombatImpactDeliveryError::ResourceCoordinationRejected;
			return Delivery;
		}
		const Fdemo_mapShanmenDefenseResourceCoordinationResult Coordination =
			Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
				*Authority,
				*BoundPlayerHealth,
				Request,
				ImpactResult);
		Delivery.CommitResult = Coordination.VitalityCommand;
		Delivery.Error = Coordination.IsSuccess()
			? Edemo_mapCombatImpactDeliveryError::None
			: Edemo_mapCombatImpactDeliveryError::ResourceCoordinationRejected;
		return Delivery;
	}

	FShanmenVitalityCommitCommand Command;
	if (!FShanmenVitalityCommitCommand::TryCreate(
		Request,
		ImpactResult,
		Command))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CommandConstructionFailed;
		return Delivery;
	}
	Delivery.CommitResult = BoundPlayerHealth->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01EnemyBasicMeleeStrike(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	float RawDamage)
{
	return ExecuteM01EnemyAttack(
		SourceEnemy,
		TargetPlayer,
		RawDamage,
		Edemo_mapM01EnemyAttackFamily::BasicMelee,
		0,
		0,
		FVector::ZeroVector,
		FVector::ZeroVector);
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01EnemyMeleeDashContact(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	FName SkillProfileId,
	uint32 ActivationSerial,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	Edemo_mapM01EnemyAttackFamily Family =
		Edemo_mapM01EnemyAttackFamily::None;
	if (!TryResolveM01MeleeDashFamily(SkillProfileId, Family))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile;
		return ProductResult;
	}
	if (ActivationSerial == 0)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence;
		return ProductResult;
	}
	return ExecuteM01EnemyAttack(
		SourceEnemy,
		TargetPlayer,
		RawDamage,
		Family,
		static_cast<uint64>(ActivationSerial),
		0,
		FVector::ZeroVector,
		FVector::ZeroVector);
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01EnemyRangedProjectileImpact(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	FName SkillProfileId,
	uint64 ProjectileSequence,
	float RawDamage,
	const FVector& ImpactLocation,
	const FVector& ImpactNormal)
{
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	Edemo_mapM01EnemyAttackFamily Family =
		Edemo_mapM01EnemyAttackFamily::None;
	if (!TryResolveM01RangedProjectileFamily(SkillProfileId, Family))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile;
		return ProductResult;
	}
	if (ProjectileSequence == 0)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence;
		return ProductResult;
	}
	if (ImpactLocation.ContainsNaN() || ImpactNormal.ContainsNaN())
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidContact;
		return ProductResult;
	}
	return ExecuteM01EnemyAttack(
		SourceEnemy,
		TargetPlayer,
		RawDamage,
		Family,
		ProjectileSequence,
		0,
		ImpactLocation,
		ImpactNormal);
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01EnemyHeavySectorAttack(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	uint64 AttackSequence,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	if (AttackSequence == 0)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence;
		return ProductResult;
	}
	if (AttackSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::SequenceExhausted;
		return ProductResult;
	}
	return ExecuteM01EnemyAttack(
		SourceEnemy,
		TargetPlayer,
		RawDamage,
		Edemo_mapM01EnemyAttackFamily::HeavySector,
		AttackSequence,
		0,
		FVector::ZeroVector,
		FVector::ZeroVector);
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01BossShapeAttack(
	AActor* SourceBoss,
	APawn* TargetPlayer,
	Edemo_mapM01BossAttack Attack,
	uint64 AttackSequence,
	float RawDamage)
{
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	if (AttackSequence == 0)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence;
		return ProductResult;
	}
	if (AttackSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::SequenceExhausted;
		return ProductResult;
	}

	Edemo_mapM01EnemyAttackFamily Family =
		Edemo_mapM01EnemyAttackFamily::None;
	if (Attack == Edemo_mapM01BossAttack::Sweep)
	{
		Family = Edemo_mapM01EnemyAttackFamily::BossSweep;
	}
	else if (Attack == Edemo_mapM01BossAttack::Charge)
	{
		Family = Edemo_mapM01EnemyAttackFamily::BossCharge;
	}
	else
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile;
		return ProductResult;
	}

	return ExecuteM01EnemyAttack(
		SourceBoss,
		TargetPlayer,
		RawDamage,
		Family,
		AttackSequence,
		0,
		FVector::ZeroVector,
		FVector::ZeroVector);
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01BossVolleyProjectileImpact(
	AActor* SourceBoss,
	APawn* TargetPlayer,
	uint64 AttackSequence,
	int32 ProjectileOrdinal,
	float RawDamage,
	const FVector& ImpactLocation,
	const FVector& ImpactNormal)
{
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	if (AttackSequence == 0)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence;
		return ProductResult;
	}
	if (AttackSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::SequenceExhausted;
		return ProductResult;
	}
	if (ProjectileOrdinal < 0 || ProjectileOrdinal > 2)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidHitOrdinal;
		return ProductResult;
	}
	if (ImpactLocation.ContainsNaN() || ImpactNormal.ContainsNaN())
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidContact;
		return ProductResult;
	}

	return ExecuteM01EnemyAttack(
		SourceBoss,
		TargetPlayer,
		RawDamage,
		Edemo_mapM01EnemyAttackFamily::BossVolleyProjectile,
		AttackSequence,
		ProjectileOrdinal,
		ImpactLocation,
		ImpactNormal);
}

Fdemo_mapM01EnemyAttackExecutionResult
Fdemo_mapCombatRunCoordinator::ExecuteM01EnemyAttack(
	AActor* SourceEnemy,
	APawn* TargetPlayer,
	float RawDamage,
	Edemo_mapM01EnemyAttackFamily Family,
	uint64 RequestedActivationSequence,
	int32 RequestedHitOrdinal,
	const FVector& RequestedHitLocation,
	const FVector& RequestedHitNormal)
{
	Fdemo_mapM01EnemyAttackExecutionResult ProductResult;
	FM01EnemyAttackSpec Spec;
	if (!TryGetM01EnemyAttackSpec(Family, Spec))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile;
		return ProductResult;
	}
	if (RequestedHitOrdinal < 0
		|| RequestedHitOrdinal > Spec.MaxHitOrdinal)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidHitOrdinal;
		return ProductResult;
	}
	if (!IsReady())
	{
		return ProductResult;
	}
	if (TargetPlayer != BoundPlayerPawn.Get())
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::TargetMismatch;
		return ProductResult;
	}
	if (!FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidDamage;
		return ProductResult;
	}
	if (BoundPlayerHealth->IsDefeated())
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::TargetMismatch;
		return ProductResult;
	}

	FGuid SourceEntityId;
	if (!SourceEnemy
		|| !EntityRegistry.TryResolveObject(
			GetRunId(), SourceEnemy, INDEX_NONE, SourceEntityId))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::SourceNotRegistered;
		return ProductResult;
	}
	const FM01EnemyBinding* Binding = M01EnemyBindings.Find(SourceEntityId);
	Idemo_mapCombatVitalityHost* SourceVitalityHost =
		ResolveM01VitalityHost(SourceEnemy);
	FShanmenTargetVitalitySnapshot SourceVitality;
	if (!Binding || Binding->Actor.Get() != SourceEnemy
		|| !SourceVitalityHost
		|| !SourceVitalityHost->IsCombatEntityBound()
		|| SourceVitalityHost->GetCombatEntityId() != SourceEntityId
		|| !SourceVitalityHost->TryCaptureCombatVitalitySnapshot(
			SourceVitality)
		|| SourceVitality.CurrentVitality <= 0.0f)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::SourceNotRegistered;
		return ProductResult;
	}
	Edemo_mapM01EnemyAttackFamily BoundProfileFamily =
		Edemo_mapM01EnemyAttackFamily::None;
	const bool bMeleeProfile = TryResolveM01MeleeDashFamily(
		Binding->SkillProfileId,
		BoundProfileFamily);
	const bool bRangedProfile = !bMeleeProfile
		&& TryResolveM01RangedProjectileFamily(
			Binding->SkillProfileId,
			BoundProfileFamily);
	const bool bHeavySector =
		Family == Edemo_mapM01EnemyAttackFamily::HeavySector
		&& Binding->SkillProfileId.IsNone()
		&& SourceEnemy->IsA<Ademo_mapHeavyEnemyCharacter>();
	const bool bBossAttack =
		(Family == Edemo_mapM01EnemyAttackFamily::BossSweep
			|| Family == Edemo_mapM01EnemyAttackFamily::BossCharge
			|| Family
				== Edemo_mapM01EnemyAttackFamily::BossVolleyProjectile)
		&& Binding->SkillProfileId.IsNone()
		&& SourceEnemy->IsA<Ademo_mapM01BossCharacter>();
	const bool bFamilyMatchesBinding =
		(Family == Edemo_mapM01EnemyAttackFamily::BasicMelee
			&& bMeleeProfile)
		|| (Family != Edemo_mapM01EnemyAttackFamily::BasicMelee
			&& (bMeleeProfile || bRangedProfile)
			&& Family == BoundProfileFamily)
		|| bHeavySector
		|| bBossAttack;
	if (!bFamilyMatchesBinding)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidSkillProfile;
		return ProductResult;
	}

	uint64 ActivationSequence = RequestedActivationSequence;
	const bool bOwnsBasicMeleeSequence =
		Family == Edemo_mapM01EnemyAttackFamily::BasicMelee;
	if (bOwnsBasicMeleeSequence)
	{
		ActivationSequence = 1;
		if (const uint64* Existing =
			NextM01EnemyBasicMeleeActivationSequences.Find(SourceEntityId))
		{
			ActivationSequence = *Existing;
		}
	}
	if (ActivationSequence == 0)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::InvalidActivationSequence;
		return ProductResult;
	}
	if (bOwnsBasicMeleeSequence && ActivationSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::SequenceExhausted;
		return ProductResult;
	}

	FShanmenCombatActionSnapshot Action;
	if (!TryBuildM01EnemyAttackAction(
		Spec,
		GetRunId(),
		SourceEntityId,
		ActivationSequence,
		Action))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::ActionConstructionFailed;
		return ProductResult;
	}
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt Transition;
	if (!FShanmenActionOrchestrator::TryStart(
		Action,
		ActionRuntime,
		Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::RuntimeStartFailed;
		return ProductResult;
	}
	ProductResult.ActivationId = Action.GetActivationId();
	if (bOwnsBasicMeleeSequence)
	{
		NextM01EnemyBasicMeleeActivationSequences.Add(
			SourceEntityId,
			ActivationSequence + 1);
	}

	FShanmenHitCandidate Candidate;
	Candidate.ActivationId = Action.GetActivationId();
	Candidate.SourceEntityId = SourceEntityId;
	Candidate.TargetEntityId = PlayerEntityId;
	Candidate.DetectorId = Spec.DetectorId;
	Candidate.DetectorKind = Spec.DetectorKind;
	if (Spec.DetectorKind == EShanmenHitDetectorKind::Projectile)
	{
		Candidate.HitLocation = RequestedHitLocation;
		Candidate.HitNormal = RequestedHitNormal;
	}
	else
	{
		Candidate.HitLocation = TargetPlayer->GetActorLocation();
		FVector SourceToTarget = TargetPlayer->GetActorLocation()
			- SourceEnemy->GetActorLocation();
		Candidate.HitNormal = SourceToTarget.Normalize()
			? -SourceToTarget
			: FVector::UpVector;
	}
	Candidate.HitOrdinal = RequestedHitOrdinal;

	const FGuid ImpactId = FShanmenCombatIdFactory::MakeImpactId(
		GetRunId(),
		Action.GetActivationId(),
		Candidate.DetectorId,
		PlayerEntityId,
		Candidate.HitOrdinal);
	FShanmenDefenseSnapshot Defense;
	if (!BoundPlayerHealth->TryCaptureCombatDefenseSnapshot(
		ImpactId,
		Defense))
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::DefenseSnapshotFailed;
		return ProductResult;
	}
	Fdemo_mapShanmenDefenseResourcePreparationResult ResourcePreparation;
	bool bResourceAuthorityInspected = false;
	if (UGameInstance* GameInstance = TargetPlayer->GetGameInstance())
	{
		if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
			GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
			Authority
			&& Authority->GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
		{
			bResourceAuthorityInspected = true;
			ResourcePreparation =
				Fdemo_mapShanmenDefenseResourceAdapter::PrepareImpactDefense(
					*Authority, *BoundPlayerHealth, ImpactId, Defense);
			if (!ResourcePreparation.IsSuccess())
			{
				ActionRuntime.TryInterrupt(
					EShanmenCombatActionPhase::Active,
					Transition);
				ProductResult.Error =
					Edemo_mapM01EnemyAttackExecutionError::
						ResourceDefensePreparationFailed;
				return ProductResult;
			}
			if (bPlayerRequiresResourceDefenseAuthority
				&& ResourcePreparation.Status
					== Edemo_mapShanmenDefenseResourcePreparationStatus::
						NotApplicable)
			{
				ActionRuntime.TryInterrupt(
					EShanmenCombatActionPhase::Active,
					Transition);
				ProductResult.Error =
					Edemo_mapM01EnemyAttackExecutionError::
						ResourceDefensePreparationFailed;
				return ProductResult;
			}
		}
	}
	if (bPlayerRequiresResourceDefenseAuthority
		&& !bResourceAuthorityInspected)
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::
				ResourceDefensePreparationFailed;
		return ProductResult;
	}

	auto CancelPreDeliveryResource = [&ResourcePreparation, TargetPlayer]()
	{
		if (!ResourcePreparation.HasResourceLayer())
		{
			return true;
		}
		UGameInstance* GameInstance = TargetPlayer->GetGameInstance();
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = GameInstance
			? GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>()
			: nullptr;
		FString Diagnostic;
		return Authority
			&& Fdemo_mapShanmenDefenseResourceAdapter::
				CancelPreparedDefenseReservation(
					*Authority, ResourcePreparation, Diagnostic);
	};

	// Resource recovery may have committed an earlier vitality intent. Sample
	// the target only after preparation so this request cannot carry stale CAS.
	FShanmenTargetVitalitySnapshot TargetVitality;
	if (!BoundPlayerHealth->TryCaptureCombatVitalitySnapshot(TargetVitality))
	{
		const bool bCancelled = CancelPreDeliveryResource();
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = bCancelled
			? Edemo_mapM01EnemyAttackExecutionError::VitalitySnapshotFailed
			: Edemo_mapM01EnemyAttackExecutionError::
				ResourceDefensePreparationFailed;
		return ProductResult;
	}

	FShanmenImpactRequest Request;
	Request.ImpactId = ImpactId;
	Request.Action = Action;
	Request.Candidate = Candidate;
	Request.Damage.FormulaId = Spec.FormulaId;
	Request.Damage.RawDamage = RawDamage;
	Request.Damage.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysical());
	Request.TargetVitality = TargetVitality;
	Request.Defense = Defense;
	const FShanmenImpactResult Resolution =
		FShanmenDefenseResolver::Resolve(Request);
	ProductResult.Impact.Family = Family;
	ProductResult.Impact.Request = MoveTemp(Request);
	ProductResult.Impact.Result = Resolution;
	if (!ProductResult.Impact.IsValid())
	{
		const bool bCancelled = CancelPreDeliveryResource();
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = bCancelled
			? Edemo_mapM01EnemyAttackExecutionError::ImpactResolutionFailed
			: Edemo_mapM01EnemyAttackExecutionError::
				ResourceDefensePreparationFailed;
		return ProductResult;
	}

	ProductResult.Delivery = DeliverM01EnemyAttackImpactToPlayer(
		ProductResult.Impact,
		SourceEnemy);
	if (!ProductResult.Delivery.IsSuccess())
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::DeliveryRejected;
		return ProductResult;
	}
	if (!ActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Active,
		Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery,
			Transition))
	{
		ProductResult.Error =
			Edemo_mapM01EnemyAttackExecutionError::RuntimeCompletionFailed;
		return ProductResult;
	}

	ProductResult.Error =
		Edemo_mapM01EnemyAttackExecutionError::None;
	return ProductResult;
}

Fdemo_mapBasicSwordProductExecutionResult
Fdemo_mapCombatRunCoordinator::ExecutePlayerBasicSwordSweep(
	const FGuid& SourceItemInstanceId,
	float AttackPower,
	const TArray<FHitResult>& WorldHits)
{
	Fdemo_mapBasicSwordProductExecutionResult ProductResult;
	ProductResult.WorldContactCount = WorldHits.Num();
	if (!IsReady() || NextPlayerBasicSwordActivationSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::CoordinatorNotReady;
		return ProductResult;
	}
	if (!SourceItemInstanceId.IsValid())
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::InvalidSourceItem;
		return ProductResult;
	}

	FShanmenBasicSwordOffenseSnapshot Offense;
	if (!FShanmenBasicSwordOffenseSnapshot::TryCapture(
			AttackPower,
			Offense))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::InvalidOffense;
		return ProductResult;
	}

	FShanmenCombatActionSnapshot Action;
	if (!TryBuildProductBasicSwordAction(
			GetRunId(),
			PlayerEntityId,
			SourceItemInstanceId,
			NextPlayerBasicSwordActivationSequence,
			Action))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::ActionConstructionFailed;
		return ProductResult;
	}

	FShanmenBasicSwordDefinition Definition;
	if (!TryBuildProductBasicSwordDefinition(Definition))
	{
		ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
			DefinitionConstructionFailed;
		return ProductResult;
	}
	FShanmenBasicSwordExecution SwordExecution;
	if (!FShanmenBasicSwordExecution::TryCreate(
			Action,
			Definition,
			Offense,
			SwordExecution))
	{
		ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
			ExecutionConstructionFailed;
		return ProductResult;
	}

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt Transition;
	if (!FShanmenActionOrchestrator::TryStart(
			Action,
			ActionRuntime,
			Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::RuntimeStartFailed;
		return ProductResult;
	}
	ProductResult.ActivationId = Action.GetActivationId();
	++NextPlayerBasicSwordActivationSequence;

	FShanmenWorldHitContext HitContext;
	if (!SwordExecution.TryBeginEmission(ActionRuntime, HitContext))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::EmissionStartFailed;
		return ProductResult;
	}

	for (const FHitResult& WorldHit : WorldHits)
	{
		AActor* TargetEnemy = WorldHit.GetActor();
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(TargetEnemy);
		if (!VitalityHost || !VitalityHost->IsCombatEntityBound())
		{
			continue;
		}

		FShanmenHitCandidate Candidate;
		if (!FShanmenWorldHitAdapter::TryFromSweep(
				HitContext,
				WorldHit,
				EntityRegistry,
				Candidate))
		{
			continue;
		}
		++ProductResult.ResolvedCandidateCount;

		FShanmenTargetVitalitySnapshot Vitality;
		if (!VitalityHost->TryCaptureCombatVitalitySnapshot(Vitality))
		{
			continue;
		}
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenBasicSwordImpactReceipt Impact;
		if (!SwordExecution.TryResolveCandidate(
				ActionRuntime,
				Candidate,
				Vitality,
				Defense,
				Impact))
		{
			continue;
		}

		const Fdemo_mapCombatImpactDeliveryResult Delivery =
			DeliverBasicSwordImpactToM01Enemy(Impact, TargetEnemy);
		if (!Delivery.IsSuccess())
		{
			SwordExecution.EndEmissionForTermination();
			ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Transition);
			ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
				DeliveryRejected;
			return ProductResult;
		}
		++ProductResult.DeliveredImpactCount;
		if (Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::Committed)
		{
			++ProductResult.CommittedImpactCount;
		}
		else if (Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::AlreadyCommitted)
		{
			++ProductResult.AlreadyCommittedImpactCount;
		}
	}

	if (!SwordExecution.TryEndEmission(ActionRuntime))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::EmissionEndFailed;
		return ProductResult;
	}
	if (!ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active,
			Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery,
			Transition))
	{
		ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
			RuntimeCompletionFailed;
		return ProductResult;
	}

	ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::None;
	return ProductResult;
}

uint64 Fdemo_mapCombatRunCoordinator::
GetNextPlayerShapeSkillActivationSequence(
	Edemo_mapPlayerShapeSkillFamily Family) const
{
	switch (Family)
	{
	case Edemo_mapPlayerShapeSkillFamily::GroundCircle:
		return NextPlayerGroundCircleActivationSequence;
	case Edemo_mapPlayerShapeSkillFamily::SelfSector:
		return NextPlayerSelfSectorActivationSequence;
	default:
		return 0;
	}
}

Fdemo_mapPlayerShapeSkillExecutionResult
Fdemo_mapCombatRunCoordinator::ExecutePlayerShapeSkill(
	Edemo_mapPlayerShapeSkillFamily Family,
	float RawDamage,
	const TArray<FOverlapResult>& WorldOverlaps,
	const FVector& ContactOrigin)
{
	Fdemo_mapPlayerShapeSkillExecutionResult ProductResult;
	ProductResult.Family = Family;
	ProductResult.WorldContactCount = WorldOverlaps.Num();
	if (!IsReady())
	{
		ProductResult.Error =
			Edemo_mapPlayerShapeSkillExecutionError::CoordinatorNotReady;
		return ProductResult;
	}
	FPlayerShapeSkillSpec Spec;
	if (!TryGetPlayerShapeSkillSpec(Family, Spec))
	{
		ProductResult.Error =
			Edemo_mapPlayerShapeSkillExecutionError::InvalidFamily;
		return ProductResult;
	}
	if (!FMath::IsFinite(RawDamage) || RawDamage < 0.0f
		|| ContactOrigin.ContainsNaN())
	{
		ProductResult.Error =
			Edemo_mapPlayerShapeSkillExecutionError::InvalidDamage;
		return ProductResult;
	}

	uint64* NextActivationSequence = nullptr;
	switch (Family)
	{
	case Edemo_mapPlayerShapeSkillFamily::GroundCircle:
		NextActivationSequence = &NextPlayerGroundCircleActivationSequence;
		break;
	case Edemo_mapPlayerShapeSkillFamily::SelfSector:
		NextActivationSequence = &NextPlayerSelfSectorActivationSequence;
		break;
	default:
		break;
	}
	if (!NextActivationSequence || *NextActivationSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapPlayerShapeSkillExecutionError::SequenceExhausted;
		return ProductResult;
	}

	FShanmenCombatActionSnapshot Action;
	if (!TryBuildPlayerShapeSkillAction(
		Spec,
		GetRunId(),
		PlayerEntityId,
		*NextActivationSequence,
		Action))
	{
		ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
			ActionConstructionFailed;
		return ProductResult;
	}

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt Transition;
	if (!FShanmenActionOrchestrator::TryStart(
		Action,
		ActionRuntime,
		Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition))
	{
		ProductResult.Error =
			Edemo_mapPlayerShapeSkillExecutionError::RuntimeStartFailed;
		return ProductResult;
	}
	ProductResult.ActivationId = Action.GetActivationId();
	++(*NextActivationSequence);

	FShanmenWorldHitContext HitContext;
	if (!FShanmenWorldHitContext::TryCreate(
		Action,
		Spec.DetectorId,
		EShanmenHitDetectorKind::Shape,
		0,
		HitContext))
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
			ActionConstructionFailed;
		return ProductResult;
	}

	struct FResolvedShapeContact
	{
		FShanmenHitCandidate Candidate;
		TWeakObjectPtr<AActor> TargetActor;
	};
	TArray<FResolvedShapeContact> ResolvedContacts;
	TSet<FGuid> ResolvedTargetIds;
	for (const FOverlapResult& WorldOverlap : WorldOverlaps)
	{
		AActor* TargetActor = WorldOverlap.GetActor();
		if (!TargetActor)
		{
			continue;
		}
		FVector ContactNormal = TargetActor->GetActorLocation() - ContactOrigin;
		if (!ContactNormal.Normalize())
		{
			ContactNormal = FVector::UpVector;
		}
		FShanmenHitCandidate Candidate;
		if (!FShanmenWorldHitAdapter::TryFromOverlap(
			HitContext,
			WorldOverlap,
			TargetActor->GetActorLocation(),
			ContactNormal,
			EntityRegistry,
			Candidate)
			|| Candidate.TargetEntityId == PlayerEntityId
			|| ResolvedTargetIds.Contains(Candidate.TargetEntityId))
		{
			continue;
		}
		ResolvedTargetIds.Add(Candidate.TargetEntityId);
		FResolvedShapeContact& Contact = ResolvedContacts.AddDefaulted_GetRef();
		Contact.Candidate = MoveTemp(Candidate);
		Contact.TargetActor = TargetActor;
	}
	ResolvedContacts.Sort(
		[](const FResolvedShapeContact& Left,
			const FResolvedShapeContact& Right)
		{
			return Left.Candidate.TargetEntityId.ToString(EGuidFormats::Digits)
				< Right.Candidate.TargetEntityId.ToString(EGuidFormats::Digits);
		});
	ProductResult.ResolvedCandidateCount = ResolvedContacts.Num();

	FShanmenImpactLedger ImpactLedger;
	for (const FResolvedShapeContact& Contact : ResolvedContacts)
	{
		AActor* TargetActor = Contact.TargetActor.Get();
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(TargetActor);
		FShanmenTargetVitalitySnapshot Vitality;
		if (!VitalityHost
			|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Vitality))
		{
			ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Transition);
			ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
				VitalitySnapshotFailed;
			return ProductResult;
		}

		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenDamagePacket Damage;
		Damage.FormulaId = Spec.FormulaId;
		Damage.RawDamage = RawDamage;
		Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());

		FShanmenImpactRequest Request;
		Request.Action = Action;
		Request.Candidate = Contact.Candidate;
		Request.Damage = MoveTemp(Damage);
		Request.TargetVitality = Vitality;
		Request.Defense = MoveTemp(Defense);
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Action.GetRunId(),
			Contact.Candidate.ActivationId,
			Contact.Candidate.DetectorId,
			Contact.Candidate.TargetEntityId,
			Contact.Candidate.HitOrdinal);
		if (!Request.IsValid() || !ImpactLedger.TryAccept(Request))
		{
			ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Transition);
			ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
				ImpactResolutionFailed;
			return ProductResult;
		}

		Fdemo_mapPlayerShapeSkillImpactReceipt Impact;
		Impact.Family = Family;
		Impact.Request = MoveTemp(Request);
		Impact.Result = FShanmenDefenseResolver::Resolve(Impact.Request);
		if (!Impact.IsValid())
		{
			ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Transition);
			ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
				ImpactResolutionFailed;
			return ProductResult;
		}

		const Fdemo_mapCombatImpactDeliveryResult Delivery =
			DeliverPlayerShapeSkillImpactToM01Enemy(Impact, TargetActor);
		if (!Delivery.IsSuccess())
		{
			ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Transition);
			ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
				DeliveryRejected;
			return ProductResult;
		}

		ProductResult.OrderedTargetEntityIds.Add(
			Contact.Candidate.TargetEntityId);
		ProductResult.Impacts.Add(MoveTemp(Impact));
		++ProductResult.DeliveredImpactCount;
		if (Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::Committed)
		{
			++ProductResult.CommittedImpactCount;
		}
		else if (Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::AlreadyCommitted)
		{
			++ProductResult.AlreadyCommittedImpactCount;
		}
	}

	if (!ActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Active,
		Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery,
			Transition))
	{
		ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::
			RuntimeCompletionFailed;
		return ProductResult;
	}

	ProductResult.Error = Edemo_mapPlayerShapeSkillExecutionError::None;
	return ProductResult;
}

Fdemo_mapPlayerProjectileLaunchResult
Fdemo_mapCombatRunCoordinator::PreparePlayerStraightProjectile(
	AActor* SourcePlayer,
	float RawDamage)
{
	Fdemo_mapPlayerProjectileLaunchResult Launch;
	if (!IsReady())
	{
		return Launch;
	}
	if (SourcePlayer != BoundPlayerPawn.Get())
	{
		Launch.Error = Edemo_mapPlayerProjectileLaunchError::SourceMismatch;
		return Launch;
	}
	if (!FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
	{
		Launch.Error = Edemo_mapPlayerProjectileLaunchError::InvalidDamage;
		return Launch;
	}
	if (NextPlayerStraightProjectileActivationSequence == 0
		|| NextPlayerStraightProjectileActivationSequence == MAX_uint64)
	{
		Launch.Error =
			Edemo_mapPlayerProjectileLaunchError::SequenceExhausted;
		return Launch;
	}

	const FPlayerStraightProjectileSpec Spec;
	FShanmenCombatActionSnapshot Action;
	if (!TryBuildPlayerStraightProjectileAction(
		Spec,
		GetRunId(),
		PlayerEntityId,
		NextPlayerStraightProjectileActivationSequence,
		Action))
	{
		Launch.Error =
			Edemo_mapPlayerProjectileLaunchError::ActionConstructionFailed;
		return Launch;
	}

	Launch.ActivationSequence =
		NextPlayerStraightProjectileActivationSequence;
	Launch.ActivationId = Action.GetActivationId();
	Launch.RawDamage = RawDamage;
	Launch.Error = Edemo_mapPlayerProjectileLaunchError::None;
	++NextPlayerStraightProjectileActivationSequence;
	return Launch;
}

bool Fdemo_mapCombatRunCoordinator::TryReservePlayerThrownWeaponAction(
	const FGuid& SourceItemInstanceId,
	Fdemo_mapPlayerThrownWeaponActionReservation& OutReservation,
	FString& OutDiagnostic)
{
	OutReservation = Fdemo_mapPlayerThrownWeaponActionReservation();
	OutDiagnostic.Reset();
	if (!IsReady())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon identity requires one ready combat Run.");
		return false;
	}
	if (!SourceItemInstanceId.IsValid())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon identity requires one exact source item.");
		return false;
	}
	if (NextPlayerThrownWeaponActivationSequence == 0
		|| NextPlayerThrownWeaponActivationSequence == MAX_uint64)
	{
		OutDiagnostic = TEXT("Thrown-weapon activation sequence is exhausted.");
		return false;
	}

	OutReservation.ActivationSequence =
		NextPlayerThrownWeaponActivationSequence;
	OutReservation.RunId = GetRunId();
	OutReservation.SourceEntityId = PlayerEntityId;
	OutReservation.SourceItemInstanceId = SourceItemInstanceId;
	OutReservation.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
		OutReservation.RunId,
		OutReservation.SourceEntityId,
		FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId(),
		OutReservation.ActivationSequence);
	if (!OutReservation.IsValid())
	{
		OutReservation = Fdemo_mapPlayerThrownWeaponActionReservation();
		OutDiagnostic =
			TEXT("Thrown-weapon deterministic action identity failed closed.");
		return false;
	}
	++NextPlayerThrownWeaponActivationSequence;
	OutDiagnostic = TEXT("Thrown-weapon action identity reserved by the Run.");
	return true;
}

Fdemo_mapPlayerProjectileImpactResult
Fdemo_mapCombatRunCoordinator::ExecutePlayerStraightProjectileImpact(
	AActor* SourcePlayer,
	AActor* TargetEnemy,
	UPrimitiveComponent* TargetComponent,
	uint64 ActivationSequence,
	const FGuid& ExpectedActivationId,
	float RawDamage,
	const FVector& ImpactLocation,
	const FVector& ImpactNormal)
{
	Fdemo_mapPlayerProjectileImpactResult ProductResult;
	ProductResult.ActivationSequence = ActivationSequence;
	if (!IsReady())
	{
		return ProductResult;
	}
	if (SourcePlayer != BoundPlayerPawn.Get())
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::SourceMismatch;
		return ProductResult;
	}
	if (ActivationSequence == 0 || !ExpectedActivationId.IsValid())
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::InvalidLaunchIdentity;
		return ProductResult;
	}
	if (!FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::InvalidDamage;
		return ProductResult;
	}
	if (ImpactLocation.ContainsNaN() || ImpactNormal.ContainsNaN())
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::InvalidContact;
		return ProductResult;
	}

	FGuid TargetEntityId;
	if (!TargetEnemy
		|| !EntityRegistry.TryResolveObject(
			GetRunId(), TargetEnemy, INDEX_NONE, TargetEntityId)
		|| !M01EnemyBindings.Contains(TargetEntityId))
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::TargetNotRegistered;
		return ProductResult;
	}

	const FPlayerStraightProjectileSpec Spec;
	FShanmenCombatActionSnapshot Action;
	if (!TryBuildPlayerStraightProjectileAction(
		Spec,
		GetRunId(),
		PlayerEntityId,
		ActivationSequence,
		Action))
	{
		ProductResult.Error = Edemo_mapPlayerProjectileImpactError::
			ActionConstructionFailed;
		return ProductResult;
	}
	ProductResult.ActivationId = Action.GetActivationId();
	if (ProductResult.ActivationId != ExpectedActivationId)
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::InvalidLaunchIdentity;
		return ProductResult;
	}

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt Transition;
	if (!FShanmenActionOrchestrator::TryStart(
		Action,
		ActionRuntime,
		Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition))
	{
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::RuntimeStartFailed;
		return ProductResult;
	}

	FShanmenWorldHitContext HitContext;
	if (!FShanmenWorldHitContext::TryCreate(
		Action,
		Spec.DetectorId,
		EShanmenHitDetectorKind::Projectile,
		0,
		HitContext))
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = Edemo_mapPlayerProjectileImpactError::
			ActionConstructionFailed;
		return ProductResult;
	}

	FHitResult WorldHit(
		TargetEnemy,
		TargetComponent,
		ImpactLocation,
		ImpactNormal);
	WorldHit.ImpactPoint = ImpactLocation;
	WorldHit.ImpactNormal = ImpactNormal;
	WorldHit.Item = 0;
	FShanmenHitCandidate Candidate;
	if (!FShanmenWorldHitAdapter::TryFromProjectile(
		HitContext,
		WorldHit,
		EntityRegistry,
		Candidate)
		|| Candidate.TargetEntityId != TargetEntityId)
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = Edemo_mapPlayerProjectileImpactError::
			CandidateConstructionFailed;
		return ProductResult;
	}

	Idemo_mapCombatVitalityHost* VitalityHost =
		ResolveM01VitalityHost(TargetEnemy);
	FShanmenTargetVitalitySnapshot Vitality;
	if (!VitalityHost
		|| !VitalityHost->IsCombatEntityBound()
		|| VitalityHost->GetCombatEntityId() != TargetEntityId
		|| !VitalityHost->TryCaptureCombatVitalitySnapshot(Vitality))
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = Edemo_mapPlayerProjectileImpactError::
			VitalitySnapshotFailed;
		return ProductResult;
	}

	FShanmenImpactRequest Request;
	Request.Action = Action;
	Request.Candidate = Candidate;
	Request.Damage.FormulaId = Spec.FormulaId;
	Request.Damage.RawDamage = RawDamage;
	Request.Damage.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysical());
	Request.TargetVitality = Vitality;
	Request.Defense.TargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
		GetRunId(),
		Action.GetActivationId(),
		Candidate.DetectorId,
		Candidate.TargetEntityId,
		Candidate.HitOrdinal);
	ProductResult.Impact.Request = MoveTemp(Request);
	ProductResult.Impact.Result = FShanmenDefenseResolver::Resolve(
		ProductResult.Impact.Request);
	if (!ProductResult.Impact.IsValid())
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error = Edemo_mapPlayerProjectileImpactError::
			ImpactResolutionFailed;
		return ProductResult;
	}

	ProductResult.Delivery = DeliverPlayerProjectileImpactToM01Enemy(
		ProductResult.Impact,
		TargetEnemy);
	if (!ProductResult.Delivery.IsSuccess())
	{
		ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Active,
			Transition);
		ProductResult.Error =
			Edemo_mapPlayerProjectileImpactError::DeliveryRejected;
		return ProductResult;
	}
	if (!ActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Active,
		Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery,
			Transition))
	{
		ProductResult.Error = Edemo_mapPlayerProjectileImpactError::
			RuntimeCompletionFailed;
		return ProductResult;
	}

	ProductResult.Error = Edemo_mapPlayerProjectileImpactError::None;
	return ProductResult;
}
