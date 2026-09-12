#include "demo_mapShanmenFormationDiagramAccessAdapter.h"

namespace
{
	using EAccessStatus = Edemo_mapShanmenFormationDiagramAccessStatus;

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool HasValidRequestEvidence(
		const Fdemo_mapShanmenFormationDiagramAccessResult& Result)
	{
		return Result.ActiveCatalogId.IsValid()
			&& Result.ActiveContent.IsValid()
			&& Result.RequestedOwnerId.IsValid()
			&& !Result.RequestedDiagramDefinitionId.IsNone();
	}

	bool ProjectionMatchesRequest(
		const Fdemo_mapShanmenFormationDiagramAccessResult& Result)
	{
		return Result.KnowledgeProjection.IsProjected()
			&& Result.KnowledgeProjection.Knowledge.GetCatalogId()
				== Result.ActiveCatalogId
			&& Result.KnowledgeProjection.Knowledge.GetOwnerId()
				== Result.RequestedOwnerId
			&& ContentMatches(
				Result.KnowledgeProjection.Knowledge.GetContent(),
				Result.ActiveContent);
	}
}

bool Fdemo_mapShanmenFormationDiagramAccessResult::IsValid() const
{
	if (Status == EAccessStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}

	const bool HasNoNestedEvidence =
		!KnowledgeProjection.IsValid() && !Selection.IsValid();
	switch (Status)
	{
	case EAccessStatus::CatalogInvalid:
		return !ActiveCatalogId.IsValid()
			&& !ActiveContent.IsValid()
			&& KnowledgeProjectionCount == 0
			&& SelectionInvocationCount == 0
			&& HasNoNestedEvidence;
	case EAccessStatus::OwnerInvalid:
		return ActiveCatalogId.IsValid()
			&& ActiveContent.IsValid()
			&& !RequestedOwnerId.IsValid()
			&& KnowledgeProjectionCount == 0
			&& SelectionInvocationCount == 0
			&& HasNoNestedEvidence;
	case EAccessStatus::RequestInvalid:
		return ActiveCatalogId.IsValid()
			&& ActiveContent.IsValid()
			&& RequestedOwnerId.IsValid()
			&& RequestedDiagramDefinitionId.IsNone()
			&& KnowledgeProjectionCount == 0
			&& SelectionInvocationCount == 0
			&& HasNoNestedEvidence;
	case EAccessStatus::DiagramUnknown:
		return HasValidRequestEvidence(*this)
			&& KnowledgeProjectionCount == 0
			&& SelectionInvocationCount == 0
			&& HasNoNestedEvidence;
	case EAccessStatus::KnowledgeProjectionRejected:
		return HasValidRequestEvidence(*this)
			&& KnowledgeProjectionCount == 1
			&& SelectionInvocationCount == 0
			&& KnowledgeProjection.IsValid()
			&& !KnowledgeProjection.IsProjected()
			&& KnowledgeProjection.Status
				!= Edemo_mapShanmenFormationKnowledgeProjectionStatus::
					CatalogInvalid
			&& KnowledgeProjection.Status
				!= Edemo_mapShanmenFormationKnowledgeProjectionStatus::OwnerInvalid
			&& Diagnostic == KnowledgeProjection.Diagnostic
			&& !Selection.IsValid();
	case EAccessStatus::SelectionRejected:
		return HasValidRequestEvidence(*this)
			&& KnowledgeProjectionCount == 1
			&& SelectionInvocationCount == 1
			&& ProjectionMatchesRequest(*this)
			&& Selection.IsValid()
			&& !Selection.IsSelected()
			&& Selection.Status
				== Edemo_mapShanmenFormationDiagramSelectionStatus::
					DiagramUnavailable
			&& Diagnostic == Selection.Diagnostic;
	case EAccessStatus::Selected:
		return HasValidRequestEvidence(*this)
			&& KnowledgeProjectionCount == 1
			&& SelectionInvocationCount == 1
			&& ProjectionMatchesRequest(*this)
			&& Selection.IsSelected()
			&& Selection.Selection.GetCatalogId() == ActiveCatalogId
			&& Selection.Selection.GetKnowledgeSnapshotId()
				== KnowledgeProjection.Knowledge.GetSnapshotId()
			&& Selection.Selection.GetOwnerId() == RequestedOwnerId
			&& Selection.Selection.GetKnowledgeAuthorityRevision()
				== KnowledgeProjection.Knowledge.GetAuthorityRevision()
			&& ContentMatches(
				Selection.Selection.GetContent(), ActiveContent)
			&& Selection.Selection.GetDiagram().GetDiagramDefinitionId()
				== RequestedDiagramDefinitionId;
	case EAccessStatus::Invalid:
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationDiagramAccessResult::IsSelected() const
{
	return Status == EAccessStatus::Selected && IsValid();
}

Fdemo_mapShanmenFormationDiagramAccessResult
Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
	const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
	const FGuid& RequestedOwnerId,
	const FName RequestedDiagramDefinitionId,
	FReadAuthority ReadAuthority)
{
	Fdemo_mapShanmenFormationDiagramAccessResult Result;
	Result.RequestedOwnerId = RequestedOwnerId;
	Result.RequestedDiagramDefinitionId = RequestedDiagramDefinitionId;
	if (!Catalog.IsValid())
	{
		Result.Status = EAccessStatus::CatalogInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram access requires a valid active catalog.");
		return Result;
	}
	Result.ActiveCatalogId = Catalog.GetCatalogId();
	Result.ActiveContent = Catalog.GetContent();
	if (!RequestedOwnerId.IsValid())
	{
		Result.Status = EAccessStatus::OwnerInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram access requires one valid owner.");
		return Result;
	}
	if (RequestedDiagramDefinitionId.IsNone())
	{
		Result.Status = EAccessStatus::RequestInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram access requires one diagram identity.");
		return Result;
	}
	if (!Catalog.FindDiagram(RequestedDiagramDefinitionId))
	{
		Result.Status = EAccessStatus::DiagramUnknown;
		Result.Diagnostic =
			TEXT("Requested formation diagram is not authored in the active catalog.");
		return Result;
	}

	Result.KnowledgeProjectionCount = 1;
	Result.KnowledgeProjection =
		Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
			Catalog, RequestedOwnerId, ReadAuthority);
	if (!Result.KnowledgeProjection.IsProjected())
	{
		Result.Status = EAccessStatus::KnowledgeProjectionRejected;
		Result.Diagnostic = Result.KnowledgeProjection.Diagnostic;
		return Result;
	}

	Result.SelectionInvocationCount = 1;
	Result.Selection = Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
		Catalog,
		Result.KnowledgeProjection.Knowledge,
		RequestedDiagramDefinitionId);
	if (!Result.Selection.IsSelected())
	{
		Result.Status = EAccessStatus::SelectionRejected;
		Result.Diagnostic = Result.Selection.Diagnostic;
		return Result;
	}

	Result.Status = EAccessStatus::Selected;
	Result.Diagnostic =
		TEXT("Formation diagram selected from one authority revision.");
	return Result;
}
