#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenItemTags.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	uint32 FloatValueBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return Bits;
	}

	FString FloatBits(float Value)
	{
		return FString::Printf(TEXT("%08X"), FloatValueBits(Value));
	}

	bool TryParseHex32(const FString& Text, uint32& OutValue)
	{
		OutValue = 0;
		if (Text.Len() != 8)
		{
			return false;
		}
		for (const TCHAR Character : Text)
		{
			uint32 Digit = 0;
			if (Character >= TEXT('0') && Character <= TEXT('9'))
			{
				Digit = static_cast<uint32>(Character - TEXT('0'));
			}
			else if (Character >= TEXT('A') && Character <= TEXT('F'))
			{
				Digit = static_cast<uint32>(Character - TEXT('A') + 10);
			}
			else if (Character >= TEXT('a') && Character <= TEXT('f'))
			{
				Digit = static_cast<uint32>(Character - TEXT('a') + 10);
			}
			else
			{
				return false;
			}
			OutValue = (OutValue << 4) | Digit;
		}
		return true;
	}

	bool TryParseNonNegativeInt64(const FString& Text, int64& OutValue)
	{
		OutValue = 0;
		if (Text.IsEmpty())
		{
			return false;
		}
		for (const TCHAR Character : Text)
		{
			if (Character < TEXT('0') || Character > TEXT('9'))
			{
				return false;
			}
			const int64 Digit = Character - TEXT('0');
			if (OutValue > (MAX_int64 - Digit) / 10)
			{
				return false;
			}
			OutValue = OutValue * 10 + Digit;
		}
		return true;
	}

	float FloatFromBits(uint32 Bits)
	{
		float Value = 0.0f;
		FMemory::Memcpy(&Value, &Bits, sizeof(Value));
		return Value;
	}

	bool IsPreparedEquipment(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FGuid& ItemInstanceId)
	{
		return ItemInstanceId == Correlation.WeaponItemInstanceId
			|| ItemInstanceId == Correlation.ArmorItemInstanceId
			|| ItemInstanceId == Correlation.AccessoryItemInstanceId
			|| ItemInstanceId == Correlation.SpatialRingItemInstanceId
			|| ItemInstanceId == Correlation.BackpackItemInstanceId;
	}

	bool IsExternallyCoordinated(
		const FGuid& LayerId,
		const TSet<FGuid>* ExternallyCoordinatedLayerIds)
	{
		return ExternallyCoordinatedLayerIds
			&& ExternallyCoordinatedLayerIds->Contains(LayerId);
	}

	bool HasResourceBackedDefense(
		const FShanmenImpactRequest& Request,
		const TSet<FGuid>* ExternallyCoordinatedLayerIds)
	{
		return Request.Defense.Layers.ContainsByPredicate(
			[ExternallyCoordinatedLayerIds](const FShanmenDefenseLayer& Layer)
			{
				return Layer.bRequiresCommitOnTrigger
					&& !IsExternallyCoordinated(
						Layer.LayerId, ExternallyCoordinatedLayerIds);
			});
	}

	FName SpiritGuardRobeRuleId()
	{
		return TEXT("Combat.Defense.Player.SpiritGuardRobe.Durability.r1");
	}

	FName HeartProtectingMirrorRuleId()
	{
		return TEXT("Combat.Defense.Player.HeartProtectingMirror.Charge.r1");
	}

	FName LegacyFlatReductionRuleId()
	{
		return TEXT("Combat.Defense.Player.FlatDamageReduction.r1");
	}

	FName MakeTemporaryDefensePurpose(const FGuid& ImpactId)
	{
		return ImpactId.IsValid()
			? FName(*FString::Printf(TEXT("SMDR1_%s"), *GuidDigits(ImpactId)))
			: NAME_None;
	}

	bool TryParseTemporaryDefensePurpose(FName PurposeId, FGuid& OutImpactId)
	{
		OutImpactId.Invalidate();
		const FString Text = PurposeId.ToString();
		return Text.StartsWith(TEXT("SMDR1_"), ESearchCase::CaseSensitive)
			&& FGuid::ParseExact(
				Text.RightChop(6), EGuidFormats::Digits, OutImpactId);
	}

	const FShanmenItemInstance* FindItem(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemInstanceId)
	{
		return Snapshot.Items.FindByPredicate(
			[&ItemInstanceId](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == ItemInstanceId;
			});
	}

	const FShanmenItemDefinition* FindDefinition(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FName DefinitionId)
	{
		return Snapshot.Definitions.FindByPredicate(
			[DefinitionId](const FShanmenItemDefinition& Definition)
			{
				return Definition.DefinitionId == DefinitionId;
			});
	}

	const FShanmenItemReservationSnapshot* FindReservation(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ReservationId)
	{
		return Snapshot.Reservations.FindByPredicate(
			[&ReservationId](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ReservationId == ReservationId;
			});
	}

	bool TryGetSpiritGuardReduction(float& OutReduction)
	{
		OutReduction = 0.0f;
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::SpiritGuardRobe);
		if (!Definition || Definition->MaxDurability <= 0)
		{
			return false;
		}
		int32 MatchCount = 0;
		for (const Fdemo_mapItemEffectParameter& Parameter :
			Definition->EffectParameters)
		{
			if (Parameter.ParameterId
				!= Fdemo_mapItemEffectIds::FlatDamageReduction)
			{
				continue;
			}
			++MatchCount;
			OutReduction = static_cast<float>(Parameter.Value);
		}
		return MatchCount == 1 && FMath::IsFinite(OutReduction)
			&& OutReduction > 0.0f;
	}

	bool TryGetHeartMirrorVitalityFloor(float& OutVitalityFloor)
	{
		OutVitalityFloor = 0.0f;
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(
				Fdemo_mapItemIds::HeartProtectingMirror);
		if (!Definition
			|| !Definition->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::LethalInterception)
			|| Definition->MaxCharges != 1)
		{
			return false;
		}
		int32 MatchCount = 0;
		for (const Fdemo_mapItemEffectParameter& Parameter :
			Definition->EffectParameters)
		{
			if (Parameter.ParameterId
				!= Fdemo_mapItemEffectIds::LethalVitalityFloor)
			{
				continue;
			}
			++MatchCount;
			OutVitalityFloor = static_cast<float>(Parameter.Value);
		}
		return MatchCount == 1 && FMath::IsFinite(OutVitalityFloor)
			&& OutVitalityFloor > 0.0f;
	}

	bool RemoveSpiritGuardFromAggregate(
		FShanmenDefenseSnapshot& InOutDefense,
		float SpiritGuardReduction,
		FString& OutDiagnostic)
	{
		int32 AggregateIndex = INDEX_NONE;
		for (int32 Index = 0; Index < InOutDefense.Layers.Num(); ++Index)
		{
			const FShanmenDefenseLayer& Layer = InOutDefense.Layers[Index];
			if (Layer.RuleId != LegacyFlatReductionRuleId())
			{
				continue;
			}
			if (AggregateIndex != INDEX_NONE
				|| Layer.Operation != EShanmenDefenseOperation::AbsorbPoints
				|| Layer.Order != FShanmenDefenseOrder::Resistance
				|| Layer.bRequiresCommitOnTrigger
				|| !Layer.LayerTags.HasTagExact(
					FShanmenCombatNativeTags::DefenseArmor())
				|| !FMath::IsFinite(Layer.Magnitude)
				|| Layer.Magnitude + KINDA_SMALL_NUMBER
					< SpiritGuardReduction)
			{
				OutDiagnostic =
					TEXT("Legacy armor aggregation does not contain one exact Spirit Guard contribution.");
				return false;
			}
			AggregateIndex = Index;
		}
		if (AggregateIndex == INDEX_NONE)
		{
			OutDiagnostic =
				TEXT("Equipped Spirit Guard Robe is absent from the captured armor aggregation.");
			return false;
		}

		FShanmenDefenseLayer& Aggregate = InOutDefense.Layers[AggregateIndex];
		const float Residual = FMath::Max(
			0.0f, Aggregate.Magnitude - SpiritGuardReduction);
		if (Residual <= KINDA_SMALL_NUMBER)
		{
			InOutDefense.Layers.RemoveAt(AggregateIndex);
		}
		else
		{
			Aggregate.Magnitude = Residual;
		}
		return true;
	}

	bool HasPreparedIntentForReservation(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ImpactId,
		const FGuid& ReservationId)
	{
		return Snapshot.ProcessedRequests.ContainsByPredicate(
			[&ImpactId, &ReservationId](
				const FShanmenItemProcessedRequestSnapshot& Processed)
			{
				return Processed.Receipt.IsSuccess()
					&& Processed.Receipt.Operation
						== EShanmenItemTransactionOperation::PreparePreparedRunResourceIntent
					&& Processed.Receipt.ReservationId == ImpactId
					&& Processed.Receipt.ReservationIds.Contains(ReservationId);
			});
	}

	FGuid MakeDefenseReserveRequestId(
		FName Namespace,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FGuid& ImpactId,
		const FGuid& ItemInstanceId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			Namespace,
			{
				GuidDigits(Correlation.OwnerId),
				GuidDigits(Correlation.ScopeId),
				GuidDigits(Correlation.ActiveRunId),
				GuidDigits(ImpactId),
				GuidDigits(ItemInstanceId)
			});
	}

	FGuid MakeDefenseCancelRequestId(const FGuid& ReservationId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.SpiritGuardRobe.DurabilityCancel.r1"),
			{ GuidDigits(ReservationId) });
	}

	void AppendSpiritGuardLayer(
		FShanmenDefenseSnapshot& InOutDefense,
		const FGuid& ReservationId,
		const FGuid& ItemInstanceId,
		float Reduction)
	{
		FShanmenDefenseLayer& Layer =
			InOutDefense.Layers.AddDefaulted_GetRef();
		Layer.LayerId = ReservationId;
		Layer.RuleId = SpiritGuardRobeRuleId();
		Layer.SourceInstanceId = ItemInstanceId;
		Layer.Operation = EShanmenDefenseOperation::AbsorbPoints;
		Layer.Order = FShanmenDefenseOrder::Resistance + 1;
		Layer.Magnitude = Reduction;
		Layer.bRequiresCommitOnTrigger = true;
		Layer.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseArmor());
		Layer.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
	}

	void AppendHeartMirrorLayer(
		FShanmenDefenseSnapshot& InOutDefense,
		const FGuid& ReservationId,
		const FGuid& ItemInstanceId,
		float VitalityFloor)
	{
		FShanmenDefenseLayer& Layer =
			InOutDefense.Layers.AddDefaulted_GetRef();
		Layer.LayerId = ReservationId;
		Layer.RuleId = HeartProtectingMirrorRuleId();
		Layer.SourceInstanceId = ItemInstanceId;
		Layer.Operation = EShanmenDefenseOperation::PreventLethal;
		Layer.Order = FShanmenDefenseOrder::LethalInterception;
		Layer.Magnitude = VitalityFloor;
		Layer.bRequiresCommitOnTrigger = true;
		Layer.LayerTags.AddTag(
			FShanmenCombatNativeTags::DefenseLethalIntercept());
		Layer.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
	}
}

