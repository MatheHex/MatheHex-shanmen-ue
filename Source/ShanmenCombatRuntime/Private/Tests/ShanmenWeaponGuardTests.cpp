#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWeaponGuard.h"

namespace
{
	const FGuid GuardRunId(
		0xDB100001, 0xDB100002, 0xDB100003, 0xDB100004);
	const FGuid GuardOwnerId(
		0xDB110001, 0xDB110002, 0xDB110003, 0xDB110004);
	const FGuid GuardSourceId(
		0xDB120001, 0xDB120002, 0xDB120003, 0xDB120004);
	const FGuid GuardWeaponId(
		0xDB130001, 0xDB130002, 0xDB130003, 0xDB130004);
	const FGuid GuardAttackSourceId(
		0xDB140001, 0xDB140002, 0xDB140003, 0xDB140004);

	FShanmenCombatActionSnapshot MakeGuardAction(
		const FString& Digest = TEXT("TEST-DIGEST-P11.0-GUARD"),
		uint64 ActivationSequence = 1100,
		bool bHasWeapon = true)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = GuardRunId;
		Capture.OwnerId = GuardOwnerId;
		Capture.SourceEntityId = GuardSourceId;
		if (bHasWeapon)
		{
			Capture.SourceItemInstanceId = GuardWeaponId;
		}
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.0");
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

	FShanmenWeaponGuardDefinitionCapture MakeDefinitionCapture(
		FName RuleId = TEXT("Defense.Sword.WeaponGuard01"),
		float GuardFraction = 0.25f)
	{
		FShanmenWeaponGuardDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = RuleId;
		Capture.GuardFraction = GuardFraction;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.BlockedDamageTags.AddTag(
			FShanmenCombatNativeTags::DamageMental());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Capture;
	}

