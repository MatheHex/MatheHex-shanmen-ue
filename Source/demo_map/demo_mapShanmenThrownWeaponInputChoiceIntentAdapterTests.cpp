#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputChoiceIntentAdapter.h"

#include "demo_mapPlayerController.h"

#include "Misc/AutomationTest.h"

#include <limits>

namespace
{
	using EIntentKind =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentKind;
	using EIntentStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentStatus;
	using EControllerStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus;
	using ESessionStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus;
	using EReduceStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceIntent MakeTrajectoryIntent(
		const ETrajectory Trajectory = ETrajectory::BallisticArc)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureTrajectorySelection(Trajectory, Intent));
		return Intent;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceIntent MakeTargetIntent(
		const FVector2D& Target = FVector2D(0.6, 0.8))
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcTargetIntent(Target, Intent));
		return Intent;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceIntent MakeApexIntent(
		const double Delta = 0.5)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcApexAdjustment(Delta, Intent));
		return Intent;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceIntent MakeClearIntent()
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcTargetClear(Intent));
		return Intent;
	}

	struct FIntentRouteProbe
	{
		bool bChoiceSourceAvailable = true;
		bool bReturnInvalidChoiceState = false;
		bool bGameplayInputAllowed = true;
		bool bGameplaySurface = true;
		bool bGameOnlyInputMode = true;
		bool bCombatRunActive = false;
		bool bProductLifecycleEmpty = true;
		bool bPreApplySameCommand = false;
		bool bPreApplyArcCommand = false;
		bool bReturnDefaultControllerResult = false;
		bool bReturnDifferentControllerCommand = false;
		int32 ChoiceSourceResolutionCount = 0;
		int32 ChoiceStateReadCount = 0;
		int32 ControllerRouteCount = 0;
		int32 ControllerSessionResolutionCount = 0;
		int32 ControllerSessionSubmissionCount = 0;
		TArray<uint64> CapturedExpectedRevisions;
		TArray<FGuid> CapturedCommandIds;
		Fdemo_mapShanmenThrownWeaponInputChoiceSession Session;

		Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult
		RouteThroughController(
			const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
		{
			if (bReturnDefaultControllerResult)
			{
				return Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult();
			}
			if (bReturnDifferentControllerCommand)
			{
				Fdemo_mapShanmenThrownWeaponInputChoiceCommand Other;
				const ETrajectory OtherTrajectory =
					Command.GetTrajectoryKind() == ETrajectory::Straight
						? ETrajectory::BallisticArc
						: ETrajectory::Straight;
				if (!Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
					TryCaptureTrajectorySelection(
						Command.GetExpectedRevision(),
						OtherTrajectory,
						Other))
				{
					return Fdemo_mapShanmenThrownWeaponInputChoiceControllerResult();
				}
				Fdemo_mapShanmenThrownWeaponInputChoiceSession OtherSession;
				return Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter::
					Route(
						Other,
						true,
						true,
						true,
						[]() { return true; },
						[&OtherSession](
							const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&
								Submitted)
						{
							return OtherSession.Submit(Submitted, false, true);
						});
			}

			if (bPreApplySameCommand)
			{
				Session.Submit(Command, false, true);
			}
			if (bPreApplyArcCommand)
			{
				Fdemo_mapShanmenThrownWeaponInputChoiceCommand Arc;
				if (Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
					TryCaptureTrajectorySelection(
						Command.GetExpectedRevision(),
						ETrajectory::BallisticArc,
						Arc))
				{
					Session.Submit(Arc, false, true);
				}
			}

			return Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter::
				Route(
					Command,
					bGameplayInputAllowed,
					bGameplaySurface,
					bGameOnlyInputMode,
					[this]()
					{
						++ControllerSessionResolutionCount;
						return true;
					},
					[this](
						const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&
							Submitted)
					{
						++ControllerSessionSubmissionCount;
						return Session.Submit(
							Submitted,
							bCombatRunActive,
							bProductLifecycleEmpty);
					});
		}

		Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult Route(
			const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent)
		{
			return Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter::Route(
				Intent,
				[this]()
				{
					++ChoiceSourceResolutionCount;
					return bChoiceSourceAvailable;
				},
				[this]()
				{
					++ChoiceStateReadCount;
					return bReturnInvalidChoiceState
						? Fdemo_mapShanmenThrownWeaponInputChoiceState()
						: Session.GetState();
				},
				[this](
					const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&
						Command)
				{
					++ControllerRouteCount;
					CapturedExpectedRevisions.Add(
						Command.GetExpectedRevision());
					CapturedCommandIds.Add(Command.GetCommandId());
					return RouteThroughController(Command);
				});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentCanonicalizationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.IntentCanonicalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentCanonicalizationTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Target;
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent EqualTarget;
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Apex;
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Clear;
	if (!TestTrue(TEXT("Raw target enters canonical logical intent"),
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureArcTargetIntent(FVector2D(3.0, 4.0), Target))
		|| !TestTrue(TEXT("Equal canonical target captures"),
			Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcTargetIntent(
					FVector2D(0.6, 0.8), EqualTarget))
		|| !TestTrue(TEXT("Apex clamps into canonical range"),
			Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcApexAdjustment(2.0, Apex))
		|| !TestTrue(TEXT("Clear intent captures"),
			Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcTargetClear(Clear)))
	{
		return false;
	}

	TestTrue(TEXT("Equivalent target values reproduce one intent identity"),
		Target.IsValid() && EqualTarget.IsValid()
			&& Target.Matches(EqualTarget)
			&& Target.GetIntentId() == EqualTarget.GetIntentId()
			&& Target.GetArcTargetIntent().Equals(FVector2D(0.6, 0.8)));
	TestTrue(TEXT("Apex and clear retain disjoint canonical shapes"),
		Apex.IsValid() && Clear.IsValid()
			&& Apex.GetKind() == EIntentKind::AdjustArcApex
			&& Apex.GetArcApexAdjustmentDelta() == 1.0
			&& Clear.GetKind() == EIntentKind::ClearArcTargetIntent
			&& Apex.GetIntentId() != Clear.GetIntentId());

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
	TestTrue(TEXT("Intent freezes caller-supplied current revision once"),
		Target.TryCaptureCommand(42, Command)
			&& Command.IsValid()
			&& Command.GetExpectedRevision() == 42
			&& Command.GetKind()
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					SetArcTargetIntent
			&& Command.GetArcTargetIntent().Equals(
				Target.GetArcTargetIntent()));

	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Reusable = Apex;
	const double NaN = std::numeric_limits<double>::quiet_NaN();
	TestTrue(TEXT("Invalid raw logical values fail closed and clear output"),
		!Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureTrajectorySelection(
				ETrajectory::Invalid, Reusable)
			&& !Reusable.IsValid()
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcTargetIntent(FVector2D::ZeroVector, Reusable)
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcTargetIntent(FVector2D(NaN, 1.0), Reusable)
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcApexAdjustment(0.0, Reusable)
			&& !Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureArcApexAdjustment(NaN, Reusable)
			&& !Reusable.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentSourceOrderTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.SourceOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentSourceOrderTest::RunTest(
	const FString&)
{
	FIntentRouteProbe InvalidProbe;
	const auto Invalid = InvalidProbe.Route(
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent());
	FIntentRouteProbe MissingProbe;
	MissingProbe.bChoiceSourceAvailable = false;
	const auto Missing = MissingProbe.Route(MakeTrajectoryIntent());
	FIntentRouteProbe BadStateProbe;
	BadStateProbe.bReturnInvalidChoiceState = true;
	const auto BadState = BadStateProbe.Route(MakeTrajectoryIntent());

	TestTrue(TEXT("Invalid intent stops before authoritative source"),
		Invalid.IsValid()
			&& Invalid.GetStatus() == EIntentStatus::IntentInvalid
			&& InvalidProbe.ChoiceSourceResolutionCount == 0
			&& InvalidProbe.ChoiceStateReadCount == 0
			&& InvalidProbe.ControllerRouteCount == 0);
	TestTrue(TEXT("Missing source resolves once without reading state"),
		Missing.IsValid()
			&& Missing.GetStatus()
				== EIntentStatus::ChoiceSourceUnavailable
			&& MissingProbe.ChoiceSourceResolutionCount == 1
			&& MissingProbe.ChoiceStateReadCount == 0
			&& MissingProbe.ControllerRouteCount == 0);
	TestTrue(TEXT("Invalid authoritative state is read once and fails closed"),
		BadState.IsValid()
			&& BadState.GetStatus() == EIntentStatus::ChoiceStateInvalid
			&& BadStateProbe.ChoiceSourceResolutionCount == 1
			&& BadStateProbe.ChoiceStateReadCount == 1
			&& BadStateProbe.ControllerRouteCount == 0
			&& BadState.GetCommandCaptureCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentAllKindsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.AllIntentKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentAllKindsTest::RunTest(
	const FString&)
{
	FIntentRouteProbe Probe;
	const auto Arc = Probe.Route(MakeTrajectoryIntent());
	const auto Target = Probe.Route(MakeTargetIntent());
	const auto Apex = Probe.Route(MakeApexIntent());
	const auto Clear = Probe.Route(MakeClearIntent());
	const auto Straight = Probe.Route(
		MakeTrajectoryIntent(ETrajectory::Straight));

	TestTrue(TEXT("All five logical edits are accepted in order"),
		Arc.IsAccepted() && Target.IsAccepted() && Apex.IsAccepted()
			&& Clear.IsAccepted() && Straight.IsAccepted()
			&& Arc.GetCommand().GetKind()
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					SelectTrajectory
			&& Target.GetCommand().GetKind()
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					SetArcTargetIntent
			&& Apex.GetCommand().GetKind()
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					AdjustArcApex
			&& Clear.GetCommand().GetKind()
				== Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::
					ClearArcTargetIntent);
	TestTrue(TEXT("Each edit reads and freezes the current revision once"),
		Probe.ChoiceSourceResolutionCount == 5
			&& Probe.ChoiceStateReadCount == 5
			&& Probe.ControllerRouteCount == 5
			&& Probe.CapturedExpectedRevisions
				== TArray<uint64>({0, 1, 2, 3, 4})
			&& Probe.CapturedCommandIds.Num() == 5);
	TestTrue(TEXT("Only the P20.11 session owns the five transitions"),
		Probe.ControllerSessionResolutionCount == 5
			&& Probe.ControllerSessionSubmissionCount == 5
			&& Probe.Session.GetState().GetRevision() == 5
			&& Probe.Session.GetTrajectoryKind() == ETrajectory::Straight
			&& !Probe.Session.GetState().HasArcTargetIntent()
			&& Probe.Session.GetState().GetArcApexAdjustment() == 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentReplayConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.NoChangeReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentReplayConflictTest::RunTest(
	const FString&)
{
	FIntentRouteProbe NoChangeProbe;
	const auto NoChange = NoChangeProbe.Route(
		MakeTrajectoryIntent(ETrajectory::Straight));
	FIntentRouteProbe ReplayProbe;
	ReplayProbe.bPreApplySameCommand = true;
	const auto Replay = ReplayProbe.Route(MakeTrajectoryIntent());
	FIntentRouteProbe ConflictProbe;
	ConflictProbe.bPreApplyArcCommand = true;
	const auto Conflict = ConflictProbe.Route(
		MakeTrajectoryIntent(ETrajectory::Straight));

	TestTrue(TEXT("Fresh equal intent preserves typed no-change evidence"),
		NoChange.IsAccepted()
			&& NoChange.GetControllerResult().GetSessionResult().Status
				== ESessionStatus::NoChange
			&& NoChange.GetControllerResult().GetSessionResult().ReduceStatus
				== EReduceStatus::NoChange
			&& NoChange.GetChoiceState().GetRevision() == 0
			&& NoChange.GetCommand().GetExpectedRevision() == 0);
	TestTrue(TEXT("Concurrent exact application preserves replay evidence"),
		Replay.IsAccepted()
			&& Replay.GetControllerResult().GetSessionResult().Status
				== ESessionStatus::Replay
			&& Replay.GetControllerResult().GetSessionResult().ReduceStatus
				== EReduceStatus::Replay
			&& Replay.GetChoiceState().GetRevision() == 0
			&& ReplayProbe.Session.GetState().GetRevision() == 1);
	TestTrue(TEXT("Concurrent different edit preserves revision conflict"),
		Conflict.IsValid() && !Conflict.IsAccepted()
			&& Conflict.WasRejectedByController()
			&& Conflict.GetControllerResult().WasRejectedBySession()
			&& Conflict.GetControllerResult().GetSessionResult().Status
				== ESessionStatus::ReductionRejected
			&& Conflict.GetControllerResult().GetSessionResult().ReduceStatus
				== EReduceStatus::RevisionMismatch
			&& Conflict.GetCommand().GetExpectedRevision() == 0
			&& ConflictProbe.Session.GetState().GetRevision() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentRejectionsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.ControllerRejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentRejectionsTest::RunTest(
	const FString&)
{
	FIntentRouteProbe GameplayProbe;
	GameplayProbe.bGameplayInputAllowed = false;
	const auto Gameplay = GameplayProbe.Route(MakeTrajectoryIntent());
	FIntentRouteProbe RunProbe;
	RunProbe.bCombatRunActive = true;
	const auto Run = RunProbe.Route(MakeTrajectoryIntent());
	FIntentRouteProbe LifecycleProbe;
	LifecycleProbe.bProductLifecycleEmpty = false;
	const auto Lifecycle = LifecycleProbe.Route(MakeTrajectoryIntent());

	TestTrue(TEXT("P20.18 gameplay gate remains the sole local gate"),
		Gameplay.IsValid() && Gameplay.WasRejectedByController()
			&& Gameplay.GetStatus() == EIntentStatus::Delegated
			&& Gameplay.GetControllerResult().GetStatus()
				== EControllerStatus::GameplayBlocked
			&& GameplayProbe.ControllerSessionResolutionCount == 0
			&& GameplayProbe.ControllerSessionSubmissionCount == 0);
	TestTrue(TEXT("Run and lifecycle session fences remain typed"),
		Run.IsValid() && Run.WasRejectedByController()
			&& Run.GetControllerResult().WasRejectedBySession()
			&& Run.GetControllerResult().GetSessionResult().Status
				== ESessionStatus::CombatRunActive
			&& Lifecycle.IsValid()
			&& Lifecycle.WasRejectedByController()
			&& Lifecycle.GetControllerResult().WasRejectedBySession()
			&& Lifecycle.GetControllerResult().GetSessionResult().Status
				== ESessionStatus::ProductLifecycleNotEmpty);
	TestTrue(TEXT("Each eligible intent still captures and delegates once"),
		Gameplay.GetChoiceStateReadCount() == 1
			&& Gameplay.GetCommandCaptureCount() == 1
			&& Gameplay.GetControllerRouteCount() == 1
			&& RunProbe.ControllerSessionSubmissionCount == 1
			&& LifecycleProbe.ControllerSessionSubmissionCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.ControllerProtocol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentProtocolTest::RunTest(
	const FString&)
{
	FIntentRouteProbe MissingProbe;
	MissingProbe.bReturnDefaultControllerResult = true;
	const auto Missing = MissingProbe.Route(MakeTrajectoryIntent());
	FIntentRouteProbe MismatchProbe;
	MismatchProbe.bReturnDifferentControllerCommand = true;
	const auto Mismatch = MismatchProbe.Route(MakeTrajectoryIntent());

	TestTrue(TEXT("Missing and wrong-command P20.18 evidence fail protocol"),
		Missing.IsValid() && !Missing.IsAccepted()
			&& Missing.GetStatus()
				== EIntentStatus::ControllerProtocolRejected
			&& Mismatch.IsValid() && !Mismatch.IsAccepted()
			&& Mismatch.GetStatus()
				== EIntentStatus::ControllerProtocolRejected
			&& Mismatch.GetControllerResult().IsValid()
			&& Mismatch.GetControllerResult().GetCommand().GetCommandId()
				!= Mismatch.GetCommand().GetCommandId());
	TestTrue(TEXT("Protocol rejection records one synchronous route only"),
		MissingProbe.ChoiceSourceResolutionCount == 1
			&& MissingProbe.ChoiceStateReadCount == 1
			&& MissingProbe.ControllerRouteCount == 1
			&& MismatchProbe.ChoiceSourceResolutionCount == 1
			&& MismatchProbe.ChoiceStateReadCount == 1
			&& MismatchProbe.ControllerRouteCount == 1
			&& Missing.GetControllerRouteCount() == 1
			&& Mismatch.GetControllerRouteCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputChoiceIntentPlayerBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter.PlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputChoiceIntentPlayerBoundaryTest::RunTest(
	const FString&)
{
	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}
	const auto Intent = MakeTrajectoryIntent();
	const auto Result = Controller->RouteThrownWeaponInputChoiceIntent(Intent);
	TestTrue(TEXT("Missing World/GameMode is a typed source rejection"),
		Result.IsValid() && !Result.IsAccepted()
			&& Result.GetStatus()
				== EIntentStatus::ChoiceSourceUnavailable
			&& Controller->GetInputSurfaceState() == TEXT("Gameplay")
			&& Controller->GetInputModeState() == TEXT("Uninitialized"));
	TestTrue(TEXT("Controller boundary stops before state, command, or route"),
		Result.GetIntent().Matches(Intent)
			&& Result.GetChoiceSourceResolutionCount() == 1
			&& Result.GetChoiceStateReadCount() == 0
			&& Result.GetCommandCaptureCount() == 0
			&& Result.GetControllerRouteCount() == 0);
	return true;
}

#endif
