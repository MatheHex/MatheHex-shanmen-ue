#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenArmorResistanceProjection.h"

#include <limits>

namespace
{
	const FGuid& ArmorItemId()
	{
		static const FGuid Value(10, 20, 30, 40);
		return Value;
	}

	const FGuid& TargetEntityId()
	{
		static const FGuid Value(50, 60, 70, 80);
		return Value;
	}

	Fdemo_mapItemDamageResistance Resistance(
		const FGameplayTag& DamageTag,
		float Fraction)
	{
		Fdemo_mapItemDamageResistance Result;
		Result.DamageTag = DamageTag;
		Result.ResistanceFraction = Fraction;
		return Result;
	}

	Fdemo_mapItemDefinition MakeResistanceArmor()
	{
		Fdemo_mapItemDefinition Definition;
		Definition.DefinitionId = TEXT("Test.Item.Armor.TaggedResistance");
		Definition.CategoryId = Fdemo_mapItemIds::ArmorCategory;
		Definition.MaxStackSize = 1;
		Definition.EquipmentSlotId = Fdemo_mapItemIds::ArmorSlot;
		Definition.CompatibleSlotIds.Add(Fdemo_mapItemIds::ArmorSlot);
		Definition.GameplaySemantics.Add(
			Edemo_mapItemGameplaySemantic::DamageResistance);
		// Deliberately authored out of order; projection canonicalizes by tag.
		Definition.DamageResistances = {
			Resistance(FShanmenCombatNativeTags::DamageSpirit(), 0.40f),
			Resistance(FShanmenCombatNativeTags::DamagePhysical(), 0.25f)
		};
		return Definition;
	}

