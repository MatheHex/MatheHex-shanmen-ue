#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationDiagramSelectionPort.h"

#include "demo_mapShanmenFormationInputAdapter.h"
#include "Misc/AutomationTest.h"

namespace
{
	const FName DiagramA(TEXT("Formation.Diagram.P27.6.Test.A"));
	const FName DiagramB(TEXT("Formation.Diagram.P27.6.Test.B"));
	const FName DiagramUnknown(TEXT("Formation.Diagram.P27.6.Test.Unknown"));
	const FGuid KnowledgeOwner(0xF8760001, 0, 0, 1);

	FShanmenContentStamp MakeContent(const TCHAR* Digest)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.6.Test");
		Content.Digest = Digest;
		return Content;
	}

	FShanmenFormationDiagramCapture MakeDiagramCapture(
		const FName DiagramId,
		const double OffsetX,
		const int32 Quantity = 1)
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId = DiagramId;
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
			TEXT("Item.FormationMaterial.P27.6.Test");
		Requirement.Quantity = Quantity;
		return Capture;
	}

	bool TryMakeCatalog(
		const FShanmenContentStamp& Content,
		const TArray<FShanmenFormationDiagramCapture>& Diagrams,
		Fdemo_mapShanmenFormationDiagramCatalog& OutCatalog,
		FString& OutDiagnostic)
	{
		Fdemo_mapShanmenFormationDiagramCatalogCapture Capture;
		Capture.Content = Content;
		Capture.Diagrams = Diagrams;
		return Fdemo_mapShanmenFormationDiagramCatalog::TryCapture(
			Capture, OutCatalog, OutDiagnostic);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramCatalogIdentityTest,
	"Shanmen.0_0_10.Product.FormationDiagramSelection.CatalogIdentityAndDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramCatalogIdentityTest::RunTest(const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("catalog-a"));
	const TArray<FShanmenFormationDiagramCapture> Captures = {
		MakeDiagramCapture(DiagramA, 100.0),
		MakeDiagramCapture(DiagramB, 200.0, 2)
	};
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog First;
	Fdemo_mapShanmenFormationDiagramCatalog Replay;
	const bool bFirst = TryMakeCatalog(
		Content, Captures, First, Diagnostic);
	const bool bReplay = TryMakeCatalog(
		Content, Captures, Replay, Diagnostic);
	TestTrue(TEXT("Exact authored catalog replay has one stable identity"),
		bFirst && bReplay && First.IsValid() && Replay.IsValid()
			&& First.GetCatalogId() == Replay.GetCatalogId()
			&& First.GetDiagrams().Num() == 2
			&& First.GetDiagrams()[0].GetDiagramDefinitionId() == DiagramA
			&& First.FindDiagram(DiagramB) != nullptr
			&& First.FindDiagram(DiagramUnknown) == nullptr);

	Fdemo_mapShanmenFormationDiagramCatalog Reordered;
	const TArray<FShanmenFormationDiagramCapture> ReorderedCaptures = {
		Captures[1], Captures[0]
	};
	const bool bReordered = TryMakeCatalog(
		Content, ReorderedCaptures, Reordered, Diagnostic);
	Fdemo_mapShanmenFormationDiagramCatalog GeometryDrift;
	const TArray<FShanmenFormationDiagramCapture> GeometryCaptures = {
		MakeDiagramCapture(DiagramA, 101.0), Captures[1]
	};
	const bool bGeometry = TryMakeCatalog(
		Content, GeometryCaptures, GeometryDrift, Diagnostic);
	Fdemo_mapShanmenFormationDiagramCatalog ContentDrift;
	const bool bContent = TryMakeCatalog(
		MakeContent(TEXT("catalog-b")),
		Captures,
		ContentDrift,
		Diagnostic);
	TestTrue(TEXT("Content geometry and authored order are identity-bound"),
		bReordered && bGeometry && bContent
			&& Reordered.GetCatalogId() != First.GetCatalogId()
			&& GeometryDrift.GetCatalogId() != First.GetCatalogId()
			&& ContentDrift.GetCatalogId() != First.GetCatalogId());

	Fdemo_mapShanmenFormationDiagramCatalog Duplicate;
	const TArray<FShanmenFormationDiagramCapture> DuplicateCaptures = {
		Captures[0], Captures[0]
	};
	const bool bDuplicate = TryMakeCatalog(
		Content, DuplicateCaptures, Duplicate, Diagnostic);
	FShanmenFormationDiagramCapture InvalidDiagram = Captures[0];
	InvalidDiagram.Anchors.Reset();
	Fdemo_mapShanmenFormationDiagramCatalog Invalid;
	const bool bInvalid = TryMakeCatalog(
		Content, { InvalidDiagram }, Invalid, Diagnostic);
	Fdemo_mapShanmenFormationDiagramCatalog Empty;
	const bool bEmpty = TryMakeCatalog(Content, {}, Empty, Diagnostic);
	TestTrue(TEXT("Duplicate invalid and empty authored catalogs fail closed"),
		!bDuplicate && !Duplicate.IsValid()
			&& !bInvalid && !Invalid.IsValid()
			&& !bEmpty && !Empty.IsValid()
			&& !Diagnostic.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramKnowledgeSelectionTest,
	"Shanmen.0_0_10.Product.FormationDiagramSelection.KnowledgeAndCatalogFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramKnowledgeSelectionTest::RunTest(const FString&)
{
	const FShanmenContentStamp Content = MakeContent(TEXT("knowledge"));
	const TArray<FShanmenFormationDiagramCapture> Captures = {
		MakeDiagramCapture(DiagramA, 100.0),
		MakeDiagramCapture(DiagramB, 200.0)
	};
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	if (!TryMakeCatalog(Content, Captures, Catalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot Ordered;
	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot ReverseOrder;
	const bool bOrdered =
		Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
			Catalog,
			KnowledgeOwner,
			4,
			{ DiagramA, DiagramB },
			Ordered,
			Diagnostic);
	const bool bReverse =
		Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
			Catalog,
			KnowledgeOwner,
			4,
			{ DiagramB, DiagramA },
			ReverseOrder,
			Diagnostic);
	TestTrue(TEXT("Knowledge identity canonicalizes set ordering"),
		bOrdered && bReverse && Ordered.IsValid() && ReverseOrder.IsValid()
			&& Ordered.GetSnapshotId() == ReverseOrder.GetSnapshotId()
			&& Ordered.GetKnownDiagramDefinitionIds().Num() == 2
			&& Ordered.GetKnownDiagramDefinitionIds()[0] == DiagramA
			&& Ordered.GetOwnerId() == KnowledgeOwner
			&& Ordered.GetAuthorityRevision() == 4);

	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot OnlyB;
	if (!Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
		Catalog,
		KnowledgeOwner,
		5,
		{ DiagramB },
		OnlyB,
		Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const Fdemo_mapShanmenFormationDiagramSelectionResult Selected =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, OnlyB, DiagramB);
	const Fdemo_mapShanmenFormationDiagramSelectionResult Replayed =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, OnlyB, DiagramB);
	const Fdemo_mapShanmenFormationDiagramSelectionResult Unavailable =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, OnlyB, DiagramA);
	const Fdemo_mapShanmenFormationDiagramSelectionResult Unknown =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, OnlyB, DiagramUnknown);
	const Fdemo_mapShanmenFormationDiagramSelectionResult NoRequest =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, OnlyB, NAME_None);
	TestTrue(TEXT("Only authored and owner-known diagrams can be selected"),
		Selected.IsSelected() && Replayed.IsSelected()
			&& Selected.Selection.GetSelectionId()
				== Replayed.Selection.GetSelectionId()
			&& Selected.Selection.GetOwnerId() == KnowledgeOwner
			&& Selected.Selection.GetKnowledgeAuthorityRevision() == 5
			&& Selected.Selection.GetDiagram().GetDiagramDefinitionId()
				== DiagramB
			&& Unavailable.IsValid()
			&& Unavailable.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					DiagramUnavailable
			&& Unknown.IsValid()
			&& Unknown.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					DiagramUnknown
			&& NoRequest.IsValid()
			&& NoRequest.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					RequestInvalid);

	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot EmptyKnowledge;
	const bool bEmptyKnowledge =
		Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
			Catalog, KnowledgeOwner, 6, {}, EmptyKnowledge, Diagnostic);
	const auto EmptyUnavailable =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, EmptyKnowledge, DiagramA);
	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot DuplicateKnowledge;
	const bool bDuplicate =
		Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
			Catalog,
			KnowledgeOwner,
			6,
			{ DiagramA, DiagramA },
			DuplicateKnowledge,
			Diagnostic);
	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot UnknownKnowledge;
	const bool bUnknown =
		Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
			Catalog,
			KnowledgeOwner,
			6,
			{ DiagramUnknown },
			UnknownKnowledge,
			Diagnostic);
	TestTrue(TEXT("Empty knowledge is valid while malformed claims fail closed"),
		bEmptyKnowledge && EmptyKnowledge.IsValid()
			&& EmptyUnavailable.IsValid()
			&& EmptyUnavailable.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					DiagramUnavailable
			&& !bDuplicate && !DuplicateKnowledge.IsValid()
			&& !bUnknown && !UnknownKnowledge.IsValid());

	Fdemo_mapShanmenFormationDiagramCatalog ChangedCatalog;
	const TArray<FShanmenFormationDiagramCapture> ChangedCaptures = {
		Captures[0], MakeDiagramCapture(DiagramB, 201.0)
	};
	if (!TryMakeCatalog(
		Content, ChangedCaptures, ChangedCatalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const auto Stale =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			ChangedCatalog, OnlyB, DiagramB);
	TestTrue(TEXT("Knowledge from a prior catalog cannot select changed content"),
		Stale.IsValid()
			&& Stale.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					CatalogMismatch
			&& !Stale.IsSelected());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationDiagramSelectionInputEvidenceTest,
	"Shanmen.0_0_10.Product.FormationDiagramSelection.InputEvidence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationDiagramSelectionInputEvidenceTest::RunTest(
	const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	if (!TryMakeCatalog(
		MakeContent(TEXT("input-evidence")),
		{ MakeDiagramCapture(DiagramA, 100.0) },
		Catalog,
		Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot Knowledge;
	if (!Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
		Catalog,
		KnowledgeOwner,
		7,
		{ DiagramA },
		Knowledge,
		Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const auto Selected =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, Knowledge, DiagramA);
	Fdemo_mapShanmenFormationStartInputSample Sample;
	const bool bCaptured =
		Fdemo_mapShanmenFormationStartInputSample::TryCapture(
			Selected.Selection,
			FVector(5.0, -6.0, 7.0),
			FVector(3.0, 4.0, 9.0),
			Sample);
	TestTrue(TEXT("Physical start sample preserves selected knowledge proof"),
		Selected.IsSelected() && bCaptured && Sample.IsValid()
			&& Sample.GetSelection().GetSelectionId()
				== Selected.Selection.GetSelectionId()
			&& Sample.GetSelection().GetOwnerId() == KnowledgeOwner
			&& Sample.GetDiagram().GetDiagramDefinitionId() == DiagramA
			&& Sample.GetOrigin().Equals(FVector(5.0, -6.0, 7.0))
			&& Sample.GetForward().Equals(FVector(0.6, 0.8, 0.0)));

	Fdemo_mapShanmenFormationStartInputSample NoSelection;
	const bool bNoSelection =
		Fdemo_mapShanmenFormationStartInputSample::TryCapture(
			Fdemo_mapShanmenFormationDiagramSelection(),
			FVector::ZeroVector,
			FVector::ForwardVector,
			NoSelection);
	Fdemo_mapShanmenFormationStartInputSample NoFacing;
	const bool bNoFacing =
		Fdemo_mapShanmenFormationStartInputSample::TryCapture(
			Selected.Selection,
			FVector::ZeroVector,
			FVector::UpVector,
			NoFacing);
	TestTrue(TEXT("Unselected diagrams and non-planar facing fail closed"),
		!bNoSelection && !NoSelection.IsValid()
			&& !bNoFacing && !NoFacing.IsValid());
	return true;
}

#endif
