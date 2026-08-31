#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapShanmenWeaponGuardFixedTimeline.h"

#include <limits>

namespace
{
	constexpr EAutomationTestFlags GuardTimelineFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid GuardTimelineRunA(
		0xDFD00001, 0xDFD00002, 0xDFD00003, 0xDFD00004);
	const FGuid GuardTimelineRunB(
		0xDFD10001, 0xDFD10002, 0xDFD10003, 0xDFD10004);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardFixedTimelineIdentityTest,
	"Shanmen.0_0_10.Product.WeaponGuardFixedTimeline.IdentityLifecycle",
	GuardTimelineFlags)

bool Fdemo_mapWeaponGuardFixedTimelineIdentityTest::RunTest(const FString&)
{
	Fdemo_mapShanmenWeaponGuardFixedTimeline First;
	Fdemo_mapShanmenWeaponGuardFixedTimeline Replay;
	FString Diagnostic;
	TestTrue(TEXT("fresh timeline is valid and empty"),
		First.IsValid() && First.IsEmpty());
	TestTrue(TEXT("first Run begins"), First.TryBegin(GuardTimelineRunA, Diagnostic));
	TestTrue(TEXT("same Run begin is idempotent"),
		First.TryBegin(GuardTimelineRunA, Diagnostic));
	TestFalse(TEXT("foreign concurrent Run is rejected"),
		First.TryBegin(GuardTimelineRunB, Diagnostic));
	TestTrue(TEXT("replay begins"), Replay.TryBegin(GuardTimelineRunA, Diagnostic));
	TestTrue(TEXT("same Run derives identical timeline identity"),
		First.GetTimelineId().IsValid()
			&& First.GetTimelineId() == Replay.GetTimelineId());
	TestTrue(TEXT("different Run derives different identity"),
		Fdemo_mapShanmenWeaponGuardFixedTimeline::MakeTimelineId(
			GuardTimelineRunA)
			!= Fdemo_mapShanmenWeaponGuardFixedTimeline::MakeTimelineId(
				GuardTimelineRunB));
	TestFalse(TEXT("mismatched teardown is rejected atomically"),
		First.TryEnd(GuardTimelineRunB, Diagnostic));
	TestTrue(TEXT("timeline remains on original Run"),
		First.IsActiveForRun(GuardTimelineRunA));
	TestTrue(TEXT("matching teardown empties timeline"),
		First.TryEnd(GuardTimelineRunA, Diagnostic)
			&& First.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardFixedTimelineCadenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardFixedTimeline.FixedRatePartition",
	GuardTimelineFlags)

bool Fdemo_mapWeaponGuardFixedTimelineCadenceTest::RunTest(const FString&)
{
	Fdemo_mapShanmenWeaponGuardFixedTimeline Fine;
	Fdemo_mapShanmenWeaponGuardFixedTimeline Coarse;
	FString Diagnostic;
	TestTrue(TEXT("fine timeline begins"), Fine.TryBegin(GuardTimelineRunA, Diagnostic));
	TestTrue(TEXT("coarse timeline begins"), Coarse.TryBegin(GuardTimelineRunA, Diagnostic));
	int64 Advanced = 0;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		TestTrue(TEXT("1/60 delta advances safely"),
			Fine.TryAdvance(1.0 / 60.0, Advanced, Diagnostic));
	}
	TestTrue(TEXT("coarse partition advances safely"),
		Coarse.TryAdvance(0.1, Advanced, Diagnostic)
			&& Coarse.TryAdvance(1.0 / 15.0, Advanced, Diagnostic));
	TestEqual(TEXT("five 30 Hz ticks represent ten 60 Hz deltas"),
		Fine.GetCurrentTick(), static_cast<int64>(5));
	TestEqual(TEXT("different frame partition reaches same fixed tick"),
		Coarse.GetCurrentTick(), Fine.GetCurrentTick());
	TestEqual(TEXT("canonical timeline rate is explicit"),
		Fdemo_mapShanmenWeaponGuardFixedTimeline::CanonicalTicksPerSecond(),
		static_cast<int64>(30));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardFixedTimelineCarryTest,
	"Shanmen.0_0_10.Product.WeaponGuardFixedTimeline.SubTickCarry",
	GuardTimelineFlags)

