#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSpiritShieldHUDPresentation.h"

#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	using EShieldTone = Edemo_mapShanmenSpiritShieldHUDTone;
	using FShieldPresentation =
		Fdemo_mapShanmenSpiritShieldHUDPresentation;
	using EFeedbackReason =
		Edemo_mapShanmenSpiritShieldInputFeedbackReason;
	using EFeedbackTone = Edemo_mapShanmenSpiritShieldInputFeedbackTone;
	using FFeedback =
		Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation;

	constexpr EAutomationTestFlags PresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	Fdemo_mapShanmenSpiritShieldProductActivationResult MakeRejected(
		const Edemo_mapShanmenSpiritShieldProductActivationError Error)
	{
		Fdemo_mapShanmenSpiritShieldProductActivationResult Result;
		Result.Status =
			Edemo_mapShanmenSpiritShieldProductActivationStatus::Rejected;
		Result.Error = Error;
		Result.Diagnostic = TEXT("Typed test rejection.");
		return Result;
	}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDAlreadyActiveFeedbackTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.AlreadyActiveFeedback",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDAlreadyActiveFeedbackTest::RunTest(
	const FString&)
{
	const auto Result = MakeRejected(
		Edemo_mapShanmenSpiritShieldProductActivationError::AlreadyActive);
	FFeedback Feedback;
	FFeedback Replay;
	TestTrue(TEXT("an active-session rejection projects from typed evidence"),
		FFeedback::TryProject(Result, TEXT("H"), Feedback)
			&& FFeedback::TryProject(Result, TEXT("G"), Replay));
	TestTrue(TEXT("already-active copy is deterministic and diagnostic-free"),
		Feedback.IsValid()
			&& Feedback.Matches(Replay)
			&& Feedback.GetReason() == EFeedbackReason::AlreadyActive
			&& Feedback.GetTone() == EFeedbackTone::Warning
			&& Feedback.GetDisplayText()
				== TEXT("SPIRIT SHIELD · ALREADY ACTIVE"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDInsufficientFeedbackTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.InsufficientFeedback",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDInsufficientFeedbackTest::RunTest(
	const FString&)
{
	auto Result = MakeRejected(
		Edemo_mapShanmenSpiritShieldProductActivationError::SessionRejected);
	Result.Begin.Status = EShanmenSpiritShieldActionStatus::Rejected;
	Result.Begin.Error =
		EShanmenSpiritShieldActionError::ResourceReservationRejected;
	Result.Begin.ResourceError =
		EShanmenActionResourceTransactionError::InsufficientAvailable;
	FFeedback Feedback;
	TestTrue(TEXT("typed insufficient-resource proof becomes readable"),
		Result.Begin.IsValid()
			&& FFeedback::TryProject(Result, TEXT("H"), Feedback));
	TestTrue(TEXT("feedback states the canonical Spirit cost"),
		Feedback.GetReason() == EFeedbackReason::InsufficientSpirit
			&& Feedback.GetTone() == EFeedbackTone::Warning
			&& Feedback.GetDisplayText()
				== TEXT("SPIRIT SHIELD · NEED 20 SPIRIT"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDActionBusyFeedbackTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.ActionBusyFeedback",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDActionBusyFeedbackTest::RunTest(const FString&)
{
	const auto Result = MakeRejected(
		Edemo_mapShanmenSpiritShieldProductActivationError::ActionConflict);
	FFeedback Feedback;
	TestTrue(TEXT("action-lane rejection retains the current remapped key"),
		FFeedback::TryProject(Result, TEXT(" G "), Feedback));
	TestTrue(TEXT("busy feedback tells the player how to retry"),
		Feedback.GetReason() == EFeedbackReason::ActionBusy
			&& Feedback.GetTone() == EFeedbackTone::Warning
			&& Feedback.GetDisplayText()
				== TEXT("SPIRIT SHIELD · ACTION BUSY · TRY [G] AGAIN"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldHUDInputFeedbackFencesTest,
	"Shanmen.0_0_10.Product.SpiritShieldHUDPresentation.InputFeedbackFences",
	PresentationFlags)

bool Fdemo_mapSpiritShieldHUDInputFeedbackFencesTest::RunTest(const FString&)
{
	const auto Technical = MakeRejected(
		Edemo_mapShanmenSpiritShieldProductActivationError::
			StateDesynchronized);
	FFeedback Reused;
	TestTrue(TEXT("technical typed rejection uses error feedback"),
		FFeedback::TryProject(Technical, TEXT("H"), Reused)
			&& Reused.GetReason() == EFeedbackReason::Failed
			&& Reused.GetTone() == EFeedbackTone::Error
			&& Reused.GetDisplayText()
				== TEXT("SPIRIT SHIELD · ACTIVATION FAILED"));

	const Fdemo_mapShanmenSpiritShieldProductActivationResult Invalid;
	TestFalse(TEXT("invalid product evidence fails closed"),
		FFeedback::TryProject(Invalid, TEXT("H"), Reused));
	TestFalse(TEXT("a failed projection clears reusable output"),
		Reused.IsValid());
	TestFalse(TEXT("blank remapped keys cannot produce misleading copy"),
		FFeedback::TryProject(Technical, TEXT("  "), Reused));
	return true;
}

#endif
