#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritShieldSession.h"

namespace
{
	const FGuid SessionRunId(
		0xC9600001, 0xC9600002, 0xC9600003, 0xC9600004);
	const FGuid SessionOwnerId(
		0xC9610001, 0xC9610002, 0xC9610003, 0xC9610004);
	const FGuid SessionShieldSourceId(
		0xC9620001, 0xC9620002, 0xC9620003, 0xC9620004);
	const FGuid SessionAttackSourceId(
		0xC9630001, 0xC9630002, 0xC9630003, 0xC9630004);
	const FGuid SessionTimelineId(
		0xC9640001, 0xC9640002, 0xC9640003, 0xC9640004);
	const FGuid ForeignTimelineId(
		0xC9650001, 0xC9650002, 0xC9650003, 0xC9650004);

	FShanmenCombatActionSnapshot MakeSessionAction(
		uint64 ActivationSequence = 960,
		const FString& Digest = TEXT("TEST-DIGEST-P9.6-SHIELD"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = SessionRunId;
		Capture.OwnerId = SessionOwnerId;
		Capture.SourceEntityId = SessionShieldSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P9.6");
		Capture.Content.Digest = Digest;
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSpiritShieldDefinition MakeSessionDefinition()
	{
		FShanmenSpiritShieldDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Spell.SpiritShield.P9_6");
		Capture.MaximumCapacity = 30.0f;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.BlockedDamageTags.AddTag(
			FShanmenCombatNativeTags::DamageMental());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenSpiritShieldDefinition Definition;
		check(FShanmenSpiritShieldDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenActionResourceCost MakeSessionCost(float Amount = 20.0f)
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = TEXT("Cost.Spell.SpiritShield.P9_6");
		Capture.ResourceChannel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	FShanmenActionResourceAuthority MakeSessionAuthority(
		float CurrentAmount = 100.0f,
		float MaximumAmount = 100.0f)
	{
		FShanmenActionResourceAuthority Authority;
		check(FShanmenActionResourceAuthority::TryCreate(
			SessionShieldSourceId,
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
			CurrentAmount,
			MaximumAmount,
			0,
			Authority));
		return Authority;
	}

	FShanmenSpiritShieldSchedule MakeSessionSchedule(
		const FGuid& TimelineId = SessionTimelineId,
		int64 StartTick = 100,
		int64 DeadlineTick = 120)
	{
		FShanmenSpiritShieldSchedule Schedule;
		check(FShanmenSpiritShieldSchedule::TryCapture(
			TimelineId, StartTick, DeadlineTick, Schedule));
		return Schedule;
	}

	FShanmenSpiritShieldActionResult BeginSession(
		FShanmenActionResourceAuthority& Authority,
		FShanmenSpiritShieldSession& Session,
		const FShanmenCombatActionSnapshot& Action = MakeSessionAction(),
		const FShanmenSpiritShieldSchedule& Schedule = MakeSessionSchedule())
	{
		return FShanmenSpiritShieldSession::Begin(
			Action,
			MakeSessionDefinition(),
			MakeSessionCost(),
			Schedule,
			Authority,
			Session);
	}

	FShanmenCombatActionSnapshot MakeAttackAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = SessionRunId;
		Capture.OwnerId = FGuid(
			0xC9660001, 0xC9660002, 0xC9660003, 0xC9660004);
		Capture.SourceEntityId = SessionAttackSourceId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.P9_6Attack");
		Capture.Content.Version = TEXT("0.0.10.P9.6");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P9.6-ATTACK");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			961);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseLayer& ShieldLayer,
		float RawDamage,
		int32 HitOrdinal)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAttackAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = SessionShieldSourceId;
		Request.Candidate.DetectorId = TEXT("Detector.Test.P9_6Attack");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Request.Candidate.HitOrdinal = HitOrdinal;
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.P9_6Attack");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 6;
		Request.Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Request.Defense.Layers.Add(ShieldLayer);
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	FShanmenSpiritShieldCapacityCommitCommand MakeCapacityCommand(
		const FShanmenSpiritShieldProjectionReceipt& Projection,
		float RawDamage,
		int32 HitOrdinal)
	{
		const FShanmenImpactRequest Request = MakeImpactRequest(
			Projection.GetLayer(), RawDamage, HitOrdinal);
		const FShanmenImpactResult Result =
			FShanmenDefenseResolver::Resolve(Request);
		FShanmenSpiritShieldCapacityCommitCommand Command;
		check(FShanmenSpiritShieldCapacityCommitCommand::TryCreate(
			Projection, Request, Result, Command));
		return Command;
	}

	void ActivateSession(
		FShanmenActionResourceAuthority& Authority,
		FShanmenSpiritShieldSession& Session,
		const FShanmenCombatActionSnapshot& Action = MakeSessionAction())
	{
		check(BeginSession(Authority, Session, Action).IsSuccess());
		check(Session.Commit(Authority).IsSuccess());
		check(Session.IsValid());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionScheduleTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.ScheduleContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionScheduleTest::RunTest(const FString&)
{
	FShanmenSpiritShieldSchedule Invalid;
	TestFalse(TEXT("A timeline identity is mandatory"),
		FShanmenSpiritShieldSchedule::TryCapture(
			FGuid(), 100, 120, Invalid));
	TestFalse(TEXT("A schedule cannot start before the monotonic origin"),
		FShanmenSpiritShieldSchedule::TryCapture(
			SessionTimelineId, -1, 120, Invalid));
	TestFalse(TEXT("A deadline must be strictly after its start"),
		FShanmenSpiritShieldSchedule::TryCapture(
			SessionTimelineId, 120, 120, Invalid));

	const FShanmenSpiritShieldSchedule First = MakeSessionSchedule();
	const FShanmenSpiritShieldSchedule Replay = MakeSessionSchedule();
	const FShanmenSpiritShieldSchedule Later = MakeSessionSchedule(
		SessionTimelineId, 100, 121);
	TestTrue(TEXT("Equivalent schedule input reproduces one identity"),
		First.IsValid() && Replay.IsValid()
			&& First.GetScheduleId() == Replay.GetScheduleId()
			&& First.GetScheduleId() != Later.GetScheduleId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionActivationTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.AtomicActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionActivationTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeSessionAction();
	const FShanmenSpiritShieldSchedule Schedule = MakeSessionSchedule();
	FShanmenActionResourceAuthority Authority = MakeSessionAuthority();
	FShanmenSpiritShieldSession Session;
	const FShanmenSpiritShieldActionResult Begun = BeginSession(
		Authority, Session, Action, Schedule);
	TestTrue(TEXT("Begin reserves energy without publishing active authorities"),
		Begun.IsSuccess()
			&& Begun.Status == EShanmenSpiritShieldActionStatus::Begun
			&& Session.IsValid()
			&& !Session.HasActivatedAuthorities()
			&& FMath::IsNearlyEqual(Authority.GetReservedAmount(), 20.0f)
			&& Authority.GetAuthorityRevision() == 1);

	const FShanmenSpiritShieldActionResult Replay = BeginSession(
		Authority, Session, Action, Schedule);
	const FShanmenSpiritShieldActionResult ScheduleConflict = BeginSession(
		Authority,
		Session,
		Action,
		MakeSessionSchedule(SessionTimelineId, 100, 121));
	TestTrue(TEXT("Begin replays exactly and rejects a rewritten schedule"),
		Replay.Status == EShanmenSpiritShieldActionStatus::AlreadyBegun
			&& ScheduleConflict.IsValid()
			&& !ScheduleConflict.IsSuccess()
			&& ScheduleConflict.Error
				== EShanmenSpiritShieldActionError::CoordinatorConflict
			&& Authority.GetAuthorityRevision() == 1);

	const FShanmenSpiritShieldActionResult Activated =
		Session.Commit(Authority);
	TestTrue(TEXT("Commit atomically publishes capacity and deadline owners"),
		Activated.IsSuccess()
			&& Activated.Status
				== EShanmenSpiritShieldActionStatus::Activated
			&& Session.IsValid() && Session.HasActivatedAuthorities()
			&& Session.GetActionCoordinator().GetState()
				== EShanmenSpiritShieldActionState::Activated
			&& FMath::IsNearlyEqual(
				Session.GetCapacityAuthority().GetAvailableCapacity(), 30.0f)
			&& Session.GetDeadlineContract().GetTimelineId()
				== Schedule.GetTimelineId()
			&& Session.GetDeadlineContract().GetStartTick() == 100
			&& Session.GetDeadlineContract().GetDeadlineTick() == 120
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 80.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount())
			&& Authority.GetAuthorityRevision() == 2);

	const FGuid TerminalId = Activated.Terminal.GetReceiptId();
	const FShanmenSpiritShieldActionResult CommitReplay =
		Session.Commit(Authority);
	TestTrue(TEXT("Commit replay cannot recreate owners or spend twice"),
		CommitReplay.Status
				== EShanmenSpiritShieldActionStatus::AlreadyFinalized
			&& CommitReplay.Terminal.GetReceiptId() == TerminalId
			&& Session.GetCapacityAuthority().GetAuthorityRevision() == 0
			&& Authority.GetAuthorityRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionCapacityTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.CapacityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionCapacityTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeSessionAuthority();
	FShanmenSpiritShieldSession Session;
	ActivateSession(Authority, Session);

	FShanmenSpiritShieldProjectionReceipt FirstProjection;
	check(Session.TryProjectDefenseLayer(FirstProjection));
	const FShanmenSpiritShieldCapacityCommitCommand FirstCommand =
		MakeCapacityCommand(FirstProjection, 12.0f, 0);
	const FShanmenSpiritShieldCapacityCommitResult First =
		Session.CommitCapacity(FirstCommand);
	TestTrue(TEXT("Session commits only the exact projected mitigation"),
		First.IsSuccess()
			&& First.Status
				== EShanmenSpiritShieldCapacityCommitStatus::Committed
			&& FMath::IsNearlyEqual(
				Session.GetCapacityAuthority().GetAvailableCapacity(), 18.0f)
			&& Session.GetCapacityAuthority().GetAuthorityRevision() == 1);

	const FShanmenSpiritShieldCapacityCommitResult Replay =
		Session.CommitCapacity(FirstCommand);
	FShanmenSpiritShieldProjectionReceipt SecondProjection;
	check(Session.TryProjectDefenseLayer(SecondProjection));
	const FShanmenSpiritShieldCapacityCommitCommand Pending =
		MakeCapacityCommand(SecondProjection, 5.0f, 1);
	check(Session.Close(
		EShanmenSpiritShieldDeactivationReason::Explicit).IsSuccess());
	const FShanmenSpiritShieldCapacityCommitResult ClosedReplay =
		Session.CommitCapacity(FirstCommand);
	const FShanmenSpiritShieldCapacityCommitResult ClosedNew =
		Session.CommitCapacity(Pending);
	TestTrue(TEXT("Closed sessions permit proof replay but no new capacity write"),
		Replay.Status
				== EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted
			&& ClosedReplay.Status
				== EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted
			&& ClosedReplay.Receipt.GetReceiptId()
				== First.Receipt.GetReceiptId()
			&& ClosedNew.IsValid() && !ClosedNew.IsSuccess()
			&& ClosedNew.Error
				== EShanmenSpiritShieldCapacityCommitError::AuthorityNotReady
			&& Session.GetCapacityAuthority().GetAuthorityRevision() == 1
			&& FMath::IsNearlyEqual(
				Session.GetCapacityAuthority().GetAvailableCapacity(), 18.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionDeadlineTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.AtomicDeadlineClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionDeadlineTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeSessionAuthority();
	FShanmenSpiritShieldSession Session;
	ActivateSession(Authority, Session);
	FShanmenSpiritShieldTimelineObservation Early;
	check(FShanmenSpiritShieldTimelineObservation::TryCapture(
		SessionTimelineId, 119, Early));
	const FShanmenSpiritShieldDeadlineClosureResult NotDue =
		Session.ObserveDeadline(Early);
	TestTrue(TEXT("An early sample rejects without deactivating either owner"),
		NotDue.IsValid() && !NotDue.IsSuccess()
			&& NotDue.Error
				== EShanmenSpiritShieldDeadlineClosureError::DeadlineRejected
			&& NotDue.DeadlineError
				== EShanmenSpiritShieldDeadlineError::DeadlineNotReached
			&& Session.IsValid()
			&& Session.GetActionCoordinator().GetState()
				== EShanmenSpiritShieldActionState::Activated
			&& !Session.GetDeadlineGate().HasElapsed());

	FShanmenSpiritShieldTimelineObservation Due;
	check(FShanmenSpiritShieldTimelineObservation::TryCapture(
		SessionTimelineId, 120, Due));
	const FShanmenSpiritShieldDeadlineClosureResult Closed =
		Session.ObserveDeadline(Due);
	TestTrue(TEXT("Due observation closes deadline, shield and action atomically"),
		Closed.IsSuccess()
			&& Closed.Status
				== EShanmenSpiritShieldDeadlineClosureStatus::Closed
			&& Closed.DeadlineReceipt.GetDeactivation().GetReceiptId()
				== Closed.ClosureReceipt.GetDeactivation().GetReceiptId()
			&& Session.IsValid()
			&& Session.GetDeadlineGate().HasElapsed()
			&& Session.GetActionCoordinator().GetState()
				== EShanmenSpiritShieldActionState::Completed);

	const FShanmenSpiritShieldDeadlineClosureResult Replay =
		Session.ObserveDeadline(Due);
	FShanmenSpiritShieldTimelineObservation Conflicting;
	check(FShanmenSpiritShieldTimelineObservation::TryCapture(
		SessionTimelineId, 121, Conflicting));
	const FShanmenSpiritShieldDeadlineClosureResult Conflict =
		Session.ObserveDeadline(Conflicting);
	TestTrue(TEXT("Exact deadline replay is stable and later proof conflicts"),
		Replay.IsSuccess()
			&& Replay.Status
				== EShanmenSpiritShieldDeadlineClosureStatus::AlreadyClosed
			&& Replay.ClosureReceipt.GetReceiptId()
				== Closed.ClosureReceipt.GetReceiptId()
			&& Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.DeadlineError
				== EShanmenSpiritShieldDeadlineError::DeadlineConflict
			&& Session.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionExplicitCloseTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.ExplicitClosureBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionExplicitCloseTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeSessionAuthority();
	FShanmenSpiritShieldSession Session;
	ActivateSession(Authority, Session);
	const FShanmenSpiritShieldActionResult Bypass = Session.Close(
		EShanmenSpiritShieldDeactivationReason::DurationElapsed);
	TestTrue(TEXT("Direct close cannot impersonate a deadline observation"),
		Bypass.IsValid() && !Bypass.IsSuccess()
			&& Bypass.Error == EShanmenSpiritShieldActionError::InvalidInput
			&& Session.GetActionCoordinator().GetState()
				== EShanmenSpiritShieldActionState::Activated);

	const FShanmenSpiritShieldActionResult Closed = Session.Close(
		EShanmenSpiritShieldDeactivationReason::Explicit);
	FShanmenSpiritShieldTimelineObservation Due;
	check(FShanmenSpiritShieldTimelineObservation::TryCapture(
		SessionTimelineId, 120, Due));
	const FShanmenSpiritShieldDeadlineClosureResult LateDeadline =
		Session.ObserveDeadline(Due);
	const FShanmenSpiritShieldActionResult Replay = Session.Close(
		EShanmenSpiritShieldDeactivationReason::Explicit);
	TestTrue(TEXT("Explicit closure leaves deadline history unspent"),
		Closed.IsSuccess()
			&& Closed.Status == EShanmenSpiritShieldActionStatus::Completed
			&& !Session.GetDeadlineGate().HasElapsed()
			&& LateDeadline.IsValid() && !LateDeadline.IsSuccess()
			&& LateDeadline.Error
				== EShanmenSpiritShieldDeadlineClosureError::SessionNotReady
			&& Replay.Status
				== EShanmenSpiritShieldActionStatus::AlreadyClosed
			&& Session.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionAbortTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.PreCommitAbort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionAbortTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeSessionAuthority();
	FShanmenSpiritShieldSession Session;
	check(BeginSession(Authority, Session).IsSuccess());
	const FShanmenSpiritShieldActionResult Aborted = Session.Abort(
		EShanmenActionTerminalReason::Cancelled, Authority);
	const FShanmenSpiritShieldActionResult CommitAfterAbort =
		Session.Commit(Authority);
	TestTrue(TEXT("Pre-commit abort releases without creating active owners"),
		Aborted.IsSuccess()
			&& Aborted.Status == EShanmenSpiritShieldActionStatus::Aborted
			&& Session.IsValid() && !Session.HasActivatedAuthorities()
			&& Session.GetActionCoordinator().GetState()
				== EShanmenSpiritShieldActionState::Aborted
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount())
			&& Authority.GetAuthorityRevision() == 2
			&& CommitAfterAbort.IsValid()
			&& !CommitAfterAbort.IsSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionFailClosedTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.FailClosedAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionFailClosedTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Insufficient =
		MakeSessionAuthority(10.0f, 10.0f);
	FShanmenSpiritShieldSession Empty;
	const FShanmenSpiritShieldActionResult NoCapacity =
		BeginSession(Insufficient, Empty);
	TestTrue(TEXT("Failed Begin publishes neither session nor reservation"),
		NoCapacity.IsValid() && !NoCapacity.IsSuccess()
			&& !Empty.IsValid()
			&& Insufficient.GetAuthorityRevision() == 0
			&& FMath::IsNearlyZero(Insufficient.GetReservedAmount()));

	FShanmenActionResourceAuthority Authority = MakeSessionAuthority();
	FShanmenSpiritShieldSession Session;
	const FShanmenCombatActionSnapshot Action = MakeSessionAction();
	check(BeginSession(Authority, Session, Action).IsSuccess());
	FShanmenActionOrchestrator ExternalAction;
	FShanmenActionTransitionReceipt Transition;
	check(FShanmenActionOrchestrator::TryStart(
		Action, ExternalAction, Transition));
	check(ExternalAction.TryCancel(
		EShanmenCombatActionPhase::Startup, Transition));
	FShanmenActionResourceFinalizationRequest Release;
	check(FShanmenActionResourceFinalizationRequest::TryCreate(
		Session.GetActionCoordinator().GetStartupReceipt().GetReservation(),
		Transition,
		Release));
	check(Authority.Finalize(Release).IsSuccess());
	const FShanmenSpiritShieldActionResult Desynchronized =
		Session.Commit(Authority);
	TestTrue(TEXT("External history rewrite cannot half-publish activation"),
		Desynchronized.IsValid() && !Desynchronized.IsSuccess()
			&& Desynchronized.Error
				== EShanmenSpiritShieldActionError::ResourceFinalizationRejected
			&& Session.IsValid() && !Session.HasActivatedAuthorities()
			&& Session.GetActionCoordinator().GetState()
				== EShanmenSpiritShieldActionState::Reserved
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldSessionDeterminismTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldSession.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldSessionDeterminismTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeSessionAction(
		970, TEXT("TEST-DIGEST-P9.6-DETERMINISM"));
	FShanmenActionResourceAuthority FirstAuthority = MakeSessionAuthority();
	FShanmenActionResourceAuthority ReplayAuthority = MakeSessionAuthority();
	FShanmenSpiritShieldSession First;
	FShanmenSpiritShieldSession Replay;
	check(BeginSession(FirstAuthority, First, Action).IsSuccess());
	check(BeginSession(ReplayAuthority, Replay, Action).IsSuccess());
	const FShanmenSpiritShieldActionResult FirstActivated =
		First.Commit(FirstAuthority);
	const FShanmenSpiritShieldActionResult ReplayActivated =
		Replay.Commit(ReplayAuthority);
	check(FirstActivated.IsSuccess() && ReplayActivated.IsSuccess());

	FShanmenSpiritShieldProjectionReceipt FirstProjection;
	FShanmenSpiritShieldProjectionReceipt ReplayProjection;
	check(First.TryProjectDefenseLayer(FirstProjection));
	check(Replay.TryProjectDefenseLayer(ReplayProjection));
	const FShanmenSpiritShieldCapacityCommitResult FirstCapacity =
		First.CommitCapacity(MakeCapacityCommand(FirstProjection, 9.0f, 0));
	const FShanmenSpiritShieldCapacityCommitResult ReplayCapacity =
		Replay.CommitCapacity(MakeCapacityCommand(ReplayProjection, 9.0f, 0));
	check(FirstCapacity.IsSuccess() && ReplayCapacity.IsSuccess());
	FShanmenSpiritShieldTimelineObservation Due;
	check(FShanmenSpiritShieldTimelineObservation::TryCapture(
		SessionTimelineId, 120, Due));
	const FShanmenSpiritShieldDeadlineClosureResult FirstClosed =
		First.ObserveDeadline(Due);
	const FShanmenSpiritShieldDeadlineClosureResult ReplayClosed =
		Replay.ObserveDeadline(Due);
	TestTrue(TEXT("Equivalent aggregate input reproduces every proof identity"),
		FirstClosed.IsSuccess() && ReplayClosed.IsSuccess()
			&& First.GetSchedule().GetScheduleId()
				== Replay.GetSchedule().GetScheduleId()
			&& FirstActivated.Terminal.GetReceiptId()
				== ReplayActivated.Terminal.GetReceiptId()
			&& First.GetDeadlineContract().GetContractId()
				== Replay.GetDeadlineContract().GetContractId()
			&& FirstProjection.GetProjectionId()
				== ReplayProjection.GetProjectionId()
			&& FirstCapacity.Receipt.GetReceiptId()
				== ReplayCapacity.Receipt.GetReceiptId()
			&& FirstClosed.DeadlineReceipt.GetReceiptId()
				== ReplayClosed.DeadlineReceipt.GetReceiptId()
			&& FirstClosed.ClosureReceipt.GetReceiptId()
				== ReplayClosed.ClosureReceipt.GetReceiptId()
			&& First.IsValid() && Replay.IsValid());
	return true;
}

#endif
