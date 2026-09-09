#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponLaunchRejectionPresentation.h"

#include "Misc/AutomationTest.h"

namespace
{
	using ECommand = Edemo_mapShanmenThrownWeaponRunCommandStatus;
	using EKind = Edemo_mapShanmenThrownWeaponLaunchRejectionKind;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation;

	constexpr EAutomationTestFlags FeedbackFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	Fdemo_mapShanmenThrownWeaponSessionResult MakeReleaseBlockedResult(
		const ECommand CommandStatus = ECommand::LaunchRejectedCancelled)
	{
		const FGuid SelectionId(
			0xF4720001, 0xF4720002, 0xF4720003, 0xF4720004);
		const FGuid RunId(
			0xF4720011, 0xF4720012, 0xF4720013, 0xF4720014);
		const FGuid ItemId(
			0xF4720021, 0xF4720022, 0xF4720023, 0xF4720024);
		const FGuid ActivationId(
			0xF4720031, 0xF4720032, 0xF4720033, 0xF4720034);

		Fdemo_mapShanmenThrownWeaponSessionResult Result;
		Result.Status =
			Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected;
		Result.SelectionId = SelectionId;
		Result.RunId = RunId;
		Result.HotbarSlotNumber = 2;
		Result.ItemInstanceId = ItemId;
		Result.Diagnostic = TEXT("Exact release path was blocked.");
		Result.Product.Status =
			Edemo_mapShanmenThrownWeaponProductStatus::RouterRejected;
		Result.Product.SelectionId = SelectionId;
		Result.Product.RunId = RunId;
		Result.Product.SourceItemInstanceId = ItemId;
		Result.Product.ActivationSequence = 7;
		Result.Product.ActivationId = ActivationId;
		Result.Product.Command.Status = CommandStatus;
		Result.Product.Command.IntentId = ActivationId;
		Result.Product.Command.RunId = RunId;
		Result.Product.Command.ItemInstanceId = ItemId;
		Result.Product.Command.HostStart.Error =
			Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected;
		Result.Product.Command.HostStart.Launch.Error =
			Edemo_mapShanmenThrownWeaponLaunchError::ReleasePathBlocked;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponLaunchRejectionPresentationTypedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponLaunchRejectionPresentation.TypedReleasePath",
	FeedbackFlags)

bool Fdemo_mapThrownWeaponLaunchRejectionPresentationTypedTest::RunTest(
	const FString&)
{
	const auto Result = MakeReleaseBlockedResult();
	FPresentation Presentation;
	FPresentation Replay;
	TestTrue(TEXT("Exact release-path rejection projects deterministic feedback"),
		FPresentation::TryProject(Result, Presentation)
			&& FPresentation::TryProject(Result, Replay)
			&& Presentation.IsValid()
			&& Presentation.Matches(Replay)
			&& Presentation.GetKind() == EKind::ReleasePathBlocked
			&& Presentation.GetRunId() == Result.RunId
			&& Presentation.GetSelectionId() == Result.SelectionId
			&& !Presentation.RequiresCancellationRecovery()
			&& Presentation.GetDisplayText()
				== TEXT("飞刀 · 释放路径受阻"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponLaunchRejectionPresentationRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponLaunchRejectionPresentation.RecoveryEvidence",
	FeedbackFlags)

bool Fdemo_mapThrownWeaponLaunchRejectionPresentationRecoveryTest::RunTest(
	const FString&)
{
	FPresentation Presentation;
	TestTrue(TEXT("The same physical rejection preserves recovery evidence"),
		FPresentation::TryProject(
			MakeReleaseBlockedResult(ECommand::RecoveryRequired),
			Presentation)
			&& Presentation.IsValid()
			&& Presentation.RequiresCancellationRecovery());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponLaunchRejectionPresentationFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponLaunchRejectionPresentation.SourceFences",
	FeedbackFlags)

bool Fdemo_mapThrownWeaponLaunchRejectionPresentationFenceTest::RunTest(
	const FString&)
{
	FPresentation Reused;
	check(FPresentation::TryProject(MakeReleaseBlockedResult(), Reused));

	auto ContractRejected = MakeReleaseBlockedResult();
	ContractRejected.Product.Command.HostStart.Launch.Error =
		Edemo_mapShanmenThrownWeaponLaunchError::ProjectileStageRejected;
	TestFalse(TEXT("A non-geometry staging rejection cannot masquerade as a wall"),
		FPresentation::TryProject(ContractRejected, Reused));
	TestTrue(TEXT("Failed projection clears reusable output"),
		!Reused.IsValid());

	auto ActionCommitRejected = MakeReleaseBlockedResult();
	ActionCommitRejected.Product.Command.HostStart =
		Fdemo_mapShanmenThrownWeaponHostStartResult();
	TestFalse(TEXT("A pre-host action failure has no release-path feedback"),
		FPresentation::TryProject(ActionCommitRejected, Reused));

	auto Accepted = MakeReleaseBlockedResult(ECommand::Applied);
	Accepted.Status = Edemo_mapShanmenThrownWeaponSessionStatus::Applied;
	Accepted.Product.Status =
		Edemo_mapShanmenThrownWeaponProductStatus::Applied;
	TestFalse(TEXT("An accepted launch cannot retain rejection feedback"),
		FPresentation::TryProject(Accepted, Reused));
	return true;
}

#endif
