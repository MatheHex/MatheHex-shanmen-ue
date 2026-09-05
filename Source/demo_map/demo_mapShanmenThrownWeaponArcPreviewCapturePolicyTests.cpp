#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapShanmenThrownWeaponArcPreviewCapturePolicy.h"

namespace
{
	using ECaptureStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewCaptureStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	constexpr EAutomationTestFlags CapturePolicyFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	const FGuid PreviewRequestId(0x20310001, 0, 0, 1);
	const FGuid AlternatePreviewRequestId(0x20310002, 0, 0, 1);
	const FGuid RunId(0x20310003, 0, 0, 1);
	const FGuid PlayerEntityId(0x20310004, 0, 0, 1);
	const FGuid SourceItemId(0x20310005, 0, 0, 1);

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P20.31");
		Content.Digest = TEXT("ARC-PREVIEW-CAPTURE-P20.31");
		return Content;
	}

	FShanmenThrownWeaponDefinitionCapture MakeDefinition(
		const FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId())
	{
		FShanmenThrownWeaponDefinitionCapture Definition;
		Definition.ActionDefinitionId = ActionDefinitionId;
		Definition.DetectorId =
			TEXT("Detector.ThrownWeapon.P20.31.ArcPreviewCapture");
		Definition.FormulaId =
			TEXT("Formula.ThrownWeapon.P20.31.ArcPreviewCapture");
		Definition.BaseDamage = 12.0f;
		Definition.TechniquePowerCoefficient = 0.75f;
		Definition.LaunchSpeed = 4000.0f;
		Definition.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Definition.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Definition;
	}

	Fdemo_mapShanmenThrownWeaponSessionConfig MakeArcSessionConfig()
	{
		FGameplayTagContainer SourceTags;
		SourceTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
		Fdemo_mapShanmenThrownWeaponSessionConfig Config;
		check(Fdemo_mapShanmenThrownWeaponSessionConfig::TryCaptureArc(
			MakeDefinition(),
			SourceTags,
			EShanmenThrownWeaponTechniqueTier::Intermediate,
			980.0,
			10.0,
			Config));
		return Config;
	}

	Fdemo_mapShanmenThrownWeaponSessionConfig MakeStraightSessionConfig()
	{
		Fdemo_mapShanmenThrownWeaponSessionConfig Config;
		check(Fdemo_mapShanmenThrownWeaponSessionConfig::TryCapture(
			MakeDefinition(
				FShanmenThrownWeaponDefinition::StraightActionDefinitionId()),
			FGameplayTagContainer(),
			Config));
		return Config;
	}

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy MakeChoicePolicy()
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
		check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 100.0, 500.0, Policy));
		return Policy;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest MakeRequest(
		const FGuid& RequestId = PreviewRequestId,
		const uint64 ObservedSequence = 17,
		const int32 SegmentCount = 8)
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest Request;
		check(Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
			RequestId,
			RunId,
			PlayerEntityId,
			SourceItemId,
			MakeContent(),
			ObservedSequence,
			MakeArcSessionConfig(),
			MakeChoicePolicy(),
			SegmentCount,
			Request));
		return Request;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeChoice()
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Command));
		auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(1, FVector2D(0.5, 0.5), Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(2, 0.25, Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	Fdemo_mapShanmenThrownWeaponArcChoiceBasis MakeBasis()
	{
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
		check(Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			FVector(100.0, 200.0, 50.0),
			FVector::ForwardVector,
			FVector::RightVector,
			Basis));
		return Basis;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureCanonicalTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.CanonicalCapture",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureCanonicalTest::RunTest(
	const FString&)
{
	const auto Result =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			MakeRequest());
	TestTrue(TEXT("canonical request captures self-validating evidence"),
		Result.IsCaptured());
	if (!Result.IsCaptured())
	{
		return false;
	}
	TestTrue(TEXT("preview and prospective real identities are distinct"),
		Result.GetPreviewActivationId().IsValid()
			&& Result.GetProspectiveRealActivationId().IsValid()
			&& Result.GetPreviewActivationId()
				!= Result.GetProspectiveRealActivationId());
	TestTrue(TEXT("preview action remains bound to exact frozen inputs"),
		Result.GetPreviewAction().GetRunId() == RunId
			&& Result.GetPreviewAction().GetOwnerId() == PlayerEntityId
			&& Result.GetPreviewAction().GetSourceEntityId() == PlayerEntityId
			&& Result.GetPreviewAction().GetSourceItemInstanceId() == SourceItemId
			&& Result.GetPreviewAction().GetActivationId()
				== Result.GetPreviewActivationId()
			&& Result.GetConfiguration().GetAction().GetActivationId()
				== Result.GetPreviewActivationId());
	TestTrue(TEXT("session tags and canonical player tag are preserved"),
		Result.GetPreviewAction().GetSourceTags().HasTagExact(
			FShanmenCombatNativeTags::DamagePhysicalSlash())
			&& Result.GetPreviewAction().GetSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureDeterminismTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.DeterministicReplay",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureDeterminismTest::RunTest(
	const FString&)
{
	const auto Request = MakeRequest();
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(Request);
	const auto Replay =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(Request);
	TestTrue(TEXT("equal read-only inputs replay exact capture evidence"),
		First.Matches(Replay)
			&& First.GetPreviewActivationId()
				== Replay.GetPreviewActivationId()
			&& First.GetConfiguration().GetConfigurationId()
				== Replay.GetConfiguration().GetConfigurationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureRevisionIsolationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.RevisionIsolation",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureRevisionIsolationTest::RunTest(
	const FString&)
{
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			MakeRequest());
	const auto Revised =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			MakeRequest(AlternatePreviewRequestId));
	TestTrue(TEXT("preview request revision changes only preview identity"),
		First.IsCaptured()
			&& Revised.IsCaptured()
			&& First.GetPreviewActivationId()
				!= Revised.GetPreviewActivationId()
			&& First.GetProspectiveRealActivationId()
				== Revised.GetProspectiveRealActivationId()
			&& First.GetConfiguration().GetConfigurationId()
				!= Revised.GetConfiguration().GetConfigurationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureSequenceSnapshotTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.SequenceSnapshot",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureSequenceSnapshotTest::RunTest(
	const FString&)
{
	uint64 NextActivationSequence = 17;
	const auto FirstRequest = MakeRequest(
		PreviewRequestId, NextActivationSequence);
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			FirstRequest);
	TestEqual(TEXT("pure capture cannot advance caller sequence"),
		NextActivationSequence, static_cast<uint64>(17));
	const auto Later =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			MakeRequest(PreviewRequestId, 18));
	TestTrue(TEXT("observed sequence participates in both identity domains"),
		First.IsCaptured()
			&& Later.IsCaptured()
			&& First.GetRequest().GetObservedNextActivationSequence() == 17
			&& First.GetPreviewActivationId()
				!= Later.GetPreviewActivationId()
			&& First.GetProspectiveRealActivationId()
				!= Later.GetProspectiveRealActivationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureIdentityDomainTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.IdentityDomain",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureIdentityDomainTest::RunTest(
	const FString&)
{
	const auto Result =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			MakeRequest());
	const FGuid ExpectedReal = FShanmenCombatIdFactory::MakeActivationId(
		RunId,
		PlayerEntityId,
		FShanmenThrownWeaponDefinition::ArcActionDefinitionId(),
		17);
	TestTrue(TEXT("real identity evidence uses the canonical activation factory"),
		Result.IsCaptured()
			&& Result.GetProspectiveRealActivationId() == ExpectedReal
			&& Result.GetPreviewActivationId() != ExpectedReal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureRequestFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.RequestFences",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureRequestFenceTest::RunTest(
	const FString&)
{
	auto Reused = MakeRequest();
	TestTrue(TEXT("zero sequence fails and clears reusable request"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
			PreviewRequestId, RunId, PlayerEntityId, SourceItemId,
			MakeContent(), 0, MakeArcSessionConfig(), MakeChoicePolicy(), 8,
			Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("exhausted sequence fails closed"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
			PreviewRequestId, RunId, PlayerEntityId, SourceItemId,
			MakeContent(), MAX_uint64, MakeArcSessionConfig(),
			MakeChoicePolicy(), 8, Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("straight session cannot enter Arc preview capture"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
			PreviewRequestId, RunId, PlayerEntityId, SourceItemId,
			MakeContent(), 17, MakeStraightSessionConfig(), MakeChoicePolicy(),
			8, Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("segment fences remain delegated to preview sampler"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
			PreviewRequestId, RunId, PlayerEntityId, SourceItemId,
			MakeContent(), 17, MakeArcSessionConfig(), MakeChoicePolicy(), 1,
			Reused)
			&& !Reused.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureTypedRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.TypedRejection",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureTypedRejectionTest::RunTest(
	const FString&)
{
	const auto Rejected =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest());
	TestTrue(TEXT("invalid request returns typed self-validating rejection"),
		Rejected.IsValid()
			&& Rejected.GetStatus() == ECaptureStatus::RequestRejected
			&& !Rejected.GetPreviewActivationId().IsValid()
			&& !Rejected.GetProspectiveRealActivationId().IsValid()
			&& !Rejected.GetPreviewAction().IsValid()
			&& !Rejected.GetConfiguration().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCaptureCompositionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewCapture.CompositionCompatibility",
	CapturePolicyFlags)

bool Fdemo_mapThrownWeaponArcPreviewCaptureCompositionTest::RunTest(
	const FString&)
{
	const auto Captured =
		Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
			MakeRequest());
	const auto Choice = MakeChoice();
	const auto Basis = MakeBasis();
	const auto Preview =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			Captured.GetConfiguration(),
			[&Choice]() { return Choice; },
			[&Basis]() { return Basis; });
	TestTrue(TEXT("captured configuration composes without real reservation"),
		Captured.IsCaptured()
			&& Preview.IsComposed()
			&& Preview.GetConfiguration().GetAction().GetActivationId()
				== Captured.GetPreviewActivationId()
			&& Preview.GetProjection().GetTarget()
				== FVector(1100.0, 350.0, 50.0));
	return true;
}

#endif
