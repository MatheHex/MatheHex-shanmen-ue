#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapShanmenThrownWeaponInputChoiceSession.h"

namespace
{
	using EChoiceSessionStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus;
	using EChoiceReduceStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	bool SubmitSequence(
		Fdemo_mapShanmenThrownWeaponInputChoiceSession& Session,
		const FVector2D& Target,
		const double ApexDelta)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				Session.GetState().GetRevision(),
				ETrajectory::BallisticArc,
				Command)
			|| !Session.Submit(Command, false, true).DidChange()
			|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcTargetIntent(
					Session.GetState().GetRevision(),
					Target,
					Command)
			|| !Session.Submit(Command, false, true).DidChange()
			|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcApexAdjustment(
					Session.GetState().GetRevision(),
					ApexDelta,
					Command)
			|| !Session.Submit(Command, false, true).DidChange())
		{
			return false;
		}
		return Session.IsValid();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceSessionLifecycleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceSession.LifecycleAndReadModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceSessionLifecycleTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceSession Session;
	TestTrue(TEXT("Session starts with one valid canonical read model"),
		Session.IsValid()
			&& Session.GetState().GetRevision() == 0
			&& Session.GetTrajectoryKind() == ETrajectory::Straight
			&& !Session.GetState().HasArcTargetIntent()
			&& Session.GetState().GetArcApexAdjustment() == 0.0);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Arc;
	if (!TestTrue(TEXT("Arc selection command captures"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Arc)))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Applied =
		Session.Submit(Arc, false, true);
	const FGuid AppliedStateId = Session.GetState().GetStateId();
	TestTrue(TEXT("Unlocked selection advances exactly once"),
		Applied.IsSuccess()
			&& Applied.DidChange()
			&& Applied.Status == EChoiceSessionStatus::Applied
			&& Applied.ReduceStatus == EChoiceReduceStatus::Reduced
			&& Applied.State.Matches(Session.GetState())
			&& Session.GetState().GetRevision() == 1
			&& Session.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Session.GetState().GetLastCommandId() == Arc.GetCommandId());

	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Replay =
		Session.Submit(Arc, true, false);
	TestTrue(TEXT("Exact retry is acknowledged while product state is locked"),
		Replay.IsSuccess()
			&& Replay.IsReplay()
			&& Replay.ReduceStatus == EChoiceReduceStatus::Replay
			&& Replay.State.GetStateId() == AppliedStateId
			&& Session.GetState().GetStateId() == AppliedStateId);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand SameArc;
	if (!TestTrue(TEXT("Fresh same-value selection captures"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				1, ETrajectory::BallisticArc, SameArc)))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult NoChange =
		Session.Submit(SameArc, true, false);
	TestTrue(TEXT("Fresh no-op is acknowledged without advancing while locked"),
		NoChange.IsSuccess()
			&& !NoChange.DidChange()
			&& NoChange.Status == EChoiceSessionStatus::NoChange
			&& NoChange.ReduceStatus == EChoiceReduceStatus::NoChange
			&& Session.GetState().GetRevision() == 1
			&& Session.GetState().GetStateId() == AppliedStateId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceSessionFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceSession.RunAndLifecycleFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceSessionFenceTest::RunTest(const FString&)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceSession Session;
	const FGuid InitialStateId = Session.GetState().GetStateId();
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Arc;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Arc))
	{
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult RunLocked =
		Session.Submit(Arc, true, true);
	TestTrue(TEXT("Active Run rejects a state-changing command"),
		!RunLocked.IsSuccess()
			&& RunLocked.Status == EChoiceSessionStatus::CombatRunActive
			&& RunLocked.ReduceStatus == EChoiceReduceStatus::Reduced
			&& !RunLocked.Diagnostic.IsEmpty()
			&& !RunLocked.State.IsValid()
			&& Session.GetState().GetStateId() == InitialStateId);

	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult LifecycleLocked =
		Session.Submit(Arc, false, false);
	TestTrue(TEXT("Stale lifecycle rejects the same state-changing command"),
		!LifecycleLocked.IsSuccess()
			&& LifecycleLocked.Status
				== EChoiceSessionStatus::ProductLifecycleNotEmpty
			&& LifecycleLocked.ReduceStatus == EChoiceReduceStatus::Reduced
			&& !LifecycleLocked.Diagnostic.IsEmpty()
			&& Session.GetState().GetStateId() == InitialStateId);

	if (!TestTrue(TEXT("Unlocked command can advance after both rejections"),
		Session.Submit(Arc, false, true).DidChange()))
	{
		return false;
	}
	const FGuid ArcStateId = Session.GetState().GetStateId();
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Target;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcTargetIntent(1, FVector2D(0.25, 0.5), Target))
	{
		return false;
	}
	TestTrue(TEXT("Run lock protects Arc-only choice state"),
		Session.Submit(Target, true, true).Status
			== EChoiceSessionStatus::CombatRunActive
			&& Session.GetState().GetStateId() == ArcStateId);
	TestTrue(TEXT("Lifecycle lock protects Arc-only choice state"),
		Session.Submit(Target, false, false).Status
			== EChoiceSessionStatus::ProductLifecycleNotEmpty
			&& Session.GetState().GetStateId() == ArcStateId);
	TestTrue(TEXT("Unlocked Arc target advances once"),
		Session.Submit(Target, false, true).DidChange()
			&& Session.GetState().GetRevision() == 2
			&& Session.GetState().HasArcTargetIntent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceSessionReductionFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceSession.ReductionFailurePropagation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceSessionReductionFailureTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceSession Session;
	const FGuid InitialStateId = Session.GetState().GetStateId();
	const Fdemo_mapShanmenThrownWeaponInputChoiceCommand Invalid;
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult InvalidResult =
		Session.Submit(Invalid, false, true);
	TestTrue(TEXT("Malformed command preserves exact reducer reason"),
		!InvalidResult.IsSuccess()
			&& InvalidResult.Status
				== EChoiceSessionStatus::ReductionRejected
			&& InvalidResult.ReduceStatus
				== EChoiceReduceStatus::CommandInvalid
			&& !InvalidResult.Diagnostic.IsEmpty()
			&& !InvalidResult.State.IsValid()
			&& Session.GetState().GetStateId() == InitialStateId);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Arc;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand StaleStraight;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::BallisticArc, Arc)
		|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(0, ETrajectory::Straight, StaleStraight)
		|| !Session.Submit(Arc, false, true).DidChange())
	{
		return false;
	}
	const FGuid ArcStateId = Session.GetState().GetStateId();
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Stale =
		Session.Submit(StaleStraight, false, true);
	TestTrue(TEXT("Stale command preserves revision-mismatch reason"),
		!Stale.IsSuccess()
			&& Stale.Status == EChoiceSessionStatus::ReductionRejected
			&& Stale.ReduceStatus == EChoiceReduceStatus::RevisionMismatch
			&& Session.GetState().GetStateId() == ArcStateId
			&& Session.GetState().GetRevision() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceSessionDeterminismTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceSession.DeterminismAndModeReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceSessionDeterminismTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceSession First;
	Fdemo_mapShanmenThrownWeaponInputChoiceSession Replay;
	Fdemo_mapShanmenThrownWeaponInputChoiceSession Different;
	if (!TestTrue(TEXT("Equivalent sessions accept the same command sequence"),
		SubmitSequence(First, FVector2D(3.0, 4.0), 0.75)
			&& SubmitSequence(Replay, FVector2D(0.6, 0.8), 0.75))
		|| !TestTrue(TEXT("Distinct target sequence remains valid"),
			SubmitSequence(Different, FVector2D(4.0, 3.0), 0.75)))
	{
		return false;
	}
	TestTrue(TEXT("Equivalent normalized sequences share one state identity"),
		First.GetState().Matches(Replay.GetState())
			&& First.GetState().GetRevision() == 3
			&& First.GetState().HasArcTargetIntent()
			&& First.GetState().GetArcApexAdjustment() == 0.75);
	TestFalse(TEXT("Different target sequence has a different state identity"),
		First.GetState().Matches(Different.GetState()));

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand StraightFirst;
	Fdemo_mapShanmenThrownWeaponInputChoiceCommand StraightReplay;
	if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(3, ETrajectory::Straight, StraightFirst)
		|| !Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(3, ETrajectory::Straight, StraightReplay)
		|| !First.Submit(StraightFirst, false, true).DidChange()
		|| !Replay.Submit(StraightReplay, false, true).DidChange())
	{
		return false;
	}
	TestTrue(TEXT("Straight transition deterministically clears Arc-only state"),
		First.GetState().Matches(Replay.GetState())
			&& First.GetState().GetRevision() == 4
			&& First.GetTrajectoryKind() == ETrajectory::Straight
			&& !First.GetState().HasArcTargetIntent()
			&& First.GetState().GetArcTargetIntent() == FVector2D::ZeroVector
			&& First.GetState().GetArcApexAdjustment() == 0.0);
	return true;
}

#endif
