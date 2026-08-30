#include "ShanmenSpiritShield.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	float CanonicalZero(float Value)
	{
		return Value == 0.0f ? 0.0f : Value;
	}

	FString FloatBits(float Value)
	{
		Value = CanonicalZero(Value);
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
				if (RequiredTag.MatchesTag(BlockedTag))
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

	bool DefinitionsMatch(
		const FShanmenSpiritShieldDefinition& Left,
		const FShanmenSpiritShieldDefinition& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetRuleId() == Right.GetRuleId()
			&& Left.GetMaximumCapacity() == Right.GetMaximumCapacity()
			&& Left.GetRequiredDamageTags() == Right.GetRequiredDamageTags()
			&& Left.GetBlockedDamageTags() == Right.GetBlockedDamageTags()
			&& Left.GetRequiredSourceTags() == Right.GetRequiredSourceTags()
			&& Left.GetBlockedSourceTags() == Right.GetBlockedSourceTags()
			&& Left.GetRequiredTargetTags() == Right.GetRequiredTargetTags()
			&& Left.GetBlockedTargetTags() == Right.GetBlockedTargetTags();
	}

	FGuid MakeShieldInstanceId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritShieldDefinition& Definition)
	{
		if (!Action.IsValid() || !Definition.IsValid()
			|| Action.GetActionDefinitionId() != Definition.GetActionDefinitionId())
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
			FloatBits(Definition.GetMaximumCapacity())
		};
		AppendCanonicalTags(Parts, TEXT("RequiredDamage"), Definition.GetRequiredDamageTags());
		AppendCanonicalTags(Parts, TEXT("BlockedDamage"), Definition.GetBlockedDamageTags());
		AppendCanonicalTags(Parts, TEXT("RequiredSource"), Definition.GetRequiredSourceTags());
		AppendCanonicalTags(Parts, TEXT("BlockedSource"), Definition.GetBlockedSourceTags());
		AppendCanonicalTags(Parts, TEXT("RequiredTarget"), Definition.GetRequiredTargetTags());
		AppendCanonicalTags(Parts, TEXT("BlockedTarget"), Definition.GetBlockedTargetTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Instance.r1"), Parts);
	}

	FGuid MakeActivationReceiptId(const FGuid& ShieldInstanceId)
	{
		if (!ShieldInstanceId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Activation.r1"),
			{ GuidDigits(ShieldInstanceId) });
	}

	FGuid MakeLayerId(const FShanmenSpiritShieldActivationReceipt& Activation)
	{
		if (!Activation.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.DefenseLayer.r1"),
			{
				GuidDigits(Activation.GetShieldInstanceId()),
				Activation.GetDefinition().GetRuleId().ToString()
			});
	}

	FShanmenDefenseLayer MakeDefenseLayer(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		float AvailableCapacity)
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = MakeLayerId(Activation);
		Layer.RuleId = Activation.GetDefinition().GetRuleId();
		Layer.SourceInstanceId = Activation.GetShieldInstanceId();
		Layer.Operation = EShanmenDefenseOperation::AbsorbPoints;
		Layer.Order = FShanmenDefenseOrder::Shield;
		Layer.Magnitude = CanonicalZero(AvailableCapacity);
		Layer.bRequiresCommitOnTrigger = true;
		Layer.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
		Layer.RequiredDamageTags = Activation.GetDefinition().GetRequiredDamageTags();
		Layer.BlockedDamageTags = Activation.GetDefinition().GetBlockedDamageTags();
		Layer.RequiredSourceTags = Activation.GetDefinition().GetRequiredSourceTags();
		Layer.BlockedSourceTags = Activation.GetDefinition().GetBlockedSourceTags();
		Layer.RequiredTargetTags = Activation.GetDefinition().GetRequiredTargetTags();
		Layer.BlockedTargetTags = Activation.GetDefinition().GetBlockedTargetTags();
		return Layer;
	}

	bool LayerMatchesActivation(
		const FShanmenDefenseLayer& Layer,
		const FShanmenSpiritShieldActivationReceipt& Activation,
		float AvailableCapacity)
	{
		FGameplayTagContainer ExpectedLayerTags;
		ExpectedLayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
		const FShanmenSpiritShieldDefinition& Definition = Activation.GetDefinition();
		return Layer.IsValid()
			&& Layer.LayerId == MakeLayerId(Activation)
			&& Layer.RuleId == Definition.GetRuleId()
			&& Layer.SourceInstanceId == Activation.GetShieldInstanceId()
			&& Layer.Operation == EShanmenDefenseOperation::AbsorbPoints
			&& Layer.Order == FShanmenDefenseOrder::Shield
			&& Layer.Magnitude == AvailableCapacity
			&& Layer.bRequiresCommitOnTrigger
			&& Layer.LayerTags == ExpectedLayerTags
			&& Layer.RequiredDamageTags == Definition.GetRequiredDamageTags()
			&& Layer.BlockedDamageTags == Definition.GetBlockedDamageTags()
			&& Layer.RequiredSourceTags == Definition.GetRequiredSourceTags()
			&& Layer.BlockedSourceTags == Definition.GetBlockedSourceTags()
			&& Layer.RequiredTargetTags == Definition.GetRequiredTargetTags()
			&& Layer.BlockedTargetTags == Definition.GetBlockedTargetTags();
	}

	FGuid MakeProjectionId(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		int64 AuthorityRevision,
		float AvailableCapacity)
	{
		if (!Activation.IsValid() || AuthorityRevision < 0
			|| !FMath::IsFinite(AvailableCapacity) || AvailableCapacity <= 0.0f)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Projection.r1"),
			{
				GuidDigits(Activation.GetReceiptId()),
				FString::Printf(TEXT("%lld"), AuthorityRevision),
				FloatBits(AvailableCapacity)
			});
	}

	bool IsDeactivationReasonValid(EShanmenSpiritShieldDeactivationReason Reason)
	{
		return Reason == EShanmenSpiritShieldDeactivationReason::Explicit
			|| Reason == EShanmenSpiritShieldDeactivationReason::DurationElapsed
			|| Reason == EShanmenSpiritShieldDeactivationReason::Interrupted
			|| Reason == EShanmenSpiritShieldDeactivationReason::OwnerEnded;
	}

	FGuid MakeDeactivationReceiptId(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		EShanmenSpiritShieldDeactivationReason Reason)
	{
		if (!Activation.IsValid() || !IsDeactivationReasonValid(Reason))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Deactivation.r1"),
			{
				GuidDigits(Activation.GetReceiptId()),
				FString::FromInt(static_cast<int32>(Reason))
			});
	}
}

