#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator.h"

#include "demo_mapPlayerController.h"

#include "Misc/AutomationTest.h"

namespace
{
	using ERequestStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus;
	using EIntentKind =
		Edemo_mapShanmenThrownWeaponInputChoiceIntentKind;
	using EControllerStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceControllerStatus;
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

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Project(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Model;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::
			TryProject(State, Model));
		return Model;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceIntent MakeTrajectoryIntent(
		const ETrajectory Trajectory)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
			TryCaptureTrajectorySelection(Trajectory, Intent));
		return Intent;
	}

	struct FRequestRouteProbe
	{
		bool bChoiceSourceAvailable = true;
		bool bReturnInvalidChoiceState = false;
		bool bGameplayInputAllowed = true;
		int32 InteractionReadCalls = 0;
		int32 IntentRouteCalls = 0;
		int32 SessionSubmissions = 0;
		Fdemo_mapShanmenThrownWeaponInputChoiceSession Session;

		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult Read()
		{
			++InteractionReadCalls;
			return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
				[this]()
				{
					return bChoiceSourceAvailable;
				},
				[this]()
				{
					return bReturnInvalidChoiceState
						? Fdemo_mapShanmenThrownWeaponInputChoiceState()
						: Session.GetState();
				});
		}

		Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult Route(
			const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent)
		{
			++IntentRouteCalls;
			return Fdemo_mapShanmenThrownWeaponInputChoiceIntentAdapter::Route(
				Intent,
				[this]()
				{
					return bChoiceSourceAvailable;
				},
				[this]()
				{
					return bReturnInvalidChoiceState
						? Fdemo_mapShanmenThrownWeaponInputChoiceState()
						: Session.GetState();
				},
				[this](
					const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
				{
					return Fdemo_mapShanmenThrownWeaponInputChoiceControllerAdapter::
						Route(
							Command,
							bGameplayInputAllowed,
							true,
							true,
							[]() { return true; },
							[this](
								const Fdemo_mapShanmenThrownWeaponInputChoiceCommand&
									Submitted)
							{
								++SessionSubmissions;
								return Session.Submit(
									Submitted, false, true);
							});
				});
		}

		bool SelectInSession(const ETrajectory Trajectory)
		{
			Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
			return Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureTrajectorySelection(
					Session.GetState().GetRevision(), Trajectory, Command)
				&& Session.Submit(Command, false, true).IsSuccess();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestCanonicalizationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.RequestCanonicalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestCanonicalizationTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto Arc = Project(Select(Initial, ETrajectory::BallisticArc));
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Raw;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Canonical;
	if (!TestTrue(TEXT("Raw target request captures"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureArcTargetIntent(Arc, FVector2D(3.0, 4.0), Raw))
		|| !TestTrue(TEXT("Canonical target request captures"),
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
				TryCaptureArcTargetIntent(
					Arc, FVector2D(0.6, 0.8), Canonical)))
	{
		return false;
	}

	TestTrue(TEXT("Equivalent visible edit reproduces request identity"),
		Raw.IsValid() && Canonical.IsValid()
			&& Raw.Matches(Canonical)
			&& Raw.GetRequestId() == Canonical.GetRequestId()
			&& Raw.GetExpectedReadModelId() == Arc.GetReadModelId()
			&& Raw.GetIntent().Matches(Canonical.GetIntent()));

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Reusable = Raw;
	const auto Straight = Project(Initial);
	TestTrue(TEXT("Disallowed visible operation fails closed and clears output"),
		!Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureTrajectorySelection(
				Straight, ETrajectory::Straight, Reusable)
			&& !Reusable.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestAllKindsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.AllRequestKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestAllKindsTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto Straight = Project(Initial);
	const auto ArcState = Select(Initial, ETrajectory::BallisticArc);
	const auto Arc = Project(ArcState);
	const auto Targeted = Project(SetTarget(ArcState));
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest SelectArc;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest SelectStraight;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Target;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Apex;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Clear;

	TestTrue(TEXT("All five visible edits capture immutable requests"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureTrajectorySelection(
				Straight, ETrajectory::BallisticArc, SelectArc)
			&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
				TryCaptureTrajectorySelection(
					Arc, ETrajectory::Straight, SelectStraight)
			&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
				TryCaptureArcTargetIntent(
					Arc, FVector2D(3.0, 4.0), Target)
			&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
				TryCaptureArcApexAdjustment(Arc, 2.0, Apex)
			&& Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
				TryCaptureArcTargetClear(Targeted, Clear));
	TestTrue(TEXT("Requests retain only their canonical P20.19 intent shape"),
		SelectArc.GetIntent().GetKind() == EIntentKind::SelectTrajectory
			&& SelectStraight.GetIntent().GetKind()
				== EIntentKind::SelectTrajectory
			&& Target.GetIntent().GetKind() == EIntentKind::SetArcTargetIntent
			&& Target.GetIntent().GetArcTargetIntent().Equals(
				FVector2D(0.6, 0.8))
			&& Apex.GetIntent().GetKind() == EIntentKind::AdjustArcApex
			&& Apex.GetIntent().GetArcApexAdjustmentDelta() == 1.0
			&& Clear.GetIntent().GetKind() == EIntentKind::ClearArcTargetIntent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestReadRejectionsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.ReadRejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestReadRejectionsTest::RunTest(
	const FString&)
{
	int32 InvalidReads = 0;
	int32 InvalidRoutes = 0;
	const auto Invalid =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest(),
				[&InvalidReads]()
				{
					++InvalidReads;
					return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult();
				},
				[&InvalidRoutes](
					const Fdemo_mapShanmenThrownWeaponInputChoiceIntent&)
				{
					++InvalidRoutes;
					return Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult();
				});
	TestTrue(TEXT("Invalid request stops before every callback"),
		Invalid.IsValid()
			&& Invalid.GetStatus() == ERequestStatus::RequestInvalid
			&& InvalidReads == 0 && InvalidRoutes == 0
			&& Invalid.GetInteractionReadCount() == 0
			&& Invalid.GetIntentRouteCount() == 0);

	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
		TryCaptureTrajectorySelection(
			Project(Initial), ETrajectory::BallisticArc, Request));
	FRequestRouteProbe MissingProbe;
	MissingProbe.bChoiceSourceAvailable = false;
	const auto Missing =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&MissingProbe]() { return MissingProbe.Read(); },
				[&MissingProbe](const auto& Intent)
				{
					return MissingProbe.Route(Intent);
				});
	FRequestRouteProbe BadStateProbe;
	BadStateProbe.bReturnInvalidChoiceState = true;
	const auto BadState =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&BadStateProbe]() { return BadStateProbe.Read(); },
				[&BadStateProbe](const auto& Intent)
				{
					return BadStateProbe.Route(Intent);
				});

	TestTrue(TEXT("Typed source and state read rejections stop before routing"),
		Missing.IsValid()
			&& Missing.GetStatus()
				== ERequestStatus::InteractionReadRejected
			&& BadState.IsValid()
			&& BadState.GetStatus()
				== ERequestStatus::InteractionReadRejected
			&& MissingProbe.InteractionReadCalls == 1
			&& BadStateProbe.InteractionReadCalls == 1
			&& MissingProbe.IntentRouteCalls == 0
			&& BadStateProbe.IntentRouteCalls == 0
			&& Missing.GetDiagnostic()
				== Missing.GetInteractionReadResult().GetDiagnostic()
			&& BadState.GetDiagnostic()
				== BadState.GetInteractionReadResult().GetDiagnostic());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestStaleFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.StaleReadModelFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestStaleFenceTest::RunTest(
	const FString&)
{
	FRequestRouteProbe Probe;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
		TryCaptureTrajectorySelection(
			Project(Probe.Session.GetState()),
			ETrajectory::BallisticArc,
			Request));
	check(Probe.SelectInSession(ETrajectory::BallisticArc));

	const auto Result =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&Probe]() { return Probe.Read(); },
				[&Probe](const auto& Intent)
				{
					return Probe.Route(Intent);
				});
	TestTrue(TEXT("Changed visible projection rejects stale request"),
		Result.IsValid()
			&& Result.GetStatus() == ERequestStatus::StaleReadModel
			&& Result.GetInteractionReadCount() == 1
			&& Result.GetIntentRouteCount() == 0
			&& Probe.InteractionReadCalls == 1
			&& Probe.IntentRouteCalls == 0
			&& Result.GetInteractionReadResult().GetReadModel().GetReadModelId()
				!= Request.GetExpectedReadModelId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestRevisionlessRouteTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.RevisionlessCurrentRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestRevisionlessRouteTest::RunTest(
	const FString&)
{
	FRequestRouteProbe Probe;
	const auto OriginalProjection = Project(Probe.Session.GetState());
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
		TryCaptureTrajectorySelection(
			OriginalProjection, ETrajectory::BallisticArc, Request));
	check(Probe.SelectInSession(ETrajectory::BallisticArc));
	check(Probe.SelectInSession(ETrajectory::Straight));
	const auto CurrentProjection = Project(Probe.Session.GetState());
	check(Probe.Session.GetState().GetRevision() == 2);

	const auto Result =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&Probe]() { return Probe.Read(); },
				[&Probe](const auto& Intent)
				{
					return Probe.Route(Intent);
				});
	TestTrue(TEXT("Equal visible state does not false-positive as stale"),
		OriginalProjection.Matches(CurrentProjection)
			&& OriginalProjection.GetReadModelId()
				== CurrentProjection.GetReadModelId()
			&& Result.IsAccepted()
			&& Result.GetStatus() == ERequestStatus::Routed);
	TestTrue(TEXT("P20.19 freezes the current revision after the fence"),
		Result.GetIntentResult().GetChoiceState().GetRevision() == 2
			&& Result.GetIntentResult().GetCommand().GetExpectedRevision() == 2
			&& Probe.Session.GetState().GetRevision() == 3
			&& Probe.Session.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Probe.InteractionReadCalls == 1
			&& Probe.IntentRouteCalls == 1
			&& Probe.SessionSubmissions == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.ProtocolAndDownstreamRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestProtocolTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
		TryCaptureTrajectorySelection(
			Project(Initial), ETrajectory::BallisticArc, Request));

	const auto InvalidRead =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[]()
				{
					return Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult();
				},
				[](const auto&)
				{
					return Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult();
				});
	FRequestRouteProbe InvalidRouteProbe;
	const auto InvalidRoute =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&InvalidRouteProbe]() { return InvalidRouteProbe.Read(); },
				[&InvalidRouteProbe](const auto&)
				{
					++InvalidRouteProbe.IntentRouteCalls;
					return Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult();
				});
	FRequestRouteProbe MismatchProbe;
	const auto Mismatch =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&MismatchProbe]() { return MismatchProbe.Read(); },
				[&MismatchProbe](const auto&)
				{
					return MismatchProbe.Route(
						MakeTrajectoryIntent(ETrajectory::Straight));
				});
	TestTrue(TEXT("Invalid or mismatched nested evidence fails protocol"),
		InvalidRead.IsValid()
			&& InvalidRead.GetStatus()
				== ERequestStatus::InteractionReadProtocolRejected
			&& InvalidRead.GetIntentRouteCount() == 0
			&& InvalidRoute.IsValid()
			&& InvalidRoute.GetStatus()
				== ERequestStatus::IntentRouteProtocolRejected
			&& Mismatch.IsValid()
			&& Mismatch.GetStatus()
				== ERequestStatus::IntentRouteProtocolRejected
			&& InvalidRouteProbe.IntentRouteCalls == 1
			&& MismatchProbe.IntentRouteCalls == 1);

	FRequestRouteProbe GameplayProbe;
	GameplayProbe.bGameplayInputAllowed = false;
	const auto Gameplay =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator::
			Execute(
				Request,
				[&GameplayProbe]() { return GameplayProbe.Read(); },
				[&GameplayProbe](const auto& Intent)
				{
					return GameplayProbe.Route(Intent);
				});
	TestTrue(TEXT("Valid downstream rejection remains typed routed evidence"),
		Gameplay.IsValid() && !Gameplay.IsAccepted()
			&& Gameplay.WasRejectedByIntentRoute()
			&& Gameplay.GetStatus() == ERequestStatus::Routed
			&& Gameplay.GetIntentResult().WasRejectedByController()
			&& Gameplay.GetIntentResult().GetControllerResult().GetStatus()
				== EControllerStatus::GameplayBlocked
			&& GameplayProbe.SessionSubmissions == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponChoiceInteractionRequestPlayerBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator.PlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponChoiceInteractionRequestPlayerBoundaryTest::RunTest(
	const FString&)
{
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
		TryCaptureTrajectorySelection(
			Project(Initial), ETrajectory::BallisticArc, Request));
	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}

	const auto Result =
		Controller->RouteThrownWeaponInputChoiceInteractionRequest(Request);
	TestTrue(TEXT("Missing World/GameMode rejects at the current-read fence"),
		Result.IsValid() && !Result.IsAccepted()
			&& Result.GetStatus()
				== ERequestStatus::InteractionReadRejected
			&& Result.GetInteractionReadCount() == 1
			&& Result.GetIntentRouteCount() == 0
			&& Result.GetInteractionReadResult().GetChoiceSourceResolutionCount()
				== 1
			&& Result.GetInteractionReadResult().GetChoiceStateReadCount() == 0
			&& !Result.GetIntentResult().IsValid());
	return true;
}

#endif
