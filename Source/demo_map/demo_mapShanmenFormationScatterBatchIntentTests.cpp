#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationScatterBatchIntent.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"

namespace
{
	using EPlanStatus = Edemo_mapShanmenFormationScatterBatchPlanStatus;

	const FGuid RunId(0xF8F17001, 0, 0, 1);
	const FGuid OwnerId(0xF8F17002, 0, 0, 2);
	const FGuid SourceEntityId(0xF8F17003, 0, 0, 3);
	const FGuid OperationId(0xF8F17004, 0, 0, 4);
	const FName EastAnchor(TEXT("Formation.Anchor.P27_17.East"));
	const FName NorthAnchor(TEXT("Formation.Anchor.P27_17.North"));
	const FName WoodMaterial(TEXT("Item.Material.P27_17.Wood"));
	const FName StoneMaterial(TEXT("Item.Material.P27_17.Stone"));

	FShanmenContentStamp MakeContent(const TCHAR* Digest)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.17.Test");
		Content.Digest = Digest;
		return Content;
	}

	FShanmenFormationMaterialRequirementCapture MakeRequirement(
		const int32 Order,
		const FName MaterialDefinitionId,
		const int32 Quantity)
	{
		FShanmenFormationMaterialRequirementCapture Requirement;
		Requirement.Order = Order;
		Requirement.MaterialDefinitionId = MaterialDefinitionId;
		Requirement.Quantity = Quantity;
		return Requirement;
	}

	FShanmenFormationAnchorCapture MakeAnchor(
		const int32 Order,
		const FName AnchorDefinitionId,
		const FVector& RelativeOffset,
		const TArray<FShanmenFormationMaterialRequirementCapture>&
			Requirements)
	{
		FShanmenFormationAnchorCapture Anchor;
		Anchor.Order = Order;
		Anchor.AnchorDefinitionId = AnchorDefinitionId;
		Anchor.RelativeOffset = RelativeOffset;
		Anchor.Requirements = Requirements;
		return Anchor;
	}

	FShanmenFormationDiagramDefinition MakeDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.P27_17.ScatterBatch");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.Energy.P27_17.ScatterBatch");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 1.0f;
		Capture.Anchors = {
			MakeAnchor(
				1,
				NorthAnchor,
				FVector(0.0, 100.0, 0.0),
				{ MakeRequirement(0, WoodMaterial, 3) }),
			MakeAnchor(
				0,
				EastAnchor,
				FVector(100.0, 0.0, 5.0),
				{
					MakeRequirement(1, StoneMaterial, 1),
					MakeRequirement(0, WoodMaterial, 2)
				})
		};

		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			Capture, Diagram));
		return Diagram;
	}

	FShanmenCombatActionSnapshot MakeAction(
		const FShanmenContentStamp& Content,
		const uint64 Sequence = 17)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = OwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.Content = Content;
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			Sequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	void MakeDeployment(
		const FShanmenContentStamp& Content,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenFormationDeployment& OutDeployment,
		const uint64 Sequence = 17)
	{
		const FShanmenCombatActionSnapshot Action =
			MakeAction(Content, Sequence);
		FShanmenActionTransitionReceipt Receipt;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Receipt));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Receipt));
		check(FShanmenFormationDeployment::TryCreate(
			Action,
			MakeDiagram(),
			FVector(10.0, 20.0, 0.0),
			FVector::ForwardVector,
			OutDeployment));
		FShanmenFormationDeploymentReceipt Begin;
		check(OutDeployment.TryBeginDeployment(OutRuntime, Begin));
	}

	Fdemo_mapShanmenFormationMasteryProjectionResult MakeProjection(
		const FShanmenContentStamp& Content,
		const int64 Revision,
		const EShanmenFormationMasteryTier Tier =
			EShanmenFormationMasteryTier::Master)
	{
		auto ReadAuthority = [&Content, Revision, Tier](
			const FGuid&,
			Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
			FString& OutDiagnostic)
		{
			OutCapture.OwnerId = OwnerId;
			OutCapture.AuthorityRevision = Revision;
			OutCapture.Content = Content;
			OutCapture.MasteryTier = Tier;
			OutDiagnostic = TEXT("P27.17 fixture authority read.");
			return true;
		};
		return Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, OwnerId, ReadAuthority);
	}

	Fdemo_mapShanmenFormationMasteryOperationAuthorization MakeAuthorization(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const EShanmenFormationMaterialDeliveryMode Mode =
			EShanmenFormationMaterialDeliveryMode::ScatterFormation,
		const FName AnchorDefinitionId = NAME_None)
	{
		const auto Result =
			Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
				Projection,
				Deployment,
				OperationId,
				Mode,
				AnchorDefinitionId);
		check(Result.IsAuthorized());
		return Result.Authorization;
	}

	FShanmenFormationAnchorFulfillmentEvidence MakeEastEvidence(
		const FShanmenFormationDeployment& Deployment)
	{
		FShanmenFormationMaterialFulfillmentLine Wood;
		Wood.ItemInstanceId = FGuid(0xF8F17100, 0, 0, 1);
		Wood.MaterialDefinitionId = WoodMaterial;
		Wood.Quantity = 2;

		FShanmenFormationMaterialFulfillmentLine Stone;
		Stone.ItemInstanceId = FGuid(0xF8F17101, 0, 0, 2);
		Stone.MaterialDefinitionId = StoneMaterial;
		Stone.Quantity = 1;

		FShanmenFormationAnchorFulfillmentEvidence Evidence;
		Evidence.FulfillmentId = FGuid(0xF8F17102, 0, 0, 3);
		Evidence.RunId = Deployment.GetAction().GetRunId();
		Evidence.OwnerId = Deployment.GetAction().GetOwnerId();
		Evidence.DeploymentId = Deployment.GetDeploymentId();
		Evidence.AnchorDefinitionId = EastAnchor;
		Evidence.Content = Deployment.GetAction().GetContent();
		Evidence.AuthorityRevision = 17;
		Evidence.Lines = { Stone, Wood };
		return Evidence;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterBatchDeterministicPlanTest,
	"Shanmen.0_0_10.Product.FormationScatterBatchIntent.DeterministicCanonicalPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterBatchDeterministicPlanTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("determinism"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Projection = MakeProjection(Content, 17);
	const auto Authorization = MakeAuthorization(Projection, Deployment);

	const auto First = Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
		Projection, Deployment, Authorization);
	const auto Replay = Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
		Projection, Deployment, Authorization);
	TestTrue(TEXT("Exact scatter evidence deterministically replays"),
		First.IsPlanned()
			&& Replay.IsPlanned()
			&& First.Batch == Replay.Batch
			&& First.Batch.GetBatchIntentId()
				== Replay.Batch.GetBatchIntentId());
	TestTrue(TEXT("Batch binds authorization and complete shape"),
		First.Batch.GetAuthorization() == Authorization
			&& First.Batch.GetAnchorIntents().Num() == 2
			&& First.Batch.GetTotalRequirementCount() == 3
			&& First.Batch.GetTotalMaterialQuantity() == 6
			&& Fdemo_mapShanmenFormationScatterBatchPlanner::
				IsCurrentBatch(Projection, Deployment, First.Batch));

	const auto& East = First.Batch.GetAnchorIntents()[0];
	const auto& North = First.Batch.GetAnchorIntents()[1];
	TestTrue(TEXT("Authored reverse input is emitted in canonical anchor order"),
		East.IsValid()
			&& East.GetAnchorOrder() == 0
			&& East.GetAnchorDefinitionId() == EastAnchor
			&& East.GetWorldLocation() == FVector(110.0, 20.0, 5.0)
			&& North.IsValid()
			&& North.GetAnchorOrder() == 1
			&& North.GetAnchorDefinitionId() == NorthAnchor
			&& North.GetWorldLocation() == FVector(10.0, 120.0, 0.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterBatchRequirementIsolationTest,
	"Shanmen.0_0_10.Product.FormationScatterBatchIntent.RequirementIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterBatchRequirementIsolationTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("requirements"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Projection = MakeProjection(Content, 18);
	const auto Authorization = MakeAuthorization(Projection, Deployment);
	const auto Result = Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
		Projection, Deployment, Authorization);
	check(Result.IsPlanned());

	const auto& East = Result.Batch.GetAnchorIntents()[0];
	const auto& North = Result.Batch.GetAnchorIntents()[1];
	const auto& EastWood = East.GetMaterialIntents()[0];
	const auto& EastStone = East.GetMaterialIntents()[1];
	const auto& NorthWood = North.GetMaterialIntents()[0];
	TestTrue(TEXT("Requirement order and quantity remain exact per anchor"),
		East.GetMaterialIntents().Num() == 2
			&& EastWood.GetRequirementOrder() == 0
			&& EastWood.GetMaterialDefinitionId() == WoodMaterial
			&& EastWood.GetQuantity() == 2
			&& EastStone.GetRequirementOrder() == 1
			&& EastStone.GetMaterialDefinitionId() == StoneMaterial
			&& EastStone.GetQuantity() == 1
			&& North.GetMaterialIntents().Num() == 1
			&& NorthWood.GetRequirementOrder() == 0
			&& NorthWood.GetMaterialDefinitionId() == WoodMaterial
			&& NorthWood.GetQuantity() == 3);
	TestTrue(TEXT("Shared material types are never merged across anchors"),
		EastWood.IsValid()
			&& NorthWood.IsValid()
			&& EastWood.GetIntentId() != NorthWood.GetIntentId()
			&& EastWood.GetAnchorInstanceId()
				!= NorthWood.GetAnchorInstanceId()
			&& East.GetTotalMaterialQuantity() == 3
			&& North.GetTotalMaterialQuantity() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterBatchAuthorizationFenceTest,
	"Shanmen.0_0_10.Product.FormationScatterBatchIntent.AuthorizationFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterBatchAuthorizationFenceTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("fences"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Projection = MakeProjection(Content, 19);
	const auto ScatterAuthorization = MakeAuthorization(
		Projection, Deployment);
	const auto AnchorAuthorization = MakeAuthorization(
		Projection,
		Deployment,
		EShanmenFormationMaterialDeliveryMode::RemoteThrow,
		EastAnchor);

	Fdemo_mapShanmenFormationMasteryOperationAuthorization InvalidAuthorization;
	const auto InvalidAuth =
		Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
			Projection, Deployment, InvalidAuthorization);
	const auto WrongMode =
		Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
			Projection, Deployment, AnchorAuthorization);
	Fdemo_mapShanmenFormationMasteryProjectionResult InvalidProjection;
	const auto InvalidMastery =
		Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
			InvalidProjection, Deployment, ScatterAuthorization);
	FShanmenFormationDeployment InvalidDeployment;
	const auto InvalidDeploymentResult =
		Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
			Projection, InvalidDeployment, ScatterAuthorization);
	TestTrue(TEXT("Invalid inputs and non-scatter authorization fail closed"),
		InvalidAuth.IsValid()
			&& InvalidAuth.Status == EPlanStatus::AuthorizationInvalid
			&& WrongMode.IsValid()
			&& WrongMode.Status == EPlanStatus::AuthorizationNotScatter
			&& InvalidMastery.IsValid()
			&& InvalidMastery.Status
				== EPlanStatus::MasteryProjectionRejected
			&& InvalidDeploymentResult.IsValid()
			&& InvalidDeploymentResult.Status
				== EPlanStatus::DeploymentInvalid);

	const auto RevisedProjection = MakeProjection(Content, 20);
	const auto Revised =
		Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
			RevisedProjection, Deployment, ScatterAuthorization);
	FShanmenActionOrchestrator OtherRuntime;
	FShanmenFormationDeployment OtherDeployment;
	MakeDeployment(Content, OtherRuntime, OtherDeployment, 18);
	const auto Other = Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
		Projection, OtherDeployment, ScatterAuthorization);
	TestTrue(TEXT("Revised mastery and another deployment reject stale proof"),
		Revised.IsValid()
			&& Revised.Status == EPlanStatus::AuthorizationStale
			&& Other.IsValid()
			&& Other.Status == EPlanStatus::AuthorizationStale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationScatterBatchCurrentNoMutationTest,
	"Shanmen.0_0_10.Product.FormationScatterBatchIntent.CurrentnessAndNoMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationScatterBatchCurrentNoMutationTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("current"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Projection = MakeProjection(Content, 21);
	const auto Authorization = MakeAuthorization(Projection, Deployment);
	const int32 ReceiptsBefore = Deployment.GetReceipts().Num();
	const int32 CommittedBefore = Deployment.GetCommittedAnchorCount();
	const EShanmenFormationDeploymentState StateBefore =
		Deployment.GetState();

	const auto Planned = Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
		Projection, Deployment, Authorization);
	TestTrue(TEXT("Planning is side-effect free and current"),
		Planned.IsPlanned()
			&& Deployment.GetReceipts().Num() == ReceiptsBefore
			&& Deployment.GetCommittedAnchorCount() == CommittedBefore
			&& Deployment.GetState() == StateBefore
			&& Fdemo_mapShanmenFormationScatterBatchPlanner::
				IsCurrentBatch(Projection, Deployment, Planned.Batch));

	FShanmenFormationDeploymentReceipt Commit;
	check(Deployment.TryCommitAnchor(
		Runtime, MakeEastEvidence(Deployment), Commit));
	TestTrue(TEXT("Historical batch remains internally auditable"),
		Planned.Batch.IsValid());
	TestFalse(TEXT("Deployment progress invalidates batch currentness"),
		Fdemo_mapShanmenFormationScatterBatchPlanner::IsCurrentBatch(
			Projection, Deployment, Planned.Batch));
	const auto Stale = Fdemo_mapShanmenFormationScatterBatchPlanner::Plan(
		Projection, Deployment, Authorization);
	TestTrue(TEXT("Changed deployment rejects the old authorization"),
		Stale.IsValid()
			&& Stale.Status == EPlanStatus::AuthorizationStale
			&& Deployment.GetCommittedAnchorCount() == 1
			&& Deployment.GetReceipts().Num() == ReceiptsBefore + 1);
	return true;
}

#endif
