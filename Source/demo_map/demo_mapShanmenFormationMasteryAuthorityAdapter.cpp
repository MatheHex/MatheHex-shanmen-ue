#include "demo_mapShanmenFormationMasteryAuthorityAdapter.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EProjectionStatus =
		Edemo_mapShanmenFormationMasteryProjectionStatus;

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
}

bool Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
	const Fdemo_mapShanmenFormationMasteryAuthorityCapture& Capture,
	Fdemo_mapShanmenFormationMasteryAuthorityRead& OutRead,
	FString& OutDiagnostic)
{
	OutRead = Fdemo_mapShanmenFormationMasteryAuthorityRead();
	OutDiagnostic.Reset();
	if (!Capture.OwnerId.IsValid()
		|| Capture.AuthorityRevision < 0
		|| !Capture.Content.IsValid())
	{
		OutDiagnostic =
			TEXT("Formation mastery authority returned invalid identity evidence.");
		return false;
	}

	FShanmenFormationMasteryPolicy Policy;
	if (!FShanmenFormationMasteryPolicy::TryCreate(
			Capture.MasteryTier, Policy))
	{
		OutDiagnostic =
			TEXT("Formation mastery authority returned an invalid mastery tier.");
		return false;
	}

	OutRead.OwnerId = Capture.OwnerId;
	OutRead.AuthorityRevision = Capture.AuthorityRevision;
	OutRead.Content = Capture.Content;
	OutRead.MasteryPolicy = Policy;
	OutRead.ReadId = BuildReadId(
		OutRead.OwnerId,
		OutRead.AuthorityRevision,
		OutRead.Content,
		OutRead.MasteryPolicy.GetTier());
	if (!OutRead.IsValid())
	{
		OutRead = Fdemo_mapShanmenFormationMasteryAuthorityRead();
		OutDiagnostic =
			TEXT("Formation mastery authority read could not enter its immutable contract.");
		return false;
	}

	OutDiagnostic = TEXT("Formation mastery authority read captured.");
	return true;
}

bool Fdemo_mapShanmenFormationMasteryAuthorityRead::IsValid() const
{
	return ReadId.IsValid()
		&& OwnerId.IsValid()
		&& AuthorityRevision >= 0
		&& Content.IsValid()
		&& MasteryPolicy.IsValid()
		&& ReadId == BuildReadId(
			OwnerId,
			AuthorityRevision,
			Content,
			MasteryPolicy.GetTier());
}

FGuid Fdemo_mapShanmenFormationMasteryAuthorityRead::BuildReadId(
	const FGuid& OwnerId,
	const int64 AuthorityRevision,
	const FShanmenContentStamp& Content,
	const EShanmenFormationMasteryTier MasteryTier)
{
	if (!OwnerId.IsValid()
		|| AuthorityRevision < 0
		|| !Content.IsValid()
		|| !FShanmenFormationMasteryPolicy::IsTierValid(MasteryTier))
	{
		return FGuid();
	}

	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.MasteryAuthorityRead.r1"),
		{
			GuidDigits(OwnerId),
			LexToString(AuthorityRevision),
			CanonicalName(Content.Version),
			Content.Digest,
			FString::FromInt(static_cast<int32>(MasteryTier))
		});
}

bool Fdemo_mapShanmenFormationMasteryProjectionResult::IsValid() const
{
	if (Status == EProjectionStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}

	switch (Status)
	{
	case EProjectionStatus::ContentInvalid:
		return !ExpectedContent.IsValid()
			&& AuthorityReadCount == 0
			&& !AuthorityRead.IsValid();
	case EProjectionStatus::OwnerInvalid:
		return ExpectedContent.IsValid()
			&& !RequestedOwnerId.IsValid()
			&& AuthorityReadCount == 0
			&& !AuthorityRead.IsValid();
	case EProjectionStatus::AuthorityUnavailable:
	case EProjectionStatus::AuthorityReadInvalid:
		return ExpectedContent.IsValid()
			&& RequestedOwnerId.IsValid()
			&& AuthorityReadCount == 1
			&& !AuthorityRead.IsValid();
	case EProjectionStatus::OwnerMismatch:
		return ExpectedContent.IsValid()
			&& RequestedOwnerId.IsValid()
			&& AuthorityReadCount == 1
			&& AuthorityRead.IsValid()
			&& AuthorityRead.GetOwnerId() != RequestedOwnerId;
	case EProjectionStatus::ContentMismatch:
		return ExpectedContent.IsValid()
			&& RequestedOwnerId.IsValid()
			&& AuthorityReadCount == 1
			&& AuthorityRead.IsValid()
			&& AuthorityRead.GetOwnerId() == RequestedOwnerId
			&& !ContentMatches(
				AuthorityRead.GetContent(), ExpectedContent);
	case EProjectionStatus::Projected:
		return ExpectedContent.IsValid()
			&& RequestedOwnerId.IsValid()
			&& AuthorityReadCount == 1
			&& AuthorityRead.IsValid()
			&& AuthorityRead.GetOwnerId() == RequestedOwnerId
			&& ContentMatches(
				AuthorityRead.GetContent(), ExpectedContent);
	case EProjectionStatus::Invalid:
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationMasteryProjectionResult::IsProjected() const
{
	return Status == EProjectionStatus::Projected && IsValid();
}

Fdemo_mapShanmenFormationMasteryProjectionResult
Fdemo_mapShanmenFormationMasteryAuthorityAdapter::Project(
	const FShanmenContentStamp& ExpectedContent,
	const FGuid& RequestedOwnerId,
	FReadAuthority ReadAuthority)
{
	Fdemo_mapShanmenFormationMasteryProjectionResult Result;
	Result.ExpectedContent = ExpectedContent;
	Result.RequestedOwnerId = RequestedOwnerId;
	if (!ExpectedContent.IsValid())
	{
		Result.Status = EProjectionStatus::ContentInvalid;
		Result.Diagnostic =
			TEXT("Formation mastery projection requires valid active content.");
		return Result;
	}
	if (!RequestedOwnerId.IsValid())
	{
		Result.Status = EProjectionStatus::OwnerInvalid;
		Result.Diagnostic =
			TEXT("Formation mastery projection requires one valid owner.");
		return Result;
	}

	Fdemo_mapShanmenFormationMasteryAuthorityCapture Capture;
	FString ReadDiagnostic;
	Result.AuthorityReadCount = 1;
	if (!ReadAuthority(RequestedOwnerId, Capture, ReadDiagnostic))
	{
		Result.Status = EProjectionStatus::AuthorityUnavailable;
		Result.Diagnostic = ReadDiagnostic.IsEmpty()
			? FString(TEXT("Formation mastery authority is unavailable."))
			: MoveTemp(ReadDiagnostic);
		return Result;
	}
	if (!Fdemo_mapShanmenFormationMasteryAuthorityRead::TryCapture(
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
			TEXT("Formation mastery authority returned another owner's evidence.");
		return Result;
	}
	if (!ContentMatches(
			Result.AuthorityRead.GetContent(), ExpectedContent))
	{
		Result.Status = EProjectionStatus::ContentMismatch;
		Result.Diagnostic =
			TEXT("Formation mastery authority evidence does not match active content.");
		return Result;
	}

	Result.Status = EProjectionStatus::Projected;
	Result.Diagnostic =
		TEXT("Formation mastery projected from one authority read.");
	return Result;
}
