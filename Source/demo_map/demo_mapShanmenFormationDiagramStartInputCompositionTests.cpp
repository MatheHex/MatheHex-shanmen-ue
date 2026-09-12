#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationDiagramStartInputComposition.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EAccessStatus = Edemo_mapShanmenFormationDiagramAccessStatus;
	using ECompositionStatus =
		Edemo_mapShanmenFormationDiagramStartInputCompositionStatus;
	using EInputStatus = Edemo_mapShanmenFormationStartInputStatus;

	const FName DiagramA(TEXT("Formation.Diagram.P27.11.Test.A"));
	const FName DiagramB(TEXT("Formation.Diagram.P27.11.Test.B"));
	const FName DiagramUnknown(TEXT("Formation.Diagram.P27.11.Test.Unknown"));
	const FGuid OwnerId(0xF8810001, 0, 0, 1);
	const FGuid RunId(0xF8810002, 0, 0, 2);
	const FGuid InputEventA(0xF8810003, 0, 0, 3);
	const FGuid InputEventB(0xF8810004, 0, 0, 4);

	FShanmenFormationDiagramCapture MakeDiagramCapture(
		const FName DiagramDefinitionId,
		const double OffsetX)
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId = DiagramDefinitionId;
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.ActivationEnergy.P27.11.Test");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 10.0f;
		FShanmenFormationAnchorCapture& Anchor =
			Capture.Anchors.AddDefaulted_GetRef();
		Anchor.Order = 0;
		Anchor.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("%s.Anchor"), *DiagramDefinitionId.ToString()));
		Anchor.RelativeOffset = FVector(OffsetX, 25.0, 0.0);
		FShanmenFormationMaterialRequirementCapture& Requirement =
			Anchor.Requirements.AddDefaulted_GetRef();
		Requirement.Order = 0;
		Requirement.MaterialDefinitionId =
			TEXT("Item.FormationMaterial.P27.11.Test");
		Requirement.Quantity = 1;
		return Capture;
	}

	Fdemo_mapShanmenFormationDiagramCatalog MakeCatalog()
	{
		Fdemo_mapShanmenFormationDiagramCatalogCapture Capture;
		Capture.Content.Version = TEXT("0.0.10.P27.11.Test");
		Capture.Content.Digest = TEXT("FormationDiagramStartInputComposition");
		Capture.Diagrams = {
			MakeDiagramCapture(DiagramA, 100.0),
			MakeDiagramCapture(DiagramB, 200.0)
		};
		Fdemo_mapShanmenFormationDiagramCatalog Catalog;
		FString Diagnostic;
		check(Fdemo_mapShanmenFormationDiagramCatalog::TryCapture(
			Capture, Catalog, Diagnostic));
		return Catalog;
	}

	Fdemo_mapShanmenFormationKnowledgeAuthorityCapture MakeKnowledgeCapture(
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		const TArray<FName>& KnownDiagramIds,
		const int64 Revision = 27)
	{
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture Capture;
		Capture.OwnerId = OwnerId;
		Capture.AuthorityRevision = Revision;
		Capture.CatalogId = Catalog.GetCatalogId();
		Capture.Content = Catalog.GetContent();
		Capture.KnownDiagramDefinitionIds = KnownDiagramIds;
		return Capture;
	}

	Fdemo_mapShanmenFormationControllerResult MakeRejectedLifecycleResult()
	{
		Fdemo_mapShanmenFormationControllerResult Result;
		Result.Diagnostic = TEXT("Synthetic lifecycle rejection.");
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramStartInputPreflightTest,
	"Shanmen.0_0_10.Product.FormationDiagramStartInputComposition.PreflightIsLazy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramStartInputPreflightTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationDiagramCatalog Catalog = MakeCatalog();
	int32 AuthorityReads = 0;
	int32 LifecycleCalls = 0;
	auto UnexpectedRead = [&AuthorityReads](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture&,
		FString&)
	{
		++AuthorityReads;
		return false;
	};
	auto UnexpectedRoute = [&LifecycleCalls](
		const Fdemo_mapShanmenFormationIntent&)
	{
		++LifecycleCalls;
		return MakeRejectedLifecycleResult();
	};
	const auto Blocked =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			false, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnexpectedRead, UnexpectedRoute);
	const auto NoLifecycle =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, false, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnexpectedRead, UnexpectedRoute);
	const auto NoRun =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, FGuid(), OwnerId, InputEventA,
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnexpectedRead, UnexpectedRoute);
	const auto NoOwner =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, FGuid(), InputEventA,
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnexpectedRead, UnexpectedRoute);
	const auto NoEvent =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, FGuid(),
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnexpectedRead, UnexpectedRoute);

	TestTrue(TEXT("Every input preflight completes before personal knowledge access"),
		Blocked.IsValid()
			&& NoLifecycle.IsValid()
			&& NoRun.IsValid()
			&& NoOwner.IsValid()
			&& NoEvent.IsValid()
			&& Blocked.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& NoLifecycle.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& NoRun.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& NoOwner.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& NoEvent.GetStatus()
				== ECompositionStatus::InputCompletedBeforeAccess
			&& Blocked.GetInput().Status == EInputStatus::GameplayBlocked
			&& NoLifecycle.GetInput().Status
				== EInputStatus::LifecycleUnavailable
			&& NoRun.GetInput().Status == EInputStatus::RunUnavailable
			&& NoOwner.GetInput().Status == EInputStatus::OwnerUnavailable
			&& NoEvent.GetInput().Status
				== EInputStatus::EventIdentityInvalid);
	TestTrue(TEXT("Preflight rejection performs no access, sample, or lifecycle work"),
		AuthorityReads == 0
			&& LifecycleCalls == 0
			&& Blocked.GetAccessInvocationCount() == 0
			&& NoLifecycle.GetAccessInvocationCount() == 0
			&& NoRun.GetSpatialSampleCaptureCount() == 0
			&& NoOwner.GetSpatialSampleCaptureCount() == 0
			&& NoEvent.GetInput().LifecycleInvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramStartInputAccessFenceTest,
	"Shanmen.0_0_10.Product.FormationDiagramStartInputComposition.AccessFencesLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramStartInputAccessFenceTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenFormationDiagramCatalog Catalog = MakeCatalog();
	int32 AuthorityReads = 0;
	int32 LifecycleCalls = 0;
	auto UnavailableAuthority = [&AuthorityReads](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture&,
		FString& OutDiagnostic)
	{
		++AuthorityReads;
		OutDiagnostic = TEXT("Synthetic knowledge authority unavailable.");
		return false;
	};
	auto RouteLifecycle = [&LifecycleCalls](
		const Fdemo_mapShanmenFormationIntent&)
	{
		++LifecycleCalls;
		return MakeRejectedLifecycleResult();
	};
	const auto Unknown =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramUnknown, FVector::ZeroVector, FVector::ForwardVector,
			UnavailableAuthority, RouteLifecycle);
	const auto AuthorityUnavailable =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnavailableAuthority, RouteLifecycle);
	auto UnlearnedAuthority = [&AuthorityReads, &Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++AuthorityReads;
		OutCapture = MakeKnowledgeCapture(Catalog, { DiagramB }, 28);
		return true;
	};
	const auto Unlearned =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, FVector::ZeroVector, FVector::ForwardVector,
			UnlearnedAuthority, RouteLifecycle);

	TestTrue(TEXT("Publicly unknown diagram is rejected without personal read"),
		Unknown.IsValid()
			&& Unknown.GetStatus() == ECompositionStatus::DiagramAccessRejected
			&& Unknown.GetAccess().Status == EAccessStatus::DiagramUnknown
			&& Unknown.GetAccess().KnowledgeProjectionCount == 0);
	TestTrue(TEXT("Authority and learned-state failures preserve exact access proof"),
		AuthorityUnavailable.IsValid()
			&& AuthorityUnavailable.GetStatus()
				== ECompositionStatus::DiagramAccessRejected
			&& AuthorityUnavailable.GetAccess().Status
				== EAccessStatus::KnowledgeProjectionRejected
			&& Unlearned.IsValid()
			&& Unlearned.GetAccess().Status == EAccessStatus::SelectionRejected
			&& Unlearned.GetAccess().Selection.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					DiagramUnavailable);
	TestTrue(TEXT("Rejected access never captures space or reaches lifecycle"),
		AuthorityReads == 2
			&& LifecycleCalls == 0
			&& Unknown.GetSpatialSampleCaptureCount() == 0
			&& AuthorityUnavailable.GetSpatialSampleCaptureCount() == 0
			&& Unlearned.GetSpatialSampleCaptureCount() == 0
			&& Unknown.GetInput().Status == EInputStatus::SampleRejected
			&& Unlearned.GetInput().LifecycleInvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramStartInputSpatialFenceTest,
	"Shanmen.0_0_10.Product.FormationDiagramStartInputComposition.SpatialFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramStartInputSpatialFenceTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenFormationDiagramCatalog Catalog = MakeCatalog();
	int32 AuthorityReads = 0;
	int32 LifecycleCalls = 0;
	auto ReadAuthority = [&AuthorityReads, &Catalog](
		const FGuid& RequestedOwnerId,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++AuthorityReads;
		check(RequestedOwnerId == OwnerId);
		OutCapture = MakeKnowledgeCapture(Catalog, { DiagramA });
		return true;
	};
	auto RouteLifecycle = [&LifecycleCalls](
		const Fdemo_mapShanmenFormationIntent&)
	{
		++LifecycleCalls;
		return MakeRejectedLifecycleResult();
	};
	const auto InvalidSpatial =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, FVector(10.0, 20.0, 30.0), FVector::ZeroVector,
			ReadAuthority, RouteLifecycle);

	TestTrue(TEXT("Known diagram remains visible when spatial capture rejects"),
		InvalidSpatial.IsValid()
			&& InvalidSpatial.GetStatus()
				== ECompositionStatus::SpatialSampleRejected
			&& InvalidSpatial.GetAccess().IsSelected()
			&& InvalidSpatial.GetAccessInvocationCount() == 1
			&& InvalidSpatial.GetSpatialSampleCaptureCount() == 1
			&& InvalidSpatial.GetInput().Status == EInputStatus::SampleRejected);
	TestTrue(TEXT("Invalid space cannot produce an intent or lifecycle call"),
		AuthorityReads == 1
			&& LifecycleCalls == 0
			&& !InvalidSpatial.GetInput().Intent.IsValid()
			&& InvalidSpatial.GetInput().LifecycleInvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramStartInputDelegationTest,
	"Shanmen.0_0_10.Product.FormationDiagramStartInputComposition.DeterministicDelegation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramStartInputDelegationTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenFormationDiagramCatalog Catalog = MakeCatalog();
	int32 AuthorityReads = 0;
	int32 LifecycleCalls = 0;
	auto ReadAuthority = [&AuthorityReads, &Catalog](
		const FGuid& RequestedOwnerId,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++AuthorityReads;
		check(RequestedOwnerId == OwnerId);
		OutCapture = MakeKnowledgeCapture(Catalog, { DiagramA }, 29);
		return true;
	};
	FGuid RoutedIntentId;
	auto RejectLifecycle = [&LifecycleCalls, &RoutedIntentId](
		const Fdemo_mapShanmenFormationIntent& Intent)
	{
		++LifecycleCalls;
		RoutedIntentId = Intent.GetIntentId();
		return MakeRejectedLifecycleResult();
	};
	const FVector Origin(10.0, 20.0, 30.0);
	const FVector Forward(2.0, 0.0, 5.0);
	const auto First =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, Origin, Forward,
			ReadAuthority, RejectLifecycle);
	const auto Replay =
		Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
			true, true, RunId, OwnerId, InputEventA,
			Catalog, DiagramA, Origin, Forward,
			ReadAuthority, RejectLifecycle);

	TestTrue(TEXT("Selected diagram delegates through the existing input contract"),
		First.IsValid()
			&& !First.IsAccepted()
			&& First.GetStatus() == ECompositionStatus::Delegated
			&& First.GetAccess().IsSelected()
			&& First.GetInput().Status == EInputStatus::LifecycleRejected
			&& First.GetInput().Sample.GetForward().Equals(
				FVector::ForwardVector)
			&& First.GetInput().Intent.GetDiagram().GetDiagramDefinitionId()
				== DiagramA
			&& RoutedIntentId == First.GetInput().IntentId);
	TestTrue(TEXT("Exact access and event replay preserve both identities"),
		Replay.IsValid()
			&& Replay.GetAccess().Selection.Selection.GetSelectionId()
				== First.GetAccess().Selection.Selection.GetSelectionId()
			&& Replay.GetInput().IntentId == First.GetInput().IntentId
			&& Replay.GetInput().IntentId
				== Fdemo_mapShanmenFormationInputAdapter::MakeIntentId(
					RunId, InputEventA)
			&& Replay.GetInput().IntentId
				!= Fdemo_mapShanmenFormationInputAdapter::MakeIntentId(
					RunId, InputEventB)
			&& AuthorityReads == 2
			&& LifecycleCalls == 2);
	return true;
}

#endif
