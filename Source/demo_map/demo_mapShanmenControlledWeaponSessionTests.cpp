#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenControlledWeaponSession.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemTags.h"

namespace
{
	const FGuid RunId(0xD3620001, 0, 0, 1);
	const FGuid OwnerId(0xD3620002, 0, 0, 1);
	const FGuid SourceEntityId(0xD3620003, 0, 0, 1);
	const FGuid ItemId(0xD3620004, 0, 0, 1);
	const FGuid TargetA(0xD3620005, 0, 0, 1);

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P6.2");
		Content.Digest = TEXT("P6.2.ControlledWeaponProductSession.v1");
		return Content;
	}

	FShanmenControlledWeaponDefinition MakeDefinition(
		FName DetectorId = TEXT("Detector.ControlledWeapon.P6.2.FlyingSword"))
	{
		FShanmenControlledWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = DetectorId;
		Capture.FormulaId = TEXT("Formula.ControlledWeapon.P6.2.Test");
		Capture.BaseDamage = 12.0f;
		Capture.ControlPowerCoefficient = 0.2f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenControlledWeaponDefinition Definition;
		check(FShanmenControlledWeaponDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	Fdemo_mapShanmenControlledWeaponPrepareResult MakePrepared(
		uint64 ActivationSequence = 1)
	{
		Fdemo_mapShanmenControlledWeaponPrepareResult Prepared;
		Prepared.Status =
			Edemo_mapShanmenControlledWeaponPrepareStatus::Prepared;
		Prepared.Evidence.CorrelationId = FGuid(0xD3620010, 0, 0, 1);
		Prepared.Evidence.ActiveRunId = RunId;
		Prepared.Evidence.OwnerId = OwnerId;
		Prepared.Evidence.ItemInstanceId = ItemId;
		Prepared.Evidence.ItemDefinitionId =
			TEXT("Item.Test.FlyingSword.P6.2");
		Prepared.Evidence.DeploymentReservationId =
			FGuid(0xD3620011, 0, 0, 1);
		Prepared.Evidence.AuthorityRevision = 8;
		Prepared.Evidence.ItemRevision = 3;
		Prepared.Evidence.Content = MakeContent();

		FShanmenCombatActionCapture ActionCapture;
		ActionCapture.RunId = RunId;
		ActionCapture.OwnerId = OwnerId;
		ActionCapture.SourceEntityId = SourceEntityId;
		ActionCapture.SourceItemInstanceId = ItemId;
		ActionCapture.ActionDefinitionId =
			FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
		ActionCapture.Content = MakeContent();
		ActionCapture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		ActionCapture.SourceTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponFlyingSword());
		ActionCapture.ActivationId =
			FShanmenCombatIdFactory::MakeActivationId(
				RunId,
				SourceEntityId,
				ActionCapture.ActionDefinitionId,
				ActivationSequence);
		check(FShanmenCombatActionSnapshot::TryCapture(
			ActionCapture, Prepared.Action));
		Prepared.Definition = MakeDefinition();
		check(FShanmenControlledWeaponOffenseSnapshot::TryCapture(
			40.0f, Prepared.Offense));
		check(FShanmenControlledWeaponExecution::TryCreate(
			Prepared.Action,
			Prepared.Definition,
			Prepared.Offense,
			Prepared.Execution));
		check(Prepared.IsPrepared());
		return Prepared;
	}

	bool StartSession(
		Fdemo_mapShanmenControlledWeaponSession& OutSession)
	{
		FShanmenActionTransitionReceipt Startup;
		FShanmenActionTransitionReceipt Active;
		return Fdemo_mapShanmenControlledWeaponSession::TryStart(
			MakePrepared(), OutSession, Startup, Active);
	}

	FShanmenHitCandidate MakeCandidate(
		const FShanmenWorldHitContext& Context,
		const FGuid& TargetId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = Context.GetAction().GetActivationId();
		Candidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
		Candidate.TargetEntityId = TargetId;
		Candidate.DetectorId = Context.GetDetectorId();
		Candidate.DetectorKind = Context.GetDetectorKind();
		Candidate.HitOrdinal = Context.GetHitOrdinal();
		Candidate.HitLocation = FVector(100.0, 20.0, 10.0);
		Candidate.HitNormal = FVector::BackwardVector;
		return Candidate;
	}

	FShanmenTargetVitalitySnapshot MakeVitality()
	{
		FShanmenTargetVitalitySnapshot Vitality;
		Vitality.CurrentVitality = 100.0f;
		Vitality.MaximumVitality = 100.0f;
		return Vitality;
	}

	FShanmenDefenseSnapshot MakeDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponSessionLifecycleTest,
	"Shanmen.0_0_10.Product.ControlledWeaponSession.LifecycleAndCommands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponSessionLifecycleTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenControlledWeaponPrepareResult Prepared =
		MakePrepared();
	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;
	TestTrue(TEXT("Prepared authority evidence starts one active session"),
		Fdemo_mapShanmenControlledWeaponSession::TryStart(
			Prepared, Session, Startup, Active));
	TestTrue(TEXT("Start crosses the canonical action commit point"),
		Session.IsActive()
		&& Startup.IsValid()
		&& Active.IsValid()
		&& Active.CrossedCommitPointNow()
		&& Session.GetEvidence().ItemInstanceId == ItemId
		&& Session.GetExecution().GetState()
			== EShanmenControlledWeaponState::Orbiting);

	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Sequence zero launches the same physical sword"),
		Session.TryIssueControl(
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));
	const FGuid LaunchCommandId = Command.GetCommandId();
	TestTrue(TEXT("Exact launch replay remains idempotent"),
		Session.TryIssueControl(
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector(2.0, 0.0, 0.0),
			Command)
		&& Command.GetCommandId() == LaunchCommandId
		&& Session.GetExecution().GetNextCommandSequence() == 1);
	TestTrue(TEXT("Sequence one redirects without replacing the item"),
		Session.TryIssueControl(
			1,
			EShanmenControlledWeaponCommandKind::Redirect,
			FVector::RightVector,
			Command)
		&& Command.GetSourceItemInstanceId() == ItemId);

	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestTrue(TEXT("Recall completes the action lifecycle atomically"),
		Session.TryRecallAndComplete(
			2, Recall, Recovery, Completed));
	TestTrue(TEXT("Completed session has no live detector or action"),
		Session.IsTerminal()
		&& Session.GetState()
			== Edemo_mapShanmenControlledWeaponSessionState::Completed
		&& Recall.GetStateAfter()
			== EShanmenControlledWeaponState::Recalled
		&& Recovery.GetToPhase()
			== EShanmenCombatActionPhase::Recovery
		&& Completed.GetTerminalReason()
			== EShanmenActionTerminalReason::Completed
		&& !Session.GetExecution().IsEmissionActive());
	TestFalse(TEXT("Completed session rejects further control"),
		Session.TryIssueControl(
			3,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponSessionImpactTest,
	"Shanmen.0_0_10.Product.ControlledWeaponSession.ContactWindows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponSessionImpactTest::RunTest(const FString&)
{
	Fdemo_mapShanmenControlledWeaponSession Session;
	if (!StartSession(Session))
	{
		AddError(TEXT("Could not start P6.2 contact-window session."));
		return false;
	}
	FShanmenControlledWeaponCommandReceipt Command;
	TestTrue(TEXT("Flying sword launches before contact sampling"),
		Session.TryIssueControl(
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));
	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("Directed session opens a ControlledObject window"),
		Session.TryBeginContactWindow(FirstContext)
		&& FirstContext.GetDetectorKind()
			== EShanmenHitDetectorKind::ControlledObject);

	const FShanmenHitCandidate FirstCandidate =
		MakeCandidate(FirstContext, TargetA);
	FShanmenControlledWeaponImpactReceipt Impact;
	TestTrue(TEXT("Session resolves through the frozen controlled runtime"),
		Session.TryResolveCandidate(
			FirstCandidate,
			MakeVitality(),
			MakeDefense(),
			Impact));
	const FGuid FirstImpactId = Impact.GetResult().ImpactId;
	TestTrue(TEXT("Impact remains item-bound and conserved"),
		Impact.IsValid()
		&& Impact.GetRequest().Action.GetSourceItemInstanceId() == ItemId
		&& Impact.GetResult().IsConserved()
		&& FMath::IsNearlyEqual(Impact.GetResult().RawDamage, 20.0f));
	TestFalse(TEXT("Same target callback cannot mutate the ledger twice"),
		Session.TryResolveCandidate(
			FirstCandidate,
			MakeVitality(),
			MakeDefense(),
			Impact));
	TestEqual(TEXT("Rejected duplicate leaves one accepted impact"),
		Session.GetExecution().NumAcceptedImpacts(), 1);
	TestTrue(TEXT("First contact window closes"),
		Session.TryEndContactWindow());

	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("A later window advances the stable ordinal"),
		Session.TryBeginContactWindow(SecondContext)
		&& SecondContext.GetHitOrdinal() == 1);
	TestTrue(TEXT("Same target can resolve in the later window"),
		Session.TryResolveCandidate(
			MakeCandidate(SecondContext, TargetA),
			MakeVitality(),
			MakeDefense(),
			Impact)
		&& Impact.GetResult().ImpactId != FirstImpactId);

	FShanmenActionTransitionReceipt Interrupted;
	TestTrue(TEXT("Interrupt closes an open detector and action together"),
		Session.TryInterrupt(Interrupted));
	TestTrue(TEXT("Interrupted session is terminal and clean"),
		Session.IsTerminal()
		&& Session.GetState()
			== Edemo_mapShanmenControlledWeaponSessionState::Interrupted
		&& Interrupted.GetTerminalReason()
			== EShanmenActionTerminalReason::Interrupted
		&& !Session.GetExecution().IsEmissionActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponSessionAtomicFailureTest,
	"Shanmen.0_0_10.Product.ControlledWeaponSession.AtomicFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponSessionAtomicFailureTest::RunTest(const FString&)
{
	Fdemo_mapShanmenControlledWeaponSession Session;
	if (!StartSession(Session))
	{
		AddError(TEXT("Could not start P6.2 atomic-failure session."));
		return false;
	}
	FShanmenControlledWeaponCommandReceipt Command;
	TestFalse(TEXT("Redirect before launch fails without consuming sequence"),
		Session.TryIssueControl(
			0,
			EShanmenControlledWeaponCommandKind::Redirect,
			FVector::RightVector,
			Command));
	TestTrue(TEXT("Failed redirect leaves orbiting sequence zero"),
		Session.IsActive()
		&& Session.GetExecution().GetState()
			== EShanmenControlledWeaponState::Orbiting
		&& Session.GetExecution().GetNextCommandSequence() == 0);
	TestFalse(TEXT("Skipped launch sequence fails closed"),
		Session.TryIssueControl(
			1,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));
	TestTrue(TEXT("Valid launch still owns sequence zero"),
		Session.TryIssueControl(
			0,
			EShanmenControlledWeaponCommandKind::Launch,
			FVector::ForwardVector,
			Command));
	FShanmenWorldHitContext Context;
	TestTrue(TEXT("Open contact window prepares recall-race check"),
		Session.TryBeginContactWindow(Context));

	FShanmenControlledWeaponCommandReceipt Recall;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;
	TestFalse(TEXT("Recall cannot race an open contact window"),
		Session.TryRecallAndComplete(
			1, Recall, Recovery, Completed));
	TestTrue(TEXT("Rejected recall leaves every subsystem active and unchanged"),
		Session.IsActive()
		&& Session.GetExecution().IsEmissionActive()
		&& Session.GetExecution().GetState()
			== EShanmenControlledWeaponState::Directed
		&& Session.GetExecution().GetNextCommandSequence() == 1
		&& Session.GetActionRuntime().GetPhase()
			== EShanmenCombatActionPhase::Active);
	TestTrue(TEXT("Window can still close after rejected recall"),
		Session.TryEndContactWindow());
	TestTrue(TEXT("Then recall and completion succeed"),
		Session.TryRecallAndComplete(
			1, Recall, Recovery, Completed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponSessionTamperTest,
	"Shanmen.0_0_10.Product.ControlledWeaponSession.TamperFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapControlledWeaponSessionTamperTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenControlledWeaponPrepareResult Canonical =
		MakePrepared();
	Fdemo_mapShanmenControlledWeaponSession Session;
	FShanmenActionTransitionReceipt Startup;
	FShanmenActionTransitionReceipt Active;

	Fdemo_mapShanmenControlledWeaponPrepareResult ContentTamper = Canonical;
	ContentTamper.Evidence.Content.Digest = TEXT("tampered-but-valid");
	TestFalse(TEXT("Evidence content cannot diverge from action content"),
		ContentTamper.IsPrepared());
	TestFalse(TEXT("Session rejects content-divergent evidence"),
		Fdemo_mapShanmenControlledWeaponSession::TryStart(
			ContentTamper, Session, Startup, Active));

	Fdemo_mapShanmenControlledWeaponPrepareResult DefinitionTamper =
		Canonical;
	DefinitionTamper.Definition =
		MakeDefinition(TEXT("Detector.ControlledWeapon.P6.2.Tampered"));
	TestFalse(TEXT("Definition cannot diverge from frozen execution"),
		DefinitionTamper.IsPrepared());

	Fdemo_mapShanmenControlledWeaponPrepareResult OffenseTamper = Canonical;
	check(FShanmenControlledWeaponOffenseSnapshot::TryCapture(
		41.0f, OffenseTamper.Offense));
	TestFalse(TEXT("Offense cannot diverge from frozen execution"),
		OffenseTamper.IsPrepared());

	Fdemo_mapShanmenControlledWeaponPrepareResult ActionTamper =
		MakePrepared(2);
	ActionTamper.Execution = Canonical.Execution;
	TestFalse(TEXT("Action cannot diverge from frozen execution"),
		ActionTamper.IsPrepared());
	TestFalse(TEXT("No tampered result starts a live session"),
		Fdemo_mapShanmenControlledWeaponSession::TryStart(
			ActionTamper, Session, Startup, Active));
	return true;
}

#endif
