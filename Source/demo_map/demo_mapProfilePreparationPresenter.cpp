#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapRewardAffix.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapRewardEventTypes.h"

namespace
{
	FString DisplayNameFor(const Fdemo_mapProfilePreparationStashRow& Row)
	{
		if (const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Row.ItemDefinitionId))
		{
			return Definition->DisplayName.ToString();
		}
		return Row.ItemDefinitionId.IsNone() ? TEXT("Unknown Item") : Row.ItemDefinitionId.ToString();
	}

	FGuid SelectedEquipmentId(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		FName SlotId)
	{
		if (SlotId == Fdemo_mapItemIds::WeaponSlot) return Snapshot.SelectedWeaponId;
		if (SlotId == Fdemo_mapItemIds::ArmorSlot) return Snapshot.SelectedArmorId;
		if (SlotId == Fdemo_mapItemIds::AccessorySlot) return Snapshot.SelectedAccessoryId;
		if (SlotId == Fdemo_mapItemIds::SpatialRingSlot) return Snapshot.SelectedSpatialRingId;
		return Snapshot.SelectedBackpackId;
	}

	FString ResourceLabelFor(
		const Fdemo_mapProfilePreparationStashRow& Row)
	{
		TArray<FString> Parts;
		if (Row.MaxDurability > 0)
		{
			Parts.Add(FString::Printf(
				TEXT("DUR %d/%d"), Row.Durability, Row.MaxDurability));
		}
		if (Row.MaxCharges > 0)
		{
			Parts.Add(FString::Printf(
				TEXT("CHG %d/%d"), Row.Charges, Row.MaxCharges));
		}
		return FString::Join(Parts, TEXT(" | "));
	}
}

