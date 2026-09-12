#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenFormationDeployment.h"

namespace
{
	const FGuid FormationRunId(0xF8000001, 0, 0, 1);
	const FGuid FormationOwnerId(0xF8000002, 0, 0, 1);
	const FGuid FormationSourceEntityId(0xF8000003, 0, 0, 1);
	const FName AnchorEastId(TEXT("Formation.Anchor.East"));
	const FName AnchorNorthId(TEXT("Formation.Anchor.North"));
	const FName FormationWoodId(TEXT("Item.Material.FormationWood"));
	const FName SpiritCoreId(TEXT("Item.Material.SpiritCore"));
	const FName SpiritStoneId(TEXT("Item.Material.SpiritStone"));

	FShanmenCombatActionSnapshot MakeFormationAction(
		const FString& Digest = TEXT("TEST-DIGEST-P8.0"),
		const FGuid& RunId = FormationRunId,
		const uint64 Sequence = 8)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = FormationOwnerId;
		Capture.SourceEntityId = FormationSourceEntityId;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P8.0");
		Capture.Content.Digest = Digest;
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			Sequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenFormationMaterialRequirementCapture MakeRequirement(
		const int32 Order,
		const FName DefinitionId,
		const int32 Quantity)
	{
		FShanmenFormationMaterialRequirementCapture Requirement;
		Requirement.Order = Order;
		Requirement.MaterialDefinitionId = DefinitionId;
		Requirement.Quantity = Quantity;
		return Requirement;
	}

	FShanmenFormationAnchorCapture MakeAnchor(
		const int32 Order,
		const FName AnchorId,
		const FVector& Offset,
		const TArray<FShanmenFormationMaterialRequirementCapture>& Requirements)
	{
		FShanmenFormationAnchorCapture Anchor;
		Anchor.Order = Order;
		Anchor.AnchorDefinitionId = AnchorId;
		Anchor.RelativeOffset = Offset;
		Anchor.Requirements = Requirements;
		return Anchor;
	}