Fdemo_mapShanmenDefenseOrphanRecoveryResult
Fdemo_mapShanmenDefenseResourceAdapter::RecoverOrphanedDefenseReservations(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority)
{
	Fdemo_mapShanmenDefenseOrphanRecoveryResult Result;
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		Result.Diagnostic =
			TEXT("Orphan defense recovery requires the ready item authority on the Game Thread.");
		return Result;
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Result.Diagnostic)
		|| !Authority.TryCaptureSnapshot(Snapshot))
	{
		if (Result.Diagnostic.IsEmpty())
		{
			Result.Diagnostic =
				TEXT("Orphan defense recovery could not capture the active authority snapshot.");
		}
		return Result;
	}

	TArray<FGuid> OrphanIds;
	for (const FShanmenItemReservationSnapshot& Reservation :
		Snapshot.Reservations)
	{
		FGuid PurposeImpactId;
		if (Reservation.State == EShanmenItemReservationState::Reserved
			&& (Reservation.ResourceKind
					== EShanmenItemResourceKind::Durability
				|| Reservation.ResourceKind
					== EShanmenItemResourceKind::Charges)
			&& Reservation.RunId == Correlation.ScopeId
			&& Reservation.OwnerId == Correlation.OwnerId
			&& IsPreparedEquipment(Correlation, Reservation.ItemInstanceId)
			&& TryParseTemporaryDefensePurpose(
				Reservation.PurposeId, PurposeImpactId))
		{
			OrphanIds.Add(Reservation.ReservationId);
		}
	}
	OrphanIds.Sort([](const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	});

	for (const FGuid& ReservationId : OrphanIds)
	{
		const FShanmenItemReservationSnapshot* Reservation =
			FindReservation(Snapshot, ReservationId);
		if (!Reservation
			|| Reservation->State != EShanmenItemReservationState::Reserved)
		{
			Result.Diagnostic =
				TEXT("An orphan defense reservation changed during bounded recovery.");
			return Result;
		}
		FShanmenItemReservationActionRequest Request;
		Request.Context.RunId = Reservation->RunId;
		Request.Context.OwnerId = Reservation->OwnerId;
		Request.Context.RequestId =
			MakeDefenseCancelRequestId(ReservationId);
		Request.Context.Content = Snapshot.Content;
		Request.ReservationId = ReservationId;
		FShanmenItemDurableCommandResult& Command =
			Result.CancellationCommands.AddDefaulted_GetRef();
		Command = Authority.CancelDurable(Request);
		if (!Command.IsCommandSuccess())
		{
			Result.Diagnostic = Command.Diagnostic.IsEmpty()
				? TEXT("An orphan defense reservation could not be cancelled durably.")
				: Command.Diagnostic;
			return Result;
		}
		++Result.CancelledReservationCount;
		if (!Authority.TryCaptureSnapshot(Snapshot))
		{
			Result.Diagnostic =
				TEXT("Authority snapshot disappeared during orphan defense recovery.");
			return Result;
		}
	}

	Result.bSuccess = true;
	Result.Diagnostic = Result.CancelledReservationCount > 0
		? TEXT("Pre-intent defense reservations were cancelled durably.")
		: TEXT("No pre-intent defense reservation requires recovery.");
	return Result;
}

