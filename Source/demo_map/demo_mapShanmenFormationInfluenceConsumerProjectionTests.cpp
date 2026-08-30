#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceConsumerProjection.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeDefinitions.h"

namespace
{
	const FGuid ConsumerRunId(0xF8C00001, 0, 0, 1);
	const FGuid ConsumerOwnerId(0xF8C00002, 0, 0, 1);
	const FGuid ConsumerSourceId(0xF8C00003, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp ConsumerContent(const int32 Variant = 1)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.25");
		Content.Digest = FString::Printf(
			TEXT("formation-influence-consumer-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeConsumerAnchor(
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		auto& Intent = Receipt.Intent;
		Intent.RunId = ConsumerRunId;
		Intent.OwnerId = ConsumerOwnerId;
		Intent.DeploymentId = FGuid(0xF8C00100, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Consumer.%02d"), Ordinal));
		Intent.AnchorInstanceId = FGuid(0xF8C01000 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId = FGuid(0xF8C02000 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId = FGuid(0xF8C03000 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8C04000 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Ordinal;
		Intent.Content = ConsumerContent();
		Intent.PlacementId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacement.r1"),
			{
				GuidDigits(Intent.RunId), GuidDigits(Intent.OwnerId),
				GuidDigits(Intent.DeploymentId),
				Intent.AnchorDefinitionId.ToString(),
				GuidDigits(Intent.AnchorInstanceId),
				GuidDigits(Intent.AttemptId),
				GuidDigits(Intent.FulfillmentId),
				GuidDigits(Intent.DeploymentReceiptId),
				FString::FromInt(Intent.AuthorityRevision),
				Intent.Content.Version.ToString(), Intent.Content.Digest
			});
		Receipt.ActorClassPath = TEXT("/Script/Engine.Actor");
		Receipt.PlacementTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
				Intent.PlacementId);
		Receipt.DeploymentTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Intent.DeploymentId);
		Receipt.ReceiptId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacementReceipt.r1"),
			{ GuidDigits(Intent.PlacementId), Receipt.ActorClassPath });
		check(Receipt.IsValid());
		return Receipt;
	}

	Fdemo_mapShanmenFormationAreaSnapshot MakeConsumerArea()
	{
		const auto Built = Fdemo_mapShanmenFormationAreaProvider::BuildArea({
			MakeConsumerAnchor(1, FVector(0.0, 0.0, 0.0)),
			MakeConsumerAnchor(2, FVector(100.0, 0.0, 0.0)),
			MakeConsumerAnchor(3, FVector(100.0, 100.0, 0.0)),
			MakeConsumerAnchor(4, FVector(0.0, 100.0, 0.0))
		});
		check(Built.IsSuccess());
		return Built.Area;
	}

	Fdemo_mapShanmenFormationInfluencePolicy MakeConsumerPolicy(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area)
	{
		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.Consumer.Offense");
		Policy.InfluenceDefinitionId =
			TEXT("Formation.Influence.Consumer.OffensePower");
		Policy.Content = Area.Content;
		check(Policy.IsValid());
		return Policy;
	}

	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot MakeActiveLease(
		const int32 SubjectOrdinal,
		const FGameplayTag& Channel,
		const TArray<int32>& Magnitudes)
	{
		const auto Area = MakeConsumerArea();
		const auto Policy = MakeConsumerPolicy(Area);
		const FGuid SubjectId(0xF8C05000 + SubjectOrdinal, 0, 0, 1);
		const auto Intent = Fdemo_mapShanmenFormationInfluenceIntent::Make(
			Area, ConsumerSourceId, Policy, SubjectId,
			Edemo_mapShanmenFormationInfluenceOperation::Apply,
			FGuid(0xF8C06000 + SubjectOrdinal, 0, 0, 1));
		check(Intent.IsValid());

		TArray<Fdemo_mapShanmenFormationInfluenceModifierSpecification> Specs;
		for (int32 Index = 0; Index < Magnitudes.Num(); ++Index)
		{
			Fdemo_mapShanmenFormationInfluenceModifierSpecification Spec;
			check(Fdemo_mapShanmenFormationInfluenceModifierSpecification::
				TryCreate(
					Policy,
					FName(*FString::Printf(
						TEXT("Formation.Modifier.Consumer.%02d"), Index)),
					Channel, FGameplayTagContainer(), FGameplayTagContainer(),
					Magnitudes[Index],
					FName(*FString::Printf(
						TEXT("Formation.Stack.Consumer.%02d"), Index)),
					Edemo_mapShanmenFormationInfluenceStackPolicy::Additive,
					Index, Spec));
			Specs.Add(Spec);
		}
		Fdemo_mapShanmenFormationInfluenceEvaluationContext Context;
		Context.RunId = Intent.RunId;
		Context.SubjectEntityId = Intent.SubjectEntityId;
		Context.Channel = Channel;
		Context.Content = Intent.Content;
		const auto Evaluated =
			Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
				Context, Specs);
		check(Evaluated.IsSuccess());
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding Binding;
		check(Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
			TryCaptureApply(Evaluated.Receipt, Binding));

		Fdemo_mapShanmenFormationInfluenceExecutorInvocation Invocation;
		Invocation.LedgerId = FGuid(0xF8C07000 + SubjectOrdinal, 0, 0, 1);
		Invocation.Intent = Intent;
		Invocation.AttemptId = FGuid(0xF8C08000 + SubjectOrdinal, 0, 0, 1);
		Invocation.Evaluation = Binding;
		check(Invocation.IsValid());
		Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
		check(Executor.Execute(Invocation).IsSuccess());
		Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
		Fdemo_mapShanmenFormationInfluenceLeaseSnapshot Lease;
		check(Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
			Intent, Key));
		check(Executor.TryGetActiveLease(Key, Lease));
		check(Lease.IsValid());
		return Lease;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerDefinition MakeDefinition(
		const int64 UnitsPerPoint = 100,
		const int32 Priority = 25,
		const int32 ContentVariant = 1,
		const TCHAR* DefinitionId =
			TEXT("Formation.Consumer.OffensePower.Primary"))
	{
		Fdemo_mapShanmenFormationInfluenceConsumerDefinition Definition;
		check(Fdemo_mapShanmenFormationInfluenceConsumerDefinition::
			TryCreateOffensePowerAdditive(
				FName(DefinitionId), UnitsPerPoint, Priority,
				ConsumerContent(ContentVariant), Definition));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerDeterministicCommandTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection.DeterministicApplyRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerDeterministicCommandTest::RunTest(
	const FString&)
{
	const auto Lease = MakeActiveLease(
		1, FShanmenCombatNativeTags::InfluenceOffensePower(), { 250 });
	const auto Definition = MakeDefinition();
	const auto First =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			Lease, Definition);
	const auto Replay =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			Lease, Definition);

	TestTrue(TEXT("One exact lease projects to one deterministic product handle"),
		First.IsSuccess() && First.HasProjection()
			&& Replay.HasProjection()
			&& First.Projection.Matches(Replay.Projection)
			&& First.Projection.GetHandle()
				== Replay.Projection.GetHandle()
			&& !First.Projection.GetSourceId().IsNone());
	TestTrue(TEXT("Projection retains exact rational magnitude and legacy target"),
		First.Projection.GetMagnitudeUnits() == 250
			&& First.Projection.GetMagnitudeUnitsPerAttributePoint() == 100
			&& First.Projection.GetDefinition().GetTargetAttributeId()
				== Fdemo_mapAttributeIds::AttackPower
			&& First.Projection.GetDefinition().GetOperation()
				== Edemo_mapModifierOperation::Add
			&& First.Projection.GetLease().EvaluationReceipt.ReceiptId
				== Lease.EvaluationReceipt.ReceiptId);

	Fdemo_mapShanmenFormationInfluenceConsumerCommand Apply;
	Fdemo_mapShanmenFormationInfluenceConsumerCommand Remove;
	const bool bApply =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::TryBuildCommand(
			First.Projection,
			Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply,
			Apply);
	const bool bRemove =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::TryBuildCommand(
			First.Projection,
			Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove,
			Remove);
	TestTrue(TEXT("Apply and Remove are distinct commands over one reversible handle"),
		bApply && bRemove && Apply.IsValid() && Remove.IsValid()
			&& Apply.GetCommandId() != Remove.GetCommandId()
			&& Apply.GetHandle() == Remove.GetHandle()
			&& Apply.GetProjection().Matches(Remove.GetProjection()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerFailClosedTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection.FailClosedContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerFailClosedTest::RunTest(
	const FString&)
{
	const auto OffenseLease = MakeActiveLease(
		2, FShanmenCombatNativeTags::InfluenceOffensePower(), { 100 });
	const auto DefenseLease = MakeActiveLease(
		3, FShanmenCombatNativeTags::InfluenceDefenseGuard(), { 100 });
	const auto Definition = MakeDefinition();
	const auto ContentMismatch =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			OffenseLease, MakeDefinition(100, 25, 2));
	const auto ChannelMismatch =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			DefenseLease, Definition);
	const auto InvalidDefinition =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			OffenseLease,
			Fdemo_mapShanmenFormationInfluenceConsumerDefinition());
	const auto InvalidLease =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			Fdemo_mapShanmenFormationInfluenceLeaseSnapshot(), Definition);
	TestTrue(TEXT("Invalid, stale-content, and unsupported-channel inputs fail closed"),
		ContentMismatch.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
					ContentMismatch
			&& ChannelMismatch.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
					ChannelUnsupported
			&& InvalidDefinition.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
					DefinitionInvalid
			&& InvalidLease.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
					LeaseInvalid
			&& !ContentMismatch.Projection.IsValid()
			&& !ChannelMismatch.Projection.IsValid()
			&& !InvalidDefinition.Projection.IsValid()
			&& !InvalidLease.Projection.IsValid());

