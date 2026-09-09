#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcSourceBasisAdapter.h"

#include "demo_mapGameMode.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

namespace
{
	using ERouteStatus =
		Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus;
	using ESampleStatus =
		Edemo_mapShanmenThrownWeaponArcSourceBasisStatus;
	using ECompositionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;
	using EProjectionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoice()
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

	AActor* MakeSourceActor(
		const FVector& Location = FVector(100.0, 200.0, 0.0),
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		AActor* Actor = NewObject<AActor>(
			GetTransientPackage(), NAME_None, RF_Transient);
		check(Actor);
		USceneComponent* Root = NewObject<USceneComponent>(
			Actor, NAME_None, RF_Transient);
		check(Root);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocationAndRotation(Location, Rotation.Quaternion());
		return Actor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcSourceBasisSampleContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter.SampleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcSourceBasisSampleContractTest::RunTest(
	const FString&)
{
	AActor* Source = MakeSourceActor();
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::Sample(Source);
	const auto Replay =
		Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::Sample(Source);
	TestTrue(TEXT("One transform snapshot yields the canonical launch basis"),
		First.IsSampled()
			&& First.GetTransformSampleCount() == 1
			&& !First.UsedSkeletalHandOrigin()
			&& First.GetBasis().GetOrigin().Equals(
				FVector(155.0, 228.0, 50.0))
			&& First.GetBasis().GetForward().Equals(FVector::ForwardVector)
			&& First.GetBasis().GetRight().Equals(FVector::RightVector));
	TestTrue(TEXT("Unchanged source transform replays the same basis identity"),
		Replay.IsSampled()
			&& First.GetBasis().Matches(Replay.GetBasis()));

	Source->GetRootComponent()->SetWorldLocationAndRotation(
		FVector(-20.0, 300.0, 10.0),
		FRotator(0.0, 90.0, 0.0).Quaternion());
	const auto Moved =
		Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::Sample(Source);
	TestTrue(TEXT("Moved source produces a fresh oriented launch basis"),
		Moved.IsSampled()
			&& Moved.GetBasis().GetOrigin().Equals(
				FVector(-48.0, 355.0, 60.0))
			&& Moved.GetBasis().GetForward().Equals(FVector::RightVector)
			&& Moved.GetBasis().GetRight().Equals(FVector::BackwardVector)
			&& !First.GetBasis().Matches(Moved.GetBasis()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcSourceBasisLazyClassificationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter.LazyClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcSourceBasisLazyClassificationTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter Adapter;
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	int32 CompositionRoutes = 0;
	const auto Result = Adapter.Route(
		nullptr,
		[&](TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>
			SampleBasis)
		{
			++CompositionRoutes;
			return Composition.Route(
				MakeArcChoice(),
				MakePolicy(),
				SampleBasis,
				[](TFunctionRef<FVector()>, TFunctionRef<double()>)
				{
					return MakeInputResult(
						Edemo_mapShanmenThrownWeaponInputStatus::PassThrough,
						TEXT("Synthetic item classification passed through."));
				});
		});
	TestTrue(TEXT("Early input completion is a valid lazy source outcome"),
		Result.IsValid()
			&& Result.GetStatus()
				== ERouteStatus::InputCompletedBeforeSourceSample
			&& Result.GetComposition().GetStatus()
				== ECompositionStatus::InputCompletedBeforeProjection);
	TestTrue(TEXT("Unclaimed input performs no source transform work"),
		CompositionRoutes == 1
			&& Result.GetRouteInvocationCount() == 1
			&& Result.GetSourceSampleRequestCount() == 0
			&& !Result.GetSourceSample().IsValid()
			&& Result.GetComposition().GetBasisSampleCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcSourceBasisDelegationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter.ProjectAndDelegate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcSourceBasisDelegationTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter Adapter;
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	FVector CapturedTarget = FVector::ZeroVector;
	double CapturedApex = 0.0;
	const auto Result = Adapter.Route(
		MakeSourceActor(),
		[&](TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>
			SampleBasis)
		{
			return Composition.Route(
				MakeArcChoice(),
				MakePolicy(),
				SampleBasis,
				[&](TFunctionRef<FVector()> SampleTarget,
					TFunctionRef<double()> SampleApex)
				{
					CapturedTarget = SampleTarget();
					CapturedApex = SampleApex();
					return MakeInputResult(
						Edemo_mapShanmenThrownWeaponInputStatus::ProductRejected,
						TEXT("Synthetic downstream route completed."),
						true,
						true);
				});
		});
	TestTrue(TEXT("Live source basis projects and delegates one Arc choice"),
		Result.IsValid()
			&& Result.GetStatus() == ERouteStatus::Composed
			&& Result.GetSourceSample().IsSampled()
			&& Result.GetSourceSample().GetTransformSampleCount() == 1
			&& Result.GetComposition().GetStatus()
				== ECompositionStatus::Delegated
			&& CapturedTarget.Equals(FVector(1155.0, 378.0, 50.0))
			&& CapturedApex == 350.0);
	TestTrue(TEXT("Source, basis, target, and apex are each requested once"),
		Result.GetSourceSampleRequestCount() == 1
			&& Result.GetComposition().GetBasisSampleCount() == 1
			&& Result.GetComposition().GetTargetRequestCount() == 1
			&& Result.GetComposition().GetApexRequestCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcSourceBasisFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter.SourceFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcSourceBasisFailureTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter Adapter;
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	const auto Result = Adapter.Route(
		nullptr,
		[&](TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>
			SampleBasis)
		{
			return Composition.Route(
				MakeArcChoice(),
				MakePolicy(),
				SampleBasis,
				[](TFunctionRef<FVector()> SampleTarget,
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
						TEXT("Synthetic route sampled the source target."),
						true,
						false);
				});
		});
	TestTrue(TEXT("Unavailable source is preserved as a valid rejection"),
		Result.IsValid()
			&& Result.GetStatus() == ERouteStatus::SourceSampleRejected
			&& Result.GetSourceSample().GetStatus()
				== ESampleStatus::SourceUnavailable
			&& Result.GetSourceSample().GetTransformSampleCount() == 0
			&& Result.GetComposition().GetStatus()
				== ECompositionStatus::ProjectionRejected
			&& Result.GetComposition().GetProjection().GetStatus()
				== EProjectionStatus::BasisInvalid
			&& Result.GetComposition().GetInput().Status
				== Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable);
	TestTrue(TEXT("Rejected source is requested once and never reaches apex"),
		Result.GetSourceSampleRequestCount() == 1
			&& Result.GetComposition().GetBasisSampleCount() == 1
			&& Result.GetComposition().GetApexRequestCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcSourceBasisProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter.ProtocolFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcSourceBasisProtocolTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter Adapter;
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
	const auto ApexFirst = Adapter.Route(
		nullptr,
		[&](TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>
			SampleBasis)
		{
			return Composition.Route(
				MakeArcChoice(),
				MakePolicy(),
				SampleBasis,
				[](TFunctionRef<FVector()>, TFunctionRef<double()> SampleApex)
				{
					SampleApex();
					return MakeInputResult(
						Edemo_mapShanmenThrownWeaponInputStatus::ApexClearanceUnavailable,
						TEXT("Synthetic route requested apex first."),
						false,
						true);
				});
		});
	TestTrue(TEXT("Apex-before-target rejects without source sampling"),
		ApexFirst.IsValid()
			&& ApexFirst.GetStatus()
				== ERouteStatus::CompositionProtocolRejected
			&& ApexFirst.GetSourceSampleRequestCount() == 0);

	const auto DuplicateTarget = Adapter.Route(
		MakeSourceActor(),
		[&](TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>
			SampleBasis)
		{
			return Composition.Route(
				MakeArcChoice(),
				MakePolicy(),
				SampleBasis,
				[](TFunctionRef<FVector()> SampleTarget,
					TFunctionRef<double()>)
				{
					SampleTarget();
					SampleTarget();
					return MakeInputResult(
						Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable,
						TEXT("Synthetic route requested target twice."),
						true,
						false);
				});
		});
	TestTrue(TEXT("Duplicate target never resamples the source transform"),
		DuplicateTarget.IsValid()
			&& DuplicateTarget.GetStatus()
				== ERouteStatus::CompositionProtocolRejected
			&& DuplicateTarget.GetSourceSampleRequestCount() == 1
			&& DuplicateTarget.GetSourceSample().GetTransformSampleCount() == 1
			&& DuplicateTarget.GetComposition().GetTargetRequestCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcSourceBasisGameModeBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcSourceBasisAdapter.GameModeBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcSourceBasisGameModeBoundaryTest::RunTest(
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

	const auto Result =
		GameMode->RouteThrownWeaponArcChoiceFromSourceHotbarInput(
			2, nullptr, MakePolicy());
	TestTrue(TEXT("GameMode retains existing unclaimed input classification"),
		Result.IsValid()
			&& Result.GetStatus()
				== ERouteStatus::InputCompletedBeforeSourceSample
			&& Result.GetComposition().GetInput().Status
				== Edemo_mapShanmenThrownWeaponInputStatus::PassThrough);
	TestTrue(TEXT("Unclaimed route samples no source and preserves choice"),
		Result.GetSourceSampleRequestCount() == 0
			&& Result.GetComposition().GetBasisSampleCount() == 0
			&& GameMode->GetThrownWeaponInputChoiceState().GetStateId()
				== FrozenStateId
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 3);
	return true;
}

#endif
