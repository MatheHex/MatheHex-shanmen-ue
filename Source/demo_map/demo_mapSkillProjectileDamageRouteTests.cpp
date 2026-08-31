#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapSkillProjectile.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSkillProjectileDamageRouteCanonicalPrecedenceTest,
	"Shanmen.0_0_10.Product.SkillProjectileDamageRoute.CanonicalPrecedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSkillProjectileDamageRouteCanonicalPrecedenceTest::RunTest(
	const FString&)
{
	for (const bool bM01EnemyAttackProductPath : {false, true})
	{
		for (const bool bTargetHasPlayerVitality : {false, true})
		{
			TestEqual(
				TEXT("A selected canonical route always owns delivery"),
				Fdemo_mapSkillProjectileDamageRoutePolicy::Resolve(
					true,
					bM01EnemyAttackProductPath,
					bTargetHasPlayerVitality),
				Edemo_mapSkillProjectileDamageRoute::CanonicalProduct);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSkillProjectileDamageRouteM01HostilePlayerFailClosedTest,
	"Shanmen.0_0_10.Product.SkillProjectileDamageRoute.M01HostilePlayerFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSkillProjectileDamageRouteM01HostilePlayerFailClosedTest::RunTest(
	const FString&)
{
	TestEqual(
		TEXT("An unregistered M01 hostile source cannot bypass canonical player defense"),
		Fdemo_mapSkillProjectileDamageRoutePolicy::Resolve(
			false,
			true,
			true),
		Edemo_mapSkillProjectileDamageRoute::
			RejectedUnregisteredM01HostilePlayer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSkillProjectileDamageRouteCompatibilityBoundaryTest,
	"Shanmen.0_0_10.Product.SkillProjectileDamageRoute.CompatibilityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSkillProjectileDamageRouteCompatibilityBoundaryTest::RunTest(
	const FString&)
{
	TestEqual(
		TEXT("M01 non-player targets retain legacy compatibility"),
		Fdemo_mapSkillProjectileDamageRoutePolicy::Resolve(false, true, false),
		Edemo_mapSkillProjectileDamageRoute::LegacyCompatibility);
	TestEqual(
		TEXT("Non-M01 player targets retain legacy compatibility"),
		Fdemo_mapSkillProjectileDamageRoutePolicy::Resolve(false, false, true),
		Edemo_mapSkillProjectileDamageRoute::LegacyCompatibility);
	TestEqual(
		TEXT("Non-M01 non-player targets retain legacy compatibility"),
		Fdemo_mapSkillProjectileDamageRoutePolicy::Resolve(false, false, false),
		Edemo_mapSkillProjectileDamageRoute::LegacyCompatibility);
	return true;
}

#endif