Fdemo_mapShanmenDefenseResourcePreparationResult
Fdemo_mapShanmenDefenseResourceAdapter::PrepareImpactDefense(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Udemo_mapPlayerHealthComponent& VitalityHost,
	const FGuid& ImpactId,
	FShanmenDefenseSnapshot& InOutDefense)
{
	Fdemo_mapShanmenDefenseResourcePreparationResult Result;
	const FShanmenDefenseSnapshot OriginalDefense = InOutDefense;
	auto Reject = [&Result, &InOutDefense, &OriginalDefense](
		Edemo_mapShanmenDefenseResourcePreparationStatus Status,
		const FString& Diagnostic)
	{
		InOutDefense = OriginalDefense;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenDefenseResourcePreparationStatus::AuthorityNotReady,
			TEXT("Defense preparation requires the ready item authority on the Game Thread."));
	}
	if (!ImpactId.IsValid() || !VitalityHost.IsCombatEntityBound()
		|| !InOutDefense.IsValid())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourcePreparationStatus::SnapshotInvalid,
			TEXT("Defense preparation requires one valid impact, vitality host, and base snapshot."));
	}

	const Fdemo_mapShanmenDefenseResourceCoordinationResult PriorRecovery =
		RecoverPendingIntent(Authority, VitalityHost);
	if (!PriorRecovery.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourcePreparationStatus::PriorRecoveryRejected,
			PriorRecovery.Diagnostic);
	}
	const Fdemo_mapShanmenDefenseOrphanRecoveryResult OrphanRecovery =
		RecoverOrphanedDefenseReservations(Authority);
	if (!OrphanRecovery.bSuccess)
	{
		return Reject(
			Edemo_mapShanmenDefenseResourcePreparationStatus::PriorRecoveryRejected,
			OrphanRecovery.Diagnostic);
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Result.Diagnostic)
		|| !Authority.TryCaptureSnapshot(Snapshot))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourcePreparationStatus::RunCorrelationInvalid;
		return Result;
	}

	bool bRecognizedContract = false;
	bool bUnavailableContract = false;
	bool bAnyPersisted = false;
	bool bAnyReplayed = false;
	auto CancelPartial = [&Authority, &Result]()
	{
		if (Result.ReservationIds.IsEmpty())
		{
			return true;
		}
		Result.Status = Edemo_mapShanmenDefenseResourcePreparationStatus::Prepared;
		FString Diagnostic;
		return Fdemo_mapShanmenDefenseResourceAdapter::
			CancelPreparedDefenseReservation(Authority, Result, Diagnostic);
	};

	auto ReserveLayer = [
		&Authority,
		&Correlation,
		&ImpactId,
		&Snapshot,
		&Result,
		&bUnavailableContract,
		&bAnyPersisted,
		&bAnyReplayed](
			const FShanmenItemInstance& Item,
			EShanmenItemResourceKind ResourceKind,
			FName RequestNamespace,
			FString& OutDiagnostic) -> bool
	{
		FShanmenItemReserveRequest Request;
		Request.Context.RunId = Correlation.ScopeId;
		Request.Context.OwnerId = Correlation.OwnerId;
		Request.Context.RequestId = MakeDefenseReserveRequestId(
			RequestNamespace, Correlation, ImpactId, Item.ItemInstanceId);
		Request.Context.Content = Snapshot.Content;
		Request.ItemInstanceId = Item.ItemInstanceId;
		Request.ResourceKind = ResourceKind;
		Request.Amount = 1;
		Request.ExpectedItemRevision = Item.Revision;
		Request.PurposeId = MakeTemporaryDefensePurpose(ImpactId);

		const FShanmenItemProcessedRequestSnapshot* PreviousReserve =
			Snapshot.ProcessedRequests.FindByPredicate(
				[&Request](
					const FShanmenItemProcessedRequestSnapshot& Processed)
				{
					return Processed.RequestId
							== Request.Context.RequestId
						&& Processed.Receipt.IsSuccess()
						&& Processed.Receipt.Operation
							== EShanmenItemTransactionOperation::Reserve;
				});
		FGuid PreviousReservationId;
		if (PreviousReserve)
		{
			Request.ExpectedItemRevision =
				PreviousReserve->Receipt.ItemRevision;
			PreviousReservationId = PreviousReserve->Receipt.ReservationId;
			const FShanmenItemReservationSnapshot* PreviousReservation =
				FindReservation(Snapshot, PreviousReservationId);
			if (!PreviousReservation
				|| PreviousReservation->ItemInstanceId != Item.ItemInstanceId
				|| PreviousReservation->ResourceKind != ResourceKind)
			{
				OutDiagnostic =
					TEXT("A replayed defense reservation is absent or mismatched.");
				return false;
			}
			if (PreviousReservation->State
					!= EShanmenItemReservationState::Reserved
				&& !HasPreparedIntentForReservation(
					Snapshot, ImpactId, PreviousReservationId))
			{
				bUnavailableContract = true;
				return true;
			}
		}
		else
		{
			int32 ReservedAmount = 0;
			for (const FShanmenItemReservationSnapshot& Reservation :
				Snapshot.Reservations)
			{
				if (Reservation.ItemInstanceId == Item.ItemInstanceId
					&& Reservation.ResourceKind == ResourceKind
					&& Reservation.State
						== EShanmenItemReservationState::Reserved)
				{
					ReservedAmount += Reservation.Amount;
				}
			}
			const int32 Available = ResourceKind
				== EShanmenItemResourceKind::Durability
				? Item.Durability
				: Item.Charges;
			if (Available - ReservedAmount <= 0)
			{
				bUnavailableContract = true;
				return true;
			}
		}

		FShanmenItemDurableCommandResult Command =
			Authority.ReserveDurable(Request);
		if (!Command.IsCommandSuccess())
		{
			if (Command.Receipt.Error
				== EShanmenItemTransactionError::InsufficientResource)
			{
				bUnavailableContract = true;
				return true;
			}
			OutDiagnostic = Command.Diagnostic.IsEmpty()
				? TEXT("A defense resource reservation was rejected.")
				: Command.Diagnostic;
			return false;
		}
		const FGuid ReservationId = Command.Receipt.ReservationId;
		if (!ReservationId.IsValid()
			|| Command.Receipt.ItemInstanceId != Item.ItemInstanceId)
		{
			OutDiagnostic =
				TEXT("A defense reserve returned no exact reservation identity.");
			return false;
		}
		Result.ReserveRequests.Add(MoveTemp(Request));
		Result.ReserveCommands.Add(Command);
		Result.ReservationIds.Add(ReservationId);
		bAnyReplayed |= Command.Status
			== EShanmenItemDurableCommandStatus::Replayed;
		bAnyPersisted |= Command.Status
			== EShanmenItemDurableCommandStatus::Persisted;
		return true;
	};

	if (Correlation.ArmorItemInstanceId.IsValid())
	{
		const FShanmenItemInstance* Armor =
			FindItem(Snapshot, Correlation.ArmorItemInstanceId);
		if (!Armor)
		{
			return Reject(
				Edemo_mapShanmenDefenseResourcePreparationStatus::SnapshotInvalid,
				TEXT("Prepared armor identity is absent from item authority."));
		}
		if (Armor->DefinitionId == Fdemo_mapItemIds::SpiritGuardRobe)
		{
			bRecognizedContract = true;
			const FShanmenItemDefinition* AuthorityDefinition =
				FindDefinition(Snapshot, Armor->DefinitionId);
			float Reduction = 0.0f;
			if (!AuthorityDefinition
				|| !AuthorityDefinition->Supports(
					EShanmenItemResourceKind::Durability)
				|| Armor->State != EShanmenItemInstanceState::Deployed
				|| Armor->RunId != Correlation.ScopeId
				|| Armor->OwnerId != Correlation.OwnerId
				|| !Armor->DeploymentReservationId.IsValid()
				|| !TryGetSpiritGuardReduction(Reduction))
			{
				return Reject(
					Edemo_mapShanmenDefenseResourcePreparationStatus::SnapshotInvalid,
					TEXT("Spirit Guard Robe is not one exact deployed durability-capable authority item."));
			}
			if (!RemoveSpiritGuardFromAggregate(
					InOutDefense, Reduction, Result.Diagnostic))
			{
				return Reject(
					Edemo_mapShanmenDefenseResourcePreparationStatus::DefenseMismatch,
					Result.Diagnostic);
			}
			const int32 BeforeCount = Result.ReservationIds.Num();
			if (!ReserveLayer(
					*Armor,
					EShanmenItemResourceKind::Durability,
					TEXT("Shanmen.Product.SpiritGuardRobe.DurabilityReserve.r1"),
					Result.Diagnostic))
			{
				CancelPartial();
				return Reject(
					Edemo_mapShanmenDefenseResourcePreparationStatus::ReservationRejected,
					Result.Diagnostic);
			}
			if (Result.ReservationIds.Num() > BeforeCount)
			{
				AppendSpiritGuardLayer(
					InOutDefense,
					Result.ReservationIds.Last(),
					Armor->ItemInstanceId,
					Reduction);
			}
		}
	}

	if (Correlation.AccessoryItemInstanceId.IsValid())
	{
		const FShanmenItemInstance* Accessory =
			FindItem(Snapshot, Correlation.AccessoryItemInstanceId);
		if (!Accessory)
		{
			CancelPartial();
			return Reject(
				Edemo_mapShanmenDefenseResourcePreparationStatus::SnapshotInvalid,
				TEXT("Prepared accessory identity is absent from item authority."));
		}
		if (Accessory->DefinitionId
			== Fdemo_mapItemIds::HeartProtectingMirror)
		{
			bRecognizedContract = true;
			const FShanmenItemDefinition* AuthorityDefinition =
				FindDefinition(Snapshot, Accessory->DefinitionId);
			float VitalityFloor = 0.0f;
			if (!AuthorityDefinition
				|| !AuthorityDefinition->Supports(
					EShanmenItemResourceKind::Charges)
				|| Accessory->State != EShanmenItemInstanceState::Deployed
				|| Accessory->RunId != Correlation.ScopeId
				|| Accessory->OwnerId != Correlation.OwnerId
				|| !Accessory->DeploymentReservationId.IsValid()
				|| !TryGetHeartMirrorVitalityFloor(VitalityFloor))
			{
				CancelPartial();
				return Reject(
					Edemo_mapShanmenDefenseResourcePreparationStatus::SnapshotInvalid,
					TEXT("Heart-Protecting Mirror is not one exact deployed charge-capable authority item."));
			}
			const int32 BeforeCount = Result.ReservationIds.Num();
			if (!ReserveLayer(
					*Accessory,
					EShanmenItemResourceKind::Charges,
					TEXT("Shanmen.Product.HeartProtectingMirror.ChargeReserve.r1"),
					Result.Diagnostic))
			{
				CancelPartial();
				return Reject(
					Edemo_mapShanmenDefenseResourcePreparationStatus::ReservationRejected,
					Result.Diagnostic);
			}
			if (Result.ReservationIds.Num() > BeforeCount)
			{
				AppendHeartMirrorLayer(
					InOutDefense,
					Result.ReservationIds.Last(),
					Accessory->ItemInstanceId,
					VitalityFloor);
			}
		}
	}

	if (!bRecognizedContract)
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourcePreparationStatus::NotApplicable;
		Result.Diagnostic =
			TEXT("Prepared equipment has no resource-backed defense contract.");
		return Result;
	}
	if (Result.ReservationIds.IsEmpty())
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourcePreparationStatus::ResourceUnavailable;
		Result.Diagnostic = bUnavailableContract
			? TEXT("Every recognized equipment defense resource is unavailable for this impact.")
			: TEXT("No equipment defense resource layer was prepared.");
		return Result;
	}
	if (!InOutDefense.IsValid())
	{
		CancelPartial();
		return Reject(
			Edemo_mapShanmenDefenseResourcePreparationStatus::ReservationInvalid,
			TEXT("Resource-backed equipment layers failed canonical validation."));
	}
	Result.Status = bAnyReplayed && !bAnyPersisted
		? Edemo_mapShanmenDefenseResourcePreparationStatus::Replayed
		: Edemo_mapShanmenDefenseResourcePreparationStatus::Prepared;
	Result.Diagnostic = FString::Printf(
		TEXT("Prepared %d resource-backed equipment defense layer(s); unavailable contracts=%d."),
		Result.ReservationIds.Num(), bUnavailableContract ? 1 : 0);
	return Result;
}

