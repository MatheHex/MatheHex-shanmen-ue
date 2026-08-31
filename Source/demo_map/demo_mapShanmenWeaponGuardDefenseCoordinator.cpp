#include "demo_mapShanmenWeaponGuardDefenseCoordinator.h"

#include "ShanmenDeterministicId.h"

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

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const TCHAR* Label,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		Parts.Add(Label);
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
	}

	bool LayersMatch(
		const FShanmenDefenseLayer& Left,
		const FShanmenDefenseLayer& Right)
	{
		return Left.LayerId == Right.LayerId
			&& Left.RuleId == Right.RuleId
			&& Left.SourceInstanceId == Right.SourceInstanceId
			&& Left.Operation == Right.Operation
			&& Left.Order == Right.Order
			&& FloatValueBits(Left.Magnitude)
				== FloatValueBits(Right.Magnitude)
			&& Left.bRequiresCommitOnTrigger
				== Right.bRequiresCommitOnTrigger
			&& Left.LayerTags == Right.LayerTags
			&& Left.RequiredDamageTags == Right.RequiredDamageTags
			&& Left.BlockedDamageTags == Right.BlockedDamageTags
			&& Left.RequiredSourceTags == Right.RequiredSourceTags
			&& Left.BlockedSourceTags == Right.BlockedSourceTags
			&& Left.RequiredTargetTags == Right.RequiredTargetTags
			&& Left.BlockedTargetTags == Right.BlockedTargetTags;
	}

	bool SnapshotsMatch(
		const FShanmenDefenseSnapshot& Left,
		const FShanmenDefenseSnapshot& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.TargetTags != Right.TargetTags
			|| Left.Layers.Num() != Right.Layers.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Layers.Num(); ++Index)
		{
			if (!LayersMatch(Left.Layers[Index], Right.Layers[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool ContainsLayerId(
		const FShanmenDefenseSnapshot& Defense,
		const FGuid& LayerId)
	{
		return LayerId.IsValid()
			&& Defense.Layers.ContainsByPredicate(
				[&LayerId](const FShanmenDefenseLayer& Layer)
				{
					return Layer.LayerId == LayerId;
				});
	}

	bool IsAppendOf(
		const FShanmenDefenseSnapshot& Base,
		const FShanmenDefenseSnapshot& Composed,
		const FShanmenDefenseLayer& AppendedLayer)
	{
		if (!Base.IsValid() || !Composed.IsValid()
			|| !AppendedLayer.IsValid()
			|| Base.TargetTags != Composed.TargetTags
			|| Composed.Layers.Num() != Base.Layers.Num() + 1
			|| ContainsLayerId(Base, AppendedLayer.LayerId))
		{
			return false;
		}
		for (int32 Index = 0; Index < Base.Layers.Num(); ++Index)
		{
			if (!LayersMatch(Base.Layers[Index], Composed.Layers[Index]))
			{
				return false;
			}
		}
		return LayersMatch(Composed.Layers.Last(), AppendedLayer);
	}

	void AppendLayerParts(
		TArray<FString>& Parts,
		int32 Index,
		const FShanmenDefenseLayer& Layer)
	{
		Parts.Add(FString::Printf(TEXT("Layer.%d"), Index));
		Parts.Add(GuidDigits(Layer.LayerId));
		Parts.Add(Layer.RuleId.ToString());
		Parts.Add(GuidDigits(Layer.SourceInstanceId));
		Parts.Add(FString::FromInt(static_cast<int32>(Layer.Operation)));
		Parts.Add(FString::FromInt(Layer.Order));
		Parts.Add(FloatBits(Layer.Magnitude));
		Parts.Add(Layer.bRequiresCommitOnTrigger ? TEXT("1") : TEXT("0"));
		AppendCanonicalTags(Parts, TEXT("LayerTags"), Layer.LayerTags);
		AppendCanonicalTags(
			Parts, TEXT("RequiredDamageTags"), Layer.RequiredDamageTags);
		AppendCanonicalTags(
			Parts, TEXT("BlockedDamageTags"), Layer.BlockedDamageTags);
		AppendCanonicalTags(
			Parts, TEXT("RequiredSourceTags"), Layer.RequiredSourceTags);
		AppendCanonicalTags(
			Parts, TEXT("BlockedSourceTags"), Layer.BlockedSourceTags);
		AppendCanonicalTags(
			Parts, TEXT("RequiredTargetTags"), Layer.RequiredTargetTags);
		AppendCanonicalTags(
			Parts, TEXT("BlockedTargetTags"), Layer.BlockedTargetTags);
	}

	FGuid MakeSnapshotDigest(const FShanmenDefenseSnapshot& Defense)
	{
		if (!Defense.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts;
		AppendCanonicalTags(Parts, TEXT("TargetTags"), Defense.TargetTags);
		Parts.Add(TEXT("LayerCount"));
		Parts.Add(FString::FromInt(Defense.Layers.Num()));
		for (int32 Index = 0; Index < Defense.Layers.Num(); ++Index)
		{
			AppendLayerParts(Parts, Index, Defense.Layers[Index]);
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Sword.WeaponGuard.DefenseSnapshotEvidence.r1"),
			Parts);
	}

	bool IsComposedStatus(Edemo_mapShanmenWeaponGuardDefenseStatus Status)
	{
		return Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedQualified
			|| Status
				== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedOutsideArc;
	}

	FGuid MakeReceiptId(
		Edemo_mapShanmenWeaponGuardDefenseStatus Status,
		const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection,
		const Fdemo_mapShanmenWeaponGuardWorldResult& WorldEvaluation,
		const FShanmenDefenseSnapshot& BaseDefense,
		const FShanmenDefenseSnapshot& Defense)
	{
		const FGuid BaseDigest = MakeSnapshotDigest(BaseDefense);
		const FGuid DefenseDigest = MakeSnapshotDigest(Defense);
		if (!IsComposedStatus(Status)
			|| !TimingProjection.IsValid()
			|| !WorldEvaluation.IsSuccess()
			|| !BaseDigest.IsValid()
			|| !DefenseDigest.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Sword.WeaponGuard.DefenseCoordinator.r1"),
			{
				FString::FromInt(static_cast<int32>(Status)),
				GuidDigits(TimingProjection.GetReceiptId()),
				GuidDigits(WorldEvaluation.ReceiptId),
				GuidDigits(BaseDigest),
				GuidDigits(DefenseDigest)
			});
	}

	Fdemo_mapShanmenWeaponGuardDefenseResult Reject(
		Edemo_mapShanmenWeaponGuardDefenseStatus Status,
		const TCHAR* Diagnostic,
		const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection =
			FShanmenWeaponGuardTimingProjectionReceipt(),
		const Fdemo_mapShanmenWeaponGuardWorldResult& WorldEvaluation =
			Fdemo_mapShanmenWeaponGuardWorldResult())
	{
		Fdemo_mapShanmenWeaponGuardDefenseResult Result;
		Result.Status = Status;
		Result.TimingProjection = TimingProjection;
		Result.WorldEvaluation = WorldEvaluation;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenWeaponGuardDefenseResult::IsSuccess() const
{
	if (!IsComposedStatus(Status)
		|| !ReceiptId.IsValid()
		|| !TimingProjection.IsValid()
		|| !WorldEvaluation.IsSuccess()
		|| !BaseDefense.IsValid()
		|| !Defense.IsValid()
		|| WorldEvaluation.Evaluation.GetTimingProjection().GetReceiptId()
			!= TimingProjection.GetReceiptId())
	{
		return false;
	}

	if (Status
		== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedQualified)
	{
		if (!WorldEvaluation.IsQualified()
			|| !WorldEvaluation.Evaluation.HasLayer()
			|| !IsAppendOf(
				BaseDefense,
				Defense,
				WorldEvaluation.Evaluation.GetLayer()))
		{
			return false;
		}
	}
	else if (WorldEvaluation.IsQualified()
		|| WorldEvaluation.Evaluation.HasLayer()
		|| !SnapshotsMatch(BaseDefense, Defense))
	{
		return false;
	}

	return ReceiptId == MakeReceiptId(
		Status,
		TimingProjection,
		WorldEvaluation,
		BaseDefense,
		Defense);
}

Fdemo_mapShanmenWeaponGuardDefenseResult
Fdemo_mapShanmenWeaponGuardDefenseCoordinator::Compose(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* DefenderActor,
	AActor* ThreatActor,
	const FShanmenWeaponGuardWindow& Window,
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenWeaponPerfectGuardPolicy& TimingPolicy,
	const FShanmenWeaponGuardTimelineObservation& Observation,
	const FShanmenWeaponGuardArcPolicy& ArcPolicy,
	const FShanmenHitCandidate& Candidate,
	const FShanmenDefenseSnapshot& BaseDefense)
{
	if (!Window.IsValid()
		|| !ActionRuntime.IsValid()
		|| !TimingPolicy.IsValid()
		|| !Observation.IsValid()
		|| !ArcPolicy.IsValid()
		|| !Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::InvalidInput,
			TEXT("Weapon guard defense composition requires valid frozen inputs."));
	}
	if (!BaseDefense.IsValid())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::BaseDefenseInvalid,
			TEXT("Weapon guard defense composition requires one valid base snapshot."));
	}

	FShanmenWeaponGuardTimingProjectionReceipt TimingProjection;
	if (!FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window,
			ActionRuntime,
			TimingPolicy,
			Observation,
			TimingProjection))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::TimingProjectionRejected,
			TEXT("P11.1 rejected the caller-owned timeline observation."));
	}

	const Fdemo_mapShanmenWeaponGuardWorldResult WorldEvaluation =
		Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate(
			World,
			EntityRegistry,
			DefenderActor,
			ThreatActor,
			Window,
			ActionRuntime,
			ArcPolicy,
			TimingProjection,
			Candidate);
	if (!WorldEvaluation.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::WorldEvaluationRejected,
			*WorldEvaluation.Diagnostic,
			TimingProjection,
			WorldEvaluation);
	}

	FShanmenDefenseSnapshot Defense = BaseDefense;
	const Edemo_mapShanmenWeaponGuardDefenseStatus Status =
		WorldEvaluation.IsQualified()
		? Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedQualified
		: Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedOutsideArc;
	if (Status
		== Edemo_mapShanmenWeaponGuardDefenseStatus::ComposedQualified)
	{
		const FShanmenDefenseLayer& Layer =
			WorldEvaluation.Evaluation.GetLayer();
		if (!Layer.IsValid())
		{
			return Reject(
				Edemo_mapShanmenWeaponGuardDefenseStatus::
					SnapshotCompositionRejected,
				TEXT("Qualified World evidence did not provide one valid layer."),
				TimingProjection,
				WorldEvaluation);
		}
		if (ContainsLayerId(BaseDefense, Layer.LayerId))
		{
			return Reject(
				Edemo_mapShanmenWeaponGuardDefenseStatus::LayerConflict,
				TEXT("Base defense already contains the selected guard layer identity."),
				TimingProjection,
				WorldEvaluation);
		}
		Defense.Layers.Add(Layer);
	}
	else if (WorldEvaluation.Evaluation.HasLayer())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::
				SnapshotCompositionRejected,
			TEXT("Outside-arc World evidence cannot expose a defense layer."),
			TimingProjection,
			WorldEvaluation);
	}

	if (!Defense.IsValid())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::
				SnapshotCompositionRejected,
			TEXT("Appending the selected guard layer produced an invalid snapshot."),
			TimingProjection,
			WorldEvaluation);
	}

	Fdemo_mapShanmenWeaponGuardDefenseResult Result;
	Result.Status = Status;
	Result.TimingProjection = TimingProjection;
	Result.WorldEvaluation = WorldEvaluation;
	Result.BaseDefense = BaseDefense;
	Result.Defense = MoveTemp(Defense);
	Result.ReceiptId = MakeReceiptId(
		Result.Status,
		Result.TimingProjection,
		Result.WorldEvaluation,
		Result.BaseDefense,
		Result.Defense);
	Result.Diagnostic = Result.HasGuardLayer()
		? TEXT("Qualified weapon guard layer appended to the base defense snapshot.")
		: TEXT("Outside-arc evidence preserved the base defense snapshot unchanged.");
	if (!Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardDefenseStatus::
				SnapshotCompositionRejected,
			TEXT("Weapon guard defense composition failed its final evidence audit."),
			TimingProjection,
			WorldEvaluation);
	}
	return Result;
}
