#include "demo_mapShanmenPreparationAdapter.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	struct FPreparationSlotSpec
	{
		FName SlotId;
		FName ContainerType;
		FName PurposeId;
	};

	const TArray<FPreparationSlotSpec>& SlotSpecs()
	{
		static const TArray<FPreparationSlotSpec> Specs =
		{
			{ Fdemo_mapItemIds::WeaponSlot, TEXT("Weapon"), TEXT("Shanmen.Preparation.Weapon") },
			{ Fdemo_mapItemIds::ArmorSlot, TEXT("Armor"), TEXT("Shanmen.Preparation.Armor") },
			{ Fdemo_mapItemIds::AccessorySlot, TEXT("Accessory0"), TEXT("Shanmen.Preparation.Accessory") },
			{ Fdemo_mapItemIds::SpatialRingSlot, TEXT("SpatialRing"), TEXT("Shanmen.Preparation.SpatialRing") },
			{ Fdemo_mapItemIds::BackpackSlot, TEXT("Backpack"), TEXT("Shanmen.Preparation.Backpack") }
		};
		return Specs;
	}

	const FPreparationSlotSpec* FindSlotSpec(FName SlotId)
	{
		return SlotSpecs().FindByPredicate([SlotId](const FPreparationSlotSpec& Spec)
		{
			return Spec.SlotId == SlotId;
		});
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	}

	const FShanmenItemDefinition* FindDefinition(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FName DefinitionId)
	{
		return Snapshot.Definitions.FindByPredicate([DefinitionId](
			const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == DefinitionId;
		});
	}

	const FShanmenItemInstance* FindItem(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemId)
	{
		return Snapshot.Items.FindByPredicate([&ItemId](
			const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ItemId;
		});
	}

	int32 ReserveRevision(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemReservationSnapshot& Reservation)
	{
		const FShanmenItemProcessedRequestSnapshot* Processed =
			Snapshot.ProcessedRequests.FindByPredicate([&Reservation](
				const FShanmenItemProcessedRequestSnapshot& Candidate)
			{
				return Candidate.RequestId == Reservation.ReserveRequestId;
			});
		return Processed && Processed->Receipt.IsSuccess()
			&& Processed->Receipt.Operation == EShanmenItemTransactionOperation::Reserve
			? Processed->Receipt.AuthorityRevision
			: INDEX_NONE;
	}

	struct FSlotAnalysis
	{
		const FPreparationSlotSpec* Spec = nullptr;
		FGuid BaselineItemId;
		bool bHasHistory = false;
		const FShanmenItemReservationSnapshot* Latest = nullptr;
		int32 LatestReserveRevision = INDEX_NONE;
		FGuid SelectedItemId;
		TArray<const FShanmenItemReservationSnapshot*> ActiveReservations;
		TArray<const FShanmenItemReservationSnapshot*> CommittedReservations;
	};

	bool AnalyzeSlot(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FPreparationSlotSpec& Spec,
		FSlotAnalysis& Out,
		FString& OutDiagnostic)
	{
		Out = FSlotAnalysis();
		Out.Spec = &Spec;
		const FShanmenItemContainer* BaselineContainer = nullptr;
		for (const FShanmenItemContainer& Container : Snapshot.Containers)
		{
			if (Container.ContainerType != Spec.ContainerType)
			{
				continue;
			}
			if (BaselineContainer || Container.Slots.Num() != 1)
			{
				OutDiagnostic = FString::Printf(
					TEXT("Preparation slot %s requires exactly one one-cell authority container of type %s."),
					*Spec.SlotId.ToString(), *Spec.ContainerType.ToString());
				return false;
			}
			BaselineContainer = &Container;
		}
		if (!BaselineContainer)
		{
			OutDiagnostic = FString::Printf(
				TEXT("Preparation authority is missing container type %s."),
				*Spec.ContainerType.ToString());
			return false;
		}
		Out.BaselineItemId = BaselineContainer->Slots[0];

		for (const FShanmenItemReservationSnapshot& Reservation : Snapshot.Reservations)
		{
			if (Reservation.ResourceKind != EShanmenItemResourceKind::DeploymentLock
				|| Reservation.PurposeId != Spec.PurposeId)
			{
				continue;
			}
			Out.bHasHistory = true;
			const int32 Revision = ReserveRevision(Snapshot, Reservation);
			if (Revision == INDEX_NONE)
			{
				OutDiagnostic = TEXT("Preparation reservation has no matching successful Reserve receipt.");
				return false;
			}
			if (!Out.Latest || Revision > Out.LatestReserveRevision
				|| (Revision == Out.LatestReserveRevision
					&& GuidLess(Out.Latest->ReservationId, Reservation.ReservationId)))
			{
				Out.Latest = &Reservation;
				Out.LatestReserveRevision = Revision;
			}
			if (Reservation.State == EShanmenItemReservationState::Reserved)
			{
				Out.ActiveReservations.Add(&Reservation);
			}
			else if (Reservation.State == EShanmenItemReservationState::Committed)
			{
				Out.CommittedReservations.Add(&Reservation);
			}
		}

		if (!Out.bHasHistory)
		{
			Out.SelectedItemId = Out.BaselineItemId;
		}
		else if (Out.Latest
			&& (Out.Latest->State == EShanmenItemReservationState::Reserved
				|| Out.Latest->State == EShanmenItemReservationState::Committed))
		{
			Out.SelectedItemId = Out.Latest->ItemInstanceId;
		}
		if (Out.CommittedReservations.Num() > 1
			|| (!Out.CommittedReservations.IsEmpty()
				&& (!Out.Latest
					|| Out.Latest->State
						!= EShanmenItemReservationState::Committed)))
		{
			OutDiagnostic = FString::Printf(
				TEXT("Preparation purpose %s has an ambiguous committed deployment history."),
				*Spec.PurposeId.ToString());
			return false;
		}
		return true;
	}

	bool AnalyzeAllSlots(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		TArray<FSlotAnalysis>& OutSlots,
		FString& OutDiagnostic)
	{
		OutSlots.Reset();
		for (const FPreparationSlotSpec& Spec : SlotSpecs())
		{
			FSlotAnalysis& Slot = OutSlots.AddDefaulted_GetRef();
			if (!AnalyzeSlot(Snapshot, Spec, Slot, OutDiagnostic))
			{
				OutSlots.Reset();
				return false;
			}
		}
		return true;
	}

	FGuid* SelectedField(
		Fdemo_mapShanmenPreparationAuthorityProjection& Projection,
		FName SlotId)
	{
		if (SlotId == Fdemo_mapItemIds::WeaponSlot) return &Projection.SelectedWeaponId;
		if (SlotId == Fdemo_mapItemIds::ArmorSlot) return &Projection.SelectedArmorId;
		if (SlotId == Fdemo_mapItemIds::AccessorySlot) return &Projection.SelectedAccessoryId;
		if (SlotId == Fdemo_mapItemIds::SpatialRingSlot) return &Projection.SelectedSpatialRingId;
		if (SlotId == Fdemo_mapItemIds::BackpackSlot) return &Projection.SelectedBackpackId;
		return nullptr;
	}

	bool IsSelected(
		const Fdemo_mapShanmenPreparationAuthorityProjection& Projection,
		const FGuid& ItemId)
	{
		return Projection.SelectedWeaponId == ItemId
			|| Projection.SelectedArmorId == ItemId
			|| Projection.SelectedAccessoryId == ItemId
			|| Projection.SelectedSpatialRingId == ItemId
			|| Projection.SelectedBackpackId == ItemId;
	}

	Fdemo_mapShanmenPreparationAdapterResult MakeResult(
		Edemo_mapShanmenPreparationAdapterStatus Status,
		const FString& Diagnostic,
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr)
	{
		Fdemo_mapShanmenPreparationAdapterResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		if (Authority)
		{
			FShanmenItemAuthoritySnapshot Snapshot;
			FString ProjectionDiagnostic;
			if (Authority->TryCaptureSnapshot(Snapshot))
			{
				Fdemo_mapShanmenPreparationAdapter::BuildProjection(
					Snapshot, Authority->GetBoundOwnerId(), Result.Projection,
					&ProjectionDiagnostic);
			}
		}
		return Result;
	}

	FShanmenOperationContext MakeContext(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ScopeId,
		const FGuid& OwnerId,
		const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = RequestId;
		Context.Content = Snapshot.Content;
		return Context;
	}

	FGuid MakeReserveRequestId(
		const FSlotAnalysis& Slot,
		const FGuid& OwnerId,
		const FGuid& ScopeId,
		const FGuid& ItemId,
		int32 ItemRevision)
	{
		const FString Previous = Slot.Latest
			? GuidDigits(Slot.Latest->ReservationId)
			: FString::Printf(TEXT("baseline:%s"), *GuidDigits(Slot.BaselineItemId));
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Preparation.EquipmentReserve.r1"),
			{
				GuidDigits(OwnerId), GuidDigits(ScopeId),
				Slot.Spec->PurposeId.ToString(), Previous,
				GuidDigits(ItemId), FString::FromInt(ItemRevision)
			});
	}

	FGuid MakeCancelRequestId(const FGuid& ReservationId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Preparation.EquipmentCancel.r1"),
			{ GuidDigits(ReservationId) });
	}

	bool CancelReservation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemReservationSnapshot& Reservation,
		FString& OutDiagnostic)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context = MakeContext(
			Snapshot, Reservation.RunId, Reservation.OwnerId,
			MakeCancelRequestId(Reservation.ReservationId));
		Request.ReservationId = Reservation.ReservationId;
		const FShanmenItemDurableCommandResult Command =
			Authority.CancelDurable(Request);
		if (!Command.IsCommandSuccess())
		{
			OutDiagnostic = FString::Printf(
				TEXT("Authority could not cancel superseded preparation reservation %s: %s"),
				*GuidDigits(Reservation.ReservationId), *Command.Diagnostic);
			return false;
		}
		return true;
	}

	bool CleanupActiveReservations(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FSlotAnalysis& Slot,
		const FGuid& KeepReservationId,
		FString& OutDiagnostic)
	{
		TArray<const FShanmenItemReservationSnapshot*> Ordered =
			Slot.ActiveReservations;
		Ordered.Sort([&Snapshot](
			const FShanmenItemReservationSnapshot& Left,
			const FShanmenItemReservationSnapshot& Right)
		{
			const int32 LeftRevision = ReserveRevision(Snapshot, Left);
			const int32 RightRevision = ReserveRevision(Snapshot, Right);
			return LeftRevision == RightRevision
				? GuidLess(Left.ReservationId, Right.ReservationId)
				: LeftRevision < RightRevision;
		});
		for (const FShanmenItemReservationSnapshot* Reservation : Ordered)
		{
			if (Reservation && Reservation->ReservationId != KeepReservationId
				&& !CancelReservation(Authority, Snapshot, *Reservation, OutDiagnostic))
			{
				return false;
			}
		}
		return true;
	}

	constexpr int32 PreparationHotbarSlotCount =
		Fdemo_mapPersistentPreparationLayout::HotbarSlotCount;
	const FString RunInventoryPurposePrefix(
		TEXT("Shanmen.Preparation.RunInventory.r1.O"));

	struct FRunReservationMetadata
	{
		int32 Ordinal = INDEX_NONE;
		int32 HotbarSlot = 0;

		bool operator==(const FRunReservationMetadata& Other) const
		{
			return Ordinal == Other.Ordinal
				&& HotbarSlot == Other.HotbarSlot;
		}
	};

	struct FRunSelectionAnalysis
	{
		const FShanmenItemReservationSnapshot* Reservation = nullptr;
		const FShanmenItemInstance* Item = nullptr;
		FRunReservationMetadata Metadata;
		int32 ReserveAuthorityRevision = INDEX_NONE;
	};

	struct FRunInventoryAnalysis
	{
		TArray<FRunSelectionAnalysis> OrderedSelections;
		int32 NextOrdinal = 0;
	};

	FName MakeRunInventoryPurpose(const FRunReservationMetadata& Metadata)
	{
		return FName(*FString::Printf(
			TEXT("%s%08d.H%02d"), *RunInventoryPurposePrefix,
			Metadata.Ordinal, Metadata.HotbarSlot));
	}

	bool ParseRunInventoryPurpose(
		FName PurposeId,
		FRunReservationMetadata& OutMetadata)
	{
		OutMetadata = FRunReservationMetadata();
		const FString Value = PurposeId.ToString();
		if (!Value.StartsWith(RunInventoryPurposePrefix,
			ESearchCase::IgnoreCase))
		{
			return false;
		}
		const FString Payload = Value.Mid(RunInventoryPurposePrefix.Len());
		FString OrdinalText;
		FString HotbarText;
		if (!Payload.Split(TEXT(".H"), &OrdinalText, &HotbarText,
			ESearchCase::IgnoreCase, ESearchDir::FromStart)
			|| OrdinalText.Len() != 8 || HotbarText.Len() != 2
			|| !OrdinalText.IsNumeric() || !HotbarText.IsNumeric())
		{
			return false;
		}
		OutMetadata.Ordinal = FCString::Atoi(*OrdinalText);
		OutMetadata.HotbarSlot = FCString::Atoi(*HotbarText);
		return OutMetadata.Ordinal >= 0
			&& OutMetadata.HotbarSlot >= 0
			&& OutMetadata.HotbarSlot <= PreparationHotbarSlotCount;
	}

	bool HasRunInventoryPurpose(FName PurposeId)
	{
		return PurposeId.ToString().StartsWith(
			RunInventoryPurposePrefix, ESearchCase::IgnoreCase);
	}

	bool IsRunInventoryDefinition(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemInstance& Item,
		const Fdemo_mapItemDefinition** OutLegacyDefinition = nullptr)
	{
		const FShanmenItemDefinition* Definition =
			FindDefinition(Snapshot, Item.DefinitionId);
		const Fdemo_mapItemDefinition* LegacyDefinition =
			Fdemo_mapItemDefinitions::Find(Item.DefinitionId);
		if (OutLegacyDefinition)
		{
			*OutLegacyDefinition = LegacyDefinition;
		}
		return Definition
			&& Definition->Supports(EShanmenItemResourceKind::Quantity)
			&& LegacyDefinition
			&& (LegacyDefinition->CategoryId
					== Fdemo_mapItemIds::MaterialCategory
				|| LegacyDefinition->CategoryId
					== Fdemo_mapItemIds::ConsumableCategory)
			&& Item.State == EShanmenItemInstanceState::Stored
			&& Item.Quantity > 0;
	}

	bool AnalyzeRunInventory(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FRunInventoryAnalysis& Out,
		FString& OutDiagnostic)
	{
		Out = FRunInventoryAnalysis();
		TSet<FGuid> ActiveItems;
		TSet<int32> ActiveOrdinals;
		TSet<int32> ActiveHotbarSlots;
		for (const FShanmenItemReservationSnapshot& Reservation :
			Snapshot.Reservations)
		{
			if (Reservation.ResourceKind
					!= EShanmenItemResourceKind::Quantity
				|| !HasRunInventoryPurpose(Reservation.PurposeId))
			{
				continue;
			}
			FRunReservationMetadata Metadata;
			if (!ParseRunInventoryPurpose(
				Reservation.PurposeId, Metadata))
			{
				OutDiagnostic = FString::Printf(
					TEXT("Preparation Quantity reservation %s has malformed loadout metadata."),
					*GuidDigits(Reservation.ReservationId));
				return false;
			}
			const int32 Revision = ReserveRevision(Snapshot, Reservation);
			if (Revision == INDEX_NONE)
			{
				OutDiagnostic = TEXT("Preparation Quantity reservation has no successful Reserve receipt.");
				return false;
			}
			if (Reservation.State
					!= EShanmenItemReservationState::Reserved)
			{
				continue;
			}
			const FShanmenItemInstance* Item =
				FindItem(Snapshot, Reservation.ItemInstanceId);
			const Fdemo_mapItemDefinition* LegacyDefinition = nullptr;
			if (!Item || !IsRunInventoryDefinition(
					Snapshot, *Item, &LegacyDefinition)
				|| Reservation.Amount != Item->Quantity)
			{
				OutDiagnostic = TEXT("Active preparation Quantity intent must reserve one complete eligible stack.");
				return false;
			}
			if (ActiveItems.Contains(Item->ItemInstanceId)
				|| ActiveOrdinals.Contains(Metadata.Ordinal))
			{
				OutDiagnostic = TEXT("Preparation Quantity intents contain duplicate item or order identities.");
				return false;
			}
			if (Metadata.HotbarSlot > 0
				&& (ActiveHotbarSlots.Contains(Metadata.HotbarSlot)
					|| !LegacyDefinition->bHotbarEligible
					|| LegacyDefinition->CategoryId
						!= Fdemo_mapItemIds::ConsumableCategory))
			{
				OutDiagnostic = TEXT("Preparation Hotbar metadata is duplicated or references a non-consumable stack.");
				return false;
			}
			ActiveItems.Add(Item->ItemInstanceId);
			ActiveOrdinals.Add(Metadata.Ordinal);
			if (Metadata.HotbarSlot > 0)
			{
				ActiveHotbarSlots.Add(Metadata.HotbarSlot);
			}
			FRunSelectionAnalysis& Selection =
				Out.OrderedSelections.AddDefaulted_GetRef();
			Selection.Reservation = &Reservation;
			Selection.Item = Item;
			Selection.Metadata = Metadata;
			Selection.ReserveAuthorityRevision = Revision;
			Out.NextOrdinal = FMath::Max(
				Out.NextOrdinal, Metadata.Ordinal + 1);
		}
		Out.OrderedSelections.Sort([](
			const FRunSelectionAnalysis& Left,
			const FRunSelectionAnalysis& Right)
		{
			if (Left.Metadata.Ordinal != Right.Metadata.Ordinal)
			{
				return Left.Metadata.Ordinal < Right.Metadata.Ordinal;
			}
			if (Left.ReserveAuthorityRevision
				!= Right.ReserveAuthorityRevision)
			{
				return Left.ReserveAuthorityRevision
					< Right.ReserveAuthorityRevision;
			}
			return GuidLess(
				Left.Item->ItemInstanceId, Right.Item->ItemInstanceId);
		});
		return true;
	}

	const FRunSelectionAnalysis* FindRunSelection(
		const FRunInventoryAnalysis& Analysis,
		const FGuid& ItemId)
	{
		return Analysis.OrderedSelections.FindByPredicate(
			[&ItemId](const FRunSelectionAnalysis& Selection)
			{
				return Selection.Item
					&& Selection.Item->ItemInstanceId == ItemId;
			});
	}

	const FShanmenItemReservationSnapshot* FindLatestRunReservation(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemId)
	{
		const FShanmenItemReservationSnapshot* Latest = nullptr;
		int32 LatestRevision = INDEX_NONE;
		for (const FShanmenItemReservationSnapshot& Reservation :
			Snapshot.Reservations)
		{
			if (Reservation.ItemInstanceId != ItemId
				|| Reservation.ResourceKind
					!= EShanmenItemResourceKind::Quantity
				|| !HasRunInventoryPurpose(Reservation.PurposeId))
			{
				continue;
			}
			const int32 Revision = ReserveRevision(Snapshot, Reservation);
			if (Revision > LatestRevision
				|| (Revision == LatestRevision && Latest
					&& GuidLess(Latest->ReservationId,
						Reservation.ReservationId)))
			{
				Latest = &Reservation;
				LatestRevision = Revision;
			}
		}
		return Latest;
	}

	FName DefinitionIdForSelectedItem(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemId)
	{
		const FShanmenItemInstance* Item = FindItem(Snapshot, ItemId);
		return Item ? Item->DefinitionId : NAME_None;
	}

	bool ValidateRunCapacityAndHotbar(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenPreparationAuthorityProjection& Projection,
		const FRunInventoryAnalysis& Analysis,
		int32& OutQuickCapacity,
		FString& OutDiagnostic)
	{
		const Fdemo_mapInventoryCapacityResult Capacity =
			Fdemo_mapItemDefinitions::ResolveInventoryCapacity(
				DefinitionIdForSelectedItem(
					Snapshot, Projection.SelectedBackpackId),
				DefinitionIdForSelectedItem(
					Snapshot, Projection.SelectedSpatialRingId));
		if (!Capacity.bSuccess)
		{
			OutDiagnostic = Capacity.Diagnostic;
			return false;
		}
		if (Analysis.OrderedSelections.Num() > Capacity.Capacity)
		{
			OutDiagnostic = FString::Printf(
				TEXT("Preparation reserves %d complete stacks but selected storage holds only %d."),
				Analysis.OrderedSelections.Num(), Capacity.Capacity);
			return false;
		}
		OutQuickCapacity =
			Fdemo_mapPersistentPreparationLayout::BaseQuickItemSlotCount
			+ Capacity.RingQuickCapacity;
		for (int32 Index = 0;
			Index < Analysis.OrderedSelections.Num(); ++Index)
		{
			const FRunSelectionAnalysis& Selection =
				Analysis.OrderedSelections[Index];
			if (Selection.Metadata.HotbarSlot > 0
				&& Index >= OutQuickCapacity)
			{
				OutDiagnostic = TEXT("A Hotbar binding references a consumable outside the current Base Quick area.");
				return false;
			}
		}
		return true;
	}

	bool CanApplyEquipmentCapacityChange(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenPreparationAuthorityProjection& Projection,
		FName SlotId,
		const FGuid& ItemId,
		FString& OutDiagnostic)
	{
		if (SlotId != Fdemo_mapItemIds::BackpackSlot
			&& SlotId != Fdemo_mapItemIds::SpatialRingSlot)
		{
			return true;
		}
		Fdemo_mapShanmenPreparationAuthorityProjection Candidate =
			Projection;
		FGuid* Field = SelectedField(Candidate, SlotId);
		if (!Field)
		{
			OutDiagnostic = TEXT("Preparation capacity slot mapping is incomplete.");
			return false;
		}
		*Field = ItemId;
		FRunInventoryAnalysis Analysis;
		if (!AnalyzeRunInventory(Snapshot, Analysis, OutDiagnostic))
		{
			return false;
		}
		int32 QuickCapacity = 0;
		return ValidateRunCapacityAndHotbar(
			Snapshot, Candidate, Analysis, QuickCapacity, OutDiagnostic);
	}

	FGuid MakeRunReserveRequestId(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& OwnerId,
		const FGuid& ScopeId,
		const FShanmenItemInstance& Item,
		const FRunReservationMetadata& Metadata)
	{
		const FShanmenItemReservationSnapshot* Previous =
			FindLatestRunReservation(Snapshot, Item.ItemInstanceId);
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Preparation.RunInventoryReserve.r1"),
			{
				GuidDigits(OwnerId), GuidDigits(ScopeId),
				GuidDigits(Item.ItemInstanceId),
				Previous ? GuidDigits(Previous->ReservationId) : TEXT("none"),
				MakeRunInventoryPurpose(Metadata).ToString(),
				FString::FromInt(Item.Quantity),
				FString::FromInt(Item.Revision)
			});
	}

	FGuid MakeRunCancelRequestId(const FGuid& ReservationId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Preparation.RunInventoryCancel.r1"),
			{ GuidDigits(ReservationId) });
	}

	bool CancelRunReservation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemReservationSnapshot& Reservation,
		FString& OutDiagnostic)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context = MakeContext(
			Snapshot, Reservation.RunId, Reservation.OwnerId,
			MakeRunCancelRequestId(Reservation.ReservationId));
		Request.ReservationId = Reservation.ReservationId;
		const FShanmenItemDurableCommandResult Command =
			Authority.CancelDurable(Request);
		if (!Command.IsCommandSuccess())
		{
			OutDiagnostic = FString::Printf(
				TEXT("Authority could not cancel preparation Quantity reservation %s: %s"),
				*GuidDigits(Reservation.ReservationId), *Command.Diagnostic);
			return false;
		}
		return true;
	}

	FShanmenItemDurableCommandResult ReserveRunSelection(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& OwnerId,
		const FGuid& ScopeId,
		const FShanmenItemInstance& Item,
		const FRunReservationMetadata& Metadata)
	{
		FShanmenItemReserveRequest Request;
		Request.Context = MakeContext(
			Snapshot, ScopeId, OwnerId,
			MakeRunReserveRequestId(
				Snapshot, OwnerId, ScopeId, Item, Metadata));
		Request.ItemInstanceId = Item.ItemInstanceId;
		Request.ResourceKind = EShanmenItemResourceKind::Quantity;
		Request.Amount = Item.Quantity;
		Request.ExpectedItemRevision = Item.Revision;
		Request.PurposeId = MakeRunInventoryPurpose(Metadata);
		return Authority.ReserveDurable(Request);
	}

	bool ReplaceRunSelectionMetadata(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FRunSelectionAnalysis& Selection,
		const FRunReservationMetadata& NewMetadata,
		bool& OutRestored,
		FString& OutDiagnostic)
	{
		OutRestored = false;
		if (!Selection.Reservation || !Selection.Item)
		{
			OutDiagnostic = TEXT("Preparation Quantity metadata replacement has no active selection.");
			return false;
		}
		if (Selection.Metadata == NewMetadata)
		{
			return true;
		}
		FShanmenItemAuthoritySnapshot Snapshot;
		if (!Authority.TryCaptureSnapshot(Snapshot)
			|| !CancelRunReservation(
				Authority, Snapshot, *Selection.Reservation, OutDiagnostic))
		{
			return false;
		}
		if (!Authority.TryCaptureSnapshot(Snapshot))
		{
			OutDiagnostic = TEXT("Authority snapshot disappeared after cancelling old preparation metadata.");
			return false;
		}
		const FShanmenItemInstance* Item =
			FindItem(Snapshot, Selection.Item->ItemInstanceId);
		if (!Item)
		{
			OutDiagnostic = TEXT("Preparation stack disappeared while replacing metadata.");
			return false;
		}
		const FShanmenItemDurableCommandResult Replacement =
			ReserveRunSelection(
				Authority, Snapshot, Selection.Reservation->OwnerId,
				Selection.Reservation->RunId, *Item, NewMetadata);
		if (Replacement.IsCommandSuccess())
		{
			return true;
		}

		const FString ReplacementFailure = Replacement.Diagnostic;
		if (Authority.TryCaptureSnapshot(Snapshot))
		{
			Item = FindItem(Snapshot, Selection.Item->ItemInstanceId);
			if (Item)
			{
				const FShanmenItemDurableCommandResult Rollback =
					ReserveRunSelection(
						Authority, Snapshot,
						Selection.Reservation->OwnerId,
						Selection.Reservation->RunId,
						*Item, Selection.Metadata);
				if (Rollback.IsCommandSuccess())
				{
					OutRestored = true;
					OutDiagnostic = FString::Printf(
						TEXT("Preparation metadata replacement failed and the previous binding was restored: %s"),
						*ReplacementFailure);
					return false;
				}
				OutDiagnostic = FString::Printf(
					TEXT("Preparation metadata replacement failed (%s) and rollback also failed (%s)."),
					*ReplacementFailure, *Rollback.Diagnostic);
				return false;
			}
		}
		OutDiagnostic = FString::Printf(
			TEXT("Preparation metadata replacement failed and rollback state is unavailable: %s"),
			*ReplacementFailure);
		return false;
	}
}

