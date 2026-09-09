#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseHUDPresentation.h"

#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	using EPlacement =
		Edemo_mapShanmenDivineSenseMarkerPlacement;
	using EFeedbackReason =
		Edemo_mapShanmenDivineSenseHUDFeedbackReason;
	using EFeedbackTone =
		Edemo_mapShanmenDivineSenseHUDFeedbackTone;
	using FPlan = Fdemo_mapShanmenDivineSenseHUDMarkerPlan;
	using FFeedback =
		Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation;
	using FTacticalSummary =
		Fdemo_mapShanmenDivineSenseHUDTacticalSummary;

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDUnavailableFeedbackTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.UnavailableFeedback",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDUnavailableFeedbackTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenDivineSenseLogicalInputResult Unavailable;
	Unavailable.Status =
		Edemo_mapShanmenDivineSenseLogicalInputStatus::AdapterInactive;
	Unavailable.Diagnostic =
		TEXT("Divine Sense physical input requires the product GameMode.");
	FFeedback Presentation;
	FFeedback Replay;
	TestTrue(TEXT("a structured inactive result becomes readable HUD feedback"),
		Unavailable.IsValid()
			&& FFeedback::TryProject(Unavailable, TEXT("V"), Presentation)
			&& FFeedback::TryProject(Unavailable, TEXT("Z"), Replay));
	TestTrue(TEXT("unavailable feedback is concise stable and warning-toned"),
		Presentation.IsValid()
			&& Presentation.Matches(Replay)
			&& Presentation.GetReason() == EFeedbackReason::Unavailable
			&& Presentation.GetTone() == EFeedbackTone::Warning
			&& Presentation.GetDisplayText()
				== TEXT("DIVINE SENSE · UNAVAILABLE"));

	const Fdemo_mapShanmenDivineSenseLogicalInputResult Invalid;
	TestFalse(TEXT("invalid evidence cannot reuse stale HUD feedback"),
		FFeedback::TryProject(Invalid, TEXT("V"), Presentation));
	TestFalse(TEXT("failed projection clears reusable output"),
		Presentation.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDTacticalContactsTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.TacticalContacts",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDTacticalContactsTest::RunTest(const FString&)
{
	FTacticalSummary Summary;
	FTacticalSummary Replay;
	TestTrue(TEXT("accepted scan values produce a tactical contact summary"),
		FTacticalSummary::TryProject(3, 2, 14.26, 70.0f, 100.0f, Summary)
			&& FTacticalSummary::TryProject(
				3, 2, 14.26, 70.0f, 100.0f, Replay));
	TestTrue(TEXT("contact summary exposes hidden count nearest range and spirit"),
		Summary.IsValid()
			&& Summary.Matches(Replay)
			&& !Summary.IsAreaClear()
			&& Summary.GetContactCount() == 3
			&& Summary.GetOccludedContactCount() == 2
			&& FMath::IsNearlyEqual(
				Summary.GetNearestDistanceMeters(), 14.26)
			&& Summary.GetPrimaryText()
				== TEXT("DIVINE SENSE · 3 CONTACTS · 2 OCCLUDED")
			&& Summary.GetSecondaryText()
				== TEXT("NEAREST 14.3m · SPIRIT 70 / 100"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDTacticalAreaClearTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.TacticalAreaClear",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDTacticalAreaClearTest::RunTest(const FString&)
{
	FTacticalSummary Summary;
	TestTrue(TEXT("an empty accepted scan explicitly reports a clear area"),
		FTacticalSummary::TryProject(0, 0, 0.0, 90.0f, 100.0f, Summary));
	TestTrue(TEXT("area-clear summary retains the authoritative spirit amount"),
		Summary.IsAreaClear()
			&& Summary.GetPrimaryText() == TEXT("DIVINE SENSE · AREA CLEAR")
			&& Summary.GetSecondaryText() == TEXT("SPIRIT 90 / 100"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseHUDTacticalSummaryFenceTest,
	"Shanmen.0_0_10.Product.DivineSenseHUDPresentation.TacticalSummaryFences",
	PresentationFlags)

bool Fdemo_mapDivineSenseHUDTacticalSummaryFenceTest::RunTest(
	const FString&)
{
	FTacticalSummary Reused;
	check(FTacticalSummary::TryProject(
		1, 0, 5.0, 90.0f, 100.0f, Reused));
	TestFalse(TEXT("occluded count cannot exceed total contacts"),
		FTacticalSummary::TryProject(
			1, 2, 5.0, 90.0f, 100.0f, Reused));
	TestFalse(TEXT("failed projection clears reusable tactical output"),
		Reused.IsValid());
	TestFalse(TEXT("negative contacts and non-finite range fail closed"),
		FTacticalSummary::TryProject(
			-1, 0, 0.0, 90.0f, 100.0f, Reused)
			|| FTacticalSummary::TryProject(
				1,
				0,
				std::numeric_limits<double>::quiet_NaN(),
				90.0f,
				100.0f,
				Reused));
	TestFalse(TEXT("an empty scan cannot claim a nearest contact distance"),
		FTacticalSummary::TryProject(
			0, 0, 1.0, 90.0f, 100.0f, Reused));
	TestFalse(TEXT("invalid spirit snapshots fail closed"),
		FTacticalSummary::TryProject(
			1, 0, 5.0, 101.0f, 100.0f, Reused)
			|| FTacticalSummary::TryProject(
				1, 0, 5.0, 0.0f, 0.0f, Reused));
	return true;
}

#endif
