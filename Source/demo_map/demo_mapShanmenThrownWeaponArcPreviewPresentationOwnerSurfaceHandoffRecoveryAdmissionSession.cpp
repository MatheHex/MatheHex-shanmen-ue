#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest;
	using FResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionStatus;
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus;
	using ELoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus;
	using EDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using ERecordKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;
	using FStorage =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter;
	using FRecovery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;

	FGuid MakeRequestId(
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId,
		const FGuid& ExpectedJournalId,
		const FGuid& ExpectedCheckpointId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewRecoveryAdmissionRequest.r1"),
			{
				AuthorityDomainId.ToString(EGuidFormats::Digits),
				LineageId.ToString(EGuidFormats::Digits),
				ExpectedJournalId.ToString(EGuidFormats::Digits),
				ExpectedCheckpointId.ToString(EGuidFormats::Digits)
			});
	}
}

bool FRequest::TryCreate(
	const FGuid& InAuthorityDomainId,
	const FGuid& InLineageId,
	const FGuid& InExpectedJournalId,
	const FGuid& InExpectedCheckpointId,
	FRequest& OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = {};
	OutDiagnostic.Reset();
	if (!InAuthorityDomainId.IsValid() || !InLineageId.IsValid()
		|| !InExpectedJournalId.IsValid()
		|| !InExpectedCheckpointId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Recovery admission requires explicit authority, lineage, journal and checkpoint identities.");
		return false;
	}

	OutRequest.AuthorityDomainId = InAuthorityDomainId;
	OutRequest.LineageId = InLineageId;
	OutRequest.ExpectedJournalId = InExpectedJournalId;
	OutRequest.ExpectedCheckpointId = InExpectedCheckpointId;
	OutRequest.RequestId = MakeRequestId(
		InAuthorityDomainId,
		InLineageId,
		InExpectedJournalId,
		InExpectedCheckpointId);
	if (!OutRequest.IsValid())
	{
		OutRequest = {};
		OutDiagnostic = TEXT(
			"Recovery admission request failed deterministic validation.");
		return false;
	}
	OutDiagnostic = TEXT("Explicit recovery admission request created.");
	return true;
}

bool FRequest::IsValid() const
{
	return AuthorityDomainId.IsValid()
		&& LineageId.IsValid()
		&& ExpectedJournalId.IsValid()
		&& ExpectedCheckpointId.IsValid()
		&& RequestId.IsValid()
		&& RequestId == MakeRequestId(
			AuthorityDomainId,
			LineageId,
			ExpectedJournalId,
			ExpectedCheckpointId);
}

bool FResult::IsSuccess() const
{
	return IsValid()
		&& (Status == EStatus::Recovered || Status == EStatus::Replayed);
}

bool FResult::WasRecovered() const
{
	return IsValid() && Status == EStatus::Recovered;
}

bool FResult::IsReplay() const
{
	return IsValid() && Status == EStatus::Replayed;
}

bool FResult::DidMutateSurface() const
{
	return bRecoveryInvoked && RecoveryResult.IsValid()
		&& RecoveryResult.DidMutateSurface();
}

