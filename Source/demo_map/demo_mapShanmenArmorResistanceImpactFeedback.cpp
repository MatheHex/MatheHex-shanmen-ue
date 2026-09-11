#include "demo_mapShanmenArmorResistanceImpactFeedback.h"

#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	using EFeedbackKind =
		Edemo_mapShanmenArmorResistanceImpactFeedbackKind;
	using FFeedback =
		Fdemo_mapShanmenArmorResistanceImpactFeedbackPresentation;

	constexpr float DamageTolerance = 1.0e-4f;

	FString FormatDamage(const float Value)
	{
		FString Text = FString::Printf(TEXT("%.2f"), Value);
		while (Text.EndsWith(TEXT("0")))
		{
			Text.LeftChopInline(1, EAllowShrinking::No);
		}
		if (Text.EndsWith(TEXT(".")))
		{
			Text.LeftChopInline(1, EAllowShrinking::No);
		}
		return Text;
	}

	FString MakeDisplayText(
		const FString& ArmorDisplayName,
		const float PreventedDamage)
	{
		return FString::Printf(
			TEXT("%s · 抵消 %s"),
			*ArmorDisplayName,
			*FormatDamage(PreventedDamage));
	}
}

bool FFeedback::TryProject(
	const Fdemo_mapM01EnemyAttackExecutionResult& AttackResult,
	FFeedback& OutPresentation)
{
	OutPresentation = FFeedback();
	if (!AttackResult.IsExecuted()
		|| !AttackResult.bArmorResistanceInspected
		|| !AttackResult.ArmorResistance.HasProjection()
		|| AttackResult.Delivery.CommitResult.Status
			!= EShanmenVitalityCommitStatus::Committed)
	{
		return false;
	}

	const FShanmenImpactRequest& Request = AttackResult.Impact.GetRequest();
	const FShanmenImpactResult& Result = AttackResult.Impact.GetResult();
	const FShanmenVitalityCommitResult& Commit =
		AttackResult.Delivery.CommitResult;
	const Fdemo_mapShanmenArmorResistanceItemResult& Armor =
		AttackResult.ArmorResistance;
	if (!Request.IsValid() || !Result.bAccepted || !Result.IsConserved()
		|| !Commit.IsValid()
		|| Commit.Receipt.GetImpactId() != Result.ImpactId
		|| !FMath::IsNearlyEqual(
			Commit.Receipt.GetRequestedDamage(),
			Result.FinalDamage,
			DamageTolerance))
	{
		return false;
	}

	TSet<FGuid> ProjectedLayerIds;
	for (const FGuid& LayerId : Armor.Projection.ProjectedLayerIds)
	{
		if (!LayerId.IsValid() || ProjectedLayerIds.Contains(LayerId))
		{
			return false;
		}
		ProjectedLayerIds.Add(LayerId);
	}

	float ArmorPreventedDamage = 0.0f;
	int32 TriggeredArmorLayerCount = 0;
	for (const FShanmenDefenseLayerResult& Triggered :
		Result.TriggeredLayers)
	{
		if (!ProjectedLayerIds.Contains(Triggered.LayerId))
		{
			continue;
		}
		const FShanmenDefenseLayer* RequestLayer =
			Request.Defense.Layers.FindByPredicate(
				[&Triggered](const FShanmenDefenseLayer& Layer)
				{
					return Layer.LayerId == Triggered.LayerId;
				});
		const FShanmenDefenseLayer* ProjectedLayer =
			Armor.Projection.Defense.Layers.FindByPredicate(
				[&Triggered](const FShanmenDefenseLayer& Layer)
				{
					return Layer.LayerId == Triggered.LayerId;
				});
		if (!RequestLayer || !ProjectedLayer
			|| Triggered.SourceInstanceId
				!= Armor.Evidence.ArmorItemInstanceId
			|| RequestLayer->SourceInstanceId
				!= Armor.Evidence.ArmorItemInstanceId
			|| ProjectedLayer->SourceInstanceId
				!= Armor.Evidence.ArmorItemInstanceId
			|| Triggered.Operation
				!= EShanmenDefenseOperation::ReduceFraction
			|| RequestLayer->Operation != Triggered.Operation
			|| ProjectedLayer->Operation != Triggered.Operation
			|| Triggered.Order != FShanmenDefenseOrder::Resistance
			|| RequestLayer->Order != Triggered.Order
			|| ProjectedLayer->Order != Triggered.Order
			|| Triggered.bRequiresCommit
			|| RequestLayer->bRequiresCommitOnTrigger
			|| ProjectedLayer->bRequiresCommitOnTrigger
			|| !Triggered.LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseArmor())
			|| !FMath::IsFinite(Triggered.PreventedDamage)
			|| Triggered.PreventedDamage <= 0.0f)
		{
			return false;
		}
		ArmorPreventedDamage += Triggered.PreventedDamage;
		++TriggeredArmorLayerCount;
	}
	if (TriggeredArmorLayerCount <= 0
		|| !FMath::IsFinite(ArmorPreventedDamage)
		|| ArmorPreventedDamage <= 0.0f
		|| ArmorPreventedDamage
			> Result.PreventedDamage + DamageTolerance)
	{
		return false;
	}

	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Armor.Evidence.ArmorDefinitionId);
	const FString ArmorDisplayName = Definition
		? Definition->DisplayName.ToString().TrimStartAndEnd()
		: FString();
	if (!Definition || ArmorDisplayName.IsEmpty()
		|| !Definition->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::DamageResistance))
	{
		return false;
	}

	FFeedback Candidate;
	Candidate.Kind = EFeedbackKind::Reduced;
	Candidate.ImpactId = Result.ImpactId;
	Candidate.ArmorItemInstanceId =
		Armor.Evidence.ArmorItemInstanceId;
	Candidate.ArmorDefinitionId = Armor.Evidence.ArmorDefinitionId;
	Candidate.ArmorDisplayName = ArmorDisplayName;
	Candidate.RawDamage = Result.RawDamage;
	Candidate.ArmorPreventedDamage = ArmorPreventedDamage;
	Candidate.FinalDamage = Result.FinalDamage;
	Candidate.AppliedDamage = Commit.Receipt.GetAppliedDamage();
	Candidate.DisplayText = MakeDisplayText(
		Candidate.ArmorDisplayName,
		Candidate.ArmorPreventedDamage);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FFeedback::IsValid() const
{
	return Kind == EFeedbackKind::Reduced
		&& ImpactId.IsValid()
		&& ArmorItemInstanceId.IsValid()
		&& !ArmorDefinitionId.IsNone()
		&& !ArmorDisplayName.TrimStartAndEnd().IsEmpty()
		&& FMath::IsFinite(RawDamage)
		&& FMath::IsFinite(ArmorPreventedDamage)
		&& FMath::IsFinite(FinalDamage)
		&& FMath::IsFinite(AppliedDamage)
		&& RawDamage > 0.0f
		&& ArmorPreventedDamage > 0.0f
		&& FinalDamage >= 0.0f
		&& AppliedDamage >= 0.0f
		&& AppliedDamage <= FinalDamage + DamageTolerance
		&& ArmorPreventedDamage
			<= RawDamage - FinalDamage + DamageTolerance
		&& DisplayText
			== MakeDisplayText(ArmorDisplayName, ArmorPreventedDamage);
}

bool FFeedback::Matches(const FFeedback& Other) const
{
	return IsValid() && Other.IsValid()
		&& Kind == Other.Kind
		&& ImpactId == Other.ImpactId
		&& ArmorItemInstanceId == Other.ArmorItemInstanceId
		&& ArmorDefinitionId == Other.ArmorDefinitionId
		&& ArmorDisplayName == Other.ArmorDisplayName
		&& RawDamage == Other.RawDamage
		&& ArmorPreventedDamage == Other.ArmorPreventedDamage
		&& FinalDamage == Other.FinalDamage
		&& AppliedDamage == Other.AppliedDamage
		&& DisplayText == Other.DisplayText;
}
