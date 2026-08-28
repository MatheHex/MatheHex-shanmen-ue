#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "ShanmenDeterministicId.h"
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

	bool HasResourceBackedDefense(const FShanmenImpactRequest& Request)
	{
		return Request.Defense.Layers.ContainsByPredicate(
			[](const FShanmenDefenseLayer& Layer)
			{
				return Layer.bRequiresCommitOnTrigger;
			});
	}
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
	FString& OutDiagnostic)
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
	for (const FShanmenDefenseLayer& Layer : Request.Defense.Layers)
	{
		if (!Layer.bRequiresCommitOnTrigger)
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
		if (!Layer.bRequiresCommit)
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
	const FShanmenImpactResult& Impact)
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
	if (!HasResourceBackedDefense(Request))
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
			Result.PrepareRequest, VitalityCommand, Result.Diagnostic))
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