bool Fdemo_mapShanmenPreparationAdapter::BuildProjection(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const FGuid& ExpectedOwnerId,
	Fdemo_mapShanmenPreparationAuthorityProjection& OutProjection,
	FString* OutDiagnostic)
{
	auto Fail = [&OutProjection, OutDiagnostic](const FString& Diagnostic)
	{
		OutProjection = Fdemo_mapShanmenPreparationAuthorityProjection();
		if (OutDiagnostic) *OutDiagnostic = Diagnostic;
		return false;
	};

	FShanmenItemRepository Validation;
	EShanmenItemTransactionError ValidationError =
		EShanmenItemTransactionError::None;
	if (!ExpectedOwnerId.IsValid()
		|| !Validation.TryLoadSnapshot(Snapshot, &ValidationError))
	{
		return Fail(FString::Printf(
			TEXT("Preparation authority snapshot failed validation (%d)."),
			static_cast<int32>(ValidationError)));
	}

	FGuid ScopeId;
	for (const FShanmenItemContainer& Container : Snapshot.Containers)
	{
		if (Container.OwnerId != ExpectedOwnerId
			|| (ScopeId.IsValid() && Container.RunId != ScopeId))
		{
			return Fail(TEXT("Preparation authority containers do not share the bound owner and scope."));
		}
		ScopeId = Container.RunId;
	}
	if (!ScopeId.IsValid())
	{
		return Fail(TEXT("Preparation authority has no scoped containers."));
	}
	for (const FShanmenItemInstance& Item : Snapshot.Items)
	{
		if (Item.OwnerId != ExpectedOwnerId || Item.RunId != ScopeId)
		{
			return Fail(TEXT("Preparation authority items do not share the bound owner and scope."));
		}
	}

	TArray<FSlotAnalysis> Slots;
	FString Diagnostic;
	if (!AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
	{
		return Fail(Diagnostic);
	}

	OutProjection = Fdemo_mapShanmenPreparationAuthorityProjection();
	OutProjection.OwnerId = ExpectedOwnerId;
	OutProjection.ScopeId = ScopeId;
	OutProjection.AuthorityRevision = Snapshot.AuthorityRevision;
	for (const FSlotAnalysis& Slot : Slots)
	{
		FGuid* Field = SelectedField(OutProjection, Slot.Spec->SlotId);
		if (!Field)
		{
			return Fail(TEXT("Preparation slot projection mapping is incomplete."));
		}
		*Field = Slot.SelectedItemId;
	}
	FRunInventoryAnalysis RunInventory;
	if (!AnalyzeRunInventory(Snapshot, RunInventory, Diagnostic))
	{
		return Fail(Diagnostic);
	}
	int32 QuickCapacity = 0;
	if (!ValidateRunCapacityAndHotbar(
		Snapshot, OutProjection, RunInventory,
		QuickCapacity, Diagnostic))
	{
		return Fail(Diagnostic);
	}
	OutProjection.HotbarBindings.SlotBindings.Init(
		FGuid(), PreparationHotbarSlotCount);
	for (const FRunSelectionAnalysis& Selection :
		RunInventory.OrderedSelections)
	{
		OutProjection.OrderedSelectedMaterialIds.Add(
			Selection.Item->ItemInstanceId);
		if (Selection.Metadata.HotbarSlot > 0)
		{
			OutProjection.HotbarBindings.SlotBindings[
				Selection.Metadata.HotbarSlot - 1] =
				Selection.Item->ItemInstanceId;
		}
	}

	const FShanmenItemContainer* Warehouse = nullptr;
	for (const FShanmenItemContainer& Container : Snapshot.Containers)
	{
		if (Container.ContainerType != FName(TEXT("Warehouse"))) continue;
		if (Warehouse)
		{
			return Fail(TEXT("Preparation authority has more than one Warehouse container."));
		}
		Warehouse = &Container;
	}
	if (!Warehouse
		|| Warehouse->Slots.Num() != Fdemo_mapPersistentWarehouseLayout::SlotCount)
	{
		return Fail(TEXT("Preparation authority requires one canonical 30-cell Warehouse container."));
	}
	OutProjection.WarehouseLayout.bInitialized = true;
	OutProjection.WarehouseLayout.SlotItemInstanceIds = Warehouse->Slots;

	for (const FShanmenItemInstance& Item : Snapshot.Items)
	{
		if (Item.State == EShanmenItemInstanceState::Depleted)
		{
			continue;
		}
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = Item.ItemInstanceId;
		Row.ItemDefinitionId = Item.DefinitionId;
		Row.StackCount = Item.Quantity;
		Row.bSafeInPermanentStash = true;
		const int32 RunInventoryIndex =
			OutProjection.OrderedSelectedMaterialIds.IndexOfByKey(
				Item.ItemInstanceId);
		Row.bSelected = IsSelected(OutProjection, Item.ItemInstanceId)
			|| RunInventoryIndex != INDEX_NONE;
		if (const Fdemo_mapItemDefinition* LegacyDefinition =
			Fdemo_mapItemDefinitions::Find(Item.DefinitionId))
		{
			Row.ItemCategoryId = LegacyDefinition->CategoryId;
			Row.CompatibleEquipmentSlotId =
				LegacyDefinition->CompatibleSlotIds.Num() == 1
				? LegacyDefinition->CompatibleSlotIds[0] : NAME_None;
		}
		Row.bMaterialSelectionEligible =
			IsRunInventoryDefinition(Snapshot, Item);
		Row.bInBaseQuickItemArea = RunInventoryIndex >= 0
			&& RunInventoryIndex < QuickCapacity;
		OutProjection.OrderedRows.Add(MoveTemp(Row));
	}
	OutProjection.OrderedRows.Sort([](
		const Fdemo_mapProfilePreparationStashRow& Left,
		const Fdemo_mapProfilePreparationStashRow& Right)
	{
		return GuidLess(Left.ItemInstanceId, Right.ItemInstanceId);
	});
	OutProjection.Diagnostic =
		TEXT("P1.7 equipment, complete-stack RunInventory reservations, and Hotbar bindings are projected from ShanmenItems; Start Run commit remains disabled.");
	if (OutDiagnostic) OutDiagnostic->Reset();
	return true;
}

Fdemo_mapShanmenPreparationAdapterResult
Fdemo_mapShanmenPreparationAdapter::SelectEquipment(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	FName SlotId,
	const FGuid& ItemInstanceId)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::AuthorityNotReady,
			TEXT("Authority-native preparation requires a Ready ShanmenItems owner."),
			&Authority);
	}
	const FPreparationSlotSpec* Spec = FindSlotSpec(SlotId);
	if (!Spec)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidSlot,
			TEXT("Preparation equipment requires Weapon, Armor, Accessory, SpatialRing, or Backpack."),
			&Authority);
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			TEXT("ShanmenItems snapshot is unavailable."), &Authority);
	}
	Fdemo_mapShanmenPreparationAuthorityProjection Projection;
	FString Diagnostic;
	if (!BuildProjection(
		Snapshot, Authority.GetBoundOwnerId(), Projection, &Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic, &Authority);
	}
	TArray<FSlotAnalysis> Slots;
	if (!AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic, &Authority);
	}
	FSlotAnalysis* Slot = Slots.FindByPredicate([SlotId](const FSlotAnalysis& Candidate)
	{
		return Candidate.Spec && Candidate.Spec->SlotId == SlotId;
	});
	if (!Slot)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidSlot,
			TEXT("Preparation slot analysis mapping is incomplete."), &Authority);
	}

	if (ItemInstanceId.IsValid())
	{
		const FShanmenItemInstance* Item = FindItem(Snapshot, ItemInstanceId);
		if (!Item || Item->OwnerId != Projection.OwnerId
			|| Item->RunId != Projection.ScopeId
			|| Item->State == EShanmenItemInstanceState::Depleted)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::ItemNotFound,
				TEXT("Equipment ItemInstanceId is absent from the bound ShanmenItems scope."),
				&Authority);
		}
		const FShanmenItemDefinition* Definition =
			FindDefinition(Snapshot, Item->DefinitionId);
		const Fdemo_mapItemDefinition* LegacyDefinition =
			Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
		if (!Definition
			|| !Definition->Supports(EShanmenItemResourceKind::DeploymentLock)
			|| !LegacyDefinition || Item->Quantity != 1
			|| !LegacyDefinition->CompatibleSlotIds.Contains(SlotId))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::SlotRejected,
				TEXT("Equipment definition, quantity, or immutable slot compatibility rejected the selection."),
				&Authority);
		}
		for (const FSlotAnalysis& Other : Slots)
		{
			if (Other.Spec->SlotId != SlotId
				&& Other.SelectedItemId == ItemInstanceId)
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::DuplicateSelection,
					TEXT("The same ItemInstanceId cannot occupy two preparation equipment slots."),
					&Authority);
			}
		}
		for (const FShanmenItemReservationSnapshot& Reservation : Snapshot.Reservations)
		{
			if (Reservation.ItemInstanceId == ItemInstanceId
				&& Reservation.ResourceKind == EShanmenItemResourceKind::DeploymentLock
				&& Reservation.PurposeId != Spec->PurposeId
				&& (Reservation.State == EShanmenItemReservationState::Reserved
					|| Reservation.State == EShanmenItemReservationState::Committed))
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::DuplicateSelection,
					TEXT("The ItemInstanceId already has another active deployment purpose."),
					&Authority);
			}
		}
		if (!CanApplyEquipmentCapacityChange(
			Snapshot, Projection, SlotId, ItemInstanceId, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::SelectionLimitExceeded,
				Diagnostic, &Authority);
		}

		if (Slot->SelectedItemId == ItemInstanceId)
		{
			if (!Slot->Latest || Slot->Latest->State == EShanmenItemReservationState::Committed)
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::NoChange,
					TEXT("Equipment selection already matches the durable authority projection."),
					&Authority);
			}
			if (!CleanupActiveReservations(
				Authority, Snapshot, *Slot, Slot->Latest->ReservationId, Diagnostic))
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
					Diagnostic, &Authority);
			}
			return MakeResult(
				Slot->ActiveReservations.Num() > 1
					? Edemo_mapShanmenPreparationAdapterStatus::Accepted
					: Edemo_mapShanmenPreparationAdapterStatus::NoChange,
				TEXT("Equipment selection is durable and superseded locks are reconciled."),
				&Authority);
		}
		if (!Slot->CommittedReservations.IsEmpty())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::DeployedSelectionLocked,
				TEXT("A committed deployment owns this slot and must be released by run settlement."),
				&Authority);
		}

		FShanmenItemReserveRequest Request;
		Request.Context = MakeContext(
			Snapshot, Projection.ScopeId, Projection.OwnerId,
			MakeReserveRequestId(
				*Slot, Projection.OwnerId, Projection.ScopeId,
				ItemInstanceId, Item->Revision));
		Request.ItemInstanceId = ItemInstanceId;
		Request.ResourceKind = EShanmenItemResourceKind::DeploymentLock;
		Request.Amount = 1;
		Request.ExpectedItemRevision = Item->Revision;
		Request.PurposeId = Spec->PurposeId;
		const FShanmenItemDurableCommandResult Reserve =
			Authority.ReserveDurable(Request);
		if (!Reserve.IsCommandSuccess())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CommandRejected,
				FString::Printf(TEXT("Equipment reserve was rejected: %s"),
					*Reserve.Diagnostic), &Authority);
		}
		if (!Authority.TryCaptureSnapshot(Snapshot)
			|| !AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Authority could not project the durable equipment reserve."),
				&Authority);
		}
		Slot = Slots.FindByPredicate([SlotId](const FSlotAnalysis& Candidate)
		{
			return Candidate.Spec && Candidate.Spec->SlotId == SlotId;
		});
		if (!Slot || !Slot->Latest
			|| Slot->Latest->ReservationId != Reserve.Receipt.ReservationId)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Durable reserve did not become the newest slot intent."),
				&Authority);
		}
		if (!CleanupActiveReservations(
			Authority, Snapshot, *Slot, Reserve.Receipt.ReservationId, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				Diagnostic, &Authority);
		}
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::Accepted,
			TEXT("Authority-native equipment selection committed."), &Authority);
	}

	if (!Slot->CommittedReservations.IsEmpty())
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::DeployedSelectionLocked,
			TEXT("A committed deployment cannot be cleared before run settlement."),
			&Authority);
	}
	if (!CanApplyEquipmentCapacityChange(
		Snapshot, Projection, SlotId, FGuid(), Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::SelectionLimitExceeded,
			Diagnostic, &Authority);
	}
	if (!Slot->bHasHistory && !Slot->BaselineItemId.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::NoChange,
			TEXT("Preparation equipment slot is already empty."), &Authority);
	}

	if (!Slot->bHasHistory)
	{
		const FShanmenItemInstance* Baseline =
			FindItem(Snapshot, Slot->BaselineItemId);
		if (!Baseline)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Migrated equipment baseline references a missing item."),
				&Authority);
		}
		FShanmenItemReserveRequest ReserveRequest;
		ReserveRequest.Context = MakeContext(
			Snapshot, Projection.ScopeId, Projection.OwnerId,
			MakeReserveRequestId(
				*Slot, Projection.OwnerId, Projection.ScopeId,
				Baseline->ItemInstanceId, Baseline->Revision));
		ReserveRequest.ItemInstanceId = Baseline->ItemInstanceId;
		ReserveRequest.ResourceKind = EShanmenItemResourceKind::DeploymentLock;
		ReserveRequest.Amount = 1;
		ReserveRequest.ExpectedItemRevision = Baseline->Revision;
		ReserveRequest.PurposeId = Spec->PurposeId;
		const FShanmenItemDurableCommandResult Reserve =
			Authority.ReserveDurable(ReserveRequest);
		if (!Reserve.IsCommandSuccess())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CommandRejected,
				FString::Printf(TEXT("Baseline clear tombstone reserve failed: %s"),
					*Reserve.Diagnostic), &Authority);
		}
		if (!Authority.TryCaptureSnapshot(Snapshot))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Authority snapshot disappeared after baseline reserve."),
				&Authority);
		}
		const FShanmenItemReservationSnapshot* Reserved =
			Snapshot.Reservations.FindByPredicate([&Reserve](
				const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == Reserve.Receipt.ReservationId;
			});
		if (!Reserved || !CancelReservation(
			Authority, Snapshot, *Reserved, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				Diagnostic.IsEmpty()
					? TEXT("Baseline clear tombstone remains reserved; retry will reconcile it.")
					: Diagnostic,
				&Authority);
		}
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::Accepted,
			TEXT("Migrated equipment baseline cleared with a durable authority tombstone."),
			&Authority);
	}

	if (Slot->Latest
		&& Slot->Latest->State == EShanmenItemReservationState::Reserved
		&& !CancelReservation(Authority, Snapshot, *Slot->Latest, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
			Diagnostic, &Authority);
	}
	if (!Authority.TryCaptureSnapshot(Snapshot)
		|| !AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			TEXT("Authority could not re-project the cleared equipment slot."),
			&Authority);
	}
	Slot = Slots.FindByPredicate([SlotId](const FSlotAnalysis& Candidate)
	{
		return Candidate.Spec && Candidate.Spec->SlotId == SlotId;
	});
	if (!Slot || !CleanupActiveReservations(
		Authority, Snapshot, *Slot, FGuid(), Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
			Diagnostic, &Authority);
	}
	return MakeResult(
		Edemo_mapShanmenPreparationAdapterStatus::Accepted,
		TEXT("Authority-native equipment slot cleared."), &Authority);
}

