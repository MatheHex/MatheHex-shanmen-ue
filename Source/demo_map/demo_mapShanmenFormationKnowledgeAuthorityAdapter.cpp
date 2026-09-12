#include "demo_mapShanmenFormationKnowledgeAuthorityAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EProjectionStatus =
		Edemo_mapShanmenFormationKnowledgeProjectionStatus;

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

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool HasCanonicalDiagramOrder(const TArray<FName>& DiagramIds)
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

bool Fdemo_mapShanmenFormationKnowledgeAuthorityRead::TryCapture(
	const Fdemo_mapShanmenFormationKnowledgeAuthorityCapture& Capture,
	Fdemo_mapShanmenFormationKnowledgeAuthorityRead& OutRead,
	FString& OutDiagnostic)
{
	OutRead = Fdemo_mapShanmenFormationKnowledgeAuthorityRead();
	OutDiagnostic.Reset();
	if (!Capture.OwnerId.IsValid()
		|| Capture.AuthorityRevision < 0
		|| !Capture.CatalogId.IsValid()
		|| !Capture.Content.IsValid())
	{
		OutDiagnostic =
			TEXT("Formation knowledge authority returned invalid identity evidence.");
		return false;
	}

	TArray<FName> CanonicalIds = Capture.KnownDiagramDefinitionIds;
	CanonicalIds.Sort([](const FName Left, const FName Right)
	{
		return Left.LexicalLess(Right);
	});
	if (!HasCanonicalDiagramOrder(CanonicalIds))
	{
		OutDiagnostic =
			TEXT("Formation knowledge authority returned invalid or duplicate claims.");
		return false;
	}

	OutRead.OwnerId = Capture.OwnerId;
	OutRead.AuthorityRevision = Capture.AuthorityRevision;
	OutRead.CatalogId = Capture.CatalogId;
	OutRead.Content = Capture.Content;
	OutRead.KnownDiagramDefinitionIds = MoveTemp(CanonicalIds);
	OutRead.ReadId = BuildReadId(
		OutRead.OwnerId,
		OutRead.AuthorityRevision,
		OutRead.CatalogId,
		OutRead.Content,
		OutRead.KnownDiagramDefinitionIds);
	if (!OutRead.IsValid())
	{
		OutRead = Fdemo_mapShanmenFormationKnowledgeAuthorityRead();
		OutDiagnostic =
			TEXT("Formation knowledge authority read could not enter its immutable contract.");
		return false;
	}
	OutDiagnostic = TEXT("Formation knowledge authority read captured.");
	return true;
}

bool Fdemo_mapShanmenFormationKnowledgeAuthorityRead::IsValid() const
{
	return ReadId.IsValid()
		&& OwnerId.IsValid()
		&& AuthorityRevision >= 0
		&& CatalogId.IsValid()
		&& Content.IsValid()
		&& HasCanonicalDiagramOrder(KnownDiagramDefinitionIds)
		&& ReadId == BuildReadId(
			OwnerId,
			AuthorityRevision,
			CatalogId,
			Content,
			KnownDiagramDefinitionIds);
}

