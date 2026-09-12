#include "demo_mapShanmenFormationDiagramSelectionPort.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using ESelectionStatus =
		Edemo_mapShanmenFormationDiagramSelectionStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString CanonicalName(const FName Value)
	{
		FString Result = Value.ToString();
		Result.ToLowerInline();
		return Result;
	}

	double CanonicalZero(const double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	void AppendDiagramParts(
		const FShanmenFormationDiagramDefinition& Diagram,
		TArray<FString>& OutParts)
	{
		OutParts.Append({
			CanonicalName(Diagram.GetActionDefinitionId()),
			CanonicalName(Diagram.GetDiagramDefinitionId()),
			FString::FromInt(Diagram.GetAnchors().Num())
		});
		for (const FShanmenFormationAnchorDefinition& Anchor :
			Diagram.GetAnchors())
		{
			OutParts.Append({
				FString::FromInt(Anchor.GetOrder()),
				CanonicalName(Anchor.GetAnchorDefinitionId()),
				DoubleBits(Anchor.GetRelativeOffset().X),
				DoubleBits(Anchor.GetRelativeOffset().Y),
				DoubleBits(Anchor.GetRelativeOffset().Z),
				FString::FromInt(Anchor.GetRequirements().Num())
			});
			for (const FShanmenFormationMaterialRequirement& Requirement :
				Anchor.GetRequirements())
			{
				OutParts.Append({
					FString::FromInt(Requirement.GetOrder()),
					CanonicalName(
						Requirement.GetMaterialDefinitionId()),
					FString::FromInt(Requirement.GetQuantity())
				});
			}
		}
	}

	bool HasCanonicalKnownDiagramOrder(const TArray<FName>& DiagramIds)
	{
		for (int32 Index = 0; Index < DiagramIds.Num(); ++Index)
		{
			if (DiagramIds[Index].IsNone()
				|| (Index > 0
					&& (DiagramIds[Index] == DiagramIds[Index - 1]
						|| DiagramIds[Index].LexicalLess(
							DiagramIds[Index - 1]))))
			{
				return false;
			}
		}
		return true;
	}
}

bool Fdemo_mapShanmenFormationDiagramCatalog::TryCapture(
	const Fdemo_mapShanmenFormationDiagramCatalogCapture& Capture,
	Fdemo_mapShanmenFormationDiagramCatalog& OutCatalog,
	FString& OutDiagnostic)
{
	OutCatalog = Fdemo_mapShanmenFormationDiagramCatalog();
	OutDiagnostic.Reset();
	if (!Capture.Content.IsValid())
	{
		OutDiagnostic =
			TEXT("Formation diagram catalog requires a valid content stamp.");
		return false;
	}
	if (Capture.Diagrams.IsEmpty())
	{
		OutDiagnostic =
			TEXT("Formation diagram catalog requires authored diagrams.");
		return false;
	}

	OutCatalog.Content = Capture.Content;
	TSet<FName> DiagramIds;
	for (const FShanmenFormationDiagramCapture& DiagramCapture :
		Capture.Diagrams)
	{
		FShanmenFormationDiagramDefinition Diagram;
		if (!FShanmenFormationDiagramDefinition::TryCapture(
				DiagramCapture, Diagram))
		{
			OutCatalog = Fdemo_mapShanmenFormationDiagramCatalog();
			OutDiagnostic =
				TEXT("Formation diagram catalog contains an invalid diagram.");
			return false;
		}
		if (DiagramIds.Contains(Diagram.GetDiagramDefinitionId()))
		{
			OutCatalog = Fdemo_mapShanmenFormationDiagramCatalog();
			OutDiagnostic =
				TEXT("Formation diagram catalog contains a duplicate diagram identity.");
			return false;
		}
		DiagramIds.Add(Diagram.GetDiagramDefinitionId());
		OutCatalog.Diagrams.Add(Diagram);
	}

	OutCatalog.CatalogId = BuildCatalogId(
		OutCatalog.Content, OutCatalog.Diagrams);
	if (!OutCatalog.IsValid())
	{
		OutCatalog = Fdemo_mapShanmenFormationDiagramCatalog();
		OutDiagnostic =
			TEXT("Formation diagram catalog could not enter its immutable contract.");
		return false;
	}
	OutDiagnostic = TEXT("Formation diagram catalog captured.");
	return true;
}

bool Fdemo_mapShanmenFormationDiagramCatalog::IsValid() const
{
	if (!CatalogId.IsValid() || !Content.IsValid() || Diagrams.IsEmpty())
	{
		return false;
	}
	TSet<FName> DiagramIds;
	for (const FShanmenFormationDiagramDefinition& Diagram : Diagrams)
	{
		if (!Diagram.IsValid()
			|| DiagramIds.Contains(Diagram.GetDiagramDefinitionId()))
		{
			return false;
		}
		DiagramIds.Add(Diagram.GetDiagramDefinitionId());
	}
	return CatalogId == BuildCatalogId(Content, Diagrams);
}