Fdemo_mapShanmenPreparationAdapterResult
Fdemo_mapShanmenPreparationAdapter::SelectMaterial(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FGuid& ItemInstanceId,
	bool bSelected)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::AuthorityNotReady,
			TEXT("Authority-native RunInventory selection requires a Ready ShanmenItems owner."),
			&Authority);
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			TEXT("ShanmenItems snapshot is unavailable."), &Authority);
	}
	Fdemo_mapShanmenPreparationAuthorityProjection Projection;
	FString Diagnostic;
	if (!BuildProjection(
		Snapshot, Authority.GetBoundOwnerId(), Projection, &Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic, &Authority);
	}
	FRunInventoryAnalysis RunInventory;
	if (!AnalyzeRunInventory(Snapshot, RunInventory, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic, &Authority);
	}
	const FRunSelectionAnalysis* Existing =
		FindRunSelection(RunInventory, ItemInstanceId);
	const FShanmenItemInstance* Item = FindItem(Snapshot, ItemInstanceId);
	if (!Item || Item->OwnerId != Projection.OwnerId
		|| Item->RunId != Projection.ScopeId
		|| Item->State == EShanmenItemInstanceState::Depleted)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::ItemNotFound,
			TEXT("RunInventory ItemInstanceId is absent from the bound ShanmenItems scope."),
			&Authority);
	}
	if (!IsRunInventoryDefinition(Snapshot, *Item)
		|| IsSelected(Projection, ItemInstanceId))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::MaterialRejected,
			TEXT("RunInventory accepts complete Material or Consumable stacks that are not equipment selections."),
			&Authority);
	}

	if (bSelected)
	{
		if (Existing)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::DuplicateSelection,
				TEXT("The same ItemInstanceId cannot appear twice in RunInventory."),
				&Authority);
		}
		if (RunInventory.NextOrdinal >= 100000000)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::SelectionLimitExceeded,
				TEXT("RunInventory ordering metadata is exhausted and requires reconciliation."),
				&Authority);
		}
		FRunSelectionAnalysis CandidateSelection;
		CandidateSelection.Item = Item;
		CandidateSelection.Metadata.Ordinal = RunInventory.NextOrdinal;
		RunInventory.OrderedSelections.Add(CandidateSelection);
		int32 QuickCapacity = 0;
		if (!ValidateRunCapacityAndHotbar(
			Snapshot, Projection, RunInventory,
			QuickCapacity, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::SelectionLimitExceeded,
				Diagnostic, &Authority);
		}
		const FRunReservationMetadata Metadata =
			{ RunInventory.NextOrdinal, 0 };
		const FShanmenItemDurableCommandResult Reserve =
			ReserveRunSelection(
				Authority, Snapshot, Projection.OwnerId,
				Projection.ScopeId, *Item, Metadata);
		if (!Reserve.IsCommandSuccess())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CommandRejected,
				FString::Printf(
					TEXT("Complete-stack preparation reserve was rejected: %s"),
					*Reserve.Diagnostic),
				&Authority);
		}
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::Accepted,
			TEXT("Complete stack reserved for the next Run without consuming it."),
			&Authority);
	}

	if (!Existing)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::NoChange,
			TEXT("RunInventory stack is already unselected."), &Authority);
	}
	if (!CancelRunReservation(
		Authority, Snapshot, *Existing->Reservation, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::CommandRejected,
			Diagnostic, &Authority);
	}
	return MakeResult(
		Edemo_mapShanmenPreparationAdapterStatus::Accepted,
		TEXT("Complete-stack preparation reserve released; any Hotbar binding was cleared with it."),
		&Authority);
}