bool FResult::Validate() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == EStatus::RequestRejected)
	{
		return AuthorityReadStatus == EReadStatus::Invalid
			&& BundleLoadStatus == ELoadStatus::Invalid
			&& !bCheckpointAdmitted && !bRecoveryInvoked
			&& !Checkpoint.IsValid() && !RecoveryResult.IsValid();
	}
	if (!Request.IsValid())
	{
		return false;
	}
	if (Status == EStatus::OperationInProgress
		|| Status == EStatus::CurrentJournalRejected)
	{
		return AuthorityReadStatus == EReadStatus::Invalid
			&& BundleLoadStatus == ELoadStatus::Invalid
			&& !AuthorityState.IsValid()
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	if (Status == EStatus::AuthorityMissing)
	{
		return AuthorityReadStatus == EReadStatus::Missing
			&& !AuthorityState.IsValid()
			&& BundleLoadStatus == ELoadStatus::Invalid
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	if (Status == EStatus::AuthorityUnavailable)
	{
		return AuthorityReadStatus == EReadStatus::Unavailable
			&& !AuthorityState.IsValid()
			&& BundleLoadStatus == ELoadStatus::Invalid
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	if (Status == EStatus::AuthorityReadRejected)
	{
		return (AuthorityReadStatus == EReadStatus::Invalid
				|| AuthorityReadStatus == EReadStatus::Rejected)
			&& !AuthorityState.IsValid()
			&& BundleLoadStatus == ELoadStatus::Invalid
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	if (Status == EStatus::AuthorityStateRejected)
	{
		return AuthorityReadStatus == EReadStatus::Current
			&& AuthorityState.IsValid()
			&& BundleLoadStatus == ELoadStatus::Invalid
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}

	const bool bHasTrustedLoad =
		AuthorityReadStatus == EReadStatus::Current
		&& AuthorityState.IsValid();
	if (Status == EStatus::BundleLoadRejected)
	{
		return bHasTrustedLoad && BundleLoadStatus != ELoadStatus::Invalid
			&& BundleLoadStatus != ELoadStatus::Loaded
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	const bool bHasLoadedBundle = bHasTrustedLoad
		&& BundleLoadStatus == ELoadStatus::Loaded
		&& LoadedGeneration >= 1
		&& LoadedBundleId.IsValid()
		&& LoadedJournalId.IsValid();
	if (Status == EStatus::BundleWatermarkMismatch
		|| Status == EStatus::BundleJournalMismatch)
	{
		return bHasLoadedBundle
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	if (Status == EStatus::CheckpointEvidenceRejected)
	{
		return bHasLoadedBundle
			&& !bCheckpointAdmitted && !bRecoveryInvoked;
	}
	if (Status == EStatus::RecoveryRejected)
	{
		return bHasLoadedBundle && LoadedCheckpointId.IsValid()
			&& Checkpoint.IsValid() && bCheckpointAdmitted
			&& bRecoveryInvoked && RecoveryResult.IsValid()
			&& !RecoveryResult.IsAccepted();
	}
	if (Status == EStatus::Recovered || Status == EStatus::Replayed)
	{
		return bHasLoadedBundle && LoadedCheckpointId.IsValid()
			&& Checkpoint.IsValid() && bCheckpointAdmitted
			&& bRecoveryInvoked && RecoveryResult.IsValid()
			&& RecoveryResult.IsAccepted()
			&& ((Status == EStatus::Recovered
					&& RecoveryResult.WasRecovered())
				|| (Status == EStatus::Replayed
					&& RecoveryResult.IsReplay()));
	}
	return false;
}

FResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession::
ExecuteExplicit(
	const FRequest& Request,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext&
		StorageContext,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal&
		CurrentJournal,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner& Owner,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
		ExpectedRetiredSurface,
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
		NewSurface,
	const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageFileSystem&
		FileSystem,
	const Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority&
		Authority)
{
	FResult Result;
	Result.Request = Request;
	const auto Finish = [&Result](
		const EStatus Status,
		const TCHAR* Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.bValidated = Result.Validate();
		return Result;
	};

	if (!Request.IsValid() || !StorageContext.IsValid()
		|| StorageContext.GetExpectedLineageId() != Request.GetLineageId())
	{
		return Finish(
			EStatus::RequestRejected,
			TEXT("Recovery admission rejected invalid request or cross-lineage storage context before callbacks."));
	}
	if (bOperationInProgress)
	{
		return Finish(
			EStatus::OperationInProgress,
			TEXT("Recovery admission rejects re-entrant execution before authority and storage callbacks."));
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord
		LatestRecord;
	if (!CurrentJournal.IsValid()
		|| CurrentJournal.GetJournalId()
			!= Request.GetExpectedJournalId()
		|| CurrentJournal.GetLatestDisposition()
			!= EDisposition::CheckpointPending
		|| !CurrentJournal.TryGetLatestRecord(LatestRecord)
		|| LatestRecord.GetKind() != ERecordKind::CheckpointPrepared)
	{
		return Finish(
			EStatus::CurrentJournalRejected,
			TEXT("Recovery admission requires the caller's exact current pending journal."));
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const auto Read = Authority.Read(
		Request.GetAuthorityDomainId(),
		Request.GetLineageId());
	Result.AuthorityReadStatus = Read.GetStatus();
	if (!Read.IsValid())
	{
		return Finish(
			EStatus::AuthorityReadRejected,
			TEXT("Recovery admission received a malformed trusted authority read result."));
	}
	if (Read.GetStatus() == EReadStatus::Missing)
	{
		return Finish(
			EStatus::AuthorityMissing,
			TEXT("Recovery admission cannot execute without a committed trusted watermark."));
	}
	if (Read.GetStatus() == EReadStatus::Rejected)
	{
		return Finish(
			EStatus::AuthorityReadRejected,
			TEXT("Trusted authority rejected the recovery admission read."));
	}
	if (Read.GetStatus() == EReadStatus::Unavailable)
	{
		return Finish(
			EStatus::AuthorityUnavailable,
			TEXT("Trusted authority was unavailable during recovery admission."));
	}

	Result.AuthorityState = Read.GetState();
	if (!Result.AuthorityState.IsValid()
		|| Result.AuthorityState.GetAuthorityDomainId()
			!= Request.GetAuthorityDomainId()
		|| Result.AuthorityState.GetLineageId()
			!= Request.GetLineageId())
	{
		return Finish(
			EStatus::AuthorityStateRejected,
			TEXT("Trusted watermark state did not match the explicit authority domain and lineage."));
	}

	FStorage Storage;
	const auto Loaded = Storage.Load(
		StorageContext,
		Result.AuthorityState.GetGeneration(),
		FileSystem);
	Result.BundleLoadStatus = Loaded.GetStatus();
	if (!Loaded.IsSuccess())
	{
		return Finish(
			EStatus::BundleLoadRejected,
			TEXT("Recovery admission could not load canonical bundle evidence at the trusted watermark."));
	}

	Result.LoadedGeneration = Loaded.GetGeneration();
	Result.LoadedBundleId = Loaded.GetBundle().GetBundleId();
	Result.LoadedJournalId = Loaded.GetBundle().GetJournal().GetJournalId();
	FGuid LoadedLineageId;
	if (Result.LoadedGeneration != Result.AuthorityState.GetGeneration()
		|| Result.LoadedBundleId != Result.AuthorityState.GetBundleId()
		|| !FStorage::TryDeriveLineageId(
			Loaded.GetBundle(), LoadedLineageId)
		|| LoadedLineageId != Request.GetLineageId())
	{
		return Finish(
			EStatus::BundleWatermarkMismatch,
			TEXT("Loaded bundle did not exactly match the trusted generation, bundle identity and lineage."));
	}
	if (Result.LoadedJournalId != Request.GetExpectedJournalId()
		|| Result.LoadedJournalId != CurrentJournal.GetJournalId()
		|| Loaded.GetBundle().GetJournal().GetRecordCount()
			!= CurrentJournal.GetRecordCount())
	{
		return Finish(
			EStatus::BundleJournalMismatch,
			TEXT("Trusted bundle did not contain the caller's exact current pending journal."));
	}

	if (!Loaded.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			CurrentJournal,
			Result.AuthorityState.GetGeneration(),
			Result.Checkpoint)
		|| !Result.Checkpoint.IsValid()
		|| Result.Checkpoint.GetCheckpointId()
			!= Request.GetExpectedCheckpointId()
		|| !LatestRecord.MatchesCheckpoint(Result.Checkpoint))
	{
		return Finish(
			EStatus::CheckpointEvidenceRejected,
			TEXT("Trusted bundle could not rehydrate the exact explicitly requested pending checkpoint."));
	}
	Result.LoadedCheckpointId = Result.Checkpoint.GetCheckpointId();
	Result.bCheckpointAdmitted = true;

	FRecovery Recovery;
	Result.bRecoveryInvoked = true;
	Result.RecoveryResult = Recovery.Execute(
		Owner,
		Result.Checkpoint,
		ExpectedRetiredSurface,
		NewSurface);
	if (!Result.RecoveryResult.IsValid()
		|| !Result.RecoveryResult.IsAccepted())
	{
		return Finish(
			EStatus::RecoveryRejected,
			TEXT("P20.48 fresh live Owner, Host, Adapter or surface authority rejected recovery."));
	}
	if (Result.RecoveryResult.WasRecovered())
	{
		return Finish(
			EStatus::Recovered,
			TEXT("Explicit trusted admission executed one P20.48 zero-surface-mutation recovery."));
	}
	if (Result.RecoveryResult.IsReplay())
	{
		return Finish(
			EStatus::Replayed,
			TEXT("Explicit trusted admission replayed the already recovered P20.48 checkpoint."));
	}
	return Finish(
		EStatus::RecoveryRejected,
		TEXT("P20.48 returned an unsupported accepted recovery result."));
}