const FShanmenFormationDiagramDefinition*
Fdemo_mapShanmenFormationDiagramCatalog::FindDiagram(
	const FName DiagramDefinitionId) const
{
	if (DiagramDefinitionId.IsNone())
	{
		return nullptr;
	}
	return Diagrams.FindByPredicate(
		[DiagramDefinitionId](
			const FShanmenFormationDiagramDefinition& Diagram)
		{
			return Diagram.GetDiagramDefinitionId() == DiagramDefinitionId;
		});
}

FGuid Fdemo_mapShanmenFormationDiagramCatalog::BuildCatalogId(
	const FShanmenContentStamp& Content,
	const TArray<FShanmenFormationDiagramDefinition>& Diagrams)
{
	if (!Content.IsValid() || Diagrams.IsEmpty())
	{
		return FGuid();
	}
	TArray<FString> Parts = {
		CanonicalName(Content.Version),
		Content.Digest,
		FString::FromInt(Diagrams.Num())
	};
	for (const FShanmenFormationDiagramDefinition& Diagram : Diagrams)
	{
		if (!Diagram.IsValid())
		{
			return FGuid();
		}
		AppendDiagramParts(Diagram, Parts);
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.DiagramCatalog.r1"), Parts);
}

bool Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
	const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
	const FGuid& OwnerId,
	const int64 AuthorityRevision,
	const TArray<FName>& KnownDiagramDefinitionIds,
	Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot& OutSnapshot,
	FString& OutDiagnostic)
{
	OutSnapshot = Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot();
	OutDiagnostic.Reset();
	if (!Catalog.IsValid())
	{
		OutDiagnostic =
			TEXT("Formation diagram knowledge requires a valid catalog.");
		return false;
	}
	if (!OwnerId.IsValid() || AuthorityRevision < 0)
	{
		OutDiagnostic =
			TEXT("Formation diagram knowledge requires valid owner revision evidence.");
		return false;
	}

	TArray<FName> CanonicalIds = KnownDiagramDefinitionIds;
	CanonicalIds.Sort([](const FName Left, const FName Right)
	{
		return Left.LexicalLess(Right);
	});
	TSet<FName> UniqueIds;
	for (const FName DiagramId : CanonicalIds)
	{
		if (DiagramId.IsNone() || UniqueIds.Contains(DiagramId))
		{
			OutDiagnostic =
				TEXT("Formation diagram knowledge contains an invalid or duplicate identity.");
			return false;
		}
		if (!Catalog.FindDiagram(DiagramId))
		{
			OutDiagnostic =
				TEXT("Formation diagram knowledge references a diagram outside its catalog.");
			return false;
		}
		UniqueIds.Add(DiagramId);
	}

	OutSnapshot.CatalogId = Catalog.GetCatalogId();
	OutSnapshot.OwnerId = OwnerId;
	OutSnapshot.AuthorityRevision = AuthorityRevision;
	OutSnapshot.Content = Catalog.GetContent();
	OutSnapshot.KnownDiagramDefinitionIds = MoveTemp(CanonicalIds);
	OutSnapshot.SnapshotId = BuildSnapshotId(
		OutSnapshot.Content,
		OutSnapshot.CatalogId,
		OutSnapshot.OwnerId,
		OutSnapshot.AuthorityRevision,
		OutSnapshot.KnownDiagramDefinitionIds);
	if (!OutSnapshot.IsValid())
	{
		OutSnapshot = Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot();
		OutDiagnostic =
			TEXT("Formation diagram knowledge could not enter its immutable contract.");
		return false;
	}
	OutDiagnostic = TEXT("Formation diagram knowledge captured.");
	return true;
}

bool Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::IsValid() const
{
	return SnapshotId.IsValid()
		&& CatalogId.IsValid()
		&& OwnerId.IsValid()
		&& AuthorityRevision >= 0
		&& Content.IsValid()
		&& HasCanonicalKnownDiagramOrder(KnownDiagramDefinitionIds)
		&& SnapshotId == BuildSnapshotId(
			Content,
			CatalogId,
			OwnerId,
			AuthorityRevision,
			KnownDiagramDefinitionIds);
}

bool Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::Contains(
	const FName DiagramDefinitionId) const
{
	return !DiagramDefinitionId.IsNone()
		&& KnownDiagramDefinitionIds.Contains(DiagramDefinitionId);
}

