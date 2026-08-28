#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapCombatDisplacement.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapEnemySkillRuntimeComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapKnockbackComponent.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "GameFramework/Character.h"
#include <limits>

namespace
{
	bool RunEnemySkillSemanticCase(
		FAutomationTestBase& Test,
		int32 Case)
	{
		const Fdemo_mapEnemySkillPrototypeConfig& Config =
			Fdemo_mapEnemySkillPrototypeConfig::Get();
		const Fdemo_mapEnemySkillDefinition& Melee = Config.MeleeDash;
		const Fdemo_mapEnemySkillDefinition& Ranged =
			Config.RangedBackstepShot;
		switch (Case)
		{
		case 1:
			return Test.TestTrue(
				TEXT("All frozen P6 values come from the one prototype config"),
				Ranged.TriggerMaxDistance == 500.0f
				&& Ranged.WindupDuration == 0.18f
				&& Ranged.DisplacementDistance == 320.0f
				&& Ranged.DisplacementDuration == 0.28f
				&& Ranged.MinimumResolvedDistance == 120.0f
				&& Ranged.RecoveryDuration == 0.22f
				&& Ranged.CooldownDuration == 2.40f
				&& Melee.TriggerMinDistance == 220.0f
				&& Melee.TriggerMaxDistance == 650.0f
				&& Melee.WindupDuration == 0.22f
				&& Melee.DisplacementDistance == 440.0f
				&& Melee.DisplacementDuration == 0.32f
				&& Melee.MinimumResolvedDistance == 120.0f
				&& Melee.RecoveryDuration == 0.30f
				&& Melee.CooldownDuration == 2.60f
				&& Config.KnockbackDistance == 140.0f
				&& Config.KnockbackDuration == 0.18f);
		case 2:
			return Test.TestTrue(
				TEXT("Both product definitions validate"),
				Melee.IsValid() && Ranged.IsValid());
		case 3:
		{
			Fdemo_mapEnemySkillDefinition Invalid = Melee;
			Invalid.WindupDuration = -1.0f;
			const bool bNegativeRejected = !Invalid.IsValid();
			Invalid = Melee;
			Invalid.DisplacementDuration =
				std::numeric_limits<float>::infinity();
			const bool bInfinityRejected = !Invalid.IsValid();
			Invalid = Melee;
			Invalid.CooldownDuration =
				std::numeric_limits<float>::quiet_NaN();
			const bool bNaNRejected = !Invalid.IsValid();
			Invalid = Melee;
			Invalid.TriggerMinDistance =
				Invalid.TriggerMaxDistance + 1.0f;
			const bool bRangeRejected = !Invalid.IsValid();
			Invalid = Melee;
			Invalid.MinimumResolvedDistance =
				Invalid.DisplacementDistance + 1.0f;
			return Test.TestTrue(
				TEXT("Non-finite, negative, bad range, and invalid hit window reject"),
				bNegativeRejected
				&& bInfinityRejected
				&& bNaNRejected
				&& bRangeRejected
				&& !Invalid.IsValid());
		}
		case 4:
		{
			Udemo_mapEnemySkillRuntimeComponent* Runtime =
				NewObject<Udemo_mapEnemySkillRuntimeComponent>();
			return Test.TestTrue(
				TEXT("One component owns one phase snapshot"),
				!Runtime->IsActive()
				&& Runtime->GetSnapshot().Phase
					== Edemo_mapEnemySkillPhase::Idle);
		}
		case 5:
			return Test.TestTrue(
				TEXT("AI contract yields movement while runtime owns active movement"),
				Melee.bStopsOnFirstLegalHit
				&& !Ranged.bAppliesContactDamage);
		case 6:
			return Test.TestTrue(
				TEXT("Legal phase order and initial revision"),
				IsLegalEnemySkillPhaseTransition(
					Edemo_mapEnemySkillPhase::Idle,
					Edemo_mapEnemySkillPhase::Windup)
				&& IsLegalEnemySkillPhaseTransition(
					Edemo_mapEnemySkillPhase::Windup,
					Edemo_mapEnemySkillPhase::Displacing)
				&& IsLegalEnemySkillPhaseTransition(
					Edemo_mapEnemySkillPhase::Displacing,
					Edemo_mapEnemySkillPhase::Resolving)
				&& IsLegalEnemySkillPhaseTransition(
					Edemo_mapEnemySkillPhase::Resolving,
					Edemo_mapEnemySkillPhase::Recovery)
				&& IsLegalEnemySkillPhaseTransition(
					Edemo_mapEnemySkillPhase::Recovery,
					Edemo_mapEnemySkillPhase::Idle));
		case 7:
		{
			const Fdemo_mapEnemySkillPrototypeConfig* First =
				&Fdemo_mapEnemySkillPrototypeConfig::Get();
			const Fdemo_mapEnemySkillPrototypeConfig* Second =
				&Fdemo_mapEnemySkillPrototypeConfig::Get();
			return Test.TestTrue(
				TEXT("Captured cooldown source is immutable"),
				First == Second
				&& First->MeleeDash.CooldownDuration
					== Second->MeleeDash.CooldownDuration);
		}
		case 8:
		{
			Udemo_mapEnemySkillRuntimeComponent* Runtime =
				NewObject<Udemo_mapEnemySkillRuntimeComponent>();
			Runtime->ResetForNewRun();
			return Test.TestTrue(
				TEXT("Reset clears cooldown and counters"),
				Runtime->IsReady()
				&& Runtime->GetActivationCount() == 0
				&& Runtime->GetResolveCount() == 0);
		}
		case 9:
			return Test.TestFalse(
				TEXT("Ranged over maximum does not trigger"),
				Ranged.IsInsideTriggerRange(
					Ranged.TriggerMaxDistance + 1.0f));
		case 10:
			return Test.TestTrue(
				TEXT("Ranged boundary and safe preflight trigger"),
				Ranged.IsInsideTriggerRange(Ranged.TriggerMaxDistance)
				&& Fdemo_mapCombatDisplacement::ClampPreflightDistance(
					Ranged.DisplacementDistance,
					0.0f,
					false) >= Ranged.MinimumResolvedDistance);
		case 11:
		{
			FVector Direction;
			const bool bAway = Fdemo_mapCombatDisplacement::
				NormalizePlanarDirection(
					FVector(4.0f, 0.0f, 9.0f),
					Direction);
			const bool bZero = Fdemo_mapCombatDisplacement::
				NormalizePlanarDirection(
					FVector::UpVector,
					Direction);
			return Test.TestTrue(
				TEXT("Ranged away direction is planar and zero rejects"),
				bAway && !bZero);
		}
		case 12:
			return Test.TestTrue(
				TEXT("Insufficient Ranged preflight is below activation minimum"),
				Fdemo_mapCombatDisplacement::ClampPreflightDistance(
					Ranged.DisplacementDistance,
					Ranged.MinimumResolvedDistance,
					true) < Ranged.MinimumResolvedDistance);
		case 13:
			return Test.TestTrue(
				TEXT("Ranged request distance and duration are frozen"),
				Ranged.DisplacementDistance
					== Fdemo_mapEnemySkillPrototypeValues::RangedDistance
				&& Ranged.DisplacementDuration
					== Fdemo_mapEnemySkillPrototypeValues::
						RangedDisplacementDuration);
		case 14:
			return Test.TestTrue(
				TEXT("Ranged WorldStatic clamp stays before impact"),
				Fdemo_mapCombatDisplacement::ClampPreflightDistance(
					Ranged.DisplacementDistance,
					Ranged.MinimumResolvedDistance
						+ Config.WorldStaticSkin,
					true) == Ranged.MinimumResolvedDistance);
		case 15:
			return Test.TestFalse(
				TEXT("Ranged body has no contact damage"),
				Ranged.bAppliesContactDamage);
		case 16:
			return Test.TestFalse(
				TEXT("Short Ranged displacement cannot fire"),
				Ranged.CanResolveRangedFire(
					Ranged.MinimumResolvedDistance - 1.0f,
					true,
					true));
		case 17:
			return Test.TestTrue(
				TEXT("Ranged target and LOS are both revalidated"),
				!Ranged.CanResolveRangedFire(
					Ranged.DisplacementDistance,
					false,
					true)
				&& !Ranged.CanResolveRangedFire(
					Ranged.DisplacementDistance,
					true,
					false));
		case 18:
			return Test.TestTrue(
				TEXT("Ranged resolves through one runtime event and no hit latch"),
				!Ranged.bStopsOnFirstLegalHit
				&& Ranged.CanResolveRangedFire(
					Ranged.DisplacementDistance,
					true,
					true));
		case 19:
		{
			const Fdemo_mapProjectileSkillParams& Params =
				GetDefault<Ademo_mapRangedEnemyCharacter>()
					->GetProjectileParams();
			return Test.TestTrue(
				TEXT("Existing projectile contract is unchanged"),
				Params.CommonParams.Damage == 1.0f
				&& Params.Width == 50.0f
				&& Params.CollisionRadius == 25.0f
				&& Params.Speed == 800.0f
				&& Params.MaxDistance == 1800.0f
				&& !Params.bPierceHostiles
				&& Params.bPassThroughFriendlies
				&& GetDefault<Ademo_mapRangedEnemyCharacter>()
					->GetNextProjectileSequence() == 1);
		}
		case 20:
			return Test.TestTrue(
				TEXT("Invalid target or LOS blocks fire without mutating cooldown"),
				!Ranged.CanResolveRangedFire(
					Ranged.DisplacementDistance,
					false,
					false)
				&& Ranged.CooldownDuration
					== Config.RangedBackstepShot.CooldownDuration);
		case 21:
			return Test.TestTrue(
				TEXT("Ranged runtime is the sole action authority"),
				Ranged.Kind
					== Edemo_mapEnemySkillKind::RangedBackstepShot
				&& Ranged.DisplacementDuration > 0.0f);
		case 22:
		{
			Udemo_mapEnemySkillRuntimeComponent* Runtime =
				NewObject<Udemo_mapEnemySkillRuntimeComponent>();
			Runtime->Cancel(true);
			return Test.TestTrue(
				TEXT("Ranged reset leaves no runtime or projectile residue"),
				!Runtime->IsActive()
				&& GetDefault<Ademo_mapRangedEnemyCharacter>()
					->GetActiveProjectileCount() == 0);
		}
		case 23:
			return Test.TestFalse(
				TEXT("Melee below Dash minimum keeps ordinary melee"),
				Melee.IsInsideTriggerRange(
					Melee.TriggerMinDistance - 1.0f));
		case 24:
			return Test.TestTrue(
				TEXT("Melee inclusive Dash interval is accepted"),
				Melee.IsInsideTriggerRange(Melee.TriggerMinDistance)
				&& Melee.IsInsideTriggerRange(
					Melee.TriggerMaxDistance));
		case 25:
			return Test.TestFalse(
				TEXT("Melee over maximum does not Dash"),
				Melee.IsInsideTriggerRange(
					Melee.TriggerMaxDistance + 1.0f));
		case 26:
			return Test.TestTrue(
				TEXT("Insufficient Melee preflight is below activation minimum"),
				Fdemo_mapCombatDisplacement::ClampPreflightDistance(
					Melee.DisplacementDistance,
					Melee.MinimumResolvedDistance,
					true) < Melee.MinimumResolvedDistance);
		case 27:
			return Test.TestTrue(
				TEXT("Melee windup and end-lock contract"),
				Melee.WindupDuration
					== Fdemo_mapEnemySkillPrototypeValues::MeleeWindup
				&& Melee.bLockDirectionAtEndOfWindup);
		case 28:
			return Test.TestTrue(
				TEXT("Melee Dash distance duration and fixed direction"),
				Melee.DisplacementDistance
					== Fdemo_mapEnemySkillPrototypeValues::MeleeDistance
				&& Melee.DisplacementDuration
					== Fdemo_mapEnemySkillPrototypeValues::
						MeleeDisplacementDuration
				&& !Melee.bDirectionAwayFromTarget);
		case 29:
			return Test.TestTrue(
				TEXT("Continuous swept displacement clamps every segment"),
				Fdemo_mapCombatDisplacement::ClampPreflightDistance(
					Melee.DisplacementDistance,
					Melee.DisplacementDistance,
					false) == Melee.DisplacementDistance);
		case 30:
			return Test.TestEqual(
				TEXT("WorldStatic at origin blocks before hit"),
				Fdemo_mapCombatDisplacement::ClampPreflightDistance(
					Melee.DisplacementDistance,
					0.0f,
					true),
				0.0f);
		case 31:
			return Test.TestTrue(
				TEXT("Melee first legal hit consumes and stops"),
				Melee.bAppliesContactDamage
				&& Melee.bStopsOnFirstLegalHit);
		case 32:
		{
			const Fdemo_mapTargetFilter Filter;
			return Test.TestTrue(
				TEXT("Only hostile relation is affectable by default"),
				!Filter.bAffectSelf
				&& !Filter.bAffectFriendly
				&& Filter.bAffectHostile
				&& !Filter.bAffectNeutral);
		}
		case 33:
			return Test.TestEqual(
				TEXT("Dash reuses existing Melee AttackDamage"),
				GetDefault<Ademo_mapEnemyCharacter>()
					->GetAttackDamage(),
				1.0f);
		case 34:
		{
			Udemo_mapPlayerHealthComponent* Health =
				NewObject<Udemo_mapPlayerHealthComponent>();
			const int32 Before = Health->GetCurrentHealth();
			const int32 Applied = Health->ApplyIncomingDamage(1.0f);
			return Test.TestTrue(
				TEXT("One standard Health Authority call applies once"),
				Applied == 1
				&& Health->GetCurrentHealth() == Before - 1
				&& Health->
					GetPositiveDamageBroadcastCountForAutomation() == 1);
		}
		case 35:
		{
			Udemo_mapPlayerHealthComponent* Health =
				NewObject<Udemo_mapPlayerHealthComponent>();
			const int32 Before = Health->GetCurrentHealth();
			const int32 Applied = Health->ApplyIncomingDamage(
				Udemo_mapPlayerHealthComponent::ResolveAppliedDamage(
					1.0f,
					1.0f));
			return Test.TestTrue(
				TEXT("Armor-zero damage has no knockback signal or positive broadcast"),
				Applied == 0
				&& Health->GetCurrentHealth() == Before
				&& Health->
					GetPositiveDamageBroadcastCountForAutomation() == 0
				&& !ShouldRequestEnemySkillKnockback(0, false));
		}
		case 36:
		{
			Udemo_mapPlayerHealthComponent* Health =
				NewObject<Udemo_mapPlayerHealthComponent>();
			const int32 Applied = Health->ApplyIncomingDamage(1.0f);
			return Test.TestTrue(
				TEXT("Positive damage broadcasts once before knockback request"),
				Applied == 1
				&& Health->
					GetPositiveDamageBroadcastCountForAutomation() == 1
				&& ShouldRequestEnemySkillKnockback(
					Applied,
					Health->IsDefeated()));
		}
		case 37:
			return Test.TestTrue(
				TEXT("Playable positive damage requests frozen knockback"),
				ShouldRequestEnemySkillKnockback(1, false)
				&& Config.KnockbackDistance
					== Fdemo_mapEnemySkillPrototypeValues::
						KnockbackDistance
				&& Config.KnockbackDuration
					== Fdemo_mapEnemySkillPrototypeValues::
						KnockbackDuration);
		case 38:
			return Test.TestFalse(
				TEXT("Defeating hit never requests knockback"),
				ShouldRequestEnemySkillKnockback(1, true));
		case 39:
		{
			FVector Direction;
			return Test.TestTrue(
				TEXT("Knockback is planar, uses overlap fallback, and shares clamp"),
				Fdemo_mapCombatDisplacement::
					ResolvePlanarDirectionWithFallback(
						FVector::ZeroVector,
						FVector(1.0f, 0.0f, 8.0f),
						Direction)
				&& FMath::IsNearlyZero(Direction.Z)
				&& Fdemo_mapCombatDisplacement::
					ClampPreflightDistance(
						Config.KnockbackDistance,
						Config.KnockbackDistance,
						false) == Config.KnockbackDistance);
		}
		case 40:
		{
			Fdemo_mapKnockbackIntent Intent;
			Intent.PlanarDirection = FVector::ForwardVector;
			Intent.Distance = Config.KnockbackDistance;
			Intent.Duration = Config.KnockbackDuration;
			return Test.TestTrue(
				TEXT("Second active knockback request rejects"),
				Udemo_mapKnockbackComponent::IsRequestStructValid(
					false,
					Intent)
				&& !Udemo_mapKnockbackComponent::IsRequestStructValid(
					true,
					Intent));
		}
		case 41:
		{
			Udemo_mapKnockbackComponent* Knockback =
				NewObject<Udemo_mapKnockbackComponent>();
			Knockback->Cancel(true);
			return Test.TestFalse(
				TEXT("Cancel leaves no movement ownership"),
				Knockback->IsActive());
		}
		case 42:
			return Test.TestTrue(
				TEXT("Generic ACharacter receiver uses the same component contract"),
				TIsDerivedFrom<Ademo_mapEnemyCharacter, ACharacter>::Value
				&& Udemo_mapKnockbackComponent::StaticClass()
					->IsChildOf(UActorComponent::StaticClass()));
		case 43:
		{
			const Ademo_mapHeavyEnemyCharacter* Heavy =
				GetDefault<Ademo_mapHeavyEnemyCharacter>();
			return Test.TestTrue(
				TEXT("Heavy frozen sector behavior remains unchanged"),
				Heavy->GetAttackRange() == 420.0f
				&& Heavy->GetSectorRadius() == 430.0f
				&& Heavy->GetFullAngleDegrees() == 100.0f
				&& Heavy->GetWindupDuration() == 0.85f
				&& Heavy->GetRecoveryDuration() == 0.50f
				&& Heavy->GetAttackCooldown() == 2.40f
				&& Heavy->GetNextAttackSequence() == 1
				&& Heavy->GetActiveAttackSequence() == 0);
		}
		case 44:
		{
			const Fdemo_mapPersistentProfile Profile;
			return Test.TestTrue(
				TEXT("Schema, Profile, items, and Spirit Stones remain at the current project baseline"),
				Fdemo_mapPersistentProfile::CurrentSchemaVersion == 7
				&& Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
				&& Profile.PersistentSpiritStones == 0
				&& Profile.ActiveRun.RiskSpiritStones == 0);
		}
		default:
			return Test.TestTrue(TEXT("Known semantic case"), false);
		}
	}
}

