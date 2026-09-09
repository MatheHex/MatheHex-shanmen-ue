#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseHUDPresentation.h"

#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	using EPlacement =
		Edemo_mapShanmenDivineSenseMarkerPlacement;
	using FPlan = Fdemo_mapShanmenDivineSenseHUDMarkerPlan;

	constexpr EAutomationTestFlags PresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDProjectedMarkerTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.Projected",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDProjectedMarkerTest::RunTest(const FString&)
{
	FPlan Plan;
	FPlan Replay;
	TestTrue(TEXT("a readable world projection remains at its exact pixel"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			true,
			FVector2D(810.0, 420.0),
			FVector2D(99.0, 99.0),
			Plan)
			&& FPlan::TryPlan(
				FVector2D(1920.0, 1080.0),
				true,
				FVector2D(810.0, 420.0),
				FVector2D(-99.0, -99.0),
				Replay));
	TestTrue(TEXT("on-screen placement ignores fallback bearing deterministically"),
		Plan.IsValid()
			&& Plan.Matches(Replay)
			&& Plan.GetPlacement() == EPlacement::WorldProjected
			&& !Plan.IsAtScreenEdge()
			&& Plan.GetScreenPosition() == FVector2D(810.0, 420.0)
			&& Plan.GetEdgeDirection().IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDProjectedEdgeMarkerTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.ProjectedEdge",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDProjectedEdgeMarkerTest::RunTest(
	const FString&)
{
	FPlan Plan;
	TestTrue(TEXT("a projected target outside the viewport clamps to the safe edge"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			true,
			FVector2D(2500.0, 500.0),
			FVector2D(-1.0, 0.0),
			Plan));
	TestTrue(TEXT("projection displacement wins and points toward screen right"),
		Plan.IsAtScreenEdge()
			&& Plan.GetPlacement() == EPlacement::ScreenEdge
			&& FMath::IsNearlyEqual(
				Plan.GetScreenPosition().X, 1884.0)
			&& Plan.GetScreenPosition().Y >= FPlan::GetTopSafeMargin()
			&& Plan.GetScreenPosition().Y
				<= 1080.0 - FPlan::GetBottomSafeMargin()
			&& Plan.GetEdgeDirection().X > 0.99
			&& Plan.GetEdgeDirection().Y < 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDBehindCameraMarkerTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.BehindCamera",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDBehindCameraMarkerTest::RunTest(
	const FString&)
{
	FPlan Plan;
	TestTrue(TEXT("a failed projection uses the camera-relative rear bearing"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			false,
			FVector2D::ZeroVector,
			FVector2D(0.0, 1.0),
			Plan));
	TestTrue(TEXT("a subject directly behind the camera points to bottom center"),
		Plan.IsAtScreenEdge()
			&& Plan.GetScreenPosition() == FVector2D(960.0, 1024.0)
			&& Plan.GetEdgeDirection() == FVector2D(0.0, 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDPresentationFenceTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.Fences",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDPresentationFenceTest::RunTest(
	const FString&)
{
	FPlan Reused;
	check(FPlan::TryPlan(
		FVector2D(1920.0, 1080.0),
		true,
		FVector2D(810.0, 420.0),
		FVector2D::ZeroVector,
		Reused));
	TestFalse(TEXT("a too-small viewport is rejected"),
		FPlan::TryPlan(
			FVector2D(319.0, 1080.0),
			true,
			FVector2D(100.0, 200.0),
			FVector2D::ZeroVector,
			Reused));
	TestTrue(TEXT("failed planning clears reusable output"),
		!Reused.IsValid()
			&& Reused.GetPlacement() == EPlacement::Invalid);
	TestFalse(TEXT("failed projection without a bearing fails closed"),
		FPlan::TryPlan(
			FVector2D(1920.0, 1080.0),
			false,
			FVector2D(900.0, 500.0),
			FVector2D::ZeroVector,
			Reused));
	TestFalse(TEXT("non-finite canvas and bearing inputs are rejected"),
		FPlan::TryPlan(
			FVector2D(
				std::numeric_limits<double>::quiet_NaN(),
				1080.0),
			false,
			FVector2D::ZeroVector,
			FVector2D(0.0, 1.0),
			Reused)
			|| FPlan::TryPlan(
				FVector2D(1920.0, 1080.0),
				false,
				FVector2D::ZeroVector,
				FVector2D(
					std::numeric_limits<double>::infinity(),
					1.0),
				Reused));
	return true;
}

#endif