FGuid Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::BuildSnapshotId(
	const FShanmenContentStamp& Content,
	const FGuid& CatalogId,
	const FGuid& OwnerId,
	const int64 AuthorityRevision,
	const TArray<FName>& KnownDiagramDefinitionIds)
{
	if (!Content.IsValid()
		|| !CatalogId.IsValid()
		|| !OwnerId.IsValid()
		|| AuthorityRevision < 0
		|| !HasCanonicalKnownDiagramOrder(KnownDiagramDefinitionIds))
	{
		return FGuid();
	}
	TArray<FString> Parts = {
		CanonicalName(Content.Version),
		Content.Digest,
		GuidDigits(CatalogId),
		GuidDigits(OwnerId),
		LexToString(AuthorityRevision),
		FString::FromInt(KnownDiagramDefinitionIds.Num())
	};
	for (const FName DiagramId : KnownDiagramDefinitionIds)
	{
		Parts.Add(CanonicalName(DiagramId));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.DiagramKnowledge.r1"), Parts);
}

bool Fdemo_mapShanmenFormationDiagramSelection::IsValid() const
{
	return SelectionId.IsValid()
		&& CatalogId.IsValid()
		&& KnowledgeSnapshotId.IsValid()
		&& OwnerId.IsValid()
		&& KnowledgeAuthorityRevision >= 0
		&& Content.IsValid()
		&& Diagram.IsValid()
		&& SelectionId == BuildSelectionId(
			CatalogId,
			KnowledgeSnapshotId,
			OwnerId,
			KnowledgeAuthorityRevision,
			Content,
			Diagram);
}

FGuid Fdemo_mapShanmenFormationDiagramSelection::BuildSelectionId(
	const FGuid& CatalogId,
	const FGuid& KnowledgeSnapshotId,
	const FGuid& OwnerId,
	const int64 KnowledgeAuthorityRevision,
	const FShanmenContentStamp& Content,
	const FShanmenFormationDiagramDefinition& Diagram)
{
	if (!CatalogId.IsValid()
		|| !KnowledgeSnapshotId.IsValid()
		|| !OwnerId.IsValid()
		|| KnowledgeAuthorityRevision < 0
		|| !Content.IsValid()
		|| !Diagram.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.DiagramSelection.r1"),
		{
			GuidDigits(CatalogId),
			GuidDigits(KnowledgeSnapshotId),
			GuidDigits(OwnerId),
			LexToString(KnowledgeAuthorityRevision),
			CanonicalName(Content.Version),
			Content.Digest,
			CanonicalName(Diagram.GetDiagramDefinitionId())
		});
}

bool Fdemo_mapShanmenFormationDiagramSelectionResult::IsValid() const
{
	if (Status == ESelectionStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	return Status == ESelectionStatus::Selected
		? Selection.IsValid()
		: !Selection.IsValid();
}

bool Fdemo_mapShanmenFormationDiagramSelectionResult::IsSelected() const
{
	return Status == ESelectionStatus::Selected && IsValid();
}

Fdemo_mapShanmenFormationDiagramSelectionResult
Fdemo_mapShanmenFormationDiagramSelectionPort::Select(
	const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
	const Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot& Knowledge,
	const FName RequestedDiagramDefinitionId)
{
	Fdemo_mapShanmenFormationDiagramSelectionResult Result;
	if (!Catalog.IsValid())
	{
		Result.Status = ESelectionStatus::CatalogInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram selection requires a valid catalog.");
		return Result;
	}
	if (!Knowledge.IsValid())
	{
		Result.Status = ESelectionStatus::KnowledgeInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram selection requires valid knowledge evidence.");
		return Result;
	}
	if (Knowledge.GetCatalogId() != Catalog.GetCatalogId()
		|| !ContentMatches(Knowledge.GetContent(), Catalog.GetContent()))
	{
		Result.Status = ESelectionStatus::CatalogMismatch;
		Result.Diagnostic =
			TEXT("Formation diagram knowledge does not match the active catalog.");
		return Result;
	}
	if (RequestedDiagramDefinitionId.IsNone())
	{
		Result.Status = ESelectionStatus::RequestInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram selection requires one diagram identity.");
		return Result;
	}
	const FShanmenFormationDiagramDefinition* Diagram =
		Catalog.FindDiagram(RequestedDiagramDefinitionId);
	if (!Diagram)
	{
		Result.Status = ESelectionStatus::DiagramUnknown;
		Result.Diagnostic =
			TEXT("Requested formation diagram is not authored in the active catalog.");
		return Result;
	}
	if (!Knowledge.Contains(RequestedDiagramDefinitionId))
	{
		Result.Status = ESelectionStatus::DiagramUnavailable;
		Result.Diagnostic =
			TEXT("Requested formation diagram is not known by this owner.");
		return Result;
	}

	Result.Selection.CatalogId = Catalog.GetCatalogId();
	Result.Selection.KnowledgeSnapshotId = Knowledge.GetSnapshotId();
	Result.Selection.OwnerId = Knowledge.GetOwnerId();
	Result.Selection.KnowledgeAuthorityRevision =
		Knowledge.GetAuthorityRevision();
	Result.Selection.Content = Catalog.GetContent();
	Result.Selection.Diagram = *Diagram;
	Result.Selection.SelectionId =
		Fdemo_mapShanmenFormationDiagramSelection::BuildSelectionId(
			Result.Selection.CatalogId,
			Result.Selection.KnowledgeSnapshotId,
			Result.Selection.OwnerId,
			Result.Selection.KnowledgeAuthorityRevision,
			Result.Selection.Content,
			Result.Selection.Diagram);
	if (!Result.Selection.IsValid())
	{
		Result.Selection = Fdemo_mapShanmenFormationDiagramSelection();
		Result.Status = ESelectionStatus::KnowledgeInvalid;
		Result.Diagnostic =
			TEXT("Formation diagram selection could not enter its immutable contract.");
		return Result;
	}
	Result.Status = ESelectionStatus::Selected;
	Result.Diagnostic = TEXT("Known formation diagram selected.");
	return Result;
}
