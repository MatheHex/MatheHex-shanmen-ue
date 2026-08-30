#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritShieldCapacityAuthority.h"

namespace
{
	const FGuid CapacityRunId(0x91100001, 0x91100002, 0x91100003, 0x91100004);
	const FGuid CapacityOwnerId(0x92100001, 0x92100002, 0x92100003, 0x92100004);
	const FGuid CapacityShieldSourceId(0x93100001, 0x93100002, 0x93100003, 0x93100004);
	const FGuid CapacityAttackSourceId(0x94100001, 0x94100002, 0x94100003, 0x94100004);

	FShanmenCombatActionSnapshot MakeShieldAction(
		uint64 ActivationSequence = 91,
		const FString& Digest = TEXT("TEST-DIGEST-P9.1-SHIELD"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = CapacityRunId;
		Capture.OwnerId = CapacityOwnerId;
		Capture.SourceEntityId = CapacityShieldSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P9.1");
		Capture.Content.Digest = Digest;
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSpiritShieldDefinition MakeShieldDefinition()
	{
		FShanmenSpiritShieldDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Spell.SpiritShield01");
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

	void StartActiveShield(
		FShanmenSpiritShieldRuntime& OutShieldRuntime,
		FShanmenSpiritShieldCapacityAuthority& OutAuthority,
		uint64 ActivationSequence = 91,
		const FString& Digest = TEXT("TEST-DIGEST-P9.1-SHIELD"))
	{
		const FShanmenCombatActionSnapshot Action =
			MakeShieldAction(ActivationSequence, Digest);
		FShanmenActionOrchestrator ActionRuntime;
		FShanmenActionTransitionReceipt PhaseReceipt;
		check(FShanmenActionOrchestrator::TryStart(
			Action, ActionRuntime, PhaseReceipt));
		check(FShanmenSpiritShieldRuntime::TryPrepare(
			Action, MakeShieldDefinition(), OutShieldRuntime));
		check(ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, PhaseReceipt));
		FShanmenSpiritShieldActivationReceipt Activation;
		check(OutShieldRuntime.TryActivate(ActionRuntime, Activation));
		check(FShanmenSpiritShieldCapacityAuthority::TryCreate(
			Activation, OutAuthority));
	}

	FShanmenCombatActionSnapshot MakeAttackAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = CapacityRunId;
		Capture.OwnerId = FGuid(
			0x95100001, 0x95100002, 0x95100003, 0x95100004);
		Capture.SourceEntityId = CapacityAttackSourceId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.PhysicalAttack");
		Capture.Content.Version = TEXT("0.0.10.P9.1");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P9.1-ATTACK");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			92);

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
		Request.Candidate.TargetEntityId = CapacityShieldSourceId;
		Request.Candidate.DetectorId = TEXT("Detector.Test.PhysicalAttack");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Request.Candidate.HitOrdinal = HitOrdinal;
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.PhysicalAttack");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 9;
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

	FShanmenSpiritShieldCapacityCommitCommand MakeCommitCommand(
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldCapacityProjectionAuthorityTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity.AuthoritativeProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldCapacityProjectionAuthorityTest::RunTest(
	const FString&)
{
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldCapacityAuthority Authority;
	StartActiveShield(ShieldRuntime, Authority);
	TestTrue(TEXT("Activation initializes one valid single-writer authority"),
		Authority.IsValid()
			&& Authority.GetAuthorityRevision() == 0
			&& FMath::IsNearlyEqual(Authority.GetMaximumCapacity(), 30.0f)
			&& FMath::IsNearlyEqual(Authority.GetAvailableCapacity(), 30.0f)
			&& Authority.NumCommittedImpacts() == 0);

	FShanmenSpiritShieldProjectionReceipt Projection;
	TestTrue(TEXT("Authority alone supplies the current projection revision"),
		Authority.TryProjectDefenseLayer(ShieldRuntime, Projection)
			&& Projection.IsValid()
			&& Projection.GetAuthorityRevision() == 0
			&& FMath::IsNearlyEqual(
				Projection.GetAvailableCapacity(), 30.0f));
	const FGuid ProjectionId = Projection.GetProjectionId();
	TestTrue(TEXT("Unchanged authority projection is an exact replay"),
		Authority.TryProjectDefenseLayer(ShieldRuntime, Projection)
			&& Projection.GetProjectionId() == ProjectionId);

	FShanmenSpiritShieldRuntime ForeignRuntime;
	FShanmenSpiritShieldCapacityAuthority ForeignAuthority;
	StartActiveShield(
		ForeignRuntime,
		ForeignAuthority,
		93,
		TEXT("TEST-DIGEST-P9.1-FOREIGN"));
	TestFalse(TEXT("Authority cannot project through a foreign shield runtime"),
		Authority.TryProjectDefenseLayer(ForeignRuntime, Projection));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldCapacityCanonicalCommitTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity.CanonicalCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldCapacityCanonicalCommitTest::RunTest(const FString&)
{
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldCapacityAuthority Authority;
	StartActiveShield(ShieldRuntime, Authority);
	FShanmenSpiritShieldProjectionReceipt Projection;
	check(Authority.TryProjectDefenseLayer(ShieldRuntime, Projection));

	const FShanmenSpiritShieldCapacityCommitCommand Command =
		MakeCommitCommand(Projection, 12.0f, 0);
	TestTrue(TEXT("Canonical resolver proof creates an immutable command"),
		Command.IsValid()
			&& FMath::IsNearlyEqual(Command.GetRequestedCapacity(), 12.0f));
	const FShanmenSpiritShieldCapacityCommitResult Result =
		Authority.Commit(Command);
	TestTrue(TEXT("Exact current projection atomically consumes capacity"),
		Result.IsSuccess()
			&& Result.Status
				== EShanmenSpiritShieldCapacityCommitStatus::Committed
			&& Result.Receipt.IsValid()
			&& Result.Receipt.GetImpactId() == Command.GetImpactId()
			&& Result.Receipt.GetProjectionId()
				== Projection.GetProjectionId()
			&& Result.Receipt.GetAuthorityRevisionBefore() == 0
			&& Result.Receipt.GetAuthorityRevisionAfter() == 1
			&& FMath::IsNearlyEqual(
				Result.Receipt.GetCommittedCapacity(), 12.0f)
			&& FMath::IsNearlyEqual(
				Result.Receipt.GetCapacityAfter(), 18.0f));
	TestTrue(TEXT("Authority state advances exactly once"),
		Authority.IsValid()
			&& Authority.GetAuthorityRevision() == 1
			&& Authority.NumCommittedImpacts() == 1
			&& FMath::IsNearlyEqual(Authority.GetAvailableCapacity(), 18.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldCapacityFailClosedTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity.FailClosedProofs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldCapacityFailClosedTest::RunTest(const FString&)
{
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldCapacityAuthority Authority;
	StartActiveShield(ShieldRuntime, Authority);
	FShanmenSpiritShieldProjectionReceipt Projection;
	check(Authority.TryProjectDefenseLayer(ShieldRuntime, Projection));

	const FShanmenSpiritShieldCapacityCommitCommand First =
		MakeCommitCommand(Projection, 5.0f, 0);
	const FShanmenSpiritShieldCapacityCommitCommand Stale =
		MakeCommitCommand(Projection, 4.0f, 1);
	check(Authority.Commit(First).IsSuccess());
	const FShanmenSpiritShieldCapacityCommitResult StaleResult =
		Authority.Commit(Stale);
	TestTrue(TEXT("A prior revision cannot consume current capacity"),
		StaleResult.Status
			== EShanmenSpiritShieldCapacityCommitStatus::Rejected
			&& StaleResult.Error
				== EShanmenSpiritShieldCapacityCommitError::StaleProjection);

	const FShanmenImpactRequest TamperedRequest = MakeImpactRequest(
		Projection.GetLayer(), 4.0f, 2);
	FShanmenImpactResult TamperedResult =
		FShanmenDefenseResolver::Resolve(TamperedRequest);
	check(TamperedResult.TriggeredLayers.Num() == 1);
	TamperedResult.TriggeredLayers[0].PreventedDamage = 3.0f;
	TamperedResult.PreventedDamage = 3.0f;
	TamperedResult.FinalDamage = 1.0f;
	FShanmenSpiritShieldCapacityCommitCommand TamperedCommand;
	TestFalse(TEXT("Conserved but noncanonical resolver output is rejected"),
		FShanmenSpiritShieldCapacityCommitCommand::TryCreate(
			Projection,
			TamperedRequest,
			TamperedResult,
			TamperedCommand));

	FShanmenSpiritShieldRuntime ForeignRuntime;
	FShanmenSpiritShieldCapacityAuthority ForeignAuthority;
	StartActiveShield(
		ForeignRuntime,
		ForeignAuthority,
		93,
		TEXT("TEST-DIGEST-P9.1-FOREIGN"));
	FShanmenSpiritShieldProjectionReceipt ForeignProjection;
	check(ForeignAuthority.TryProjectDefenseLayer(
		ForeignRuntime, ForeignProjection));
	const FShanmenSpiritShieldCapacityCommitResult ForeignResult =
		Authority.Commit(MakeCommitCommand(ForeignProjection, 3.0f, 3));
	TestTrue(TEXT("Foreign shield proof cannot mutate this authority"),
		ForeignResult.Status
			== EShanmenSpiritShieldCapacityCommitStatus::Rejected
			&& ForeignResult.Error
				== EShanmenSpiritShieldCapacityCommitError::ShieldMismatch);
	TestTrue(TEXT("All rejections preserve the committed state"),
		Authority.GetAuthorityRevision() == 1
			&& Authority.NumCommittedImpacts() == 1
			&& FMath::IsNearlyEqual(Authority.GetAvailableCapacity(), 25.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldCapacityReplayAfterProgressTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity.ReplayAfterProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldCapacityReplayAfterProgressTest::RunTest(
	const FString&)
{
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldCapacityAuthority Authority;
	StartActiveShield(ShieldRuntime, Authority);
	FShanmenSpiritShieldProjectionReceipt FirstProjection;
	check(Authority.TryProjectDefenseLayer(
		ShieldRuntime, FirstProjection));
	const FShanmenSpiritShieldCapacityCommitCommand First =
		MakeCommitCommand(FirstProjection, 10.0f, 0);
	const FShanmenSpiritShieldCapacityCommitResult FirstResult =
		Authority.Commit(First);
	check(FirstResult.IsSuccess());

	FShanmenSpiritShieldProjectionReceipt SecondProjection;
	check(Authority.TryProjectDefenseLayer(
		ShieldRuntime, SecondProjection));
	TestTrue(TEXT("Successful commit creates the next lower projection"),
		SecondProjection.GetAuthorityRevision() == 1
			&& FMath::IsNearlyEqual(
				SecondProjection.GetAvailableCapacity(), 20.0f));
	const FShanmenSpiritShieldCapacityCommitResult SecondResult =
		Authority.Commit(MakeCommitCommand(SecondProjection, 8.0f, 1));
	check(SecondResult.IsSuccess());

	const FShanmenSpiritShieldCapacityCommitResult Replay =
		Authority.Commit(First);
	TestTrue(TEXT("Exact old Impact replay returns its original receipt"),
		Replay.Status
			== EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted
			&& Replay.Receipt.GetReceiptId()
				== FirstResult.Receipt.GetReceiptId());
	TestTrue(TEXT("Replay after progress never double-spends capacity"),
		Authority.GetAuthorityRevision() == 2
			&& Authority.NumCommittedImpacts() == 2
			&& FMath::IsNearlyEqual(Authority.GetAvailableCapacity(), 12.0f));

	FShanmenSpiritShieldProjectionReceipt CurrentProjection;
	check(Authority.TryProjectDefenseLayer(
		ShieldRuntime, CurrentProjection));
	const FShanmenSpiritShieldCapacityCommitResult Conflict =
		Authority.Commit(MakeCommitCommand(CurrentProjection, 3.0f, 0));
	TestTrue(TEXT("Same ImpactId with a different proof conflicts"),
		Conflict.Status
			== EShanmenSpiritShieldCapacityCommitStatus::Rejected
			&& Conflict.Error
				== EShanmenSpiritShieldCapacityCommitError::ImpactConflict);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldCapacityDepletionBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity.DepletionBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldCapacityDepletionBoundaryTest::RunTest(
	const FString&)
{
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldCapacityAuthority Authority;
	StartActiveShield(ShieldRuntime, Authority);
	FShanmenSpiritShieldProjectionReceipt Projection;
	check(Authority.TryProjectDefenseLayer(ShieldRuntime, Projection));
	const FShanmenSpiritShieldCapacityCommitResult Result =
		Authority.Commit(MakeCommitCommand(Projection, 100.0f, 0));
	TestTrue(TEXT("Resolver caps consumption at projected capacity"),
		Result.IsSuccess()
			&& Result.Receipt.IsDepleted()
			&& FMath::IsNearlyEqual(
				Result.Receipt.GetCommittedCapacity(), 30.0f)
			&& Authority.IsDepleted());
	TestFalse(TEXT("A depleted authority cannot project another layer"),
		Authority.TryProjectDefenseLayer(ShieldRuntime, Projection));
	TestTrue(TEXT("Capacity exhaustion does not steal lifecycle authority"),
		ShieldRuntime.GetState() == EShanmenSpiritShieldState::Active);

	FShanmenSpiritShieldDeactivationReceipt Deactivation;
	TestTrue(TEXT("Product owner still ends lifecycle explicitly"),
		ShieldRuntime.TryDeactivate(
			Authority.GetActivation().GetShieldInstanceId(),
			EShanmenSpiritShieldDeactivationReason::Explicit,
			Deactivation)
			&& Deactivation.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldCapacityDeterminismTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldCapacityDeterminismTest::RunTest(const FString&)
{
	FShanmenSpiritShieldRuntime FirstRuntime;
	FShanmenSpiritShieldCapacityAuthority FirstAuthority;
	StartActiveShield(FirstRuntime, FirstAuthority);
	FShanmenSpiritShieldRuntime SecondRuntime;
	FShanmenSpiritShieldCapacityAuthority SecondAuthority;
	StartActiveShield(SecondRuntime, SecondAuthority);

	FShanmenSpiritShieldProjectionReceipt FirstProjection;
	FShanmenSpiritShieldProjectionReceipt SecondProjection;
	check(FirstAuthority.TryProjectDefenseLayer(
		FirstRuntime, FirstProjection));
	check(SecondAuthority.TryProjectDefenseLayer(
		SecondRuntime, SecondProjection));
	const FShanmenSpiritShieldCapacityCommitCommand FirstCommand =
		MakeCommitCommand(FirstProjection, 7.0f, 4);
	const FShanmenSpiritShieldCapacityCommitCommand SecondCommand =
		MakeCommitCommand(SecondProjection, 7.0f, 4);
	const FShanmenSpiritShieldCapacityCommitResult FirstResult =
		FirstAuthority.Commit(FirstCommand);
	const FShanmenSpiritShieldCapacityCommitResult SecondResult =
		SecondAuthority.Commit(SecondCommand);
	TestTrue(TEXT("Equivalent inputs reproduce projection and command IDs"),
		FirstProjection.GetProjectionId()
				== SecondProjection.GetProjectionId()
			&& FirstCommand.GetResolutionId()
				== SecondCommand.GetResolutionId()
			&& FirstCommand.GetCommandId() == SecondCommand.GetCommandId());
	TestTrue(TEXT("Equivalent authorities reproduce the commit receipt"),
		FirstResult.IsSuccess()
			&& SecondResult.IsSuccess()
			&& FirstResult.Receipt.GetReceiptId()
				== SecondResult.Receipt.GetReceiptId()
			&& FMath::IsNearlyEqual(
				FirstAuthority.GetAvailableCapacity(),
				SecondAuthority.GetAvailableCapacity()));
	return true;
}

#endif
