#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include <limits>

namespace
{
	float FinalValue(const Udemo_mapAttributeComponent* Component, FName Id)
	{
		float Value = 0.0f;
		Component->GetFinalValue(Id, Value);
		return Value;
	}

	Fdemo_mapModifierSpec Modifier(FName Source, FName Attribute, Edemo_mapModifierOperation Operation, float Value, int32 Priority = 0)
	{
		Fdemo_mapModifierSpec Spec;
		Spec.SourceId = Source;
		Spec.AttributeId = Attribute;
		Spec.Operation = Operation;
		Spec.Value = Value;
		Spec.Priority = Priority;
		return Spec;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapAttributeDefinitionsTest, "demo_map.V3.Attributes.DefinitionsAndDefaults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapAttributeDefinitionsTest::RunTest(const FString&)
{
	const TArray<Fdemo_mapAttributeDefinition>& Definitions = Fdemo_mapAttributeDefinitions::GetAll();
	TestEqual(TEXT("Nine definitions"), Definitions.Num(), 9);
	TestTrue(TEXT("Registry valid"), Fdemo_mapAttributeDefinitions::ValidateDefinitions(Definitions));
	TArray<Fdemo_mapAttributeDefinition> Duplicate = Definitions;
	Duplicate.Add(Definitions[0]);
	TestFalse(TEXT("Duplicate detected"), Fdemo_mapAttributeDefinitions::ValidateDefinitions(Duplicate));

	Udemo_mapAttributeComponent* Attributes = NewObject<Udemo_mapAttributeComponent>();
	TestTrue(TEXT("Primary01 registered"), Attributes->IsAttributeRegistered(Fdemo_mapAttributeIds::Primary01));
	TestFalse(TEXT("Unknown not registered"), Attributes->IsAttributeRegistered(TEXT("Unknown.Attribute")));
	float Unknown = 123.0f;
	TestFalse(TEXT("Unknown query fails safely"), Attributes->GetFinalValue(TEXT("Unknown.Attribute"), Unknown));
	TestTrue(TEXT("Primary01 default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::Primary01), 0.0f));
	TestTrue(TEXT("Primary02 default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::Primary02), 0.0f));
	TestTrue(TEXT("Primary03 default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::Primary03), 0.0f));
	TestTrue(TEXT("MaxHealth default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::MaxHealth), 5.0f));
	TestTrue(TEXT("MoveSpeed V2 default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::MoveSpeed), 600.0f));
	TestTrue(TEXT("AttackPower default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower), 1.0f));
	TestTrue(TEXT("DodgeChance default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::DodgeChance), 0.0f));
	TestTrue(TEXT("FlatDamageReduction default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::FlatDamageReduction), 0.0f));
	TestTrue(TEXT("CooldownMultiplier default"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::CooldownMultiplier), 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapAttributeModifierTest, "demo_map.V3.Attributes.ModifierSemantics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapAttributeModifierTest::RunTest(const FString&)
{
	Udemo_mapAttributeComponent* AddThenMultiply = NewObject<Udemo_mapAttributeComponent>();
	TestTrue(TEXT("Set base 10"), AddThenMultiply->SetBaseValue(Fdemo_mapAttributeIds::Primary01, 10.0f));
	Fdemo_mapModifierHandle AddHandle, MultiplyHandle;
	TestTrue(TEXT("Add modifier"), AddThenMultiply->AddModifier(Modifier(TEXT("Priority.A"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, 2.0f, 0), AddHandle));
	TestTrue(TEXT("Multiply modifier"), AddThenMultiply->AddModifier(Modifier(TEXT("Priority.B"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Multiply, 2.0f, 10), MultiplyHandle));
	TestTrue(TEXT("Add then multiply = 24"), FMath::IsNearlyEqual(FinalValue(AddThenMultiply, Fdemo_mapAttributeIds::Primary01), 24.0f));
	TestTrue(TEXT("Unique handles"), AddHandle.IsValid() && MultiplyHandle.IsValid() && !(AddHandle == MultiplyHandle));
	TestTrue(TEXT("Remove handle"), AddThenMultiply->RemoveModifier(AddHandle));
	TestFalse(TEXT("Repeated remove fails"), AddThenMultiply->RemoveModifier(AddHandle));

	Udemo_mapAttributeComponent* MultiplyThenAdd = NewObject<Udemo_mapAttributeComponent>();
	MultiplyThenAdd->SetBaseValue(Fdemo_mapAttributeIds::Primary01, 10.0f);
	Fdemo_mapModifierHandle H1, H2;
	MultiplyThenAdd->AddModifier(Modifier(TEXT("Priority.C"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Multiply, 2.0f, 0), H1);
	MultiplyThenAdd->AddModifier(Modifier(TEXT("Priority.D"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, 2.0f, 10), H2);
	TestTrue(TEXT("Multiply then add = 22"), FMath::IsNearlyEqual(FinalValue(MultiplyThenAdd, Fdemo_mapAttributeIds::Primary01), 22.0f));

	for (float Factor : { 1.10f, 0.50f, 1.00f })
	{
		Udemo_mapAttributeComponent* FactorAttributes = NewObject<Udemo_mapAttributeComponent>();
		FactorAttributes->SetBaseValue(Fdemo_mapAttributeIds::Primary01, 10.0f);
		Fdemo_mapModifierHandle FactorHandle;
		FactorAttributes->AddModifier(Modifier(TEXT("Factor"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Multiply, Factor), FactorHandle);
		TestTrue(FString::Printf(TEXT("Factor %.2f semantics"), Factor), FMath::IsNearlyEqual(FinalValue(FactorAttributes, Fdemo_mapAttributeIds::Primary01), 10.0f * Factor));
	}

	Udemo_mapAttributeComponent* Sources = NewObject<Udemo_mapAttributeComponent>();
	Fdemo_mapModifierHandle SourceA1, SourceA2, SourceB;
	Sources->AddModifier(Modifier(TEXT("Source.A"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, 2.0f), SourceA1);
	Sources->AddModifier(Modifier(TEXT("Source.A"), Fdemo_mapAttributeIds::Primary02, Edemo_mapModifierOperation::Add, 3.0f), SourceA2);
	Sources->AddModifier(Modifier(TEXT("Source.B"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, 1.0f), SourceB);
	TestTrue(TEXT("Multiple Add modifiers accumulate"), FMath::IsNearlyEqual(FinalValue(Sources, Fdemo_mapAttributeIds::Primary01), 3.0f));
	TestEqual(TEXT("Remove two by source"), Sources->RemoveModifiersBySource(TEXT("Source.A")), 2);
	TestTrue(TEXT("Other source preserved"), FMath::IsNearlyEqual(FinalValue(Sources, Fdemo_mapAttributeIds::Primary01), 1.0f));
	TestEqual(TEXT("One modifier remains"), Sources->GetActiveModifierCount(), 1);

	const int32 BeforeInvalid = Sources->GetActiveModifierCount();
	Fdemo_mapModifierHandle InvalidHandle;
	TestFalse(TEXT("Unknown ID rejected"), Sources->AddModifier(Modifier(TEXT("Invalid"), TEXT("Unknown.Attribute"), Edemo_mapModifierOperation::Add, 1.0f), InvalidHandle));
	TestFalse(TEXT("NaN rejected"), Sources->AddModifier(Modifier(TEXT("Invalid"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, std::numeric_limits<float>::quiet_NaN()), InvalidHandle));
	TestFalse(TEXT("Infinity rejected"), Sources->AddModifier(Modifier(TEXT("Invalid"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, std::numeric_limits<float>::infinity()), InvalidHandle));
	TestFalse(TEXT("Negative multiply rejected"), Sources->AddModifier(Modifier(TEXT("Invalid"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Multiply, -0.1f), InvalidHandle));
	TestEqual(TEXT("Invalid input does not pollute"), Sources->GetActiveModifierCount(), BeforeInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(Fdemo_mapAttributeDerivedTest, "demo_map.V3.Attributes.DerivedHealthDodgeAndResidue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool Fdemo_mapAttributeDerivedTest::RunTest(const FString&)
{
	Udemo_mapAttributeComponent* Attributes = NewObject<Udemo_mapAttributeComponent>();
	Fdemo_mapModifierHandle P01, P02, P03;
	Attributes->AddModifier(Modifier(TEXT("Primaries"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, 2.0f), P01);
	Attributes->AddModifier(Modifier(TEXT("Primaries"), Fdemo_mapAttributeIds::Primary02, Edemo_mapModifierOperation::Add, 3.0f), P02);
	Attributes->AddModifier(Modifier(TEXT("Primaries"), Fdemo_mapAttributeIds::Primary03, Edemo_mapModifierOperation::Add, 2.0f), P03);
	TestTrue(TEXT("P01 -> AttackPower 3"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower), 3.0f));
	TestTrue(TEXT("P02 -> MoveSpeed 630"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::MoveSpeed), 630.0f));
	TestTrue(TEXT("P02 -> DodgeChance 0.15"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::DodgeChance), 0.15f));
	TestTrue(TEXT("P03 -> MaxHealth 7"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::MaxHealth), 7.0f));

	Fdemo_mapModifierHandle MaxAdd, MoveAdd, AttackMultiply, DodgeClamp;
	Attributes->AddModifier(Modifier(TEXT("Derived"), Fdemo_mapAttributeIds::MaxHealth, Edemo_mapModifierOperation::Add, 2.0f), MaxAdd);
	Attributes->AddModifier(Modifier(TEXT("Derived"), Fdemo_mapAttributeIds::MoveSpeed, Edemo_mapModifierOperation::Add, 20.0f), MoveAdd);
	Attributes->AddModifier(Modifier(TEXT("Derived"), Fdemo_mapAttributeIds::AttackPower, Edemo_mapModifierOperation::Multiply, 2.0f), AttackMultiply);
	Attributes->AddModifier(Modifier(TEXT("Derived"), Fdemo_mapAttributeIds::DodgeChance, Edemo_mapModifierOperation::Add, 2.0f), DodgeClamp);
	TestTrue(TEXT("Direct MaxHealth Add"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::MaxHealth), 9.0f));
	TestTrue(TEXT("Direct MoveSpeed Add"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::MoveSpeed), 650.0f));
	TestTrue(TEXT("Direct AttackPower Multiply"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower), 6.0f));
	TestTrue(TEXT("DodgeChance clamped"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::DodgeChance), 1.0f));
	TestEqual(TEXT("Remove derived source"), Attributes->RemoveModifiersBySource(TEXT("Derived")), 4);

	TestFalse(TEXT("Chance 0"), Udemo_mapPlayerHealthComponent::ShouldDodge(0.0f, 0.2f));
	TestTrue(TEXT("Chance .25 roll .249"), Udemo_mapPlayerHealthComponent::ShouldDodge(0.25f, 0.249f));
	TestFalse(TEXT("Chance .25 roll .25"), Udemo_mapPlayerHealthComponent::ShouldDodge(0.25f, 0.25f));
	TestTrue(TEXT("Chance 1"), Udemo_mapPlayerHealthComponent::ShouldDodge(1.0f, 0.999f));

	Attributes->RemoveModifiersBySource(TEXT("Primaries"));
	for (int32 Round = 0; Round < 3; ++Round)
	{
		Fdemo_mapModifierHandle Residue;
		TestTrue(TEXT("Residue add"), Attributes->AddModifier(Modifier(TEXT("Residue"), Fdemo_mapAttributeIds::Primary01, Edemo_mapModifierOperation::Add, 2.0f), Residue));
		TestTrue(TEXT("Residue value"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower), 3.0f));
		TestTrue(TEXT("Residue remove"), Attributes->RemoveModifier(Residue));
		TestTrue(TEXT("Residue restore"), FMath::IsNearlyEqual(FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower), 1.0f));
	}
	TestEqual(TEXT("No active modifiers"), Attributes->GetActiveModifierCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapAttributeExactHandleConvergenceTest,
	"demo_map.V3.Attributes.ExactHandleConvergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapAttributeExactHandleConvergenceTest::RunTest(const FString&)
{
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	Fdemo_mapModifierHandle Handle;
	Handle.Value = FGuid(0xA7700001, 0, 0, 1);
	const auto Spec = Modifier(
		TEXT("Exact.Handle"), Fdemo_mapAttributeIds::AttackPower,
		Edemo_mapModifierOperation::Add, 2.5f, 25);
	const auto Applied = Attributes->EnsureModifierApplied(Spec, Handle);
	const auto ApplyReplay = Attributes->EnsureModifierApplied(Spec, Handle);
	const auto Conflict = Attributes->EnsureModifierApplied(
		Modifier(
			TEXT("Exact.Handle"), Fdemo_mapAttributeIds::AttackPower,
			Edemo_mapModifierOperation::Add, 3.5f, 25),
		Handle);

	TestTrue(TEXT("Exact handle applies once and replays without duplication"),
		Applied == Edemo_mapExactModifierMutationStatus::Applied
			&& ApplyReplay
				== Edemo_mapExactModifierMutationStatus::ApplyReplayed
			&& Attributes->GetActiveModifierCount() == 1
			&& FMath::IsNearlyEqual(
				FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower),
				3.5f));
	TestTrue(TEXT("Same handle with different spec fails closed"),
		Conflict == Edemo_mapExactModifierMutationStatus::HandleConflict
			&& Attributes->GetActiveModifierCount() == 1);

	const auto Removed = Attributes->EnsureModifierRemoved(Spec, Handle);
	const auto RemoveReplay = Attributes->EnsureModifierRemoved(Spec, Handle);
	TestTrue(TEXT("Exact removal converges and missing replay is a no-op"),
		Removed == Edemo_mapExactModifierMutationStatus::Removed
			&& RemoveReplay
				== Edemo_mapExactModifierMutationStatus::RemoveReplayed
			&& Attributes->GetActiveModifierCount() == 0
			&& FMath::IsNearlyEqual(
				FinalValue(Attributes, Fdemo_mapAttributeIds::AttackPower),
				1.0f));
	TestTrue(TEXT("Invalid exact inputs do not mutate state"),
		Attributes->EnsureModifierApplied(
			Spec, Fdemo_mapModifierHandle())
			== Edemo_mapExactModifierMutationStatus::InvalidHandle
			&& Attributes->EnsureModifierRemoved(
				Spec, Fdemo_mapModifierHandle())
				== Edemo_mapExactModifierMutationStatus::InvalidHandle
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

#endif