	FShanmenDefenseSnapshot MakeBaseDefense(bool bWithShield = false)
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		if (bWithShield)
		{
			FShanmenDefenseLayer& Shield = Defense.Layers.AddDefaulted_GetRef();
			Shield.LayerId = FGuid(1, 2, 3, 4);
			Shield.RuleId = TEXT("Test.Defense.Shield");
			Shield.Operation = EShanmenDefenseOperation::AbsorbPoints;
			Shield.Order = FShanmenDefenseOrder::Shield;
			Shield.Magnitude = 10.0f;
			Shield.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
			Shield.RequiredTargetTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
		}
		return Defense;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FShanmenDefenseSnapshot& Defense,
		const FGameplayTag& DamageTag)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = FGuid(101, 102, 103, 104);
		Capture.OwnerId = FGuid(105, 106, 107, 108);
		Capture.SourceEntityId = FGuid(109, 110, 111, 112);
		Capture.ActionDefinitionId = TEXT("Test.Action.EnemyImpact");
		Capture.Content.Version = TEXT("0.0.10.P26.0");
		Capture.Content.Digest = TEXT("Test.ArmorResistanceProjection.r1");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenImpactRequest Request;
		check(FShanmenCombatActionSnapshot::TryCapture(
			Capture, Request.Action));
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = TargetEntityId();
		Request.Candidate.DetectorId = TEXT("Test.Detector.EnemyImpact");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Request.Candidate.HitOrdinal = 0;
		Request.Damage.FormulaId = TEXT("Test.Damage.Constant100");
		Request.Damage.RawDamage = 100.0f;
		Request.Damage.DamageTags.AddTag(DamageTag);
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.Defense = Defense;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Action.GetActivationId(),
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceProjectionCatalogTest,
	"Shanmen.0_0_10.Product.ArmorResistanceProjection.CatalogAndNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceProjectionCatalogTest::RunTest(const FString&)
{
	FString RegistryError;
	TestTrue(TEXT("Existing canonical item catalog remains valid"),
		Fdemo_mapItemDefinitions::Validate(&RegistryError));
	const Fdemo_mapItemDefinition* Existing =
		Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::ArmorRobeLevel1);
	if (!TestNotNull(TEXT("Existing armor definition exists"), Existing))
	{
		return false;
	}
	TestTrue(TEXT("Formal resistance tuning remains unassigned"),
		Existing->DamageResistances.IsEmpty()
			&& !Existing->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::DamageResistance));

	const FShanmenDefenseSnapshot Base = MakeBaseDefense(true);
	const Fdemo_mapShanmenArmorResistanceProjectionResult Result =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			*Existing, ArmorItemId(), TargetEntityId(), Base);
	TestTrue(TEXT("Unassigned armor is an explicit successful no-op"),
		Result.IsSuccess()
			&& Result.Status
				== Edemo_mapShanmenArmorResistanceProjectionStatus::NotApplicable);
	TestTrue(TEXT("No-op preserves the exact caller defense"),
		Result.Defense.Layers.Num() == Base.Layers.Num()
			&& Result.Defense.Layers[0].LayerId == Base.Layers[0].LayerId
			&& Result.ProjectedLayerIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceProjectionDamageChannelsTest,
	"Shanmen.0_0_10.Product.ArmorResistanceProjection.DamageChannels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceProjectionDamageChannelsTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenArmorResistanceProjectionResult Projection =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			MakeResistanceArmor(),
			ArmorItemId(),
			TargetEntityId(),
			MakeBaseDefense());
	if (!TestTrue(TEXT("Typed armor projection succeeds"),
		Projection.IsSuccess() && Projection.HasProjection())
		|| !TestEqual(TEXT("Two typed layers projected"),
			Projection.Defense.Layers.Num(), 2))
	{
		return false;
	}

	const FShanmenImpactResult Physical = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.Defense,
			FShanmenCombatNativeTags::DamagePhysicalSlash()));
	const FShanmenImpactResult Spirit = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.Defense,
			FShanmenCombatNativeTags::DamageSpirit()));
	const FShanmenImpactResult Mental = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			Projection.Defense,
			FShanmenCombatNativeTags::DamageMental()));

	TestTrue(TEXT("Physical parent resistance catches slash and leaves 75"),
		Physical.TriggeredLayers.Num() == 1
			&& FMath::IsNearlyEqual(Physical.FinalDamage, 75.0f)
			&& Physical.IsConserved());
	TestTrue(TEXT("Spirit resistance remains independently tuned at 40 percent"),
		Spirit.TriggeredLayers.Num() == 1
			&& FMath::IsNearlyEqual(Spirit.FinalDamage, 60.0f)
			&& Spirit.IsConserved());
	TestTrue(TEXT("Unlisted mental damage bypasses armor resistance"),
		Mental.TriggeredLayers.IsEmpty()
			&& FMath::IsNearlyEqual(Mental.FinalDamage, 100.0f)
			&& Mental.IsConserved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceProjectionDeterminismTest,
	"Shanmen.0_0_10.Product.ArmorResistanceProjection.DeterminismAndComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceProjectionDeterminismTest::RunTest(
	const FString&)
{
	const Fdemo_mapItemDefinition Definition = MakeResistanceArmor();
	const FShanmenDefenseSnapshot Base = MakeBaseDefense(true);
	const Fdemo_mapShanmenArmorResistanceProjectionResult First =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			Definition, ArmorItemId(), TargetEntityId(), Base);
	const Fdemo_mapShanmenArmorResistanceProjectionResult Replay =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			Definition, ArmorItemId(), TargetEntityId(), Base);
	const Fdemo_mapShanmenArmorResistanceProjectionResult OtherItem =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			Definition,
			FGuid(11, 21, 31, 41),
			TargetEntityId(),
			Base);
	if (!TestTrue(TEXT("All independent projections succeed"),
		First.HasProjection()
			&& Replay.HasProjection()
			&& OtherItem.HasProjection()))
	{
		return false;
	}

	TestTrue(TEXT("Exact replay reproduces ordered layer identities"),
		First.ProjectedLayerIds == Replay.ProjectedLayerIds);
	TestTrue(TEXT("Exact source item participates in every identity"),
		First.ProjectedLayerIds != OtherItem.ProjectedLayerIds);
	TestEqual(TEXT("Caller-owned base remains untouched"),
		Base.Layers.Num(), 1);

	const FShanmenImpactResult Resolution = FShanmenDefenseResolver::Resolve(
		MakeImpactRequest(
			First.Defense,
			FShanmenCombatNativeTags::DamagePhysicalSlash()));
	TestTrue(TEXT("Shield precedes armor and composition leaves 67.5"),
		Resolution.TriggeredLayers.Num() == 2
			&& Resolution.TriggeredLayers[0].Operation
				== EShanmenDefenseOperation::AbsorbPoints
			&& Resolution.TriggeredLayers[1].Operation
				== EShanmenDefenseOperation::ReduceFraction
			&& FMath::IsNearlyEqual(Resolution.FinalDamage, 67.5f)
			&& Resolution.IsConserved());

	const Fdemo_mapShanmenArmorResistanceProjectionResult Conflict =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			Definition,
			ArmorItemId(),
			TargetEntityId(),
			First.Defense);
	TestTrue(TEXT("Appending the same canonical item twice fails closed"),
		!Conflict.IsSuccess()
			&& Conflict.Status
				== Edemo_mapShanmenArmorResistanceProjectionStatus::LayerConflict);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapArmorResistanceProjectionFailClosedTest,
	"Shanmen.0_0_10.Product.ArmorResistanceProjection.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapArmorResistanceProjectionFailClosedTest::RunTest(
	const FString&)
{
	const FShanmenDefenseSnapshot Base = MakeBaseDefense();
	auto IsRejectedDefinition = [&Base](
		const Fdemo_mapItemDefinition& Definition)
	{
		return !Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			Definition, ArmorItemId(), TargetEntityId(), Base).IsSuccess();
	};

	TestTrue(TEXT("Invalid item identity is rejected"),
		!Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			MakeResistanceArmor(), FGuid(), TargetEntityId(), Base).IsSuccess());

	Fdemo_mapItemDefinition MetadataWithoutSemantic = MakeResistanceArmor();
	MetadataWithoutSemantic.GameplaySemantics.Reset();
	TestTrue(TEXT("Resistance metadata cannot infer capability"),
		IsRejectedDefinition(MetadataWithoutSemantic));

	Fdemo_mapItemDefinition SemanticWithoutMetadata = MakeResistanceArmor();
	SemanticWithoutMetadata.DamageResistances.Reset();
	TestTrue(TEXT("Capability without typed metadata is rejected"),
		IsRejectedDefinition(SemanticWithoutMetadata));

	Fdemo_mapItemDefinition WrongSlot = MakeResistanceArmor();
	WrongSlot.EquipmentSlotId = Fdemo_mapItemIds::AccessorySlot;
	TestTrue(TEXT("Non-armor slot cannot project armor resistance"),
		IsRejectedDefinition(WrongSlot));

	Fdemo_mapItemDefinition RootTag = MakeResistanceArmor();
	RootTag.DamageResistances[0].DamageTag =
		FShanmenCombatNativeTags::Damage();
	TestTrue(TEXT("Catch-all damage resistance is rejected"),
		IsRejectedDefinition(RootTag));

	Fdemo_mapItemDefinition FullPrevention = MakeResistanceArmor();
	FullPrevention.DamageResistances[0].ResistanceFraction = 1.0f;
	TestTrue(TEXT("Passive armor cannot become full prevention"),
		IsRejectedDefinition(FullPrevention));

	Fdemo_mapItemDefinition NonFinite = MakeResistanceArmor();
	NonFinite.DamageResistances[0].ResistanceFraction =
		std::numeric_limits<float>::quiet_NaN();
	TestTrue(TEXT("Non-finite resistance is rejected"),
		IsRejectedDefinition(NonFinite));

	Fdemo_mapItemDefinition Overlap = MakeResistanceArmor();
	Overlap.DamageResistances.Add(Resistance(
		FShanmenCombatNativeTags::DamagePhysicalSlash(), 0.10f));
	TestTrue(TEXT("Ancestor and child resistance cannot double-apply"),
		IsRejectedDefinition(Overlap));
	return true;
}

#endif
