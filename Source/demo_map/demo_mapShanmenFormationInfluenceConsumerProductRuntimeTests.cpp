#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceConsumerProductRuntime.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenFormationAreaProvider.h"
#include "demo_mapShanmenFormationInfluenceConsumerProjection.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid TestGuid(const int32 Variant, const int32 Offset)
	{
		return FGuid(0xF8E00000 + Variant, 0, 0, Offset);
	}

	struct FConsumerRuntimeFixture
	{
		Fdemo_mapShanmenRunCorrelation Correlation;
		Fdemo_mapShanmenFormationProductHost Host;

		bool Start(
			const int32 IdentityVariant,
			const int32 DiagramVariant = INDEX_NONE)
		{
			const int32 EffectiveDiagramVariant = DiagramVariant == INDEX_NONE
				? IdentityVariant
				: DiagramVariant;
			Correlation.CorrelationId = TestGuid(IdentityVariant, 1);
			Correlation.OwnerId = TestGuid(IdentityVariant, 2);
			Correlation.ScopeId = TestGuid(IdentityVariant, 3);
			Correlation.ActiveRunId = TestGuid(IdentityVariant, 4);
			Correlation.PreparedRequestId = TestGuid(IdentityVariant, 5);
			Correlation.PreparedReceiptId = TestGuid(IdentityVariant, 6);
			Correlation.LifecycleRequestId = TestGuid(IdentityVariant, 7);
			Correlation.LifecycleReceiptId = TestGuid(IdentityVariant, 8);
			Correlation.PreparedAuthorityRevision = 1;
			Correlation.LifecycleAuthorityRevision = 2;
			const FGuid PreparedItem = TestGuid(IdentityVariant, 9);
			Correlation.OrderedPreparedItemInstanceIds.Add(PreparedItem);
			Correlation.OrderedRunInventoryItemInstanceIds.Add(PreparedItem);
			Correlation.HotbarItemInstanceIds.Init(
				FGuid(), Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
			Correlation.HotbarItemInstanceIds[0] = PreparedItem;
			if (!Correlation.IsValid())
			{
				return false;
			}

			FShanmenContentStamp Content;
			Content.Version = TEXT("0.0.10.P8.32");
			Content.Digest = FString::Printf(
				TEXT("formation-consumer-product-runtime-r%d"),
				IdentityVariant);
			FShanmenCombatActionCapture ActionCapture;
			ActionCapture.RunId = Correlation.ActiveRunId;
			ActionCapture.OwnerId = Correlation.OwnerId;
			ActionCapture.SourceEntityId = TestGuid(IdentityVariant, 10);
			ActionCapture.ActionDefinitionId =
				FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
			ActionCapture.Content = Content;
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId, ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId, IdentityVariant);
			FShanmenCombatActionSnapshot Action;
			if (!FShanmenCombatActionSnapshot::TryCapture(ActionCapture, Action))
			{
				return false;
			}

			FShanmenFormationDiagramCapture DiagramCapture;
			DiagramCapture.ActionDefinitionId =
				ActionCapture.ActionDefinitionId;
			DiagramCapture.DiagramDefinitionId = FName(*FString::Printf(
				TEXT("Formation.Diagram.ConsumerProductRuntime.%d"),
				EffectiveDiagramVariant));
			auto& Anchor = DiagramCapture.Anchors.AddDefaulted_GetRef();
			Anchor.Order = 0;
			Anchor.AnchorDefinitionId = FName(*FString::Printf(
				TEXT("Formation.Anchor.ConsumerProductRuntime.%d"),
				EffectiveDiagramVariant));
			Anchor.RelativeOffset = FVector(100.0, 0.0, 0.0);
			auto& Requirement = Anchor.Requirements.AddDefaulted_GetRef();
			Requirement.Order = 0;
			Requirement.MaterialDefinitionId =
				TEXT("Item.Material.ConsumerProductRuntime");
			Requirement.Quantity = 1;
			FShanmenFormationDiagramDefinition Diagram;
			FShanmenActionTransitionReceipt Startup;
			FShanmenActionTransitionReceipt Active;
			FShanmenFormationDeploymentReceipt Begin;
			return FShanmenFormationDiagramDefinition::TryCapture(
					DiagramCapture, Diagram)
				&& Fdemo_mapShanmenFormationProductHost::TryStart(
					Correlation, Action, Diagram, FVector::ZeroVector,
					FVector::ForwardVector, Host, Startup, Active, Begin)
				&& Host.IsValid();
		}

		const FShanmenCombatActionSnapshot& GetAction() const
		{
			return Host.GetSession().GetActionRuntime().GetAction();
		}
	};

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeAnchorReceipt(
		const FConsumerRuntimeFixture& Fixture,
		const int32 SubjectOrdinal,
		const int32 AnchorOrdinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		auto& Intent = Receipt.Intent;
		Intent.RunId = Fixture.Correlation.ActiveRunId;
		Intent.OwnerId = Fixture.Correlation.OwnerId;
		Intent.DeploymentId =
			Fixture.Host.GetSession().GetDeployment().GetDeploymentId();
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.ProductRuntime.%02d.%02d"),
			SubjectOrdinal, AnchorOrdinal));
		Intent.AnchorInstanceId =
			TestGuid(100 + SubjectOrdinal, 20 + AnchorOrdinal);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			TestGuid(100 + SubjectOrdinal, 30 + AnchorOrdinal);
		Intent.FulfillmentId =
			TestGuid(100 + SubjectOrdinal, 40 + AnchorOrdinal);
		Intent.DeploymentReceiptId =
			TestGuid(100 + SubjectOrdinal, 50 + AnchorOrdinal);
		Intent.AuthorityRevision = AnchorOrdinal;
		Intent.Content = Fixture.GetAction().GetContent();
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

	struct FConsumerRuntimeCommands
	{
		FGuid SubjectEntityId;
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Apply;
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Remove;
	};

	bool BuildConsumerRuntimeCommands(
		const FConsumerRuntimeFixture& Fixture,
		const int32 SubjectOrdinal,
		FConsumerRuntimeCommands& OutCommands)
	{
		OutCommands = FConsumerRuntimeCommands();
		const auto AreaResult =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeAnchorReceipt(
					Fixture, SubjectOrdinal, 1, FVector(0.0, 0.0, 0.0)),
				MakeAnchorReceipt(
					Fixture, SubjectOrdinal, 2, FVector(100.0, 0.0, 0.0)),
				MakeAnchorReceipt(
					Fixture, SubjectOrdinal, 3, FVector(100.0, 100.0, 0.0)),
				MakeAnchorReceipt(
					Fixture, SubjectOrdinal, 4, FVector(0.0, 100.0, 0.0))
			});
		if (!AreaResult.IsSuccess())
		{
			return false;
		}

		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.ProductRuntime");
		Policy.InfluenceDefinitionId =
			TEXT("Formation.Influence.ProductRuntime.OffensePower");
		Policy.Content = Fixture.GetAction().GetContent();
		OutCommands.SubjectEntityId = TestGuid(200 + SubjectOrdinal, 1);
		const auto Intent =
			Fdemo_mapShanmenFormationInfluenceIntent::Make(
				AreaResult.Area, Fixture.GetAction().GetSourceEntityId(),
				Policy, OutCommands.SubjectEntityId,
				Edemo_mapShanmenFormationInfluenceOperation::Apply,
				TestGuid(200 + SubjectOrdinal, 2));
		if (!Intent.IsValid())
		{
			return false;
		}

		Fdemo_mapShanmenFormationInfluenceModifierSpecification Specification;
		if (!Fdemo_mapShanmenFormationInfluenceModifierSpecification::
			TryCreate(
				Policy, TEXT("Formation.Modifier.ProductRuntime.OffensePower"),
				FShanmenCombatNativeTags::InfluenceOffensePower(),
				FGameplayTagContainer(), FGameplayTagContainer(), 200,
				TEXT("Formation.Stack.ProductRuntime.OffensePower"),
				Edemo_mapShanmenFormationInfluenceStackPolicy::Additive,
				0, Specification))
		{
			return false;
		}
		Fdemo_mapShanmenFormationInfluenceEvaluationContext Context;
		Context.RunId = Intent.RunId;
		Context.SubjectEntityId = Intent.SubjectEntityId;
		Context.Channel = FShanmenCombatNativeTags::InfluenceOffensePower();
		Context.Content = Intent.Content;
		const auto Evaluated =
			Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
				Context, { Specification });
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding Binding;
		if (!Evaluated.IsSuccess()
			|| !Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
				TryCaptureApply(Evaluated.Receipt, Binding))
		{
			return false;
		}

		Fdemo_mapShanmenFormationInfluenceExecutorInvocation Invocation;
		Invocation.LedgerId = TestGuid(200 + SubjectOrdinal, 3);
		Invocation.Intent = Intent;
		Invocation.AttemptId = TestGuid(200 + SubjectOrdinal, 4);
		Invocation.Evaluation = Binding;
		Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
		Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
		Fdemo_mapShanmenFormationInfluenceLeaseSnapshot Lease;
		if (!Invocation.IsValid() || !Executor.Execute(Invocation).IsSuccess()
			|| !Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
				Intent, Key)
			|| !Executor.TryGetActiveLease(Key, Lease))
		{
			return false;
		}

		Fdemo_mapShanmenFormationInfluenceConsumerDefinition Definition;
		if (!Fdemo_mapShanmenFormationInfluenceConsumerDefinition::
			TryCreateOffensePowerAdditive(
				TEXT("Formation.Consumer.ProductRuntime.OffensePower"),
				100, 30, Fixture.GetAction().GetContent(), Definition))
		{
			return false;
		}
		const auto Projected =
			Fdemo_mapShanmenFormationInfluenceConsumerProjector::
				ProjectActiveLease(Lease, Definition);
		if (!Projected.HasProjection())
		{
			return false;
		}
		return Fdemo_mapShanmenFormationInfluenceConsumerProjector::
				TryBuildCommand(
					Projected.Projection,
					Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply,
					OutCommands.Apply)
			&& Fdemo_mapShanmenFormationInfluenceConsumerProjector::
				TryBuildCommand(
					Projected.Projection,
					Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove,
					OutCommands.Remove);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductRuntimeLifecycleTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime.ActivateDeactivateTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductRuntimeLifecycleTest::RunTest(
	const FString&)
{
	FConsumerRuntimeFixture Fixture;
	FConsumerRuntimeCommands Commands;
	if (!Fixture.Start(1)
		|| !BuildConsumerRuntimeCommands(Fixture, 1, Commands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime Empty;
	Fdemo_mapShanmenFormationProductHost InvalidHost;
	const bool bInvalidOpen =
		!Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
			InvalidHost, Empty);
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime Runtime;
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime ReplayRuntime;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
			Fixture.Host, Runtime)
		|| !Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
			Fixture.Host, ReplayRuntime))
	{
		return false;
	}
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto InitiallyReady = Runtime.CheckTeardownReady(Fixture.Host);
	const auto Activated = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, Attributes, Commands.Apply);
	const auto ActivationReplay = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, Attributes, Commands.Apply);
	const auto BlockedTeardown = Runtime.CheckTeardownReady(Fixture.Host);
	const bool bActivationStateWasExact = Runtime.GetBindingCount() == 1
		&& Runtime.GetActiveApplicationCount() == 1
		&& Attributes->GetActiveModifierCount() == 1;
	const auto Deactivated = Runtime.TryDeactivate(
		Fixture.Host, Commands.Remove);
	const auto DeactivationReplay = Runtime.TryDeactivate(
		Fixture.Host, Commands.Remove);
	const auto Ready = Runtime.CheckTeardownReady(Fixture.Host);

	TestTrue(TEXT("Open freezes one deterministic product runtime"),
		bInvalidOpen && !Empty.IsValid() && Runtime.IsValid()
			&& Runtime.MatchesProductHost(Fixture.Host)
			&& Runtime.GetRuntimeId() == ReplayRuntime.GetRuntimeId()
			&& Runtime.GetBridge().GetDeploymentId()
				== Fixture.Host.GetSession().GetDeployment().GetDeploymentId());
	TestTrue(TEXT("Apply and replay are one explicit activation lifecycle"),
		InitiallyReady.IsSuccess() && Activated.IsSuccess()
			&& Activated.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					Activated
			&& ActivationReplay.IsSuccess()
			&& ActivationReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					ActivationReplayed
			&& bActivationStateWasExact);
	TestTrue(TEXT("Teardown requires explicit Remove and then becomes ready"),
		!BlockedTeardown.IsSuccess()
			&& BlockedTeardown.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					ActiveApplicationsRemain
			&& Deactivated.IsSuccess() && DeactivationReplay.IsSuccess()
			&& DeactivationReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					DeactivationReplayed
			&& Ready.IsSuccess() && Runtime.IsDrained()
			&& Runtime.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductRuntimePreflightTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime.ActivationPreflightFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductRuntimePreflightTest::RunTest(
	const FString&)
{
	FConsumerRuntimeFixture Fixture;
	FConsumerRuntimeFixture SameScopeForeignDeployment;
	FConsumerRuntimeCommands Commands;
	FConsumerRuntimeCommands ForeignCommands;
	if (!Fixture.Start(2) || !SameScopeForeignDeployment.Start(2, 22)
		|| !BuildConsumerRuntimeCommands(Fixture, 2, Commands)
		|| !BuildConsumerRuntimeCommands(
			SameScopeForeignDeployment, 3, ForeignCommands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime Runtime;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
		Fixture.Host, Runtime));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto Invalid = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, Attributes,
		Fdemo_mapShanmenFormationInfluenceConsumerCommand());
	const auto WrongOperation = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, Attributes, Commands.Remove);
	const auto WrongSubject = Runtime.TryActivate(
		Fixture.Host, TestGuid(300, 1), Attributes, Commands.Apply);
	const auto ForeignDeployment = Runtime.TryActivate(
		Fixture.Host, ForeignCommands.SubjectEntityId, Attributes,
		ForeignCommands.Apply);
	const auto MissingComponent = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, nullptr, Commands.Apply);
	const bool bAllPreflightsWereNonMutating = Runtime.GetBindingCount() == 0
		&& Runtime.GetCompletedTransactionCount() == 0
		&& Attributes->GetActiveModifierCount() == 0;
	const auto Exact = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, Attributes, Commands.Apply);
	const auto Removed = Runtime.TryDeactivate(Fixture.Host, Commands.Remove);

	TestTrue(TEXT("Invalid operation and subject fail before append-only bind"),
		!Invalid.IsSuccess()
			&& Invalid.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					CommandInvalid
			&& !WrongOperation.IsSuccess()
			&& WrongOperation.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					CommandOperationMismatch
			&& !WrongSubject.IsSuccess()
			&& WrongSubject.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					CommandSubjectMismatch);
	TestTrue(TEXT("Foreign deployment fails before binding or native mutation"),
		Fixture.Correlation == SameScopeForeignDeployment.Correlation
			&& Fixture.GetAction().GetSourceEntityId()
				== SameScopeForeignDeployment.GetAction().GetSourceEntityId()
			&& Fixture.Host.GetSession().GetDeployment().GetDeploymentId()
				!= SameScopeForeignDeployment.Host.GetSession()
					.GetDeployment().GetDeploymentId()
			&& !ForeignDeployment.IsSuccess()
			&& ForeignDeployment.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					CommandProductIdentityMismatch);
	TestTrue(TEXT("Binding validation remains nested and exact activation works"),
		!MissingComponent.IsSuccess()
			&& MissingComponent.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					BindingRejected
			&& MissingComponent.Binding.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentUnavailable
			&& bAllPreflightsWereNonMutating
			&& Exact.IsSuccess() && Removed.IsSuccess() && Runtime.IsDrained());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductRuntimeBindingTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime.ProductAndBindingFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductRuntimeBindingTest::RunTest(
	const FString&)
{
	FConsumerRuntimeFixture Fixture;
	FConsumerRuntimeFixture ForeignFixture;
	FConsumerRuntimeCommands First;
	FConsumerRuntimeCommands Second;
	if (!Fixture.Start(3) || !ForeignFixture.Start(4)
		|| !BuildConsumerRuntimeCommands(Fixture, 31, First)
		|| !BuildConsumerRuntimeCommands(Fixture, 32, Second))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime Runtime;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
		Fixture.Host, Runtime));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto ForeignHost = Runtime.TryActivate(
		ForeignFixture.Host, First.SubjectEntityId, Attributes, First.Apply);
	const auto ForeignTeardown = Runtime.CheckTeardownReady(ForeignFixture.Host);
	const bool bForeignHostWasNonMutating = Runtime.GetBindingCount() == 0
		&& Runtime.GetCompletedTransactionCount() == 0
		&& Attributes->GetActiveModifierCount() == 0;
	const auto FirstActivated = Runtime.TryActivate(
		Fixture.Host, First.SubjectEntityId, Attributes, First.Apply);
	const auto ComponentReuse = Runtime.TryActivate(
		Fixture.Host, Second.SubjectEntityId, Attributes, Second.Apply);
	const bool bFirstBindingRemainedExact = Runtime.GetBindingCount() == 1
		&& Runtime.GetActiveApplicationCount() == 1
		&& Attributes->GetActiveModifierCount() == 1;
	const auto FirstRemoved = Runtime.TryDeactivate(Fixture.Host, First.Remove);

	TestTrue(TEXT("Foreign ProductHost cannot bind, route, or check teardown"),
		!ForeignHost.IsSuccess()
			&& ForeignHost.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					ProductIdentityMismatch
			&& !ForeignTeardown.IsSuccess()
			&& ForeignTeardown.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					ProductIdentityMismatch
			&& bForeignHostWasNonMutating);
	TestTrue(TEXT("One native component cannot be rebound to another subject"),
		FirstActivated.IsSuccess() && !ComponentReuse.IsSuccess()
			&& ComponentReuse.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					BindingRejected
			&& ComponentReuse.Binding.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentBindingConflict
			&& bFirstBindingRemainedExact);
	TestTrue(TEXT("The accepted subject still deactivates without replacement"),
		FirstRemoved.IsSuccess() && Runtime.IsDrained()
			&& Runtime.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductRuntimeDeactivateFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime.DeactivationFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductRuntimeDeactivateFenceTest::
RunTest(const FString&)
{
	FConsumerRuntimeFixture Fixture;
	FConsumerRuntimeFixture SameScopeForeignDeployment;
	FConsumerRuntimeCommands Commands;
	FConsumerRuntimeCommands Unbound;
	FConsumerRuntimeCommands ForeignCommands;
	if (!Fixture.Start(5) || !SameScopeForeignDeployment.Start(5, 55)
		|| !BuildConsumerRuntimeCommands(Fixture, 51, Commands)
		|| !BuildConsumerRuntimeCommands(Fixture, 52, Unbound)
		|| !BuildConsumerRuntimeCommands(
			SameScopeForeignDeployment, 53, ForeignCommands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime Runtime;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
		Fixture.Host, Runtime));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto WrongOperation = Runtime.TryDeactivate(
		Fixture.Host, Commands.Apply);
	const auto UnboundRemove = Runtime.TryDeactivate(
		Fixture.Host, Unbound.Remove);
	const auto ForeignRemove = Runtime.TryDeactivate(
		Fixture.Host, ForeignCommands.Remove);
	const bool bRejectedDeactivationsWereNonMutating =
		Runtime.GetBindingCount() == 0
		&& Runtime.GetCompletedTransactionCount() == 0
		&& Attributes->GetActiveModifierCount() == 0;
	const auto Activated = Runtime.TryActivate(
		Fixture.Host, Commands.SubjectEntityId, Attributes, Commands.Apply);
	const auto Removed = Runtime.TryDeactivate(Fixture.Host, Commands.Remove);
	const auto Replay = Runtime.TryDeactivate(Fixture.Host, Commands.Remove);

	TestTrue(TEXT("Deactivate rejects Apply and unbound Remove without mutation"),
		!WrongOperation.IsSuccess()
			&& WrongOperation.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					CommandOperationMismatch
			&& !UnboundRemove.IsSuccess()
			&& UnboundRemove.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					RouteRejected
			&& UnboundRemove.Route.Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					SubjectUnbound
			&& bRejectedDeactivationsWereNonMutating);
	TestTrue(TEXT("Foreign deployment Remove fails at product identity fence"),
		!ForeignRemove.IsSuccess()
			&& ForeignRemove.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					CommandProductIdentityMismatch);
	TestTrue(TEXT("Exact Remove and replay drain one accepted activation"),
		Activated.IsSuccess() && Removed.IsSuccess() && Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					DeactivationReplayed
			&& Runtime.IsDrained()
			&& Runtime.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

#endif
