#include "ShanmenWeaponGuard.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString FloatBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	bool TagsConflict(
		const FGameplayTagContainer& Required,
		const FGameplayTagContainer& Blocked)
	{
		TArray<FGameplayTag> RequiredTags;
		TArray<FGameplayTag> BlockedTags;
		Required.GetGameplayTagArray(RequiredTags);
		Blocked.GetGameplayTagArray(BlockedTags);
		for (const FGameplayTag& RequiredTag : RequiredTags)
		{
			for (const FGameplayTag& BlockedTag : BlockedTags)
			{
				if (RequiredTag.MatchesTag(BlockedTag)
					|| BlockedTag.MatchesTag(RequiredTag))
				{
					return true;
				}
			}
		}
		return false;
	}

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const TCHAR* Label,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		Parts.Add(Label);
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId() == Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool IsCommitTransitionFor(
		const FShanmenActionTransitionReceipt& Transition,
		const FShanmenCombatActionSnapshot& Action)
	{
		return Transition.IsValid()
			&& Action.IsValid()
			&& Transition.GetActivationId() == Action.GetActivationId()
			&& Transition.GetFromPhase() == EShanmenCombatActionPhase::Startup
			&& Transition.GetToPhase() == EShanmenCombatActionPhase::Active
			&& Transition.GetTerminalReason() == EShanmenActionTerminalReason::None
			&& Transition.CrossedCommitPointNow()
			&& Transition.HasReachedCommitPoint();
	}

	bool IsCurrentActiveRuntime(
		const FShanmenActionOrchestrator& Runtime,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenActionTransitionReceipt& CommitTransition)
	{
		return Runtime.IsValid()
			&& !Runtime.IsTerminal()
			&& Runtime.GetPhase() == EShanmenCombatActionPhase::Active
			&& Runtime.HasReachedCommitPoint()
			&& Runtime.GetNextSequence() == CommitTransition.GetSequence() + 1
			&& ActionsMatch(Runtime.GetAction(), Action)
			&& IsCommitTransitionFor(CommitTransition, Action);
	}

	FGuid MakeWindowId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenWeaponGuardDefinition& Definition,
		const FShanmenActionTransitionReceipt& CommitTransition)
	{
		if (!Action.IsValid()
			|| !Action.GetSourceItemInstanceId().IsValid()
			|| !Definition.IsValid()
			|| Action.GetActionDefinitionId() != Definition.GetActionDefinitionId()
			|| !IsCommitTransitionFor(CommitTransition, Action))
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			GuidDigits(Action.GetRunId()),
			GuidDigits(Action.GetOwnerId()),
			GuidDigits(Action.GetActivationId()),
			GuidDigits(Action.GetSourceEntityId()),
			GuidDigits(Action.GetSourceItemInstanceId()),
			Action.GetActionDefinitionId().ToString(),
			Action.GetContent().Version.ToString(),
			Action.GetContent().Digest,
			Definition.GetRuleId().ToString(),
			FloatBits(Definition.GetGuardFraction()),
			FString::Printf(TEXT("%lld"), CommitTransition.GetSequence())
		};
		AppendCanonicalTags(Parts, TEXT("RequiredDamage"), Definition.GetRequiredDamageTags());
		AppendCanonicalTags(Parts, TEXT("BlockedDamage"), Definition.GetBlockedDamageTags());
		AppendCanonicalTags(Parts, TEXT("RequiredSource"), Definition.GetRequiredSourceTags());
		AppendCanonicalTags(Parts, TEXT("BlockedSource"), Definition.GetBlockedSourceTags());
		AppendCanonicalTags(Parts, TEXT("RequiredTarget"), Definition.GetRequiredTargetTags());
		AppendCanonicalTags(Parts, TEXT("BlockedTarget"), Definition.GetBlockedTargetTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.Window.r1"), Parts);
	}

	FGuid MakeWindowReceiptId(const FGuid& WindowId)
	{
		if (!WindowId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.WindowReceipt.r1"),
			{ GuidDigits(WindowId) });
	}

	FGuid MakeLayerId(const FShanmenWeaponGuardWindowReceipt& Window)
	{
		if (!Window.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.DefenseLayer.r1"),
			{
				GuidDigits(Window.GetWindowId()),
				GuidDigits(Window.GetAction().GetSourceItemInstanceId()),
				Window.GetDefinition().GetRuleId().ToString()
			});
	}

	FShanmenDefenseLayer MakeDefenseLayer(
		const FShanmenWeaponGuardWindowReceipt& Window)
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = MakeLayerId(Window);
		Layer.RuleId = Window.GetDefinition().GetRuleId();
		Layer.SourceInstanceId = Window.GetAction().GetSourceItemInstanceId();
		Layer.Operation = EShanmenDefenseOperation::ReduceFraction;
		Layer.Order = FShanmenDefenseOrder::Guard;
		Layer.Magnitude = Window.GetDefinition().GetGuardFraction();
		Layer.bRequiresCommitOnTrigger = false;
		Layer.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseGuard());
		Layer.RequiredDamageTags = Window.GetDefinition().GetRequiredDamageTags();
		Layer.BlockedDamageTags = Window.GetDefinition().GetBlockedDamageTags();
		Layer.RequiredSourceTags = Window.GetDefinition().GetRequiredSourceTags();
		Layer.BlockedSourceTags = Window.GetDefinition().GetBlockedSourceTags();
		Layer.RequiredTargetTags = Window.GetDefinition().GetRequiredTargetTags();
		Layer.BlockedTargetTags = Window.GetDefinition().GetBlockedTargetTags();
		return Layer;
	}

	bool LayerMatchesWindow(
		const FShanmenDefenseLayer& Layer,
		const FShanmenWeaponGuardWindowReceipt& Window)
	{
		FGameplayTagContainer ExpectedTags;
		ExpectedTags.AddTag(FShanmenCombatNativeTags::DefenseGuard());
		const FShanmenWeaponGuardDefinition& Definition = Window.GetDefinition();
		return Layer.IsValid()
			&& Layer.LayerId == MakeLayerId(Window)
			&& Layer.RuleId == Definition.GetRuleId()
			&& Layer.SourceInstanceId == Window.GetAction().GetSourceItemInstanceId()
			&& Layer.Operation == EShanmenDefenseOperation::ReduceFraction
			&& Layer.Order == FShanmenDefenseOrder::Guard
			&& Layer.Magnitude == Definition.GetGuardFraction()
			&& !Layer.bRequiresCommitOnTrigger
			&& Layer.LayerTags == ExpectedTags
			&& Layer.RequiredDamageTags == Definition.GetRequiredDamageTags()
			&& Layer.BlockedDamageTags == Definition.GetBlockedDamageTags()
			&& Layer.RequiredSourceTags == Definition.GetRequiredSourceTags()
			&& Layer.BlockedSourceTags == Definition.GetBlockedSourceTags()
			&& Layer.RequiredTargetTags == Definition.GetRequiredTargetTags()
			&& Layer.BlockedTargetTags == Definition.GetBlockedTargetTags();
	}

	FGuid MakeProjectionId(
		const FShanmenWeaponGuardWindowReceipt& Window,
		const FShanmenDefenseLayer& Layer)
	{
		if (!Window.IsValid() || !LayerMatchesWindow(Layer, Window))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.Projection.r1"),
			{ GuidDigits(Window.GetReceiptId()), GuidDigits(Layer.LayerId) });
	}
}

