#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationMasteryOperationAuthorization.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"

namespace
{
	using EAuthorizationStatus =
		Edemo_mapShanmenFormationMasteryOperationAuthorizationStatus;
	using EOperationTarget =
		Edemo_mapShanmenFormationMasteryOperationTarget;

	const FGuid RunId(0xF8F16001, 0, 0, 1);
	const FGuid OwnerId(0xF8F16002, 0, 0, 2);
	const FGuid ForeignOwnerId(0xF8F16003, 0, 0, 3);
	const FGuid SourceEntityId(0xF8F16004, 0, 0, 4);
	const FGuid OperationId(0xF8F16005, 0, 0, 5);
	const FName EastAnchor(TEXT("Formation.Anchor.P27_16.East"));
	const FName NorthAnchor(TEXT("Formation.Anchor.P27_16.North"));
	const FName MaterialId(TEXT("Item.Material.P27_16.FormationWood"));

	FShanmenContentStamp MakeContent(const TCHAR* Digest)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.16.Test");
		Content.Digest = Digest;
		return Content;
	}

	FShanmenFormationAnchorCapture MakeAnchor(
		const int32 Order,
		const FName AnchorDefinitionId,
		const FVector& RelativeOffset)
	{
		FShanmenFormationMaterialRequirementCapture Requirement;
		Requirement.Order = 0;
		Requirement.MaterialDefinitionId = MaterialId;
		Requirement.Quantity = 1;

		FShanmenFormationAnchorCapture Anchor;
		Anchor.Order = Order;
		Anchor.AnchorDefinitionId = AnchorDefinitionId;
		Anchor.RelativeOffset = RelativeOffset;
		Anchor.Requirements = { Requirement };
		return Anchor;
	}

	FShanmenFormationDiagramDefinition MakeDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.P27_16.Authorization");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.Energy.P27_16.Authorization");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 1.0f;
		Capture.Anchors = {
			MakeAnchor(0, EastAnchor, FVector(100.0, 0.0, 0.0)),
			MakeAnchor(1, NorthAnchor, FVector(0.0, 100.0, 0.0))
		};

		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			Capture, Diagram));
		return Diagram;
	}

	FShanmenCombatActionSnapshot MakeAction(
		const FShanmenContentStamp& Content,
		const FGuid& RequestedOwnerId = OwnerId,
		const FGuid& RequestedRunId = RunId,
		const uint64 Sequence = 16)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RequestedRunId;
		Capture.OwnerId = RequestedOwnerId;
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
		const bool bBegin = true,
		const FGuid& RequestedOwnerId = OwnerId,
		const FGuid& RequestedRunId = RunId,
		const uint64 Sequence = 16)
	{
		const FShanmenCombatActionSnapshot Action = MakeAction(
			Content, RequestedOwnerId, RequestedRunId, Sequence);
		FShanmenActionTransitionReceipt Receipt;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, Receipt));
		check(OutRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Receipt));
		check(FShanmenFormationDeployment::TryCreate(
			Action,
			MakeDiagram(),
			FVector::ZeroVector,
			FVector::ForwardVector,
			OutDeployment));
		if (bBegin)
		{
			FShanmenFormationDeploymentReceipt Begin;
			check(OutDeployment.TryBeginDeployment(OutRuntime, Begin));
		}
	}

	Fdemo_mapShanmenFormationMasteryProjectionResult MakeProjection(
		const FShanmenContentStamp& Content,
		const FGuid& RequestedOwnerId,
		const int64 Revision,
		const EShanmenFormationMasteryTier Tier)
	{
		auto ReadAuthority = [&Content, RequestedOwnerId, Revision, Tier](
			const FGuid&,
			Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
			FString& OutDiagnostic)
		{
			OutCapture.OwnerId = RequestedOwnerId;
			OutCapture.AuthorityRevision = Revision;
			OutCapture.Content = Content;
			OutCapture.MasteryTier = Tier;
			OutDiagnostic = TEXT("P27.16 fixture authority read.");
			return true;
		};
		return Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
			Content, RequestedOwnerId, ReadAuthority);
	}

	FShanmenFormationAnchorFulfillmentEvidence MakeEvidence(
		const FShanmenFormationDeployment& Deployment,
		const FName AnchorDefinitionId)
	{
		FShanmenFormationMaterialFulfillmentLine Line;
		Line.ItemInstanceId = FGuid(0xF8F16100, 0, 0, 1);
		Line.MaterialDefinitionId = MaterialId;
		Line.Quantity = 1;

		FShanmenFormationAnchorFulfillmentEvidence Evidence;
		Evidence.FulfillmentId = FGuid(0xF8F16101, 0, 0, 1);
		Evidence.RunId = Deployment.GetAction().GetRunId();
		Evidence.OwnerId = Deployment.GetAction().GetOwnerId();
		Evidence.DeploymentId = Deployment.GetDeploymentId();
		Evidence.AnchorDefinitionId = AnchorDefinitionId;
		Evidence.Content = Deployment.GetAction().GetContent();
		Evidence.AuthorityRevision = 16;
		Evidence.Lines = { Line };
		return Evidence;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryDeterministicAnchorAuthorizationTest,
	"Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization.DeterministicAnchorAuthorization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryDeterministicAnchorAuthorizationTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("determinism"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Projection = MakeProjection(
		Content,
		OwnerId,
		16,
		EShanmenFormationMasteryTier::Beginner);

	const auto First =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Projection,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			EastAnchor);
	const auto Replay =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Projection,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			EastAnchor);
	TestTrue(TEXT("Exact operation replay has deterministic authorization"),
		First.IsAuthorized()
			&& Replay.IsAuthorized()
			&& First.Authorization == Replay.Authorization
			&& First.Authorization.GetAuthorizationId()
				== Replay.Authorization.GetAuthorizationId());
	TestTrue(TEXT("Authorization binds mastery deployment and anchor identity"),
		First.Authorization.GetOperationId() == OperationId
			&& First.Authorization.GetMasteryReadId()
				== Projection.AuthorityRead.GetReadId()
			&& First.Authorization.GetMasteryAuthorityRevision() == 16
			&& First.Authorization.GetMasteryTier()
				== EShanmenFormationMasteryTier::Beginner
			&& First.Authorization.GetRunId() == RunId
			&& First.Authorization.GetOwnerId() == OwnerId
			&& First.Authorization.GetDeploymentId()
				== Deployment.GetDeploymentId()
			&& First.Authorization.GetTarget()
				== EOperationTarget::Anchor
			&& First.Authorization.GetAnchorDefinitionId() == EastAnchor
			&& First.Authorization.GetAnchorInstanceId().IsValid()
			&& First.Authorization.GetDeploymentReceiptCount() == 1
			&& First.Authorization.GetCommittedAnchorCount() == 0
			&& First.Authorization.GetTotalAnchorCount() == 2);
	TestTrue(TEXT("Unchanged evidence remains current"),
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::
			IsCurrentAuthorization(
				Projection,
				Deployment,
				OperationId,
				EShanmenFormationMaterialDeliveryMode::ProximityFill,
				EastAnchor,
				First.Authorization));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryOperationMatrixTest,
	"Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization.CapabilityAndTargetMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryOperationMatrixTest::RunTest(const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("matrix"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Beginner = MakeProjection(
		Content, OwnerId, 1, EShanmenFormationMasteryTier::Beginner);
	const auto Intermediate = MakeProjection(
		Content, OwnerId, 2, EShanmenFormationMasteryTier::Intermediate);
	const auto Master = MakeProjection(
		Content, OwnerId, 3, EShanmenFormationMasteryTier::Master);

	const auto BeginnerRemote =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Beginner,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::RemoteThrow,
			EastAnchor);
	const auto IntermediateRemote =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Intermediate,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::RemoteThrow,
			EastAnchor);
	const auto IntermediateScatter =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Intermediate,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ScatterFormation);
	const auto MasterScatter =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Master,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ScatterFormation);
	TestTrue(TEXT("Authorizer reuses the exact mastery capability matrix"),
		BeginnerRemote.IsValid()
			&& BeginnerRemote.Status == EAuthorizationStatus::CapabilityDenied
			&& IntermediateRemote.IsAuthorized()
			&& IntermediateScatter.IsValid()
			&& IntermediateScatter.Status
				== EAuthorizationStatus::CapabilityDenied
			&& MasterScatter.IsAuthorized()
			&& MasterScatter.Authorization.GetTarget()
				== EOperationTarget::Deployment
			&& MasterScatter.Authorization.GetAnchorDefinitionId().IsNone()
			&& !MasterScatter.Authorization.GetAnchorInstanceId().IsValid());

	const auto MissingAnchorTarget =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Intermediate,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::RemoteThrow);
	const auto ScatterWithAnchor =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Master,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ScatterFormation,
			EastAnchor);
	const auto InvalidMode =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Master,
			Deployment,
			OperationId,
			static_cast<EShanmenFormationMaterialDeliveryMode>(255));
	TestTrue(TEXT("Operation modes require their exact target shape"),
		MissingAnchorTarget.IsValid()
			&& MissingAnchorTarget.Status
				== EAuthorizationStatus::TargetShapeInvalid
			&& ScatterWithAnchor.IsValid()
			&& ScatterWithAnchor.Status
				== EAuthorizationStatus::TargetShapeInvalid
			&& InvalidMode.IsValid()
			&& InvalidMode.Status
				== EAuthorizationStatus::DeliveryModeInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryOperationIdentityFenceTest,
	"Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization.IdentityAndLifecycleFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryOperationIdentityFenceTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("current"));
	const FShanmenContentStamp StaleContent = MakeContent(TEXT("stale"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto Current = MakeProjection(
		Content, OwnerId, 1, EShanmenFormationMasteryTier::Master);
	const auto Foreign = MakeProjection(
		Content, ForeignOwnerId, 1, EShanmenFormationMasteryTier::Master);
	const auto Stale = MakeProjection(
		StaleContent, OwnerId, 1, EShanmenFormationMasteryTier::Master);

	Fdemo_mapShanmenFormationMasteryProjectionResult InvalidProjection;
	const auto InvalidMastery =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			InvalidProjection,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			EastAnchor);
	const auto ForeignOwner =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Foreign,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ScatterFormation);
	const auto StaleContentResult =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Stale,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ScatterFormation);
	const auto InvalidOperation =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Current,
			Deployment,
			FGuid(),
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			EastAnchor);
	const auto MissingAnchor =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Current,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			TEXT("Formation.Anchor.P27_16.Missing"));
	TestTrue(TEXT("Authority operation and target identities fail closed"),
		InvalidMastery.IsValid()
			&& InvalidMastery.Status
				== EAuthorizationStatus::MasteryProjectionRejected
			&& ForeignOwner.IsValid()
			&& ForeignOwner.Status == EAuthorizationStatus::OwnerMismatch
			&& StaleContentResult.IsValid()
			&& StaleContentResult.Status
				== EAuthorizationStatus::ContentMismatch
			&& InvalidOperation.IsValid()
			&& InvalidOperation.Status
				== EAuthorizationStatus::OperationIdentityInvalid
			&& MissingAnchor.IsValid()
			&& MissingAnchor.Status
				== EAuthorizationStatus::AnchorUnavailable);

	FShanmenActionOrchestrator PlannedRuntime;
	FShanmenFormationDeployment PlannedDeployment;
	MakeDeployment(
		Content, PlannedRuntime, PlannedDeployment, false, OwnerId, RunId, 17);
	FShanmenFormationDeployment InvalidDeployment;
	const auto Planned =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Current,
			PlannedDeployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			EastAnchor);
	const auto Invalid =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			Current,
			InvalidDeployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::ProximityFill,
			EastAnchor);
	TestTrue(TEXT("Only a live material-accepting deployment is authorized"),
		Planned.IsValid()
			&& Planned.Status
				== EAuthorizationStatus::DeploymentNotAcceptingMaterials
			&& Invalid.IsValid()
			&& Invalid.Status == EAuthorizationStatus::DeploymentInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationMasteryOperationCurrentSnapshotTest,
	"Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization.CurrentDeploymentAndMasterySnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationMasteryOperationCurrentSnapshotTest::RunTest(
	const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("snapshot"));
	FShanmenActionOrchestrator Runtime;
	FShanmenFormationDeployment Deployment;
	MakeDeployment(Content, Runtime, Deployment);
	const auto FirstProjection = MakeProjection(
		Content, OwnerId, 7, EShanmenFormationMasteryTier::Intermediate);
	const auto RevisedProjection = MakeProjection(
		Content, OwnerId, 8, EShanmenFormationMasteryTier::Intermediate);
	const auto MasterProjection = MakeProjection(
		Content, OwnerId, 9, EShanmenFormationMasteryTier::Master);
	const auto First =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			FirstProjection,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::RemoteThrow,
			EastAnchor);
	check(First.IsAuthorized());
	TestFalse(TEXT("A revised mastery read invalidates old authorization"),
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::
			IsCurrentAuthorization(
				RevisedProjection,
				Deployment,
				OperationId,
				EShanmenFormationMaterialDeliveryMode::RemoteThrow,
				EastAnchor,
				First.Authorization));

	FShanmenFormationDeploymentReceipt Commit;
	check(Deployment.TryCommitAnchor(
		Runtime, MakeEvidence(Deployment, EastAnchor), Commit));
	TestFalse(TEXT("Deployment mutation invalidates old authorization"),
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::
			IsCurrentAuthorization(
				FirstProjection,
				Deployment,
				OperationId,
				EShanmenFormationMaterialDeliveryMode::RemoteThrow,
				EastAnchor,
				First.Authorization));
	const auto CommittedAnchor =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			FirstProjection,
			Deployment,
			OperationId,
			EShanmenFormationMaterialDeliveryMode::RemoteThrow,
			EastAnchor);
	TestTrue(TEXT("Committed anchor cannot receive another authorization"),
		CommittedAnchor.IsValid()
			&& CommittedAnchor.Status
				== EAuthorizationStatus::AnchorAlreadyCommitted);
	const auto LateScatter =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			MasterProjection,
			Deployment,
			FGuid(0xF8F16007, 0, 0, 7),
			EShanmenFormationMaterialDeliveryMode::ScatterFormation);
	TestTrue(TEXT("Scatter cannot replace a partially completed deployment"),
		LateScatter.IsValid()
			&& LateScatter.Status
				== EAuthorizationStatus::ScatterRequiresFreshDeployment);

	const auto North =
		Fdemo_mapShanmenFormationMasteryOperationAuthorizer::Authorize(
			FirstProjection,
			Deployment,
			FGuid(0xF8F16006, 0, 0, 6),
			EShanmenFormationMaterialDeliveryMode::RemoteThrow,
			NorthAnchor);
	TestTrue(TEXT("Fresh authorization captures the advanced deployment snapshot"),
		North.IsAuthorized()
			&& North.Authorization.GetDeploymentReceiptCount() == 2
			&& North.Authorization.GetCommittedAnchorCount() == 1
			&& North.Authorization.GetAnchorDefinitionId() == NorthAnchor);
	return true;
}

#endif
