#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"

#include "demo_mapPlayerController.h"

#include "Misc/AutomationTest.h"

namespace
{
	using ECapability =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability;
	using EIntentKind =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentKind;
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceState Apply(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		const auto Result =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Result.IsSuccess());
		return Result.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState Select(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const ETrajectory Trajectory)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				State.GetRevision(), Trajectory, Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState SetTarget(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const FVector2D& Target = FVector2D(3.0, 4.0))
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(State.GetRevision(), Target, Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState AdjustApex(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const double Delta)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(
				State.GetRevision(), Delta, Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Project(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Model;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::
			TryProject(State, Model));
		return Model;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionCanonicalProjectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.CanonicalProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionCanonicalProjectionTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto Straight = Project(Initial);
	const auto ArcState = SetTarget(Select(Initial, ETrajectory::BallisticArc));
	const auto Arc = Project(ArcState);

	TestTrue(TEXT("Straight projection is canonical and revisionless"),
		Straight.IsValid()
			&& Straight.GetTrajectoryKind() == ETrajectory::Straight
			&& !Straight.HasArcTargetIntent()
			&& Straight.GetArcTargetIntent().IsZero()
			&& Straight.GetArcApexAdjustment() == 0.0
			&& Straight.GetCapabilities()
				== ECapability::SelectBallisticArcTrajectory);
	TestTrue(TEXT("Arc projection retains canonical visible values"),
		Arc.IsValid()
			&& Arc.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Arc.HasArcTargetIntent()
			&& Arc.GetArcTargetIntent().Equals(FVector2D(0.6, 0.8))
			&& Arc.GetReadModelId() != Straight.GetReadModelId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionCapabilitiesTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.CapabilityTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionCapabilitiesTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto Straight = Project(Initial);
	const auto Arc = Project(Select(Initial, ETrajectory::BallisticArc));
	const auto Targeted = Project(SetTarget(
		Select(Initial, ETrajectory::BallisticArc)));

	TestTrue(TEXT("Straight exposes only the alternate trajectory"),
		Straight.HasCapability(ECapability::SelectBallisticArcTrajectory)
			&& !Straight.HasCapability(
				ECapability::SelectStraightTrajectory)
			&& !Straight.HasCapability(ECapability::SetArcTargetIntent));
	TestTrue(TEXT("Neutral Arc exposes meaningful Arc edits"),
		Arc.HasCapability(ECapability::SelectStraightTrajectory)
			&& Arc.HasCapability(ECapability::SetArcTargetIntent)
			&& Arc.HasCapability(ECapability::IncreaseArcApex)
			&& Arc.HasCapability(ECapability::DecreaseArcApex)
			&& !Arc.HasCapability(ECapability::ClearArcTargetIntent));
	TestTrue(TEXT("Target presence alone enables clear"),
		Targeted.HasCapability(ECapability::ClearArcTargetIntent)
			&& Targeted.HasCapability(ECapability::SetArcTargetIntent));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRevisionlessIdentityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.RevisionlessIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRevisionlessIdentityTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	auto Later = Select(Initial, ETrajectory::BallisticArc);
	Later = Select(Later, ETrajectory::Straight);
	const auto FirstProjection = Project(Initial);
	const auto LaterProjection = Project(Later);

	TestTrue(TEXT("Fixture states have distinct revisions and identities"),
		Initial.GetRevision() == 0 && Later.GetRevision() == 2
			&& Initial.GetStateId() != Later.GetStateId());
	TestTrue(TEXT("Equal visible choices reproduce one read-model identity"),
		FirstProjection.Matches(LaterProjection)
			&& FirstProjection.GetReadModelId()
				== LaterProjection.GetReadModelId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionIntentEmissionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.IntentEmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionIntentEmissionTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto Straight = Project(Initial);
	const auto Arc = Project(Select(Initial, ETrajectory::BallisticArc));
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;

	TestTrue(TEXT("Straight projection emits only Arc selection"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
			TryEmitTrajectorySelection(
				Straight, ETrajectory::BallisticArc, Intent)
			&& Intent.IsValid()
			&& Intent.GetKind() == EIntentKind::SelectTrajectory
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
				TryEmitTrajectorySelection(
					Straight, ETrajectory::Straight, Intent)
			&& !Intent.IsValid());
	TestTrue(TEXT("Arc projection emits canonical target intent"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
			TryEmitArcTargetIntent(Arc, FVector2D(3.0, 4.0), Intent)
			&& Intent.GetKind() == EIntentKind::SetArcTargetIntent
			&& Intent.GetArcTargetIntent().Equals(FVector2D(0.6, 0.8)));
	TestTrue(TEXT("Arc projection emits canonical apex intent"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
			TryEmitArcApexAdjustment(Arc, 2.0, Intent)
			&& Intent.GetKind() == EIntentKind::AdjustArcApex
			&& Intent.GetArcApexAdjustmentDelta() == 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionSaturationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.SaturationAndClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionSaturationTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto ArcState = Select(Initial, ETrajectory::BallisticArc);
	const auto PositiveLimit = Project(AdjustApex(ArcState, 1.0));
	const auto NegativeLimit = Project(AdjustApex(ArcState, -1.0));
	const auto Targeted = Project(SetTarget(ArcState));
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;

	TestTrue(TEXT("Positive saturation disables only increase"),
		!PositiveLimit.HasCapability(ECapability::IncreaseArcApex)
			&& PositiveLimit.HasCapability(ECapability::DecreaseArcApex)
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
				TryEmitArcApexAdjustment(PositiveLimit, 0.25, Intent)
			&& !Intent.IsValid());
	TestTrue(TEXT("Negative saturation disables only decrease"),
		NegativeLimit.HasCapability(ECapability::IncreaseArcApex)
			&& !NegativeLimit.HasCapability(ECapability::DecreaseArcApex)
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
				TryEmitArcApexAdjustment(NegativeLimit, -0.25, Intent));
	TestTrue(TEXT("Only a targeted Arc emits clear"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
			TryEmitArcTargetClear(Targeted, Intent)
			&& Intent.GetKind() == EIntentKind::ClearArcTargetIntent
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
				TryEmitArcTargetClear(Project(ArcState), Intent)
			&& !Intent.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionReadOrderTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.ReadOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionReadOrderTest::RunTest(
	const FString&)
{
	int32 MissingResolves = 0;
	int32 MissingReads = 0;
	const auto Missing =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[&MissingResolves]()
			{
				++MissingResolves;
				return false;
			},
			[&MissingReads]()
			{
				++MissingReads;
				return Fdemo_mapShanmenThrownWeaponInputChoiceState::
					CreateInitial();
			});
	int32 ValidResolves = 0;
	int32 ValidReads = 0;
	const auto Valid =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[&ValidResolves]()
			{
				++ValidResolves;
				return true;
			},
			[&ValidReads]()
			{
				++ValidReads;
				return Fdemo_mapShanmenThrownWeaponInputChoiceState::
					CreateInitial();
			});

	TestTrue(TEXT("Unavailable source stops before state read"),
		Missing.IsValid()
			&& Missing.GetStatus() == EReadStatus::ChoiceSourceUnavailable
			&& MissingResolves == 1 && MissingReads == 0
			&& Missing.GetChoiceSourceResolutionCount() == 1
			&& Missing.GetChoiceStateReadCount() == 0
			&& Missing.GetProjectionCount() == 0);
	TestTrue(TEXT("Valid source is read and projected exactly once"),
		Valid.IsProjected() && ValidResolves == 1 && ValidReads == 1
			&& Valid.GetChoiceSourceResolutionCount() == 1
			&& Valid.GetChoiceStateReadCount() == 1
			&& Valid.GetProjectionCount() == 1
			&& Valid.GetReadModel().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionCompositionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort.CompositionAndPlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionCompositionTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto ReadModel = Project(Initial);
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
	const bool bEmitted =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::
			TryEmitTrajectorySelection(
				ReadModel, ETrajectory::BallisticArc, Intent);
	const bool bCaptured =
		Intent.TryCaptureCommand(Initial.GetRevision(), Command);
	const auto Reduced =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			Initial, Command);
	TestTrue(TEXT("Interaction emission composes with P20.19 and P20.10"),
		bEmitted && bCaptured && Reduced.IsSuccess()
			&& Reduced.DidChange()
			&& Reduced.State.GetTrajectoryKind()
				== ETrajectory::BallisticArc);

	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}
	const auto Boundary =
		Controller->ReadThrownWeaponInputChoiceInteraction();
	TestTrue(TEXT("Missing World/GameMode is one typed source rejection"),
		Boundary.IsValid() && !Boundary.IsProjected()
			&& Boundary.GetStatus()
				== EReadStatus::ChoiceSourceUnavailable
			&& Boundary.GetChoiceSourceResolutionCount() == 1
			&& Boundary.GetChoiceStateReadCount() == 0
			&& Boundary.GetProjectionCount() == 0);
	return true;
}

#endif