	FShanmenFormationDiagramCapture MakeDiagramCapture()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId = TEXT("Formation.Diagram.FoundationTest.r1");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.ActivationEnergy.FoundationTest.r1");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 12.0f;
		Capture.Anchors.Add(MakeAnchor(
			1,
			AnchorNorthId,
			FVector(0.0, 200.0, 0.0),
			{ MakeRequirement(0, SpiritStoneId, 2) }));
		Capture.Anchors.Add(MakeAnchor(
			0,
			AnchorEastId,
			FVector(100.0, 0.0, 0.0),
			{
				MakeRequirement(1, SpiritCoreId, 1),
				MakeRequirement(0, FormationWoodId, 3)
			}));
		return Capture;
	}

	FShanmenFormationDiagramDefinition MakeDiagram()
	{
		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			MakeDiagramCapture(), Diagram));
		return Diagram;
	}

	void StartAction(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime)
	{
		FShanmenActionTransitionReceipt Receipt;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Receipt));
	}

	void AdvanceActionToActive(FShanmenActionOrchestrator& Runtime)
	{
		FShanmenActionTransitionReceipt Receipt;
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Receipt));
	}

	FShanmenFormationMaterialFulfillmentLine MakeLine(
		const uint32 Identity,
		const FName DefinitionId,
		const int32 Quantity)
	{
		FShanmenFormationMaterialFulfillmentLine Line;
		Line.ItemInstanceId = FGuid(Identity, 0, 0, Identity);
		Line.MaterialDefinitionId = DefinitionId;
		Line.Quantity = Quantity;
		return Line;
	}

	FShanmenFormationAnchorFulfillmentEvidence MakeEastEvidence(
		const FShanmenFormationDeployment& Deployment,
		const bool bReverseLines = false)
	{
		FShanmenFormationAnchorFulfillmentEvidence Evidence;
		Evidence.FulfillmentId = FGuid(0xF8100001, 0, 0, 1);
		Evidence.RunId = Deployment.GetAction().GetRunId();
		Evidence.OwnerId = Deployment.GetAction().GetOwnerId();
		Evidence.DeploymentId = Deployment.GetDeploymentId();
		Evidence.AnchorDefinitionId = AnchorEastId;
		Evidence.Content = Deployment.GetAction().GetContent();
		Evidence.AuthorityRevision = 31;
		Evidence.Lines = {
			MakeLine(0xF8200001, FormationWoodId, 1),
			MakeLine(0xF8200002, FormationWoodId, 2),
			MakeLine(0xF8200003, SpiritCoreId, 1)
		};
		if (bReverseLines)
		{
			Algo::Reverse(Evidence.Lines);
		}
		return Evidence;
	}

	FShanmenFormationAnchorFulfillmentEvidence MakeNorthEvidence(
		const FShanmenFormationDeployment& Deployment)
	{
		FShanmenFormationAnchorFulfillmentEvidence Evidence;
		Evidence.FulfillmentId = FGuid(0xF8100002, 0, 0, 2);
		Evidence.RunId = Deployment.GetAction().GetRunId();
		Evidence.OwnerId = Deployment.GetAction().GetOwnerId();
		Evidence.DeploymentId = Deployment.GetDeploymentId();
		Evidence.AnchorDefinitionId = AnchorNorthId;
		Evidence.Content = Deployment.GetAction().GetContent();
		Evidence.AuthorityRevision = 32;
		Evidence.Lines = {
			MakeLine(0xF8200004, SpiritStoneId, 2)
		};
		return Evidence;
	}

	void MakeActiveDeployment(
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenFormationDeployment& OutDeployment)
	{
		const FShanmenCombatActionSnapshot Action = MakeFormationAction();
		StartAction(Action, OutRuntime);
		AdvanceActionToActive(OutRuntime);
		check(FShanmenFormationDeployment::TryCreate(
			Action,
			MakeDiagram(),
			FVector(10.0, 20.0, 30.0),
			FVector(1.0, 0.0, 0.0),
			OutDeployment));
		FShanmenFormationDeploymentReceipt Receipt;
		check(OutDeployment.TryBeginDeployment(OutRuntime, Receipt));
		check(OutDeployment.TryCommitAnchor(
			OutRuntime, MakeEastEvidence(OutDeployment), Receipt));
		check(OutDeployment.TryCommitAnchor(
			OutRuntime, MakeNorthEvidence(OutDeployment), Receipt));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenFormationDiagramContractTest,
	"Shanmen.0_0_10.CombatRuntime.FormationDeployment.DiagramAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenFormationDiagramContractTest::RunTest(const FString&)
{
	const FShanmenFormationDiagramDefinition Diagram = MakeDiagram();
	TestTrue(TEXT("Diagram capture freezes energy, anchor and material requirements"),
		Diagram.IsValid()
		&& Diagram.GetActivationEnergyCost().IsValid()
		&& Diagram.GetActivationEnergyCost().GetRuleId()
			== FName(TEXT("Formation.ActivationEnergy.FoundationTest.r1"))
		&& Diagram.GetActivationEnergyCost().GetResourceChannel()
			== FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel()
		&& Diagram.GetActivationEnergyCost().GetAmount() == 12.0f
		&& Diagram.GetAnchors().Num() == 2
		&& Diagram.GetAnchors()[0].GetAnchorDefinitionId() == AnchorEastId
		&& Diagram.GetAnchors()[0].GetRequirements()[0]
			.GetMaterialDefinitionId() == FormationWoodId
		&& Diagram.GetAnchors()[0].GetRequirements()[1]
			.GetMaterialDefinitionId() == SpiritCoreId);

	FShanmenFormationDiagramCapture Invalid = MakeDiagramCapture();
	Invalid.Anchors[1].Order = 1;
	FShanmenFormationDiagramDefinition Rejected;
	TestFalse(TEXT("Duplicate anchor order fails closed"),
		FShanmenFormationDiagramDefinition::TryCapture(
			Invalid, Rejected));
	Invalid = MakeDiagramCapture();
	Invalid.Anchors[1].Requirements[0].MaterialDefinitionId =
		FormationWoodId;
	TestFalse(TEXT("Duplicate requirement identity in one anchor fails closed"),
		FShanmenFormationDiagramDefinition::TryCapture(
			Invalid, Rejected));
	Invalid = MakeDiagramCapture();
	Invalid.ActivationEnergyCost = FShanmenActionResourceCostCapture();
	TestFalse(TEXT("A diagram without an activation energy requirement fails closed"),
		FShanmenFormationDiagramDefinition::TryCapture(
			Invalid, Rejected));
	Invalid = MakeDiagramCapture();
	Invalid.ActivationEnergyCost.ResourceChannel =
		FShanmenCombatNativeTags::SourcePlayer();
	TestFalse(TEXT("A non-SpiritEnergy activation channel fails closed"),
		FShanmenFormationDiagramDefinition::TryCapture(
			Invalid, Rejected));
	Invalid = MakeDiagramCapture();
	Invalid.ActivationEnergyCost.Amount = 0.0f;
	TestFalse(TEXT("A non-positive activation energy amount fails closed"),
		FShanmenFormationDiagramDefinition::TryCapture(
			Invalid, Rejected));

	const FShanmenCombatActionSnapshot Action = MakeFormationAction();
	FShanmenFormationDeployment First;
	FShanmenFormationDeployment Replay;
	TestTrue(TEXT("Finite planar placement creates deterministic anchor instances"),
		FShanmenFormationDeployment::TryCreate(
			Action,
			Diagram,
			FVector(10.0, 20.0, 30.0),
			FVector(10.0, 0.0, 7.0),
			First)
		&& FShanmenFormationDeployment::TryCreate(
			Action,
			Diagram,
			FVector(10.0, 20.0, 30.0),
			FVector(2.0, 0.0, -9.0),
			Replay)
		&& First.GetDeploymentId() == Replay.GetDeploymentId()
		&& First.GetForward() == FVector::ForwardVector
		&& First.GetAnchors()[0].GetWorldLocation()
			== FVector(110.0, 20.0, 30.0)
		&& First.GetAnchors()[1].GetWorldLocation()
			== FVector(10.0, 220.0, 30.0));

	FShanmenFormationDeployment ChangedContent;
	TestTrue(TEXT("Content identity isolates otherwise equal deployments"),
		FShanmenFormationDeployment::TryCreate(
			MakeFormationAction(TEXT("CHANGED-DIGEST")),
			Diagram,
			FVector(10.0, 20.0, 30.0),
			FVector::ForwardVector,
			ChangedContent)
		&& ChangedContent.GetDeploymentId() != First.GetDeploymentId());
	FShanmenFormationDiagramCapture ChangedGeometryCapture =
		MakeDiagramCapture();
	ChangedGeometryCapture.Anchors[1].RelativeOffset.X = 101.0;
	FShanmenFormationDiagramDefinition ChangedGeometryDiagram;
	check(FShanmenFormationDiagramDefinition::TryCapture(
		ChangedGeometryCapture, ChangedGeometryDiagram));
	FShanmenFormationDeployment ChangedGeometry;
	TestTrue(TEXT("Diagram structure participates directly in deployment identity"),
		FShanmenFormationDeployment::TryCreate(
			Action,
			ChangedGeometryDiagram,
			FVector(10.0, 20.0, 30.0),
			FVector::ForwardVector,
			ChangedGeometry)
		&& ChangedGeometry.GetDeploymentId() != First.GetDeploymentId());
	FShanmenFormationDiagramCapture ChangedEnergyCapture =
		MakeDiagramCapture();
	ChangedEnergyCapture.ActivationEnergyCost.Amount = 13.0f;
	FShanmenFormationDiagramDefinition ChangedEnergyDiagram;
	check(FShanmenFormationDiagramDefinition::TryCapture(
		ChangedEnergyCapture, ChangedEnergyDiagram));
	FShanmenFormationDeployment ChangedEnergy;
	TestTrue(TEXT("Activation energy participates directly in deployment identity"),
		FShanmenFormationDeployment::TryCreate(
			Action,
			ChangedEnergyDiagram,
			FVector(10.0, 20.0, 30.0),
			FVector::ForwardVector,
			ChangedEnergy)
		&& ChangedEnergy.GetDeploymentId() != First.GetDeploymentId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenFormationDeploymentProgressTest,
	"Shanmen.0_0_10.CombatRuntime.FormationDeployment.ProgressAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenFormationDeploymentProgressTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeFormationAction();
	FShanmenActionOrchestrator Runtime;
	StartAction(Action, Runtime);
	FShanmenFormationDeployment Deployment;
	check(FShanmenFormationDeployment::TryCreate(
		Action,
		MakeDiagram(),
		FVector::ZeroVector,
		FVector::ForwardVector,
		Deployment));

	FShanmenFormationDeploymentReceipt Begin;
	TestFalse(TEXT("Deployment cannot begin before the action commit point"),
		Deployment.TryBeginDeployment(Runtime, Begin));
	AdvanceActionToActive(Runtime);
	TestTrue(TEXT("Active action opens deployment exactly once"),
		Deployment.TryBeginDeployment(Runtime, Begin)
		&& Begin.IsValid()
		&& Begin.GetSequence() == 0
		&& Deployment.GetState()
			== EShanmenFormationDeploymentState::Deploying);
	FShanmenFormationDeploymentReceipt BeginReplay;
	TestTrue(TEXT("Begin replay returns the original receipt"),
		Deployment.TryBeginDeployment(Runtime, BeginReplay)
		&& BeginReplay.GetReceiptId() == Begin.GetReceiptId()
		&& Deployment.GetReceipts().Num() == 1);

	FShanmenFormationDeploymentReceipt East;
	TestTrue(TEXT("Exact split stacks satisfy one ordered material requirement"),
		Deployment.TryCommitAnchor(
			Runtime, MakeEastEvidence(Deployment), East)
		&& East.IsValid()
		&& East.GetSequence() == 1
		&& East.GetCommittedAnchorCount() == 1
		&& Deployment.GetState()
			== EShanmenFormationDeploymentState::Deploying);
	FShanmenFormationDeploymentReceipt EastReplay;
	TestTrue(TEXT("Permuted physical lines replay the same canonical commit"),
		Deployment.TryCommitAnchor(
			Runtime, MakeEastEvidence(Deployment, true), EastReplay)
		&& EastReplay.GetReceiptId() == East.GetReceiptId()
		&& Deployment.GetReceipts().Num() == 2);

	FShanmenFormationDeploymentReceipt North;
	TestTrue(TEXT("The final exact anchor atomically activates the formation"),
		Deployment.TryCommitAnchor(
			Runtime, MakeNorthEvidence(Deployment), North)
		&& North.GetStateAfter()
			== EShanmenFormationDeploymentState::Active
		&& North.GetCommittedAnchorCount() == 2
		&& Deployment.GetState()
			== EShanmenFormationDeploymentState::Active
		&& Deployment.IsValid());

	FShanmenFormationAnchorFulfillmentEvidence Conflict =
		MakeEastEvidence(Deployment);
	Conflict.FulfillmentId = FGuid(0xF8100011, 0, 0, 11);
	FShanmenFormationDeploymentReceipt Rejected;
	TestFalse(TEXT("A committed anchor rejects conflicting replacement evidence"),
		Deployment.TryCommitAnchor(Runtime, Conflict, Rejected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenFormationFulfillmentBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.FormationDeployment.FulfillmentFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenFormationFulfillmentBoundaryTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeFormationAction();
	FShanmenActionOrchestrator Runtime;
	StartAction(Action, Runtime);
	AdvanceActionToActive(Runtime);
	FShanmenFormationDeployment Deployment;
	check(FShanmenFormationDeployment::TryCreate(
		Action,
		MakeDiagram(),
		FVector::ZeroVector,
		FVector::ForwardVector,
		Deployment));
	FShanmenFormationDeploymentReceipt Receipt;
	check(Deployment.TryBeginDeployment(Runtime, Receipt));

	auto ExpectRejected = [this, &Deployment, &Runtime](
		const TCHAR* Label,
		const FShanmenFormationAnchorFulfillmentEvidence& Evidence)
	{
		const int32 ReceiptCount = Deployment.GetReceipts().Num();
		FShanmenFormationDeploymentReceipt Rejected;
		TestFalse(Label, Deployment.TryCommitAnchor(
			Runtime, Evidence, Rejected));
		TestTrue(TEXT("Rejected fulfillment remains mutation-free"),
			Deployment.GetState()
				== EShanmenFormationDeploymentState::Deploying
			&& Deployment.GetCommittedAnchorCount() == 0
			&& Deployment.GetReceipts().Num() == ReceiptCount
			&& Deployment.IsValid());
	};

	FShanmenFormationAnchorFulfillmentEvidence Invalid =
		MakeEastEvidence(Deployment);
	Invalid.RunId = FGuid(0xF8300001, 0, 0, 1);
	ExpectRejected(TEXT("Wrong Run evidence is rejected"), Invalid);
	Invalid = MakeEastEvidence(Deployment);
	Invalid.OwnerId = FGuid(0xF8300002, 0, 0, 2);
	ExpectRejected(TEXT("Wrong owner evidence is rejected"), Invalid);
	Invalid = MakeEastEvidence(Deployment);
	Invalid.Content.Digest = TEXT("FOREIGN-CONTENT");
	ExpectRejected(TEXT("Wrong content evidence is rejected"), Invalid);
	Invalid = MakeEastEvidence(Deployment);
	Invalid.Lines[1].Quantity = 1;
	ExpectRejected(TEXT("Insufficient material quantity is rejected"), Invalid);
	Invalid = MakeEastEvidence(Deployment);
	Invalid.Lines.Add(MakeLine(0xF8200005, SpiritStoneId, 1));
	ExpectRejected(TEXT("Extra material definition is rejected"), Invalid);
	Invalid = MakeEastEvidence(Deployment);
	Invalid.Lines[1].ItemInstanceId = Invalid.Lines[0].ItemInstanceId;
	ExpectRejected(TEXT("Duplicate physical item identity is rejected"), Invalid);
	Invalid = MakeEastEvidence(Deployment);
	Invalid.AnchorDefinitionId = TEXT("Formation.Anchor.Unknown");
	ExpectRejected(TEXT("Unknown anchor evidence is rejected"), Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenFormationTerminationTest,
	"Shanmen.0_0_10.CombatRuntime.FormationDeployment.Termination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenFormationTerminationTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeFormationAction();
	FShanmenActionOrchestrator Runtime;
	StartAction(Action, Runtime);
	FShanmenFormationDeployment Planned;
	check(FShanmenFormationDeployment::TryCreate(
		Action, MakeDiagram(), FVector::ZeroVector,
		FVector::ForwardVector, Planned));
	FShanmenFormationDeploymentReceipt Cancel;
	TestTrue(TEXT("A planned deployment can cancel without inventing progress"),
		Planned.TryCancel(Runtime, Cancel)
		&& Cancel.GetStateBefore()
			== EShanmenFormationDeploymentState::Planned
		&& Planned.GetState()
			== EShanmenFormationDeploymentState::Cancelled
		&& Planned.GetCommittedAnchorCount() == 0);
	FShanmenFormationDeploymentReceipt CancelReplay;
	TestTrue(TEXT("Cancellation replay is idempotent"),
		Planned.TryCancel(Runtime, CancelReplay)
		&& CancelReplay.GetReceiptId() == Cancel.GetReceiptId()
		&& Planned.GetReceipts().Num() == 1);

	FShanmenActionOrchestrator ActiveRuntime;
	FShanmenFormationDeployment Active;
	MakeActiveDeployment(ActiveRuntime, Active);
	FShanmenFormationDeploymentReceipt Rejected;
	TestFalse(TEXT("An active formation cannot be retroactively cancelled"),
		Active.TryCancel(ActiveRuntime, Rejected));
	FShanmenFormationDeploymentReceipt End;
	TestTrue(TEXT("An active formation has one explicit terminal end"),
		Active.TryEnd(ActiveRuntime, End)
		&& End.IsValid()
		&& End.GetStateAfter() == EShanmenFormationDeploymentState::Ended
		&& Active.GetState() == EShanmenFormationDeploymentState::Ended
		&& Active.IsValid());
	FShanmenFormationDeploymentReceipt EndReplay;
	TestTrue(TEXT("End replay returns the original receipt"),
		Active.TryEnd(ActiveRuntime, EndReplay)
		&& EndReplay.GetReceiptId() == End.GetReceiptId()
		&& Active.GetReceipts().Num() == 4);

	FShanmenActionOrchestrator ForeignRuntime;
	StartAction(MakeFormationAction(
		TEXT("TEST-DIGEST-P8.0"),
		FGuid(0xF8000011, 0, 0, 11),
		18), ForeignRuntime);
	AdvanceActionToActive(ForeignRuntime);
	FShanmenFormationDeployment Fresh;
	check(FShanmenFormationDeployment::TryCreate(
		Action, MakeDiagram(), FVector::ZeroVector,
		FVector::ForwardVector, Fresh));
	TestFalse(TEXT("A foreign Run cannot begin this deployment"),
		Fresh.TryBeginDeployment(ForeignRuntime, Rejected));
	TestFalse(TEXT("A foreign Run cannot replay another deployment's end receipt"),
		Active.TryEnd(ForeignRuntime, Rejected));
	return true;
}

#endif
