#include "demo_mapShanmenRunCorrelation.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemTypes.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapShanmenPreparationAdapter.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	void AppendGuidArray(const TArray<FGuid>& Values, TArray<FString>& Parts)
	{
		Parts.Add(FString::FromInt(Values.Num()));
		for (const FGuid& Value : Values)
		{
			Parts.Add(GuidDigits(Value));
		}
	}
}

bool Fdemo_mapShanmenRunCorrelation::IsValid() const
{
	const bool bIdentityValid = CorrelationId.IsValid()
		&& OwnerId.IsValid() && ScopeId.IsValid()
		&& ActiveRunId.IsValid() && PreparedRequestId.IsValid()
		&& PreparedReceiptId.IsValid() && LifecycleRequestId.IsValid()
		&& LifecycleReceiptId.IsValid() && PreparedAuthorityRevision >= 0
		&& LifecycleAuthorityRevision >= PreparedAuthorityRevision
		&& !OrderedPreparedItemInstanceIds.IsEmpty()
		&& HotbarItemInstanceIds.Num()
			== Fdemo_mapPersistentPreparationLayout::HotbarSlotCount;
	if (!bIdentityValid)
	{
		return false;
	}
	TSet<FGuid> PreparedItems;
	for (const FGuid& ItemId : OrderedPreparedItemInstanceIds)
	{
		if (!ItemId.IsValid() || PreparedItems.Contains(ItemId))
		{
			return false;
		}
		PreparedItems.Add(ItemId);
	}
	const TArray<FGuid> EquipmentItems =
	{
		WeaponItemInstanceId, ArmorItemInstanceId, AccessoryItemInstanceId,
		SpatialRingItemInstanceId, BackpackItemInstanceId
	};
	for (const FGuid& ItemId : EquipmentItems)
	{
		if (ItemId.IsValid() && !PreparedItems.Contains(ItemId))
		{
			return false;
		}
	}
	TSet<FGuid> RunInventoryItems;
	for (const FGuid& ItemId : OrderedRunInventoryItemInstanceIds)
	{
		if (!ItemId.IsValid() || RunInventoryItems.Contains(ItemId)
			|| !PreparedItems.Contains(ItemId))
		{
			return false;
		}
		RunInventoryItems.Add(ItemId);
	}
	for (const FGuid& ItemId : HotbarItemInstanceIds)
	{
		if (ItemId.IsValid() && !RunInventoryItems.Contains(ItemId))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenRunCorrelation::operator==(
	const Fdemo_mapShanmenRunCorrelation& Other) const
{
	return CorrelationId == Other.CorrelationId
		&& OwnerId == Other.OwnerId
		&& ScopeId == Other.ScopeId
		&& ActiveRunId == Other.ActiveRunId
		&& PreparedRequestId == Other.PreparedRequestId
		&& PreparedReceiptId == Other.PreparedReceiptId
		&& LifecycleRequestId == Other.LifecycleRequestId
		&& LifecycleReceiptId == Other.LifecycleReceiptId
		&& PreparedAuthorityRevision == Other.PreparedAuthorityRevision
		&& LifecycleAuthorityRevision == Other.LifecycleAuthorityRevision
		&& WeaponItemInstanceId == Other.WeaponItemInstanceId
		&& ArmorItemInstanceId == Other.ArmorItemInstanceId
		&& AccessoryItemInstanceId == Other.AccessoryItemInstanceId
		&& SpatialRingItemInstanceId == Other.SpatialRingItemInstanceId
		&& BackpackItemInstanceId == Other.BackpackItemInstanceId
		&& OrderedPreparedItemInstanceIds
			== Other.OrderedPreparedItemInstanceIds
		&& OrderedRunInventoryItemInstanceIds
			== Other.OrderedRunInventoryItemInstanceIds
		&& HotbarItemInstanceIds == Other.HotbarItemInstanceIds;
}

FString Fdemo_mapShanmenRunCorrelation::ToLogString() const
{
	return FString::Printf(
		TEXT("CorrelationId=%s OwnerId=%s ScopeId=%s ActiveRunId=%s PreparedRequestId=%s PreparedReceiptId=%s LifecycleRequestId=%s LifecycleReceiptId=%s PreparedRevision=%d LifecycleRevision=%d PreparedItems=%d RunInventoryItems=%d HotbarSlots=%d"),
		*CorrelationId.ToString(EGuidFormats::DigitsWithHyphens),
		*OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*ScopeId.ToString(EGuidFormats::DigitsWithHyphens),
		*ActiveRunId.ToString(EGuidFormats::DigitsWithHyphens),
		*PreparedRequestId.ToString(EGuidFormats::DigitsWithHyphens),
		*PreparedReceiptId.ToString(EGuidFormats::DigitsWithHyphens),
		*LifecycleRequestId.ToString(EGuidFormats::DigitsWithHyphens),
		*LifecycleReceiptId.ToString(EGuidFormats::DigitsWithHyphens),
		PreparedAuthorityRevision,
		LifecycleAuthorityRevision,
		OrderedPreparedItemInstanceIds.Num(),
		OrderedRunInventoryItemInstanceIds.Num(),
		HotbarItemInstanceIds.Num());
}

bool Fdemo_mapShanmenRunCorrelation::Build(
	const Fdemo_mapShanmenPreparedLoadoutReceipt& Prepared,
	const FShanmenItemTransactionReceipt& Lifecycle,
	Fdemo_mapShanmenRunCorrelation& OutCorrelation,
	FString& OutDiagnostic)
{
	OutCorrelation = Fdemo_mapShanmenRunCorrelation();
	const bool bLifecycleStart = Lifecycle.Operation
		== EShanmenItemTransactionOperation::StartPreparedRun
		|| Lifecycle.Operation
			== EShanmenItemTransactionOperation::ClaimPreparedRun;
	if (!Prepared.IsValid() || !Lifecycle.IsSuccess() || !bLifecycleStart
		|| Lifecycle.Phase != EShanmenItemTransactionPhase::Committed
		|| !Lifecycle.RequestId.IsValid() || !Lifecycle.ReceiptId.IsValid()
		|| !Lifecycle.ReservationId.IsValid()
		|| Lifecycle.ItemInstanceId != Prepared.BatchRequestId
		|| Lifecycle.ReservationIds.Num() != Prepared.OrderedLines.Num())
	{
		OutDiagnostic =
			TEXT("Run correlation requires one structurally matching committed prepared-loadout lifecycle receipt.");
		return false;
	}
	for (int32 Index = 0; Index < Prepared.OrderedLines.Num(); ++Index)
	{
		if (Prepared.OrderedLines[Index].ReservationId
			!= Lifecycle.ReservationIds[Index])
		{
			OutDiagnostic =
				TEXT("Run correlation rejected reordered prepared reservation identity.");
			return false;
		}
	}

	OutCorrelation.OwnerId = Prepared.OwnerId;
	OutCorrelation.ScopeId = Prepared.ScopeId;
	OutCorrelation.ActiveRunId = Lifecycle.ReservationId;
	OutCorrelation.PreparedRequestId = Prepared.BatchRequestId;
	OutCorrelation.PreparedReceiptId = Prepared.BatchReceiptId;
	OutCorrelation.LifecycleRequestId = Lifecycle.RequestId;
	OutCorrelation.LifecycleReceiptId = Lifecycle.ReceiptId;
	OutCorrelation.PreparedAuthorityRevision = Prepared.AuthorityRevision;
	OutCorrelation.LifecycleAuthorityRevision = Lifecycle.AuthorityRevision;
	OutCorrelation.WeaponItemInstanceId = Prepared.WeaponItemInstanceId;
	OutCorrelation.ArmorItemInstanceId = Prepared.ArmorItemInstanceId;
	OutCorrelation.AccessoryItemInstanceId = Prepared.AccessoryItemInstanceId;
	OutCorrelation.SpatialRingItemInstanceId =
		Prepared.SpatialRingItemInstanceId;
	OutCorrelation.BackpackItemInstanceId = Prepared.BackpackItemInstanceId;
	for (const Fdemo_mapShanmenPreparedLoadoutLine& Line : Prepared.OrderedLines)
	{
		OutCorrelation.OrderedPreparedItemInstanceIds.Add(
			Line.ItemInstanceId);
	}
	OutCorrelation.OrderedRunInventoryItemInstanceIds =
		Prepared.OrderedRunInventoryItemInstanceIds;
	OutCorrelation.HotbarItemInstanceIds = Prepared.HotbarItemInstanceIds;

	TArray<FString> Parts =
	{
		GuidDigits(OutCorrelation.OwnerId),
		GuidDigits(OutCorrelation.ScopeId),
		GuidDigits(OutCorrelation.ActiveRunId),
		GuidDigits(OutCorrelation.PreparedRequestId),
		GuidDigits(OutCorrelation.PreparedReceiptId),
		GuidDigits(OutCorrelation.LifecycleRequestId),
		GuidDigits(OutCorrelation.LifecycleReceiptId),
		FString::FromInt(OutCorrelation.PreparedAuthorityRevision),
		FString::FromInt(OutCorrelation.LifecycleAuthorityRevision),
		GuidDigits(OutCorrelation.WeaponItemInstanceId),
		GuidDigits(OutCorrelation.ArmorItemInstanceId),
		GuidDigits(OutCorrelation.AccessoryItemInstanceId),
		GuidDigits(OutCorrelation.SpatialRingItemInstanceId),
		GuidDigits(OutCorrelation.BackpackItemInstanceId),
		FString::FromInt(Prepared.OrderedLines.Num())
	};
	for (const Fdemo_mapShanmenPreparedLoadoutLine& Line : Prepared.OrderedLines)
	{
		Parts.Add(GuidDigits(Line.ReservationId));
		Parts.Add(GuidDigits(Line.ItemInstanceId));
		Parts.Add(Line.ItemDefinitionId.ToString());
		Parts.Add(FString::FromInt(static_cast<int32>(Line.ResourceKind)));
		Parts.Add(FString::FromInt(Line.Amount));
		Parts.Add(Line.PurposeId.ToString());
		Parts.Add(GuidDigits(Line.SourceContainerId));
		Parts.Add(FString::FromInt(Line.SourceSlotIndex));
	}
	AppendGuidArray(
		OutCorrelation.OrderedRunInventoryItemInstanceIds, Parts);
	AppendGuidArray(OutCorrelation.HotbarItemInstanceIds, Parts);
	OutCorrelation.CorrelationId = FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("Shanmen.Product.RunCorrelation.r1")), Parts);
	if (!OutCorrelation.IsValid())
	{
		OutCorrelation = Fdemo_mapShanmenRunCorrelation();
		OutDiagnostic =
			TEXT("Run correlation failed immutable identity validation.");
		return false;
	}
	OutDiagnostic.Reset();
	return true;
}
