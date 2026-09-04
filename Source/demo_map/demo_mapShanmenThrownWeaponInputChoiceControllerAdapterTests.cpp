#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputChoiceControllerAdapter.h"

#include "demo_mapPlayerController.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EControllerStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus;
	using ESessionStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus;
	using EReduceStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand MakeTrajectoryCommand(
		const uint64 ExpectedRevision = 0,
		const ETrajectory Trajectory = ETrajectory::BallisticArc)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				ExpectedRevision, Trajectory, Command));
		return Command;
	}

	struct FRouteProbe
	{
		bool bSessionAvailable = true;
		bool bCombatRunActive = false;
		bool bProductLifecycleEmpty = true;
		bool bReturnDefaultResult = false;
		bool bCorruptReduceStatus = false;
		int32 ResolutionCount = 0;
		int32 SubmissionCount = 0;
		FGuid SubmittedCommandId;
		uint64 SubmittedExpectedRevision = MAX_uint64;
		Fdemo_mapShanmenThrownWeaponInputChoiceSession Session;

		Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult Route(
			const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command,
			const bool bGameplayInputAllowed = true,
			const bool bGameplaySurface = true,
			const bool bGameOnlyInputMode = true)
		{
			return Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter::
				Route(
					Command,
					bGameplayInputAllowed,
					bGameplaySurface,
					bGameOnlyInputMode,
					[this]()
					{
						++ResolutionCount;
						return bSessionAvailable;
					},
					[this](
						const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&
							Submitted)
					{
						++SubmissionCount;
						SubmittedCommandId = Submitted.GetCommandId();
						SubmittedExpectedRevision =
							Submitted.GetExpectedRevision();
						if (bReturnDefaultResult)
						{
							return Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult();
						}
						auto Result = Session.Submit(
							Submitted,
							bCombatRunActive,
							bProductLifecycleEmpty);
						if (bCorruptReduceStatus)
						{
							Result.ReduceStatus = EReduceStatus::NoChange;
						}
						return Result;
					});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerGateOrderTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.GateOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerGateOrderTest::RunTest(
	const FString&)
{
	FRouteProbe Probe;
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand InvalidCommand;
	const auto Invalid = Probe.Route(InvalidCommand, false, false, false);
	const auto Command = MakeTrajectoryCommand();
	const auto Gameplay = Probe.Route(Command, false, false, false);
	const auto Surface = Probe.Route(Command, true, false, false);
	const auto Mode = Probe.Route(Command, true, true, false);
	TestTrue(TEXT("Malformed command fails before every local gate"),
		Invalid.IsValid()
			&& Invalid.GetStatus() == EControllerStatus::CommandInvalid
			&& !Invalid.GetCommand().IsValid());
	TestTrue(TEXT("Gameplay, surface, and input mode reject in fixed order"),
		Gameplay.IsValid()
			&& Gameplay.GetStatus() == EControllerStatus::GameplayBlocked
			&& Surface.IsValid()
			&& Surface.GetStatus() == EControllerStatus::InputSurfaceBlocked
			&& Mode.IsValid()
			&& Mode.GetStatus() == EControllerStatus::InputModeBlocked);
	TestTrue(TEXT("Local rejection never resolves or submits the session"),
		Probe.ResolutionCount == 0
			&& Probe.SubmissionCount == 0
			&& Gameplay.GetChoiceSessionResolutionCount() == 0
			&& Surface.GetChoiceSessionSubmissionCount() == 0
			&& Mode.GetChoiceSessionResolutionCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerAvailabilityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.SessionAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerAvailabilityTest::RunTest(
	const FString&)
{
	FRouteProbe Probe;
	Probe.bSessionAvailable = false;
	const auto Command = MakeTrajectoryCommand();
	const auto Result = Probe.Route(Command);
	TestTrue(TEXT("Missing GameMode session is a typed valid rejection"),
		Result.IsValid()
			&& !Result.IsAccepted()
			&& !Result.WasRejectedBySession()
			&& Result.GetStatus()
				== EControllerStatus::ChoiceSessionUnavailable);
	TestTrue(TEXT("Availability resolves once and stops before submission"),
		Probe.ResolutionCount == 1
			&& Probe.SubmissionCount == 0
			&& Result.GetChoiceSessionResolutionCount() == 1
			&& Result.GetChoiceSessionSubmissionCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.ApplyReplayNoChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerReplayTest::RunTest(
	const FString&)
{
	FRouteProbe Probe;
	const auto Arc = MakeTrajectoryCommand();
	const auto Applied = Probe.Route(Arc);
	const auto Replay = Probe.Route(Arc);
	const auto FreshNoOp = MakeTrajectoryCommand(1);
	const auto NoChange = Probe.Route(FreshNoOp);
	TestTrue(TEXT("First submission advances the sole session once"),
		Applied.IsValid() && Applied.IsAccepted()
			&& Applied.GetStatus() == EControllerStatus::Delegated
			&& Applied.GetSessionResult().Status == ESessionStatus::Applied
			&& Applied.GetSessionResult().ReduceStatus
				== EReduceStatus::Reduced
			&& Applied.GetSessionResult().State.GetRevision() == 1);
	TestTrue(TEXT("Exact command retry preserves session replay evidence"),
		Replay.IsValid() && Replay.IsAccepted()
			&& Replay.GetSessionResult().Status == ESessionStatus::Replay
			&& Replay.GetSessionResult().ReduceStatus
				== EReduceStatus::Replay
			&& Replay.GetSessionResult().State.GetLastCommandId()
				== Arc.GetCommandId());
	TestTrue(TEXT("Fresh equal value preserves no-change evidence"),
		NoChange.IsValid() && NoChange.IsAccepted()
			&& NoChange.GetSessionResult().Status
				== ESessionStatus::NoChange
			&& NoChange.GetSessionResult().ReduceStatus
				== EReduceStatus::NoChange
			&& NoChange.GetSessionResult().State.GetRevision() == 1);
	TestTrue(TEXT("Each eligible event resolves and submits exactly once"),
		Probe.ResolutionCount == 3
			&& Probe.SubmissionCount == 3
			&& Probe.SubmittedCommandId == FreshNoOp.GetCommandId()
			&& Probe.SubmittedExpectedRevision == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerCommandKindsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.AllCommandKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerCommandKindsTest::RunTest(
	const FString&)
{
	FRouteProbe Probe;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
	if (!TestTrue(TEXT("Arc trajectory command routes"),
		Probe.Route(MakeTrajectoryCommand()).IsAccepted())
		|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				1, FVector2D(0.6, 0.8), Command)
		|| !TestTrue(TEXT("Arc target command routes"),
			Probe.Route(Command).IsAccepted())
		|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(2, 0.5, Command)
		|| !TestTrue(TEXT("Arc apex command routes"),
			Probe.Route(Command).IsAccepted())
		|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetClear(3, Command)
		|| !TestTrue(TEXT("Arc target clear command routes"),
			Probe.Route(Command).IsAccepted())
		|| !TestTrue(TEXT("Straight trajectory command routes"),
			Probe.Route(MakeTrajectoryCommand(4, ETrajectory::Straight)).
				IsAccepted()))
	{
		return false;
	}
	TestTrue(TEXT("All command kinds share one session and five transitions"),
		Probe.ResolutionCount == 5
			&& Probe.SubmissionCount == 5
			&& Probe.Session.GetState().GetRevision() == 5
			&& Probe.Session.GetTrajectoryKind() == ETrajectory::Straight
			&& !Probe.Session.GetState().HasArcTargetIntent()
			&& Probe.Session.GetState().GetArcApexAdjustment() == 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerSessionRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.SessionRejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerSessionRejectionTest::RunTest(
	const FString&)
{
	const auto Arc = MakeTrajectoryCommand();
	FRouteProbe RunLocked;
	RunLocked.bCombatRunActive = true;
	const auto RunResult = RunLocked.Route(Arc);
	FRouteProbe LifecycleLocked;
	LifecycleLocked.bProductLifecycleEmpty = false;
	const auto LifecycleResult = LifecycleLocked.Route(Arc);
	TestTrue(TEXT("Run and lifecycle fences retain exact session reasons"),
		RunResult.IsValid() && RunResult.WasRejectedBySession()
			&& RunResult.GetSessionResult().Status
				== ESessionStatus::CombatRunActive
			&& RunResult.GetSessionResult().ReduceStatus
				== EReduceStatus::Reduced
			&& LifecycleResult.IsValid()
			&& LifecycleResult.WasRejectedBySession()
			&& LifecycleResult.GetSessionResult().Status
				== ESessionStatus::ProductLifecycleNotEmpty
			&& LifecycleResult.GetSessionResult().ReduceStatus
				== EReduceStatus::Reduced);

	FRouteProbe Stale;
	if (!TestTrue(TEXT("Stale setup advances once"),
		Stale.Route(Arc).IsAccepted()))
	{
		return false;
	}
	const auto StaleStraight =
		MakeTrajectoryCommand(0, ETrajectory::Straight);
	const auto StaleResult = Stale.Route(StaleStraight);
	FRouteProbe ModeMismatch;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand ArcTarget;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcTargetIntent(
			0, FVector2D(0.25, 0.5), ArcTarget))
	{
		return false;
	}
	const auto ModeResult = ModeMismatch.Route(ArcTarget);
	TestTrue(TEXT("Reducer conflicts remain typed session rejections"),
		StaleResult.IsValid() && StaleResult.WasRejectedBySession()
			&& StaleResult.GetSessionResult().Status
				== ESessionStatus::ReductionRejected
			&& StaleResult.GetSessionResult().ReduceStatus
				== EReduceStatus::RevisionMismatch
			&& ModeResult.IsValid() && ModeResult.WasRejectedBySession()
			&& ModeResult.GetSessionResult().Status
				== ESessionStatus::ReductionRejected
			&& ModeResult.GetSessionResult().ReduceStatus
				== EReduceStatus::ModeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.SessionProtocol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerProtocolTest::RunTest(
	const FString&)
{
	const auto Command = MakeTrajectoryCommand();
	FRouteProbe MissingEvidence;
	MissingEvidence.bReturnDefaultResult = true;
	const auto Missing = MissingEvidence.Route(Command);
	FRouteProbe CorruptEvidence;
	CorruptEvidence.bCorruptReduceStatus = true;
	const auto Corrupt = CorruptEvidence.Route(Command);
	TestTrue(TEXT("Default and internally inconsistent evidence fail protocol"),
		Missing.IsValid() && !Missing.IsAccepted()
			&& Missing.GetStatus()
				== EControllerStatus::SessionProtocolRejected
			&& Corrupt.IsValid() && !Corrupt.IsAccepted()
			&& Corrupt.GetStatus()
				== EControllerStatus::SessionProtocolRejected);
	TestTrue(TEXT("Protocol rejection still records one synchronous submission"),
		MissingEvidence.ResolutionCount == 1
			&& MissingEvidence.SubmissionCount == 1
			&& CorruptEvidence.ResolutionCount == 1
			&& CorruptEvidence.SubmissionCount == 1
			&& Missing.GetChoiceSessionResolutionCount() == 1
			&& Corrupt.GetChoiceSessionSubmissionCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceControllerPlayerBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter.PlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceControllerPlayerBoundaryTest::RunTest(
	const FString&)
{
	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}
	const auto Command = MakeTrajectoryCommand();
	const auto Result =
		Controller->RouteThrownWeaponInputChoiceCommand(Command);
	TestTrue(TEXT("Uninitialized controller mode blocks before GameMode"),
		Result.IsValid()
			&& Result.GetStatus() == EControllerStatus::InputModeBlocked
			&& Controller->GetInputSurfaceState() == TEXT("Gameplay")
			&& Controller->GetInputModeState() == TEXT("Uninitialized"));
	TestTrue(TEXT("Controller boundary preserves command and performs no I/O"),
		Result.GetCommand().GetCommandId() == Command.GetCommandId()
			&& Result.GetCommand().GetExpectedRevision()
				== Command.GetExpectedRevision()
			&& Result.GetChoiceSessionResolutionCount() == 0
			&& Result.GetChoiceSessionSubmissionCount() == 0);
	return true;
}

#endif
