#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceConsumerProjection.h"
#include "demo_mapShanmenFormationInfluenceConsumerRegistry.h"
#include "demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.h"
#include "demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.h"
#include "demo_mapShanmenFormationInfluenceConsumerCommandHost.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"
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

	Fdemo_mapShanmenFormationInfluenceConsumerProjection MakeProjection(
		const int32 SubjectOrdinal,
		const int32 MagnitudeUnits = 100,
		const int64 UnitsPerPoint = 100)
	{
		const auto Result =
			Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
				MakeActiveLease(
					SubjectOrdinal,
					FShanmenCombatNativeTags::InfluenceOffensePower(),
					{ MagnitudeUnits }),
				MakeDefinition(UnitsPerPoint));
		check(Result.HasProjection());
		return Result.Projection;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerCommand MakeConsumerCommand(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
		const Edemo_mapShanmenFormationInfluenceConsumerCommandOperation
			Operation)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Command;
		check(Fdemo_mapShanmenFormationInfluenceConsumerProjector::
			TryBuildCommand(Projection, Operation, Command));
		return Command;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry
	MakeConsumerRegistry(const int32 ContentVariant = 1)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry Registry;
		check(Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
			TryCreate(ConsumerRunId, ConsumerContent(ContentVariant), Registry));
		return Registry;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator
	MakeConsumerCoordinator(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
		Udemo_mapAttributeComponent* AttributeComponent)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator
			Coordinator;
		check(Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
			TryCreate(
				ConsumerRunId, ConsumerContent(),
				Projection.GetLease().Key.SubjectEntityId,
				AttributeComponent, Coordinator));
		return Coordinator;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerCommandHost MakeConsumerHost(
		const int32 ContentVariant = 1)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerCommandHost Host;
		check(Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::TryOpen(
			ConsumerRunId, ConsumerContent(ContentVariant), Host));
		return Host;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerRegistryReplayTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry.ApplyRemoveReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerRegistryReplayTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(20, 250);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	auto Registry = MakeConsumerRegistry();
	const auto Applied = Registry.Execute(Apply);
	const auto ApplyReplay = Registry.Execute(Apply);
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot Active;
	const bool bReadActive = Registry.TryGetActiveApplication(
		Projection.GetHandle(), Active);
	const auto Removed = Registry.Execute(Remove);
	const auto RemoveReplay = Registry.Execute(Remove);

	TestTrue(TEXT("Apply replay returns one receipt without duplicate state"),
		Applied.IsSuccess() && ApplyReplay.IsSuccess()
			&& !Applied.bCommandReplayed && ApplyReplay.bCommandReplayed
			&& Applied.Receipt.Matches(ApplyReplay.Receipt)
			&& bReadActive && Active.IsValid()
			&& Active.GetHandle() == Projection.GetHandle());
	TestTrue(TEXT("Remove and replay drain exactly the same application"),
		Removed.IsSuccess() && RemoveReplay.IsSuccess()
			&& !Removed.bCommandReplayed && RemoveReplay.bCommandReplayed
			&& Removed.Receipt.Matches(RemoveReplay.Receipt)
			&& Applied.Receipt.GetApplicationId()
				== Removed.Receipt.GetApplicationId()
			&& Registry.GetCompletedCommandCount() == 2
			&& Registry.GetActiveApplicationCount() == 0
			&& Registry.IsDrained() && Registry.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerRegistryCollisionTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry.SlotCollisionRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerRegistryCollisionTest::RunTest(
	const FString&)
{
	const auto FirstProjection = MakeProjection(21, 100, 100);
	const auto SecondProjection = MakeProjection(21, 100, 1000);
	const auto ApplyFirst = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto RemoveFirst = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	const auto ApplySecond = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto RemoveSecond = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	auto Registry = MakeConsumerRegistry();
	const auto First = Registry.Execute(ApplyFirst);
	const auto Collision = Registry.Execute(ApplySecond);
	const auto StaleRemove = Registry.Execute(RemoveSecond);
	const int32 RecordsAfterTransientFailures =
		Registry.GetCompletedCommandCount();
	const auto FirstRemoved = Registry.Execute(RemoveFirst);
	const auto Retried = Registry.Execute(ApplySecond);
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot Active;
	const bool bReadSecond = Registry.TryGetActiveApplicationForSlot(
		SecondProjection.GetLease().LeaseId,
		SecondProjection.GetDefinition().GetTargetAttributeId(), Active);
	const auto SecondRemoved = Registry.Execute(RemoveSecond);

	TestTrue(TEXT("Different handle cannot occupy one live lease/attribute slot"),
		First.IsSuccess() && !Collision.IsSuccess()
			&& Collision.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					SlotOccupied
			&& !StaleRemove.IsSuccess()
			&& StaleRemove.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					StaleHandle
			&& RecordsAfterTransientFailures == 1);
	TestTrue(TEXT("Transient collision remains retryable after exact removal"),
		FirstRemoved.IsSuccess() && Retried.IsSuccess() && bReadSecond
			&& Active.GetHandle() == SecondProjection.GetHandle()
			&& SecondRemoved.IsSuccess() && Registry.IsDrained()
			&& Registry.GetCompletedCommandCount() == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerRegistryStaleRemoveTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry.StaleRemoveAfterReapply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerRegistryStaleRemoveTest::RunTest(
	const FString&)
{
	const auto FirstProjection = MakeProjection(22, 100, 100);
	const auto SecondProjection = MakeProjection(22, 100, 1000);
	const auto ApplyFirst = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto RemoveFirst = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	const auto ApplySecond = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto RemoveSecond = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	auto Registry = MakeConsumerRegistry();
	const auto First = Registry.Execute(ApplyFirst);
	const auto FirstRemoved = Registry.Execute(RemoveFirst);
	const auto Second = Registry.Execute(ApplySecond);
	const auto StaleRemove = Registry.Execute(RemoveFirst);
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot Active;
	const bool bSecondSurvives = Registry.TryGetActiveApplication(
		SecondProjection.GetHandle(), Active);
	const auto SecondRemoved = Registry.Execute(RemoveSecond);

	TestTrue(TEXT("Completed old Remove replays without touching a newer handle"),
		First.IsSuccess() && FirstRemoved.IsSuccess() && Second.IsSuccess()
			&& StaleRemove.IsSuccess() && StaleRemove.bCommandReplayed
			&& StaleRemove.Receipt.Matches(FirstRemoved.Receipt)
			&& bSecondSurvives
			&& Active.GetHandle() == SecondProjection.GetHandle());
	TestTrue(TEXT("New handle retains its own removable lifecycle"),
		SecondRemoved.IsSuccess() && Registry.IsDrained()
			&& Registry.GetCompletedCommandCount() == 4
			&& Registry.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerRegistryScopeTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry.ScopeMissingAndDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerRegistryScopeTest::RunTest(
	const FString&)
{
	const auto FirstProjection = MakeProjection(23, -75);
	const auto SecondProjection = MakeProjection(24, 125);
	const auto ApplyFirst = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto RemoveFirst = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	const auto ApplySecond = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto RemoveSecond = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	auto WrongScope = MakeConsumerRegistry(2);
	const auto ScopeRejected = WrongScope.Execute(ApplyFirst);
	auto Registry = MakeConsumerRegistry();
	const auto Missing = Registry.Execute(RemoveFirst);
	const auto First = Registry.Execute(ApplyFirst);
	const auto FirstRemoved = Registry.Execute(RemoveFirst);
	const auto Second = Registry.Execute(ApplySecond);
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult StoredSecond;
	const bool bReadStored = Registry.TryGetCompletedResult(
		ApplySecond.GetCommandId(), StoredSecond);
	const bool bBlockedBeforeDrain = !Registry.IsDrained();
	const auto SecondRemoved = Registry.Execute(RemoveSecond);
	const auto Invalid = Registry.Execute(
		Fdemo_mapShanmenFormationInfluenceConsumerCommand());

	TestTrue(TEXT("Scope and missing application failures leave no history"),
		!ScopeRejected.IsSuccess()
			&& ScopeRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					ScopeMismatch
			&& WrongScope.GetCompletedCommandCount() == 0
			&& !Missing.IsSuccess()
			&& Missing.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					ApplicationMissing);
	TestTrue(TEXT("Retry after missing succeeds and drain gates teardown"),
		First.IsSuccess() && FirstRemoved.IsSuccess() && Second.IsSuccess()
			&& bReadStored && StoredSecond.IsSuccess()
			&& !StoredSecond.bCommandReplayed && bBlockedBeforeDrain
			&& SecondRemoved.IsSuccess() && Registry.IsDrained()
			&& Registry.GetCompletedCommandCount() == 4);
	TestTrue(TEXT("Invalid command is rejected without corrupting drained state"),
		!Invalid.IsSuccess()
			&& Invalid.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					CommandInvalid
			&& Registry.IsConsistent() && Registry.IsDrained());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerAttributeApplyReplayTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter.ApplyReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerAttributeApplyReplayTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(30, 250);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	auto Registry = MakeConsumerRegistry();
	const auto Accepted = Registry.Execute(Apply);
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto First =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, Accepted);
	const auto RegistryReplay = Registry.Execute(Apply);
	const auto NativeReplay =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, RegistryReplay);
	float AttackPower = 0.0f;
	const bool bReadPower = Attributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, AttackPower);

	TestTrue(TEXT("Accepted Apply reaches the exact native modifier once"),
		Accepted.IsSuccess() && First.IsSuccess()
			&& First.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					Applied
			&& First.bComponentMutated
			&& First.Acknowledgement.GetModifierSpec().Value == 2.5f
			&& First.Acknowledgement.GetApplicationReceipt().GetCommand().
				GetHandle() == Projection.GetHandle()
			&& Attributes->GetActiveModifierCount() == 1
			&& Attributes->GetModifierCountBySource(Projection.GetSourceId()) == 1
			&& bReadPower && FMath::IsNearlyEqual(AttackPower, 3.5f));
	TestTrue(TEXT("Registry/native replay preserves one stable acknowledgement"),
		RegistryReplay.IsSuccess() && RegistryReplay.bCommandReplayed
			&& NativeReplay.IsSuccess()
			&& NativeReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					ApplyReplayed
			&& !NativeReplay.bComponentMutated
			&& First.Acknowledgement.Matches(NativeReplay.Acknowledgement)
			&& Attributes->GetActiveModifierCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerAttributeRemoveReplayTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter.RemoveReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerAttributeRemoveReplayTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(31, 125);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	auto Registry = MakeConsumerRegistry();
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto NativeApply =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, Registry.Execute(Apply));
	const auto Removed = Registry.Execute(Remove);
	const auto NativeRemove =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, Removed);
	const auto RegistryReplay = Registry.Execute(Remove);
	const auto NativeReplay =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, RegistryReplay);
	float AttackPower = 0.0f;
	const bool bReadPower = Attributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, AttackPower);

	TestTrue(TEXT("Exact Remove restores authoritative baseline once"),
		NativeApply.IsSuccess() && Removed.IsSuccess()
			&& NativeRemove.IsSuccess()
			&& NativeRemove.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					Removed
			&& NativeRemove.bComponentMutated
			&& Attributes->GetActiveModifierCount() == 0
			&& bReadPower && FMath::IsNearlyEqual(AttackPower, 1.0f));
	TestTrue(TEXT("Lost Remove acknowledgement replays as desired-state no-op"),
		RegistryReplay.IsSuccess() && RegistryReplay.bCommandReplayed
			&& NativeReplay.IsSuccess()
			&& NativeReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					RemoveReplayed
			&& !NativeReplay.bComponentMutated
			&& NativeRemove.Acknowledgement.Matches(
				NativeReplay.Acknowledgement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerAttributeCompensationTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter.CompensationAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerAttributeCompensationTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(32, 100);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	auto Registry = MakeConsumerRegistry();
	const auto AcceptedApply = Registry.Execute(Apply);
	const auto AcceptedRemove = Registry.Execute(Remove);
	Udemo_mapAttributeComponent* EmptyAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto Compensated =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			EmptyAttributes, AcceptedRemove);

	TestTrue(TEXT("Remove converges safely when accepted Apply never reached native state"),
		AcceptedApply.IsSuccess() && AcceptedRemove.IsSuccess()
			&& Compensated.IsSuccess()
			&& Compensated.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					RemoveReplayed
			&& EmptyAttributes->GetActiveModifierCount() == 0);

	const auto ConflictProjection = MakeProjection(33, 200);
	const auto ConflictApply = MakeConsumerCommand(
		ConflictProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	auto ConflictRegistry = MakeConsumerRegistry();
	const auto ConflictAccepted = ConflictRegistry.Execute(ConflictApply);
	Fdemo_mapModifierSpec WrongSpec;
	check(Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::
		TryBuildModifierSpec(ConflictProjection, WrongSpec));
	WrongSpec.Value += 1.0f;
	Udemo_mapAttributeComponent* ConflictedAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto Seeded = ConflictedAttributes->EnsureModifierApplied(
		WrongSpec, ConflictProjection.GetHandle());
	const auto Rejected =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			ConflictedAttributes, ConflictAccepted);
	const auto ConflictRemove = MakeConsumerCommand(
		ConflictProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	const auto RemoveRejected =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			ConflictedAttributes,
			ConflictRegistry.Execute(ConflictRemove));

	TestTrue(TEXT("Same exact handle with different native spec fails closed"),
		Seeded == Edemo_mapExactModifierMutationStatus::Applied
			&& !Rejected.IsSuccess()
			&& Rejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					NativeMutationRejected
			&& Rejected.NativeStatus
				== Edemo_mapExactModifierMutationStatus::HandleConflict
			&& !Rejected.Acknowledgement.IsValid()
			&& !RemoveRejected.IsSuccess()
			&& RemoveRejected.NativeStatus
				== Edemo_mapExactModifierMutationStatus::HandleConflict
			&& ConflictedAttributes->GetActiveModifierCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerAttributeEvidenceFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter.SignedConversionAndEvidenceFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerAttributeEvidenceFenceTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(34, -75, 100);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	auto Registry = MakeConsumerRegistry();
	const auto Accepted = Registry.Execute(Apply);
	Fdemo_mapModifierSpec Spec;
	const bool bConverted =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::
			TryBuildModifierSpec(Projection, Spec);
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto Synchronized =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, Accepted);
	float AttackPower = 0.0f;
	const bool bReadPower = Attributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, AttackPower);

	TestTrue(TEXT("Signed rational crosses the single float boundary exactly once"),
		bConverted && Spec.SourceId == Projection.GetSourceId()
			&& Spec.AttributeId == Fdemo_mapAttributeIds::AttackPower
			&& Spec.Operation == Edemo_mapModifierOperation::Add
			&& Spec.Value == -0.75f && Spec.Priority == 25
			&& Synchronized.IsSuccess() && bReadPower
			&& FMath::IsNearlyEqual(AttackPower, 0.25f));

	auto Tampered = Accepted;
	Tampered.Status =
		Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::Removed;
	const auto Mismatch =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes, Tampered);
	const auto MissingComponent =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			nullptr, Accepted);
	const auto RejectedApplication =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			Attributes,
			Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult());
	TestTrue(TEXT("Status tamper null component and rejected registry evidence fail closed"),
		!Mismatch.IsSuccess()
			&& Mismatch.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					EvidenceMismatch
			&& !MissingComponent.IsSuccess()
			&& MissingComponent.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					ComponentUnavailable
			&& !RejectedApplication.IsSuccess()
			&& RejectedApplication.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					ApplicationRejected
			&& Attributes->GetActiveModifierCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCoordinatorAtomicLifecycleTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator.AtomicApplyRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCoordinatorAtomicLifecycleTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(40, 250);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Coordinator = MakeConsumerCoordinator(Projection, Attributes);
	const auto Applied = Coordinator.Execute(Apply);
	float AppliedPower = 0.0f;
	const bool bReadApplied = Attributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, AppliedPower);
	const bool bApplyCommitted =
		Coordinator.GetCompletedTransactionCount() == 1
		&& Coordinator.GetActiveApplicationCount() == 1
		&& Attributes->GetActiveModifierCount() == 1
		&& !Coordinator.IsDrained();
	const auto Removed = Coordinator.Execute(Remove);
	float RemovedPower = 0.0f;
	const bool bReadRemoved = Attributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, RemovedPower);

	TestTrue(TEXT("Apply commits registry and native authority as one transaction"),
		Applied.IsSuccess()
			&& Applied.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					Applied
			&& Applied.bCoordinatorStateCommitted
			&& Applied.Receipt.IsValid()
			&& Applied.Receipt.GetSubjectEntityId()
				== Projection.GetLease().Key.SubjectEntityId
			&& bReadApplied && FMath::IsNearlyEqual(AppliedPower, 3.5f)
			&& bApplyCommitted);
	TestTrue(TEXT("Remove drains registry only after native authority acknowledges"),
		Removed.IsSuccess()
			&& Removed.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					Removed
			&& Coordinator.GetCompletedTransactionCount() == 2
			&& Coordinator.GetActiveApplicationCount() == 0
			&& Attributes->GetActiveModifierCount() == 0
			&& bReadRemoved && FMath::IsNearlyEqual(RemovedPower, 1.0f)
			&& Coordinator.IsConsistent() && Coordinator.IsDrained());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCoordinatorAtomicRetryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator.NativeFailureRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCoordinatorAtomicRetryTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(41, 200);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	Fdemo_mapModifierSpec Expected;
	check(Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::
		TryBuildModifierSpec(Projection, Expected));
	auto Foreign = Expected;
	Foreign.Value += 1.0f;
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Coordinator = MakeConsumerCoordinator(Projection, Attributes);
	check(Attributes->EnsureModifierApplied(Foreign, Projection.GetHandle())
		== Edemo_mapExactModifierMutationStatus::Applied);
	const auto ApplyRejected = Coordinator.Execute(Apply);
	const bool bApplyRollback =
		Coordinator.GetActiveApplicationCount() == 0
		&& Coordinator.GetCompletedTransactionCount() == 0
		&& Attributes->GetActiveModifierCount() == 1;
	check(Attributes->RemoveModifier(Projection.GetHandle()));
	check(Attributes->EnsureModifierApplied(Expected, Projection.GetHandle())
		== Edemo_mapExactModifierMutationStatus::Applied);
	const auto ApplyRetried = Coordinator.Execute(Apply);

	check(Attributes->EnsureModifierRemoved(Expected, Projection.GetHandle())
		== Edemo_mapExactModifierMutationStatus::Removed);
	check(Attributes->EnsureModifierApplied(Foreign, Projection.GetHandle())
		== Edemo_mapExactModifierMutationStatus::Applied);
	const auto RemoveRejected = Coordinator.Execute(Remove);
	const bool bRemoveRollback =
		Coordinator.GetActiveApplicationCount() == 1
		&& Coordinator.GetCompletedTransactionCount() == 1
		&& Attributes->GetActiveModifierCount() == 1;
	check(Attributes->RemoveModifier(Projection.GetHandle()));
	const auto RemoveRetried = Coordinator.Execute(Remove);

	TestTrue(TEXT("Rejected native Apply discards candidate registry and remains retryable"),
		!ApplyRejected.IsSuccess()
			&& ApplyRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					NativeRejected
			&& ApplyRejected.Native.NativeStatus
				== Edemo_mapExactModifierMutationStatus::HandleConflict
			&& !ApplyRejected.bCoordinatorStateCommitted && bApplyRollback
			&& ApplyRetried.IsSuccess()
			&& ApplyRetried.Native.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					ApplyReplayed
			&& !ApplyRetried.Native.bComponentMutated);
	TestTrue(TEXT("Rejected native Remove preserves active registry then retries from truth"),
		!RemoveRejected.IsSuccess()
			&& RemoveRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					NativeRejected
			&& RemoveRejected.Native.NativeStatus
				== Edemo_mapExactModifierMutationStatus::HandleConflict
			&& bRemoveRollback && RemoveRetried.IsSuccess()
			&& RemoveRetried.Native.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					RemoveReplayed
			&& Coordinator.IsDrained()
			&& Coordinator.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCoordinatorHistoricalReplayTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator.HistoricalReplayFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCoordinatorHistoricalReplayTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(42, 150);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Coordinator = MakeConsumerCoordinator(Projection, Attributes);
	const auto Applied = Coordinator.Execute(Apply);
	const auto Removed = Coordinator.Execute(Remove);
	const auto ApplyReplay = Coordinator.Execute(Apply);
	const auto RemoveReplay = Coordinator.Execute(Remove);

	TestTrue(TEXT("Historical Apply replay cannot resurrect a later removed modifier"),
		Applied.IsSuccess() && Removed.IsSuccess() && ApplyReplay.IsSuccess()
			&& ApplyReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					TransactionReplayed
			&& ApplyReplay.bTransactionReplayed
			&& !ApplyReplay.bCoordinatorStateCommitted
			&& ApplyReplay.Receipt.Matches(Applied.Receipt)
			&& Attributes->GetActiveModifierCount() == 0
			&& Coordinator.GetActiveApplicationCount() == 0);
	TestTrue(TEXT("Completed Remove replay is immutable and does not add history"),
		RemoveReplay.IsSuccess() && RemoveReplay.bTransactionReplayed
			&& RemoveReplay.Receipt.Matches(Removed.Receipt)
			&& Coordinator.GetCompletedTransactionCount() == 2
			&& Coordinator.IsDrained());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCoordinatorBindingTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator.BindingAndEvidenceFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCoordinatorBindingTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(43, 100);
	const auto ForeignProjection = MakeProjection(44, 100);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	const auto ForeignApply = MakeConsumerCommand(
		ForeignProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* OtherAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator Invalid;
	const bool bNullRejected =
		!Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
			TryCreate(
				ConsumerRunId, ConsumerContent(),
				Projection.GetLease().Key.SubjectEntityId, nullptr, Invalid);
	auto Coordinator = MakeConsumerCoordinator(Projection, Attributes);
	const auto WrongTarget = Coordinator.Execute(ForeignApply);
	const auto InvalidCommand = Coordinator.Execute(
		Fdemo_mapShanmenFormationInfluenceConsumerCommand());
	const auto Applied = Coordinator.Execute(Apply);
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Stored;
	const bool bReadStored = Coordinator.TryGetCompletedResult(
		Apply.GetCommandId(), Stored);
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Missing;
	const bool bMissingRejected = !Coordinator.TryGetCompletedResult(
		FGuid(0xF8C09999, 0, 0, 1), Missing);
	const auto Removed = Coordinator.Execute(Remove);

	TestTrue(TEXT("Coordinator freezes exact subject and component binding"),
		bNullRejected && Coordinator.MatchesBinding(
			Projection.GetLease().Key.SubjectEntityId, Attributes)
			&& !Coordinator.MatchesBinding(
				Projection.GetLease().Key.SubjectEntityId, OtherAttributes)
			&& !Coordinator.MatchesBinding(
				ForeignProjection.GetLease().Key.SubjectEntityId, Attributes)
			&& Coordinator.HasLiveAttributeComponent());
	TestTrue(TEXT("Wrong target and malformed commands leave both authorities untouched"),
		!WrongTarget.IsSuccess()
			&& WrongTarget.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					TargetMismatch
			&& !InvalidCommand.IsSuccess()
			&& InvalidCommand.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					CommandInvalid);
	TestTrue(TEXT("Completed evidence is queryable and teardown requires exact Remove"),
		Applied.IsSuccess() && bReadStored && Stored.IsSuccess()
			&& Stored.Receipt.Matches(Applied.Receipt)
			&& bMissingRejected && !Missing.Receipt.IsValid()
			&& Removed.IsSuccess() && Coordinator.IsDrained()
			&& Attributes->GetActiveModifierCount() == 0
			&& OtherAttributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCommandHostMultiSubjectTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerCommandHost.MultiSubjectRoutingAndDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCommandHostMultiSubjectTest::RunTest(
	const FString&)
{
	const auto FirstProjection = MakeProjection(50, 250);
	const auto SecondProjection = MakeProjection(51, 150);
	const auto FirstApply = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto FirstRemove = MakeConsumerCommand(
		FirstProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	const auto SecondApply = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto SecondRemove = MakeConsumerCommand(
		SecondProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	Udemo_mapAttributeComponent* FirstAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* SecondAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Host = MakeConsumerHost();
	const auto FirstBinding = Host.TryBindSubject(
		FirstProjection.GetLease().Key.SubjectEntityId, FirstAttributes);
	const auto SecondBinding = Host.TryBindSubject(
		SecondProjection.GetLease().Key.SubjectEntityId, SecondAttributes);
	const auto BindingReplay = Host.TryBindSubject(
		FirstProjection.GetLease().Key.SubjectEntityId, FirstAttributes);
	const auto FirstApplied = Host.TryRoute(FirstApply);
	const auto SecondApplied = Host.TryRoute(SecondApply);
	float FirstPower = 0.0f;
	float SecondPower = 0.0f;
	const bool bReadFirst = FirstAttributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, FirstPower);
	const bool bReadSecond = SecondAttributes->GetFinalValue(
		Fdemo_mapAttributeIds::AttackPower, SecondPower);
	const auto FirstRemoved = Host.TryRoute(FirstRemove);
	const auto SecondRemoved = Host.TryRoute(SecondRemove);

	TestTrue(TEXT("Host freezes two independent subject/component bindings"),
		FirstBinding.IsSuccess() && FirstBinding.bHostStateCommitted
			&& SecondBinding.IsSuccess() && SecondBinding.bHostStateCommitted
			&& BindingReplay.IsSuccess()
			&& BindingReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					BindingReplayed
			&& !BindingReplay.bHostStateCommitted
			&& BindingReplay.Receipt.Matches(FirstBinding.Receipt)
			&& !FirstBinding.Receipt.Matches(SecondBinding.Receipt)
			&& Host.GetBindingCount() == 2);
	TestTrue(TEXT("Each routed Apply mutates only its bound native authority"),
		FirstApplied.IsSuccess() && SecondApplied.IsSuccess()
			&& FirstApplied.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::Routed
			&& SecondApplied.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::Routed
			&& bReadFirst && FMath::IsNearlyEqual(FirstPower, 3.5f)
			&& bReadSecond && FMath::IsNearlyEqual(SecondPower, 2.5f));
	TestTrue(TEXT("Exact Removes drain every coordinator without deleting binding history"),
		FirstRemoved.IsSuccess() && SecondRemoved.IsSuccess()
			&& Host.IsConsistent() && Host.IsDrained()
			&& Host.GetBindingCount() == 2
			&& Host.GetActiveApplicationCount() == 0
			&& Host.GetCompletedTransactionCount() == 4
			&& FirstAttributes->GetActiveModifierCount() == 0
			&& SecondAttributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCommandHostBindingFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerCommandHost.BindingAndScopeFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCommandHostBindingFenceTest::RunTest(
	const FString&)
{
	const auto BoundProjection = MakeProjection(60, 100);
	const auto ForeignSubjectProjection = MakeProjection(61, 100);
	const auto UnboundProjection = MakeProjection(62, 100);
	Udemo_mapAttributeComponent* BoundAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* OtherAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Host = MakeConsumerHost();
	const auto Bound = Host.TryBindSubject(
		BoundProjection.GetLease().Key.SubjectEntityId, BoundAttributes);
	const auto SubjectConflict = Host.TryBindSubject(
		BoundProjection.GetLease().Key.SubjectEntityId, OtherAttributes);
	const auto ComponentConflict = Host.TryBindSubject(
		ForeignSubjectProjection.GetLease().Key.SubjectEntityId,
		BoundAttributes);
	const auto InvalidSubject = Host.TryBindSubject(FGuid(), OtherAttributes);
	const auto MissingComponent = Host.TryBindSubject(
		ForeignSubjectProjection.GetLease().Key.SubjectEntityId, nullptr);
	const auto Unbound = Host.TryRoute(MakeConsumerCommand(
		UnboundProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply));
	const auto InvalidCommand = Host.TryRoute(
		Fdemo_mapShanmenFormationInfluenceConsumerCommand());

	auto ForeignContentHost = MakeConsumerHost(2);
	const auto ForeignContentBound = ForeignContentHost.TryBindSubject(
		BoundProjection.GetLease().Key.SubjectEntityId, OtherAttributes);
	const auto ScopeMismatch = ForeignContentHost.TryRoute(MakeConsumerCommand(
		BoundProjection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply));

	TestTrue(TEXT("Subject and component identities are each one-to-one"),
		Bound.IsSuccess() && !SubjectConflict.IsSuccess()
			&& SubjectConflict.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectBindingConflict
			&& !ComponentConflict.IsSuccess()
			&& ComponentConflict.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentBindingConflict
			&& Host.GetBindingCount() == 1);
	TestTrue(TEXT("Malformed binding and routing inputs fail without Host mutation"),
		!InvalidSubject.IsSuccess()
			&& InvalidSubject.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectInvalid
			&& !MissingComponent.IsSuccess()
			&& MissingComponent.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentUnavailable
			&& !Unbound.IsSuccess()
			&& Unbound.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					SubjectUnbound
			&& !InvalidCommand.IsSuccess()
			&& InvalidCommand.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					CommandInvalid
			&& Host.GetCompletedTransactionCount() == 0);
	TestTrue(TEXT("Run/content scope is checked before coordinator dispatch"),
		ForeignContentBound.IsSuccess() && !ScopeMismatch.IsSuccess()
			&& ScopeMismatch.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					ScopeMismatch
			&& ForeignContentHost.GetCompletedTransactionCount() == 0
			&& OtherAttributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCommandHostRetryReplayTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerCommandHost.RetryAndHistoricalReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCommandHostRetryReplayTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(70, 200);
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	Fdemo_mapModifierSpec Expected;
	check(Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::
		TryBuildModifierSpec(Projection, Expected));
	auto Foreign = Expected;
	Foreign.Value += 1.0f;
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Host = MakeConsumerHost();
	check(Host.TryBindSubject(
		Projection.GetLease().Key.SubjectEntityId, Attributes).IsSuccess());
	check(Attributes->EnsureModifierApplied(Foreign, Projection.GetHandle())
		== Edemo_mapExactModifierMutationStatus::Applied);
	const auto Rejected = Host.TryRoute(Apply);
	const bool bRejectedWithoutCommit =
		Host.GetActiveApplicationCount() == 0
		&& Host.GetCompletedTransactionCount() == 0;
	check(Attributes->RemoveModifier(Projection.GetHandle()));
	check(Attributes->EnsureModifierApplied(Expected, Projection.GetHandle())
		== Edemo_mapExactModifierMutationStatus::Applied);
	const auto Retried = Host.TryRoute(Apply);
	const auto Removed = Host.TryRoute(Remove);
	const auto HistoricalApply = Host.TryRoute(Apply);

	TestTrue(TEXT("Native conflict remains visible and retryable through Host routing"),
		!Rejected.IsSuccess()
			&& Rejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					TransactionRejected
			&& Rejected.Binding.IsValid()
			&& Rejected.Transaction.Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					NativeRejected
			&& Rejected.Transaction.Native.NativeStatus
				== Edemo_mapExactModifierMutationStatus::HandleConflict
			&& bRejectedWithoutCommit && Retried.IsSuccess()
			&& Retried.Transaction.Native.Status
				== Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
					ApplyReplayed);
	TestTrue(TEXT("Historical Apply replay returns evidence without resurrecting state"),
		Removed.IsSuccess() && HistoricalApply.IsSuccess()
			&& HistoricalApply.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					TransactionReplayed
			&& HistoricalApply.Transaction.bTransactionReplayed
			&& !HistoricalApply.bHostStateChanged
			&& HistoricalApply.Transaction.Receipt.Matches(
				Retried.Transaction.Receipt)
			&& Host.GetCompletedTransactionCount() == 2
			&& Host.GetActiveApplicationCount() == 0
			&& Attributes->GetActiveModifierCount() == 0
			&& Host.IsDrained());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerCommandHostLifecycleFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerCommandHost.AppendOnlyLifecycleFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerCommandHostLifecycleFenceTest::RunTest(
	const FString&)
{
	const auto Projection = MakeProjection(80, 100);
	const auto SubjectEntityId =
		Projection.GetLease().Key.SubjectEntityId;
	const auto Apply = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply);
	const auto Remove = MakeConsumerCommand(
		Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove);
	Udemo_mapAttributeComponent* OriginalAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* ReplacementAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	auto Host = MakeConsumerHost();
	const bool bInitiallyDrained = Host.IsDrained();
	const auto Bound = Host.TryBindSubject(
		SubjectEntityId, OriginalAttributes);
	const auto Applied = Host.TryRoute(Apply);
	const auto LiveReplacementRejected = Host.TryBindSubject(
		SubjectEntityId, ReplacementAttributes);
	const auto Removed = Host.TryRoute(Remove);
	const auto DrainedReplacementRejected = Host.TryBindSubject(
		SubjectEntityId, ReplacementAttributes);
	const auto OriginalReplay = Host.TryBindSubject(
		SubjectEntityId, OriginalAttributes);
	Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt Stored;
	const bool bReadStored = Host.TryGetBindingReceipt(
		SubjectEntityId, Stored);

	TestTrue(TEXT("One active application blocks whole-Host teardown"),
		bInitiallyDrained && Bound.IsSuccess() && Applied.IsSuccess()
			&& !LiveReplacementRejected.IsSuccess()
			&& LiveReplacementRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectBindingConflict);
	TestTrue(TEXT("Drained binding remains append-only and exact replayable"),
		Removed.IsSuccess() && Host.IsDrained()
			&& !DrainedReplacementRejected.IsSuccess()
			&& DrainedReplacementRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectBindingConflict
			&& OriginalReplay.IsSuccess()
			&& OriginalReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					BindingReplayed
			&& bReadStored && Stored.Matches(Bound.Receipt)
			&& OriginalReplay.Receipt.Matches(Stored)
			&& Host.HasBinding(SubjectEntityId)
			&& Host.HasLiveBinding(SubjectEntityId)
			&& Host.GetBindingCount() == 1
			&& Host.GetCompletedTransactionCount() == 2);
	return true;
}

#endif
