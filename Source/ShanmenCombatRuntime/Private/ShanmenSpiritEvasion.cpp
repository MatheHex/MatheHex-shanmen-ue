#include "ShanmenSpiritEvasion.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
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
		const FShanmenSpiritEvasionDefinition& Definition,
		const FShanmenActionTransitionReceipt& CommitTransition)
	{
		if (!Action.IsValid() || !Definition.IsValid()
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
			FString::Printf(TEXT("%lld"), CommitTransition.GetSequence())
		};
		AppendCanonicalTags(Parts, TEXT("RequiredDamage"), Definition.GetRequiredDamageTags());
		AppendCanonicalTags(Parts, TEXT("BlockedDamage"), Definition.GetBlockedDamageTags());
		AppendCanonicalTags(Parts, TEXT("RequiredSource"), Definition.GetRequiredSourceTags());
		AppendCanonicalTags(Parts, TEXT("BlockedSource"), Definition.GetBlockedSourceTags());
		AppendCanonicalTags(Parts, TEXT("RequiredTarget"), Definition.GetRequiredTargetTags());
		AppendCanonicalTags(Parts, TEXT("BlockedTarget"), Definition.GetBlockedTargetTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritEvasion.Window.r1"), Parts);
	}

	FGuid MakeWindowReceiptId(const FGuid& WindowId)
	{
		if (!WindowId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritEvasion.WindowReceipt.r1"),
			{ GuidDigits(WindowId) });
	}

	FGuid MakeLayerId(const FShanmenSpiritEvasionWindowReceipt& Window)
	{
		if (!Window.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritEvasion.DefenseLayer.r1"),
			{ GuidDigits(Window.GetWindowId()), Window.GetDefinition().GetRuleId().ToString() });
	}

	FShanmenDefenseLayer MakeDefenseLayer(
		const FShanmenSpiritEvasionWindowReceipt& Window)
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = MakeLayerId(Window);
		Layer.RuleId = Window.GetDefinition().GetRuleId();
		Layer.SourceInstanceId = Window.GetWindowId();
		Layer.Operation = EShanmenDefenseOperation::PreventAll;
		Layer.Order = FShanmenDefenseOrder::Avoidance;
		Layer.Magnitude = 0.0f;
		Layer.bRequiresCommitOnTrigger = false;
		Layer.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseEvade());
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
		const FShanmenSpiritEvasionWindowReceipt& Window)
	{
		FGameplayTagContainer ExpectedTags;
		ExpectedTags.AddTag(FShanmenCombatNativeTags::DefenseEvade());
		const FShanmenSpiritEvasionDefinition& Definition = Window.GetDefinition();
		return Layer.IsValid()
			&& Layer.LayerId == MakeLayerId(Window)
			&& Layer.RuleId == Definition.GetRuleId()
			&& Layer.SourceInstanceId == Window.GetWindowId()
			&& Layer.Operation == EShanmenDefenseOperation::PreventAll
			&& Layer.Order == FShanmenDefenseOrder::Avoidance
			&& Layer.Magnitude == 0.0f
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
		const FShanmenSpiritEvasionWindowReceipt& Window,
		const FShanmenDefenseLayer& Layer)
	{
		if (!Window.IsValid() || !LayerMatchesWindow(Layer, Window))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritEvasion.Projection.r1"),
			{ GuidDigits(Window.GetReceiptId()), GuidDigits(Layer.LayerId) });
	}
}

FName FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Spell.SpiritEvasion01");
}

bool FShanmenSpiritEvasionDefinition::TryCapture(
	const FShanmenSpiritEvasionDefinitionCapture& Capture,
	FShanmenSpiritEvasionDefinition& OutDefinition)
{
	FShanmenSpiritEvasionDefinition Candidate;
	Candidate.ActionDefinitionId = Capture.ActionDefinitionId;
	Candidate.RuleId = Capture.RuleId;
	Candidate.RequiredDamageTags = Capture.RequiredDamageTags;
	Candidate.BlockedDamageTags = Capture.BlockedDamageTags;
	Candidate.RequiredSourceTags = Capture.RequiredSourceTags;
	Candidate.BlockedSourceTags = Capture.BlockedSourceTags;
	Candidate.RequiredTargetTags = Capture.RequiredTargetTags;
	Candidate.BlockedTargetTags = Capture.BlockedTargetTags;
	if (!Candidate.IsValid())
	{
		OutDefinition = FShanmenSpiritEvasionDefinition();
		return false;
	}
	OutDefinition = Candidate;
	return true;
}

bool FShanmenSpiritEvasionDefinition::IsValid() const
{
	return ActionDefinitionId == CanonicalActionDefinitionId()
		&& !RuleId.IsNone()
		&& !TagsConflict(RequiredDamageTags, BlockedDamageTags)
		&& !TagsConflict(RequiredSourceTags, BlockedSourceTags)
		&& !TagsConflict(RequiredTargetTags, BlockedTargetTags);
}

bool FShanmenSpiritEvasionWindowReceipt::IsValid() const
{
	return ReceiptId.IsValid()
		&& WindowId.IsValid()
		&& Action.IsValid()
		&& Definition.IsValid()
		&& Action.GetActionDefinitionId() == Definition.GetActionDefinitionId()
		&& IsCommitTransitionFor(CommitTransition, Action)
		&& WindowId == MakeWindowId(Action, Definition, CommitTransition)
		&& ReceiptId == MakeWindowReceiptId(WindowId);
}

bool FShanmenSpiritEvasionProjectionReceipt::IsValid() const
{
	return ProjectionId.IsValid()
		&& Window.IsValid()
		&& LayerMatchesWindow(Layer, Window)
		&& ProjectionId == MakeProjectionId(Window, Layer);
}

bool FShanmenSpiritEvasionWindow::TryOpen(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritEvasionDefinition& Definition,
	const FShanmenActionTransitionReceipt& CommitTransition,
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenSpiritEvasionWindow& OutWindow,
	FShanmenSpiritEvasionWindowReceipt& OutReceipt)
{
	OutWindow.Reset();
	OutReceipt = FShanmenSpiritEvasionWindowReceipt();
	if (!Action.IsValid() || !Definition.IsValid()
		|| Action.GetActionDefinitionId() != Definition.GetActionDefinitionId()
		|| !IsCurrentActiveRuntime(ActionRuntime, Action, CommitTransition))
	{
		return false;
	}

	FShanmenSpiritEvasionWindow Candidate;
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

bool FShanmenSpiritEvasionWindow::IsValid() const
{
	return bOpen && OpenReceipt.IsValid();
}

bool FShanmenSpiritEvasionWindow::IsActiveFor(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	return IsValid()
		&& IsCurrentActiveRuntime(
			ActionRuntime,
			OpenReceipt.GetAction(),
			OpenReceipt.GetCommitTransition());
}

bool FShanmenSpiritEvasionWindow::TryProjectDefenseLayer(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenSpiritEvasionProjectionReceipt& OutReceipt) const
{
	OutReceipt = FShanmenSpiritEvasionProjectionReceipt();
	if (!IsActiveFor(ActionRuntime))
	{
		return false;
	}

	FShanmenSpiritEvasionProjectionReceipt Candidate;
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

void FShanmenSpiritEvasionWindow::Reset()
{
	*this = FShanmenSpiritEvasionWindow();
}
