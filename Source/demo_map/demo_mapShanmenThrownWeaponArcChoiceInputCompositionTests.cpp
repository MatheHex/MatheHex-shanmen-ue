#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcChoiceInputComposition.h"

#include "demo_mapGameMode.h"

#include "Misc/AutomationTest.h"

namespace
{
	using ECompositionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;
	using EProjectionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoice(
		const bool bIncludeTarget = true)
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
		if (!bIncludeTarget)
		{
			return State;
		}

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

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy MakePolicy()
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
		check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 100.0, 500.0, Policy));
		return Policy;
	}

	Fdemo_mapShanmenThrownWeaponInputResult MakeInputResult(
		const Edemo_mapShanmenThrownWeaponInputStatus Status,
		const TCHAR* Diagnostic,
		const bool bTargetSampled = false,
		const bool bApexSampled = false)
	{
		Fdemo_mapShanmenThrownWeaponInputResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.TrajectoryKind = ETrajectory::BallisticArc;
		Result.bTargetSampled = bTargetSampled;
		Result.bApexClearanceSampled = bApexSampled;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceCompositionLazyClassificationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceInputComposition.LazyClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceCompositionLazyClassificationTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	int32 BasisSamples = 0;
	int32 RouteCalls = 0;
	const auto Result = Composition.Route(
		MakeArcChoice(),
		MakePolicy(),
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		},
		[&RouteCalls](
			TFunctionRef<FVector()>,
			TFunctionRef<double()>)
		{
			++RouteCalls;
			return MakeInputResult(
				Edemo_mapShanmenThrownWeaponInputStatus::PassThrough,
				TEXT("Synthetic item classification passed through."));
		});
	TestTrue(TEXT("Early input completion is a valid audited outcome"),
		Result.IsValid()
			&& Result.GetStatus()
				== ECompositionStatus::InputCompletedBeforeProjection
			&& Result.GetInput().ShouldPassThrough());
	TestTrue(TEXT("Classification runs once without eager geometry work"),
		RouteCalls == 1
			&& BasisSamples == 0
			&& Result.GetRouteInvocationCount() == 1
			&& Result.GetBasisSampleCount() == 0
			&& Result.GetTargetRequestCount() == 0
			&& Result.GetApexRequestCount() == 0
			&& !Result.GetProjection().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceCompositionDelegationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceInputComposition.ProjectAndDelegate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceCompositionDelegationTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Choice = MakeArcChoice();
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy = MakePolicy();
	int32 BasisSamples = 0;
	FVector CapturedTarget = FVector::ZeroVector;
	double CapturedApex = 0.0;
	const auto RouteOnce = [&BasisSamples, &CapturedTarget, &CapturedApex](
		TFunctionRef<FVector()> SampleTarget,
		TFunctionRef<double()> SampleApex)
	{
		CapturedTarget = SampleTarget();
		CapturedApex = SampleApex();
		return MakeInputResult(
			Edemo_mapShanmenThrownWeaponInputStatus::ProductRejected,
			TEXT("Synthetic downstream route completed."),
			true,
			true);
	};
	const auto First = Composition.Route(
		Choice,
		Policy,
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		},
		RouteOnce);
	TestTrue(TEXT("One projection delegates target and apex unchanged"),
		First.IsValid()
			&& First.GetStatus() == ECompositionStatus::Delegated
			&& First.GetProjection().IsValid()
			&& CapturedTarget.Equals(FVector(1100.0, 350.0, 50.0))
			&& CapturedApex == 350.0
			&& First.GetProjection().GetTarget().Equals(CapturedTarget)
			&& First.GetProjection().GetApexClearance() == CapturedApex);
	TestTrue(TEXT("Basis, target, and apex are each requested exactly once"),
		BasisSamples == 1
			&& First.GetBasisSampleCount() == 1
			&& First.GetTargetRequestCount() == 1
			&& First.GetApexRequestCount() == 1);

	BasisSamples = 0;
	const auto Replay = Composition.Route(
		Choice,
		Policy,
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		},
		RouteOnce);
	TestTrue(TEXT("Equivalent composition reproduces the projection identity"),
		Replay.IsValid()
			&& First.GetProjection().Matches(Replay.GetProjection())
			&& BasisSamples == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceCompositionProjectionFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceInputComposition.ProjectionFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceCompositionProjectionFailureTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	const auto RejectNonFiniteTarget = [](
		TFunctionRef<FVector()> SampleTarget,
		TFunctionRef<double()>)
	{
		const FVector Target = SampleTarget();
		const bool bFinite = FMath::IsFinite(Target.X)
			&& FMath::IsFinite(Target.Y)
			&& FMath::IsFinite(Target.Z);
		return MakeInputResult(
			bFinite
				? Edemo_mapShanmenThrownWeaponInputStatus::ProductRejected
				: Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable,
			bFinite
				? TEXT("Synthetic route unexpectedly received a target.")
				: TEXT("Synthetic route rejected the target sentinel."),
			true,
			false);
	};
	int32 BasisSamples = 0;
	const auto MissingTarget = Composition.Route(
		MakeArcChoice(false),
		MakePolicy(),
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		},
		RejectNonFiniteTarget);
	TestTrue(TEXT("Missing choice target preserves projector reason"),
		MissingTarget.IsValid()
			&& MissingTarget.GetStatus()
				== ECompositionStatus::ProjectionRejected
			&& MissingTarget.GetProjection().GetStatus()
				== EProjectionStatus::TargetIntentMissing
			&& MissingTarget.GetInput().Status
				== Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable
			&& MissingTarget.GetApexRequestCount() == 0
			&& BasisSamples == 1);

	BasisSamples = 0;
	const auto InvalidBasis = Composition.Route(
		MakeArcChoice(),
		MakePolicy(),
		[&BasisSamples]()
		{
			++BasisSamples;
			return Fdemo_mapShanmenThrownWeaponArcChoiceBasis();
		},
		RejectNonFiniteTarget);
	TestTrue(TEXT("Invalid sampled basis fails once without requesting apex"),
		InvalidBasis.IsValid()
			&& InvalidBasis.GetStatus()
				== ECompositionStatus::ProjectionRejected
			&& InvalidBasis.GetProjection().GetStatus()
				== EProjectionStatus::BasisInvalid
			&& InvalidBasis.GetBasisSampleCount() == 1
			&& InvalidBasis.GetTargetRequestCount() == 1
			&& InvalidBasis.GetApexRequestCount() == 0
			&& BasisSamples == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceCompositionProtocolFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceInputComposition.ProtocolFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceCompositionProtocolFailureTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	int32 BasisSamples = 0;
	const auto ApexFirst = Composition.Route(
		MakeArcChoice(),
		MakePolicy(),
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		},
		[](TFunctionRef<FVector()>, TFunctionRef<double()> SampleApex)
		{
			SampleApex();
			return MakeInputResult(
				Edemo_mapShanmenThrownWeaponInputStatus::ApexClearanceUnavailable,
				TEXT("Synthetic route requested apex before target."),
				false,
				true);
		});
	TestTrue(TEXT("Apex-before-target protocol fails closed without basis work"),
		ApexFirst.IsValid()
			&& ApexFirst.GetStatus()
				== ECompositionStatus::RouteProtocolRejected
			&& !ApexFirst.IsAccepted()
			&& BasisSamples == 0
			&& ApexFirst.GetTargetRequestCount() == 0
			&& ApexFirst.GetApexRequestCount() == 1);

	BasisSamples = 0;
	const auto DuplicateTarget = Composition.Route(
		MakeArcChoice(),
		MakePolicy(),
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		},
		[](TFunctionRef<FVector()> SampleTarget, TFunctionRef<double()>)
		{
			SampleTarget();
			SampleTarget();
			return MakeInputResult(
				Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable,
				TEXT("Synthetic route requested target twice."),
				true,
				false);
		});
	TestTrue(TEXT("Duplicate target request never resamples caller basis"),
		DuplicateTarget.IsValid()
			&& DuplicateTarget.GetStatus()
				== ECompositionStatus::RouteProtocolRejected
			&& DuplicateTarget.GetTargetRequestCount() == 2
			&& DuplicateTarget.GetBasisSampleCount() == 1
			&& BasisSamples == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcChoiceCompositionGameModeBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcChoiceInputComposition.GameModeBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcChoiceCompositionGameModeBoundaryTest::RunTest(
	const FString&)
{
	Ademo_mapGameMode* GameMode = NewObject<Ademo_mapGameMode>(
		GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient GameMode exists"), GameMode))
	{
		return false;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Command));
	TestTrue(TEXT("GameMode accepts Arc choice"),
		GameMode->SubmitThrownWeaponInputChoiceCommand(Command).DidChange());
	check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcTargetIntent(1, FVector2D(0.5, 0.5), Command));
	TestTrue(TEXT("GameMode accepts target choice"),
		GameMode->SubmitThrownWeaponInputChoiceCommand(Command).DidChange());
	check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcApexAdjustment(2, 0.25, Command));
	TestTrue(TEXT("GameMode accepts apex choice"),
		GameMode->SubmitThrownWeaponInputChoiceCommand(Command).DidChange());
	const FGuid FrozenStateId =
		GameMode->GetThrownWeaponInputChoiceState().GetStateId();

	int32 BasisSamples = 0;
	const auto Result = GameMode->RouteThrownWeaponArcChoiceHotbarInput(
		2,
		nullptr,
		MakePolicy(),
		[&BasisSamples]()
		{
			++BasisSamples;
			return MakeBasis();
		});
	TestTrue(TEXT("GameMode delegates through existing unclaimed input route"),
		Result.IsValid()
			&& Result.GetStatus()
				== ECompositionStatus::InputCompletedBeforeProjection
			&& Result.GetInput().Status
				== Edemo_mapShanmenThrownWeaponInputStatus::PassThrough);
	TestTrue(TEXT("Unclaimed route samples nothing and preserves frozen choice"),
		BasisSamples == 0
			&& Result.GetBasisSampleCount() == 0
			&& !Result.GetProjection().IsValid()
			&& GameMode->GetThrownWeaponInputChoiceState().GetStateId()
				== FrozenStateId
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 3);
	return true;
}

#endif
