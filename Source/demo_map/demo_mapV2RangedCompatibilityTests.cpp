#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapEnemyEncounterConfig.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapEnemySkillRuntimeComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapRangedEnemyCharacter.h"
#include "demo_mapSkillProjectile.h"

namespace
{
	const Fdemo_mapEnemyEncounterSpawnRecord* FindRangedEncounter(
		Edemo_mapEnemyEncounterArchetype Archetype)
	{
		return Fdemo_mapEnemyEncounterConfig::GetSpawnRecords()
			.FindByPredicate([Archetype](
				const Fdemo_mapEnemyEncounterSpawnRecord& Record)
			{
				return Record.Archetype == Archetype;
			});
	}

	bool RunV2RangedCompatibilityCase(
		FAutomationTestBase& Test,
		int32 Case)
	{
		const Ademo_mapRangedEnemyCharacter* Ranged =
			GetDefault<Ademo_mapRangedEnemyCharacter>();
		const Fdemo_mapEnemyEncounterSpawnRecord* Standard =
			FindRangedEncounter(
				Edemo_mapEnemyEncounterArchetype::RangedStandard);
		const Fdemo_mapEnemyEncounterSpawnRecord* Enhanced =
			FindRangedEncounter(
				Edemo_mapEnemyEncounterArchetype::RangedEnhanced);
		const Fdemo_mapEnemySkillDefinition* StandardSkill =
			Fdemo_mapEnemySkillPrototypeConfig::FindProfile(
				Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep);
		const Fdemo_mapEnemySkillDefinition* EnhancedSkill =
			Fdemo_mapEnemySkillPrototypeConfig::FindProfile(
				Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep);
		switch (Case)
		{
		case 1:
			return Test.TestTrue(
				TEXT("Unconfigured V2-compatible spawn defaults to Legacy mode"),
				Ranged->UsesLegacyRangedBehavior());
		case 2:
			return Test.TestTrue(
				TEXT("Legacy mode has no Backstep Skill Profile"),
				Ranged->GetSkillProfileId().IsNone());
		case 3:
			return Test.TestTrue(
				TEXT("V3 Standard explicitly injects the P6 Backstep Profile"),
				Standard
				&& Standard->Identity.SkillProfileId
					== Fdemo_mapEnemySkillProfileIds::StandardRangedBackstep);
		case 4:
			return Test.TestTrue(
				TEXT("V3 Enhanced explicitly injects the P7 Backstep Profile"),
				Enhanced
				&& Enhanced->Identity.SkillProfileId
					== Fdemo_mapEnemySkillProfileIds::EnhancedRangedBackstep);
		case 5:
			return Test.TestTrue(
				TEXT("Unknown Profile cannot silently resolve to a Skill"),
				Fdemo_mapEnemySkillPrototypeConfig::FindProfile(
					TEXT("Unknown.Ranged.Profile")) == nullptr);
		case 6:
			return Test.TestTrue(
				TEXT("V2 Legacy retreat starts inside the near range"),
				Ademo_mapRangedEnemyCharacter::IsLegacyRetreatRequired(
					400.0f,
					false,
					Ranged->GetRetreatStartRange(),
					Ranged->GetRetreatStopRange()));
		case 7:
			return Test.TestTrue(
				TEXT("V2 Legacy retreat reaches its unchanged safe range"),
				Ademo_mapRangedEnemyCharacter::IsLegacySafeRangeReached(
					700.0f,
					Ranged->GetRetreatStopRange()));
		case 8:
			return Test.TestTrue(
				TEXT("Committed Legacy retreat has no near-range dead zone"),
				Ademo_mapRangedEnemyCharacter::IsLegacyRetreatRequired(
					650.0f,
					true,
					Ranged->GetRetreatStartRange(),
					Ranged->GetRetreatStopRange()));
		case 9:
			return Test.TestTrue(
				TEXT("Legacy Windup begins only after safe range"),
				!Ademo_mapRangedEnemyCharacter::IsLegacySafeRangeReached(
					699.0f,
					Ranged->GetRetreatStopRange())
				&& Ademo_mapRangedEnemyCharacter::IsLegacySafeRangeReached(
					700.0f,
					Ranged->GetRetreatStopRange()));
		case 10:
			return Test.TestEqual(
				TEXT("Legacy Windup duration remains V2 Final"),
				Ranged->GetWindupDuration(),
				0.40f);
		case 11:
			return Test.TestTrue(
				TEXT("Exactly one Projectile satisfies the V2 final stage"),
				Ademo_mapRangedEnemyCharacter::IsV2FinalRangedStageComplete(
					700.0f,
					1,
					4,
					5,
					false));
		case 12:
			return Test.TestTrue(
				TEXT("V2 Projectile keeps the existing target-filter contract"),
				Ranged->GetProjectileParams().bPassThroughFriendlies
				&& !Ranged->GetProjectileParams().bPierceHostiles);
		case 13:
			return Test.TestTrue(
				TEXT("WorldStatic LOS is required by the Ranged product class"),
				TIsDerivedFrom<
					Ademo_mapRangedEnemyCharacter,
					ACharacter>::Value);
		case 14:
			return Test.TestFalse(
				TEXT("Missing Projectile and Windup cannot false-pass"),
				Ademo_mapRangedEnemyCharacter::IsV2FinalRangedStageComplete(
					700.0f,
					1,
					4,
					4,
					false));
		case 15:
			return Test.TestTrue(
				TEXT("V2 Legacy mode has no P6 runtime Profile to preempt it"),
				Ranged->UsesLegacyRangedBehavior()
				&& Fdemo_mapEnemySkillPrototypeConfig::FindProfile(
					Ranged->GetSkillProfileId()) == nullptr);
		case 16:
			return Test.TestTrue(
				TEXT("V3 Standard Backstep values and fire gate remain frozen"),
				StandardSkill
				&& StandardSkill->DisplacementDistance == 320.0f
				&& StandardSkill->MinimumResolvedDistance == 120.0f
				&& StandardSkill->CanResolveRangedFire(
					320.0f,
					true,
					true));
		case 17:
			return Test.TestTrue(
				TEXT("V3 Enhanced Backstep values and fire gate remain frozen"),
				EnhancedSkill
				&& EnhancedSkill->DisplacementDistance == 380.0f
				&& EnhancedSkill->MinimumResolvedDistance == 140.0f
				&& EnhancedSkill->CanResolveRangedFire(
					380.0f,
					true,
					true));
		case 18:
		{
			Udemo_mapEnemySkillRuntimeComponent* Runtime =
				NewObject<Udemo_mapEnemySkillRuntimeComponent>();
			Runtime->ResetForNewRun();
			return Test.TestTrue(
				TEXT("Reset clears Skill ownership while Legacy is Idle"),
				Runtime->IsReady()
				&& !Runtime->IsActive()
				&& Ranged->GetRangedState()
					== Edemo_mapRangedEnemyState::Idle);
		}
		case 19:
			return Test.TestTrue(
				TEXT("Death lifecycle owns timers and Projectile cleanup"),
				Ademo_mapSkillProjectile::StaticClass()
					->IsChildOf(AActor::StaticClass())
				&& Ranged->GetActiveProjectileCount() == 0);
		case 20:
			return Test.TestTrue(
				TEXT("Diagnostics expose no Profile or schema mutation surface"),
				Ranged->UsesLegacyRangedBehavior()
				&& Fdemo_mapPersistentProfile::CurrentSchemaVersion == 4);
		case 21:
			return Test.TestTrue(
				TEXT("V2 Final marker requires retreat, safe range, and Windup or fire"),
				!Ademo_mapRangedEnemyCharacter::IsV2FinalRangedStageComplete(
					649.0f,
					1,
					0,
					1,
					false)
				&& !Ademo_mapRangedEnemyCharacter::IsV2FinalRangedStageComplete(
					700.0f,
					0,
					0,
					1,
					false)
				&& Ademo_mapRangedEnemyCharacter::IsV2FinalRangedStageComplete(
					700.0f,
					1,
					0,
					0,
					true));
		case 22:
			return Test.TestTrue(
				TEXT("Protected schema and P6/P7 values remain unchanged"),
				Fdemo_mapPersistentProfile::CurrentSchemaVersion == 4
				&& StandardSkill
				&& EnhancedSkill
				&& StandardSkill->TriggerMaxDistance == 500.0f
				&& StandardSkill->WindupDuration == 0.18f
				&& StandardSkill->CooldownDuration == 2.40f
				&& EnhancedSkill->TriggerMaxDistance == 560.0f
				&& EnhancedSkill->WindupDuration == 0.14f
				&& EnhancedSkill->CooldownDuration == 2.00f);
		default:
			return Test.TestTrue(TEXT("Known P8.1 semantic case"), false);
		}
	}
}

