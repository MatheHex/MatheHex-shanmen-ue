#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPolicy.h"

#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	using EMode =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode;
	using FPlan =
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan;

	constexpr EAutomationTestFlags LayoutFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponMainHUDCombatHintStandardLayoutTest,
	"Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy.Standard",
	LayoutFlags)

bool Fdemo_mapThrownWeaponMainHUDCombatHintStandardLayoutTest::RunTest(
	const FString&)
{
	FPlan Plan;
	TestTrue(TEXT("a normal viewport admits terminal feedback above the P20.65 stack"),
		FPlan::TryPlan(FVector2D(1920.0, 1080.0), 6, Plan));
	FVector2D Bottom;
	FVector2D Top;
	TestTrue(TEXT("all six lines receive deterministic positions"),
		Plan.TryGetLinePosition(0, Bottom)
			&& Plan.TryGetLinePosition(5, Top));
	TestTrue(TEXT("standard values preserve anchor spacing and scale"),
		Plan.IsValid()
			&& Plan.GetMode() == EMode::Standard
			&& !Plan.IsCompact()
			&& Plan.GetLineCount() == 6
			&& Plan.GetLeftMargin() == 28.0
			&& Plan.GetBottomAnchor() == 112.0
			&& Plan.GetLineSpacing() == 22.5
			&& Plan.GetScaleMultiplier() == 1.0
			&& Bottom == FVector2D(28.0, 968.0)
			&& Top == FVector2D(28.0, 855.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponMainHUDCombatHintCompactLayoutTest,
	"Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy.Compact",
	LayoutFlags)

bool Fdemo_mapThrownWeaponMainHUDCombatHintCompactLayoutTest::RunTest(
	const FString&)
{
	FPlan Compact;
	FPlan Replay;
	TestTrue(TEXT("the minimum complete six-line viewport is compact"),
		FPlan::TryPlan(FVector2D(640.0, 216.0), 6, Compact)
			&& FPlan::TryPlan(FVector2D(640.0, 216.0), 6, Replay));
	FVector2D Bottom;
	FVector2D Top;
	TestTrue(TEXT("compact layout retains every requested line"),
		Compact.TryGetLinePosition(0, Bottom)
			&& Compact.TryGetLinePosition(5, Top));
	TestTrue(TEXT("compact replay and safe margins are exact"),
		Compact.IsCompact()
			&& Compact.Matches(Replay)
			&& Compact.GetLeftMargin() == 16.0
			&& Compact.GetLineSpacing() == 16.0
			&& Compact.GetScaleMultiplier() == 0.86
			&& Bottom == FVector2D(16.0, 104.0)
			&& Top == FVector2D(16.0, 24.0));

	FPlan HeightCompact;
	TestTrue(TEXT("height pressure alone selects compact mode"),
		FPlan::TryPlan(FVector2D(1920.0, 220.0), 6, HeightCompact)
			&& HeightCompact.IsCompact()
			&& FMath::IsNearlyEqual(
				HeightCompact.GetLineSpacing(), 16.8));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponMainHUDCombatHintLayoutFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponMainHUDCombatHintLayoutPolicy.Fences",
	LayoutFlags)

bool Fdemo_mapThrownWeaponMainHUDCombatHintLayoutFenceTest::RunTest(
	const FString&)
{
	FPlan Reused;
	check(FPlan::TryPlan(FVector2D(1920.0, 1080.0), 5, Reused));
	TestFalse(TEXT("narrow viewport fails rather than clipping text"),
		FPlan::TryPlan(FVector2D(639.0, 1080.0), 5, Reused));
	TestTrue(TEXT("failed planning clears reusable output"),
		!Reused.IsValid() && Reused.GetLineCount() == 0);
	TestFalse(TEXT("insufficient height rejects an incomplete six-line stack"),
		FPlan::TryPlan(FVector2D(640.0, 215.0), 6, Reused));
	TestFalse(TEXT("empty and oversized stacks are rejected"),
		FPlan::TryPlan(FVector2D(1920.0, 1080.0), 0, Reused)
			|| FPlan::TryPlan(FVector2D(1920.0, 1080.0), 7, Reused));
	TestFalse(TEXT("non-finite viewport input is rejected"),
		FPlan::TryPlan(
			FVector2D(
				std::numeric_limits<double>::quiet_NaN(),
				1080.0),
			5,
			Reused));
	FVector2D InvalidPosition(99.0, 99.0);
	FPlan Valid;
	check(FPlan::TryPlan(FVector2D(1920.0, 1080.0), 6, Valid));
	TestFalse(TEXT("out-of-range line access fails closed"),
		Valid.TryGetLinePosition(6, InvalidPosition));
	TestTrue(TEXT("failed position output is canonical zero"),
		InvalidPosition.IsZero());
	return true;
}

#endif
