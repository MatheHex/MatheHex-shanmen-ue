#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceConsumerProductBridge.h"

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
		return FGuid(0xF8D00000 + Variant, 0, 0, Offset);
	}

	struct FProductBridgeFixture
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
			Content.Version = TEXT("0.0.10.P8.31");
			Content.Digest = FString::Printf(
				TEXT("formation-consumer-product-bridge-r%d"),
				IdentityVariant);
			FShanmenCombatActionCapture ActionCapture;
			ActionCapture.RunId = Correlation.ActiveRunId;
			ActionCapture.OwnerId = Correlation.OwnerId;
			ActionCapture.SourceEntityId = TestGuid(IdentityVariant, 10);
			ActionCapture.ActionDefinitionId =
				FShanmenFormationDiagramDefinition::
					CanonicalActionDefinitionId();
			ActionCapture.Content = Content;
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId, ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId, IdentityVariant);
			FShanmenCombatActionSnapshot Action;
			if (!FShanmenCombatActionSnapshot::TryCapture(
				ActionCapture, Action))
			{
				return false;
			}

			FShanmenFormationDiagramCapture DiagramCapture;
			DiagramCapture.ActionDefinitionId =
				ActionCapture.ActionDefinitionId;
			DiagramCapture.DiagramDefinitionId = FName(*FString::Printf(
				TEXT("Formation.Diagram.ConsumerProductBridge.%d"),
				EffectiveDiagramVariant));
			auto& Anchor = DiagramCapture.Anchors.AddDefaulted_GetRef();
			Anchor.Order = 0;
			Anchor.AnchorDefinitionId = FName(*FString::Printf(
				TEXT("Formation.Anchor.ConsumerProductBridge.%d"),
				EffectiveDiagramVariant));
			Anchor.RelativeOffset = FVector(100.0, 0.0, 0.0);
			auto& Requirement = Anchor.Requirements.AddDefaulted_GetRef();
			Requirement.Order = 0;
			Requirement.MaterialDefinitionId =
				TEXT("Item.Material.ConsumerProductBridge");
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
		const FProductBridgeFixture& Fixture,
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
			TEXT("Formation.Anchor.ProductBridge.%02d.%02d"),
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

	struct FProductConsumerCommands
	{
		FGuid SubjectEntityId;
		Fdemo_mapShanmenFormationInfluenceConsumerProjection Projection;
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Apply;
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Remove;
	};

	bool BuildProductConsumerCommands(
		const FProductBridgeFixture& Fixture,
		const int32 SubjectOrdinal,
		FProductConsumerCommands& OutCommands)
	{
		OutCommands = FProductConsumerCommands();
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
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.ProductBridge");
		Policy.InfluenceDefinitionId =
			TEXT("Formation.Influence.ProductBridge.OffensePower");
		Policy.Content = Fixture.GetAction().GetContent();
		OutCommands.SubjectEntityId =
			TestGuid(200 + SubjectOrdinal, 1);
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
		if (!Fdemo_mapShanmenFormationInfluenceModifierSpecification::TryCreate(
				Policy, TEXT("Formation.Modifier.ProductBridge.OffensePower"),
				FShanmenCombatNativeTags::InfluenceOffensePower(),
				FGameplayTagContainer(), FGameplayTagContainer(), 200,
				TEXT("Formation.Stack.ProductBridge.OffensePower"),
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
				TEXT("Formation.Consumer.ProductBridge.OffensePower"),
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
		OutCommands.Projection = Projected.Projection;
		return Fdemo_mapShanmenFormationInfluenceConsumerProjector::
				TryBuildCommand(
					OutCommands.Projection,
					Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::
						Apply,
					OutCommands.Apply)
			&& Fdemo_mapShanmenFormationInfluenceConsumerProjector::
				TryBuildCommand(
					OutCommands.Projection,
					Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::
						Remove,
					OutCommands.Remove);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductBridgeRouteTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge.RouteAndDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductBridgeRouteTest::RunTest(
	const FString&)
{
	FProductBridgeFixture Fixture;
	FProductConsumerCommands Commands;
	if (!Fixture.Start(1)
		|| !BuildProductConsumerCommands(Fixture, 1, Commands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Empty;
	Fdemo_mapShanmenFormationProductHost EmptyHost;
	const bool bInvalidOpen =
		!Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
			EmptyHost, Empty);
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Bridge;
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge ReplayBridge;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
			Fixture.Host, Bridge)
		|| !Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
			Fixture.Host, ReplayBridge))
	{
		return false;
	}
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto Bound = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes);
	const auto BindingReplay = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes);
	const auto Applied = Bridge.TryRoute(Fixture.Host, Commands.Apply);
	const auto ApplyReplay = Bridge.TryRoute(Fixture.Host, Commands.Apply);
	const auto Removed = Bridge.TryRoute(Fixture.Host, Commands.Remove);

	TestTrue(TEXT("Open freezes one deterministic product identity"),
		bInvalidOpen && !Empty.IsValid() && Bridge.IsValid()
			&& Bridge.MatchesProductHost(Fixture.Host)
			&& Bridge.MatchesCommandProductIdentity(Commands.Apply)
			&& Bridge.GetBridgeId() == ReplayBridge.GetBridgeId()
			&& Bridge.GetCorrelation() == Fixture.Correlation
			&& Bridge.GetActionActivationId()
				== Fixture.GetAction().GetActivationId()
			&& Bridge.GetDeploymentId()
				== Fixture.Host.GetSession().GetDeployment().GetDeploymentId());
	TestTrue(TEXT("Bind and exact replay preserve P8.29 evidence"),
		Bound.IsSuccess() && Bound.bBridgeStateChanged
			&& BindingReplay.IsSuccess()
			&& BindingReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
					BindingReplayed
			&& !BindingReplay.bBridgeStateChanged
			&& BindingReplay.Binding.Receipt.Matches(Bound.Binding.Receipt));
	TestTrue(TEXT("Route delegates mutation and replay without second history"),
		Applied.IsSuccess() && Applied.bBridgeStateChanged
			&& ApplyReplay.IsSuccess()
			&& ApplyReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					TransactionReplayed
			&& !ApplyReplay.bBridgeStateChanged
			&& Removed.IsSuccess() && Bridge.IsDrained()
			&& Bridge.GetBindingCount() == 1
			&& Bridge.GetActiveApplicationCount() == 0
			&& Bridge.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductBridgeIdentityTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge.ProductIdentityFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductBridgeIdentityTest::RunTest(
	const FString&)
{
	FProductBridgeFixture Fixture;
	FProductBridgeFixture ForeignFixture;
	FProductConsumerCommands Commands;
	if (!Fixture.Start(2) || !ForeignFixture.Start(3)
		|| !BuildProductConsumerCommands(Fixture, 2, Commands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Bridge;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
		Fixture.Host, Bridge));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	const auto ForeignBind = Bridge.TryBindSubject(
		ForeignFixture.Host, Commands.SubjectEntityId, Attributes);
	const bool bNoForeignBind = Bridge.GetBindingCount() == 0;
	const auto Bound = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes);
	const auto ForeignRoute = Bridge.TryRoute(
		ForeignFixture.Host, Commands.Apply);
	const bool bNoForeignMutation =
		Bridge.GetCompletedTransactionCount() == 0
		&& Attributes->GetActiveModifierCount() == 0;
	Fdemo_mapShanmenFormationProductHost InvalidHost;
	const auto InvalidRoute = Bridge.TryRoute(InvalidHost, Commands.Apply);
	const auto Applied = Bridge.TryRoute(Fixture.Host, Commands.Apply);
	const auto Removed = Bridge.TryRoute(Fixture.Host, Commands.Remove);

	TestTrue(TEXT("Foreign ProductHost fails before subject binding"),
		!ForeignBind.IsSuccess()
			&& ForeignBind.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
					ProductIdentityMismatch
			&& bNoForeignBind);
	TestTrue(TEXT("Foreign and invalid hosts fail before command routing"),
		Bound.IsSuccess() && !ForeignRoute.IsSuccess()
			&& ForeignRoute.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					ProductIdentityMismatch
			&& bNoForeignMutation && !InvalidRoute.IsSuccess()
			&& InvalidRoute.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					ProductHostInvalid);
	TestTrue(TEXT("The frozen ProductHost remains the only accepted caller"),
		Applied.IsSuccess() && Removed.IsSuccess() && Bridge.IsDrained()
			&& Bridge.GetCompletedTransactionCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductBridgeBindingTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge.BindingLifecycleFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductBridgeBindingTest::RunTest(
	const FString&)
{
	FProductBridgeFixture Fixture;
	FProductConsumerCommands Commands;
	if (!Fixture.Start(4)
		|| !BuildProductConsumerCommands(Fixture, 4, Commands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Bridge;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
		Fixture.Host, Bridge));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* Replacement =
		NewObject<Udemo_mapAttributeComponent>();
	const auto Bound = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes);
	const auto ReplacementRejected = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Replacement);
	const auto ReuseRejected = Bridge.TryBindSubject(
		Fixture.Host, TestGuid(400, 1), Attributes);
	const auto InvalidSubject = Bridge.TryBindSubject(
		Fixture.Host, FGuid(), Replacement);
	const auto MissingComponent = Bridge.TryBindSubject(
		Fixture.Host, TestGuid(400, 2), nullptr);
	const auto Applied = Bridge.TryRoute(Fixture.Host, Commands.Apply);
	const auto Removed = Bridge.TryRoute(Fixture.Host, Commands.Remove);
	const auto DrainedReplacementRejected = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Replacement);
	const auto OriginalReplay = Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes);

	TestTrue(TEXT("Bridge exposes append-only subject and component conflicts"),
		Bound.IsSuccess() && !ReplacementRejected.IsSuccess()
			&& ReplacementRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
					BindingRejected
			&& ReplacementRejected.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectBindingConflict
			&& !ReuseRejected.IsSuccess()
			&& ReuseRejected.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentBindingConflict);
	TestTrue(TEXT("Malformed explicit bindings remain nested and non-mutating"),
		!InvalidSubject.IsSuccess()
			&& InvalidSubject.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectInvalid
			&& !MissingComponent.IsSuccess()
			&& MissingComponent.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentUnavailable
			&& Bridge.GetBindingCount() == 1);
	TestTrue(TEXT("Drain never permits component replacement or history reset"),
		Applied.IsSuccess() && Removed.IsSuccess() && Bridge.IsDrained()
			&& !DrainedReplacementRejected.IsSuccess()
			&& DrainedReplacementRejected.Binding.Status
				== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					SubjectBindingConflict
			&& OriginalReplay.IsSuccess()
			&& OriginalReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductBindingStatus::
					BindingReplayed
			&& Bridge.GetCompletedTransactionCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductBridgeRouteFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge.RouteAndReplayFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductBridgeRouteFenceTest::RunTest(
	const FString&)
{
	FProductBridgeFixture Fixture;
	FProductBridgeFixture ForeignFixture;
	FProductConsumerCommands Commands;
	FProductConsumerCommands UnboundCommands;
	FProductConsumerCommands ForeignCommands;
	if (!Fixture.Start(5) || !ForeignFixture.Start(6)
		|| !BuildProductConsumerCommands(Fixture, 5, Commands)
		|| !BuildProductConsumerCommands(Fixture, 6, UnboundCommands)
		|| !BuildProductConsumerCommands(
			ForeignFixture, 7, ForeignCommands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Bridge;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
		Fixture.Host, Bridge));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	check(Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes).IsSuccess());
	const auto Unbound = Bridge.TryRoute(
		Fixture.Host, UnboundCommands.Apply);
	const auto Invalid = Bridge.TryRoute(
		Fixture.Host,
		Fdemo_mapShanmenFormationInfluenceConsumerCommand());
	const auto WrongScope = Bridge.TryRoute(
		Fixture.Host, ForeignCommands.Apply);
	const auto Applied = Bridge.TryRoute(Fixture.Host, Commands.Apply);
	const auto Removed = Bridge.TryRoute(Fixture.Host, Commands.Remove);
	const auto HistoricalApply = Bridge.TryRoute(
		Fixture.Host, Commands.Apply);

	TestTrue(TEXT("Nested command and binding failures stay visible"),
		!Unbound.IsSuccess() && Unbound.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					RouteRejected
			&& Unbound.Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					SubjectUnbound
			&& !Invalid.IsSuccess()
			&& Invalid.Route.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
					CommandInvalid
			&& !WrongScope.IsSuccess()
			&& WrongScope.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					CommandProductIdentityMismatch
			&& Bridge.GetCompletedTransactionCount() == 2);
	TestTrue(TEXT("Historical Apply cannot resurrect after exact Remove"),
		Applied.IsSuccess() && Removed.IsSuccess()
			&& HistoricalApply.IsSuccess()
			&& HistoricalApply.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					TransactionReplayed
			&& HistoricalApply.Route.Transaction.bTransactionReplayed
			&& !HistoricalApply.bBridgeStateChanged
			&& Bridge.IsDrained()
			&& Bridge.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerProductBridgeCommandIdentityTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge.CommandProductIdentityFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerProductBridgeCommandIdentityTest::
RunTest(const FString&)
{
	FProductBridgeFixture Fixture;
	FProductBridgeFixture SameScopeForeignDeployment;
	FProductBridgeFixture ForeignFixture;
	FProductConsumerCommands Commands;
	FProductConsumerCommands SameScopeForeignCommands;
	FProductConsumerCommands ForeignCommands;
	if (!Fixture.Start(8) || !SameScopeForeignDeployment.Start(8, 88)
		|| !ForeignFixture.Start(9)
		|| !BuildProductConsumerCommands(Fixture, 8, Commands)
		|| !BuildProductConsumerCommands(
			SameScopeForeignDeployment, 9, SameScopeForeignCommands)
		|| !BuildProductConsumerCommands(
			ForeignFixture, 10, ForeignCommands))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerProductBridge Bridge;
	check(Fdemo_mapShanmenFormationInfluenceConsumerProductBridge::TryOpen(
		Fixture.Host, Bridge));
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* ForeignDeploymentAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	check(Bridge.TryBindSubject(
		Fixture.Host, Commands.SubjectEntityId, Attributes).IsSuccess());
	check(Bridge.TryBindSubject(
		Fixture.Host, SameScopeForeignCommands.SubjectEntityId,
		ForeignDeploymentAttributes).IsSuccess());

	const auto SameScopeForeignRoute = Bridge.TryRoute(
		Fixture.Host, SameScopeForeignCommands.Apply);
	const bool bSameScopeForeignWasNonMutating =
		Bridge.GetCompletedTransactionCount() == 0
		&& ForeignDeploymentAttributes->GetActiveModifierCount() == 0;
	const auto ForeignRoute = Bridge.TryRoute(
		Fixture.Host, ForeignCommands.Apply);
	const bool bForeignWasNonMutating =
		Bridge.GetCompletedTransactionCount() == 0
		&& Attributes->GetActiveModifierCount() == 0
		&& ForeignDeploymentAttributes->GetActiveModifierCount() == 0;
	const auto Applied = Bridge.TryRoute(Fixture.Host, Commands.Apply);
	const auto Removed = Bridge.TryRoute(Fixture.Host, Commands.Remove);

	TestTrue(TEXT("Same run and content cannot cross deployment identity"),
		Fixture.Correlation == SameScopeForeignDeployment.Correlation
			&& Fixture.GetAction().GetActivationId()
				== SameScopeForeignDeployment.GetAction().GetActivationId()
			&& Fixture.GetAction().GetSourceEntityId()
				== SameScopeForeignDeployment.GetAction().GetSourceEntityId()
			&& Fixture.Host.GetSession().GetDeployment().GetDeploymentId()
				!= SameScopeForeignDeployment.Host.GetSession()
					.GetDeployment().GetDeploymentId()
			&& !Bridge.MatchesCommandProductIdentity(
				SameScopeForeignCommands.Apply)
			&& !SameScopeForeignRoute.IsSuccess()
			&& SameScopeForeignRoute.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					CommandProductIdentityMismatch
			&& bSameScopeForeignWasNonMutating);
	TestTrue(TEXT("Foreign run, owner, source, and content fail at bridge"),
		!Bridge.MatchesCommandProductIdentity(ForeignCommands.Apply)
			&& !ForeignRoute.IsSuccess()
			&& ForeignRoute.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRouteStatus::
					CommandProductIdentityMismatch
			&& bForeignWasNonMutating);
	TestTrue(TEXT("Exact product commands remain routable and reversible"),
		Bridge.MatchesCommandProductIdentity(Commands.Apply)
			&& Applied.IsSuccess() && Removed.IsSuccess()
			&& Bridge.IsDrained()
			&& Bridge.GetCompletedTransactionCount() == 2
			&& Attributes->GetActiveModifierCount() == 0);
	return true;
}

#endif