FName FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Sword.Guard01");
}

bool FShanmenWeaponGuardDefinition::TryCapture(
	const FShanmenWeaponGuardDefinitionCapture& Capture,
	FShanmenWeaponGuardDefinition& OutDefinition)
{
	FShanmenWeaponGuardDefinition Candidate;
	Candidate.ActionDefinitionId = Capture.ActionDefinitionId;
	Candidate.RuleId = Capture.RuleId;
	Candidate.GuardFraction = Capture.GuardFraction;
	Candidate.RequiredDamageTags = Capture.RequiredDamageTags;
	Candidate.BlockedDamageTags = Capture.BlockedDamageTags;
	Candidate.RequiredSourceTags = Capture.RequiredSourceTags;
	Candidate.BlockedSourceTags = Capture.BlockedSourceTags;
	Candidate.RequiredTargetTags = Capture.RequiredTargetTags;
	Candidate.BlockedTargetTags = Capture.BlockedTargetTags;
	if (!Candidate.IsValid())
	{
		OutDefinition = FShanmenWeaponGuardDefinition();
		return false;
	}
	OutDefinition = Candidate;
	return true;
}

bool FShanmenWeaponGuardDefinition::IsValid() const
{
	return ActionDefinitionId == CanonicalActionDefinitionId()
		&& !RuleId.IsNone()
		&& FMath::IsFinite(GuardFraction)
		&& GuardFraction > 0.0f
		&& GuardFraction <= 1.0f
		&& !TagsConflict(RequiredDamageTags, BlockedDamageTags)
		&& !TagsConflict(RequiredSourceTags, BlockedSourceTags)
		&& !TagsConflict(RequiredTargetTags, BlockedTargetTags);
}

