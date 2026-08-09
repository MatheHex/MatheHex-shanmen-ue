#include "demo_mapAttributeDefinitions.h"

const FName Fdemo_mapAttributeIds::Primary01(TEXT("Prototype.Primary.01"));
const FName Fdemo_mapAttributeIds::Primary02(TEXT("Prototype.Primary.02"));
const FName Fdemo_mapAttributeIds::Primary03(TEXT("Prototype.Primary.03"));
const FName Fdemo_mapAttributeIds::MaxHealth(TEXT("Derived.MaxHealth"));
const FName Fdemo_mapAttributeIds::MoveSpeed(TEXT("Derived.MoveSpeed"));
const FName Fdemo_mapAttributeIds::AttackPower(TEXT("Derived.AttackPower"));
const FName Fdemo_mapAttributeIds::DodgeChance(TEXT("Derived.DodgeChance"));
const FName Fdemo_mapAttributeIds::FlatDamageReduction(TEXT("Derived.FlatDamageReduction"));
const FName Fdemo_mapAttributeIds::CooldownMultiplier(TEXT("Derived.CooldownMultiplier"));

namespace
{
	Fdemo_mapAttributeDefinition MakeDefinition(FName Id, const TCHAR* DisplayName, Edemo_mapAttributeKind Kind, float DefaultValue, float Minimum, float Maximum, const TCHAR* Format)
	{
		Fdemo_mapAttributeDefinition Definition;
		Definition.AttributeId = Id;
		Definition.DisplayName = FText::FromString(DisplayName);
		Definition.Kind = Kind;
		Definition.DefaultBaseValue = DefaultValue;
		Definition.Minimum = Minimum;
		Definition.Maximum = Maximum;
		Definition.bDerived = Kind == Edemo_mapAttributeKind::Derived;
		Definition.DisplayFormat = Format;
		return Definition;
	}
}

const TArray<Fdemo_mapAttributeDefinition>& Fdemo_mapAttributeDefinitions::GetAll()
{
	static const TArray<Fdemo_mapAttributeDefinition> Definitions = {
		MakeDefinition(Fdemo_mapAttributeIds::Primary01, TEXT("力量"), Edemo_mapAttributeKind::Primary, 0.0f, -100000.0f, 100000.0f, TEXT("%.0f")),
		MakeDefinition(Fdemo_mapAttributeIds::Primary02, TEXT("敏捷"), Edemo_mapAttributeKind::Primary, 0.0f, -100000.0f, 100000.0f, TEXT("%.0f")),
		MakeDefinition(Fdemo_mapAttributeIds::Primary03, TEXT("体魄"), Edemo_mapAttributeKind::Primary, 0.0f, -100000.0f, 100000.0f, TEXT("%.0f")),
		MakeDefinition(Fdemo_mapAttributeIds::MaxHealth, TEXT("最大生命"), Edemo_mapAttributeKind::Derived, 5.0f, 1.0f, 1000000.0f, TEXT("%.0f")),
		MakeDefinition(Fdemo_mapAttributeIds::MoveSpeed, TEXT("移动速度"), Edemo_mapAttributeKind::Derived, V2DefaultMoveSpeed, 0.0f, 1000000.0f, TEXT("%.0f")),
		MakeDefinition(Fdemo_mapAttributeIds::AttackPower, TEXT("攻击能力"), Edemo_mapAttributeKind::Derived, 1.0f, 0.0f, 1000000.0f, TEXT("%.2f")),
		MakeDefinition(Fdemo_mapAttributeIds::DodgeChance, TEXT("闪避率"), Edemo_mapAttributeKind::Derived, 0.0f, 0.0f, 1.0f, TEXT("%.1f%%")),
		MakeDefinition(Fdemo_mapAttributeIds::FlatDamageReduction, TEXT("固定减伤"), Edemo_mapAttributeKind::Derived, 0.0f, 0.0f, 1000000.0f, TEXT("%.0f")),
		MakeDefinition(Fdemo_mapAttributeIds::CooldownMultiplier, TEXT("冷却倍率"), Edemo_mapAttributeKind::Derived, 1.0f, 0.0f, 1000000.0f, TEXT("%.2f"))
	};
	return Definitions;
}

const Fdemo_mapAttributeDefinition* Fdemo_mapAttributeDefinitions::Find(FName AttributeId)
{
	return GetAll().FindByPredicate([AttributeId](const Fdemo_mapAttributeDefinition& Definition)
	{
		return Definition.AttributeId == AttributeId;
	});
}

bool Fdemo_mapAttributeDefinitions::ValidateDefinitions(const TArray<Fdemo_mapAttributeDefinition>& Definitions, FString* OutError)
{
	TSet<FName> SeenIds;
	for (const Fdemo_mapAttributeDefinition& Definition : Definitions)
	{
		if (Definition.AttributeId.IsNone() || SeenIds.Contains(Definition.AttributeId) || !FMath::IsFinite(Definition.DefaultBaseValue)
			|| !FMath::IsFinite(Definition.Minimum) || !FMath::IsFinite(Definition.Maximum) || Definition.Minimum > Definition.Maximum)
		{
			if (OutError != nullptr) *OutError = FString::Printf(TEXT("Invalid or duplicate attribute definition: %s"), *Definition.AttributeId.ToString());
			return false;
		}
		SeenIds.Add(Definition.AttributeId);
	}
	return SeenIds.Num() == 9;
}

float Fdemo_mapAttributeDefinitions::ComputePrototypeDerivedRaw(FName AttributeId, const TMap<FName, float>& PrimaryFinals)
{
	const float P01 = PrimaryFinals.FindRef(Fdemo_mapAttributeIds::Primary01);
	const float P02 = PrimaryFinals.FindRef(Fdemo_mapAttributeIds::Primary02);
	const float P03 = PrimaryFinals.FindRef(Fdemo_mapAttributeIds::Primary03);

	// Prototype Formula boundary. These formulas are intentionally replaceable.
	if (AttributeId == Fdemo_mapAttributeIds::MaxHealth) return 5.0f + P03;
	if (AttributeId == Fdemo_mapAttributeIds::MoveSpeed) return V2DefaultMoveSpeed + P02 * 10.0f;
	if (AttributeId == Fdemo_mapAttributeIds::AttackPower) return 1.0f + P01;
	if (AttributeId == Fdemo_mapAttributeIds::DodgeChance) return P02 * 0.05f;
	if (AttributeId == Fdemo_mapAttributeIds::FlatDamageReduction) return 0.0f;
	if (AttributeId == Fdemo_mapAttributeIds::CooldownMultiplier) return 1.0f;
	return 0.0f;
}
