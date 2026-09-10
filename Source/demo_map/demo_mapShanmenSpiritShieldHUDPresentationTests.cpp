#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSpiritShieldHUDPresentation.h"

#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	using EShieldTone = Edemo_mapShanmenSpiritShieldHUDTone;
	using FShieldPresentation =
		Fdemo_mapShanmenSpiritShieldHUDPresentation;

	constexpr EAutomationTestFlags PresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDStableTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.Stable",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDStableTest::RunTest(const FString&)
{
	FShieldPresentation Presentation;
	FShieldPresentation Replay;
	TestTrue(TEXT("an active authoritative snapshot produces stable HUD copy"),
		FShieldPresentation::TryProject(
			true, 30.0f, 30.0f, 100, 190, 30, TEXT("H"), Presentation)
			&& FShieldPresentation::TryProject(
				true, 30.0f, 30.0f, 100, 190, 30, TEXT("H"), Replay));
	TestTrue(TEXT("the stable projection is deterministic and exact"),
		Presentation.IsValid()
			&& Presentation.Matches(Replay)
			&& Presentation.GetTone() == EShieldTone::Stable
			&& Presentation.GetAvailableCapacity() == 30.0f
			&& Presentation.GetMaximumCapacity() == 30.0f
			&& Presentation.GetRemainingTicks() == 90
			&& Presentation.GetRemainingSeconds() == 3.0
			&& Presentation.GetDisplayText()
				== TEXT("SPIRIT SHIELD  30 / 30  ·  3.0s  ·  [H]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDLowTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.Low",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDLowTest::RunTest(const FString&)
{
	FShieldPresentation Presentation;
	TestTrue(TEXT("a quarter-capacity shield uses the low-capacity warning"),
		FShieldPresentation::TryProject(
			true, 7.5f, 30.0f, 129, 190, 30, TEXT(" H "), Presentation));
	TestTrue(TEXT("fractional capacity and sub-tick duration remain readable"),
		Presentation.GetTone() == EShieldTone::Low
			&& Presentation.GetRemainingTicks() == 61
			&& FMath::IsNearlyEqual(
				Presentation.GetRemainingSeconds(), 61.0 / 30.0)
			&& Presentation.GetDisplayText()
				== TEXT("SPIRIT SHIELD LOW  7.5 / 30  ·  2.1s  ·  [H]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDDepletedTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.Depleted",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDDepletedTest::RunTest(const FString&)
{
	FShieldPresentation Presentation;
	TestTrue(TEXT("an active depleted session remains visible until deadline"),
		FShieldPresentation::TryProject(
			true, 0.0f, 30.0f, 189, 190, 30, TEXT("H"), Presentation));
	TestTrue(TEXT("one remaining tick never renders as zero seconds"),
		Presentation.GetTone() == EShieldTone::Depleted
			&& Presentation.GetRemainingTicks() == 1
			&& Presentation.GetDisplayText()
				== TEXT(
					"SPIRIT SHIELD DEPLETED  0 / 30  ·  0.1s  ·  [H]"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDFencesTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.Fences",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDFencesTest::RunTest(const FString&)
{
	FShieldPresentation Reused;
	check(FShieldPresentation::TryProject(
		true, 15.0f, 30.0f, 100, 190, 30, TEXT("H"), Reused));
	TestFalse(TEXT("inactive sessions remain absent from the HUD"),
		FShieldPresentation::TryProject(
			false, 15.0f, 30.0f, 100, 190, 30, TEXT("H"), Reused));
	TestFalse(TEXT("failed projection clears reusable output"), Reused.IsValid());
	TestFalse(TEXT("expired and malformed timeline reads fail closed"),
		FShieldPresentation::TryProject(
			true, 15.0f, 30.0f, 190, 190, 30, TEXT("H"), Reused)
			|| FShieldPresentation::TryProject(
				true, 15.0f, 30.0f, 100, 190, 0, TEXT("H"), Reused));
	TestFalse(TEXT("invalid capacity reads and blank keys fail closed"),
		FShieldPresentation::TryProject(
			true, 31.0f, 30.0f, 100, 190, 30, TEXT("H"), Reused)
			|| FShieldPresentation::TryProject(
				true,
				std::numeric_limits<float>::quiet_NaN(),
				30.0f,
				100,
				190,
				30,
				TEXT("H"),
				Reused)
			|| FShieldPresentation::TryProject(
				true, 15.0f, 30.0f, 100, 190, 30, TEXT("  "), Reused));
	return true;
}

#endif
