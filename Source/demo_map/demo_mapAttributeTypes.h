#pragma once

#include "CoreMinimal.h"
#include "demo_mapAttributeTypes.generated.h"

UENUM()
enum class Edemo_mapAttributeKind : uint8
{
	Primary,
	Derived
};

UENUM()
enum class Edemo_mapModifierOperation : uint8
{
	Add,
	Multiply
};

USTRUCT()
struct Fdemo_mapAttributeDefinition
{
	GENERATED_BODY()

	FName AttributeId = NAME_None;
	FText DisplayName;
	Edemo_mapAttributeKind Kind = Edemo_mapAttributeKind::Primary;
	float DefaultBaseValue = 0.0f;
	float Minimum = -1000000.0f;
	float Maximum = 1000000.0f;
	bool bDerived = false;
	FString DisplayFormat = TEXT("%.2f");
};

USTRUCT()
struct Fdemo_mapModifierHandle
{
	GENERATED_BODY()

	FGuid Value;

	bool IsValid() const { return Value.IsValid(); }
	void Reset() { Value.Invalidate(); }
	bool operator==(const Fdemo_mapModifierHandle& Other) const { return Value == Other.Value; }
};

USTRUCT()
struct Fdemo_mapModifierSpec
{
	GENERATED_BODY()

	FName SourceId = NAME_None;
	FName AttributeId = NAME_None;
	Edemo_mapModifierOperation Operation = Edemo_mapModifierOperation::Add;
	float Value = 0.0f;
	int32 Priority = 0;

	bool Matches(const Fdemo_mapModifierSpec& Other) const
	{
		return SourceId == Other.SourceId
			&& AttributeId == Other.AttributeId
			&& Operation == Other.Operation && Value == Other.Value
			&& Priority == Other.Priority;
	}
};

/** Result of converging one caller-owned handle to its desired native state. */
enum class Edemo_mapExactModifierMutationStatus : uint8
{
	Applied,
	ApplyReplayed,
	Removed,
	RemoveReplayed,
	InvalidHandle,
	InvalidSpec,
	HandleConflict
};

USTRUCT()
struct Fdemo_mapAttributeSnapshot
{
	GENERATED_BODY()

	TMap<FName, float> Values;

	bool TryGetValue(FName AttributeId, float& OutValue) const
	{
		if (const float* Found = Values.Find(AttributeId))
		{
			OutValue = *Found;
			return true;
		}
		return false;
	}
};

struct Fdemo_mapAttributeChange
{
	FName AttributeId = NAME_None;
	float OldValue = 0.0f;
	float NewValue = 0.0f;
};

struct Fdemo_mapActiveModifier
{
	Fdemo_mapModifierHandle Handle;
	Fdemo_mapModifierSpec Spec;
	uint64 InsertionOrder = 0;
};
