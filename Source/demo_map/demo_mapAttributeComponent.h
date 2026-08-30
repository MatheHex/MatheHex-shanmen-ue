#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "demo_mapAttributeTypes.h"
#include "demo_mapAttributeComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(Fdemo_mapAttributeChangedSignature, const Fdemo_mapAttributeChange&);

/** Single authoritative attribute and modifier state owned by the runtime player Pawn. */
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class Udemo_mapAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapAttributeComponent();
	virtual void BeginPlay() override;

	bool IsAttributeRegistered(FName AttributeId) const;
	bool GetBaseValue(FName AttributeId, float& OutValue) const;
	bool GetFinalValue(FName AttributeId, float& OutValue) const;
	bool SetBaseValue(FName AttributeId, float NewValue);
	bool AddModifier(const Fdemo_mapModifierSpec& Spec, Fdemo_mapModifierHandle& OutHandle);
	bool RemoveModifier(Fdemo_mapModifierHandle Handle);
	Edemo_mapExactModifierMutationStatus EnsureModifierApplied(
		const Fdemo_mapModifierSpec& Spec,
		Fdemo_mapModifierHandle Handle);
	Edemo_mapExactModifierMutationStatus EnsureModifierRemoved(
		const Fdemo_mapModifierSpec& Spec,
		Fdemo_mapModifierHandle Handle);
	int32 RemoveModifiersBySource(FName SourceId);
	const Fdemo_mapAttributeSnapshot& GetFinalSnapshot() const { return FinalSnapshot; }
	int32 GetActiveModifierCount() const { return ActiveModifiers.Num(); }
	int32 GetModifierCountBySource(FName SourceId) const;
	void ForceRecalculate();

	Fdemo_mapAttributeChangedSignature OnAttributeChanged;

private:
	void InitializeDefinitions();
	bool IsModifierSpecValid(const Fdemo_mapModifierSpec& Spec) const;
	void Recalculate();
	float ApplyModifiers(FName AttributeId, float StartingValue) const;
	void ApplyMoveSpeed(float NewMoveSpeed) const;

	TMap<FName, float> BaseValues;
	TArray<Fdemo_mapActiveModifier> ActiveModifiers;
	Fdemo_mapAttributeSnapshot FinalSnapshot;
	uint64 NextInsertionOrder = 1;
	bool bDefinitionsInitialized = false;
	bool bRecalculating = false;
};