FName FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Spell.SpiritShield01");
}

bool FShanmenSpiritShieldDefinition::TryCapture(
	const FShanmenSpiritShieldDefinitionCapture& Capture,
	FShanmenSpiritShieldDefinition& OutDefinition)
{
	OutDefinition = FShanmenSpiritShieldDefinition();
	OutDefinition.ActionDefinitionId = Capture.ActionDefinitionId;
	OutDefinition.RuleId = Capture.RuleId;
	OutDefinition.MaximumCapacity = CanonicalZero(Capture.MaximumCapacity);
	OutDefinition.RequiredDamageTags = Capture.RequiredDamageTags;
	OutDefinition.BlockedDamageTags = Capture.BlockedDamageTags;
	OutDefinition.RequiredSourceTags = Capture.RequiredSourceTags;
	OutDefinition.BlockedSourceTags = Capture.BlockedSourceTags;
	OutDefinition.RequiredTargetTags = Capture.RequiredTargetTags;
	OutDefinition.BlockedTargetTags = Capture.BlockedTargetTags;
	if (!OutDefinition.IsValid())
	{
		OutDefinition = FShanmenSpiritShieldDefinition();
		return false;
	}
	return true;
}

bool FShanmenSpiritShieldDefinition::IsValid() const
{
	return ActionDefinitionId == CanonicalActionDefinitionId()
		&& !RuleId.IsNone()
		&& FMath::IsFinite(MaximumCapacity)
		&& MaximumCapacity > 0.0f
		&& !TagsConflict(RequiredDamageTags, BlockedDamageTags)
		&& !TagsConflict(RequiredSourceTags, BlockedSourceTags)
		&& !TagsConflict(RequiredTargetTags, BlockedTargetTags);
}

