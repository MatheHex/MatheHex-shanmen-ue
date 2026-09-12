#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationKnowledgeAuthorityAdapter.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EProjectionStatus =
		Edemo_mapShanmenFormationKnowledgeProjectionStatus;

	const FName DiagramA(TEXT("Formation.Diagram.P27.7.Test.A"));
	const FName DiagramB(TEXT("Formation.Diagram.P27.7.Test.B"));
	const FName DiagramUnknown(TEXT("Formation.Diagram.P27.7.Test.Unknown"));
	const FGuid KnowledgeOwner(0xF8770001, 0, 0, 1);
	const FGuid ForeignOwner(0xF8770002, 0, 0, 2);

	FShanmenContentStamp MakeContent(const TCHAR* Digest)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P27.7.Test");
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
			TEXT("Item.FormationMaterial.P27.7.Test");
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
	Fdemo_mapFormationKnowledgeAuthorityDeterminismTest,
	"Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter.DeterministicSingleReadProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationKnowledgeAuthorityDeterminismTest::RunTest(
	const FString&)
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
			Catalog, KnowledgeOwner, 17, { DiagramB, DiagramA });
		OutDiagnostic = TEXT("Authority read completed.");
		return true;
	};
	const auto First =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, ReadAuthority);
	const auto Replay =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, ReadAuthority);
	TestTrue(TEXT("Each projection performs exactly one authority read"),
		TotalReads == 2
			&& First.AuthorityReadCount == 1
			&& Replay.AuthorityReadCount == 1
			&& LastRequestedOwner == KnowledgeOwner);
	TestTrue(TEXT("Exact authority replay has deterministic canonical evidence"),
		First.IsProjected() && Replay.IsProjected()
			&& First.AuthorityRead.GetReadId()
				== Replay.AuthorityRead.GetReadId()
			&& First.Knowledge.GetSnapshotId()
				== Replay.Knowledge.GetSnapshotId()
			&& First.AuthorityRead.GetKnownDiagramDefinitionIds().Num() == 2
			&& First.AuthorityRead.GetKnownDiagramDefinitionIds()[0]
				== DiagramA
			&& First.Knowledge.GetOwnerId() == KnowledgeOwner
			&& First.Knowledge.GetAuthorityRevision() == 17);

	const auto Selection =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, First.Knowledge, DiagramA);
	TestTrue(TEXT("Projected evidence composes with the P27.6 selection port"),
		Selection.IsSelected()
			&& Selection.Selection.GetOwnerId() == KnowledgeOwner
			&& Selection.Selection.GetDiagram().GetDiagramDefinitionId()
				== DiagramA);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationKnowledgeAuthorityPreflightTest,
	"Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter.PreflightAndAvailabilityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationKnowledgeAuthorityPreflightTest::RunTest(
	const FString&)
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
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Fdemo_mapShanmenFormationDiagramCatalog(),
			KnowledgeOwner,
			UnexpectedRead);
	const auto InvalidOwner =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, FGuid(), UnexpectedRead);
	TestTrue(TEXT("Invalid preflight evidence never reaches the authority"),
		ReadCount == 0
			&& InvalidCatalog.IsValid()
			&& InvalidCatalog.Status == EProjectionStatus::CatalogInvalid
			&& InvalidOwner.IsValid()
			&& InvalidOwner.Status == EProjectionStatus::OwnerInvalid);

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
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, UnavailableRead);
	auto MalformedRead = [&ReadCount](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture&,
		FString&)
	{
		++ReadCount;
		return true;
	};
	const auto Malformed =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, MalformedRead);
	TestTrue(TEXT("Unavailable and malformed reads fail closed after one call"),
		ReadCount == 2
			&& Unavailable.IsValid()
			&& Unavailable.AuthorityReadCount == 1
			&& Unavailable.Status == EProjectionStatus::AuthorityUnavailable
			&& !Unavailable.AuthorityRead.IsValid()
			&& !Unavailable.Knowledge.IsValid()
			&& Malformed.IsValid()
			&& Malformed.AuthorityReadCount == 1
			&& Malformed.Status == EProjectionStatus::AuthorityReadInvalid
			&& !Malformed.Knowledge.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationKnowledgeAuthorityIdentityFenceTest,
	"Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter.OwnerAndCatalogFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationKnowledgeAuthorityIdentityFenceTest::RunTest(
	const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	Fdemo_mapShanmenFormationDiagramCatalog ChangedCatalog;
	if (!TryMakeCatalog(MakeContent(TEXT("catalog-current")), Catalog, Diagnostic)
		|| !TryMakeCatalog(
			MakeContent(TEXT("catalog-changed")), ChangedCatalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	int32 OwnerReads = 0;
	auto ForeignRead = [&OwnerReads, &Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++OwnerReads;
		OutCapture = MakeAuthorityCapture(
			Catalog, ForeignOwner, 3, { DiagramA });
		return true;
	};
	const auto Foreign =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, ForeignRead);
	TestTrue(TEXT("Cross-owner authority evidence is rejected"),
		OwnerReads == 1
			&& Foreign.IsValid()
			&& Foreign.Status == EProjectionStatus::OwnerMismatch
			&& Foreign.AuthorityRead.IsValid()
			&& !Foreign.Knowledge.IsValid());

	int32 CatalogReads = 0;
	auto StaleRead = [&CatalogReads, &ChangedCatalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		++CatalogReads;
		OutCapture = MakeAuthorityCapture(
			ChangedCatalog, KnowledgeOwner, 4, { DiagramA });
		return true;
	};
	const auto Stale =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, StaleRead);
	TestTrue(TEXT("Evidence from another content catalog is rejected"),
		CatalogReads == 1
			&& Stale.IsValid()
			&& Stale.Status == EProjectionStatus::CatalogMismatch
			&& Stale.AuthorityRead.IsValid()
			&& !Stale.Knowledge.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationKnowledgeAuthorityClaimsTest,
	"Shanmen.0_0_10.Product.FormationKnowledgeAuthorityAdapter.ClaimAndEmptyKnowledgeFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationKnowledgeAuthorityClaimsTest::RunTest(
	const FString&)
{
	FString Diagnostic;
	Fdemo_mapShanmenFormationDiagramCatalog Catalog;
	if (!TryMakeCatalog(MakeContent(TEXT("claims")), Catalog, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}

	auto EmptyRead = [&Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		OutCapture = MakeAuthorityCapture(
			Catalog, KnowledgeOwner, 8, {});
		return true;
	};
	const auto Empty =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, EmptyRead);
	const auto UnavailableSelection =
		Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
			Catalog, Empty.Knowledge, DiagramA);
	TestTrue(TEXT("An authoritative empty knowledge set is a valid projection"),
		Empty.IsProjected()
			&& Empty.Knowledge.GetKnownDiagramDefinitionIds().IsEmpty()
			&& UnavailableSelection.IsValid()
			&& UnavailableSelection.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					DiagramUnavailable);

	auto DuplicateRead = [&Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		OutCapture = MakeAuthorityCapture(
			Catalog, KnowledgeOwner, 9, { DiagramA, DiagramA });
		return true;
	};
	const auto Duplicate =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, DuplicateRead);
	auto UnknownRead = [&Catalog](
		const FGuid&,
		Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& OutCapture,
		FString&)
	{
		OutCapture = MakeAuthorityCapture(
			Catalog, KnowledgeOwner, 10, { DiagramUnknown });
		return true;
	};
	const auto Unknown =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, KnowledgeOwner, UnknownRead);
	TestTrue(TEXT("Duplicate and catalog-external claims fail at their boundaries"),
		Duplicate.IsValid()
			&& Duplicate.Status == EProjectionStatus::AuthorityReadInvalid
			&& !Duplicate.AuthorityRead.IsValid()
			&& Unknown.IsValid()
			&& Unknown.Status == EProjectionStatus::KnowledgeRejected
			&& Unknown.AuthorityRead.IsValid()
			&& !Unknown.Knowledge.IsValid());
	return true;
}

#endif
