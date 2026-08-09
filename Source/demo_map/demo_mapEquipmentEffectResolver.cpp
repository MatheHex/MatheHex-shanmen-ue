#include "demo_mapEquipmentEffectResolver.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardAffix.h"

namespace
{
	bool IsFiniteNonNegative(double Value)
	{
		return FMath::IsFinite(Value) && Value >= 0.0
			&& Value <= static_cast<double>(MAX_flt);
	}

	Fdemo_mapModifierSpec MakeEffectModifier(
		FName AttributeId,
		Edemo_mapModifierOperation Operation,
		double Value)
	{
		Fdemo_mapModifierSpec Modifier;
		Modifier.AttributeId = AttributeId;
		Modifier.Operation = Operation;
		Modifier.Value = static_cast<float>(Value);
		return Modifier;
	}
}

Fdemo_mapEquipmentEffectResolution
Fdemo_mapEquipmentEffectResolver::Resolve(
	const Fdemo_mapItemDefinition& Definition,
	FName EquippedSlotId)
{
	Fdemo_mapEquipmentEffectResolution Result;
	if (EquippedSlotId.IsNone()
		|| Definition.EquipmentSlotId != EquippedSlotId
		|| !Definition.CompatibleSlotIds.Contains(EquippedSlotId))
	{
		Result.Diagnostic = TEXT("Equipment Effect resolution rejected incompatible Category or Slot metadata.");
		return Result;
	}

	TSet<FName> TargetAttributes;
	for (const Fdemo_mapModifierSpec& Legacy : Definition.Modifiers)
	{
		if (Legacy.AttributeId.IsNone()
			|| !Fdemo_mapAttributeDefinitions::Find(Legacy.AttributeId)
			|| !FMath::IsFinite(Legacy.Value)
			|| (Legacy.Operation == Edemo_mapModifierOperation::Multiply && Legacy.Value < 0.0f)
			|| TargetAttributes.Contains(Legacy.AttributeId))
		{
			Result.Diagnostic = TEXT("Legacy equipment Modifier metadata is invalid or duplicated.");
			return Result;
		}
		Fdemo_mapModifierSpec Runtime = Legacy;
		Runtime.SourceId = NAME_None;
		Result.Modifiers.Add(Runtime);
		TargetAttributes.Add(Runtime.AttributeId);
	}

	for (const Fdemo_mapItemEffectParameter& Effect : Definition.EffectParameters)
	{
		FName TargetAttribute = NAME_None;
		Edemo_mapModifierOperation Operation = Edemo_mapModifierOperation::Add;
		bool bAcceptedMetadataOnly = false;
		if (Effect.ParameterId == Fdemo_mapItemEffectIds::AttackBonus)
		{
			if (Definition.CategoryId != Fdemo_mapItemIds::WeaponCategory
				|| EquippedSlotId != Fdemo_mapItemIds::WeaponSlot
				|| !IsFiniteNonNegative(Effect.Value))
			{
				Result.Diagnostic = TEXT("AttackBonus requires finite non-negative Weapon/WeaponSlot metadata.");
				return Result;
			}
			TargetAttribute = Fdemo_mapAttributeIds::AttackPower;
		}
		else if (Effect.ParameterId == Fdemo_mapItemEffectIds::MaxHealthBonus)
		{
			if (Definition.CategoryId != Fdemo_mapItemIds::ArmorCategory
				|| EquippedSlotId != Fdemo_mapItemIds::ArmorSlot
				|| !IsFiniteNonNegative(Effect.Value))
			{
				Result.Diagnostic = TEXT("MaxHealthBonus requires finite non-negative Armor/ArmorSlot metadata.");
				return Result;
			}
			TargetAttribute = Fdemo_mapAttributeIds::MaxHealth;
		}
		else if (Effect.ParameterId == Fdemo_mapItemEffectIds::FlatDamageReduction)
		{
			if (Definition.CategoryId != Fdemo_mapItemIds::ArmorCategory
				|| EquippedSlotId != Fdemo_mapItemIds::ArmorSlot
				|| !IsFiniteNonNegative(Effect.Value))
			{
				Result.Diagnostic = TEXT("FlatDamageReduction requires finite non-negative Armor/ArmorSlot metadata.");
				return Result;
			}
			TargetAttribute = Fdemo_mapAttributeIds::FlatDamageReduction;
		}
		else if (Effect.ParameterId == Fdemo_mapItemEffectIds::CooldownMultiplier)
		{
			if (Definition.CategoryId != Fdemo_mapItemIds::SpatialRingCategory
				|| EquippedSlotId != Fdemo_mapItemIds::SpatialRingSlot
				|| !FMath::IsFinite(Effect.Value)
				|| Effect.Value <= 0.0
				|| Effect.Value > static_cast<double>(MAX_flt))
			{
				Result.Diagnostic = TEXT("CooldownMultiplier requires a finite positive SpatialRing/SpatialRingSlot value.");
				return Result;
			}
			TargetAttribute = Fdemo_mapAttributeIds::CooldownMultiplier;
			Operation = Edemo_mapModifierOperation::Multiply;
		}
		else if (Effect.ParameterId == Fdemo_mapItemEffectIds::TotalCapacity)
		{
			if (Definition.CategoryId != Fdemo_mapItemIds::BackpackCategory
				|| EquippedSlotId != Fdemo_mapItemIds::BackpackSlot)
			{
				Result.Diagnostic = TEXT("TotalCapacity is only valid for the Backpack slot.");
				return Result;
			}
			bAcceptedMetadataOnly = true;
		}
		else if (Effect.ParameterId == Fdemo_mapItemEffectIds::RingQuickCapacity)
		{
			if (Definition.CategoryId != Fdemo_mapItemIds::SpatialRingCategory
				|| EquippedSlotId != Fdemo_mapItemIds::SpatialRingSlot
				|| !IsFiniteNonNegative(Effect.Value))
			{
				Result.Diagnostic = TEXT("RingQuickCapacity is only valid for a finite SpatialRing/SpatialRingSlot value.");
				return Result;
			}
			// The ring capacity changes the inventory projection; it is not an
			// attribute modifier and must not be sent to AttributeComponent.
			bAcceptedMetadataOnly = true;
		}
		else
		{
			Result.Diagnostic = TEXT("Unknown or non-equipment Effect Key cannot become a runtime equipment Modifier.");
			return Result;
		}

		if (bAcceptedMetadataOnly)
		{
			continue;
		}
		if (TargetAttributes.Contains(TargetAttribute))
		{
			Result.Diagnostic = TEXT("Equipment metadata resolves more than once to the same final Attribute.");
			return Result;
		}
		Result.Modifiers.Add(MakeEffectModifier(TargetAttribute, Operation, Effect.Value));
		TargetAttributes.Add(TargetAttribute);
	}

	Result.bSuccess = true;
	return Result;
}