bool Fdemo_mapShanmenDefenseResourceAdapter::CancelPreparedDefenseReservation(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenDefenseResourcePreparationResult& Preparation,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Preparation.HasResourceLayer())
	{
		return true;
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		OutDiagnostic =
			TEXT("Authority snapshot is unavailable for pre-delivery defense cancellation.");
		return false;
	}
	for (const FGuid& ReservationId : Preparation.ReservationIds)
	{
		const FShanmenItemReservationSnapshot* Reservation =
			FindReservation(Snapshot, ReservationId);
		if (!Reservation)
		{
			OutDiagnostic =
				TEXT("A prepared defense reservation disappeared before cancellation.");
			return false;
		}
		if (Reservation->State != EShanmenItemReservationState::Reserved)
		{
			continue;
		}
		FGuid PurposeImpactId;
		if (!TryParseTemporaryDefensePurpose(
				Reservation->PurposeId, PurposeImpactId))
		{
			OutDiagnostic =
				TEXT("A prepared defense reservation already belongs to the coordination saga.");
			return false;
		}
		FShanmenItemReservationActionRequest Request;
		Request.Context.RunId = Reservation->RunId;
		Request.Context.OwnerId = Reservation->OwnerId;
		Request.Context.RequestId =
			MakeDefenseCancelRequestId(Reservation->ReservationId);
		Request.Context.Content = Snapshot.Content;
		Request.ReservationId = Reservation->ReservationId;
		const FShanmenItemDurableCommandResult Command =
			Authority.CancelDurable(Request);
		if (!Command.IsCommandSuccess())
		{
			OutDiagnostic = Command.Diagnostic.IsEmpty()
				? TEXT("A prepared defense reservation could not be cancelled durably.")
				: Command.Diagnostic;
			return false;
		}
		if (!Authority.TryCaptureSnapshot(Snapshot))
		{
			OutDiagnostic =
				TEXT("Authority snapshot disappeared during bounded defense cancellation.");
			return false;
		}
	}
	OutDiagnostic = FString::Printf(
		TEXT("Cancelled %d prepared defense reservation(s) before impact delivery."),
		Preparation.ReservationIds.Num());
	return true;
}

