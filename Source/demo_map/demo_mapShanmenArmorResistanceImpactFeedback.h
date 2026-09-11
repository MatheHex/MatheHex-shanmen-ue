#pragma once

#include "CoreMinimal.h"

struct Fdemo_mapM01EnemyAttackExecutionResult;

enum class Edemo_mapShanmenArmorResistanceImpactFeedbackKind : uint8
{
	Invalid,
	Reduced
};

/**
 * Immutable player-facing projection of one newly committed armor-resistance
 * result. The projection reads the exact item/layer/Impact receipts and never
 * recalculates mitigation or owns combat state.
 */
class Fdemo_mapShanmenArmorResistanceImpactFeedbackPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapM01EnemyAttackExecutionResult& AttackResult,
		Fdemo_mapShanmenArmorResistanceImpactFeedbackPresentation&
			OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenArmorResistanceImpactFeedbackPresentation&
			Other) const;
	Edemo_mapShanmenArmorResistanceImpactFeedbackKind GetKind() const
	{
		return Kind;
	}
	const FGuid& GetImpactId() const { return ImpactId; }
	const FGuid& GetArmorItemInstanceId() const
	{
		return ArmorItemInstanceId;
	}
	FName GetArmorDefinitionId() const { return ArmorDefinitionId; }
	const FString& GetArmorDisplayName() const { return ArmorDisplayName; }
	float GetRawDamage() const { return RawDamage; }
	float GetArmorPreventedDamage() const { return ArmorPreventedDamage; }
	float GetFinalDamage() const { return FinalDamage; }
	float GetAppliedDamage() const { return AppliedDamage; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenArmorResistanceImpactFeedbackKind Kind =
		Edemo_mapShanmenArmorResistanceImpactFeedbackKind::Invalid;
	FGuid ImpactId;
	FGuid ArmorItemInstanceId;
	FName ArmorDefinitionId = NAME_None;
	FString ArmorDisplayName;
	float RawDamage = -1.0f;
	float ArmorPreventedDamage = -1.0f;
	float FinalDamage = -1.0f;
	float AppliedDamage = -1.0f;
	FString DisplayText;
};
