#include "ShanmenSpiritShieldCapacityAuthority.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	float CanonicalZero(float Value)
	{
		return Value == 0.0f ? 0.0f : Value;
	}

	uint32 FloatValueBits(float Value)
	{
		Value = CanonicalZero(Value);
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return Bits;
	}

	FString FloatBits(float Value)
	{
		return FString::Printf(TEXT("%08X"), FloatValueBits(Value));
	}

	bool FloatsMatchExactly(float Left, float Right)
	{
		return FloatValueBits(Left) == FloatValueBits(Right);
	}

	void AppendSortedTags(
		const FGameplayTagContainer& Tags,
		TArray<FString>& InOutParts)
	{
		TArray<FGameplayTag> SortedTags;
		Tags.GetGameplayTagArray(SortedTags);
		SortedTags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		InOutParts.Add(FString::FromInt(SortedTags.Num()));
		for (const FGameplayTag& Tag : SortedTags)
		{
			InOutParts.Add(Tag.ToString());
		}
	}

	bool DefenseLayersMatch(
		const FShanmenDefenseLayer& Left,
		const FShanmenDefenseLayer& Right)
	{
		return Left.LayerId == Right.LayerId
			&& Left.RuleId == Right.RuleId
			&& Left.SourceInstanceId == Right.SourceInstanceId
			&& Left.Operation == Right.Operation
			&& Left.Order == Right.Order
			&& FloatsMatchExactly(Left.Magnitude, Right.Magnitude)
			&& Left.bRequiresCommitOnTrigger == Right.bRequiresCommitOnTrigger
			&& Left.LayerTags == Right.LayerTags
			&& Left.RequiredDamageTags == Right.RequiredDamageTags
			&& Left.BlockedDamageTags == Right.BlockedDamageTags
			&& Left.RequiredSourceTags == Right.RequiredSourceTags
			&& Left.BlockedSourceTags == Right.BlockedSourceTags
			&& Left.RequiredTargetTags == Right.RequiredTargetTags
			&& Left.BlockedTargetTags == Right.BlockedTargetTags;
	}

	bool LayerResultMatches(
		const FShanmenDefenseLayerResult& Left,
		const FShanmenDefenseLayerResult& Right)
	{
		return Left.LayerId == Right.LayerId
			&& Left.RuleId == Right.RuleId
			&& Left.SourceInstanceId == Right.SourceInstanceId
			&& Left.Operation == Right.Operation
			&& Left.Order == Right.Order
			&& FloatsMatchExactly(Left.PreventedDamage, Right.PreventedDamage)
			&& Left.bRequiresCommit == Right.bRequiresCommit
			&& Left.LayerTags == Right.LayerTags;
	}

	bool ResultsMatch(
		const FShanmenImpactResult& Left,
		const FShanmenImpactResult& Right)
	{
		if (Left.bAccepted != Right.bAccepted
			|| Left.ImpactId != Right.ImpactId
			|| Left.Outcome != Right.Outcome
			|| !FloatsMatchExactly(Left.RawDamage, Right.RawDamage)
			|| !FloatsMatchExactly(Left.PreventedDamage, Right.PreventedDamage)
			|| !FloatsMatchExactly(Left.FinalDamage, Right.FinalDamage)
			|| Left.TriggeredLayers.Num() != Right.TriggeredLayers.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.TriggeredLayers.Num(); ++Index)
		{
			if (!LayerResultMatches(
					Left.TriggeredLayers[Index], Right.TriggeredLayers[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool TriggeredLayerMatchesProjection(
		const FShanmenDefenseLayerResult& Result,
		const FShanmenDefenseLayer& ProjectionLayer)
	{
		return Result.LayerId == ProjectionLayer.LayerId
			&& Result.RuleId == ProjectionLayer.RuleId
			&& Result.SourceInstanceId == ProjectionLayer.SourceInstanceId
			&& Result.Operation == ProjectionLayer.Operation
			&& Result.Order == ProjectionLayer.Order
			&& Result.bRequiresCommit
			&& ProjectionLayer.bRequiresCommitOnTrigger
			&& Result.LayerTags == ProjectionLayer.LayerTags
			&& FMath::IsFinite(Result.PreventedDamage)
			&& Result.PreventedDamage > 0.0f
			&& Result.PreventedDamage <= ProjectionLayer.Magnitude;
	}

	FGuid MakeResolutionId(
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Result)
	{
		TArray<FString> Parts{
			GuidDigits(Request.ImpactId),
			GuidDigits(Request.Action.GetActivationId()),
			GuidDigits(Request.Candidate.SourceEntityId),
			GuidDigits(Request.Candidate.TargetEntityId),
			Request.Candidate.DetectorId.ToString(),
			FString::FromInt(Request.Candidate.HitOrdinal),
			Request.Damage.FormulaId.ToString(),
			FloatBits(Request.Damage.RawDamage),
			FString::Printf(TEXT("%lld"), Request.TargetVitality.AuthorityRevision),
			FloatBits(Request.TargetVitality.CurrentVitality),
			FloatBits(Request.TargetVitality.MaximumVitality),
			FString::FromInt(static_cast<uint8>(Result.Outcome)),
			FloatBits(Result.RawDamage),
			FloatBits(Result.PreventedDamage),
			FloatBits(Result.FinalDamage),
			FString::FromInt(Result.TriggeredLayers.Num())
		};
		AppendSortedTags(Request.Damage.DamageTags, Parts);
		for (const FShanmenDefenseLayerResult& Layer : Result.TriggeredLayers)
		{
			Parts.Add(GuidDigits(Layer.LayerId));
			Parts.Add(Layer.RuleId.ToString());
			Parts.Add(GuidDigits(Layer.SourceInstanceId));
			Parts.Add(FString::FromInt(static_cast<uint8>(Layer.Operation)));
			Parts.Add(FString::FromInt(Layer.Order));
			Parts.Add(FloatBits(Layer.PreventedDamage));
			Parts.Add(Layer.bRequiresCommit ? TEXT("1") : TEXT("0"));
			AppendSortedTags(Layer.LayerTags, Parts);
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.CapacityResolution.r1"), Parts);
	}

	FGuid MakeCommandId(
		const FGuid& ResolutionId,
		const FGuid& ImpactId,
		const FShanmenSpiritShieldProjectionReceipt& Projection,
		float RequestedCapacity)
	{
		if (!ResolutionId.IsValid() || !ImpactId.IsValid()
			|| !Projection.IsValid()
			|| !FMath::IsFinite(RequestedCapacity)
			|| RequestedCapacity <= 0.0f)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.CapacityCommand.r1"),
			{
				GuidDigits(ResolutionId),
				GuidDigits(ImpactId),
				GuidDigits(Projection.GetProjectionId()),
				GuidDigits(Projection.GetActivation().GetShieldInstanceId()),
				GuidDigits(Projection.GetLayer().LayerId),
				FString::Printf(TEXT("%lld"), Projection.GetAuthorityRevision()),
				FloatBits(Projection.GetAvailableCapacity()),
				FloatBits(RequestedCapacity)
			});
	}

	FGuid MakeReceiptId(
		const FGuid& CommandId,
		const FGuid& ProjectionId,
		const FGuid& ShieldInstanceId,
		int64 RevisionBefore,
		int64 RevisionAfter,
		float CapacityBefore,
		float CommittedCapacity,
		float CapacityAfter)
	{
		if (!CommandId.IsValid() || !ProjectionId.IsValid()
			|| !ShieldInstanceId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.CapacityReceipt.r1"),
			{
				GuidDigits(CommandId),
				GuidDigits(ProjectionId),
				GuidDigits(ShieldInstanceId),
				FString::Printf(TEXT("%lld"), RevisionBefore),
				FString::Printf(TEXT("%lld"), RevisionAfter),
				FloatBits(CapacityBefore),
				FloatBits(CommittedCapacity),
				FloatBits(CapacityAfter)
			});
	}
}

bool FShanmenSpiritShieldCapacityCommitCommand::TryCreate(
	const FShanmenSpiritShieldProjectionReceipt& InProjection,
	const FShanmenImpactRequest& Request,
	const FShanmenImpactResult& Result,
	FShanmenSpiritShieldCapacityCommitCommand& OutCommand)
{
	OutCommand = FShanmenSpiritShieldCapacityCommitCommand();
	if (!InProjection.IsValid()
		|| !Request.IsValid()
		|| !Result.bAccepted
		|| Result.ImpactId != Request.ImpactId
		|| !Result.IsConserved()
		|| Request.Candidate.TargetEntityId
			!= InProjection.GetActivation().GetAction().GetSourceEntityId()
		|| !ResultsMatch(FShanmenDefenseResolver::Resolve(Request), Result))
	{
		return false;
	}

	const FShanmenDefenseLayer& ProjectedLayer = InProjection.GetLayer();
	int32 RequestSourceCount = 0;
	int32 ExactRequestLayerCount = 0;
	for (const FShanmenDefenseLayer& Layer : Request.Defense.Layers)
	{
		if (Layer.SourceInstanceId == ProjectedLayer.SourceInstanceId)
		{
			++RequestSourceCount;
			if (DefenseLayersMatch(Layer, ProjectedLayer))
			{
				++ExactRequestLayerCount;
			}
		}
	}

	int32 ResultSourceCount = 0;
	const FShanmenDefenseLayerResult* TriggeredShield = nullptr;
	for (const FShanmenDefenseLayerResult& Layer : Result.TriggeredLayers)
	{
		if (Layer.SourceInstanceId == ProjectedLayer.SourceInstanceId)
		{
			++ResultSourceCount;
			if (TriggeredLayerMatchesProjection(Layer, ProjectedLayer))
			{
				if (TriggeredShield != nullptr)
				{
					return false;
				}
				TriggeredShield = &Layer;
			}
		}
	}

	if (RequestSourceCount != 1
		|| ExactRequestLayerCount != 1
		|| ResultSourceCount != 1
		|| TriggeredShield == nullptr)
	{
		return false;
	}

	OutCommand.ResolutionId = MakeResolutionId(Request, Result);
	OutCommand.ImpactId = Result.ImpactId;
	OutCommand.Projection = InProjection;
	OutCommand.RequestedCapacity = CanonicalZero(
		TriggeredShield->PreventedDamage);
	OutCommand.CommandId = MakeCommandId(
		OutCommand.ResolutionId,
		OutCommand.ImpactId,
		OutCommand.Projection,
		OutCommand.RequestedCapacity);
	if (!OutCommand.IsValid())
	{
		OutCommand = FShanmenSpiritShieldCapacityCommitCommand();
		return false;
	}
	return true;
}

bool FShanmenSpiritShieldCapacityCommitCommand::IsValid() const
{
	return ResolutionId.IsValid()
		&& ImpactId.IsValid()
		&& Projection.IsValid()
		&& FMath::IsFinite(RequestedCapacity)
		&& RequestedCapacity > 0.0f
		&& RequestedCapacity <= Projection.GetAvailableCapacity()
		&& CommandId == MakeCommandId(
			ResolutionId, ImpactId, Projection, RequestedCapacity);
}

bool FShanmenSpiritShieldCapacityCommitReceipt::IsValid() const
{
	if (!ReceiptId.IsValid()
		|| !CommandId.IsValid()
		|| !ImpactId.IsValid()
		|| !ProjectionId.IsValid()
		|| !ShieldInstanceId.IsValid()
		|| AuthorityRevisionBefore < 0
		|| AuthorityRevisionBefore == MAX_int64
		|| AuthorityRevisionAfter != AuthorityRevisionBefore + 1
		|| !FMath::IsFinite(CapacityBefore)
		|| !FMath::IsFinite(CommittedCapacity)
		|| !FMath::IsFinite(CapacityAfter)
		|| CapacityBefore <= 0.0f
		|| CommittedCapacity <= 0.0f
		|| CommittedCapacity > CapacityBefore
		|| CapacityAfter < 0.0f
		|| CapacityAfter >= CapacityBefore
		|| !FloatsMatchExactly(
			CapacityAfter,
			CanonicalZero(FMath::Max(
				0.0f, CapacityBefore - CommittedCapacity))))
	{
		return false;
	}

	return ReceiptId == MakeReceiptId(
		CommandId,
		ProjectionId,
		ShieldInstanceId,
		AuthorityRevisionBefore,
		AuthorityRevisionAfter,
		CapacityBefore,
		CommittedCapacity,
		CapacityAfter);
}

bool FShanmenSpiritShieldCapacityCommitResult::IsValid() const
{
	switch (Status)
	{
	case EShanmenSpiritShieldCapacityCommitStatus::Committed:
	case EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted:
		return Error == EShanmenSpiritShieldCapacityCommitError::None
			&& Receipt.IsValid();
	case EShanmenSpiritShieldCapacityCommitStatus::Rejected:
		return Error != EShanmenSpiritShieldCapacityCommitError::None
			&& !Receipt.IsValid();
	default:
		return false;
	}
}

bool FShanmenSpiritShieldCapacityCommitResult::IsSuccess() const
{
	return IsValid()
		&& (Status == EShanmenSpiritShieldCapacityCommitStatus::Committed
			|| Status
				== EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted);
}

bool FShanmenSpiritShieldCapacityAuthority::TryCreate(
	const FShanmenSpiritShieldActivationReceipt& InActivation,
	FShanmenSpiritShieldCapacityAuthority& OutAuthority)
{
	OutAuthority.Reset();
	if (!InActivation.IsValid())
	{
		return false;
	}

	OutAuthority.Activation = InActivation;
	OutAuthority.AvailableCapacity = CanonicalZero(
		InActivation.GetDefinition().GetMaximumCapacity());
	OutAuthority.AuthorityRevision = 0;
	OutAuthority.bInitialized = true;
	return OutAuthority.IsValid();
}

bool FShanmenSpiritShieldCapacityAuthority::IsValid() const
{
	if (!bInitialized
		|| !Activation.IsValid()
		|| AuthorityRevision < 0
		|| !FMath::IsFinite(AvailableCapacity)
		|| AvailableCapacity < 0.0f
		|| AvailableCapacity > Activation.GetDefinition().GetMaximumCapacity()
		|| AuthorityRevision != ProcessedImpacts.Num())
	{
		return false;
	}

	for (const TPair<FGuid, FProcessedImpact>& Entry : ProcessedImpacts)
	{
		if (Entry.Key != Entry.Value.Receipt.GetImpactId()
			|| Entry.Value.CommandId != Entry.Value.Receipt.GetCommandId()
			|| Entry.Value.Receipt.GetShieldInstanceId()
				!= Activation.GetShieldInstanceId()
			|| !Entry.Value.Receipt.IsValid())
		{
			return false;
		}
	}
	return true;
}

bool FShanmenSpiritShieldCapacityAuthority::TryProjectDefenseLayer(
	FShanmenSpiritShieldRuntime& ShieldRuntime,
	FShanmenSpiritShieldProjectionReceipt& OutProjection) const
{
	OutProjection = FShanmenSpiritShieldProjectionReceipt();
	if (!IsValid()
		|| IsDepleted()
		|| !ShieldRuntime.IsValid()
		|| ShieldRuntime.GetState() != EShanmenSpiritShieldState::Active
		|| ShieldRuntime.GetActivationReceipt().GetReceiptId()
			!= Activation.GetReceiptId())
	{
		return false;
	}

	if (!ShieldRuntime.TryProjectDefenseLayer(
			AuthorityRevision, AvailableCapacity, OutProjection))
	{
		return false;
	}
	return OutProjection.IsValid()
		&& OutProjection.GetActivation().GetReceiptId()
			== Activation.GetReceiptId()
		&& OutProjection.GetAuthorityRevision() == AuthorityRevision
		&& FloatsMatchExactly(
			OutProjection.GetAvailableCapacity(), AvailableCapacity);
}

FShanmenSpiritShieldCapacityCommitResult
FShanmenSpiritShieldCapacityAuthority::Commit(
	const FShanmenSpiritShieldCapacityCommitCommand& Command)
{
	if (!Command.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::InvalidCommand);
	}
	if (!IsValid())
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::AuthorityNotReady);
	}

	if (const FProcessedImpact* Existing =
		ProcessedImpacts.Find(Command.GetImpactId()))
	{
		if (Existing->CommandId != Command.GetCommandId())
		{
			return Reject(
				EShanmenSpiritShieldCapacityCommitError::ImpactConflict);
		}

		FShanmenSpiritShieldCapacityCommitResult Result;
		Result.Status =
			EShanmenSpiritShieldCapacityCommitStatus::AlreadyCommitted;
		Result.Receipt = Existing->Receipt;
		return Result;
	}

	const FShanmenSpiritShieldProjectionReceipt& Projection =
		Command.GetProjection();
	if (Projection.GetActivation().GetReceiptId()
			!= Activation.GetReceiptId()
		|| Projection.GetActivation().GetShieldInstanceId()
			!= Activation.GetShieldInstanceId())
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::ShieldMismatch);
	}
	if (Projection.GetAuthorityRevision() != AuthorityRevision
		|| !FloatsMatchExactly(
			Projection.GetAvailableCapacity(), AvailableCapacity))
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::StaleProjection);
	}
	if (Command.GetRequestedCapacity() > AvailableCapacity)
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::CapacityExceeded);
	}
	if (AuthorityRevision == MAX_int64)
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::RevisionExhausted);
	}

	FShanmenSpiritShieldCapacityCommitReceipt Receipt;
	Receipt.CommandId = Command.GetCommandId();
	Receipt.ImpactId = Command.GetImpactId();
	Receipt.ProjectionId = Projection.GetProjectionId();
	Receipt.ShieldInstanceId = Activation.GetShieldInstanceId();
	Receipt.AuthorityRevisionBefore = AuthorityRevision;
	Receipt.AuthorityRevisionAfter = AuthorityRevision + 1;
	Receipt.CapacityBefore = AvailableCapacity;
	Receipt.CommittedCapacity = CanonicalZero(
		Command.GetRequestedCapacity());
	Receipt.CapacityAfter = CanonicalZero(FMath::Max(
		0.0f, AvailableCapacity - Receipt.CommittedCapacity));
	Receipt.ReceiptId = MakeReceiptId(
		Receipt.CommandId,
		Receipt.ProjectionId,
		Receipt.ShieldInstanceId,
		Receipt.AuthorityRevisionBefore,
		Receipt.AuthorityRevisionAfter,
		Receipt.CapacityBefore,
		Receipt.CommittedCapacity,
		Receipt.CapacityAfter);
	if (!Receipt.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldCapacityCommitError::InvalidCommand);
	}

	AvailableCapacity = Receipt.CapacityAfter;
	AuthorityRevision = Receipt.AuthorityRevisionAfter;
	FProcessedImpact& Processed = ProcessedImpacts.Add(Receipt.ImpactId);
	Processed.CommandId = Receipt.CommandId;
	Processed.Receipt = Receipt;

	FShanmenSpiritShieldCapacityCommitResult Result;
	Result.Status = EShanmenSpiritShieldCapacityCommitStatus::Committed;
	Result.Receipt = Receipt;
	return Result;
}

void FShanmenSpiritShieldCapacityAuthority::Reset()
{
	*this = FShanmenSpiritShieldCapacityAuthority();
}

FShanmenSpiritShieldCapacityCommitResult
FShanmenSpiritShieldCapacityAuthority::Reject(
	EShanmenSpiritShieldCapacityCommitError Error) const
{
	FShanmenSpiritShieldCapacityCommitResult Result;
	Result.Status = EShanmenSpiritShieldCapacityCommitStatus::Rejected;
	Result.Error = Error;
	return Result;
}