	const auto Valid =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			OffenseLease, Definition);
	Fdemo_mapShanmenFormationInfluenceConsumerCommand InvalidCommand;
	const bool bAcceptedUnknown =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::TryBuildCommand(
			Valid.Projection,
			static_cast<
				Edemo_mapShanmenFormationInfluenceConsumerCommandOperation>(255),
			InvalidCommand);
	TestTrue(TEXT("Unknown command operation clears output and is rejected"),
		!bAcceptedUnknown && !InvalidCommand.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerZeroAndIdentityTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection.ZeroAndIdentityFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerZeroAndIdentityTest::RunTest(
	const FString&)
{
	const auto Definition = MakeDefinition();
	const auto ZeroLease = MakeActiveLease(
		4, FShanmenCombatNativeTags::InfluenceOffensePower(), { 100, -100 });
	const auto Zero =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			ZeroLease, Definition);
	TestTrue(TEXT("A valid zero result is a successful no-op without a handle"),
		Zero.IsSuccess() && !Zero.HasProjection()
			&& Zero.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
					NoContribution
			&& !Zero.Projection.GetHandle().IsValid());

	const auto FirstLease = MakeActiveLease(
		5, FShanmenCombatNativeTags::InfluenceOffensePower(), { -75 });
	const auto OtherSubjectLease = MakeActiveLease(
		6, FShanmenCombatNativeTags::InfluenceOffensePower(), { -75 });
	const auto First =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			FirstLease, Definition);
	const auto OtherSubject =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			OtherSubjectLease, Definition);
	const auto OtherScale =
		Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
			FirstLease, MakeDefinition(1000));
	TestTrue(TEXT("Signed fixed-point evidence survives projection without rounding"),
		First.HasProjection() && First.Projection.GetMagnitudeUnits() == -75
			&& First.Projection.GetMagnitudeUnitsPerAttributePoint() == 100);
	TestTrue(TEXT("Subject and authored scale identities cannot alias one handle"),
		OtherSubject.HasProjection() && OtherScale.HasProjection()
			&& First.Projection.GetHandle()
				!= OtherSubject.Projection.GetHandle()
			&& First.Projection.GetHandle()
				!= OtherScale.Projection.GetHandle());
	return true;
}

#endif
