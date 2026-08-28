#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenVitalityAuthority.h"

namespace
{
	const FGuid VitalityRunId(0x54000001, 0, 0, 1);
	const FGuid VitalityOwnerId(0x54000002, 0, 0, 1);
	const FGuid VitalitySourceId(0x54000003, 0, 0, 1);
	const FGuid VitalityTargetA(0x54000004, 0, 0, 1);
	const FGuid VitalityTargetB(0x54000005, 0, 0, 1);
	const FGuid VitalityTargetC(0x54000008, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = VitalityRunId;
		Capture.OwnerId = VitalityOwnerId;
		Capture.SourceEntityId = VitalitySourceId;
		Capture.SourceItemInstanceId = FGuid(0x54000006, 0, 0, 1);
		Capture.ActionDefinitionId = FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P4.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P4.0");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenTargetVitalitySnapshot CaptureVitality(
		const FShanmenVitalityAuthority& Authority)
	{
		FShanmenTargetVitalitySnapshot Snapshot;
		check(Authority.TryCaptureSnapshot(Snapshot));
		return Snapshot;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FGuid& TargetEntityId,
		int32 HitOrdinal,
		float RawDamage,
		const FShanmenTargetVitalitySnapshot& Vitality,
		const FShanmenDefenseSnapshot& Defense = FShanmenDefenseSnapshot())
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = TargetEntityId;
		Request.Candidate.DetectorId = TEXT("Detector.Weapon.Main");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Request.Candidate.HitOrdinal = HitOrdinal;
		Request.Candidate.HitLocation = FVector(100.0, 0.0, 50.0);
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.VitalityAuthority.Test");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality = Vitality;
		Request.Defense = Defense;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	FShanmenVitalityCommitCommand MakeCommand(const FShanmenImpactRequest& Request)
	{
		const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
		FShanmenVitalityCommitCommand Command;
		check(FShanmenVitalityCommitCommand::TryCreate(Request, Result, Command));
		return Command;
	}

