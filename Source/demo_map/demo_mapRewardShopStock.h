#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

enum class Edemo_mapShopStockCategory : uint8
{
	Weapon,
	Robe,
	HealingPill
};

struct Fdemo_mapRewardShopStockLevelWeight
{
	int32 Level = 0;
	int32 WeightBps = 0;
};

struct Fdemo_mapRewardShopStockSlotPolicy
{
	FName SlotId = NAME_None;
	int32 SlotOrdinal = INDEX_NONE;
	Edemo_mapShopStockCategory Category =
		Edemo_mapShopStockCategory::HealingPill;
	TArray<Fdemo_mapRewardShopStockLevelWeight> LevelWeights;
	int64 AffixBudget = 0;

	bool IsValid() const;
};

struct Fdemo_mapRewardShopStockPolicy
{
	FName PolicyId = NAME_None;
	int32 StockSize = 0;
	int32 InitialGeneration = 0;
	bool bFiniteStock = false;
	int32 QuantityPerSlot = 0;
	FName EquipmentAffixPolicyId = NAME_None;
	FName EquipmentPityPolicyId = NAME_None;
	FName JackpotPolicyId = NAME_None;
	FName RareExtremePolicyId = NAME_None;
	TArray<Fdemo_mapRewardShopStockSlotPolicy> Slots;

	bool IsValid() const;
};

/** Central deterministic P6 layout, generation, validation, and quote authority. */
struct Fdemo_mapRewardShopStock
{
	static const FName DefaultPolicyId;
	static const Fdemo_mapRewardShopStockPolicy& GetDefaultPolicy();

	static FGuid MakeEventId(
		const FGuid& ProfileId,
		FName PolicyId,
		int32 Generation);

	static bool Generate(
		const FGuid& ProfileId,
		int32 Generation,
		const FGuid& LastAppliedTerminalId,
		Fdemo_mapPersistentShopStockState& OutState,
		FString* OutError = nullptr);

	static bool ValidateState(
		const FGuid& ProfileId,
		const Fdemo_mapPersistentShopStockState& State,
		FString* OutError = nullptr);

	static bool TryComputeBuyValue(
		const Fdemo_mapPersistentItemRecord& Item,
		int64& OutValue,
		FString* OutError = nullptr);

	static bool TryComputeSellValue(
		const Fdemo_mapPersistentItemRecord& Item,
		int64& OutValue,
		FString* OutError = nullptr);
};
