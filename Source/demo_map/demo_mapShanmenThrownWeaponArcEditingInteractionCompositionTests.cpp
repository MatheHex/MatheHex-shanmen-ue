#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcEditingInteractionComposition.h"

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

#include <limits>

namespace
{
	using EIntentKind =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentKind;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FComposition =
		Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition;
	using FReadResult =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult;
	using FRequest =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest;

	constexpr EAutomationTestFlags CompositionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

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
			TryCaptureArcTargetIntent(
				State.GetRevision(), Target, Command));
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

	FReadResult Read(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return true; },
			[&State]() { return State; });
	}

	FReadResult MissingRead()
	{
		return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return false; },
			[]()
			{
				return Fdemo_mapShanmenThrownWeaponInputChoiceState();
			});
	}

	FReadResult InvalidStateRead()
	{
		return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
			[]() { return true; },
			[]()
			{
				return Fdemo_mapShanmenThrownWeaponInputChoiceState();
			});
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState CreateArc()
	{
		return Select(
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			ETrajectory::BallisticArc);
	}

	FRequest MakeValidTargetRequest()
	{
		FRequest Request;
		check(FComposition::TryComposeTargetRequest(
			Read(CreateArc()), FVector2D(3.0, 4.0), Request));
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionTargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.TargetRequest",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionTargetTest::RunTest(
	const FString&)
{
	const FReadResult Current = Read(CreateArc());
	FRequest Request;
	TestTrue(TEXT("one projected read composes a target request"),
		FComposition::TryComposeTargetRequest(
			Current, FVector2D(3.0, 4.0), Request));
	TestTrue(TEXT("composition preserves stale-safe identity and canonical intent"),
		Request.IsValid()
			&& Request.GetExpectedReadModelId()
				== Current.GetReadModel().GetReadModelId()
			&& Request.GetIntent().GetKind()
				== EIntentKind::SetArcTargetIntent
			&& Request.GetIntent().GetArcTargetIntent().Equals(
				FVector2D(0.6, 0.8), 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionApexTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.ApexRequest",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionApexTest::RunTest(
	const FString&)
{
	const FReadResult Current = Read(CreateArc());
	FRequest Request;
	TestTrue(TEXT("one projected read composes an apex adjustment request"),
		FComposition::TryComposeApexAdjustmentRequest(
			Current, 0.75, Request));
	TestTrue(TEXT("composition preserves stale-safe identity and apex intent"),
		Request.IsValid()
			&& Request.GetExpectedReadModelId()
				== Current.GetReadModel().GetReadModelId()
			&& Request.GetIntent().GetKind()
				== EIntentKind::AdjustArcApex
			&& Request.GetIntent().GetArcApexAdjustmentDelta() == 0.75);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionClearTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.ClearRequest",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionClearTest::RunTest(
	const FString&)
{
	const FReadResult Current = Read(SetTarget(CreateArc()));
	FRequest Request;
	TestTrue(TEXT("one targeted Arc read composes a clear request"),
		FComposition::TryComposeTargetClearRequest(Current, Request));
	TestTrue(TEXT("composition preserves stale-safe identity and clear intent"),
		Request.IsValid()
			&& Request.GetExpectedReadModelId()
				== Current.GetReadModel().GetReadModelId()
			&& Request.GetIntent().GetKind()
				== EIntentKind::ClearArcTargetIntent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionCapabilityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.CapabilityRejections",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionCapabilityTest::RunTest(
	const FString&)
{
	const FReadResult Straight = Read(
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial());
	FRequest Reused = MakeValidTargetRequest();
	const bool bStraightTargetRejected =
		!FComposition::TryComposeTargetRequest(
			Straight, FVector2D(1.0, 0.0), Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bStraightApexRejected =
		!FComposition::TryComposeApexAdjustmentRequest(
			Straight, 0.25, Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bStraightClearRejected =
		!FComposition::TryComposeTargetClearRequest(Straight, Reused)
		&& !Reused.IsValid();

	Reused = MakeValidTargetRequest();
	const bool bAbsentTargetClearRejected =
		!FComposition::TryComposeTargetClearRequest(Read(CreateArc()), Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bUpperApexRejected =
		!FComposition::TryComposeApexAdjustmentRequest(
			Read(AdjustApex(CreateArc(), 1.0)), 0.25, Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bLowerApexRejected =
		!FComposition::TryComposeApexAdjustmentRequest(
			Read(AdjustApex(CreateArc(), -1.0)), -0.25, Reused)
		&& !Reused.IsValid();

	TestTrue(TEXT("visible capabilities reject unavailable Arc operations"),
		bStraightTargetRejected && bStraightApexRejected
			&& bStraightClearRejected && bAbsentTargetClearRejected
			&& bUpperApexRejected && bLowerApexRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionPayloadTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.InvalidPayloadClearsOutput",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionPayloadTest::RunTest(
	const FString&)
{
	const FReadResult Current = Read(CreateArc());
	FRequest Reused = MakeValidTargetRequest();
	const bool bZeroTargetRejected =
		!FComposition::TryComposeTargetRequest(
			Current, FVector2D::ZeroVector, Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bZeroApexRejected =
		!FComposition::TryComposeApexAdjustmentRequest(
			Current, 0.0, Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bNaNApexRejected =
		!FComposition::TryComposeApexAdjustmentRequest(
			Current,
			std::numeric_limits<double>::quiet_NaN(),
			Reused)
		&& !Reused.IsValid();

	TestTrue(TEXT("invalid payloads fail closed and clear reused output"),
		bZeroTargetRejected && bZeroApexRejected && bNaNApexRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionReadTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.InvalidReadClearsOutput",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionReadTest::RunTest(
	const FString&)
{
	FRequest Reused = MakeValidTargetRequest();
	const bool bMissingTargetRejected =
		!FComposition::TryComposeTargetRequest(
			MissingRead(), FVector2D(1.0, 0.0), Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bInvalidApexRejected =
		!FComposition::TryComposeApexAdjustmentRequest(
			InvalidStateRead(), 0.25, Reused)
		&& !Reused.IsValid();
	Reused = MakeValidTargetRequest();
	const bool bMissingClearRejected =
		!FComposition::TryComposeTargetClearRequest(MissingRead(), Reused)
		&& !Reused.IsValid();

	TestTrue(TEXT("rejected reads fail closed before every Arc capture"),
		bMissingTargetRejected && bInvalidApexRejected
			&& bMissingClearRejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingInteractionRevisionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition.RevisionNeutrality",
	CompositionFlags)

bool Fdemo_mapThrownWeaponArcEditingInteractionRevisionTest::RunTest(
	const FString&)
{
	const auto FirstArc = CreateArc();
	auto ReenteredArc = Select(FirstArc, ETrajectory::Straight);
	ReenteredArc = Select(ReenteredArc, ETrajectory::BallisticArc);
	const FReadResult FirstRead = Read(FirstArc);
	const FReadResult ReenteredRead = Read(ReenteredArc);
	FRequest First;
	FRequest Reentered;
	TestTrue(TEXT("equal visible Arc choices compose across revisions"),
		FirstArc.GetRevision() == 1 && ReenteredArc.GetRevision() == 3
			&& FirstRead.GetReadModel().Matches(
				ReenteredRead.GetReadModel())
			&& FComposition::TryComposeTargetRequest(
				FirstRead, FVector2D(3.0, 4.0), First)
			&& FComposition::TryComposeTargetRequest(
				ReenteredRead, FVector2D(0.6, 0.8), Reentered));
	TestTrue(TEXT("revision-neutral reads reproduce one stale-safe request"),
		First.IsValid() && Reentered.IsValid()
			&& First.Matches(Reentered));
	return true;
}

#endif
