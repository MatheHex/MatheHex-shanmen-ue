#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest;
	using FRotation =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation;
	using FAdoption =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption;
	using FCheckpoint =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using FJournalRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;
	using EDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using FBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;

	FGuid MakeRequestId(
		const FAdoption& Adoption,
		const FCheckpoint& NewCheckpoint)
	{
		if (!Adoption.IsValid() || !Adoption.HasNextGenerationCapacity()
			|| !NewCheckpoint.IsValid()
			|| NewCheckpoint.GetCheckpointId()
				== Adoption.GetRequest()
					.GetCompletionRequest()
					.GetAdmissionRequest()
					.GetExpectedCheckpointId())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest.r1"),
			{
				Adoption.GetAdoptionId().ToString(EGuidFormats::Digits),
				Adoption.GetRequest().GetRequestId().ToString(
					EGuidFormats::Digits),
				Adoption.GetTerminalJournalId().ToString(
					EGuidFormats::Digits),
				FString::FromInt(Adoption.GetGeneration()),
				FString::FromInt(Adoption.GetNextGeneration()),
				Adoption.GetCompletionId().ToString(EGuidFormats::Digits),
				Adoption.GetCompletionDigest().ToString(
					EGuidFormats::Digits),
				NewCheckpoint.GetCheckpointId().ToString(
					EGuidFormats::Digits),
				NewCheckpoint.GetTransitionTicket().GetTicketId().ToString(
					EGuidFormats::Digits),
				NewCheckpoint.GetTransitionTicket().GetRunId().ToString(
					EGuidFormats::Digits)
			});
	}

	bool JournalContainsCheckpointId(
		const FJournal& Journal,
		const FGuid& CheckpointId)
	{
		if (!Journal.IsValid() || !CheckpointId.IsValid())
		{
			return false;
		}
		for (int32 Index = 0; Index < Journal.GetRecordCount(); ++Index)
		{
			FJournalRecord Record;
			if (!Journal.TryGetRecordAt(Index, Record)
				|| !Record.IsValid())
			{
				return true;
			}
			if (Record.GetCheckpointId() == CheckpointId)
			{
				return true;
			}
		}
		return false;
	}

	bool IsExactCheckpointExtension(
		const FJournal& SourceTerminalJournal,
		const FJournal& PendingJournal,
		const FCheckpoint& NewCheckpoint)
	{
		if (!SourceTerminalJournal.IsValid()
			|| SourceTerminalJournal.GetLatestDisposition()
				!= EDisposition::RecoveryCommitted
			|| !PendingJournal.IsValid()
			|| PendingJournal.GetLatestDisposition()
				!= EDisposition::CheckpointPending
			|| !NewCheckpoint.IsValid()
			|| PendingJournal.GetRecordCount()
				!= SourceTerminalJournal.GetRecordCount() + 1
			|| JournalContainsCheckpointId(
				SourceTerminalJournal, NewCheckpoint.GetCheckpointId())
			|| !PendingJournal.MatchesLatestCheckpoint(NewCheckpoint))
		{
			return false;
		}
		for (int32 Index = 0;
			Index < SourceTerminalJournal.GetRecordCount();
			++Index)
		{
			FJournalRecord SourceRecord;
			FJournalRecord PendingRecord;
			if (!SourceTerminalJournal.TryGetRecordAt(Index, SourceRecord)
				|| !PendingJournal.TryGetRecordAt(Index, PendingRecord)
				|| !SourceRecord.Matches(PendingRecord))
			{
				return false;
			}
		}
		return true;
	}

	FGuid MakeRotationId(
		const FRequest& Request,
		const FJournal& SourceTerminalJournal,
		const FBundle& PendingBundle)
	{
		if (!Request.IsValid() || !SourceTerminalJournal.IsValid()
			|| !PendingBundle.IsValid()
			|| !IsExactCheckpointExtension(
				SourceTerminalJournal,
				PendingBundle.GetJournal(),
				Request.GetNewCheckpoint())
			|| SourceTerminalJournal.GetJournalId()
				!= Request.GetAdoption().GetTerminalJournalId()
			|| PendingBundle.GetGeneration()
				!= Request.GetAdoption().GetNextGeneration())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryNextGenerationRotation.r1"),
			{
				Request.GetRequestId().ToString(EGuidFormats::Digits),
				Request.GetAdoption().GetAdoptionId().ToString(
					EGuidFormats::Digits),
				SourceTerminalJournal.GetJournalId().ToString(
					EGuidFormats::Digits),
				Request.GetNewCheckpoint().GetCheckpointId().ToString(
					EGuidFormats::Digits),
				FString::FromInt(PendingBundle.GetGeneration()),
				PendingBundle.GetJournal().GetJournalId().ToString(
					EGuidFormats::Digits),
				PendingBundle.GetEnvelope().GetEnvelopeId().ToString(
					EGuidFormats::Digits),
				PendingBundle.GetBundleDigest().ToString(
					EGuidFormats::Digits),
				PendingBundle.GetBundleId().ToString(
					EGuidFormats::Digits)
			});
	}
}