bool Fdemo_mapShanmenDefenseResourceAdapter::EncodeVitalityIntent(
	const FShanmenVitalityCommitCommand& Command,
	FName& OutMetadata)
{
	OutMetadata = NAME_None;
	if (!Command.IsValid())
	{
		return false;
	}
	const FString Encoded = FString::Printf(
		TEXT("SMV1_%s_%s_%s_%lld_%s_%s_%s_%s_%s_%u"),
		*GuidDigits(Command.GetImpactId()),
		*GuidDigits(Command.GetResolutionId()),
		*GuidDigits(Command.GetTargetEntityId()),
		static_cast<long long>(Command.GetExpectedAuthorityRevision()),
		*FloatBits(Command.GetExpectedCurrentVitality()),
		*FloatBits(Command.GetExpectedMaximumVitality()),
		*FloatBits(Command.GetRawDamage()),
		*FloatBits(Command.GetPreventedDamage()),
		*FloatBits(Command.GetRequestedDamage()),
		static_cast<uint32>(Command.GetDefenseOutcome()));
	OutMetadata = FName(*Encoded);
	return !OutMetadata.IsNone();
}

bool Fdemo_mapShanmenDefenseResourceAdapter::DecodeVitalityIntent(
	FName Metadata,
	FShanmenVitalityCommitCommand& OutCommand)
{
	OutCommand = FShanmenVitalityCommitCommand();
	if (Metadata.IsNone())
	{
		return false;
	}
	TArray<FString> Parts;
	Metadata.ToString().ParseIntoArray(Parts, TEXT("_"), false);
	if (Parts.Num() != 11 || !Parts[0].Equals(TEXT("SMV1"), ESearchCase::IgnoreCase))
	{
		return false;
	}
	FGuid ImpactId;
	FGuid ResolutionId;
	FGuid TargetEntityId;
	int64 ExpectedRevision = INDEX_NONE;
	uint32 CurrentBits = 0;
	uint32 MaximumBits = 0;
	uint32 RawBits = 0;
	uint32 PreventedBits = 0;
	uint32 RequestedBits = 0;
	int64 OutcomeValue = 0;
	if (!FGuid::ParseExact(Parts[1], EGuidFormats::Digits, ImpactId)
		|| !FGuid::ParseExact(Parts[2], EGuidFormats::Digits, ResolutionId)
		|| !FGuid::ParseExact(Parts[3], EGuidFormats::Digits, TargetEntityId)
		|| !TryParseNonNegativeInt64(Parts[4], ExpectedRevision)
		|| !TryParseHex32(Parts[5], CurrentBits)
		|| !TryParseHex32(Parts[6], MaximumBits)
		|| !TryParseHex32(Parts[7], RawBits)
		|| !TryParseHex32(Parts[8], PreventedBits)
		|| !TryParseHex32(Parts[9], RequestedBits)
		|| !TryParseNonNegativeInt64(Parts[10], OutcomeValue)
		|| OutcomeValue <= static_cast<int32>(EShanmenDefenseOutcome::Invalid)
		|| OutcomeValue
			> static_cast<int32>(EShanmenDefenseOutcome::PerfectGuarded))
	{
		return false;
	}
	return FShanmenVitalityCommitCommand::TryRestoreFromDurableIntent(
		ImpactId,
		ResolutionId,
		TargetEntityId,
		ExpectedRevision,
		FloatFromBits(CurrentBits),
		FloatFromBits(MaximumBits),
		FloatFromBits(RawBits),
		FloatFromBits(PreventedBits),
		FloatFromBits(RequestedBits),
		static_cast<EShanmenDefenseOutcome>(OutcomeValue),
		OutCommand);
}