bool FShanmenSpiritShieldActivationReceipt::IsValid() const
{
	return Action.IsValid()
		&& Definition.IsValid()
		&& Action.GetActionDefinitionId() == Definition.GetActionDefinitionId()
		&& ShieldInstanceId == MakeShieldInstanceId(Action, Definition)
		&& ReceiptId == MakeActivationReceiptId(ShieldInstanceId);
}

bool FShanmenSpiritShieldProjectionReceipt::IsValid() const
{
	return Activation.IsValid()
		&& AuthorityRevision >= 0
		&& FMath::IsFinite(AvailableCapacity)
		&& AvailableCapacity > 0.0f
		&& AvailableCapacity <= Activation.GetDefinition().GetMaximumCapacity()
		&& LayerMatchesActivation(Layer, Activation, AvailableCapacity)
		&& ProjectionId == MakeProjectionId(
			Activation, AuthorityRevision, AvailableCapacity);
}

bool FShanmenSpiritShieldDeactivationReceipt::IsValid() const
{
	return Activation.IsValid()
		&& IsDeactivationReasonValid(Reason)
		&& ReceiptId == MakeDeactivationReceiptId(Activation, Reason);
}

bool FShanmenSpiritShieldRuntime::TryPrepare(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritShieldDefinition& Definition,
	FShanmenSpiritShieldRuntime& OutRuntime)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenSpiritShieldDefinition FrozenDefinition = Definition;
	OutRuntime.Reset();
	if (!FrozenAction.IsValid()
		|| !FrozenDefinition.IsValid()
		|| FrozenAction.GetActionDefinitionId()
			!= FrozenDefinition.GetActionDefinitionId())
	{
		return false;
	}

	OutRuntime.Action = FrozenAction;
	OutRuntime.Definition = FrozenDefinition;
	OutRuntime.State = EShanmenSpiritShieldState::Prepared;
	return OutRuntime.IsValid();
}

bool FShanmenSpiritShieldRuntime::IsValid() const
{
	if (!Action.IsValid()
		|| !Definition.IsValid()
		|| Action.GetActionDefinitionId() != Definition.GetActionDefinitionId())
	{
		return false;
	}

	if (State == EShanmenSpiritShieldState::Prepared)
	{
		return !ActivationReceipt.IsValid()
			&& !LastProjectionReceipt.IsValid()
			&& !DeactivationReceipt.IsValid();
	}

	if (!ActivationReceipt.IsValid()
		|| !ActionsMatch(ActivationReceipt.GetAction(), Action)
		|| !DefinitionsMatch(ActivationReceipt.GetDefinition(), Definition))
	{
		return false;
	}

	if (LastProjectionReceipt.IsValid()
		&& LastProjectionReceipt.GetActivation().GetReceiptId()
			!= ActivationReceipt.GetReceiptId())
	{
		return false;
	}

	if (State == EShanmenSpiritShieldState::Active)
	{
		return !DeactivationReceipt.IsValid();
	}
	if (State == EShanmenSpiritShieldState::Deactivated)
	{
		return DeactivationReceipt.IsValid()
			&& DeactivationReceipt.GetActivation().GetReceiptId()
				== ActivationReceipt.GetReceiptId();
	}
	return false;
}

bool FShanmenSpiritShieldRuntime::TryActivate(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenSpiritShieldActivationReceipt& OutReceipt)
{
	OutReceipt = FShanmenSpiritShieldActivationReceipt();
	if (!IsValid() || !MatchesActionRuntime(ActionRuntime))
	{
		return false;
	}

	if (ActivationReceipt.IsValid())
	{
		OutReceipt = ActivationReceipt;
		return true;
	}
	if (State != EShanmenSpiritShieldState::Prepared
		|| !ActionRuntime.CanEmitCandidates())
	{
		return false;
	}

	FShanmenSpiritShieldActivationReceipt Candidate;
	Candidate.Action = Action;
	Candidate.Definition = Definition;
	Candidate.ShieldInstanceId = MakeShieldInstanceId(Action, Definition);
	Candidate.ReceiptId = MakeActivationReceiptId(Candidate.ShieldInstanceId);
	if (!Candidate.IsValid())
	{
		return false;
	}

	ActivationReceipt = Candidate;
	State = EShanmenSpiritShieldState::Active;
	OutReceipt = Candidate;
	return IsValid();
}

