#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ShanmenDemo20Session.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LifecycleTest, "Shanmen.Demo20.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemo20LifecycleTest::RunTest(const FString&)
{
	FShanmenDemo20Session Session;
	const FGuid Run(20,1,1,1);
	TestFalse(TEXT("No attack before Begin"), Session.StrikeSentinel(0));
	TestFalse(TEXT("No invalid identity"), Session.Begin(FGuid()));
	TestTrue(TEXT("Fresh session"), Session.Begin(Run));
	TestFalse(TEXT("Active start cannot reset health"), Session.Begin(FGuid(20,2,1,1)));
	TestFalse(TEXT("Early exit rejected"), Session.TryExtract());
	TestFalse(TEXT("Invalid target rejected"), Session.StrikeSentinel(3));
	for (int32 Target = 0; Target < 3; ++Target)
	{
		for (int32 Hit = 0; Hit < 3; ++Hit)
		{
			Session.Advance(.4f);
			TestTrue(TEXT("Canonical sword resolves and commits"), Session.StrikeSentinel(Target));
			TestFalse(TEXT("Repeat input cannot bypass recovery"), Session.StrikeSentinel(Target));
		}
		TestEqual(TEXT("Exactly three hits defeat 78 HP sentinel"), Session.GetHealth(Target + 1), 0.f);
		TestFalse(TEXT("Dead sentinel cannot strike"), Session.ReceiveSentinelStrike(Target));
	}
	TestEqual(TEXT("Nine committed canonical impacts"), Session.GetImpactCount(), 9);
	TestTrue(TEXT("Exit after objectives"), Session.TryExtract());
	TestFalse(TEXT("Duplicate exit rejected"), Session.TryExtract());
	TestFalse(TEXT("Terminal cannot deal damage"), Session.ReceiveSentinelStrike(0));
	TestTrue(TEXT("Return to preparation"), Session.ReturnToPreparation());
	TestEqual(TEXT("Run-local ledgers released"), Session.GetImpactCount(), 0);
	TestFalse(TEXT("Cannot recycle previous Run ID"), Session.Begin(Run));
	TestTrue(TEXT("New independent session"), Session.Begin(FGuid(20,2,1,1)));
	TestEqual(TEXT("Health reset only for new Run"), Session.GetHealth(), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20DefenseTest, "Shanmen.Demo20.Defense", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemo20DefenseTest::RunTest(const FString&)
{
	FShanmenDemo20Session Session;
	TestTrue(TEXT("Begin"), Session.Begin(FGuid(20,3,1,1)));
	TestTrue(TEXT("Unguarded canonical damage"), Session.ReceiveSentinelStrike(0));
	TestEqual(TEXT("18 damage"), Session.GetHealth(), 82.f);
	Session.SetGuarding(true);
	TestFalse(TEXT("Guard and attack are mutually exclusive"), Session.StrikeSentinel(0));
	TestTrue(TEXT("Guard uses defense layer"), Session.ReceiveSentinelStrike(0));
	TestEqual(TEXT("75 percent prevention"), Session.GetHealth(), 77.5f);
	TestTrue(TEXT("Evade cancels guard"), Session.TryEvade());
	TestFalse(TEXT("Guard cleared"), Session.IsGuarding());
	TestTrue(TEXT("Evasion canonical receipt"), Session.ReceiveSentinelStrike(0));
	TestEqual(TEXT("Avoidance preserves nonzero baseline"), Session.GetHealth(), 77.5f);
	TestFalse(TEXT("Cannot spam evade"), Session.TryEvade());
	Session.Advance(-1.f);
	TestTrue(TEXT("Invalid clock cannot close evade"), Session.IsEvading());
	Session.Advance(.4f);
	TestFalse(TEXT("Evade window ended"), Session.IsEvading());
	Session.ReceiveSentinelStrike(0);
	TestEqual(TEXT("Damage resumes after window"), Session.GetHealth(), 59.5f);
	Session.Advance(1.1f);
	TestTrue(TEXT("Cooldown recovered"), Session.TryEvade());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20TerminalTest, "Shanmen.Demo20.Terminal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDemo20TerminalTest::RunTest(const FString&)
{
	FShanmenDemo20Session Session;
	Session.Begin(FGuid(20,4,1,1));
	for (int32 Hit = 0; Hit < 6; ++Hit) TestTrue(TEXT("Receive lethal sequence"), Session.ReceiveSentinelStrike(0));
	TestTrue(TEXT("Defeated state"), Session.GetPhase() == EShanmenDemo20Phase::Defeated);
	TestEqual(TEXT("Health clamped by existing authority"), Session.GetHealth(), 0.f);
	TestFalse(TEXT("Dead player cannot attack"), Session.StrikeSentinel(0));
	TestFalse(TEXT("Dead player cannot evade"), Session.TryEvade());
	TestFalse(TEXT("Dead player cannot extract"), Session.TryExtract());
	Session.Advance(20.f);
	TestEqual(TEXT("Terminal clock stops"), Session.GetElapsed(), 0.f);
	Session.ReturnToPreparation();
	Session.Begin(FGuid(20,5,1,1));
	Session.Advance(2.f);
	Session.Abandon();
	TestTrue(TEXT("Abandon distinct from successful exit"), Session.GetPhase() == EShanmenDemo20Phase::Abandoned);
	TestTrue(TEXT("Can return after abandon"), Session.ReturnToPreparation());
	return true;
}
#endif
