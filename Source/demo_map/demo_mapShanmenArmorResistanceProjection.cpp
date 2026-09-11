#include "demo_mapShanmenArmorResistanceProjection.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FName MakeRuleId(const FGameplayTag& DamageTag)
	{
		return FName(*FString::Printf(
			TEXT("Combat.Defense.Player.ArmorResistance.%s.r1"),
			*DamageTag.ToString()));
	}

	FGuid MakeLayerId(
		const FGuid& ArmorItemInstanceId,
		const FGuid& TargetEntityId,
		FName DefinitionId,
		const FGameplayTag& DamageTag)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.ArmorResistance.Layer.r1"),
			{
				GuidDigits(TargetEntityId),
				GuidDigits(ArmorItemInstanceId),
				DefinitionId.ToString(),
				DamageTag.ToString()
			});
	}

	Fdemo_mapShanmenArmorResistanceProjectionResult Reject(
		Edemo_mapShanmenArmorResistanceProjectionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenArmorResistanceProjectionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	bool HasAmbiguousTags(
		const TArray<Fdemo_mapItemDamageResistance>& Resistances)
	{
		for (int32 Index = 0; Index < Resistances.Num(); ++Index)
		{
			for (int32 OtherIndex = 0;
				OtherIndex < Index;
				++OtherIndex)
			{
				const FGameplayTag Left = Resistances[Index].DamageTag;
				const FGameplayTag Right = Resistances[OtherIndex].DamageTag;
				if (Left.MatchesTag(Right) || Right.MatchesTag(Left))
				{
					return true;
				}
			}
		}
		return false;
	}
}

Fdemo_mapShanmenArmorResistanceProjectionResult
Fdemo_mapShanmenArmorResistanceProjection::TryProject(
	const Fdemo_mapItemDefinition& Definition,
	const FGuid& ArmorItemInstanceId,
	const FGuid& TargetEntityId,
	const FShanmenDefenseSnapshot& BaseDefense)
{
	if (!ArmorItemInstanceId.IsValid()
		|| !TargetEntityId.IsValid()
		|| !BaseDefense.IsValid()
		|| !BaseDefense.TargetTags.HasTagExact(
			FShanmenCombatNativeTags::TargetLiving()))
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceProjectionStatus::InvalidInput,
			TEXT("Armor resistance projection requires exact item and living target identities plus a valid base defense."));
	}

	const bool bHasSemantic = Definition.HasGameplaySemantic(
		Edemo_mapItemGameplaySemantic::DamageResistance);
	const bool bHasMetadata = !Definition.DamageResistances.IsEmpty();
	if (!bHasSemantic && !bHasMetadata)
	{
		Fdemo_mapShanmenArmorResistanceProjectionResult Result;
		Result.Status =
			Edemo_mapShanmenArmorResistanceProjectionStatus::NotApplicable;
		Result.Defense = BaseDefense;
		Result.Diagnostic =
			TEXT("The exact item definition does not authorize damage resistance.");
		return Result;
	}
	if (bHasSemantic != bHasMetadata
		|| Definition.DefinitionId.IsNone()
		|| Definition.CategoryId != Fdemo_mapItemIds::ArmorCategory
		|| Definition.MaxStackSize != 1
		|| Definition.EquipmentSlotId != Fdemo_mapItemIds::ArmorSlot
		|| !Definition.CompatibleSlotIds.Contains(Fdemo_mapItemIds::ArmorSlot)
		|| Definition.DamageResistances.ContainsByPredicate(
			[](const Fdemo_mapItemDamageResistance& Resistance)
			{
				return !Resistance.IsValid();
			})
		|| HasAmbiguousTags(Definition.DamageResistances))
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceProjectionStatus::InvalidDefinition,
			TEXT("Armor resistance metadata is absent, malformed, ambiguous, or attached to an incompatible item definition."));
	}

	TArray<const Fdemo_mapItemDamageResistance*> OrderedResistances;
	OrderedResistances.Reserve(Definition.DamageResistances.Num());
	for (const Fdemo_mapItemDamageResistance& Resistance :
		Definition.DamageResistances)
	{
		OrderedResistances.Add(&Resistance);
	}
	OrderedResistances.Sort(
		[](const Fdemo_mapItemDamageResistance& Left,
			const Fdemo_mapItemDamageResistance& Right)
		{
			return Left.DamageTag.ToString() < Right.DamageTag.ToString();
		});

	FShanmenDefenseSnapshot Candidate = BaseDefense;
	TArray<FGuid> LayerIds;
	LayerIds.Reserve(OrderedResistances.Num());
	for (const Fdemo_mapItemDamageResistance* Resistance : OrderedResistances)
	{
		const FGuid LayerId = MakeLayerId(
			ArmorItemInstanceId,
			TargetEntityId,
			Definition.DefinitionId,
			Resistance->DamageTag);
		if (!LayerId.IsValid()
			|| Candidate.Layers.ContainsByPredicate(
				[&LayerId](const FShanmenDefenseLayer& Layer)
				{
					return Layer.LayerId == LayerId;
				}))
		{
			return Reject(
				Edemo_mapShanmenArmorResistanceProjectionStatus::LayerConflict,
				TEXT("Armor resistance projection conflicts with an existing canonical layer identity."));
		}

		FShanmenDefenseLayer& Layer = Candidate.Layers.AddDefaulted_GetRef();
		Layer.LayerId = LayerId;
		Layer.RuleId = MakeRuleId(Resistance->DamageTag);
		Layer.SourceInstanceId = ArmorItemInstanceId;
		Layer.Operation = EShanmenDefenseOperation::ReduceFraction;
		Layer.Order = FShanmenDefenseOrder::Resistance;
		Layer.Magnitude = Resistance->ResistanceFraction;
		Layer.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseArmor());
		Layer.RequiredDamageTags.AddTag(Resistance->DamageTag);
		Layer.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		LayerIds.Add(LayerId);
	}

	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceProjectionStatus::InvalidDefinition,
			TEXT("Projected armor resistance did not produce a canonical defense snapshot."));
	}

	Fdemo_mapShanmenArmorResistanceProjectionResult Result;
	Result.Status =
		Edemo_mapShanmenArmorResistanceProjectionStatus::Projected;
	Result.Defense = MoveTemp(Candidate);
	Result.ProjectedLayerIds = MoveTemp(LayerIds);
	Result.Diagnostic = TEXT("Armor resistance layers projected atomically.");
	return Result;
}
