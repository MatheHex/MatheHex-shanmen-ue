#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ShanmenCombatTags.h"
#include "demo_mapShanmenThrownWeaponArcPreviewComposition.h"

namespace
{
	using ECompositionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewCompositionStatus;
	using EProjectionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	constexpr EAutomationTestFlags PreviewCompositionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	const FGuid CompositionRunId(0x20300001, 0, 0, 1);
	const FGuid CompositionOwnerId(0x20300002, 0, 0, 1);
	const FGuid CompositionActivationId(0x20300003, 0, 0, 1);
	const FGuid CompositionSourceEntityId(0x20300004, 0, 0, 1);
	const FGuid CompositionItemId(0x20300005, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeAction(
		const FName ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId(),
		const FString& Digest = TEXT("PREVIEW-COMPOSITION-P20.30"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = CompositionRunId;
		Capture.OwnerId = CompositionOwnerId;
		Capture.ActivationId = CompositionActivationId;
		Capture.SourceEntityId = CompositionSourceEntityId;
		Capture.SourceItemInstanceId = CompositionItemId;
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P20.30");
		Capture.Content.Digest = Digest;

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenThrownWeaponDefinitionCapture MakeDefinition(
		const float MaximumLaunchSpeed)
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId();
		Capture.DetectorId =
			TEXT("Detector.ThrownWeapon.P20.30.ArcPreviewComposition");
		Capture.FormulaId =
			TEXT("Formula.ThrownWeapon.P20.30.ArcPreviewComposition");
		Capture.BaseDamage = 10.0f;
		Capture.TechniquePowerCoefficient = 0.5f;
		Capture.LaunchSpeed = MaximumLaunchSpeed;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Capture;
	}

	Fdemo_mapShanmenThrownWeaponProductCapture MakeProduct(
		const float MaximumLaunchSpeed = 4000.0f)
	{
		Fdemo_mapShanmenThrownWeaponProductCapture Product;
		check(Fdemo_mapShanmenThrownWeaponProductCapture::TryCaptureArc(
			MakeDefinition(MaximumLaunchSpeed),
			20.0f,
			FGameplayTagContainer(),
			EShanmenThrownWeaponTechniqueTier::Intermediate,
			980.0,
			10.0,
			Product));
		return Product;
	}

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy MakeChoicePolicy()
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
		check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 100.0, 500.0, Policy));
		return Policy;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoice(
		const FVector2D& TargetIntent = FVector2D(0.5, 0.5),
		const double ApexAdjustment = 0.25)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Command));
		Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(1, TargetIntent, Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		if (ApexAdjustment != 0.0)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcApexAdjustment(2, ApexAdjustment, Command));
			Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
			check(Reduced.DidChange());
			State = Reduced.State;
		}
		return State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcWithoutTarget()
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Command));
		const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
				Command);
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

	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration MakeConfiguration(
		const int32 SegmentCount = 8,
		const float MaximumLaunchSpeed = 4000.0f,
		const FString& Digest = TEXT("PREVIEW-COMPOSITION-P20.30"))
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Configuration;
		check(Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
			MakeAction(
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId(), Digest),
			MakeProduct(MaximumLaunchSpeed),
			MakeChoicePolicy(),
			SegmentCount,
			Configuration));
		return Configuration;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Compose(
		const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Configuration,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& Basis)
	{
		return Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			Configuration,
			[&Choice]() { return Choice; },
			[&Basis]() { return Basis; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionCanonicalTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.CanonicalComposition",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionCanonicalTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Configuration =
		MakeConfiguration();
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Choice = MakeArcChoice();
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = MakeBasis();
	int32 ChoiceReads = 0;
	int32 BasisSamples = 0;
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Result =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			Configuration,
			[&ChoiceReads, &Choice]()
			{
				++ChoiceReads;
				return Choice;
			},
			[&BasisSamples, &Basis]()
			{
				++BasisSamples;
				return Basis;
			});
	TestTrue(TEXT("valid inputs compose one self-validating preview"),
		Result.IsComposed());
	if (!Result.IsComposed())
	{
		return false;
	}
	TestTrue(TEXT("choice and source basis are each sampled exactly once"),
		ChoiceReads == 1
			&& BasisSamples == 1
			&& Result.GetChoiceStateReadCount() == 1
			&& Result.GetBasisSampleCount() == 1);
	TestTrue(TEXT("choice projection remains the target/apex authority"),
		Result.GetProjection().GetTarget() == FVector(1100.0, 350.0, 50.0)
			&& Result.GetProjection().GetApexClearance() == 350.0);
	TestTrue(TEXT("preview remains bound to the exact composed plan"),
		Result.GetPlanResult().IsPlanned()
			&& Result.GetPreview().GetPlan().Matches(Result.GetPlanResult().Plan)
			&& Result.GetPreview().GetPositions().Num() == 9
			&& Result.GetPreview().GetPositions()[0] == Basis.GetOrigin()
			&& Result.GetPreview().GetPositions().Last()
				== Result.GetProjection().GetTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionDeterminismTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.DeterministicReplay",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionDeterminismTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Configuration =
		MakeConfiguration();
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Choice = MakeArcChoice();
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = MakeBasis();
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult First =
		Compose(Configuration, Choice, Basis);
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Replay =
		Compose(Configuration, Choice, Basis);
	TestTrue(TEXT("equal frozen inputs replay exact composition evidence"),
		First.Matches(Replay)
			&& First.GetConfiguration().GetConfigurationId()
				== Replay.GetConfiguration().GetConfigurationId()
			&& First.GetPreview().GetPreviewId()
				== Replay.GetPreview().GetPreviewId());

	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration HigherResolution =
		MakeConfiguration(16);
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Changed =
		Compose(HigherResolution, Choice, Basis);
	TestTrue(TEXT("segment resolution changes configuration and preview identity"),
		Changed.IsComposed()
			&& !First.GetConfiguration().Matches(HigherResolution)
			&& First.GetPreview().GetPreviewId()
				!= Changed.GetPreview().GetPreviewId()
			&& Changed.GetPreview().GetPositions().Num() == 17);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionConfigurationFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.ConfigurationFences",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionConfigurationFenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Reused =
		MakeConfiguration();
	TestTrue(TEXT("mismatched action/product identity fails and clears output"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
			MakeAction(FShanmenThrownWeaponDefinition::StraightActionDefinitionId()),
			MakeProduct(),
			MakeChoicePolicy(),
			8,
			Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("below-minimum segment count fails closed"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
			MakeAction(), MakeProduct(), MakeChoicePolicy(),
			FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount - 1,
			Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("above-maximum segment count fails closed"),
		!Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
			MakeAction(), MakeProduct(), MakeChoicePolicy(),
			FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount + 1,
			Reused)
			&& !Reused.IsValid());
	TestTrue(TEXT("both inclusive preview segment fences remain admissible"),
		Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
			MakeAction(), MakeProduct(), MakeChoicePolicy(),
			FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount,
			Reused)
			&& Reused.IsValid()
			&& Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
				MakeAction(), MakeProduct(), MakeChoicePolicy(),
				FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount,
				Reused)
			&& Reused.IsValid());

	int32 ChoiceReads = 0;
	int32 BasisSamples = 0;
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Rejected =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration(),
			[&ChoiceReads]()
			{
				++ChoiceReads;
				return MakeArcChoice();
			},
			[&BasisSamples]()
			{
				++BasisSamples;
				return MakeBasis();
			});
	TestTrue(TEXT("invalid configuration rejects before live reads"),
		Rejected.IsValid()
			&& Rejected.GetStatus() == ECompositionStatus::ConfigurationRejected
			&& ChoiceReads == 0
			&& BasisSamples == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionChoiceUnavailableTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.ChoiceUnavailable",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionChoiceUnavailableTest::RunTest(
	const FString&)
{
	int32 BasisSamples = 0;
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Result =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			MakeConfiguration(),
			[]() { return Fdemo_mapShanmenThrownWeaponInputChoiceState(); },
			[&BasisSamples]()
			{
				++BasisSamples;
				return MakeBasis();
			});
	TestTrue(TEXT("invalid current choice short-circuits source sampling"),
		Result.IsValid()
			&& Result.GetStatus() == ECompositionStatus::ChoiceUnavailable
			&& Result.GetChoiceStateReadCount() == 1
			&& Result.GetBasisSampleCount() == 0
			&& BasisSamples == 0
			&& !Result.GetPlanResult().IsValid()
			&& !Result.GetPreview().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionBasisUnavailableTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.BasisUnavailable",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionBasisUnavailableTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Result =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			MakeConfiguration(),
			[]() { return MakeArcChoice(); },
			[]() { return Fdemo_mapShanmenThrownWeaponArcChoiceBasis(); });
	TestTrue(TEXT("invalid source basis stops before projection and planning"),
		Result.IsValid()
			&& Result.GetStatus() == ECompositionStatus::BasisUnavailable
			&& Result.GetChoiceStateReadCount() == 1
			&& Result.GetBasisSampleCount() == 1
			&& Result.GetProjection().GetStatus()
				== EProjectionStatus::Invalid
			&& !Result.GetPlanResult().IsValid()
			&& !Result.GetPreview().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionProjectionReasonTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.ProjectionReasons",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionProjectionReasonTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Configuration =
		MakeConfiguration();
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = MakeBasis();
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Straight =
		Compose(
			Configuration,
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			Basis);
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult NoTarget =
		Compose(Configuration, MakeArcWithoutTarget(), Basis);
	TestTrue(TEXT("non-Arc choice preserves projector reason"),
		Straight.IsValid()
			&& Straight.GetStatus() == ECompositionStatus::ProjectionRejected
			&& Straight.GetProjection().GetStatus()
				== EProjectionStatus::TrajectoryNotArc
			&& Straight.GetDiagnostic()
				== Straight.GetProjection().GetDiagnostic());
	TestTrue(TEXT("missing target preserves projector reason"),
		NoTarget.IsValid()
			&& NoTarget.GetStatus() == ECompositionStatus::ProjectionRejected
			&& NoTarget.GetProjection().GetStatus()
				== EProjectionStatus::TargetIntentMissing
			&& NoTarget.GetDiagnostic()
				== NoTarget.GetProjection().GetDiagnostic());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionPlanReasonTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewComposition.PlanReason",
	PreviewCompositionFlags)

bool Fdemo_mapThrownWeaponArcPreviewCompositionPlanReasonTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Result =
		Compose(MakeConfiguration(8, 1.0f), MakeArcChoice(), MakeBasis());
	TestTrue(TEXT("unreachable geometry preserves planner rejection"),
		Result.IsValid()
			&& Result.GetStatus() == ECompositionStatus::PlanRejected
			&& Result.GetProjection().IsProjected()
			&& Result.GetPlanResult().IsValid()
			&& Result.GetPlanResult().Status
				== EShanmenThrownWeaponArcPlanStatus::Unreachable
			&& Result.GetDiagnostic() == Result.GetPlanResult().Diagnostic
			&& !Result.GetPreview().IsValid());
	return true;
}

#endif
