#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritEvasion.h"

namespace
{
	const FGuid EvasionRunId(
		0xDA100001, 0xDA100002, 0xDA100003, 0xDA100004);
	const FGuid EvasionOwnerId(
		0xDA110001, 0xDA110002, 0xDA110003, 0xDA110004);
	const FGuid EvasionSourceId(
		0xDA120001, 0xDA120002, 0xDA120003, 0xDA120004);
	const FGuid EvasionAttackSourceId(
		0xDA130001, 0xDA130002, 0xDA130003, 0xDA130004);

	FShanmenCombatActionSnapshot MakeEvasionAction(
		const FString& Digest = TEXT("TEST-DIGEST-P10.0-EVASION"),
		uint64 ActivationSequence = 1000)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = EvasionRunId;
		Capture.OwnerId = EvasionOwnerId;
		Capture.SourceEntityId = EvasionSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P10.0");
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

	FShanmenSpiritEvasionDefinitionCapture MakeDefinitionCapture(
		FName RuleId = TEXT("Defense.Spell.SpiritEvasion01"))
	{
		FShanmenSpiritEvasionDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = RuleId;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.BlockedDamageTags.AddTag(
			FShanmenCombatNativeTags::DamageMental());
		Capture.RequiredSourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Capture;
	}

	FShanmenSpiritEvasionDefinition MakeDefinition(
		FName RuleId = TEXT("Defense.Spell.SpiritEvasion01"))
	{
		FShanmenSpiritEvasionDefinition Definition;
		check(FShanmenSpiritEvasionDefinition::TryCapture(
			MakeDefinitionCapture(RuleId), Definition));
		return Definition;
	}

