#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;
	using FStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter;
	using FRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState;
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus;
	using EAdvanceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus;
	using ECommitStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus;

	FGuid MakeRequestId(
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId,
		const int32 ExpectedGeneration,
		const int32 TargetGeneration,
		const FGuid& BundleId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewRecoveryBundleWatermarkAdvanceRequest.r1"),
			{
				AuthorityDomainId.ToString(EGuidFormats::Digits),
				LineageId.ToString(EGuidFormats::Digits),
				FString::FromInt(ExpectedGeneration),
				FString::FromInt(TargetGeneration),
				BundleId.ToString(EGuidFormats::Digits)
			});
	}

	FGuid MakeReceiptId(const FRequest& Request)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewRecoveryBundleWatermarkAdvanceReceipt.r1"),
			{
				Request.GetRequestId().ToString(EGuidFormats::Digits),
				Request.GetAuthorityDomainId().ToString(EGuidFormats::Digits),
				Request.GetLineageId().ToString(EGuidFormats::Digits),
				FString::FromInt(Request.GetExpectedGeneration()),
				FString::FromInt(Request.GetTargetGeneration()),
				Request.GetBundleId().ToString(EGuidFormats::Digits)
			});
	}

	bool IsFailureReadStatus(const EReadStatus Status)
	{
		return Status == EReadStatus::Rejected
			|| Status == EReadStatus::Unavailable;
	}

	bool IsFailureAdvanceStatus(const EAdvanceStatus Status)
	{
		return Status == EAdvanceStatus::Conflict
			|| Status == EAdvanceStatus::Rejected
			|| Status == EAdvanceStatus::Unavailable;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest::
TryCreate(
	const FGuid& InAuthorityDomainId,
	const FGuid& InLineageId,
	const int32 InExpectedGeneration,
	const int32 InTargetGeneration,
	const FGuid& InBundleId,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest&
		OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = {};
	OutDiagnostic.Reset();
	if (!InAuthorityDomainId.IsValid() || !InLineageId.IsValid()
		|| !InBundleId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Watermark advance requires valid authority, lineage and bundle identities.");
		return false;
	}
	if (InExpectedGeneration < 0
		|| InExpectedGeneration >= FBundle::MaximumGeneration()
		|| InTargetGeneration < 1
		|| InTargetGeneration > FBundle::MaximumGeneration()
		|| InTargetGeneration <= InExpectedGeneration)
	{
		OutDiagnostic = TEXT(
			"Watermark advance must move monotonically from generation zero through eight.");
		return false;
	}

	OutRequest.AuthorityDomainId = InAuthorityDomainId;
	OutRequest.LineageId = InLineageId;
	OutRequest.ExpectedGeneration = InExpectedGeneration;
	OutRequest.TargetGeneration = InTargetGeneration;
	OutRequest.BundleId = InBundleId;
	OutRequest.RequestId = MakeRequestId(
		InAuthorityDomainId,
		InLineageId,
		InExpectedGeneration,
		InTargetGeneration,
		InBundleId);
	if (!OutRequest.IsValid())
	{
		OutRequest = {};
		OutDiagnostic = TEXT(
			"Watermark advance request failed deterministic validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Watermark advance request created.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest::
IsValid() const
{
	return AuthorityDomainId.IsValid()
		&& LineageId.IsValid()
		&& BundleId.IsValid()
		&& ExpectedGeneration >= 0
		&& ExpectedGeneration < FBundle::MaximumGeneration()
		&& TargetGeneration >= 1
		&& TargetGeneration <= FBundle::MaximumGeneration()
		&& TargetGeneration > ExpectedGeneration
		&& RequestId.IsValid()
		&& RequestId == MakeRequestId(
			AuthorityDomainId,
			LineageId,
			ExpectedGeneration,
			TargetGeneration,
			BundleId);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt::
TryCreate(
	const FRequest& Request,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt&
		OutReceipt)
{
	OutReceipt = {};
	if (!Request.IsValid())
	{
		return false;
	}
	OutReceipt.AuthorityDomainId = Request.GetAuthorityDomainId();
	OutReceipt.LineageId = Request.GetLineageId();
	OutReceipt.PreviousGeneration = Request.GetExpectedGeneration();
	OutReceipt.CommittedGeneration = Request.GetTargetGeneration();
	OutReceipt.BundleId = Request.GetBundleId();
	OutReceipt.RequestId = Request.GetRequestId();
	OutReceipt.ReceiptId = MakeReceiptId(Request);
	if (!OutReceipt.IsValid())
	{
		OutReceipt = {};
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt::
IsValid() const
{
	FRequest Rebuilt;
	FString Diagnostic;
	return FRequest::TryCreate(
			AuthorityDomainId,
			LineageId,
			PreviousGeneration,
			CommittedGeneration,
			BundleId,
			Rebuilt,
			Diagnostic)
		&& RequestId == Rebuilt.GetRequestId()
		&& ReceiptId.IsValid()
		&& ReceiptId == MakeReceiptId(Rebuilt);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt::
Matches(const FRequest& Request) const
{
	return IsValid()
		&& Request.IsValid()
		&& AuthorityDomainId == Request.GetAuthorityDomainId()
		&& LineageId == Request.GetLineageId()
		&& PreviousGeneration == Request.GetExpectedGeneration()
		&& CommittedGeneration == Request.GetTargetGeneration()
		&& BundleId == Request.GetBundleId()
		&& RequestId == Request.GetRequestId()
		&& ReceiptId == MakeReceiptId(Request);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState::
TryCreate(
	const FReceipt& InLastReceipt,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState&
		OutState)
{
	OutState = {};
	if (!InLastReceipt.IsValid())
	{
		return false;
	}
	OutState.LastReceipt = InLastReceipt;
	if (!OutState.IsValid())
	{
		OutState = {};
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState::
IsValid() const
{
	return LastReceipt.IsValid()
		&& LastReceipt.GetCommittedGeneration() >= 1
		&& LastReceipt.GetCommittedGeneration() <= FBundle::MaximumGeneration();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState::
MatchesTarget(
	const FGuid& InAuthorityDomainId,
	const FGuid& InLineageId,
	const int32 InGeneration,
	const FGuid& InBundleId) const
{
	return IsValid()
		&& GetAuthorityDomainId() == InAuthorityDomainId
		&& GetLineageId() == InLineageId
		&& GetGeneration() == InGeneration
		&& GetBundleId() == InBundleId;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState::
MatchesRequest(const FRequest& Request) const
{
	return IsValid()
		&& LastReceipt.Matches(Request);
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult::
Missing(const FString& InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
		Result;
	Result.Status = EReadStatus::Missing;
	Result.Diagnostic = InDiagnostic;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult::
Current(
	const FState& InState,
	const FString& InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
		Result;
	if (!InState.IsValid())
	{
		return Result;
	}
	Result.Status = EReadStatus::Current;
	Result.Diagnostic = InDiagnostic;
	Result.State = InState;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult::
Failure(
	const EReadStatus InStatus,
	const FString& InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult
		Result;
	if (!IsFailureReadStatus(InStatus))
	{
		return Result;
	}
	Result.Status = InStatus;
	Result.Diagnostic = InDiagnostic;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult::
IsValid() const
{
	if (Status == EReadStatus::Missing
		|| IsFailureReadStatus(Status))
	{
		return !State.IsValid();
	}
	return Status == EReadStatus::Current
		&& State.IsValid();
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult::
Committed(
	const EAdvanceStatus InStatus,
	const FReceipt& InReceipt,
	const FState& InState,
	const FString& InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
		Result;
	if ((InStatus != EAdvanceStatus::Advanced
			&& InStatus != EAdvanceStatus::AlreadyCurrent)
		|| !InReceipt.IsValid()
		|| !InState.IsValid()
		|| InState.GetLastReceipt().GetReceiptId()
			!= InReceipt.GetReceiptId())
	{
		return Result;
	}
	Result.Status = InStatus;
	Result.Diagnostic = InDiagnostic;
	Result.Receipt = InReceipt;
	Result.State = InState;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult::
Failure(
	const EAdvanceStatus InStatus,
	const FString& InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
		Result;
	if (!IsFailureAdvanceStatus(InStatus))
	{
		return Result;
	}
	Result.Status = InStatus;
	Result.Diagnostic = InDiagnostic;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult::
Unknown(const FString& InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult
		Result;
	Result.Status = EAdvanceStatus::OutcomeUnknown;
	Result.Diagnostic = InDiagnostic;
	Result.bMayHaveAdvanced = true;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult::
IsValid() const
{
	if (Status == EAdvanceStatus::Advanced
		|| Status == EAdvanceStatus::AlreadyCurrent)
	{
		return !bMayHaveAdvanced
			&& Receipt.IsValid()
			&& State.IsValid()
			&& State.GetLastReceipt().GetReceiptId()
				== Receipt.GetReceiptId();
	}
	if (Status == EAdvanceStatus::OutcomeUnknown)
	{
		return bMayHaveAdvanced
			&& !Receipt.IsValid()
			&& !State.IsValid();
	}
	return IsFailureAdvanceStatus(Status)
		&& !bMayHaveAdvanced
		&& !Receipt.IsValid()
		&& !State.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult::
IsSuccess() const
{
	return IsValid()
		&& (Status == EAdvanceStatus::Advanced
			|| Status == EAdvanceStatus::AlreadyCurrent);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult::
IsSuccess() const
{
	return Status == ECommitStatus::Committed
		|| Status == ECommitStatus::AlreadyCommitted
		|| Status == ECommitStatus::CommittedAfterAuthorityRecheck;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitCoordinator::
CommitBundleThenAdvance(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
		StorageContext,
	const FBundle& Bundle,
	const FGuid& AuthorityDomainId,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem&
		FileSystem,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
		Authority) const
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitResult
		Result;
	Result.AuthorityDomainId = AuthorityDomainId;
	Result.BundleId = Bundle.GetBundleId();
	Result.TargetGeneration = Bundle.GetGeneration();
	if (!AuthorityDomainId.IsValid()
		|| !StorageContext.IsValid()
		|| !Bundle.IsValid()
		|| !FStorageAdapter::TryDeriveLineageId(Bundle, Result.LineageId)
		|| Result.LineageId != StorageContext.GetExpectedLineageId())
	{
		Result.Status = ECommitStatus::InputRejected;
		Result.Diagnostic = TEXT(
			"Watermark commit rejected invalid or cross-lineage input before authority and storage callbacks.");
		return Result;
	}

	const auto Read = Authority.Read(AuthorityDomainId, Result.LineageId);
	Result.AuthorityReadStatus = Read.GetStatus();
	if (!Read.IsValid())
	{
		Result.Status = ECommitStatus::AuthorityReadRejected;
		Result.Diagnostic = TEXT(
			"Watermark authority returned a malformed read result.");
		return Result;
	}
	if (Read.GetStatus() == EReadStatus::Rejected)
	{
		Result.Status = ECommitStatus::AuthorityReadRejected;
		Result.Diagnostic = Read.GetDiagnostic();
		return Result;
	}
	if (Read.GetStatus() == EReadStatus::Unavailable)
	{
		Result.Status = ECommitStatus::AuthorityUnavailable;
		Result.Diagnostic = Read.GetDiagnostic();
		return Result;
	}

	const bool bHadCurrentState =
		Read.GetStatus() == EReadStatus::Current;
	if (bHadCurrentState)
	{
		Result.State = Read.GetState();
		if (!Result.State.IsValid()
			|| Result.State.GetAuthorityDomainId() != AuthorityDomainId
			|| Result.State.GetLineageId() != Result.LineageId)
		{
			Result.Status = ECommitStatus::AuthorityStateRejected;
			Result.Diagnostic = TEXT(
				"Watermark authority state did not match the requested trust domain and lineage.");
			return Result;
		}
		Result.PreviousGeneration = Result.State.GetGeneration();
		if (Result.PreviousGeneration > Result.TargetGeneration)
		{
			Result.Status = ECommitStatus::AuthorityAheadOfBundle;
			Result.Diagnostic = TEXT(
				"Trusted watermark is newer than the candidate bundle.");
			return Result;
		}
		if (Result.PreviousGeneration == Result.TargetGeneration
			&& Result.State.GetBundleId() != Bundle.GetBundleId())
		{
			Result.Status = ECommitStatus::AuthorityBundleConflict;
			Result.Diagnostic = TEXT(
				"Trusted watermark binds different evidence in the candidate generation.");
			return Result;
		}
	}
	else
	{
		Result.PreviousGeneration = 0;
	}

	const int32 MinimumGeneration =
		FMath::Max(1, Result.PreviousGeneration);
	FStorageAdapter Storage;
	const auto Saved = Storage.Save(
		StorageContext,
		Bundle,
		MinimumGeneration,
		FileSystem);
	Result.BundleSaveStatus = Saved.GetStatus();
	if (!Saved.IsSuccess() && !Saved.DidReplacePrimary())
	{
		Result.Status = ECommitStatus::BundleSaveRejected;
		Result.Diagnostic = Saved.GetDiagnostic();
		return Result;
	}

	const auto Loaded = Storage.Load(
		StorageContext,
		MinimumGeneration,
		FileSystem);
	Result.BundleLoadStatus = Loaded.GetStatus();
	if (!Loaded.IsSuccess()
		|| Loaded.GetGeneration() != Result.TargetGeneration
		|| !Loaded.GetBundle().Matches(Bundle))
	{
		Result.Status = ECommitStatus::BundleVerificationRejected;
		Result.Diagnostic = Loaded.GetDiagnostic().IsEmpty()
			? TEXT(
				"Committed bundle could not be verified before watermark advance.")
			: Loaded.GetDiagnostic();
		return Result;
	}
	Result.bBundleVerified = true;

	if (bHadCurrentState
		&& Result.PreviousGeneration == Result.TargetGeneration)
	{
		Result.Status = ECommitStatus::AlreadyCommitted;
		Result.Diagnostic = TEXT(
			"Exact bundle and trusted watermark were already current.");
		Result.Receipt = Result.State.GetLastReceipt();
		Result.bWatermarkCurrent = true;
		return Result;
	}

	FRequest Request;
	FString RequestDiagnostic;
	if (!FRequest::TryCreate(
			AuthorityDomainId,
			Result.LineageId,
			Result.PreviousGeneration,
			Result.TargetGeneration,
			Bundle.GetBundleId(),
			Request,
			RequestDiagnostic))
	{
		Result.Status = ECommitStatus::WatermarkAdvanceRejected;
		Result.Diagnostic = RequestDiagnostic;
		return Result;
	}

	const auto Advanced = Authority.CompareAndAdvance(Request);
	Result.AuthorityAdvanceStatus = Advanced.GetStatus();
	if (!Advanced.IsValid())
	{
		Result.Status = ECommitStatus::WatermarkAdvanceRejected;
		Result.Diagnostic = TEXT(
			"Watermark authority returned a malformed advance result after bundle verification.");
		return Result;
	}
	if (Advanced.IsSuccess())
	{
		if (!Advanced.GetReceipt().Matches(Request)
			|| !Advanced.GetState().MatchesRequest(Request))
		{
			Result.Status = ECommitStatus::WatermarkAdvanceRejected;
			Result.Diagnostic = TEXT(
				"Watermark authority success did not match the exact compare-and-advance request.");
			return Result;
		}
		Result.Receipt = Advanced.GetReceipt();
		Result.State = Advanced.GetState();
		Result.bWatermarkCurrent = true;
		Result.Status = ECommitStatus::Committed;
		Result.Diagnostic = TEXT(
			"Bundle was verified before the trusted watermark advanced.");
		return Result;
	}
	if (Advanced.GetStatus() == EAdvanceStatus::Conflict)
	{
		Result.Status = ECommitStatus::WatermarkAdvanceConflict;
		Result.Diagnostic = Advanced.GetDiagnostic();
		return Result;
	}
	if (Advanced.GetStatus() == EAdvanceStatus::Rejected)
	{
		Result.Status = ECommitStatus::WatermarkAdvanceRejected;
		Result.Diagnostic = Advanced.GetDiagnostic();
		return Result;
	}
	if (Advanced.GetStatus() == EAdvanceStatus::Unavailable)
	{
		Result.Status = ECommitStatus::BundleCommittedWatermarkPending;
		Result.Diagnostic = Advanced.GetDiagnostic();
		return Result;
	}

	const auto Recheck = Authority.Read(
		AuthorityDomainId,
		Result.LineageId);
	Result.AuthorityRecheckStatus = Recheck.GetStatus();
	if (Recheck.IsValid()
		&& Recheck.GetStatus() == EReadStatus::Current
		&& Recheck.GetState().MatchesRequest(Request))
	{
		Result.State = Recheck.GetState();
		Result.Receipt = Result.State.GetLastReceipt();
		Result.bWatermarkCurrent = true;
		Result.Status = ECommitStatus::CommittedAfterAuthorityRecheck;
		Result.Diagnostic = TEXT(
			"Unknown watermark outcome was resolved by one exact authority re-read.");
		return Result;
	}

	Result.Status = ECommitStatus::WatermarkOutcomeUnresolved;
	Result.Diagnostic = TEXT(
		"Bundle is committed but one authority re-read could not prove the watermark advance.");
	return Result;
}