FGuid Fdemo_mapShanmenFormationKnowledgeAuthorityRead::BuildReadId(
	const FGuid& OwnerId,
	const int64 AuthorityRevision,
	const FGuid& CatalogId,
	const FShanmenContentStamp& Content,
	const TArray<FName>& KnownDiagramDefinitionIds)
{
	if (!OwnerId.IsValid()
		|| AuthorityRevision < 0
		|| !CatalogId.IsValid()
		|| !Content.IsValid()
		|| !HasCanonicalDiagramOrder(KnownDiagramDefinitionIds))
	{
		return FGuid();
	}
	TArray<FString> Parts = {
		GuidDigits(OwnerId),
		LexToString(AuthorityRevision),
		GuidDigits(CatalogId),
		CanonicalName(Content.Version),
		Content.Digest,
		FString::FromInt(KnownDiagramDefinitionIds.Num())
	};
	for (const FName DiagramId : KnownDiagramDefinitionIds)
	{
		Parts.Add(CanonicalName(DiagramId));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.KnowledgeAuthorityRead.r1"), Parts);
}

bool Fdemo_mapShanmenFormationKnowledgeProjectionResult::IsValid() const
{
	if (Status == EProjectionStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	switch (Status)
	{
	case EProjectionStatus::CatalogInvalid:
	case EProjectionStatus::OwnerInvalid:
		return AuthorityReadCount == 0
			&& !AuthorityRead.IsValid()
			&& !Knowledge.IsValid();
	case EProjectionStatus::AuthorityUnavailable:
	case EProjectionStatus::AuthorityReadInvalid:
		return AuthorityReadCount == 1
			&& !AuthorityRead.IsValid()
			&& !Knowledge.IsValid();
	case EProjectionStatus::OwnerMismatch:
	case EProjectionStatus::CatalogMismatch:
	case EProjectionStatus::KnowledgeRejected:
		return AuthorityReadCount == 1
			&& AuthorityRead.IsValid()
			&& !Knowledge.IsValid();
	case EProjectionStatus::Projected:
		return AuthorityReadCount == 1
			&& AuthorityRead.IsValid()
			&& Knowledge.IsValid()
			&& Knowledge.GetOwnerId() == AuthorityRead.GetOwnerId()
			&& Knowledge.GetAuthorityRevision()
				== AuthorityRead.GetAuthorityRevision()
			&& Knowledge.GetCatalogId() == AuthorityRead.GetCatalogId()
			&& ContentMatches(
				Knowledge.GetContent(), AuthorityRead.GetContent())
			&& Knowledge.GetKnownDiagramDefinitionIds()
				== AuthorityRead.GetKnownDiagramDefinitionIds();
	case EProjectionStatus::Invalid:
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationKnowledgeProjectionResult::IsProjected() const
{
	return Status == EProjectionStatus::Projected && IsValid();
}

Fdemo_mapShanmenFormationKnowledgeProjectionResult
Fdemo_mapShanmenFormationKnowledgeAuthorityAdapter::Project(
	const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
	const FGuid& RequestedOwnerId,
	FReadAuthority ReadAuthority)
{
	Fdemo_mapShanmenFormationKnowledgeProjectionResult Result;
	if (!Catalog.IsValid())
	{
		Result.Status = EProjectionStatus::CatalogInvalid;
		Result.Diagnostic =
			TEXT("Formation knowledge projection requires a valid active catalog.");
		return Result;
	}
	if (!RequestedOwnerId.IsValid())
	{
		Result.Status = EProjectionStatus::OwnerInvalid;
		Result.Diagnostic =
			TEXT("Formation knowledge projection requires one valid owner.");
		return Result;
	}

	Fdemo_mapShanmenFormationKnowledgeAuthorityCapture Capture;
	FString ReadDiagnostic;
	Result.AuthorityReadCount = 1;
	if (!ReadAuthority(RequestedOwnerId, Capture, ReadDiagnostic))
	{
		Result.Status = EProjectionStatus::AuthorityUnavailable;
		Result.Diagnostic = ReadDiagnostic.IsEmpty()
			? FString(TEXT("Formation knowledge authority is unavailable."))
			: MoveTemp(ReadDiagnostic);
		return Result;
	}
	if (!Fdemo_mapShanmenFormationKnowledgeAuthorityRead::TryCapture(
			Capture, Result.AuthorityRead, ReadDiagnostic))
	{
		Result.Status = EProjectionStatus::AuthorityReadInvalid;
		Result.Diagnostic = MoveTemp(ReadDiagnostic);
		return Result;
	}
	if (Result.AuthorityRead.GetOwnerId() != RequestedOwnerId)
	{
		Result.Status = EProjectionStatus::OwnerMismatch;
		Result.Diagnostic =
			TEXT("Formation knowledge authority returned another owner's evidence.");
		return Result;
	}
	if (Result.AuthorityRead.GetCatalogId() != Catalog.GetCatalogId()
		|| !ContentMatches(
			Result.AuthorityRead.GetContent(), Catalog.GetContent()))
	{
		Result.Status = EProjectionStatus::CatalogMismatch;
		Result.Diagnostic =
			TEXT("Formation knowledge authority evidence does not match the active catalog.");
		return Result;
	}

	if (!Fdemo_mapShanmenFormationDiagramKnowledgeSnapshot::TryCapture(
			Catalog,
			RequestedOwnerId,
			Result.AuthorityRead.GetAuthorityRevision(),
			Result.AuthorityRead.GetKnownDiagramDefinitionIds(),
			Result.Knowledge,
			ReadDiagnostic))
	{
		Result.Status = EProjectionStatus::KnowledgeRejected;
		Result.Diagnostic = MoveTemp(ReadDiagnostic);
		return Result;
	}
	Result.Status = EProjectionStatus::Projected;
	Result.Diagnostic = TEXT("Formation knowledge projected from one authority read.");
	return Result;
}