bool FShanmenSpiritShieldRuntime::TryProjectDefenseLayer(
	int64 AuthorityRevision,
	float AvailableCapacity,
	FShanmenSpiritShieldProjectionReceipt& OutReceipt)
{
	OutReceipt = FShanmenSpiritShieldProjectionReceipt();
	AvailableCapacity = CanonicalZero(AvailableCapacity);
	if (!IsValid()
		|| State != EShanmenSpiritShieldState::Active
		|| AuthorityRevision < 0
		|| !FMath::IsFinite(AvailableCapacity)
		|| AvailableCapacity <= 0.0f
		|| AvailableCapacity > Definition.GetMaximumCapacity())
	{
		return false;
	}

	FShanmenSpiritShieldProjectionReceipt Candidate;
	Candidate.Activation = ActivationReceipt;
	Candidate.AuthorityRevision = AuthorityRevision;
	Candidate.AvailableCapacity = AvailableCapacity;
	Candidate.Layer = MakeDefenseLayer(ActivationReceipt, AvailableCapacity);
	Candidate.ProjectionId = MakeProjectionId(
		ActivationReceipt, AuthorityRevision, AvailableCapacity);
	if (!Candidate.IsValid())
	{
		return false;
	}

	if (LastProjectionReceipt.IsValid())
	{
		if (AuthorityRevision == LastProjectionReceipt.GetAuthorityRevision())
		{
			if (Candidate.GetProjectionId()
				!= LastProjectionReceipt.GetProjectionId())
			{
				return false;
			}
			OutReceipt = LastProjectionReceipt;
			return true;
		}
		if (AuthorityRevision < LastProjectionReceipt.GetAuthorityRevision()
			|| AvailableCapacity > LastProjectionReceipt.GetAvailableCapacity())
		{
			return false;
		}
	}

	LastProjectionReceipt = Candidate;
	OutReceipt = Candidate;
	return IsValid();
}

bool FShanmenSpiritShieldRuntime::TryDeactivate(
	const FGuid& ExpectedShieldInstanceId,
	EShanmenSpiritShieldDeactivationReason Reason,
	FShanmenSpiritShieldDeactivationReceipt& OutReceipt)
{
	OutReceipt = FShanmenSpiritShieldDeactivationReceipt();
	if (!IsValid()
		|| !ExpectedShieldInstanceId.IsValid()
		|| !ActivationReceipt.IsValid()
		|| ExpectedShieldInstanceId != ActivationReceipt.GetShieldInstanceId()
		|| !IsDeactivationReasonValid(Reason))
	{
		return false;
	}

	if (DeactivationReceipt.IsValid())
	{
		if (DeactivationReceipt.GetReason() != Reason)
		{
			return false;
		}
		OutReceipt = DeactivationReceipt;
		return true;
	}
	if (State != EShanmenSpiritShieldState::Active)
	{
		return false;
	}

	FShanmenSpiritShieldDeactivationReceipt Candidate;
	Candidate.Activation = ActivationReceipt;
	Candidate.Reason = Reason;
	Candidate.ReceiptId = MakeDeactivationReceiptId(ActivationReceipt, Reason);
	if (!Candidate.IsValid())
	{
		return false;
	}

	DeactivationReceipt = Candidate;
	State = EShanmenSpiritShieldState::Deactivated;
	OutReceipt = Candidate;
	return IsValid();
}

void FShanmenSpiritShieldRuntime::Reset()
{
	*this = FShanmenSpiritShieldRuntime();
}

bool FShanmenSpiritShieldRuntime::MatchesActionRuntime(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	return ActionRuntime.IsValid()
		&& ActionsMatch(ActionRuntime.GetAction(), Action);
}