Fdemo_mapShanmenPreparationAdapterResult
Fdemo_mapShanmenPreparationAdapter::SetHotbarSlot(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	int32 ExternalSlotNumber,
	const FGuid& ItemInstanceId)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::AuthorityNotReady,
			TEXT("Authority-native Hotbar binding requires a Ready ShanmenItems owner."),
			&Authority);
	}
	if (ExternalSlotNumber < 1
		|| ExternalSlotNumber > PreparationHotbarSlotCount)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::HotbarRejected,
			TEXT("Hotbar slot must be 1..9."), &Authority);
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	Fdemo_mapShanmenPreparationAuthorityProjection Projection;
	FRunInventoryAnalysis RunInventory;
	FString Diagnostic;
	auto Capture = [&]()
	{
		return Authority.TryCaptureSnapshot(Snapshot)
			&& BuildProjection(
				Snapshot, Authority.GetBoundOwnerId(),
				Projection, &Diagnostic)
			&& AnalyzeRunInventory(
				Snapshot, RunInventory, Diagnostic);
	};
	if (!Capture())
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic.IsEmpty()
				? TEXT("ShanmenItems loadout projection is unavailable.")
				: Diagnostic,
			&Authority);
	}

	const FRunSelectionAnalysis* Occupant =
		RunInventory.OrderedSelections.FindByPredicate(
			[ExternalSlotNumber](const FRunSelectionAnalysis& Selection)
			{
				return Selection.Metadata.HotbarSlot
					== ExternalSlotNumber;
			});
	if (!ItemInstanceId.IsValid())
	{
		if (!Occupant)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::NoChange,
				TEXT("Hotbar slot is already empty."), &Authority);
		}
		FRunReservationMetadata Cleared = Occupant->Metadata;
		Cleared.HotbarSlot = 0;
		bool bRestored = false;
		if (!ReplaceRunSelectionMetadata(
			Authority, *Occupant, Cleared, bRestored, Diagnostic))
		{
			return MakeResult(
				bRestored
					? Edemo_mapShanmenPreparationAdapterStatus::CommandRejected
					: Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				Diagnostic, &Authority);
		}
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::Accepted,
			TEXT("Authority-native Hotbar slot cleared."), &Authority);
	}

	const FRunSelectionAnalysis* Desired =
		FindRunSelection(RunInventory, ItemInstanceId);
	if (!Desired)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::HotbarRejected,
			TEXT("Hotbar accepts only a stack already reserved in RunInventory."),
			&Authority);
	}
	const Fdemo_mapItemDefinition* LegacyDefinition =
		Fdemo_mapItemDefinitions::Find(Desired->Item->DefinitionId);
	int32 QuickCapacity = 0;
	if (!LegacyDefinition || !LegacyDefinition->bHotbarEligible
		|| LegacyDefinition->CategoryId
			!= Fdemo_mapItemIds::ConsumableCategory
		|| !ValidateRunCapacityAndHotbar(
			Snapshot, Projection, RunInventory,
			QuickCapacity, Diagnostic)
		|| RunInventory.OrderedSelections.IndexOfByPredicate(
			[&ItemInstanceId](const FRunSelectionAnalysis& Selection)
			{
				return Selection.Item
					&& Selection.Item->ItemInstanceId == ItemInstanceId;
			}) >= QuickCapacity)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::HotbarRejected,
			Diagnostic.IsEmpty()
				? TEXT("Hotbar accepts only selected Consumables in the current Base Quick area.")
				: Diagnostic,
			&Authority);
	}
	if (Desired->Metadata.HotbarSlot == ExternalSlotNumber)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::NoChange,
			TEXT("Hotbar binding already matches the durable authority projection."),
			&Authority);
	}

	FGuid DisplacedItemId;
	FRunReservationMetadata DisplacedMetadata;
	if (Occupant && Occupant->Item->ItemInstanceId != ItemInstanceId)
	{
		DisplacedItemId = Occupant->Item->ItemInstanceId;
		DisplacedMetadata = Occupant->Metadata;
		FRunReservationMetadata Unbound = DisplacedMetadata;
		Unbound.HotbarSlot = 0;
		bool bRestored = false;
		if (!ReplaceRunSelectionMetadata(
			Authority, *Occupant, Unbound, bRestored, Diagnostic))
		{
			return MakeResult(
				bRestored
					? Edemo_mapShanmenPreparationAdapterStatus::CommandRejected
					: Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				Diagnostic, &Authority);
		}
		if (!Capture())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				TEXT("Displaced Hotbar item was unbound but the authority cannot be re-projected."),
				&Authority);
		}
		Desired = FindRunSelection(RunInventory, ItemInstanceId);
		if (!Desired)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				TEXT("Desired RunInventory stack disappeared after Hotbar displacement."),
				&Authority);
		}
	}

	FRunReservationMetadata Bound = Desired->Metadata;
	Bound.HotbarSlot = ExternalSlotNumber;
	bool bDesiredRestored = false;
	if (!ReplaceRunSelectionMetadata(
		Authority, *Desired, Bound, bDesiredRestored, Diagnostic))
	{
		const FString DesiredFailure = Diagnostic;
		if (DisplacedItemId.IsValid() && Capture())
		{
			const FRunSelectionAnalysis* Displaced =
				FindRunSelection(RunInventory, DisplacedItemId);
			bool bDisplacedRestored = false;
			FString RestoreDiagnostic;
			if (!Displaced || !ReplaceRunSelectionMetadata(
				Authority, *Displaced, DisplacedMetadata,
				bDisplacedRestored, RestoreDiagnostic))
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
					FString::Printf(
						TEXT("Desired Hotbar binding failed (%s); displaced binding rollback failed (%s)."),
						*DesiredFailure, *RestoreDiagnostic),
					&Authority);
			}
		}
		return MakeResult(
			bDesiredRestored
				? Edemo_mapShanmenPreparationAdapterStatus::CommandRejected
				: Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
			DesiredFailure, &Authority);
	}
	return MakeResult(
		Edemo_mapShanmenPreparationAdapterStatus::Accepted,
		TEXT("Authority-native Hotbar binding committed without consuming the stack."),
		&Authority);
}