bool Fdemo_mapWeaponGuardFixedTimelineCarryTest::RunTest(const FString&)
{
	Fdemo_mapShanmenWeaponGuardFixedTimeline Timeline;
	FString Diagnostic;
	int64 Advanced = INDEX_NONE;
	TestTrue(TEXT("timeline begins"), Timeline.TryBegin(GuardTimelineRunA, Diagnostic));
	TestTrue(TEXT("first sub-tick delta is retained"),
		Timeline.TryAdvance(0.02, Advanced, Diagnostic));
	TestEqual(TEXT("first sub-tick emits zero whole ticks"),
		Advanced, static_cast<int64>(0));
	TestTrue(TEXT("second sub-tick crosses exactly one boundary"),
		Timeline.TryAdvance(0.02, Advanced, Diagnostic));
	TestEqual(TEXT("second advance emits one tick"),
		Advanced, static_cast<int64>(1));
	TestEqual(TEXT("current tick remains monotonic"),
		Timeline.GetCurrentTick(), static_cast<int64>(1));
	TestTrue(TEXT("remainder stays inside one 30 Hz tick"),
		Timeline.GetSubTickSeconds() > 0.0
			&& Timeline.GetSubTickSeconds() < 1.0 / 30.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardFixedTimelineInvalidDeltaTest,
	"Shanmen.0_0_10.Product.WeaponGuardFixedTimeline.InvalidDeltaAtomic",
	GuardTimelineFlags)

bool Fdemo_mapWeaponGuardFixedTimelineInvalidDeltaTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenWeaponGuardFixedTimeline Timeline;
	FString Diagnostic;
	int64 Advanced = INDEX_NONE;
	TestTrue(TEXT("timeline begins"), Timeline.TryBegin(GuardTimelineRunA, Diagnostic));
	TestFalse(TEXT("negative delta is rejected"),
		Timeline.TryAdvance(-0.1, Advanced, Diagnostic));
	TestFalse(TEXT("NaN delta is rejected"),
		Timeline.TryAdvance(
			std::numeric_limits<double>::quiet_NaN(),
			Advanced,
			Diagnostic));
	TestFalse(TEXT("infinite delta is rejected"),
		Timeline.TryAdvance(
			std::numeric_limits<double>::infinity(),
			Advanced,
			Diagnostic));
	TestTrue(TEXT("rejections preserve tick and structural validity"),
		Timeline.IsValid()
			&& Timeline.GetCurrentTick() == 0
			&& Timeline.GetSubTickSeconds() == 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardFixedTimelineSampleTest,
	"Shanmen.0_0_10.Product.WeaponGuardFixedTimeline.OpaqueSample",
	GuardTimelineFlags)

bool Fdemo_mapWeaponGuardFixedTimelineSampleTest::RunTest(const FString&)
{
	Fdemo_mapShanmenWeaponGuardFixedTimeline Timeline;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample Missing;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample Start;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample AdvancedSample;
	FString Diagnostic;
	int64 Advanced = 0;
	TestFalse(TEXT("empty timeline cannot produce a sample"),
		Timeline.TryCapture(Missing));
	TestTrue(TEXT("timeline begins"), Timeline.TryBegin(GuardTimelineRunA, Diagnostic));
	TestTrue(TEXT("tick-zero sample is valid"), Timeline.TryCapture(Start));
	TestTrue(TEXT("five ticks advance"),
		Timeline.TryAdvance(5.0 / 30.0, Advanced, Diagnostic)
			&& Advanced == 5);
	TestTrue(TEXT("advanced sample is valid"),
		Timeline.TryCapture(AdvancedSample));
	TestTrue(TEXT("samples retain one deterministic timeline identity"),
		Start.GetTimelineId() == AdvancedSample.GetTimelineId()
			&& Start.GetActiveStartTick() == 0
			&& AdvancedSample.GetActiveStartTick() == 5);
	return true;
}

#endif