bool Fdemo_mapShanmenDefenseResourceAdapter::BuildIntentRequest(
	const FShanmenImpactRequest& Request,
	const FShanmenImpactResult& Impact,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenContentStamp& Content,
	FShanmenItemRunResourceIntentRequest& OutRequest,
	FShanmenVitalityCommitCommand& OutVitalityCommand,
	FString& OutDiagnostic,
	const TSet<FGuid>* ExternallyCoordinatedLayerIds)
{
	OutRequest = FShanmenItemRunResourceIntentRequest();
	OutVitalityCommand = FShanmenVitalityCommitCommand();
	if (!Request.IsValid() || !Impact.bAccepted
		|| Impact.ImpactId != Request.ImpactId || !Impact.IsConserved()
		|| !FShanmenVitalityCommitCommand::TryCreate(
			Request, Impact, OutVitalityCommand))
	{
		OutDiagnostic =
			TEXT("Defense resource coordination requires one canonical impact and vitality command.");
		return false;
	}
	if (!Correlation.IsValid() || !Content.IsValid())
	{
		OutDiagnostic =
			TEXT("Defense resource coordination requires the exact active Run correlation and content stamp.");
		return false;
	}

	TMap<FGuid, const FShanmenDefenseLayer*> ResourceInputs;
	if (ExternallyCoordinatedLayerIds)
	{
		for (const FGuid& LayerId : *ExternallyCoordinatedLayerIds)
		{
			const FShanmenDefenseLayer* External =
				Request.Defense.Layers.FindByPredicate(
					[&LayerId](const FShanmenDefenseLayer& Layer)
					{
						return Layer.LayerId == LayerId;
					});
			if (!LayerId.IsValid() || !External
				|| !External->bRequiresCommitOnTrigger
				|| IsPreparedEquipment(
					Correlation, External->SourceInstanceId))
			{
				OutDiagnostic =
					TEXT("An external defense exclusion is not a valid non-item authority layer.");
				return false;
			}
		}
	}
	for (const FShanmenDefenseLayer& Layer : Request.Defense.Layers)
	{
		if (!Layer.bRequiresCommitOnTrigger
			|| IsExternallyCoordinated(
				Layer.LayerId, ExternallyCoordinatedLayerIds))
		{
			continue;
		}
		if (!Layer.LayerId.IsValid() || !Layer.SourceInstanceId.IsValid()
			|| ResourceInputs.Contains(Layer.LayerId)
			|| !IsPreparedEquipment(Correlation, Layer.SourceInstanceId))
		{
			OutDiagnostic =
				TEXT("A resource-backed defense input has no unique reservation or exact prepared-equipment source.");
			return false;
		}
		ResourceInputs.Add(Layer.LayerId, &Layer);
	}
	if (ResourceInputs.IsEmpty())
	{
		OutDiagnostic =
			TEXT("The impact snapshot contains no resource-backed defense input.");
		return false;
	}

	TSet<FGuid> TriggeredIds;
	for (const FShanmenDefenseLayerResult& Layer : Impact.TriggeredLayers)
	{
		if (!Layer.bRequiresCommit
			|| IsExternallyCoordinated(
				Layer.LayerId, ExternallyCoordinatedLayerIds))
		{
			continue;
		}
		const FShanmenDefenseLayer* const* Input =
			ResourceInputs.Find(Layer.LayerId);
		if (!Input || !*Input || TriggeredIds.Contains(Layer.LayerId)
			|| (*Input)->SourceInstanceId != Layer.SourceInstanceId)
		{
			OutDiagnostic =
				TEXT("A triggered resource receipt does not match its frozen defense input.");
			return false;
		}
		TriggeredIds.Add(Layer.LayerId);
		FShanmenItemRunResourceCommitLine& Line =
			OutRequest.OrderedLines.AddDefaulted_GetRef();
		Line.ReservationId = Layer.LayerId;
		Line.ItemInstanceId = Layer.SourceInstanceId;
	}
	OutRequest.TriggeredLineCount = OutRequest.OrderedLines.Num();
	for (const FShanmenDefenseLayer& Layer : Request.Defense.Layers)
	{
		if (!Layer.bRequiresCommitOnTrigger
			|| IsExternallyCoordinated(
				Layer.LayerId, ExternallyCoordinatedLayerIds)
			|| TriggeredIds.Contains(Layer.LayerId))
		{
			continue;
		}
		FShanmenItemRunResourceCommitLine& Line =
			OutRequest.OrderedLines.AddDefaulted_GetRef();
		Line.ReservationId = Layer.LayerId;
		Line.ItemInstanceId = Layer.SourceInstanceId;
	}

	if (!EncodeVitalityIntent(
			OutVitalityCommand, OutRequest.IntentMetadata))
	{
		OutDiagnostic = TEXT("Canonical vitality command could not be encoded.");
		return false;
	}
	TArray<FString> RequestParts =
	{
		GuidDigits(Correlation.OwnerId),
		GuidDigits(Correlation.ScopeId),
		GuidDigits(Correlation.ActiveRunId),
		GuidDigits(Impact.ImpactId),
		GuidDigits(OutVitalityCommand.GetResolutionId()),
		FString::FromInt(OutRequest.TriggeredLineCount),
		FString::FromInt(OutRequest.OrderedLines.Num())
	};
	for (const FShanmenItemRunResourceCommitLine& Line : OutRequest.OrderedLines)
	{
		RequestParts.Add(GuidDigits(Line.ReservationId));
		RequestParts.Add(GuidDigits(Line.ItemInstanceId));
	}
	OutRequest.Context.RunId = Correlation.ScopeId;
	OutRequest.Context.OwnerId = Correlation.OwnerId;
	OutRequest.Context.RequestId =
		FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.DefenseResourceIntent.Prepare.r1"),
			RequestParts);
	OutRequest.Context.Content = Content;
	OutRequest.ActiveRunId = Correlation.ActiveRunId;
	OutRequest.IntentId = Impact.ImpactId;
	if (!OutRequest.IsValid())
	{
		OutRequest = FShanmenItemRunResourceIntentRequest();
		OutVitalityCommand = FShanmenVitalityCommitCommand();
		OutDiagnostic =
			TEXT("Defense layers could not form a canonical durable resource intent.");
		return false;
	}
	OutDiagnostic.Reset();
	return true;
}