	FShanmenWeaponGuardDefinition MakeDefinition(
		FName RuleId = TEXT("Defense.Sword.WeaponGuard01"),
		float GuardFraction = 0.25f)
	{
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(
			MakeDefinitionCapture(RuleId, GuardFraction), Definition));
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
		uint64 ActivationSequence = 1101)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = GuardRunId;
		Capture.OwnerId = FGuid(
			0xDB150001, 0xDB150002, 0xDB150003, 0xDB150004);
		Capture.SourceEntityId = GuardAttackSourceId;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.P11_0Attack");
		Capture.Content.Version = TEXT("0.0.10.P11.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.0-ATTACK");
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
		const FShanmenDefenseLayer& GuardLayer,
		const FGameplayTag& DamageTag,
		bool bTargetIsLiving = true,
		int32 HitOrdinal = 0)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAttackAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = GuardSourceId;
		Request.Candidate.DetectorId = TEXT("Detector.Test.P11_0Attack");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Request.Candidate.HitOrdinal = HitOrdinal;
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.P11_0Attack");
		Request.Damage.RawDamage = 100.0f;
		Request.Damage.DamageTags.AddTag(DamageTag);
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 11;
		if (bTargetIsLiving)
		{
			Request.Defense.TargetTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
		}
		Request.Defense.Layers.Add(GuardLayer);
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
		const FShanmenWeaponGuardDefinition& Definition,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenActionTransitionReceipt& OutCommit,
		FShanmenWeaponGuardWindow& OutWindow,
		FShanmenWeaponGuardWindowReceipt& OutOpen)
	{
		StartActive(Action, OutRuntime, OutCommit);
		check(FShanmenWeaponGuardWindow::TryOpen(
			Action,
			Definition,
			OutCommit,
			OutRuntime,
			OutWindow,
			OutOpen));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardDefinitionContractTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuard.DefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardDefinitionContractTest::RunTest(const FString&)
{
	FShanmenWeaponGuardDefinition Definition;
	TestTrue(TEXT("Canonical ordinary guard definition captures immutably"),
		FShanmenWeaponGuardDefinition::TryCapture(
			MakeDefinitionCapture(), Definition)
			&& Definition.IsValid()
			&& Definition.GetActionDefinitionId()
				== FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId()
			&& Definition.GetRuleId()
				== FName(TEXT("Defense.Sword.WeaponGuard01"))
			&& FMath::IsNearlyEqual(Definition.GetGuardFraction(), 0.25f)
			&& Definition.GetRequiredDamageTags().HasTagExact(
				FShanmenCombatNativeTags::DamagePhysical())
			&& Definition.GetRequiredTargetTags().HasTagExact(
				FShanmenCombatNativeTags::TargetLiving()));

	FShanmenWeaponGuardDefinitionCapture Invalid = MakeDefinitionCapture();
	Invalid.ActionDefinitionId = TEXT("Combat.Action.Sword.GenericGuard");
	TestFalse(TEXT("Generic guard action identity fails closed"),
		FShanmenWeaponGuardDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture(NAME_None);
	TestFalse(TEXT("A named defense rule is mandatory"),
		FShanmenWeaponGuardDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture(TEXT("Defense.Sword.Zero"), 0.0f);
	TestFalse(TEXT("Zero guard has no active-defense meaning"),
		FShanmenWeaponGuardDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture(TEXT("Defense.Sword.Overflow"), 1.01f);
	TestFalse(TEXT("Guard fraction cannot exceed one"),
		FShanmenWeaponGuardDefinition::TryCapture(Invalid, Definition));
	Invalid = MakeDefinitionCapture();
	Invalid.BlockedDamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	TestFalse(TEXT("Parent-child required and blocked filters fail closed"),
		FShanmenWeaponGuardDefinition::TryCapture(Invalid, Definition));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardCommitBoundWindowTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuard.CommitBoundWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardCommitBoundWindowTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeGuardAction();
	const FShanmenWeaponGuardDefinition Definition = MakeDefinition();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Receipt;
	check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Receipt));
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	TestFalse(TEXT("Startup receipt cannot open an ordinary guard window"),
		FShanmenWeaponGuardWindow::TryOpen(
			Action, Definition, Receipt, Runtime, Window, Open));
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Startup, Receipt));
	TestTrue(TEXT("Exact Startup-to-Active commit opens one guard window"),
		FShanmenWeaponGuardWindow::TryOpen(
			Action, Definition, Receipt, Runtime, Window, Open)
			&& Window.IsValid() && Open.IsValid()
			&& Open.GetCommitTransition().CrossedCommitPointNow());

	FShanmenActionOrchestrator ForeignRuntime;
	FShanmenActionTransitionReceipt ForeignCommit;
	StartActive(
		MakeGuardAction(TEXT("TEST-DIGEST-P11.0-FOREIGN"), 1102),
		ForeignRuntime,
		ForeignCommit);
	FShanmenWeaponGuardWindow RejectedWindow;
	FShanmenWeaponGuardWindowReceipt RejectedOpen;
	TestFalse(TEXT("A foreign active runtime cannot validate this commit"),
		FShanmenWeaponGuardWindow::TryOpen(
			Action,
			Definition,
			Receipt,
			ForeignRuntime,
			RejectedWindow,
			RejectedOpen));

	const FShanmenCombatActionSnapshot Unarmed = MakeGuardAction(
		TEXT("TEST-DIGEST-P11.0-UNARMED"), 1103, false);
	FShanmenActionOrchestrator UnarmedRuntime;
	FShanmenActionTransitionReceipt UnarmedCommit;
	StartActive(Unarmed, UnarmedRuntime, UnarmedCommit);
	TestFalse(TEXT("Weapon guard requires a frozen source item identity"),
		FShanmenWeaponGuardWindow::TryOpen(
			Unarmed,
			Definition,
			UnarmedCommit,
			UnarmedRuntime,
			RejectedWindow,
			RejectedOpen));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardDefenseProjectionTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuard.DefenseProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardDefenseProjectionTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Commit;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(
		MakeGuardAction(),
		MakeDefinition(),
		Runtime,
		Commit,
		Window,
		Open);
	FShanmenWeaponGuardProjectionReceipt Projection;
	TestTrue(TEXT("Active window projects authored ordinary guard"),
		Window.TryProjectDefenseLayer(Runtime, Projection)
			&& Projection.IsValid()
			&& Projection.GetLayer().IsValid()
			&& Projection.GetLayer().Operation
				== EShanmenDefenseOperation::ReduceFraction
			&& Projection.GetLayer().Order == FShanmenDefenseOrder::Guard
			&& FMath::IsNearlyEqual(Projection.GetLayer().Magnitude, 0.25f)
			&& Projection.GetLayer().SourceInstanceId == GuardWeaponId
			&& !Projection.GetLayer().bRequiresCommitOnTrigger
			&& Projection.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard())
			&& !Projection.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard()));

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.GetLayer(),
			FShanmenCombatNativeTags::DamagePhysicalSlash()));
	TestTrue(TEXT("Twenty-five percent guard conserves a 100 point impact"),
		Result.bAccepted
			&& Result.Outcome == EShanmenDefenseOutcome::Mitigated
			&& Result.IsConserved()
			&& FMath::IsNearlyEqual(Result.PreventedDamage, 25.0f)
			&& FMath::IsNearlyEqual(Result.FinalDamage, 75.0f)
			&& Result.TriggeredLayers.Num() == 1
			&& Result.TriggeredLayers[0].SourceInstanceId == GuardWeaponId
			&& !Result.TriggeredLayers[0].bRequiresCommit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWeaponGuardTagFilteringTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuard.TagFiltering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardTagFilteringTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Commit;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(
		MakeGuardAction(),
		MakeDefinition(),
		Runtime,
		Commit,
		Window,
		Open);
	FShanmenWeaponGuardProjectionReceipt Projection;
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
	FShanmenWeaponGuardActivePhaseBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuard.ActivePhaseBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardActivePhaseBoundaryTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Commit;
	FShanmenWeaponGuardWindow Window;
	FShanmenWeaponGuardWindowReceipt Open;
	OpenWindow(
		MakeGuardAction(),
		MakeDefinition(),
		Runtime,
		Commit,
		Window,
		Open);
	FShanmenWeaponGuardProjectionReceipt Projection;
	check(Window.TryProjectDefenseLayer(Runtime, Projection));
	FShanmenActionTransitionReceipt Recovery;
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	TestFalse(TEXT("Recovery closes projection without a second lifetime owner"),
		Window.TryProjectDefenseLayer(Runtime, Projection));

	FShanmenActionOrchestrator InterruptedRuntime;
	FShanmenActionTransitionReceipt InterruptedCommit;
	FShanmenWeaponGuardWindow InterruptedWindow;
	FShanmenWeaponGuardWindowReceipt InterruptedOpen;
	OpenWindow(
		MakeGuardAction(TEXT("TEST-DIGEST-P11.0-INTERRUPTED"), 1104),
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
	FShanmenWeaponGuardDeterministicIdentityTest,
	"Shanmen.0_0_10.CombatRuntime.WeaponGuard.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWeaponGuardDeterministicIdentityTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeGuardAction();
	const FShanmenWeaponGuardDefinition Definition = MakeDefinition();
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenActionTransitionReceipt FirstCommit;
	FShanmenWeaponGuardWindow FirstWindow;
	FShanmenWeaponGuardWindowReceipt FirstOpen;
	OpenWindow(
		Action,
		Definition,
		FirstRuntime,
		FirstCommit,
		FirstWindow,
		FirstOpen);
	FShanmenWeaponGuardProjectionReceipt FirstProjection;
	check(FirstWindow.TryProjectDefenseLayer(
		FirstRuntime, FirstProjection));

	FShanmenActionOrchestrator ReplayRuntime;
	FShanmenActionTransitionReceipt ReplayCommit;
	FShanmenWeaponGuardWindow ReplayWindow;
	FShanmenWeaponGuardWindowReceipt ReplayOpen;
	OpenWindow(
		Action,
		Definition,
		ReplayRuntime,
		ReplayCommit,
		ReplayWindow,
		ReplayOpen);
	FShanmenWeaponGuardProjectionReceipt ReplayProjection;
	TestTrue(TEXT("Equivalent frozen input reproduces every guard identity"),
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
	FShanmenWeaponGuardWindow OtherWindow;
	FShanmenWeaponGuardWindowReceipt OtherOpen;
	OpenWindow(
		Action,
		MakeDefinition(TEXT("Defense.Sword.WeaponGuard01"), 0.4f),
		OtherRuntime,
		OtherCommit,
		OtherWindow,
		OtherOpen);
	TestTrue(TEXT("Different authored fraction produces a different window"),
		OtherOpen.GetWindowId() != FirstOpen.GetWindowId());
	return true;
}

#endif
