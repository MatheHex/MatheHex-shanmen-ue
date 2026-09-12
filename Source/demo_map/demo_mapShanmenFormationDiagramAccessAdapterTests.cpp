#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationDiagramAccessAdapter.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EAccessStatus = Edemo_mapShanmenFormationDiagramAccessStatus;
	using EProjectionStatus =
		Edemo_mapShanmenFormationKnowledgeProjectionStatus;
	using ESelectionStatus =
		Edemo_mapShanmenFormationDiagramSelectionStatus;

	const FName DiagramA(TEXT("Formation.Diagram.P27.8.Test.A"));
	const FName DiagramB(TEXT("Formation.Diagram.P27.8.Test.B"));
	const FName DiagramUnknown(TEXT("Formation.Diagram.P27.8.Test.Unknown"));
	const FGuid KnowledgeOwner(0xF8780001, 0, 0, 1);
	const FGuid ForeignOwner(0xF8780002, 0, 0, 2);

	FShanmenContentStamp MakeContent(const TCHAR* Digest)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.8.Test");
		Content.Digest = Digest;
		return Content;
	}

	FShanmenFormationDiagramCapture MakeDiagramCapture(
		const FName DiagramId,
		const double OffsetX)
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId = DiagramId;
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.ActivationEnergy.P27.8.Test");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 10.0f;
		FShanmenFormationAnchorCapture& Anchor =
			Capture.Anchors.AddDefaulted_GetRef();
		Anchor.Order = 0;
		Anchor.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("%s.Anchor"), *DiagramId.ToString()));
		Anchor.RelativeOffset = FVector(OffsetX, 25.0, 0.0);
		FShanmenFormationMaterialRequirementCapture& Requirement =
			Anchor.Requirements.AddDefaulted_GetRef();
		Requirement.Order = 0;
		Requirement.MaterialDefinitionId =
			TEXT("Item.FormationMaterial.P27.8.Test");
		Requirement.Quantity = 1;
		return Capture;
	}

	bool TryMakeCatalog(
		const FShanmenContentStamp& Content,
		Fdemo_mapShanmenFormationDiagramCatalog& OutCatalog,
		FString& OutDiagnostic)
	{
		Fdemo_mapShanmenFormationDiagramCatalogCapture Capture;
		Capture.Content = Content;
		Capture.Diagrams = {
			MakeDiagramCapture(DiagramA, 100.0),
			MakeDiagramCapture(DiagramB, 200.0)
		};
		return Fdemo_mapShanmenFormationDiagramCatalog::TryCapture(
			Capture, OutCatalog, OutDiagnostic);
	}

	Fdemo_mapShanmenFormationKnowledgeAuthorityCapture MakeAuthorityCapture(
		const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
		const FGuid& OwnerId,
		const int64 Revision,
		const TArray<FName>& KnownDiagramIds)
	{
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture Capture;
		Capture.OwnerId = OwnerId;
		Capture.AuthorityRevision = Revision;
		Capture.CatalogId = Catalog.GetCatalogId();
		Capture.Content = Catalog.GetContent();
		Capture.KnownDiagramDefinitionIds = KnownDiagramIds;
		return Capture;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramAccessDeterminismTest,
	"Shanmen.0_0_10.Product.FormationDiagramAccessAdapter.DeterministicSingleReadSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramAccessDeterminismTest::RunTest(const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	if (!TryMakeCatalog(MakeContent(TEXT("determinism")), Catalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	int32 TotalReads = 0;
	FGuid LastRequestedOwner;
	auto ReadAuthority = [&TotalReads, &LastRequestedOwner, &Catalog](
		const FGuid& RequestedOwner,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString& OutDiagnostic)
	{
		++TotalReads;
		LastRequestedOwner = RequestedOwner;
		OutCapture = MakeAuthorityCapture(
			Catalog, KnowledgeOwner, 27, { DiagramB, DiagramA });
		OutDiagnostic = TEXT("Authority read completed.");
		return true;
	};
	const auto First = Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
		Catalog, KnowledgeOwner, DiagramA, ReadAuthority);
	const auto Replay = Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
		Catalog, KnowledgeOwner, DiagramA, ReadAuthority);

	TestTrue(TEXT("Each access performs one authority read and one selection"),
		TotalReads == 2
			&& LastRequestedOwner == KnowledgeOwner
			&& First.KnowledgeProjectionCount == 1
			&& First.SelectionInvocationCount == 1
			&& Replay.KnowledgeProjectionCount == 1
			&& Replay.SelectionInvocationCount == 1);
	TestTrue(TEXT("Exact authority replay yields the same bound selection"),
		First.IsSelected()
			&& Replay.IsSelected()
			&& First.KnowledgeProjection.AuthorityRead.GetReadId()
				== Replay.KnowledgeProjection.AuthorityRead.GetReadId()
			&& First.KnowledgeProjection.Knowledge.GetSnapshotId()
				== Replay.KnowledgeProjection.Knowledge.GetSnapshotId()
			&& First.Selection.Selection.GetSelectionId()
				== Replay.Selection.Selection.GetSelectionId()
			&& First.Selection.Selection.GetOwnerId() == KnowledgeOwner
			&& First.Selection.Selection.GetKnowledgeSnapshotId()
				== First.KnowledgeProjection.Knowledge.GetSnapshotId()
			&& First.Selection.Selection.GetDiagram().GetDiagramDefinitionId()
				== DiagramA);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramAccessPreflightTest,
	"Shanmen.0_0_10.Product.FormationDiagramAccessAdapter.PreflightDoesNotReadAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramAccessPreflightTest::RunTest(const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	if (!TryMakeCatalog(MakeContent(TEXT("preflight")), Catalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	int32 ReadCount = 0;
	auto UnexpectedRead = [&ReadCount](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture&,
		FString&)
	{
		++ReadCount;
		return true;
	};
	const auto InvalidCatalog =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Fdemo_mapShanmenFormationDiagramCatalog(),
			KnowledgeOwner,
			DiagramA,
			UnexpectedRead);
	const auto InvalidOwner =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, FGuid(), DiagramA, UnexpectedRead);
	const auto InvalidRequest =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, KnowledgeOwner, NAME_None, UnexpectedRead);
	const auto Unknown =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, KnowledgeOwner, DiagramUnknown, UnexpectedRead);

	TestTrue(TEXT("Invalid and unknown requests never read personal authority"),
		ReadCount == 0
			&& InvalidCatalog.IsValid()
			&& InvalidCatalog.Status == EAccessStatus::CatalogInvalid
			&& InvalidOwner.IsValid()
			&& InvalidOwner.Status == EAccessStatus::OwnerInvalid
			&& InvalidRequest.IsValid()
			&& InvalidRequest.Status == EAccessStatus::RequestInvalid
			&& Unknown.IsValid()
			&& Unknown.Status == EAccessStatus::DiagramUnknown
			&& Unknown.KnowledgeProjectionCount == 0
			&& Unknown.SelectionInvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramAccessProjectionFenceTest,
	"Shanmen.0_0_10.Product.FormationDiagramAccessAdapter.KnowledgeFailureStopsSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramAccessProjectionFenceTest::RunTest(
	const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	Fdemo_mapShanmenFormationDiagramCatalog StaleCatalog;
	if (!TryMakeCatalog(MakeContent(TEXT("current")), Catalog, Diagnostic)
		|| !TryMakeCatalog(
			MakeContent(TEXT("stale")), StaleCatalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	int32 ReadCount = 0;
	auto UnavailableRead = [&ReadCount](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture&,
		FString& OutDiagnostic)
	{
		++ReadCount;
		OutDiagnostic = TEXT("Test authority unavailable.");
		return false;
	};
	const auto Unavailable =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, KnowledgeOwner, DiagramA, UnavailableRead);
	auto ForeignRead = [&ReadCount, &Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++ReadCount;
		OutCapture = MakeAuthorityCapture(
			Catalog, ForeignOwner, 4, { DiagramA });
		return true;
	};
	const auto Foreign =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, KnowledgeOwner, DiagramA, ForeignRead);
	auto StaleRead = [&ReadCount, &StaleCatalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++ReadCount;
		OutCapture = MakeAuthorityCapture(
			StaleCatalog, KnowledgeOwner, 5, { DiagramA });
		return true;
	};
	const auto Stale =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, KnowledgeOwner, DiagramA, StaleRead);

	TestTrue(TEXT("Every rejected projection preserves exact nested evidence"),
		ReadCount == 3
			&& Unavailable.IsValid()
			&& Unavailable.Status
				== EAccessStatus::KnowledgeProjectionRejected
			&& Unavailable.KnowledgeProjection.Status
				== EProjectionStatus::AuthorityUnavailable
			&& Unavailable.SelectionInvocationCount == 0
			&& Foreign.IsValid()
			&& Foreign.KnowledgeProjection.Status
				== EProjectionStatus::OwnerMismatch
			&& Foreign.SelectionInvocationCount == 0
			&& Stale.IsValid()
			&& Stale.KnowledgeProjection.Status
				== EProjectionStatus::CatalogMismatch
			&& Stale.SelectionInvocationCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramAccessUnavailableTest,
	"Shanmen.0_0_10.Product.FormationDiagramAccessAdapter.AuthoredButUnavailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramAccessUnavailableTest::RunTest(const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	if (!TryMakeCatalog(MakeContent(TEXT("unavailable")), Catalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	int32 ReadCount = 0;
	auto ReadOtherKnowledge = [&ReadCount, &Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++ReadCount;
		OutCapture = MakeAuthorityCapture(
			Catalog, KnowledgeOwner, 31, { DiagramB });
		return true;
	};
	const auto Unavailable =
		Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
			Catalog, KnowledgeOwner, DiagramA, ReadOtherKnowledge);

	TestTrue(TEXT("Authored but unlearned diagrams fail at selection only"),
		ReadCount == 1
			&& Unavailable.IsValid()
			&& !Unavailable.IsSelected()
			&& Unavailable.Status == EAccessStatus::SelectionRejected
			&& Unavailable.KnowledgeProjection.IsProjected()
			&& Unavailable.Selection.Status
				== ESelectionStatus::DiagramUnavailable
			&& Unavailable.KnowledgeProjectionCount == 1
			&& Unavailable.SelectionInvocationCount == 1);
	return true;
}

#endif