bool Fdemo_mapShanmenDefenseResourceAdapter::BuildFinalizeRequest(
	const FShanmenItemRunResourceIntentRequest& PrepareRequest,
	bool bExternalCommitSucceeded,
	FShanmenItemRunResourceIntentFinalizeRequest& OutRequest)
{
	OutRequest = FShanmenItemRunResourceIntentFinalizeRequest();
	if (!PrepareRequest.IsValid())
	{
		return false;
	}
	OutRequest.Context = PrepareRequest.Context;
	OutRequest.Context.RequestId =
		FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.DefenseResourceIntent.Finalize.r1"),
			{
				GuidDigits(PrepareRequest.Context.RequestId),
				GuidDigits(PrepareRequest.ActiveRunId),
				GuidDigits(PrepareRequest.IntentId),
				bExternalCommitSucceeded ? TEXT("1") : TEXT("0")
			});
	OutRequest.ActiveRunId = PrepareRequest.ActiveRunId;
	OutRequest.PrepareRequestId = PrepareRequest.Context.RequestId;
	OutRequest.IntentId = PrepareRequest.IntentId;
	OutRequest.bExternalCommitSucceeded = bExternalCommitSucceeded;
	return OutRequest.IsValid();
}

Fdemo_mapShanmenDefenseResourceCoordinationResult
Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Udemo_mapPlayerHealthComponent& VitalityHost,
	const FShanmenImpactRequest& Request,
	const FShanmenImpactResult& Impact,
	const TSet<FGuid>* ExternallyCoordinatedLayerIds)
{
	Fdemo_mapShanmenDefenseResourceCoordinationResult Result;
	auto Reject = [&Result](
		Edemo_mapShanmenDefenseResourceCoordinationStatus Status,
		const FString& Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!HasResourceBackedDefense(Request, ExternallyCoordinatedLayerIds))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCoordinationStatus::NoResourceIntent;
		Result.Diagnostic = TEXT("Impact requires no item resource coordination.");
		return Result;
	}
	if (!Request.IsValid() || !Impact.bAccepted
		|| Impact.ImpactId != Request.ImpactId || !Impact.IsConserved())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::ImpactInvalid,
			TEXT("Resource coordination rejected an invalid impact receipt."));
	}
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::AuthorityNotReady,
			TEXT("Resource coordination requires the ready item authority on the Game Thread."));
	}
	const Fdemo_mapShanmenDefenseResourceCoordinationResult PriorRecovery =
		RecoverPendingIntent(Authority, VitalityHost);
	if (!PriorRecovery.IsSuccess())
	{
		return PriorRecovery;
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RunCorrelationInvalid;
		return Result;
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	FShanmenVitalityCommitCommand VitalityCommand;
	if (!Authority.TryCaptureSnapshot(Snapshot)
		|| !BuildIntentRequest(
			Request, Impact, Correlation, Snapshot.Content,
			Result.PrepareRequest, VitalityCommand, Result.Diagnostic,
			ExternallyCoordinatedLayerIds))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RequestInvalid;
		return Result;
	}
	Result.PrepareCommand =
		Authority.PreparePreparedRunResourceIntentDurable(Result.PrepareRequest);
	if (!Result.PrepareCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::PrepareRejected,
			Result.PrepareCommand.Diagnostic.IsEmpty()
				? TEXT("Durable defense resource prepare was rejected.")
				: Result.PrepareCommand.Diagnostic);
	}

	Result.VitalityCommand = VitalityHost.RecoverCombatImpact(VitalityCommand);
	if (!Result.VitalityCommand.IsSuccess())
	{
		if (Result.PrepareCommand.Status
			== EShanmenItemDurableCommandStatus::Persisted)
		{
			if (!BuildFinalizeRequest(
					Result.PrepareRequest, false, Result.FinalizeRequest))
			{
				return Reject(
					Edemo_mapShanmenDefenseResourceCoordinationStatus::RequestInvalid,
					TEXT("Failed vitality command could not form a cancellation decision."));
			}
			Result.FinalizeCommand =
				Authority.FinalizePreparedRunResourceIntentDurable(
					Result.FinalizeRequest);
			if (!Result.FinalizeCommand.IsCommandSuccess())
			{
				return Reject(
					Edemo_mapShanmenDefenseResourceCoordinationStatus::FinalizeInDoubt,
					TEXT("Vitality rejected, but resource cancellation is not yet durable."));
			}
			return Reject(
				Edemo_mapShanmenDefenseResourceCoordinationStatus::VitalityRejected,
				TEXT("Vitality rejected without mutation; every resource line was cancelled."));
		}
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RecoveryAmbiguous,
			TEXT("Replayed resource prepare cannot decide an external vitality state that is neither exact before nor exact after."));
	}

	if (!BuildFinalizeRequest(
			Result.PrepareRequest, true, Result.FinalizeRequest))
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::FinalizeInDoubt,
			TEXT("Committed vitality could not form the durable resource decision."));
	}
	Result.FinalizeCommand =
		Authority.FinalizePreparedRunResourceIntentDurable(
			Result.FinalizeRequest);
	if (!Result.FinalizeCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::FinalizeInDoubt,
			Result.FinalizeCommand.Diagnostic.IsEmpty()
				? TEXT("Vitality committed, but the resource decision remains recoverable and pending.")
				: Result.FinalizeCommand.Diagnostic);
	}
	Result.Status = Result.PrepareCommand.Status
		== EShanmenItemDurableCommandStatus::Persisted
		? Edemo_mapShanmenDefenseResourceCoordinationStatus::Coordinated
		: Edemo_mapShanmenDefenseResourceCoordinationStatus::Recovered;
	Result.Diagnostic =
		TEXT("Vitality and triggered/cancelled defense resources reached one recoverable terminal decision.");
	return Result;
}

