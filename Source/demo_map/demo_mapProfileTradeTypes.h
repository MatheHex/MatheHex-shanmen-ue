#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

enum class Edemo_mapProfileTradeKind : uint8
{
	Buy,
	Sell
};

enum class Edemo_mapProfileTradeStatus : uint8
{
	Committed,
	ReconciledAfterReload,
	SessionNotReady,
	OperationInProgress,
	StaleIntent,
	InvalidIntent,
	UnknownDefinition,
	NotPurchasable,
	NotSellable,
	InsufficientFunds,
	ShopStockUnavailable,
	ShopStockStale,
	ShopSlotNotFound,
	ShopSlotSold,
	ShopQuoteMismatch,
	ItemNotFound,
	ItemSelectedForDeployment,
	InvalidStack,
	ArithmeticOverflow,
	PersistentCommitRejected,
	CommitOutcomeRequiresReload,
	FatalRecovery
};

struct Fdemo_mapProfileTradeIntent
{
	Edemo_mapProfileTradeKind Kind = Edemo_mapProfileTradeKind::Buy;
	FGuid ExpectedProfileId;
	int32 ExpectedSaveGeneration = 0;
	FName ItemDefinitionId = NAME_None;
	FGuid ItemInstanceId;
	FName ShopStockPolicyId = NAME_None;
	int32 ShopStockGeneration = INDEX_NONE;
	FGuid ShopStockEventId;
	FName ShopSlotId = NAME_None;
	int64 ExpectedBuyValue = 0;
};

struct Fdemo_mapProfileShopCatalogRow
{
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FName ItemCategoryId = NAME_None;
	int32 Level = 0;
	int32 MaxStackSize = 1;
	int64 BuyPrice = 0;
	int64 SellPrice = 0;
};

struct Fdemo_mapProfileShopStockRow
{
	FName SlotId = NAME_None;
	int32 SlotOrdinal = INDEX_NONE;
	Edemo_mapPersistentShopStockEntryState State =
		Edemo_mapPersistentShopStockEntryState::Available;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FName ItemCategoryId = NAME_None;
	int32 Level = 0;
	Fdemo_mapRewardAffixSet AffixSet;
	int64 BuyPrice = 0;
	int64 SellPrice = 0;
};

struct Fdemo_mapProfileTradeResult
{
	Edemo_mapProfileTradeStatus Status = Edemo_mapProfileTradeStatus::SessionNotReady;
	Edemo_mapProfileTradeKind Kind = Edemo_mapProfileTradeKind::Buy;
	FGuid ProfileId;
	int32 SaveGenerationBefore = 0;
	int32 SaveGenerationAfter = 0;
	int64 BalanceBefore = 0;
	int64 BalanceAfter = 0;
	int64 UnitPrice = 0;
	int32 Quantity = 0;
	int64 TotalPrice = 0;
	FName ItemDefinitionId = NAME_None;
	FGuid ItemInstanceId;
	FString Diagnostic;

	bool IsCommitted() const
	{
		return Status == Edemo_mapProfileTradeStatus::Committed
			|| Status == Edemo_mapProfileTradeStatus::ReconciledAfterReload;
	}
};