Fdemo_mapProfilePreparationViewState Fdemo_mapProfilePreparationPresenter::BuildViewState(
	const Fdemo_mapProfilePreparationSnapshot& Snapshot)
{
	Fdemo_mapProfilePreparationViewState View;
	View.SessionState = Snapshot.SessionState;
	View.ProfileId = Snapshot.ProfileId;
	View.SaveGeneration = Snapshot.SaveGeneration;
	View.PersistentSpiritStones = Snapshot.PersistentSpiritStones;
	View.ShopStockPolicyId = Snapshot.ShopStockPolicyId;
	View.ShopStockGeneration = Snapshot.ShopStockGeneration;
	View.ShopStockEventId = Snapshot.ShopStockEventId;
	View.SessionLabel = SessionStateLabel(Snapshot.SessionState);
	View.VisibleDiagnostic = Snapshot.VisibleDiagnostic;
	View.LastSettlementId = Snapshot.LastSettlementId;
	View.LastTerminalReason = Snapshot.LastTerminalReason;
	View.bPreparationOperationsEnabled = Snapshot.SessionState == Edemo_mapProfileSessionState::ReadyForPreparation;
	View.bCanStartRun = View.bPreparationOperationsEnabled && Snapshot.bCanStartRun;
	View.bCanRetrySettlement = Snapshot.SessionState == Edemo_mapProfileSessionState::PendingSettlementRetry
		&& Snapshot.bCanRetrySettlement;

	for (const Fdemo_mapProfileShopStockRow& Source : Snapshot.OrderedShopStockRows)
	{
		Fdemo_mapProfileShopRowView Row;
		Row.SlotId = Source.SlotId;
		Row.SlotOrdinal = Source.SlotOrdinal;
		Row.State = Source.State;
		Row.ItemInstanceId = Source.ItemInstanceId;
		Row.ItemDefinitionId = Source.ItemDefinitionId;
		Row.DisplayName = Source.State
				== Edemo_mapPersistentShopStockEntryState::Sold
			? TEXT("SOLD")
			: Source.DisplayName;
		Row.CategoryAndLevel = Source.State
				== Edemo_mapPersistentShopStockEntryState::Sold
			? Source.SlotId.ToString()
			: FString::Printf(
				TEXT("%s / L%d"),
				*Source.ItemCategoryId.ToString(),
				Source.Level);
		Row.MaxStackSize = 1;
		Row.BuyPrice = Source.BuyPrice;
		Row.SellPrice = Source.SellPrice;
		Row.AffixSet = Source.AffixSet;
		Row.AffixLabel =
			Fdemo_mapRewardAffixPolicyRegistry::BuildDisplayLabel(
				Source.AffixSet);
		Row.StockLabel = Source.State
				== Edemo_mapPersistentShopStockEntryState::Sold
			? TEXT("SOLD")
			: TEXT("STOCK=1");
		Row.bCanAfford = Source.State
				== Edemo_mapPersistentShopStockEntryState::Available
			&& Snapshot.PersistentSpiritStones >= Source.BuyPrice;
		Row.bCanBuy = View.bPreparationOperationsEnabled
			&& Source.State
				== Edemo_mapPersistentShopStockEntryState::Available
			&& Row.bCanAfford;
		Row.BuyDiagnostic = Source.State
				== Edemo_mapPersistentShopStockEntryState::Sold
			? TEXT("SOLD")
			: !View.bPreparationOperationsEnabled
			? TEXT("Preparation-only trade is unavailable in the current session state.")
			: (Row.bCanAfford ? TEXT("Available") : TEXT("Insufficient Persistent Spirit Stones"));
		View.OrderedShopRows.Add(MoveTemp(Row));
	}

	bool bHasPendingRisk = false;
	TArray<FGuid> OrderedStashIds;
	int32 GridIndex = 0;
	for (const Fdemo_mapProfilePreparationStashRow& Source : Snapshot.OrderedPermanentStashRows)
	{
		Fdemo_mapProfilePreparationRowView Row;
		Row.ItemInstanceId = Source.ItemInstanceId;
			Row.ItemDefinitionId = Source.ItemDefinitionId;
			Row.DisplayName = DisplayNameFor(Source);
			Row.StackCount = Source.StackCount;
			Row.Durability = Source.Durability;
			Row.MaxDurability = Source.MaxDurability;
			Row.Charges = Source.Charges;
			Row.MaxCharges = Source.MaxCharges;
			Row.ResourceLabel = ResourceLabelFor(Source);
			Row.RewardEventKind = Source.RewardEventKind;
			Row.RewardEventId = Source.RewardEventId;
			Row.RewardValueMultiplierBps = Source.RewardValueMultiplierBps;
			Row.RewardSourceRoleId = Source.RewardSourceRoleId;
			Row.RareRewardEventId = Source.RareRewardEventId;
			Row.RareRewardPolicyId = Source.RareRewardPolicyId;
			Row.RareRewardTierId = Source.RareRewardTierId;
			Row.RareRewardBonusValue = Source.RareRewardBonusValue;
			Row.AffixSet = Source.AffixSet;
			const bool bJackpot =
				Source.RewardEventKind == Edemo_mapRewardEventKind::Jackpot;
			const bool bRare = Source.RareRewardEventId.IsValid();
			const FString AffixLabel =
				Fdemo_mapRewardAffixPolicyRegistry::BuildDisplayLabel(
					Source.AffixSet);
			if (bJackpot || bRare || !AffixLabel.IsEmpty())
			{
				TArray<FString> Parts;
				if (bJackpot) Parts.Add(TEXT("JACKPOT ×6"));
				if (bRare) Parts.Add(TEXT("EXTREME VALUE"));
				if (!AffixLabel.IsEmpty()) Parts.Add(AffixLabel);
				Row.RewardLabel = FString::Join(Parts, TEXT(" | "));
				Row.DisplayName += TEXT("  ") + Row.RewardLabel;
			}
			Row.CompatibleEquipmentSlotId = Source.CompatibleEquipmentSlotId;
		Row.ItemCategoryId = Source.ItemCategoryId;
		Row.SlotOrCategoryLabel = !Source.CompatibleEquipmentSlotId.IsNone()
			? SlotLabel(Source.CompatibleEquipmentSlotId)
			: Source.ItemCategoryId.ToString();
		Row.bSelected = Source.bSelected;
		Row.bMaterialSelectionEligible = Source.bMaterialSelectionEligible;
		Row.bStashOnly = Source.CompatibleEquipmentSlotId.IsNone() && !Source.bMaterialSelectionEligible;
			if (const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Source.ItemDefinitionId))
			{
				Row.UnitSellPrice = Definition->SellPrice;
				int64 EffectiveSellValue = 0;
				FString SellValueDiagnostic;
				if (Definition->bSellable && Definition->SellPrice > 0 && Source.StackCount > 0
					&& Fdemo_mapItemSellValueRules::TryCompute(
						Source.ItemDefinitionId,
						Source.StackCount,
						Source.RewardValueMultiplierBps,
						Source.AffixSet.TotalResolvedValue(),
						Source.RareRewardBonusValue,
						EffectiveSellValue,
						&SellValueDiagnostic))
				{
					Row.TotalSellPrice = EffectiveSellValue;
					Row.bCanSell = View.bPreparationOperationsEnabled && !Source.bSelected;
					Row.SellDiagnostic = Source.bSelected
						? TEXT("Deselect before selling")
						: (View.bPreparationOperationsEnabled
							? (Row.RewardLabel.IsEmpty()
								? FString::Printf(TEXT("Effective Sell %lld"), EffectiveSellValue)
								: FString::Printf(TEXT("%s / Effective Sell %lld"), *Row.RewardLabel, EffectiveSellValue))
							: TEXT("Preparation-only trade unavailable"));
				}
				else
				{
					Row.SellDiagnostic = SellValueDiagnostic.IsEmpty()
						? TEXT("Definition is not sellable")
						: SellValueDiagnostic;
				}
			}
		Row.RiskLabel = Source.bSelected ? TEXT("WILL BE AT RISK") : TEXT("SAFE");
		Row.GridIndex = GridIndex++;
		bHasPendingRisk |= Source.bSelected;
		OrderedStashIds.Add(Source.ItemInstanceId);
		View.OrderedPermanentStashRows.Add(MoveTemp(Row));
	}
	View.PermanentStashGrid = Fdemo_mapItemViewRules::BuildGridFromOccupiedOrder(
		OrderedStashIds,
		OrderedStashIds.Num());

	for (FName SlotId : Fdemo_mapItemDefinitions::GetEquipmentSlotIds())
	{
		Fdemo_mapProfilePreparationEquipmentSlotView Slot;
		Slot.SlotId = SlotId;
		Slot.ItemInstanceId = SelectedEquipmentId(Snapshot, SlotId);
		Slot.DisplayName = TEXT("— EMPTY —");
		if (const Fdemo_mapProfilePreparationRowView* Row = View.OrderedPermanentStashRows.FindByPredicate(
			[&Slot](const Fdemo_mapProfilePreparationRowView& Candidate)
			{
				return Candidate.ItemInstanceId == Slot.ItemInstanceId;
			}))
		{
			Slot.DisplayName = Row->DisplayName;
		}
		View.OrderedEquipmentSlots.Add(MoveTemp(Slot));
	}
	View.OrderedSelectedMaterialIds = Snapshot.OrderedSelectedMaterialIds;
	View.EntityLoadout =
		Fdemo_mapEntityLoadoutPresenter::BuildPlayerPreparationView(
			Snapshot);
	View.RunInventoryUsedSlots =
		View.EntityLoadout.UsedCarriedSlots;
	View.RunInventoryCapacity =
		View.EntityLoadout.TotalCarriedCapacity;
	View.RunInventoryGrid = Fdemo_mapItemViewRules::BuildGridFromOccupiedOrder(
		Snapshot.OrderedSelectedMaterialIds,
		View.RunInventoryCapacity);
	if (View.VisibleDiagnostic.IsEmpty()
		&& !View.EntityLoadout.Diagnostic.IsEmpty())
	{
		View.VisibleDiagnostic = View.EntityLoadout.Diagnostic;
	}
	View.HotbarBindings = Snapshot.HotbarBindings.SlotBindings.Num() == Fdemo_mapHotbarBindingSnapshot::SlotCount
		? Snapshot.HotbarBindings
		: Fdemo_mapHotbarBindingSnapshot();

	if (Snapshot.SessionState == Edemo_mapProfileSessionState::RunActive)
	{
		View.RiskPhase = Edemo_mapProfilePreparationRiskPhase::AtRisk;
		View.RiskLabel = TEXT("AT RISK");
		View.RiskWarning = TEXT("RunActive: committed loadout is now in the risk domain. Preparation operations are disabled.");
	}
	else if (bHasPendingRisk)
	{
		View.RiskPhase = Edemo_mapProfilePreparationRiskPhase::WillBeAtRisk;
		View.RiskLabel = TEXT("WILL BE AT RISK");
		View.RiskWarning = TEXT("Selected loadout remains SAFE in Permanent Stash until Start Run commits; it WILL BE AT RISK after a successful commit.");
	}
	else
	{
		View.RiskPhase = Edemo_mapProfilePreparationRiskPhase::Safe;
		View.RiskLabel = TEXT("SAFE");
		View.RiskWarning = TEXT("Permanent Stash items are SAFE. Empty loadout is allowed.");
	}

	View.LastSettlementSummary = Snapshot.LastSettlementId.IsValid()
		? FString::Printf(TEXT("Last Settlement %s / %s"),
			*Snapshot.LastSettlementId.ToString(EGuidFormats::DigitsWithHyphens),
			*TerminalReasonLabel(Snapshot.LastTerminalReason))
		: FString::Printf(TEXT("Last Settlement — / %s"), *TerminalReasonLabel(Snapshot.LastTerminalReason));
	if (Snapshot.LastTradeResult.IsSet())
	{
		const Fdemo_mapProfileTradeResult& Trade = Snapshot.LastTradeResult.GetValue();
		View.LastTradeSummary = FString::Printf(
			TEXT("%s / status=%d / total=%lld / balance=%lld→%lld / generation=%d→%d / %s"),
			Trade.Kind == Edemo_mapProfileTradeKind::Buy ? TEXT("BUY") : TEXT("SELL"),
			static_cast<int32>(Trade.Status),
			Trade.TotalPrice,
			Trade.BalanceBefore,
			Trade.BalanceAfter,
			Trade.SaveGenerationBefore,
			Trade.SaveGenerationAfter,
			*Trade.Diagnostic);
	}
	else
	{
		View.LastTradeSummary = TEXT("No trade submitted.");
	}
	return View;
}