Fdemo_mapEquipmentEffectResolution
Fdemo_mapEquipmentEffectResolver::ResolveAffixes(
	const Fdemo_mapItemDefinition& Definition,
	const Fdemo_mapRewardAffixSet& AffixSet)
{
	Fdemo_mapEquipmentEffectResolution Result;
	if (!Fdemo_mapRewardAffixPolicyRegistry::ValidateSet(
		Definition.DefinitionId,
		1,
		AffixSet,
		&Result.Diagnostic))
	{
		return Result;
	}
	for (const Fdemo_mapResolvedRewardAffix& Affix : AffixSet.Affixes)
	{
		const Fdemo_mapRewardAffixDescriptor* Descriptor =
			Fdemo_mapRewardAffixPolicyRegistry::Find(Affix.AffixId);
		if (!Descriptor)
		{
			Result.Diagnostic = TEXT("Affix Effect descriptor is missing.");
			return Result;
		}
		switch (Descriptor->Effect)
		{
		case Edemo_mapRewardAffixEffect::AttackPower:
			Result.Modifiers.Add(MakeEffectModifier(
				Fdemo_mapAttributeIds::AttackPower,
				Edemo_mapModifierOperation::Add,
				Descriptor->MagnitudeScaled));
			break;
		case Edemo_mapRewardAffixEffect::MaxHealth:
			Result.Modifiers.Add(MakeEffectModifier(
				Fdemo_mapAttributeIds::MaxHealth,
				Edemo_mapModifierOperation::Add,
				Descriptor->MagnitudeScaled));
			break;
		case Edemo_mapRewardAffixEffect::FlatDamageReduction:
			Result.Modifiers.Add(MakeEffectModifier(
				Fdemo_mapAttributeIds::FlatDamageReduction,
				Edemo_mapModifierOperation::Add,
				Descriptor->MagnitudeScaled));
			break;
		case Edemo_mapRewardAffixEffect::CooldownMultiplierDeltaBps:
			Result.Modifiers.Add(MakeEffectModifier(
				Fdemo_mapAttributeIds::CooldownMultiplier,
				Edemo_mapModifierOperation::Multiply,
				1.0 + static_cast<double>(
					Descriptor->MagnitudeScaled) / 10000.0));
			break;
		default:
			Result.Diagnostic = TEXT("Unknown Affix Effect.");
			return Result;
		}
	}
	Result.bSuccess = true;
	Result.Diagnostic = TEXT("success");
	return Result;
}