	void StartActive(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenActionTransitionReceipt& OutCommit)
	{
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, OutCommit));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, OutCommit));
		check(OutCommit.CrossedCommitPointNow());
	}

	FShanmenCombatActionSnapshot MakeAttackAction(
		uint64 ActivationSequence = 1001)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = EvasionRunId;
		Capture.OwnerId = FGuid(
			0xDA140001, 0xDA140002, 0xDA140003, 0xDA140004);
		Capture.SourceEntityId = EvasionAttackSourceId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.P10_0Attack");
		Capture.Content.Version = TEXT("0.0.10.P10.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P10.0-ATTACK");
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

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseLayer& EvasionLayer,
		const FGameplayTag& DamageTag,
		bool bTargetIsLiving = true,
		int32 HitOrdinal = 0)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAttackAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = EvasionSourceId;
		Request.Candidate.DetectorId = TEXT("Detector.Test.P10_0Attack");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Request.Candidate.HitOrdinal = HitOrdinal;
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.P10_0Attack");
		Request.Damage.RawDamage = 100.0f;
		Request.Damage.DamageTags.AddTag(DamageTag);
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 10;
		if (bTargetIsLiving)
		{
			Request.Defense.TargetTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
		}
		Request.Defense.Layers.Add(EvasionLayer);
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	void OpenWindow(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenActionTransitionReceipt& OutCommit,
		FShanmenSpiritEvasionWindow& OutWindow,
		FShanmenSpiritEvasionWindowReceipt& OutOpen)
	{
		StartActive(Action, OutRuntime, OutCommit);
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			Definition,
			OutCommit,
			OutRuntime,
			OutWindow,
			OutOpen));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionDefinitionContractTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasion.DefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionDefinitionContractTest::RunTest(const FString&)
{
	FShanmenSpiritEvasionDefinition Definition;
	TestTrue(TEXT("Canonical evasion coverage captures immutably"),
		FShanmenSpiritEvasionDefinition::TryCapture(
			MakeDefinitionCapture(), Definition)
			&& Definition.IsValid()
			&& Definition.GetActionDefinitionId()
				== FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
			&& Definition.GetRuleId()
				== FName(TEXT("Defense.Spell.SpiritEvasion01"))
			&& Definition.GetRequiredDamageTags().HasTagExact(
				FShanmenCombatNativeTags::DamagePhysical())
			&& Definition.GetRequiredSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer())
			&& Definition.GetRequiredTargetTags().HasTagExact(
				FShanmenCombatNativeTags::TargetLiving()));

	FShanmenSpiritEvasionDefinitionCapture Invalid = MakeDefinitionCapture();
	Invalid.ActionDefinitionId = TEXT("Combat.Action.Spell.GenericEvasion");
	TestFalse(TEXT("Generic action identity cannot enter this contract"),
		FShanmenSpiritEvasionDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture(NAME_None);
	TestFalse(TEXT("A named defense rule is mandatory"),
		FShanmenSpiritEvasionDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.BlockedDamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	TestFalse(TEXT("Parent-child required and blocked filters fail closed"),
		FShanmenSpiritEvasionDefinition::TryCapture(Invalid, Definition));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionCommitBoundWindowTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasion.CommitBoundWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionCommitBoundWindowTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeEvasionAction();
	const FShanmenSpiritEvasionDefinition Definition = MakeDefinition();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Receipt;
	check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Receipt));
	FShanmenSpiritEvasionWindow Window;
	FShanmenSpiritEvasionWindowReceipt Open;
	TestFalse(TEXT("Startup receipt cannot open an active defense window"),
		FShanmenSpiritEvasionWindow::TryOpen(
			Action, Definition, Receipt, Runtime, Window, Open));
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Startup, Receipt));
	TestTrue(TEXT("Exact Startup-to-Active commit opens one window"),
		FShanmenSpiritEvasionWindow::TryOpen(
			Action, Definition, Receipt, Runtime, Window, Open)
			&& Window.IsValid() && Open.IsValid()
			&& Open.GetCommitTransition().CrossedCommitPointNow());

	FShanmenActionOrchestrator ForeignRuntime;
	FShanmenActionTransitionReceipt ForeignCommit;
	StartActive(
		MakeEvasionAction(TEXT("TEST-DIGEST-P10.0-FOREIGN"), 1002),
		ForeignRuntime,
		ForeignCommit);
	FShanmenSpiritEvasionWindow RejectedWindow;
	FShanmenSpiritEvasionWindowReceipt RejectedOpen;
	TestFalse(TEXT("A foreign active runtime cannot validate this commit"),
		FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			Definition,
			Receipt,
			ForeignRuntime,
			RejectedWindow,
			RejectedOpen));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionDefenseProjectionTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasion.DefenseProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionDefenseProjectionTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Commit;
	FShanmenSpiritEvasionWindow Window;
	FShanmenSpiritEvasionWindowReceipt Open;
	OpenWindow(
		MakeEvasionAction(),
		MakeDefinition(),
		Runtime,
		Commit,
		Window,
		Open);
	FShanmenSpiritEvasionProjectionReceipt Projection;
	TestTrue(TEXT("Active window projects the fixed avoidance layer"),
		Window.TryProjectDefenseLayer(Runtime, Projection)
			&& Projection.IsValid()
			&& Projection.GetLayer().IsValid()
			&& Projection.GetLayer().Operation
				== EShanmenDefenseOperation::PreventAll
			&& Projection.GetLayer().Order == FShanmenDefenseOrder::Avoidance
			&& Projection.GetLayer().SourceInstanceId == Open.GetWindowId()
			&& !Projection.GetLayer().bRequiresCommitOnTrigger
			&& Projection.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseEvade()));

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.GetLayer(),
			FShanmenCombatNativeTags::DamagePhysicalSlash()));
	TestTrue(TEXT("Applicable physical impact is audibly and conservatively evaded"),
		Result.bAccepted
			&& Result.Outcome == EShanmenDefenseOutcome::Evaded
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 100.0f)
			&& FMath::IsNearlyZero(Result.FinalDamage)
			&& Result.TriggeredLayers.Num() == 1
			&& Result.TriggeredLayers[0].SourceInstanceId == Open.GetWindowId()
			&& !Result.TriggeredLayers[0].bRequiresCommit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionTagFilteringTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasion.TagFiltering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionTagFilteringTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Commit;
	FShanmenSpiritEvasionWindow Window;
	FShanmenSpiritEvasionWindowReceipt Open;
	OpenWindow(
		MakeEvasionAction(),
		MakeDefinition(),
		Runtime,
		Commit,
		Window,
		Open);
	FShanmenSpiritEvasionProjectionReceipt Projection;
	check(Window.TryProjectDefenseLayer(Runtime, Projection));

	const FShanmenImpactResult Mental = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.GetLayer(),
			FShanmenCombatNativeTags::DamageMental()));
	const FShanmenImpactResult NonLiving = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.GetLayer(),
			FShanmenCombatNativeTags::DamagePhysicalSlash(),
			false,
			1));
	TestTrue(TEXT("Authored damage and target filters fail closed"),
		Mental.bAccepted
			&& Mental.Outcome == EShanmenDefenseOutcome::Applied
			&& Mental.TriggeredLayers.IsEmpty()
			&& FMath::IsNearlyEqual(Mental.FinalDamage, 100.0f)
			&& Mental.IsConserved()
			&& NonLiving.bAccepted
			&& NonLiving.Outcome == EShanmenDefenseOutcome::Applied
			&& NonLiving.TriggeredLayers.IsEmpty()
			&& FMath::IsNearlyEqual(NonLiving.FinalDamage, 100.0f)
			&& NonLiving.IsConserved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionActivePhaseBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasion.ActivePhaseBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionActivePhaseBoundaryTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Commit;
	FShanmenSpiritEvasionWindow Window;
	FShanmenSpiritEvasionWindowReceipt Open;
	OpenWindow(
		MakeEvasionAction(),
		MakeDefinition(),
		Runtime,
		Commit,
		Window,
		Open);
	FShanmenSpiritEvasionProjectionReceipt Projection;
	check(Window.TryProjectDefenseLayer(Runtime, Projection));
	FShanmenActionTransitionReceipt Recovery;
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	TestFalse(TEXT("Recovery closes projection without a second lifetime owner"),
		Window.TryProjectDefenseLayer(Runtime, Projection));

	FShanmenActionOrchestrator InterruptedRuntime;
	FShanmenActionTransitionReceipt InterruptedCommit;
	FShanmenSpiritEvasionWindow InterruptedWindow;
	FShanmenSpiritEvasionWindowReceipt InterruptedOpen;
	OpenWindow(
		MakeEvasionAction(TEXT("TEST-DIGEST-P10.0-INTERRUPTED"), 1003),
		MakeDefinition(),
		InterruptedRuntime,
		InterruptedCommit,
		InterruptedWindow,
		InterruptedOpen);
	FShanmenActionTransitionReceipt Interruption;
	check(InterruptedRuntime.TryInterrupt(
		EShanmenCombatActionPhase::Active, Interruption));
	TestFalse(TEXT("Interruption closes projection immediately"),
		InterruptedWindow.TryProjectDefenseLayer(
			InterruptedRuntime, Projection));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritEvasionDeterministicIdentityTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritEvasion.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritEvasionDeterministicIdentityTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeEvasionAction();
	const FShanmenSpiritEvasionDefinition Definition = MakeDefinition();
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenActionTransitionReceipt FirstCommit;
	FShanmenSpiritEvasionWindow FirstWindow;
	FShanmenSpiritEvasionWindowReceipt FirstOpen;
	OpenWindow(
		Action,
		Definition,
		FirstRuntime,
		FirstCommit,
		FirstWindow,
		FirstOpen);
	FShanmenSpiritEvasionProjectionReceipt FirstProjection;
	check(FirstWindow.TryProjectDefenseLayer(
		FirstRuntime, FirstProjection));

	FShanmenActionOrchestrator ReplayRuntime;
	FShanmenActionTransitionReceipt ReplayCommit;
	FShanmenSpiritEvasionWindow ReplayWindow;
	FShanmenSpiritEvasionWindowReceipt ReplayOpen;
	OpenWindow(
		Action,
		Definition,
		ReplayRuntime,
		ReplayCommit,
		ReplayWindow,
		ReplayOpen);
	FShanmenSpiritEvasionProjectionReceipt ReplayProjection;
	TestTrue(TEXT("Equivalent frozen input reproduces every evasion identity"),
		ReplayWindow.TryProjectDefenseLayer(
			ReplayRuntime, ReplayProjection)
			&& ReplayOpen.GetWindowId() == FirstOpen.GetWindowId()
			&& ReplayOpen.GetReceiptId() == FirstOpen.GetReceiptId()
			&& ReplayProjection.GetProjectionId()
				== FirstProjection.GetProjectionId()
			&& ReplayProjection.GetLayer().LayerId
				== FirstProjection.GetLayer().LayerId);

	FShanmenActionOrchestrator OtherRuntime;
	FShanmenActionTransitionReceipt OtherCommit;
	FShanmenSpiritEvasionWindow OtherWindow;
	FShanmenSpiritEvasionWindowReceipt OtherOpen;
	OpenWindow(
		Action,
		MakeDefinition(TEXT("Defense.Spell.SpiritEvasion02")),
		OtherRuntime,
		OtherCommit,
		OtherWindow,
		OtherOpen);
	TestTrue(TEXT("Different authored rule produces a different window"),
		OtherOpen.GetWindowId() != FirstOpen.GetWindowId());
	return true;
}

#endif