FString Fdemo_mapProfilePreparationPresenter::SessionStateLabel(Edemo_mapProfileSessionState State)
{
	switch (State)
	{
	case Edemo_mapProfileSessionState::ReadyForPreparation: return TEXT("ReadyForPreparation");
	case Edemo_mapProfileSessionState::RunActive: return TEXT("RunActive");
	case Edemo_mapProfileSessionState::PendingSettlementRetry: return TEXT("PendingSettlementRetry");
	case Edemo_mapProfileSessionState::RecoveryRequired: return TEXT("RecoveryRequired");
	case Edemo_mapProfileSessionState::FatalProfileError: return TEXT("FatalProfileError");
	default: return TEXT("Uninitialized");
	}
}

FString Fdemo_mapProfilePreparationPresenter::TerminalReasonLabel(Edemo_mapRunEndReason Reason)
{
	switch (Reason)
	{
	case Edemo_mapRunEndReason::Extraction: return TEXT("Extraction");
	case Edemo_mapRunEndReason::Death: return TEXT("Death");
	case Edemo_mapRunEndReason::Abandon: return TEXT("Abandon");
	case Edemo_mapRunEndReason::RecoveredAbandon: return TEXT("RecoveredAbandon");
	case Edemo_mapRunEndReason::ActivationFailure: return TEXT("ActivationFailure");
	default: return TEXT("None");
	}
}

FString Fdemo_mapProfilePreparationPresenter::SlotLabel(FName SlotId)
{
	if (SlotId == Fdemo_mapItemIds::WeaponSlot) return TEXT("兵器");
	if (SlotId == Fdemo_mapItemIds::ArmorSlot) return TEXT("道袍");
	if (SlotId == Fdemo_mapItemIds::AccessorySlot) return TEXT("饰品");
	if (SlotId == Fdemo_mapItemIds::SpatialRingSlot) return TEXT("空间戒指");
	if (SlotId == Fdemo_mapItemIds::BackpackSlot) return TEXT("空间道具");
	return SlotId.ToString();
}
