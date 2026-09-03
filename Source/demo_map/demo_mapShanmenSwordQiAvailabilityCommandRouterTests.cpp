#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSwordQiAvailabilityCommandRouter.h"

#include "Misc/AutomationTest.h"

namespace
{
	constexpr EAutomationTestFlags SwordQiAvailabilityCommandFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordQiAvailabilityRun(0xD3870001, 0, 0, 1);
	Fdemo_mapShanmenSwordQiInputSample MakeSample()
	{
		Fdemo_mapShanmenSwordQiInputSample Sample;
		check(Fdemo_mapShanmenSwordQiInputSample::TryCapture(
			FVector(31.0, 41.0, 71.0),
			FVector(9.0, 4.0, 2.0),
			Sample));
		return Sample;
	}

	Fdemo_mapShanmenSwordQiInputResult MakePreRouteRejection(
		const FGuid& EventId)
	{
		Fdemo_mapShanmenSwordQiInputResult Input;
		Input.Status = Edemo_mapShanmenSwordQiInputStatus::GameplayBlocked;
		Input.InputEventId = EventId;
		Input.RunId = SwordQiAvailabilityRun;
		Input.Diagnostic = TEXT("Synthetic gameplay fence.");
		return Input;
	}

	Fdemo_mapShanmenSwordQiInputResult MakeProductRejection(
		const FGuid& EventId,
		const Fdemo_mapShanmenSwordQiInputSample& Sample,
		const Edemo_mapShanmenSwordQiProductRouteStatus RouteStatus)
	{
		Fdemo_mapShanmenSwordQiInputResult Input;
		Input.Status = Edemo_mapShanmenSwordQiInputStatus::ProductRejected;
		Input.bSpatialSampled = true;
		Input.bProductRouteInvoked = true;
		Input.InputEventId = EventId;
		Input.RunId = SwordQiAvailabilityRun;
		Input.IntentId = Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
			SwordQiAvailabilityRun,
			EventId);
		Input.Sample = Sample;
		Input.Product.Status =
			Edemo_mapShanmenSwordQiControllerStatus::RouteRejected;
		Input.Product.Route.Status = RouteStatus;
		Input.Diagnostic = TEXT("Synthetic product rejection.");
		return Input;
	}

	bool Project(
		Fdemo_mapShanmenSwordQiCommandEventOwner& Owner,
		Fdemo_mapShanmenSwordQiCommandAvailabilityProjection& OutProjection)
	{
		FString Diagnostic;
		return Owner.TryProjectAvailability(OutProjection, Diagnostic);
	}

	Fdemo_mapShanmenSwordQiAvailabilityCommand Capture(
		const Fdemo_mapShanmenSwordQiCommandAvailabilityProjection& Projection,
		const Edemo_mapShanmenSwordQiAvailabilityCommandKind Kind)
	{
		Fdemo_mapShanmenSwordQiAvailabilityCommand Command;
		check(Fdemo_mapShanmenSwordQiAvailabilityCommand::TryCapture(
			Projection.GetProjectionId(),
			Kind,
			Command));
		return Command;
	}

	bool BeginOwner(Fdemo_mapShanmenSwordQiCommandEventOwner& Owner)
	{
		FString Diagnostic;
		return Owner.TryBegin(SwordQiAvailabilityRun, Diagnostic);
	}

	Fdemo_mapShanmenSwordQiCommandEventResult MakePending(
		Fdemo_mapShanmenSwordQiCommandEventOwner& Owner)
	{
		const Fdemo_mapShanmenSwordQiInputSample Sample = MakeSample();
		return Owner.TryIssue(
			[Sample](const FGuid& EventId)
			{
				return MakeProductRejection(
					EventId,
					Sample,
					Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy);
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiAvailabilityCommandAdmissionTest,
	"Shanmen.0_0_10.Product.SwordQiAvailabilityCommandRouter.AdmissionFences",
	SwordQiAvailabilityCommandFlags)

bool Fdemo_mapSwordQiAvailabilityCommandAdmissionTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordQiAvailabilityCommand DefaultCommand;
	TestFalse(TEXT("default command fails closed"), DefaultCommand.IsValid());

	Fdemo_mapShanmenSwordQiAvailabilityCommand InvalidCapture;
	TestFalse(TEXT("invalid projection identity cannot be captured"),
		Fdemo_mapShanmenSwordQiAvailabilityCommand::TryCapture(
			FGuid(),
			Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue,
			InvalidCapture));
	TestFalse(TEXT("unknown command kind cannot be captured"),
		Fdemo_mapShanmenSwordQiAvailabilityCommand::TryCapture(
			FGuid(0xD3870002, 0, 0, 1),
			static_cast<Edemo_mapShanmenSwordQiAvailabilityCommandKind>(255),
			InvalidCapture));

	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Inactive;
	if (!Project(Owner, Inactive))
	{
		AddError(TEXT("Could not project the inactive P18.10 fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiAvailabilityCommand InactiveIssue =
		Capture(
			Inactive,
			Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue);
	int32 IssueCount = 0;
	int32 FrozenCount = 0;
	const auto Route = [&](
		const Fdemo_mapShanmenSwordQiAvailabilityCommand& Command)
	{
		return Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Command,
			[&IssueCount](const FGuid&)
			{
				++IssueCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			},
			[&FrozenCount](
				const FGuid&,
				const Fdemo_mapShanmenSwordQiInputSample&)
			{
				++FrozenCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			});
	};
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult InactiveResult =
		Route(InactiveIssue);
	TestTrue(TEXT("inactive owner rejects issue before callbacks"),
		InactiveResult.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
				CommandUnavailable
			&& IssueCount == 0
			&& FrozenCount == 0);

	if (!BeginOwner(Owner))
	{
		AddError(TEXT("Could not bind the P18.10 admission fixture."));
		return false;
	}
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Ready;
	if (!Project(Owner, Ready))
	{
		AddError(TEXT("Could not project issue-ready P18.10 state."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiAvailabilityCommand WrongRetry =
		Capture(Ready, Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry);
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult WrongResult =
		Route(WrongRetry);
	TestTrue(TEXT("state-incompatible retry is callback-free"),
		WrongResult.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
				CommandUnavailable
			&& IssueCount == 0
			&& FrozenCount == 0);

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult StaleResult =
		Route(InactiveIssue);
	TestTrue(TEXT("stale projection wins before capability checks"),
		StaleResult.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
				ProjectionStale
			&& StaleResult.Before.GetProjectionId()
				== Ready.GetProjectionId()
			&& IssueCount == 0
			&& FrozenCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiAvailabilityCommandIssueTest,
	"Shanmen.0_0_10.Product.SwordQiAvailabilityCommandRouter.IssueTransition",
	SwordQiAvailabilityCommandFlags)

bool Fdemo_mapSwordQiAvailabilityCommandIssueTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	if (!BeginOwner(Owner))
	{
		AddError(TEXT("Could not bind the P18.10 issue fixture."));
		return false;
	}
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Ready;
	if (!Project(Owner, Ready))
	{
		AddError(TEXT("Could not project the P18.10 issue fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiAvailabilityCommand Issue = Capture(
		Ready,
		Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue);
	int32 IssueCount = 0;
	int32 FrozenCount = 0;
	const auto FrozenRoute = [&FrozenCount](
		const FGuid&,
		const Fdemo_mapShanmenSwordQiInputSample&)
	{
		++FrozenCount;
		return Fdemo_mapShanmenSwordQiInputResult();
	};

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Blocked =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Issue,
			[&IssueCount](const FGuid& EventId)
			{
				++IssueCount;
				return MakePreRouteRejection(EventId);
			},
			FrozenRoute);
	TestTrue(TEXT("pre-route rejection is dispatched without consuming identity"),
		Blocked.IsDispatched()
			&& Blocked.CommandEvent.Status
				== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& !Blocked.CommandEvent.bEventCommitted
			&& Blocked.Before.GetProjectionId()
				== Blocked.After.GetProjectionId()
			&& Owner.GetNextEventSequence() == 1
			&& IssueCount == 1
			&& FrozenCount == 0);

	const Fdemo_mapShanmenSwordQiInputSample Sample = MakeSample();
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Busy =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Issue,
			[&IssueCount, Sample](const FGuid& EventId)
			{
				++IssueCount;
				return MakeProductRejection(
					EventId,
					Sample,
					Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy);
			},
			FrozenRoute);
	TestTrue(TEXT("HostBusy advances into one explicit pending state"),
		Busy.IsDispatched()
			&& Busy.CommandEvent.bEventCommitted
			&& Busy.CommandEvent.bPendingRetryStored
			&& Busy.After.CanRetry()
			&& Busy.After.GetProjectionId() != Ready.GetProjectionId()
			&& Owner.GetNextEventSequence() == 2
			&& IssueCount == 2
			&& FrozenCount == 0);

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Stale =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Issue,
			[&IssueCount](const FGuid&)
			{
				++IssueCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			},
			FrozenRoute);
	TestTrue(TEXT("consumed issue decision cannot cross pending transition"),
		Stale.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::ProjectionStale
			&& IssueCount == 2
			&& FrozenCount == 0
			&& Owner.HasPendingRetry());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiAvailabilityCommandRetryTest,
	"Shanmen.0_0_10.Product.SwordQiAvailabilityCommandRouter.RetryTransition",
	SwordQiAvailabilityCommandFlags)

bool Fdemo_mapSwordQiAvailabilityCommandRetryTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	if (!BeginOwner(Owner) || !MakePending(Owner).bPendingRetryStored)
	{
		AddError(TEXT("Could not create the P18.10 retry fixture."));
		return false;
	}
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Pending;
	if (!Project(Owner, Pending))
	{
		AddError(TEXT("Could not project pending P18.10 state."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiAvailabilityCommand Retry = Capture(
		Pending,
		Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry);
	const Fdemo_mapShanmenSwordQiAvailabilityCommand WrongIssue = Capture(
		Pending,
		Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue);
	int32 IssueCount = 0;
	int32 FrozenCount = 0;
	const auto IssueRoute = [&IssueCount](const FGuid&)
	{
		++IssueCount;
		return Fdemo_mapShanmenSwordQiInputResult();
	};
	const auto FrozenBusyRoute = [&FrozenCount](
		const FGuid& EventId,
		const Fdemo_mapShanmenSwordQiInputSample& FrozenSample)
	{
		++FrozenCount;
		return MakeProductRejection(
			EventId,
			FrozenSample,
			Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy);
	};

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Wrong =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			WrongIssue,
			IssueRoute,
			FrozenBusyRoute);
	TestTrue(TEXT("pending state blocks issue before either route"),
		Wrong.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
				CommandUnavailable
			&& IssueCount == 0
			&& FrozenCount == 0);

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult StillBusy =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Retry,
			IssueRoute,
			FrozenBusyRoute);
	TestTrue(TEXT("explicit HostBusy retry reuses the same availability ID"),
		StillBusy.IsDispatched()
			&& StillBusy.CommandEvent.bPendingRetryAttempt
			&& StillBusy.CommandEvent.bPendingRetryStored
			&& StillBusy.Before.GetProjectionId()
				== StillBusy.After.GetProjectionId()
			&& IssueCount == 0
			&& FrozenCount == 1);

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult RepeatedBusy =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Retry,
			IssueRoute,
			FrozenBusyRoute);
	TestTrue(TEXT("unchanged pending state permits another explicit retry"),
		RepeatedBusy.IsDispatched()
			&& RepeatedBusy.After.GetProjectionId()
				== Pending.GetProjectionId()
			&& IssueCount == 0
			&& FrozenCount == 2);

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Finished =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Retry,
			IssueRoute,
			[&FrozenCount](
				const FGuid& EventId,
				const Fdemo_mapShanmenSwordQiInputSample& FrozenSample)
			{
				++FrozenCount;
				return MakeProductRejection(
					EventId,
					FrozenSample,
					Edemo_mapShanmenSwordQiProductRouteStatus::
						ActionGateRejected);
			});
	TestTrue(TEXT("nonbusy routed result releases pending ownership"),
		Finished.IsDispatched()
			&& !Finished.CommandEvent.bPendingRetryStored
			&& Finished.After.CanIssue()
			&& Finished.After.GetProjectionId()
				!= Pending.GetProjectionId()
			&& !Owner.HasPendingRetry()
			&& IssueCount == 0
			&& FrozenCount == 3);

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Stale =
		Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Retry,
			IssueRoute,
			FrozenBusyRoute);
	TestTrue(TEXT("completed retry decision becomes callback-free stale input"),
		Stale.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::ProjectionStale
			&& IssueCount == 0
			&& FrozenCount == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiAvailabilityCommandCancelTest,
	"Shanmen.0_0_10.Product.SwordQiAvailabilityCommandRouter.CancelTransition",
	SwordQiAvailabilityCommandFlags)

bool Fdemo_mapSwordQiAvailabilityCommandCancelTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordQiCommandEventOwner Owner;
	const Fdemo_mapShanmenSwordQiCommandEventResult PendingEvent =
		BeginOwner(Owner)
			? MakePending(Owner)
			: Fdemo_mapShanmenSwordQiCommandEventResult();
	if (!PendingEvent.bPendingRetryStored)
	{
		AddError(TEXT("Could not create the P18.10 cancel fixture."));
		return false;
	}
	Fdemo_mapShanmenSwordQiCommandAvailabilityProjection Pending;
	if (!Project(Owner, Pending))
	{
		AddError(TEXT("Could not project the P18.10 cancel fixture."));
		return false;
	}
	const Fdemo_mapShanmenSwordQiAvailabilityCommand Cancel = Capture(
		Pending,
		Edemo_mapShanmenSwordQiAvailabilityCommandKind::Cancel);
	int32 IssueCount = 0;
	int32 FrozenCount = 0;
	const auto Route = [&]()
	{
		return Fdemo_mapShanmenSwordQiAvailabilityCommandRouter::TryRoute(
			Owner,
			Cancel,
			[&IssueCount](const FGuid&)
			{
				++IssueCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			},
			[&FrozenCount](
				const FGuid&,
				const Fdemo_mapShanmenSwordQiInputSample&)
			{
				++FrozenCount;
				return Fdemo_mapShanmenSwordQiInputResult();
			});
	};

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Cancelled = Route();
	TestTrue(TEXT("cancel uses only owner cancellation and returns its proof"),
		Cancelled.IsDispatched()
			&& Cancelled.Cancellation.IsValid()
			&& Cancelled.Cancellation.Request.Matches(PendingEvent.Request)
			&& Cancelled.After.CanIssue()
			&& Cancelled.After.GetNextEventSequence()
				== Pending.GetNextEventSequence()
			&& Cancelled.After.GetProjectionId()
				!= Pending.GetProjectionId()
			&& !Owner.HasPendingRetry()
			&& IssueCount == 0
			&& FrozenCount == 0);
	TestTrue(TEXT("caller snapshot remains immutable after cancellation"),
		Pending.IsValid()
			&& Pending.CanCancel()
			&& Pending.GetPendingEvent()
			&& Pending.GetPendingEvent()->GetInputEventId()
				== PendingEvent.Event.GetInputEventId());

	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Stale = Route();
	TestTrue(TEXT("cancel replay is rejected before callbacks or mutation"),
		Stale.Status
			== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::ProjectionStale
			&& IssueCount == 0
			&& FrozenCount == 0
			&& !Owner.HasPendingRetry());
	return true;
}

#endif
