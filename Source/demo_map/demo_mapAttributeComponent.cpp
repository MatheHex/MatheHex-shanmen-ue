#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_map.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

Udemo_mapAttributeComponent::Udemo_mapAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitializeDefinitions();
	Recalculate();
}

void Udemo_mapAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	ForceRecalculate();
}

void Udemo_mapAttributeComponent::InitializeDefinitions()
{
	if (bDefinitionsInitialized) return;
	FString ValidationError;
	const TArray<Fdemo_mapAttributeDefinition>& Definitions = Fdemo_mapAttributeDefinitions::GetAll();
	if (!Fdemo_mapAttributeDefinitions::ValidateDefinitions(Definitions, &ValidationError))
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.3.1.0: attribute registry validation failed: %s"), *ValidationError);
		return;
	}
	for (const Fdemo_mapAttributeDefinition& Definition : Definitions)
	{
		BaseValues.Add(Definition.AttributeId, Definition.DefaultBaseValue);
	}
	bDefinitionsInitialized = true;
}

bool Udemo_mapAttributeComponent::IsAttributeRegistered(FName AttributeId) const
{
	return Fdemo_mapAttributeDefinitions::Find(AttributeId) != nullptr;
}

bool Udemo_mapAttributeComponent::GetBaseValue(FName AttributeId, float& OutValue) const
{
	if (const float* Value = BaseValues.Find(AttributeId))
	{
		OutValue = *Value;
		return true;
	}
	return false;
}

bool Udemo_mapAttributeComponent::GetFinalValue(FName AttributeId, float& OutValue) const
{
	return FinalSnapshot.TryGetValue(AttributeId, OutValue);
}

bool Udemo_mapAttributeComponent::SetBaseValue(FName AttributeId, float NewValue)
{
	const Fdemo_mapAttributeDefinition* Definition = Fdemo_mapAttributeDefinitions::Find(AttributeId);
	if (Definition == nullptr || Definition->bDerived || !FMath::IsFinite(NewValue)) return false;
	BaseValues.FindOrAdd(AttributeId) = NewValue;
	Recalculate();
	return true;
}

bool Udemo_mapAttributeComponent::AddModifier(const Fdemo_mapModifierSpec& Spec, Fdemo_mapModifierHandle& OutHandle)
{
	OutHandle.Reset();
	if (!IsModifierSpecValid(Spec))
	{
		return false;
	}

	Fdemo_mapModifierHandle Handle;
	do
	{
		Handle.Value = FGuid::NewGuid();
	}
	while (ActiveModifiers.ContainsByPredicate(
		[Handle](const Fdemo_mapActiveModifier& Existing)
		{
			return Existing.Handle == Handle;
		}));
	if (EnsureModifierApplied(Spec, Handle)
		!= Edemo_mapExactModifierMutationStatus::Applied)
	{
		return false;
	}
	OutHandle = Handle;
	return true;
}

bool Udemo_mapAttributeComponent::RemoveModifier(Fdemo_mapModifierHandle Handle)
{
	const int32 Index = ActiveModifiers.IndexOfByPredicate(
		[Handle](const Fdemo_mapActiveModifier& Active)
		{
			return Active.Handle == Handle;
		});
	if (Index == INDEX_NONE)
	{
		return false;
	}
	ActiveModifiers.RemoveAt(Index);
	Recalculate();
	return true;
}

Edemo_mapExactModifierMutationStatus
Udemo_mapAttributeComponent::EnsureModifierApplied(
	const Fdemo_mapModifierSpec& Spec,
	const Fdemo_mapModifierHandle Handle)
{
	if (!Handle.IsValid())
	{
		return Edemo_mapExactModifierMutationStatus::InvalidHandle;
	}
	if (!IsModifierSpecValid(Spec))
	{
		return Edemo_mapExactModifierMutationStatus::InvalidSpec;
	}
	if (const Fdemo_mapActiveModifier* Existing =
		ActiveModifiers.FindByPredicate(
			[Handle](const Fdemo_mapActiveModifier& Active)
			{
				return Active.Handle == Handle;
			}))
	{
		return Existing->Spec.Matches(Spec)
			? Edemo_mapExactModifierMutationStatus::ApplyReplayed
			: Edemo_mapExactModifierMutationStatus::HandleConflict;
	}

	Fdemo_mapActiveModifier Active;
	Active.Handle = Handle;
	Active.Spec = Spec;
	Active.InsertionOrder = NextInsertionOrder++;
	ActiveModifiers.Add(MoveTemp(Active));
	Recalculate();
	return Edemo_mapExactModifierMutationStatus::Applied;
}

Edemo_mapExactModifierMutationStatus
Udemo_mapAttributeComponent::EnsureModifierRemoved(
	const Fdemo_mapModifierSpec& Spec,
	const Fdemo_mapModifierHandle Handle)
{
	if (!Handle.IsValid())
	{
		return Edemo_mapExactModifierMutationStatus::InvalidHandle;
	}
	if (!IsModifierSpecValid(Spec))
	{
		return Edemo_mapExactModifierMutationStatus::InvalidSpec;
	}
	const int32 Index = ActiveModifiers.IndexOfByPredicate(
		[Handle](const Fdemo_mapActiveModifier& Active)
		{
			return Active.Handle == Handle;
		});
	if (Index == INDEX_NONE)
	{
		return Edemo_mapExactModifierMutationStatus::RemoveReplayed;
	}
	if (!ActiveModifiers[Index].Spec.Matches(Spec))
	{
		return Edemo_mapExactModifierMutationStatus::HandleConflict;
	}
	ActiveModifiers.RemoveAt(Index);
	Recalculate();
	return Edemo_mapExactModifierMutationStatus::Removed;
}