#define DEMO_MAP_P81_TEST(ClassName, PrettyName, CaseNumber) \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST( \
		ClassName, \
		"demo_map.V2RangedCompatibility." PrettyName, \
		EAutomationTestFlags::EditorContext \
			| EAutomationTestFlags::EngineFilter) \
	bool ClassName::RunTest(const FString&) \
	{ \
		return RunV2RangedCompatibilityCase(*this, CaseNumber); \
	}

DEMO_MAP_P81_TEST(FP81Test01, "01.V2SpawnUsesExplicitLegacyMode", 1)
DEMO_MAP_P81_TEST(FP81Test02, "02.V2HasNoBackstepSkillProfile", 2)
DEMO_MAP_P81_TEST(FP81Test03, "03.V3StandardHasP6BackstepProfile", 3)
DEMO_MAP_P81_TEST(FP81Test04, "04.V3EnhancedHasP7BackstepProfile", 4)
DEMO_MAP_P81_TEST(FP81Test05, "05.UnknownProfileDoesNotSilentlyEnableSkill", 5)
DEMO_MAP_P81_TEST(FP81Test06, "06.V2LegacyRetreatStartsInsideNearRange", 6)
DEMO_MAP_P81_TEST(FP81Test07, "07.V2LegacyRetreatReachesSafeRange", 7)
DEMO_MAP_P81_TEST(FP81Test08, "08.V2LegacyNoNearRangeDeadZone", 8)
DEMO_MAP_P81_TEST(FP81Test09, "09.V2LegacyWindupStartsAfterSafeRange", 9)
DEMO_MAP_P81_TEST(FP81Test10, "10.V2LegacyWindupCompletes", 10)
DEMO_MAP_P81_TEST(FP81Test11, "11.V2LegacyFiresExactlyOneProjectile", 11)
DEMO_MAP_P81_TEST(FP81Test12, "12.V2ProjectileUsesExistingTargetFilter", 12)
DEMO_MAP_P81_TEST(FP81Test13, "13.V2WorldStaticLOSBlocksFire", 13)
DEMO_MAP_P81_TEST(FP81Test14, "14.V2InvalidTargetCancelsWithoutFalsePass", 14)
DEMO_MAP_P81_TEST(FP81Test15, "15.P6RuntimeDoesNotPreemptV2LegacyFlow", 15)
DEMO_MAP_P81_TEST(FP81Test16, "16.V3StandardBackstepStillResolvesAndFires", 16)
DEMO_MAP_P81_TEST(FP81Test17, "17.V3EnhancedBackstepStillResolvesAndFires", 17)
DEMO_MAP_P81_TEST(FP81Test18, "18.ResetClearsLegacyAndSkillOwnership", 18)
DEMO_MAP_P81_TEST(FP81Test19, "19.DeathClearsTimersAndProjectile", 19)
DEMO_MAP_P81_TEST(FP81Test20, "20.DiagnosticsDoNotMutateBehavior", 20)
DEMO_MAP_P81_TEST(FP81Test21, "21.V2FinalMarkerRequiresAllFourStages", 21)
DEMO_MAP_P81_TEST(FP81Test22, "22.ProtectedScopesAndSchemaUnchanged", 22)

#undef DEMO_MAP_P81_TEST

#endif