Fdemo_mapShanmenDefenseResourceCoordinationResult
Fdemo_mapShanmenDefenseResourceAdapter::RecoverPendingIntent(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Udemo_mapPlayerHealthComponent& VitalityHost)
{
	Fdemo_mapShanmenDefenseResourceCoordinationResult Result;
	auto Reject = [&Result](
		Edemo_mapShanmenDefenseResourceCoordinationStatus Status,
		const FString& Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::AuthorityNotReady,
			TEXT("Pending resource recovery requires the ready item authority on the Game Thread."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Result.Diagnostic)
		|| !Authority.TryCaptureSnapshot(Snapshot))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RunCorrelationInvalid;
		return Result;
	}

	TSet<FGuid> FinalizedPrepareIds;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		if (Processed.Receipt.IsSuccess()
			&& Processed.Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRunResourceIntent)
		{
			FinalizedPrepareIds.Add(Processed.Receipt.ItemInstanceId);
		}
	}
	const FShanmenItemTransactionReceipt* Pending = nullptr;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Snapshot.ProcessedRequests)
	{
		const FShanmenItemTransactionReceipt& Receipt = Processed.Receipt;
		if (!Receipt.IsSuccess()
			|| Receipt.Operation
				!= EShanmenItemTransactionOperation::PreparePreparedRunResourceIntent
			|| FinalizedPrepareIds.Contains(Receipt.RequestId))
		{
			continue;
		}
		if (Pending)
		{
			return Reject(
				Edemo_mapShanmenDefenseResourceCoordinationStatus::RecoveryAmbiguous,
				TEXT("Item authority exposed more than one pending defense resource intent."));
		}
		Pending = &Receipt;
	}
	if (!Pending)
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCoordinationStatus::NoResourceIntent;
		Result.Diagnostic = TEXT("No pending defense resource intent exists.");
		return Result;
	}
	if (Pending->ItemInstanceId != Correlation.ActiveRunId
		|| Pending->ReservationId.IsValid() == false
		|| !VitalityHost.IsCombatEntityBound())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RecoveryAmbiguous,
			TEXT("Pending intent does not belong to the exact active Run and vitality host."));
	}

	Result.PrepareRequest.Context.RunId = Correlation.ScopeId;
	Result.PrepareRequest.Context.OwnerId = Correlation.OwnerId;
	Result.PrepareRequest.Context.RequestId = Pending->RequestId;
	Result.PrepareRequest.Context.Content = Snapshot.Content;
	Result.PrepareRequest.ActiveRunId = Pending->ItemInstanceId;
	Result.PrepareRequest.IntentId = Pending->ReservationId;
	Result.PrepareRequest.TriggeredLineCount = Pending->Amount;
	Result.PrepareRequest.IntentMetadata = Pending->PurposeId;
	for (const FGuid& ReservationId : Pending->ReservationIds)
	{
		const FShanmenItemReservationSnapshot* Reservation =
			Snapshot.Reservations.FindByPredicate(
				[&ReservationId](const FShanmenItemReservationSnapshot& Candidate)
				{
					return Candidate.ReservationId == ReservationId;
				});
		if (!Reservation
			|| Reservation->State != EShanmenItemReservationState::Reserved
			|| Reservation->PurposeId != Pending->PurposeId
			|| !IsPreparedEquipment(
				Correlation, Reservation->ItemInstanceId))
		{
			return Reject(
				Edemo_mapShanmenDefenseResourceCoordinationStatus::RecoveryAmbiguous,
				TEXT("Pending intent reservation no longer matches its exact prepared equipment."));
		}
		FShanmenItemRunResourceCommitLine& Line =
			Result.PrepareRequest.OrderedLines.AddDefaulted_GetRef();
		Line.ReservationId = ReservationId;
		Line.ItemInstanceId = Reservation->ItemInstanceId;
	}

	FShanmenVitalityCommitCommand VitalityCommand;
	if (!Result.PrepareRequest.IsValid()
		|| !DecodeVitalityIntent(Pending->PurposeId, VitalityCommand)
		|| VitalityCommand.GetImpactId() != Pending->ReservationId
		|| VitalityCommand.GetTargetEntityId()
			!= VitalityHost.GetCombatEntityId())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RecoveryAmbiguous,
			TEXT("Pending intent metadata cannot reconstruct the exact vitality command."));
	}

	Result.VitalityCommand = VitalityHost.RecoverCombatImpact(VitalityCommand);
	if (!Result.VitalityCommand.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::RecoveryAmbiguous,
			TEXT("Current vitality is neither the durable intent's exact before nor exact after state."));
	}
	if (!BuildFinalizeRequest(
			Result.PrepareRequest, true, Result.FinalizeRequest))
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::FinalizeInDoubt,
			TEXT("Recovered vitality could not form a resource finalization command."));
	}
	Result.FinalizeCommand =
		Authority.FinalizePreparedRunResourceIntentDurable(
			Result.FinalizeRequest);
	if (!Result.FinalizeCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCoordinationStatus::FinalizeInDoubt,
			Result.FinalizeCommand.Diagnostic.IsEmpty()
				? TEXT("Recovered vitality is safe, but the resource finalization remains pending.")
				: Result.FinalizeCommand.Diagnostic);
	}
	Result.Status =
		Edemo_mapShanmenDefenseResourceCoordinationStatus::Recovered;
	Result.Diagnostic =
		TEXT("Pending vitality/resource intent recovered to one durable terminal decision.");
	return Result;
}