int32 Udemo_mapAttributeComponent::RemoveModifiersBySource(FName SourceId)
{
	if (SourceId.IsNone()) return 0;
	const int32 Removed = ActiveModifiers.RemoveAll([SourceId](const Fdemo_mapActiveModifier& Active) { return Active.Spec.SourceId == SourceId; });
	if (Removed > 0) Recalculate();
	return Removed;
}

int32 Udemo_mapAttributeComponent::GetModifierCountBySource(FName SourceId) const
{
	if (SourceId.IsNone()) return 0;
	int32 Count = 0;
	for (const Fdemo_mapActiveModifier& Active : ActiveModifiers) if (Active.Spec.SourceId == SourceId) ++Count;
	return Count;
}

void Udemo_mapAttributeComponent::ForceRecalculate()
{
	InitializeDefinitions();
	Recalculate();
}

bool Udemo_mapAttributeComponent::IsModifierSpecValid(
	const Fdemo_mapModifierSpec& Spec) const
{
	const bool bKnownOperation =
		Spec.Operation == Edemo_mapModifierOperation::Add
		|| Spec.Operation == Edemo_mapModifierOperation::Multiply;
	return IsAttributeRegistered(Spec.AttributeId) && !Spec.SourceId.IsNone()
		&& bKnownOperation && FMath::IsFinite(Spec.Value)
		&& (Spec.Operation != Edemo_mapModifierOperation::Multiply
			|| Spec.Value >= 0.0f);
}

float Udemo_mapAttributeComponent::ApplyModifiers(FName AttributeId, float StartingValue) const
{
	TArray<const Fdemo_mapActiveModifier*> Sorted;
	for (const Fdemo_mapActiveModifier& Active : ActiveModifiers)
	{
		if (Active.Spec.AttributeId == AttributeId) Sorted.Add(&Active);
	}
	Sorted.Sort([](const Fdemo_mapActiveModifier& A, const Fdemo_mapActiveModifier& B)
	{
		return A.Spec.Priority == B.Spec.Priority ? A.InsertionOrder < B.InsertionOrder : A.Spec.Priority < B.Spec.Priority;
	});
	float Value = StartingValue;
	for (const Fdemo_mapActiveModifier* Active : Sorted)
	{
		Value = Active->Spec.Operation == Edemo_mapModifierOperation::Add ? Value + Active->Spec.Value : Value * Active->Spec.Value;
	}
	return Value;
}

void Udemo_mapAttributeComponent::Recalculate()
{
	if (!bDefinitionsInitialized || bRecalculating) return;
	TGuardValue<bool> Guard(bRecalculating, true);
	const TMap<FName, float> OldValues = FinalSnapshot.Values;
	TMap<FName, float> NewValues;

	for (const Fdemo_mapAttributeDefinition& Definition : Fdemo_mapAttributeDefinitions::GetAll())
	{
		if (Definition.Kind != Edemo_mapAttributeKind::Primary) continue;
		const float Raw = ApplyModifiers(Definition.AttributeId, BaseValues.FindRef(Definition.AttributeId));
		NewValues.Add(Definition.AttributeId, FMath::Clamp(Raw, Definition.Minimum, Definition.Maximum));
	}
	for (const Fdemo_mapAttributeDefinition& Definition : Fdemo_mapAttributeDefinitions::GetAll())
	{
		if (Definition.Kind != Edemo_mapAttributeKind::Derived) continue;
		const float Raw = Fdemo_mapAttributeDefinitions::ComputePrototypeDerivedRaw(Definition.AttributeId, NewValues);
		const float Modified = ApplyModifiers(Definition.AttributeId, Raw);
		NewValues.Add(Definition.AttributeId, FMath::Clamp(Modified, Definition.Minimum, Definition.Maximum));
	}

	FinalSnapshot.Values = NewValues;
	ApplyMoveSpeed(NewValues.FindRef(Fdemo_mapAttributeIds::MoveSpeed));
	for (const TPair<FName, float>& Pair : NewValues)
	{
		const float OldValue = OldValues.Contains(Pair.Key) ? OldValues.FindRef(Pair.Key) : Pair.Value;
		if (!FMath::IsNearlyEqual(OldValue, Pair.Value))
		{
			OnAttributeChanged.Broadcast({ Pair.Key, OldValue, Pair.Value });
		}
	}
}

void Udemo_mapAttributeComponent::ApplyMoveSpeed(float NewMoveSpeed) const
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			if (!FMath::IsNearlyEqual(Movement->MaxWalkSpeed, NewMoveSpeed)) Movement->MaxWalkSpeed = NewMoveSpeed;
		}
	}
}
