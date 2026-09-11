#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"
#include "demo_mapItemTypes.h"

enum class Edemo_mapShanmenArmorResistanceProjectionStatus : uint8
{
	Projected,
	NotApplicable,
	InvalidInput,
	InvalidDefinition,
	LayerConflict
};

/** Atomic result of projecting one exact equipped armor item into defense. */
struct Fdemo_mapShanmenArmorResistanceProjectionResult
{
	Edemo_mapShanmenArmorResistanceProjectionStatus Status =
		Edemo_mapShanmenArmorResistanceProjectionStatus::InvalidInput;
	FShanmenDefenseSnapshot Defense;
	TArray<FGuid> ProjectedLayerIds;
	FString Diagnostic;

	bool IsSuccess() const
	{
		return Status
			== Edemo_mapShanmenArmorResistanceProjectionStatus::Projected
			|| Status
				== Edemo_mapShanmenArmorResistanceProjectionStatus::NotApplicable;
	}

	bool HasProjection() const
	{
		return Status
			== Edemo_mapShanmenArmorResistanceProjectionStatus::Projected
			&& !ProjectedLayerIds.IsEmpty();
	}
};

/**
 * Pure Code-A item-definition to CombatCore defense projection.
 * It does not choose product resistance values, inspect mutable inventory, or
 * consume resources; a later authority adapter must provide the exact equipped
 * item identity before using this contract in an incoming Impact.
 */
struct Fdemo_mapShanmenArmorResistanceProjection
{
	static Fdemo_mapShanmenArmorResistanceProjectionResult TryProject(
		const Fdemo_mapItemDefinition& Definition,
		const FGuid& ArmorItemInstanceId,
		const FGuid& TargetEntityId,
		const FShanmenDefenseSnapshot& BaseDefense);
};