bool FRequest::TryCreate(
	const FAdoption& InAdoption,
	const FCheckpoint& InNewCheckpoint,
	FRequest& OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = {};
	OutDiagnostic.Reset();
	if (!InAdoption.IsValid() || !InAdoption.HasNextGenerationCapacity())
	{
		OutDiagnostic = TEXT(
			"Next-generation rotation requires valid adopted-terminal evidence with remaining journal capacity.");
		return false;
	}
	if (!InNewCheckpoint.IsValid()
		|| InNewCheckpoint.GetCheckpointId()
			== InAdoption.GetRequest()
				.GetCompletionRequest()
				.GetAdmissionRequest()
				.GetExpectedCheckpointId())
	{
		OutDiagnostic = TEXT(
			"Next-generation rotation requires one real checkpoint with a new identity.");
		return false;
	}

	OutRequest.Adoption = InAdoption;
	OutRequest.NewCheckpoint = InNewCheckpoint;
	OutRequest.RequestId = MakeRequestId(InAdoption, InNewCheckpoint);
	if (!OutRequest.IsValid())
	{
		OutRequest = {};
		OutDiagnostic = TEXT(
			"Next-generation rotation request failed deterministic validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Next-generation rotation request bound one adopted terminal to one new checkpoint.");
	return true;
}

bool FRequest::IsValid() const
{
	return Adoption.IsValid() && Adoption.HasNextGenerationCapacity()
		&& NewCheckpoint.IsValid()
		&& NewCheckpoint.GetCheckpointId()
			!= Adoption.GetRequest()
				.GetCompletionRequest()
				.GetAdmissionRequest()
				.GetExpectedCheckpointId()
		&& RequestId.IsValid()
		&& RequestId == MakeRequestId(Adoption, NewCheckpoint);
}

bool FRotation::TryCreate(
	const FRequest& InRequest,
	const FJournal& InSourceTerminalJournal,
	const FBundle& InPendingBundle,
	FRotation& OutRotation)
{
	OutRotation = {};
	FRotation Candidate;
	Candidate.Request = InRequest;
	Candidate.SourceTerminalJournal = InSourceTerminalJournal;
	Candidate.PendingBundle = InPendingBundle;
	Candidate.RotationId = MakeRotationId(
		Candidate.Request,
		Candidate.SourceTerminalJournal,
		Candidate.PendingBundle);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutRotation = MoveTemp(Candidate);
	return true;
}

bool FRotation::IsValid() const
{
	return Request.IsValid() && SourceTerminalJournal.IsValid()
		&& PendingBundle.IsValid() && RotationId.IsValid()
		&& SourceTerminalJournal.GetJournalId()
			== Request.GetAdoption().GetTerminalJournalId()
		&& SourceTerminalJournal.GetRecordCount()
			== Request.GetAdoption().GetGeneration() * 2
		&& PendingBundle.GetGeneration()
			== Request.GetAdoption().GetNextGeneration()
		&& IsExactCheckpointExtension(
			SourceTerminalJournal,
			PendingBundle.GetJournal(),
			Request.GetNewCheckpoint())
		&& RotationId == MakeRotationId(
			Request, SourceTerminalJournal, PendingBundle);
}

bool FRotation::MatchesRequest(const FRequest& OtherRequest) const
{
	return IsValid() && OtherRequest.IsValid()
		&& Request.GetRequestId() == OtherRequest.GetRequestId();
}

namespace
{
	using FSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession;
	using FSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionResult;
	using ESessionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionStatus;
	using FCompletion =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion;
	using FCompletionStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext;
	using FCompletionStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter;
	using ECompletionLoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus;
	using FBundleStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext;
	using FPayloadEnvelope =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope;
	using IFileSystem =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem;
	using IAuthority =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority;
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus;
	using FCommitCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitCoordinator;
	using ECommitStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus;
}

bool FSessionResult::IsSuccess() const
{
	return IsValid()
		&& (Status == ESessionStatus::Rotated
			|| Status == ESessionStatus::Replayed);
}

bool FSessionResult::IsReplay() const
{
	return IsValid() && Status == ESessionStatus::Replayed;
}

bool FSessionResult::Validate() const
{
	if (Status == ESessionStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == ESessionStatus::RequestRejected)
	{
		return !bCompletionTrusted && !bAdoptionTrusted
			&& !bPendingAuthorityAccepted;
	}
	if (!Request.IsValid()
		|| SourceGeneration != Request.GetAdoption().GetGeneration()
		|| TargetGeneration != Request.GetAdoption().GetNextGeneration())
	{
		return false;
	}
	if (bCompletionTrusted
		&& (!Completion.IsValid()
			|| !Completion.MatchesRequest(
				Request.GetAdoption().GetRequest().GetCompletionRequest())
			|| Completion.GetGeneration() != SourceGeneration
			|| Completion.GetCompletionId()
				!= Request.GetAdoption().GetCompletionId()
			|| Completion.GetCompletionDigest()
				!= Request.GetAdoption().GetCompletionDigest()))
	{
		return false;
	}
	if (bAdoptionTrusted
		&& (!bCompletionTrusted || !VerifiedAdoption.IsValid()
			|| VerifiedAdoption.GetAdoptionId()
				!= Request.GetAdoption().GetAdoptionId()
			|| !VerifiedAdoption.MatchesRequest(
				Request.GetAdoption().GetRequest())
			|| !VerifiedAdoption.MatchesCompletion(Completion)))
	{
		return false;
	}
	if (Rotation.IsValid()
		&& (!bAdoptionTrusted || !Rotation.MatchesRequest(Request)
			|| Rotation.GetSourceGeneration() != SourceGeneration
			|| Rotation.GetTargetGeneration() != TargetGeneration))
	{
		return false;
	}
	if (bPendingAuthorityAccepted && !Rotation.IsValid())
	{
		return false;
	}
	if (Status == ESessionStatus::Rotated
		|| Status == ESessionStatus::Replayed)
	{
		return bPendingAuthorityAccepted && Rotation.IsValid()
			&& BundleCommitResult.IsSuccess()
			&& (Status != ESessionStatus::Replayed
				|| BundleCommitResult.GetStatus()
					== ECommitStatus::AlreadyCommitted);
	}
	if (Status == ESessionStatus::BundleCommittedWatermarkPending)
	{
		return bPendingAuthorityAccepted && Rotation.IsValid()
			&& !BundleCommitResult.IsSuccess()
			&& BundleCommitResult.GetStatus()
				== ECommitStatus::BundleCommittedWatermarkPending;
	}
	if (Status == ESessionStatus::PendingWatermarkOutcomeUnresolved)
	{
		return bPendingAuthorityAccepted && Rotation.IsValid()
			&& !BundleCommitResult.IsSuccess()
			&& BundleCommitResult.GetStatus()
				== ECommitStatus::WatermarkOutcomeUnresolved;
	}
	if (Status == ESessionStatus::BundleCommitRejected)
	{
		return bPendingAuthorityAccepted && Rotation.IsValid()
			&& !BundleCommitResult.IsSuccess()
			&& BundleCommitResult.GetStatus() != ECommitStatus::Invalid;
	}
	return true;
}

FSessionResult FSession::ExecuteExplicit(
	const FRequest& Request,
	const FCompletionStorageContext& CompletionStorageContext,
	const FBundleStorageContext& PendingBundleStorageContext,
	const FJournal& CurrentTerminalJournal,
	IFileSystem& FileSystem,
	const IAuthority& CompletionAuthority,
	const IAuthority& AdoptionAuthority,
	IAuthority& PendingAuthority)
{
	FSessionResult Result;
	Result.Request = Request;
	auto Finish = [&Result](
		const ESessionStatus Status,
		const TCHAR* Diagnostic) -> FSessionResult
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.bValidated = Result.Validate();
		return Result;
	};

	if (!Request.IsValid())
	{
		return Finish(
			ESessionStatus::RequestRejected,
			TEXT("Next-generation rotation rejected an invalid request before callbacks."));
	}
	Result.SourceGeneration = Request.GetAdoption().GetGeneration();
	Result.TargetGeneration = Request.GetAdoption().GetNextGeneration();
	const auto& AdoptionRequest = Request.GetAdoption().GetRequest();
	const auto& CompletionRequest = AdoptionRequest.GetCompletionRequest();
	const auto& AdmissionRequest =
		CompletionRequest.GetAdmissionRequest();
	const FGuid& LineageId = AdmissionRequest.GetLineageId();
	if (!CompletionStorageContext.IsValid()
		|| !PendingBundleStorageContext.IsValid()
		|| CompletionStorageContext.GetExpectedLineageId() != LineageId
		|| PendingBundleStorageContext.GetExpectedLineageId() != LineageId)
	{
		return Finish(
			ESessionStatus::RequestRejected,
			TEXT("Next-generation rotation rejected invalid or cross-lineage storage contexts before callbacks."));
	}
	if (!CurrentTerminalJournal.IsValid()
		|| CurrentTerminalJournal.GetLatestDisposition()
			!= EDisposition::RecoveryCommitted
		|| CurrentTerminalJournal.GetJournalId()
			!= Request.GetAdoption().GetTerminalJournalId()
		|| CurrentTerminalJournal.GetRecordCount()
			!= Result.SourceGeneration * 2)
	{
		return Finish(
			ESessionStatus::CurrentJournalRejected,
			TEXT("Next-generation rotation requires the exact adopted terminal journal."));
	}
	if (JournalContainsCheckpointId(
			CurrentTerminalJournal,
			Request.GetNewCheckpoint().GetCheckpointId()))
	{
		return Finish(
			ESessionStatus::CheckpointHistoryConflict,
			TEXT("Next-generation rotation refuses to reuse any historical checkpoint identity."));
	}
	if (bOperationInProgress)
	{
		return Finish(
			ESessionStatus::OperationInProgress,
			TEXT("Next-generation rotation rejected re-entry on the same Session."));
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const FGuid& CompletionDomainId =
		CompletionRequest.GetCompletionAuthorityDomainId();
	const FGuid& AdoptionDomainId =
		AdoptionRequest.GetAdoptionAuthorityDomainId();
	const FGuid& PendingDomainId = AdmissionRequest.GetAuthorityDomainId();

	const auto CompletionRead = CompletionAuthority.Read(
		CompletionDomainId, LineageId);
	Result.CompletionAuthorityReadStatus = CompletionRead.GetStatus();
	if (!CompletionRead.IsValid()
		|| CompletionRead.GetStatus() == EReadStatus::Rejected)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityReadRejected,
			TEXT("Completion authority returned a rejected or malformed read."));
	}
	if (CompletionRead.GetStatus() == EReadStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityUnavailable,
			TEXT("Completion authority is unavailable; rotation cannot trust ordinary completion storage."));
	}
	if (CompletionRead.GetStatus() == EReadStatus::Missing)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityMissing,
			TEXT("Completion authority has no current evidence for this rotation."));
	}
	const auto& CompletionState = CompletionRead.GetState();
	if (!CompletionState.IsValid()
		|| CompletionState.GetAuthorityDomainId() != CompletionDomainId
		|| CompletionState.GetLineageId() != LineageId)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityStateRejected,
			TEXT("Completion authority state does not match the requested domain and lineage."));
	}
	if (CompletionState.GetGeneration() > Result.SourceGeneration)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityAhead,
			TEXT("Completion authority is ahead of the adopted terminal generation."));
	}
	if (CompletionState.GetGeneration() < Result.SourceGeneration)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityNotCurrent,
			TEXT("Completion authority has not reached the adopted terminal generation."));
	}
	if (CompletionState.GetBundleId()
		!= Request.GetAdoption().GetCompletionId())
	{
		return Finish(
			ESessionStatus::CompletionAuthorityConflict,
			TEXT("Completion authority binds foreign evidence in the adopted generation."));
	}

	FCompletionStorageAdapter CompletionStorage;
	const auto Loaded = CompletionStorage.Load(
		CompletionStorageContext,
		Result.SourceGeneration,
		FileSystem);
	Result.CompletionLoadStatus = Loaded.GetStatus();
	if (!Loaded.IsSuccess())
	{
		return Finish(
			ESessionStatus::TrustedCompletionLoadRejected,
			TEXT("Trusted completion artifact could not be loaded at the exact source generation."));
	}
	Result.Completion = Loaded.GetCompletion();
	if (Loaded.GetGeneration() != Result.SourceGeneration
		|| Result.Completion.GetCompletionId()
			!= Request.GetAdoption().GetCompletionId()
		|| Result.Completion.GetCompletionDigest()
			!= Request.GetAdoption().GetCompletionDigest()
		|| !Result.Completion.MatchesRequest(CompletionRequest)
		|| !Result.Completion.MatchesCurrentJournal(
			CurrentTerminalJournal))
	{
		return Finish(
			ESessionStatus::TrustedCompletionMismatch,
			TEXT("Completion authority, artifact, request and terminal journal do not match."));
	}
	Result.bCompletionTrusted = true;
	if (!FAdoption::TryCreate(
			AdoptionRequest,
			Result.Completion,
			Result.VerifiedAdoption)
		|| Result.VerifiedAdoption.GetAdoptionId()
			!= Request.GetAdoption().GetAdoptionId()
		|| !Request.GetAdoption().MatchesCompletion(Result.Completion))
	{
		return Finish(
			ESessionStatus::AdoptionEvidenceMismatch,
			TEXT("Caller adoption evidence could not be reproduced from the trusted completion."));
	}

	const auto AdoptionRead = AdoptionAuthority.Read(
		AdoptionDomainId, LineageId);
	Result.AdoptionAuthorityReadStatus = AdoptionRead.GetStatus();
	if (!AdoptionRead.IsValid()
		|| AdoptionRead.GetStatus() == EReadStatus::Rejected)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityReadRejected,
			TEXT("Adoption authority returned a rejected or malformed read."));
	}
	if (AdoptionRead.GetStatus() == EReadStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityUnavailable,
			TEXT("Adoption authority is unavailable; rotation requires prior adoption."));
	}
	if (AdoptionRead.GetStatus() == EReadStatus::Missing)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityMissing,
			TEXT("Adoption authority has not adopted this terminal generation."));
	}
	const auto& AdoptionState = AdoptionRead.GetState();
	if (!AdoptionState.IsValid()
		|| AdoptionState.GetAuthorityDomainId() != AdoptionDomainId
		|| AdoptionState.GetLineageId() != LineageId)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityStateRejected,
			TEXT("Adoption authority state does not match the requested domain and lineage."));
	}
	if (AdoptionState.GetGeneration() > Result.SourceGeneration)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityAhead,
			TEXT("Adoption authority is ahead of this rotation source."));
	}
	if (AdoptionState.GetGeneration() < Result.SourceGeneration)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityNotCurrent,
			TEXT("Adoption authority has not reached this rotation source."));
	}
	if (AdoptionState.GetBundleId()
		!= Result.VerifiedAdoption.GetAdoptionId())
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityConflict,
			TEXT("Adoption authority binds foreign evidence in the source generation."));
	}
	Result.bAdoptionTrusted = true;

	FJournal PendingJournal = CurrentTerminalJournal;
	Result.JournalAppendResult = PendingJournal.AppendCheckpoint(
		Request.GetNewCheckpoint());
	if (!Result.JournalAppendResult.DidAppend())
	{
		return Finish(
			ESessionStatus::JournalAppendRejected,
			TEXT("The new checkpoint could not extend the adopted terminal journal copy."));
	}
	FPayloadEnvelope Envelope;
	if (!FPayloadEnvelope::TryWrap(
			Request.GetNewCheckpoint(), PendingJournal, Envelope))
	{
		return Finish(
			ESessionStatus::EnvelopeCreationRejected,
			TEXT("The new checkpoint could not be wrapped for the rotated pending journal."));
	}
	FBundle PendingBundle;
	if (!FBundle::TryCreate(PendingJournal, Envelope, PendingBundle)
		|| PendingBundle.GetGeneration() != Result.TargetGeneration)
	{
		return Finish(
			ESessionStatus::BundleCreationRejected,
			TEXT("The rotated pending bundle could not be created at exactly G+1."));
	}
	if (!FRotation::TryCreate(
			Request,
			CurrentTerminalJournal,
			PendingBundle,
			Result.Rotation))
	{
		return Finish(
			ESessionStatus::RotationCreationRejected,
			TEXT("Deterministic next-generation rotation evidence could not be created."));
	}

	const auto PendingRead = PendingAuthority.Read(
		PendingDomainId, LineageId);
	Result.PendingAuthorityReadStatus = PendingRead.GetStatus();
	if (!PendingRead.IsValid()
		|| PendingRead.GetStatus() == EReadStatus::Rejected)
	{
		return Finish(
			ESessionStatus::PendingAuthorityReadRejected,
			TEXT("Pending authority returned a rejected or malformed preflight read."));
	}
	if (PendingRead.GetStatus() == EReadStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::PendingAuthorityUnavailable,
			TEXT("Pending authority is unavailable before bundle rotation."));
	}
	if (PendingRead.GetStatus() == EReadStatus::Missing)
	{
		return Finish(
			ESessionStatus::PendingAuthorityMissing,
			TEXT("Pending authority lost the exact source-generation watermark."));
	}
	const auto& PendingState = PendingRead.GetState();
	if (!PendingState.IsValid()
		|| PendingState.GetAuthorityDomainId() != PendingDomainId
		|| PendingState.GetLineageId() != LineageId)
	{
		return Finish(
			ESessionStatus::PendingAuthorityStateRejected,
			TEXT("Pending authority state does not match the requested domain and lineage."));
	}
	if (PendingState.GetGeneration() > Result.TargetGeneration)
	{
		return Finish(
			ESessionStatus::PendingAuthorityAhead,
			TEXT("Pending authority is ahead of the requested next generation."));
	}
	if (PendingState.GetGeneration() < Result.SourceGeneration)
	{
		return Finish(
			ESessionStatus::PendingAuthorityNotCurrent,
			TEXT("Pending authority is behind the trusted completion source generation."));
	}
	if (PendingState.GetGeneration() == Result.SourceGeneration
		&& PendingState.GetBundleId()
			!= Result.Completion.GetSourceBundleId())
	{
		return Finish(
			ESessionStatus::PendingAuthorityConflict,
			TEXT("Pending authority no longer binds the exact bundle consumed by completion."));
	}
	if (PendingState.GetGeneration() == Result.TargetGeneration
		&& PendingState.GetBundleId()
			!= Result.Rotation.GetPendingBundle().GetBundleId())
	{
		return Finish(
			ESessionStatus::PendingAuthorityConflict,
			TEXT("Pending authority already binds foreign evidence in the target generation."));
	}
	if (PendingState.GetGeneration() != Result.SourceGeneration
		&& PendingState.GetGeneration() != Result.TargetGeneration)
	{
		return Finish(
			ESessionStatus::PendingAuthorityNotCurrent,
			TEXT("Pending authority is not at the source or exact replay generation."));
	}
	Result.bPendingAuthorityAccepted = true;

	FCommitCoordinator Coordinator;
	Result.BundleCommitResult = Coordinator.CommitBundleThenAdvance(
		PendingBundleStorageContext,
		Result.Rotation.GetPendingBundle(),
		PendingDomainId,
		FileSystem,
		PendingAuthority);
	if (Result.BundleCommitResult.IsSuccess())
	{
		return Finish(
			Result.BundleCommitResult.GetStatus()
				== ECommitStatus::AlreadyCommitted
				? ESessionStatus::Replayed
				: ESessionStatus::Rotated,
			TEXT("Adopted terminal journal rotated to one exact next-generation pending bundle."));
	}
	if (Result.BundleCommitResult.GetStatus()
		== ECommitStatus::BundleCommittedWatermarkPending)
	{
		return Finish(
			ESessionStatus::BundleCommittedWatermarkPending,
			TEXT("Next-generation bundle is durable but its pending watermark is not current."));
	}
	if (Result.BundleCommitResult.GetStatus()
		== ECommitStatus::WatermarkOutcomeUnresolved)
	{
		return Finish(
			ESessionStatus::PendingWatermarkOutcomeUnresolved,
			TEXT("One exact reread could not resolve the pending watermark advance."));
	}
	return Finish(
		ESessionStatus::BundleCommitRejected,
		TEXT("Existing pending bundle commit protocol rejected the rotation."));
}
