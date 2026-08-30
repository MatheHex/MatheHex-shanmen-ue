#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritShield.h"

namespace
{
	const FGuid ShieldRunId(0x91000001, 0x91000002, 0x91000003, 0x91000004);
	const FGuid ShieldOwnerId(0x92000001, 0x92000002, 0x92000003, 0x92000004);
	const FGuid ShieldSourceId(0x93000001, 0x93000002, 0x93000003, 0x93000004);
	const FGuid AttackSourceId(0x94000001, 0x94000002, 0x94000003, 0x94000004);

	FShanmenCombatActionSnapshot MakeAction(
		const FString& Digest = TEXT("TEST-DIGEST-P9.0"),
		uint64 ActivationSequence = 9)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ShieldRunId;
		Capture.OwnerId = ShieldOwnerId;
		Capture.SourceEntityId = ShieldSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P9.0");
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

	FShanmenSpiritShieldDefinitionCapture MakeDefinitionCapture()
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
		return Capture;
	}

	FShanmenSpiritShieldDefinition MakeDefinition()
	{
		FShanmenSpiritShieldDefinition Definition;
		check(FShanmenSpiritShieldDefinition::TryCapture(
			MakeDefinitionCapture(), Definition));
		return Definition;
	}

	void StartActiveShield(
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenSpiritShieldRuntime& OutShieldRuntime,
		FShanmenSpiritShieldActivationReceipt& OutActivation)
	{
		const FShanmenCombatActionSnapshot Action = MakeAction();
		FShanmenActionTransitionReceipt PhaseReceipt;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutActionRuntime, PhaseReceipt));
		check(FShanmenSpiritShieldRuntime::TryPrepare(
			Action, MakeDefinition(), OutShieldRuntime));
		check(OutActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, PhaseReceipt));
		check(OutShieldRuntime.TryActivate(
			OutActionRuntime, OutActivation));
	}

	FShanmenCombatActionSnapshot MakeAttackAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ShieldRunId;
		Capture.OwnerId = FGuid(0x95000001, 0x95000002, 0x95000003, 0x95000004);
		Capture.SourceEntityId = AttackSourceId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.PhysicalAttack");
		Capture.Content.Version = TEXT("0.0.10.P9.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P9.0-ATTACK");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			10);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseLayer& ShieldLayer)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAttackAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = ShieldSourceId;
		Request.Candidate.DetectorId = TEXT("Detector.Test.PhysicalAttack");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Request.Candidate.HitOrdinal = 0;
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.PhysicalAttack");
		Request.Damage.RawDamage = 100.0f;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 5;
		Request.Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Request.Defense.Layers.Add(ShieldLayer);
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDefinitionContractTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShield.DefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDefinitionContractTest::RunTest(const FString&)
{
	FShanmenSpiritShieldDefinition Definition;
	TestTrue(TEXT("Canonical spirit-shield content captures immutably"),
		FShanmenSpiritShieldDefinition::TryCapture(
			MakeDefinitionCapture(), Definition));
	TestTrue(TEXT("Captured definition fixes shield capacity and filters"),
		Definition.IsValid()
			&& Definition.GetActionDefinitionId()
				== FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId()
			&& Definition.GetRuleId() == FName(TEXT("Defense.Spell.SpiritShield01"))
			&& FMath::IsNearlyEqual(Definition.GetMaximumCapacity(), 30.0f)
			&& Definition.GetRequiredDamageTags().HasTagExact(
				FShanmenCombatNativeTags::DamagePhysical())
			&& Definition.GetBlockedDamageTags().HasTagExact(
				FShanmenCombatNativeTags::DamageMental()));

	FShanmenSpiritShieldDefinitionCapture Invalid = MakeDefinitionCapture();
	Invalid.ActionDefinitionId = TEXT("Combat.Action.Spell.GenericShield");
	TestFalse(TEXT("Generic shield identity cannot enter this contract"),
		FShanmenSpiritShieldDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.MaximumCapacity = 0.0f;
	TestFalse(TEXT("A shield requires positive finite capacity"),
		FShanmenSpiritShieldDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.BlockedDamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysical());
	TestFalse(TEXT("Impossible required and blocked filters fail closed"),
		FShanmenSpiritShieldDefinition::TryCapture(Invalid, Definition));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActivationContractTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShield.CommitBoundActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActivationContractTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeAction();
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	check(FShanmenActionOrchestrator::TryStart(
		Action, ActionRuntime, PhaseReceipt));
	FShanmenSpiritShieldRuntime ShieldRuntime;
	TestTrue(TEXT("Valid content prepares without activating defense"),
		FShanmenSpiritShieldRuntime::TryPrepare(
			Action, MakeDefinition(), ShieldRuntime)
			&& ShieldRuntime.GetState() == EShanmenSpiritShieldState::Prepared);

	FShanmenSpiritShieldActivationReceipt Activation;
	TestFalse(TEXT("Startup cannot activate a shield before the action commit"),
		ShieldRuntime.TryActivate(ActionRuntime, Activation));
	FShanmenSpiritShieldProjectionReceipt Projection;
	TestFalse(TEXT("Prepared state cannot project a defense layer"),
		ShieldRuntime.TryProjectDefenseLayer(1, 30.0f, Projection));
	check(ActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, PhaseReceipt));
	TestTrue(TEXT("Committed active action activates one exact shield"),
		ShieldRuntime.TryActivate(ActionRuntime, Activation)
			&& Activation.IsValid()
			&& ShieldRuntime.GetState() == EShanmenSpiritShieldState::Active);
	const FGuid ActivationReceiptId = Activation.GetReceiptId();
	TestTrue(TEXT("Exact activation replay returns the same proof"),
		ShieldRuntime.TryActivate(ActionRuntime, Activation)
			&& Activation.GetReceiptId() == ActivationReceiptId);

	FShanmenActionOrchestrator ForeignActionRuntime;
	check(FShanmenActionOrchestrator::TryStart(
		MakeAction(TEXT("TEST-DIGEST-P9.0-FOREIGN"), 10),
		ForeignActionRuntime,
		PhaseReceipt));
	check(ForeignActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, PhaseReceipt));
	TestFalse(TEXT("Foreign action content cannot replay the activation"),
		ShieldRuntime.TryActivate(ForeignActionRuntime, Activation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldProjectionAndResolverTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShield.RevisionedDefenseProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldProjectionAndResolverTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartActiveShield(ActionRuntime, ShieldRuntime, Activation);

	FShanmenSpiritShieldProjectionReceipt Projection;
	TestTrue(TEXT("Capacity authority revision projects one committed layer"),
		ShieldRuntime.TryProjectDefenseLayer(5, 30.0f, Projection));
	const FShanmenSpiritShieldProjectionReceipt FirstProjection = Projection;
	TestTrue(TEXT("Projection uses the fixed CombatCore shield contract"),
		Projection.IsValid()
			&& Projection.GetLayer().Operation
				== EShanmenDefenseOperation::AbsorbPoints
			&& Projection.GetLayer().Order == FShanmenDefenseOrder::Shield
			&& Projection.GetLayer().SourceInstanceId
				== Activation.GetShieldInstanceId()
			&& Projection.GetLayer().bRequiresCommitOnTrigger
			&& Projection.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseShield()));
	TestTrue(TEXT("Exact authority replay returns the same projection"),
		ShieldRuntime.TryProjectDefenseLayer(5, 30.0f, Projection)
			&& Projection.GetProjectionId()
				== FirstProjection.GetProjectionId());
	TestFalse(TEXT("One authority revision cannot describe two capacities"),
		ShieldRuntime.TryProjectDefenseLayer(5, 29.0f, Projection));
	TestFalse(TEXT("Authority revision cannot move backward"),
		ShieldRuntime.TryProjectDefenseLayer(4, 20.0f, Projection));
	TestFalse(TEXT("Capacity cannot exceed the authored maximum"),
		ShieldRuntime.TryProjectDefenseLayer(6, 31.0f, Projection));
	TestTrue(TEXT("Later revision may expose lower remaining capacity"),
		ShieldRuntime.TryProjectDefenseLayer(6, 12.0f, Projection)
			&& Projection.GetLayer().LayerId
				== FirstProjection.GetLayer().LayerId
			&& Projection.GetProjectionId()
				!= FirstProjection.GetProjectionId());
	TestFalse(TEXT("Capacity cannot increase without a new shield activation"),
		ShieldRuntime.TryProjectDefenseLayer(7, 13.0f, Projection));

	const FShanmenImpactRequest Request =
		MakeImpactRequest(FirstProjection.GetLayer());
	TestTrue(TEXT("Projected layer forms a valid immutable impact request"),
		Request.IsValid());
	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	TestTrue(TEXT("Thirty shield points reduce one hundred damage to seventy"),
		Result.bAccepted
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 30.0f)
			&& FMath::IsNearlyEqual(Result.FinalDamage, 70.0f)
			&& Result.TriggeredLayers.Num() == 1
			&& Result.TriggeredLayers[0].SourceInstanceId
				== Activation.GetShieldInstanceId()
			&& Result.TriggeredLayers[0].bRequiresCommit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldExplicitDeactivationTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShield.ExplicitDeactivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldExplicitDeactivationTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartActiveShield(ActionRuntime, ShieldRuntime, Activation);
	FShanmenSpiritShieldProjectionReceipt Projection;
	check(ShieldRuntime.TryProjectDefenseLayer(7, 18.0f, Projection));

	FShanmenSpiritShieldDeactivationReceipt Deactivation;
	TestFalse(TEXT("Foreign shield identity cannot end this shield"),
		ShieldRuntime.TryDeactivate(
			FGuid(1, 2, 3, 4),
			EShanmenSpiritShieldDeactivationReason::DurationElapsed,
			Deactivation));
	TestTrue(TEXT("External duration owner explicitly ends the shield"),
		ShieldRuntime.TryDeactivate(
			Activation.GetShieldInstanceId(),
			EShanmenSpiritShieldDeactivationReason::DurationElapsed,
			Deactivation)
			&& Deactivation.IsValid()
			&& ShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Deactivated);
	const FGuid DeactivationId = Deactivation.GetReceiptId();
	TestTrue(TEXT("Exact deactivation replay returns the same proof"),
		ShieldRuntime.TryDeactivate(
			Activation.GetShieldInstanceId(),
			EShanmenSpiritShieldDeactivationReason::DurationElapsed,
			Deactivation)
			&& Deactivation.GetReceiptId() == DeactivationId);
	TestFalse(TEXT("Replay cannot rewrite the terminal reason"),
		ShieldRuntime.TryDeactivate(
			Activation.GetShieldInstanceId(),
			EShanmenSpiritShieldDeactivationReason::Explicit,
			Deactivation));
	TestFalse(TEXT("Ended shield cannot project a later defense layer"),
		ShieldRuntime.TryProjectDefenseLayer(8, 10.0f, Projection));
	TestTrue(TEXT("Historical activation replay never reactivates the shield"),
		ShieldRuntime.TryActivate(ActionRuntime, Activation)
			&& ShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Deactivated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeterministicIdentityTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShield.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeterministicIdentityTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator FirstActionRuntime;
	FShanmenSpiritShieldRuntime FirstShieldRuntime;
	FShanmenSpiritShieldActivationReceipt FirstActivation;
	StartActiveShield(
		FirstActionRuntime, FirstShieldRuntime, FirstActivation);
	FShanmenSpiritShieldProjectionReceipt FirstProjection;
	check(FirstShieldRuntime.TryProjectDefenseLayer(
		3, 25.0f, FirstProjection));

	FShanmenActionOrchestrator ReplayActionRuntime;
	FShanmenSpiritShieldRuntime ReplayShieldRuntime;
	FShanmenSpiritShieldActivationReceipt ReplayActivation;
	StartActiveShield(
		ReplayActionRuntime, ReplayShieldRuntime, ReplayActivation);
	FShanmenSpiritShieldProjectionReceipt ReplayProjection;
	TestTrue(TEXT("Equivalent frozen input reproduces all shield identities"),
		ReplayShieldRuntime.TryProjectDefenseLayer(
			3, 25.0f, ReplayProjection)
			&& ReplayActivation.GetShieldInstanceId()
				== FirstActivation.GetShieldInstanceId()
			&& ReplayActivation.GetReceiptId()
				== FirstActivation.GetReceiptId()
			&& ReplayProjection.GetProjectionId()
				== FirstProjection.GetProjectionId()
			&& ReplayProjection.GetLayer().LayerId
				== FirstProjection.GetLayer().LayerId);

	const FShanmenCombatActionSnapshot OtherAction =
		MakeAction(TEXT("TEST-DIGEST-P9.0-OTHER"), 11);
	FShanmenActionOrchestrator OtherActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	check(FShanmenActionOrchestrator::TryStart(
		OtherAction, OtherActionRuntime, PhaseReceipt));
	check(OtherActionRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, PhaseReceipt));
	FShanmenSpiritShieldRuntime OtherShieldRuntime;
	check(FShanmenSpiritShieldRuntime::TryPrepare(
		OtherAction, MakeDefinition(), OtherShieldRuntime));
	FShanmenSpiritShieldActivationReceipt OtherActivation;
	TestTrue(TEXT("Different action content produces a different shield"),
		OtherShieldRuntime.TryActivate(
			OtherActionRuntime, OtherActivation)
			&& OtherActivation.GetShieldInstanceId()
				!= FirstActivation.GetShieldInstanceId());
	return true;
}

#endif
