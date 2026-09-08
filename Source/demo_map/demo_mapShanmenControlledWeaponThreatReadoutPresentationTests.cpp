#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponThreatReadoutPresentation.h"

#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	using EPlacement =
		Edemo_mapShanmenControlledWeaponThreatReadoutPlacement;
	using FPlan =
		Fdemo_mapShanmenControlledWeaponThreatReadoutPlan;
	using FStyle =
		Fdemo_mapShanmenControlledWeaponThreatReadoutStyle;
	using EFlightPhase =
		Edemo_mapShanmenControlledWeaponFlightPhase;

	constexpr EAutomationTestFlags PresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponThreatReadoutWorldTrackedTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation.WorldTracked",
	PresentationFlags)

bool Fdemo_mapControlledWeaponThreatReadoutWorldTrackedTest::RunTest(
	const FString&)
{
	FPlan Plan;
	FPlan Replay;
	const FStyle Style;
	TestTrue(TEXT("an on-screen flying sword receives a tracked plan"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Orbiting,
			3,
			true,
			FVector2D(960.0, 540.0),
			Style,
			Plan)
		&& FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Orbiting,
			3,
			true,
			FVector2D(960.0, 540.0),
			Style,
			Replay));
	TestTrue(TEXT("tracked layout text and geometry are deterministic"),
		Plan.IsValid()
			&& Plan.Matches(Replay)
			&& Plan.GetPlacement() == EPlacement::WorldTracked
			&& !Plan.IsViewportFallback()
			&& Plan.GetPhase() == EFlightPhase::Orbiting
			&& Plan.GetContactCount() == 3
			&& Plan.GetText() == TEXT("飞剑 · 环绕待命 · 近身目标 3")
			&& Plan.GetPanelPosition() == FVector2D(845.0, 494.0)
			&& Plan.GetPanelSize() == FVector2D(230.0, 30.0)
			&& Plan.GetTextPosition() == FVector2D(855.0, 500.0)
			&& Plan.GetTextScale() == 0.82);

	FPlan Edge;
	TestTrue(TEXT("an on-screen edge anchor clamps without becoming fallback"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Redeployed,
			1,
			true,
			FVector2D(0.0, 0.0),
			Style,
			Edge)
			&& Edge.GetPlacement() == EPlacement::WorldTracked
			&& Edge.GetPanelPosition() == FVector2D(8.0, 54.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponThreatReadoutViewportFallbackTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation.ViewportFallback",
	PresentationFlags)

bool Fdemo_mapControlledWeaponThreatReadoutViewportFallbackTest::RunTest(
	const FString&)
{
	const FStyle Style;
	FPlan ProjectionFailed;
	FPlan OutsideViewport;
	FPlan NonFiniteProjection;
	TestTrue(TEXT("a failed projection resolves to a stable viewport fallback"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Returning,
			0,
			false,
			FVector2D::ZeroVector,
			Style,
			ProjectionFailed));
	TestTrue(TEXT("outside and non-finite projections use the same fallback"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Returning,
			0,
			true,
			FVector2D(1921.0, 540.0),
			Style,
			OutsideViewport)
		&& FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Returning,
			0,
			true,
			FVector2D(
				std::numeric_limits<double>::quiet_NaN(),
				540.0),
			Style,
			NonFiniteProjection));
	TestTrue(TEXT("fallback remains readable and deterministic"),
		ProjectionFailed.IsViewportFallback()
			&& ProjectionFailed.Matches(OutsideViewport)
			&& ProjectionFailed.Matches(NonFiniteProjection)
			&& ProjectionFailed.GetText()
				== TEXT("飞剑 · 返航 · 屏外")
			&& ProjectionFailed.GetPhase() == EFlightPhase::Returning
			&& ProjectionFailed.GetContactCount() == 0
			&& ProjectionFailed.GetPanelPosition()
				== FVector2D(845.0, 54.0));

	FStyle CustomStyle;
	CustomStyle.PanelSize = FVector2D(260.0, 36.0);
	CustomStyle.TextInset = FVector2D(12.0, 8.0);
	CustomStyle.WorldVerticalLift = 52.0;
	CustomStyle.HorizontalMargin = 20.0;
	CustomStyle.TopMargin = 64.0;
	CustomStyle.BottomMargin = 100.0;
	CustomStyle.TextScale = 0.9;
	CustomStyle.PanelColor = FLinearColor(0.1f, 0.2f, 0.3f, 0.8f);
	CustomStyle.TextColor = FLinearColor(0.8f, 0.9f, 1.0f);
	FPlan Custom;
	TestTrue(TEXT("validated style values configure the resolved plan"),
		FPlan::TryPlan(
			FVector2D(1280.0, 720.0),
			EFlightPhase::Directed,
			4,
			true,
			FVector2D(640.0, 360.0),
			CustomStyle,
			Custom)
			&& Custom.GetPanelPosition() == FVector2D(510.0, 308.0)
			&& Custom.GetPanelSize() == FVector2D(260.0, 36.0)
			&& Custom.GetTextPosition() == FVector2D(522.0, 316.0)
			&& Custom.GetTextScale() == 0.9
			&& Custom.GetPhase() == EFlightPhase::Directed
			&& Custom.GetText()
				== TEXT("飞剑 · 御剑出击 · 近身目标 4")
			&& Custom.GetPanelColor() == CustomStyle.PanelColor
			&& Custom.GetTextColor() == CustomStyle.TextColor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponThreatReadoutFenceTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation.Fences",
	PresentationFlags)

bool Fdemo_mapControlledWeaponThreatReadoutFenceTest::RunTest(
	const FString&)
{
	const FStyle Style;
	FPlan Reused;
	check(FPlan::TryPlan(
		FVector2D(1920.0, 1080.0),
		EFlightPhase::Orbiting,
		1,
		true,
		FVector2D(960.0, 540.0),
		Style,
		Reused));
	TestTrue(TEXT("zero contacts retain the authoritative flight phase"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Orbiting,
			0,
			true,
			FVector2D(960.0, 540.0),
			Style,
			Reused)
		&& Reused.IsValid()
		&& Reused.GetContactCount() == 0
		&& Reused.GetText() == TEXT("飞剑 · 环绕待命"));
	TestFalse(TEXT("negative contact evidence fails closed"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Orbiting,
			-1,
			true,
			FVector2D(960.0, 540.0),
			Style,
			Reused));
	TestTrue(TEXT("failed planning clears reusable output"),
		!Reused.IsValid()
			&& Reused.GetPlacement() == EPlacement::Invalid
			&& Reused.GetPhase() == EFlightPhase::Invalid
			&& Reused.GetContactCount() == 0
			&& Reused.GetText().IsEmpty());

	TestFalse(TEXT("invalid product phase cannot fabricate presentation"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Invalid,
			0,
			true,
			FVector2D(960.0, 540.0),
			Style,
			Reused));

	TestFalse(TEXT("undersized canvas fails closed"),
		FPlan::TryPlan(
			FVector2D(245.0, 1080.0),
			EFlightPhase::Returning,
			1,
			false,
			FVector2D::ZeroVector,
			Style,
			Reused));
	FStyle InvalidStyle = Style;
	InvalidStyle.PanelSize.X = 0.0;
	TestFalse(TEXT("invalid visual configuration is rejected"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Orbiting,
			1,
			true,
			FVector2D(960.0, 540.0),
			InvalidStyle,
			Reused));
	InvalidStyle = Style;
	InvalidStyle.TextScale =
		std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("non-finite configuration is rejected"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			EFlightPhase::Orbiting,
			1,
			true,
			FVector2D(960.0, 540.0),
			InvalidStyle,
			Reused));
	return true;
}

#endif