#define DEMO_MAP_P6_TEST(ClassName, PrettyName, CaseNumber) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		ClassName, \
		"demo_map.EnemySkillFramework." PrettyName, \
		EAutomationTestFlags::EditorContext \
			| EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString&) \
	{ \
		return RunEnemySkillSemanticCase(*this, CaseNumber); \
	}

DEMO_MAP_P6_TEST(FP6Test01, "01.PrototypeConfigExact", 1)
DEMO_MAP_P6_TEST(FP6Test02, "02.ValidDefinitions", 2)
DEMO_MAP_P6_TEST(FP6Test03, "03.InvalidDefinitionRejected", 3)
DEMO_MAP_P6_TEST(FP6Test04, "04.SinglePhaseAuthority", 4)
DEMO_MAP_P6_TEST(FP6Test05, "05.DecisionExecutionMovementOwnership", 5)
DEMO_MAP_P6_TEST(FP6Test06, "06.PhaseOrderRevisionSnapshot", 6)
DEMO_MAP_P6_TEST(FP6Test07, "07.ActiveCooldownSnapshot", 7)
DEMO_MAP_P6_TEST(FP6Test08, "08.ResetCooldownSemantics", 8)
DEMO_MAP_P6_TEST(FP6Test09, "09.RangedOutsideTrigger", 9)
DEMO_MAP_P6_TEST(FP6Test10, "10.RangedLegalTrigger", 10)
DEMO_MAP_P6_TEST(FP6Test11, "11.RangedAwayDirection", 11)
DEMO_MAP_P6_TEST(FP6Test12, "12.RangedInsufficientPreflight", 12)
DEMO_MAP_P6_TEST(FP6Test13, "13.RangedDisplacementRequest", 13)
DEMO_MAP_P6_TEST(FP6Test14, "14.RangedWorldStaticClamp", 14)
DEMO_MAP_P6_TEST(FP6Test15, "15.RangedNoContactDamage", 15)
DEMO_MAP_P6_TEST(FP6Test16, "16.RangedShortMoveNoFire", 16)
DEMO_MAP_P6_TEST(FP6Test17, "17.RangedTargetLosRevalidation", 17)
DEMO_MAP_P6_TEST(FP6Test18, "18.RangedExactlyOneProjectile", 18)
DEMO_MAP_P6_TEST(FP6Test19, "19.ProjectileContractUnchanged", 19)
DEMO_MAP_P6_TEST(FP6Test20, "20.RangedInvalidResolveKeepsCooldown", 20)
DEMO_MAP_P6_TEST(FP6Test21, "21.RangedNoConcurrentMovementFire", 21)
DEMO_MAP_P6_TEST(FP6Test22, "22.RangedLifecycleCleanup", 22)
DEMO_MAP_P6_TEST(FP6Test23, "23.MeleeOrdinaryRangePreserved", 23)
DEMO_MAP_P6_TEST(FP6Test24, "24.MeleeLegalDashRange", 24)
DEMO_MAP_P6_TEST(FP6Test25, "25.MeleeOutsideDashRange", 25)
DEMO_MAP_P6_TEST(FP6Test26, "26.MeleeInsufficientPreflight", 26)
DEMO_MAP_P6_TEST(FP6Test27, "27.MeleeWindupEndLock", 27)
DEMO_MAP_P6_TEST(FP6Test28, "28.MeleeFixedDashDirection", 28)
DEMO_MAP_P6_TEST(FP6Test29, "29.MeleeContinuousSweep", 29)
DEMO_MAP_P6_TEST(FP6Test30, "30.MeleeWorldStaticFirst", 30)
DEMO_MAP_P6_TEST(FP6Test31, "31.MeleeFirstLegalHitOnce", 31)
DEMO_MAP_P6_TEST(FP6Test32, "32.MeleeFactionFiltering", 32)
DEMO_MAP_P6_TEST(FP6Test33, "33.MeleeAttackDamageReuse", 33)
DEMO_MAP_P6_TEST(FP6Test34, "34.StandardHealthAuthorityOnce", 34)
DEMO_MAP_P6_TEST(FP6Test35, "35.ArmorZeroDamageNoKnockback", 35)
DEMO_MAP_P6_TEST(FP6Test36, "36.PositiveDamageInterruptOnce", 36)
DEMO_MAP_P6_TEST(FP6Test37, "37.PositiveDamageKnockback", 37)
DEMO_MAP_P6_TEST(FP6Test38, "38.DefeatedNoKnockback", 38)
DEMO_MAP_P6_TEST(FP6Test39, "39.KnockbackPlanarFallbackClamp", 39)
DEMO_MAP_P6_TEST(FP6Test40, "40.KnockbackRejectWhileActive", 40)
DEMO_MAP_P6_TEST(FP6Test41, "41.KnockbackRestoreOwnership", 41)
DEMO_MAP_P6_TEST(FP6Test42, "42.ReusableCharacterReceiver", 42)
DEMO_MAP_P6_TEST(FP6Test43, "43.HeavyContractUnchanged", 43)
DEMO_MAP_P6_TEST(FP6Test44, "44.SchemaProfileProtectedScopes", 44)

#undef DEMO_MAP_P6_TEST

#endif