bool FShanmenWeaponGuardWindowReceipt::IsValid() const
{
	return ReceiptId.IsValid()
		&& WindowId.IsValid()
		&& Action.IsValid()
		&& Action.GetSourceItemInstanceId().IsValid()
		&& Definition.IsValid()
		&& Action.GetActionDefinitionId() == Definition.GetActionDefinitionId()
		&& IsCommitTransitionFor(CommitTransition, Action)
		&& WindowId == MakeWindowId(Action, Definition, CommitTransition)
		&& ReceiptId == MakeWindowReceiptId(WindowId);
}

bool FShanmenWeaponGuardProjectionReceipt::IsValid() const
{
	return ProjectionId.IsValid()
		&& Window.IsValid()
		&& LayerMatchesWindow(Layer, Window)
		&& ProjectionId == MakeProjectionId(Window, Layer);
}

bool FShanmenWeaponGuardWindow::TryOpen(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenWeaponGuardDefinition& Definition,
	const FShanmenActionTransitionReceipt& CommitTransition,
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWeaponGuardWindow& OutWindow,
	FShanmenWeaponGuardWindowReceipt& OutReceipt)
{
	OutWindow.Reset();
	OutReceipt = FShanmenWeaponGuardWindowReceipt();
	if (!Action.IsValid()
		|| !Action.GetSourceItemInstanceId().IsValid()
		|| !Definition.IsValid()
		|| Action.GetActionDefinitionId() != Definition.GetActionDefinitionId()
		|| !IsCurrentActiveRuntime(ActionRuntime, Action, CommitTransition))
	{
		return false;
	}

	FShanmenWeaponGuardWindow Candidate;
	Candidate.OpenReceipt.Action = Action;
	Candidate.OpenReceipt.Definition = Definition;
	Candidate.OpenReceipt.CommitTransition = CommitTransition;
	Candidate.OpenReceipt.WindowId = MakeWindowId(
		Action, Definition, CommitTransition);
	Candidate.OpenReceipt.ReceiptId = MakeWindowReceiptId(
		Candidate.OpenReceipt.WindowId);
	Candidate.bOpen = true;
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutWindow = Candidate;
	OutReceipt = Candidate.OpenReceipt;
	return true;
}

bool FShanmenWeaponGuardWindow::IsValid() const
{
	return bOpen && OpenReceipt.IsValid();
}

bool FShanmenWeaponGuardWindow::IsActiveFor(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	return IsValid()
		&& IsCurrentActiveRuntime(
			ActionRuntime,
			OpenReceipt.GetAction(),
			OpenReceipt.GetCommitTransition());
}

bool FShanmenWeaponGuardWindow::TryProjectDefenseLayer(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenWeaponGuardProjectionReceipt& OutReceipt) const
{
	OutReceipt = FShanmenWeaponGuardProjectionReceipt();
	if (!IsActiveFor(ActionRuntime))
	{
		return false;
	}

	FShanmenWeaponGuardProjectionReceipt Candidate;
	Candidate.Window = OpenReceipt;
	Candidate.Layer = MakeDefenseLayer(OpenReceipt);
	Candidate.ProjectionId = MakeProjectionId(
		Candidate.Window, Candidate.Layer);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutReceipt = Candidate;
	return true;
}

void FShanmenWeaponGuardWindow::Reset()
{
	*this = FShanmenWeaponGuardWindow();
}