	FShanmenBasicSwordDefinition MakeSwordDefinition()
	{
		FShanmenBasicSwordDefinitionCapture Capture;
		Capture.ActionDefinitionId = FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Weapon.Main");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Basic01.r1");
		Capture.BaseDamage = 20.0f;
		Capture.AttackPowerCoefficient = 0.5f;
		Capture.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		FShanmenBasicSwordDefinition Definition;
		check(FShanmenBasicSwordDefinition::TryCapture(Capture, Definition));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenVitalityCommitLifecycleTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityAuthority.CommitLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenVitalityCommitLifecycleTest::RunTest(const FString&)
{
	FShanmenVitalityAuthority Authority;
	TestTrue(TEXT("Versioned vitality authority initializes"),
		FShanmenVitalityAuthority::TryCreate(VitalityTargetA, 100.0f, 100.0f, 7, Authority));
	const FShanmenTargetVitalitySnapshot Snapshot = CaptureVitality(Authority);
	TestEqual(TEXT("Snapshot exposes authority revision"), Snapshot.AuthorityRevision, static_cast<int64>(7));

	const FShanmenVitalityCommitCommand Command = MakeCommand(
		MakeImpactRequest(VitalityTargetA, 0, 30.0f, Snapshot));
	const FShanmenVitalityCommitResult Committed = Authority.Commit(Command);
	TestTrue(TEXT("Canonical command commits"), Committed.IsSuccess());
	TestEqual(TEXT("First command is not a replay"),
		Committed.Status,
		EShanmenVitalityCommitStatus::Committed);
	TestTrue(TEXT("Mutation receipt is internally valid"), Committed.Receipt.IsValid());
	TestTrue(TEXT("Thirty damage changes 100 to 70"),
		FMath::IsNearlyEqual(Committed.Receipt.GetAppliedDamage(), 30.0f)
			&& FMath::IsNearlyEqual(Committed.Receipt.GetVitalityAfter(), 70.0f));
	TestEqual(TEXT("Successful commit advances revision once"), Authority.GetAuthorityRevision(), static_cast<int64>(8));
	TestEqual(TEXT("Authority stores one processed Impact"), Authority.NumCommittedImpacts(), 1);

	const FShanmenTargetVitalitySnapshot After = CaptureVitality(Authority);
	TestTrue(TEXT("Published snapshot matches committed state"),
		FMath::IsNearlyEqual(After.CurrentVitality, 70.0f)
			&& FMath::IsNearlyEqual(After.MaximumVitality, 100.0f)
			&& After.AuthorityRevision == 8);

	FShanmenVitalityAuthority OverkillAuthority;
	check(FShanmenVitalityAuthority::TryCreate(VitalityTargetB, 20.0f, 100.0f, 0, OverkillAuthority));
	const FShanmenVitalityCommitResult Overkill = OverkillAuthority.Commit(
		MakeCommand(MakeImpactRequest(
			VitalityTargetB,
			0,
			30.0f,
			CaptureVitality(OverkillAuthority))));
	TestTrue(TEXT("Overkill preserves requested damage but clamps applied damage to available vitality"),
		Overkill.IsSuccess()
			&& FMath::IsNearlyEqual(Overkill.Receipt.GetRequestedDamage(), 30.0f)
			&& FMath::IsNearlyEqual(Overkill.Receipt.GetAppliedDamage(), 20.0f)
			&& FMath::IsNearlyEqual(OverkillAuthority.GetCurrentVitality(), 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenVitalityIdempotencyTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityAuthority.IdempotencyAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenVitalityIdempotencyTest::RunTest(const FString&)
{
	FShanmenVitalityAuthority Authority;
	check(FShanmenVitalityAuthority::TryCreate(VitalityTargetA, 100.0f, 100.0f, 0, Authority));
	const FShanmenTargetVitalitySnapshot Snapshot = CaptureVitality(Authority);
	const FShanmenImpactRequest FirstRequest = MakeImpactRequest(
		VitalityTargetA,
		0,
		30.0f,
		Snapshot);
	const FShanmenVitalityCommitCommand FirstCommand = MakeCommand(FirstRequest);
	const FShanmenVitalityCommitResult First = Authority.Commit(FirstCommand);
	const FShanmenVitalityCommitResult Replay = Authority.Commit(FirstCommand);

	TestEqual(TEXT("Exact duplicate returns AlreadyCommitted"),
		Replay.Status,
		EShanmenVitalityCommitStatus::AlreadyCommitted);
	TestTrue(TEXT("Exact duplicate replays the original mutation proof"),
		Replay.IsSuccess()
			&& Replay.Receipt.GetResolutionId() == First.Receipt.GetResolutionId()
			&& Replay.Receipt.GetAuthorityRevisionBefore() == First.Receipt.GetAuthorityRevisionBefore()
			&& Replay.Receipt.GetAuthorityRevisionAfter() == First.Receipt.GetAuthorityRevisionAfter()
			&& FMath::IsNearlyEqual(Replay.Receipt.GetVitalityAfter(), First.Receipt.GetVitalityAfter()));
	TestTrue(TEXT("Replay cannot apply damage or advance revision twice"),
		FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 70.0f)
			&& Authority.GetAuthorityRevision() == 1
			&& Authority.NumCommittedImpacts() == 1);

	const FShanmenVitalityCommitCommand ConflictingCommand = MakeCommand(
		MakeImpactRequest(VitalityTargetA, 0, 40.0f, Snapshot));
	TestTrue(TEXT("Same ImpactId can expose a divergent resolution fingerprint"),
		ConflictingCommand.GetImpactId() == FirstCommand.GetImpactId()
			&& ConflictingCommand.GetResolutionId() != FirstCommand.GetResolutionId());
	const FShanmenVitalityCommitResult Conflict = Authority.Commit(ConflictingCommand);
	TestTrue(TEXT("Divergent duplicate is rejected as conflict"),
		Conflict.IsValid()
			&& !Conflict.IsSuccess()
			&& Conflict.Error == EShanmenVitalityCommitError::ImpactConflict);
	TestTrue(TEXT("Conflict leaves authority unchanged"),
		FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 70.0f)
			&& Authority.GetAuthorityRevision() == 1
			&& Authority.NumCommittedImpacts() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenVitalityStaleSnapshotTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityAuthority.StaleSnapshotIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenVitalityStaleSnapshotTest::RunTest(const FString&)
{
	FShanmenVitalityAuthority Authority;
	check(FShanmenVitalityAuthority::TryCreate(VitalityTargetA, 100.0f, 100.0f, 0, Authority));
	const FShanmenTargetVitalitySnapshot Initial = CaptureVitality(Authority);
	const FShanmenVitalityCommitCommand First = MakeCommand(
		MakeImpactRequest(VitalityTargetA, 0, 10.0f, Initial));
	const FShanmenVitalityCommitCommand Stale = MakeCommand(
		MakeImpactRequest(VitalityTargetA, 1, 20.0f, Initial));
	TestTrue(TEXT("First snapshot consumer commits"), Authority.Commit(First).IsSuccess());

	const FShanmenVitalityCommitResult Rejected = Authority.Commit(Stale);
	TestTrue(TEXT("Outdated authority revision fails closed"),
		Rejected.IsValid()
			&& !Rejected.IsSuccess()
			&& Rejected.Error == EShanmenVitalityCommitError::StaleSnapshot);
	TestTrue(TEXT("Stale rejection neither damages nor consumes Impact"),
		FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 90.0f)
			&& Authority.GetAuthorityRevision() == 1
			&& Authority.NumCommittedImpacts() == 1);

	const FShanmenTargetVitalitySnapshot Fresh = CaptureVitality(Authority);
	const FShanmenVitalityCommitCommand Retried = MakeCommand(
		MakeImpactRequest(VitalityTargetA, 1, 20.0f, Fresh));
	TestTrue(TEXT("Fresh re-resolution preserves Impact identity but changes resolution identity"),
		Retried.GetImpactId() == Stale.GetImpactId()
			&& Retried.GetResolutionId() != Stale.GetResolutionId());
	TestTrue(TEXT("Fresh command may commit after stale rejection"), Authority.Commit(Retried).IsSuccess());
	TestTrue(TEXT("Retry advances exactly one additional revision"),
		FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 70.0f)
			&& Authority.GetAuthorityRevision() == 2
			&& Authority.NumCommittedImpacts() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenVitalityCanonicalGateTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityAuthority.CanonicalResultGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenVitalityCanonicalGateTest::RunTest(const FString&)
{
	FShanmenVitalityAuthority Authority;
	check(FShanmenVitalityAuthority::TryCreate(VitalityTargetA, 100.0f, 100.0f, 0, Authority));
	const FShanmenImpactRequest Request = MakeImpactRequest(
		VitalityTargetA,
		0,
		50.0f,
		CaptureVitality(Authority));
	FShanmenImpactResult Forged = FShanmenDefenseResolver::Resolve(Request);
	Forged.PreventedDamage = 1.0f;
	Forged.FinalDamage = 49.0f;
	TestTrue(TEXT("Forged output remains arithmetically conserved"), Forged.IsConserved());
	FShanmenVitalityCommitCommand RejectedCommand;
	TestFalse(TEXT("Commit command factory rejects non-canonical resolver output"),
		FShanmenVitalityCommitCommand::TryCreate(Request, Forged, RejectedCommand));

	const FShanmenVitalityCommitResult Invalid = Authority.Commit(RejectedCommand);
	TestTrue(TEXT("Invalid command returns structured rejection"),
		Invalid.IsValid()
			&& !Invalid.IsSuccess()
			&& Invalid.Error == EShanmenVitalityCommitError::InvalidCommand);
	TestTrue(TEXT("Invalid command cannot mutate authority"),
		FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 100.0f)
			&& Authority.GetAuthorityRevision() == 0
			&& Authority.NumCommittedImpacts() == 0);

	FShanmenVitalityAuthority NotReady;
	const FShanmenVitalityCommitResult Closed = NotReady.Commit(MakeCommand(Request));
	TestTrue(TEXT("Uninitialized authority rejects a valid command"),
		Closed.IsValid()
			&& Closed.Error == EShanmenVitalityCommitError::AuthorityNotReady);

	FShanmenVitalityAuthority OtherTarget;
	check(FShanmenVitalityAuthority::TryCreate(VitalityTargetB, 100.0f, 100.0f, 0, OtherTarget));
	const FShanmenVitalityCommitResult WrongTarget = Authority.Commit(
		MakeCommand(MakeImpactRequest(
			VitalityTargetB,
			1,
			10.0f,
			CaptureVitality(OtherTarget))));
	TestTrue(TEXT("Command for another stable EntityId is rejected before mutation"),
		WrongTarget.IsValid()
			&& WrongTarget.Error == EShanmenVitalityCommitError::TargetMismatch
			&& Authority.GetAuthorityRevision() == 0
			&& FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 100.0f));

	FShanmenVitalityAuthority Exhausted;
	check(FShanmenVitalityAuthority::TryCreate(
		VitalityTargetA,
		100.0f,
		100.0f,
		MAX_int64,
		Exhausted));
	const FShanmenVitalityCommitResult RevisionLimit = Exhausted.Commit(
		MakeCommand(MakeImpactRequest(
			VitalityTargetA,
			1,
			10.0f,
			CaptureVitality(Exhausted))));
	TestTrue(TEXT("Revision exhaustion fails closed before integer overflow"),
		RevisionLimit.IsValid()
			&& RevisionLimit.Error == EShanmenVitalityCommitError::RevisionExhausted
			&& Exhausted.GetAuthorityRevision() == MAX_int64
			&& FMath::IsNearlyEqual(Exhausted.GetCurrentVitality(), 100.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenExternalVitalityLedgerCommitTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityLedger.ExternalStateCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenExternalVitalityLedgerCommitTest::RunTest(const FString&)
{
	float CurrentVitality = 100.0f;
	float MaximumVitality = 100.0f;
	FShanmenVitalityCommitLedger Ledger;
	TestTrue(TEXT("External-state ledger binds one target without owning vitality"),
		FShanmenVitalityCommitLedger::TryCreate(
			VitalityTargetC,
			CurrentVitality,
			MaximumVitality,
			5,
			Ledger));

	FShanmenTargetVitalitySnapshot Snapshot;
	TestTrue(TEXT("Ledger captures the externally owned state"),
		Ledger.TryCaptureSnapshot(
			CurrentVitality,
			MaximumVitality,
			Snapshot));
	TestEqual(TEXT("External snapshot exposes ledger revision"),
		Snapshot.AuthorityRevision,
		static_cast<int64>(5));

	const FShanmenVitalityCommitCommand Command = MakeCommand(
		MakeImpactRequest(VitalityTargetC, 0, 30.0f, Snapshot));
	const FShanmenVitalityCommitResult Committed = Ledger.Commit(
		Command,
		CurrentVitality,
		MaximumVitality);
	TestTrue(TEXT("Ledger mutates the caller's sole vitality value"),
		Committed.IsSuccess()
			&& FMath::IsNearlyEqual(CurrentVitality, 70.0f)
			&& Ledger.GetAuthorityRevision() == 6
			&& Ledger.NumCommittedImpacts() == 1);

	const FShanmenVitalityCommitResult Replay = Ledger.Commit(
		Command,
		CurrentVitality,
		MaximumVitality);
	TestTrue(TEXT("External-state duplicate replays the original receipt"),
		Replay.IsSuccess()
			&& Replay.Status == EShanmenVitalityCommitStatus::AlreadyCommitted
			&& Replay.Receipt.GetResolutionId() == Committed.Receipt.GetResolutionId()
			&& FMath::IsNearlyEqual(CurrentVitality, 70.0f)
			&& Ledger.GetAuthorityRevision() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenExternalVitalityIntentRecoveryTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityLedger.DurableIntentRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenExternalVitalityIntentRecoveryTest::RunTest(const FString&)
{
	FShanmenVitalityAuthority OriginalAuthority;
	check(FShanmenVitalityAuthority::TryCreate(
		VitalityTargetC, 100.0f, 100.0f, 7, OriginalAuthority));
	const FShanmenVitalityCommitCommand Original = MakeCommand(
		MakeImpactRequest(
			VitalityTargetC, 3, 30.0f,
			CaptureVitality(OriginalAuthority)));
	FShanmenVitalityCommitCommand Restored;
	TestTrue(TEXT("Integrity-checked intent restores the exact immutable command"),
		FShanmenVitalityCommitCommand::TryRestoreFromDurableIntent(
			Original.GetImpactId(), Original.GetResolutionId(),
			Original.GetTargetEntityId(),
			Original.GetExpectedAuthorityRevision(),
			Original.GetExpectedCurrentVitality(),
			Original.GetExpectedMaximumVitality(),
			Original.GetRawDamage(), Original.GetPreventedDamage(),
			Original.GetRequestedDamage(), Original.GetDefenseOutcome(),
			Restored)
			&& Restored.GetImpactId() == Original.GetImpactId()
			&& Restored.GetResolutionId() == Original.GetResolutionId()
			&& Restored.GetTargetEntityId() == Original.GetTargetEntityId()
			&& Restored.GetExpectedAuthorityRevision()
				== Original.GetExpectedAuthorityRevision()
			&& FMath::IsNearlyEqual(
				Restored.GetExpectedVitalityAfter(), 70.0f));

	float BeforeCurrent = 100.0f;
	float BeforeMaximum = 100.0f;
	FShanmenVitalityCommitLedger BeforeLedger;
	check(FShanmenVitalityCommitLedger::TryCreate(
		VitalityTargetC, BeforeCurrent, BeforeMaximum, 0, BeforeLedger));
	const FShanmenVitalityCommitResult Applied =
		BeforeLedger.RecoverPendingExternalCommit(
			Restored, BeforeCurrent, BeforeMaximum);
	TestTrue(TEXT("Exact before state rebases the lost revision and applies once"),
		Applied.Status == EShanmenVitalityCommitStatus::Committed
			&& Applied.IsSuccess()
			&& FMath::IsNearlyEqual(BeforeCurrent, 70.0f)
			&& BeforeLedger.GetAuthorityRevision() == 1
			&& BeforeLedger.NumCommittedImpacts() == 1);
	TestTrue(TEXT("Recovered before-state application remains idempotent"),
		BeforeLedger.RecoverPendingExternalCommit(
				Restored, BeforeCurrent, BeforeMaximum).Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(BeforeCurrent, 70.0f)
			&& BeforeLedger.GetAuthorityRevision() == 1);

	float AfterCurrent = 70.0f;
	float AfterMaximum = 100.0f;
	FShanmenVitalityCommitLedger AfterLedger;
	check(FShanmenVitalityCommitLedger::TryCreate(
		VitalityTargetC, AfterCurrent, AfterMaximum, 0, AfterLedger));
	const FShanmenVitalityCommitResult Imported =
		AfterLedger.RecoverPendingExternalCommit(
			Restored, AfterCurrent, AfterMaximum);
	TestTrue(TEXT("Exact after state imports an already-applied receipt without damage"),
		Imported.Status == EShanmenVitalityCommitStatus::AlreadyCommitted
			&& Imported.IsSuccess()
			&& FMath::IsNearlyEqual(AfterCurrent, 70.0f)
			&& FMath::IsNearlyEqual(
				Imported.Receipt.GetVitalityBefore(), 100.0f)
			&& FMath::IsNearlyEqual(
				Imported.Receipt.GetVitalityAfter(), 70.0f)
			&& AfterLedger.GetAuthorityRevision() == 1
			&& AfterLedger.NumCommittedImpacts() == 1);

	float AmbiguousCurrent = 80.0f;
	float AmbiguousMaximum = 100.0f;
	FShanmenVitalityCommitLedger AmbiguousLedger;
	check(FShanmenVitalityCommitLedger::TryCreate(
		VitalityTargetC,
		AmbiguousCurrent,
		AmbiguousMaximum,
		0,
		AmbiguousLedger));
	const FShanmenVitalityCommitResult Ambiguous =
		AmbiguousLedger.RecoverPendingExternalCommit(
			Restored, AmbiguousCurrent, AmbiguousMaximum);
	TestTrue(TEXT("Neither-before-nor-after state fails closed and stays untouched"),
		Ambiguous.Error == EShanmenVitalityCommitError::StaleSnapshot
			&& !Ambiguous.IsSuccess()
			&& FMath::IsNearlyEqual(AmbiguousCurrent, 80.0f)
			&& AmbiguousLedger.GetAuthorityRevision() == 0
			&& AmbiguousLedger.NumCommittedImpacts() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenExternalVitalityMutationTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityLedger.ExternalMutationInvalidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenExternalVitalityMutationTest::RunTest(const FString&)
{
	float CurrentVitality = 100.0f;
	float MaximumVitality = 100.0f;
	FShanmenVitalityCommitLedger Ledger;
	check(FShanmenVitalityCommitLedger::TryCreate(
		VitalityTargetC,
		CurrentVitality,
		MaximumVitality,
		0,
		Ledger));

	FShanmenTargetVitalitySnapshot Initial;
	check(Ledger.TryCaptureSnapshot(CurrentVitality, MaximumVitality, Initial));
	const FShanmenVitalityCommitCommand Stale = MakeCommand(
		MakeImpactRequest(VitalityTargetC, 0, 10.0f, Initial));

	TestTrue(TEXT("A declared external mutation advances the same revision ledger"),
		Ledger.TryCommitExternalMutation(
			CurrentVitality,
			MaximumVitality,
			90.0f,
			MaximumVitality));
	TestEqual(TEXT("External mutation advances exactly one revision"),
		Ledger.GetAuthorityRevision(),
		static_cast<int64>(1));
	const FShanmenVitalityCommitResult StaleResult = Ledger.Commit(
		Stale,
		CurrentVitality,
		MaximumVitality);
	TestTrue(TEXT("External mutation invalidates a previously resolved command"),
		StaleResult.IsValid()
			&& StaleResult.Error == EShanmenVitalityCommitError::StaleSnapshot
			&& Ledger.NumCommittedImpacts() == 0
			&& FMath::IsNearlyEqual(CurrentVitality, 90.0f));

	FShanmenTargetVitalitySnapshot Fresh;
	check(Ledger.TryCaptureSnapshot(CurrentVitality, MaximumVitality, Fresh));
	const FShanmenVitalityCommitCommand Retried = MakeCommand(
		MakeImpactRequest(VitalityTargetC, 0, 10.0f, Fresh));
	TestTrue(TEXT("Re-resolution retains Impact identity but changes resolution identity"),
		Retried.GetImpactId() == Stale.GetImpactId()
			&& Retried.GetResolutionId() != Stale.GetResolutionId());
	TestTrue(TEXT("Fresh external-state command commits"),
		Ledger.Commit(Retried, CurrentVitality, MaximumVitality).IsSuccess()
			&& FMath::IsNearlyEqual(CurrentVitality, 80.0f)
			&& Ledger.GetAuthorityRevision() == 2);

	TestTrue(TEXT("Later healing is another declared external mutation"),
		Ledger.TryCommitExternalMutation(
			CurrentVitality,
			MaximumVitality,
			85.0f,
			MaximumVitality));
	const FShanmenVitalityCommitResult Replay = Ledger.Commit(
		Retried,
		CurrentVitality,
		MaximumVitality);
	TestTrue(TEXT("A committed Impact remains idempotent after later state changes"),
		Replay.Status == EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(CurrentVitality, 85.0f)
			&& Ledger.GetAuthorityRevision() == 3
			&& Ledger.NumCommittedImpacts() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenExternalVitalityDesynchronizationTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityLedger.BypassDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenExternalVitalityDesynchronizationTest::RunTest(const FString&)
{
	float CurrentVitality = 100.0f;
	float MaximumVitality = 100.0f;
	FShanmenVitalityCommitLedger Ledger;
	check(FShanmenVitalityCommitLedger::TryCreate(
		VitalityTargetC,
		CurrentVitality,
		MaximumVitality,
		0,
		Ledger));
	FShanmenTargetVitalitySnapshot Snapshot;
	check(Ledger.TryCaptureSnapshot(CurrentVitality, MaximumVitality, Snapshot));
	const FShanmenVitalityCommitCommand Command = MakeCommand(
		MakeImpactRequest(VitalityTargetC, 0, 10.0f, Snapshot));

	CurrentVitality = 99.0f;
	FShanmenTargetVitalitySnapshot RejectedSnapshot;
	TestFalse(TEXT("An unannounced product write blocks further snapshot capture"),
		Ledger.TryCaptureSnapshot(
			CurrentVitality,
			MaximumVitality,
			RejectedSnapshot));
	const FShanmenVitalityCommitResult Desynchronized = Ledger.Commit(
		Command,
		CurrentVitality,
		MaximumVitality);
	TestTrue(TEXT("An unannounced write rejects Impact delivery without mutation"),
		Desynchronized.IsValid()
			&& Desynchronized.Error == EShanmenVitalityCommitError::StateDesynchronized
			&& Ledger.GetAuthorityRevision() == 0
			&& Ledger.NumCommittedImpacts() == 0
			&& FMath::IsNearlyEqual(CurrentVitality, 99.0f));
	TestFalse(TEXT("External mutation cannot lie about the ledger's before-state"),
		Ledger.TryCommitExternalMutation(
			CurrentVitality,
			MaximumVitality,
			90.0f,
			MaximumVitality));

	FShanmenVitalityCommitLedger Exhausted;
	float ExhaustedCurrent = 100.0f;
	float ExhaustedMaximum = 100.0f;
	check(FShanmenVitalityCommitLedger::TryCreate(
		VitalityTargetC,
		ExhaustedCurrent,
		ExhaustedMaximum,
		MAX_int64,
		Exhausted));
	TestFalse(TEXT("External mutation also fails closed at revision exhaustion"),
		Exhausted.TryCommitExternalMutation(
			ExhaustedCurrent,
			ExhaustedMaximum,
			90.0f,
			ExhaustedMaximum));
	TestEqual(TEXT("Failed external mutation cannot overflow revision"),
		Exhausted.GetAuthorityRevision(),
		MAX_int64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenVitalityBasicSwordIntegrationTest,
	"Shanmen.0_0_10.CombatRuntime.VitalityAuthority.BasicSwordIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenVitalityBasicSwordIntegrationTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeAction();
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	check(FShanmenActionOrchestrator::TryStart(Action, ActionRuntime, PhaseReceipt));

	FShanmenBasicSwordOffenseSnapshot Offense;
	check(FShanmenBasicSwordOffenseSnapshot::TryCapture(60.0f, Offense));
	FShanmenBasicSwordExecution Sword;
	check(FShanmenBasicSwordExecution::TryCreate(
		Action,
		MakeSwordDefinition(),
		Offense,
		Sword));
	check(ActionRuntime.TryAdvance(EShanmenCombatActionPhase::Startup, PhaseReceipt));
	FShanmenWorldHitContext Context;
	check(Sword.TryBeginEmission(ActionRuntime, Context));

	FShanmenVitalityAuthority Authority;
	check(FShanmenVitalityAuthority::TryCreate(VitalityTargetB, 100.0f, 100.0f, 0, Authority));
	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenDefenseLayer& Guard = Defense.Layers.AddDefaulted_GetRef();
	Guard.LayerId = FGuid(0x54000007, 0, 0, 1);
	Guard.RuleId = TEXT("Defense.Guard.P4.Test");
	Guard.Operation = EShanmenDefenseOperation::ReduceFraction;
	Guard.Order = FShanmenDefenseOrder::Guard;
	Guard.Magnitude = 0.2f;
	Guard.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseGuard());

	FShanmenHitCandidate Candidate;
	Candidate.ActivationId = Context.GetAction().GetActivationId();
	Candidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
	Candidate.TargetEntityId = VitalityTargetB;
	Candidate.DetectorId = Context.GetDetectorId();
	Candidate.DetectorKind = Context.GetDetectorKind();
	Candidate.HitOrdinal = Context.GetHitOrdinal();
	Candidate.HitLocation = FVector(100.0, 0.0, 50.0);
	Candidate.HitNormal = FVector::BackwardVector;

	FShanmenBasicSwordImpactReceipt ImpactReceipt;
	TestTrue(TEXT("P3.1 sword resolves against authority snapshot"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			Candidate,
			CaptureVitality(Authority),
			Defense,
			ImpactReceipt));
	TestTrue(TEXT("Guard leaves 40 final damage from the 50-point sword formula"),
		FMath::IsNearlyEqual(ImpactReceipt.GetResult().RawDamage, 50.0f)
			&& FMath::IsNearlyEqual(ImpactReceipt.GetResult().PreventedDamage, 10.0f)
			&& FMath::IsNearlyEqual(ImpactReceipt.GetResult().FinalDamage, 40.0f));

	FShanmenVitalityCommitCommand Command;
	TestTrue(TEXT("Sword receipt becomes a canonical vitality command"),
		FShanmenVitalityCommitCommand::TryCreate(
			ImpactReceipt.GetRequest(),
			ImpactReceipt.GetResult(),
			Command));
	const FShanmenVitalityCommitResult Committed = Authority.Commit(Command);
	TestTrue(TEXT("Final damage commits exactly once to target vitality"),
		Committed.IsSuccess()
			&& FMath::IsNearlyEqual(Committed.Receipt.GetAppliedDamage(), 40.0f)
			&& FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 60.0f));
	TestEqual(TEXT("Duplicate delivery is an idempotent replay"),
		Authority.Commit(Command).Status,
		EShanmenVitalityCommitStatus::AlreadyCommitted);
	TestTrue(TEXT("Duplicate delivery cannot change the target twice"),
		FMath::IsNearlyEqual(Authority.GetCurrentVitality(), 60.0f)
			&& Authority.GetAuthorityRevision() == 1
			&& Authority.NumCommittedImpacts() == 1);
	return true;
}

#endif
