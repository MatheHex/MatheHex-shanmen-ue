#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest;
	using FAdoption =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption;
	using FCompletionRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest;
	using FCompletion =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using EJournalDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using FBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;

	FGuid MakeRequestId(
		const FGuid& AdoptionAuthorityDomainId,
		const FCompletionRequest& CompletionRequest)
	{
		if (!AdoptionAuthorityDomainId.IsValid()
			|| !CompletionRequest.IsValid())
		{
			return FGuid();
		}
		const auto& Admission = CompletionRequest.GetAdmissionRequest();
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest.r1"),
			{
				AdoptionAuthorityDomainId.ToString(EGuidFormats::Digits),
				CompletionRequest.GetCompletionAuthorityDomainId().ToString(
					EGuidFormats::Digits),
				CompletionRequest.GetRequestId().ToString(EGuidFormats::Digits),
				Admission.GetAuthorityDomainId().ToString(EGuidFormats::Digits),
				Admission.GetLineageId().ToString(EGuidFormats::Digits),
				Admission.GetExpectedJournalId().ToString(EGuidFormats::Digits),
				Admission.GetExpectedCheckpointId().ToString(
					EGuidFormats::Digits)
			});
	}

	int32 CurrentJournalGeneration(const FJournal& Journal)
	{
		if (!Journal.IsValid() || Journal.GetRecordCount() <= 0)
		{
			return 0;
		}
		const int32 RecordCount = Journal.GetRecordCount();
		const auto Disposition = Journal.GetLatestDisposition();
		const int32 Generation =
			Disposition == EJournalDisposition::CheckpointPending
				&& RecordCount % 2 == 1
			? (RecordCount + 1) / 2
			: Disposition == EJournalDisposition::RecoveryCommitted
				&& RecordCount % 2 == 0
				? RecordCount / 2
				: 0;
		return Generation >= 1
			&& Generation <= FBundle::MaximumGeneration()
			? Generation
			: 0;
	}

	int32 NextGenerationFor(const FCompletion& Completion)
	{
		return Completion.IsValid()
			&& Completion.GetGeneration() < FBundle::MaximumGeneration()
			? Completion.GetGeneration() + 1
			: 0;
	}

	FGuid MakeAdoptionId(
		const FRequest& Request,
		const FCompletion& Completion,
		const int32 NextGeneration)
	{
		if (!Request.IsValid() || !Completion.IsValid()
			|| NextGeneration != NextGenerationFor(Completion))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryTerminalAdoption.r1"),
			{
				Request.GetRequestId().ToString(EGuidFormats::Digits),
				FString::FromInt(Completion.GetGeneration()),
				FString::FromInt(NextGeneration),
				Completion.GetCompletionId().ToString(EGuidFormats::Digits),
				Completion.GetCompletionDigest().ToString(
					EGuidFormats::Digits),
				Completion.GetSourceJournal().GetJournalId().ToString(
					EGuidFormats::Digits),
				Completion.GetTerminalJournal().GetJournalId().ToString(
					EGuidFormats::Digits)
			});
	}
}

bool FRequest::TryCreate(
	const FGuid& InAdoptionAuthorityDomainId,
	const FCompletionRequest& InCompletionRequest,
	FRequest& OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = {};
	OutDiagnostic.Reset();
	if (!InAdoptionAuthorityDomainId.IsValid()
		|| !InCompletionRequest.IsValid()
		|| InAdoptionAuthorityDomainId
			== InCompletionRequest.GetCompletionAuthorityDomainId()
		|| InAdoptionAuthorityDomainId
			== InCompletionRequest.GetAdmissionRequest().GetAuthorityDomainId())
	{
		OutDiagnostic = TEXT(
			"Terminal adoption requires a valid authority domain distinct from pending and completion authority.");
		return false;
	}
	OutRequest.AdoptionAuthorityDomainId = InAdoptionAuthorityDomainId;
	OutRequest.CompletionRequest = InCompletionRequest;
	OutRequest.RequestId = MakeRequestId(
		InAdoptionAuthorityDomainId, InCompletionRequest);
	if (!OutRequest.IsValid())
	{
		OutRequest = {};
		OutDiagnostic = TEXT(
			"Terminal adoption request failed deterministic validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Terminal adoption request created for one exact trusted completion.");
	return true;
}

bool FRequest::IsValid() const
{
	return AdoptionAuthorityDomainId.IsValid()
		&& CompletionRequest.IsValid()
		&& AdoptionAuthorityDomainId
			!= CompletionRequest.GetCompletionAuthorityDomainId()
		&& AdoptionAuthorityDomainId
			!= CompletionRequest.GetAdmissionRequest().GetAuthorityDomainId()
		&& RequestId.IsValid()
		&& RequestId == MakeRequestId(
			AdoptionAuthorityDomainId, CompletionRequest);
}

bool FAdoption::TryCreate(
	const FRequest& InRequest,
	const FCompletion& InCompletion,
	FAdoption& OutAdoption)
{
	OutAdoption = {};
	if (!InRequest.IsValid() || !InCompletion.IsValid()
		|| !InCompletion.MatchesRequest(
			InRequest.GetCompletionRequest()))
	{
		return false;
	}

	FAdoption Candidate;
	Candidate.Generation = InCompletion.GetGeneration();
	Candidate.NextGeneration = NextGenerationFor(InCompletion);
	Candidate.CompletionId = InCompletion.GetCompletionId();
	Candidate.CompletionDigest = InCompletion.GetCompletionDigest();
	Candidate.SourceJournalId =
		InCompletion.GetSourceJournal().GetJournalId();
	Candidate.TerminalJournalId =
		InCompletion.GetTerminalJournal().GetJournalId();
	Candidate.Request = InRequest;
	Candidate.AdoptionId = MakeAdoptionId(
		Candidate.Request, InCompletion, Candidate.NextGeneration);
	if (!Candidate.IsValid()
		|| !Candidate.MatchesCompletion(InCompletion))
	{
		return false;
	}
	OutAdoption = MoveTemp(Candidate);
	return true;
}

bool FAdoption::IsValid() const
{
	const int32 ExpectedNext =
		Generation >= 1 && Generation < FBundle::MaximumGeneration()
			? Generation + 1
			: 0;
	return Generation >= 1
		&& Generation <= FBundle::MaximumGeneration()
		&& NextGeneration == ExpectedNext
		&& AdoptionId.IsValid() && CompletionId.IsValid()
		&& CompletionDigest.IsValid() && SourceJournalId.IsValid()
		&& TerminalJournalId.IsValid()
		&& SourceJournalId != TerminalJournalId
		&& Request.IsValid();
}

bool FAdoption::MatchesRequest(const FRequest& OtherRequest) const
{
	return IsValid() && OtherRequest.IsValid()
		&& Request.GetRequestId() == OtherRequest.GetRequestId();
}

bool FAdoption::MatchesCompletion(const FCompletion& OtherCompletion) const
{
	return IsValid() && OtherCompletion.IsValid()
		&& OtherCompletion.MatchesRequest(Request.GetCompletionRequest())
		&& Generation == OtherCompletion.GetGeneration()
		&& NextGeneration == NextGenerationFor(OtherCompletion)
		&& CompletionId == OtherCompletion.GetCompletionId()
		&& CompletionDigest == OtherCompletion.GetCompletionDigest()
		&& SourceJournalId
			== OtherCompletion.GetSourceJournal().GetJournalId()
		&& TerminalJournalId
			== OtherCompletion.GetTerminalJournal().GetJournalId()
		&& AdoptionId == MakeAdoptionId(
			Request, OtherCompletion, NextGeneration);
}

namespace
{
	using FSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession;
	using FSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionResult;
	using ESessionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionStatus;
	using FStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext;
	using FStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter;
	using IFileSystem =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageFileSystem;
	using IAuthority =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority;
	using EReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus;
	using EAdvanceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus;
	using FAdvanceRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest;
	using ELoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus;
}

bool FSessionResult::IsSuccess() const
{
	return IsValid()
		&& (Status == ESessionStatus::Adopted
			|| Status == ESessionStatus::AdoptedAfterAuthorityRecheck
			|| Status == ESessionStatus::Replayed);
}

bool FSessionResult::IsReplay() const
{
	return IsValid() && Status == ESessionStatus::Replayed;
}

bool FSessionResult::HasNextGenerationCapacity() const
{
	return IsSuccess() && Adoption.HasNextGenerationCapacity();
}

bool FSessionResult::Validate() const
{
	if (Status == ESessionStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == ESessionStatus::RequestRejected)
	{
		return !Request.IsValid() || TargetGeneration == 0;
	}
	if (!Request.IsValid())
	{
		return false;
	}
	if (Status == ESessionStatus::OperationInProgress
		|| Status == ESessionStatus::CurrentJournalRejected)
	{
		return !bCompletionTrusted && !bAuthorityCurrent;
	}
	if (TargetGeneration < 1
		|| TargetGeneration > FBundle::MaximumGeneration())
	{
		return false;
	}
	const bool bHasTrustedEvidence = bCompletionTrusted
		&& Completion.IsValid()
		&& Completion.MatchesRequest(Request.GetCompletionRequest())
		&& Completion.GetGeneration() == TargetGeneration
		&& Adoption.IsValid() && Adoption.MatchesRequest(Request)
		&& Adoption.MatchesCompletion(Completion)
		&& TerminalJournal.IsValid()
		&& TerminalJournal.GetJournalId()
			== Adoption.GetTerminalJournalId();
	if (Status == ESessionStatus::Adopted
		|| Status == ESessionStatus::AdoptedAfterAuthorityRecheck
		|| Status == ESessionStatus::Replayed)
	{
		return bHasTrustedEvidence && bAuthorityCurrent
			&& CompletionAuthorityReadStatus == EReadStatus::Current
			&& CompletionLoadStatus == ELoadStatus::Loaded
			&& AdoptionAuthorityReadStatus != EReadStatus::Invalid;
	}
	if (bAuthorityCurrent)
	{
		return false;
	}
	if (Status == ESessionStatus::AdoptionCreationRejected
		|| Status == ESessionStatus::AdoptionAuthorityReadRejected
		|| Status == ESessionStatus::AdoptionAuthorityUnavailable
		|| Status == ESessionStatus::AdoptionAuthorityStateRejected
		|| Status == ESessionStatus::AdoptionAuthorityAhead
		|| Status == ESessionStatus::AdoptionAuthorityConflict
		|| Status == ESessionStatus::AdoptionAdvanceRejected
		|| Status == ESessionStatus::AdoptionAdvanceConflict
		|| Status == ESessionStatus::AdoptionOutcomeUnresolved)
	{
		return bCompletionTrusted && Completion.IsValid();
	}
	return !bCompletionTrusted;
}

FSessionResult FSession::ExecuteExplicit(
	const FRequest& Request,
	const FStorageContext& CompletionStorageContext,
	const FJournal& CurrentJournal,
	const IFileSystem& FileSystem,
	const IAuthority& CompletionAuthority,
	IAuthority& AdoptionAuthority)
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

	if (!Request.IsValid() || !CompletionStorageContext.IsValid()
		|| CompletionStorageContext.GetExpectedLineageId()
			!= Request.GetCompletionRequest().GetAdmissionRequest().GetLineageId())
	{
		return Finish(
			ESessionStatus::RequestRejected,
			TEXT("Terminal adoption rejected invalid or cross-lineage input before authority access."));
	}
	Result.TargetGeneration = CurrentJournalGeneration(CurrentJournal);
	if (Result.TargetGeneration <= 0)
	{
		return Finish(
			ESessionStatus::CurrentJournalRejected,
			TEXT("Terminal adoption requires one valid caller-selected pending or terminal journal."));
	}
	if (bOperationInProgress)
	{
		return Finish(
			ESessionStatus::OperationInProgress,
			TEXT("Terminal adoption rejected re-entry on the same Session."));
	}

	TGuardValue<bool> OperationGuard(bOperationInProgress, true);
	const auto& CompletionRequest = Request.GetCompletionRequest();
	const auto& AdmissionRequest = CompletionRequest.GetAdmissionRequest();
	const FGuid& CompletionDomainId =
		CompletionRequest.GetCompletionAuthorityDomainId();
	const FGuid& AdoptionDomainId = Request.GetAdoptionAuthorityDomainId();
	const FGuid& LineageId = AdmissionRequest.GetLineageId();

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
			TEXT("Completion authority is unavailable; ordinary completion storage is not trusted alone."));
	}
	if (CompletionRead.GetStatus() == EReadStatus::Missing)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityMissing,
			TEXT("Completion authority has no trusted watermark for this lineage."));
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
	if (CompletionState.GetGeneration() > Result.TargetGeneration)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityAhead,
			TEXT("Trusted completion authority is ahead of the caller journal generation."));
	}
	if (CompletionState.GetGeneration() < Result.TargetGeneration)
	{
		return Finish(
			ESessionStatus::CompletionAuthorityNotCurrent,
			TEXT("Trusted completion authority has not reached the caller journal generation."));
	}

	FStorageAdapter Storage;
	const auto Loaded = Storage.Load(
		CompletionStorageContext,
		Result.TargetGeneration,
		FileSystem);
	Result.CompletionLoadStatus = Loaded.GetStatus();
	if (!Loaded.IsSuccess())
	{
		return Finish(
			ESessionStatus::TrustedCompletionLoadRejected,
			TEXT("Trusted completion artifact could not be loaded at the exact authority generation."));
	}
	Result.Completion = Loaded.GetCompletion();
	if (Loaded.GetGeneration() != Result.TargetGeneration
		|| CompletionState.GetBundleId()
			!= Result.Completion.GetCompletionId()
		|| !Result.Completion.MatchesRequest(CompletionRequest)
		|| !Result.Completion.MatchesCurrentJournal(CurrentJournal))
	{
		return Finish(
			ESessionStatus::TrustedCompletionMismatch,
			TEXT("Completion authority, artifact, request or caller journal identities do not match."));
	}
	Result.bCompletionTrusted = true;
	Result.TerminalJournal = Result.Completion.GetTerminalJournal();
	if (!FAdoption::TryCreate(
			Request, Result.Completion, Result.Adoption))
	{
		return Finish(
			ESessionStatus::AdoptionCreationRejected,
			TEXT("Deterministic terminal adoption evidence could not be created."));
	}

	const auto AdoptionRead = AdoptionAuthority.Read(
		AdoptionDomainId, LineageId);
	Result.AdoptionAuthorityReadStatus = AdoptionRead.GetStatus();
	if (!AdoptionRead.IsValid()
		|| AdoptionRead.GetStatus() == EReadStatus::Rejected)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityReadRejected,
			TEXT("Caller adoption authority returned a rejected or malformed read."));
	}
	if (AdoptionRead.GetStatus() == EReadStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::AdoptionAuthorityUnavailable,
			TEXT("Caller adoption authority is unavailable."));
	}

	Result.PreviousAdoptionGeneration = 0;
	if (AdoptionRead.GetStatus() == EReadStatus::Current)
	{
		const auto& AdoptionState = AdoptionRead.GetState();
		if (!AdoptionState.IsValid()
			|| AdoptionState.GetAuthorityDomainId() != AdoptionDomainId
			|| AdoptionState.GetLineageId() != LineageId)
		{
			return Finish(
				ESessionStatus::AdoptionAuthorityStateRejected,
				TEXT("Caller adoption authority state does not match the requested domain and lineage."));
		}
		Result.PreviousAdoptionGeneration = AdoptionState.GetGeneration();
		if (AdoptionState.GetGeneration() > Result.TargetGeneration)
		{
			return Finish(
				ESessionStatus::AdoptionAuthorityAhead,
				TEXT("Caller adoption authority is ahead of this terminal completion."));
		}
		if (AdoptionState.GetGeneration() == Result.TargetGeneration)
		{
			if (AdoptionState.GetBundleId()
					!= Result.Adoption.GetAdoptionId())
			{
				return Finish(
					ESessionStatus::AdoptionAuthorityConflict,
					TEXT("Caller adoption authority already binds this generation to foreign evidence."));
			}
			Result.bAuthorityCurrent = true;
			return Finish(
				ESessionStatus::Replayed,
				TEXT("Exact trusted terminal adoption replayed without authority mutation."));
		}
	}

	FAdvanceRequest AdvanceRequest;
	FString AdvanceDiagnostic;
	if (!FAdvanceRequest::TryCreate(
			AdoptionDomainId,
			LineageId,
			Result.PreviousAdoptionGeneration,
			Result.TargetGeneration,
			Result.Adoption.GetAdoptionId(),
			AdvanceRequest,
			AdvanceDiagnostic))
	{
		return Finish(
			ESessionStatus::AdoptionAdvanceRejected,
			TEXT("Caller adoption authority request could not be created."));
	}
	const auto Advanced = AdoptionAuthority.CompareAndAdvance(AdvanceRequest);
	Result.AdoptionAuthorityAdvanceStatus = Advanced.GetStatus();
	if (!Advanced.IsValid())
	{
		return Finish(
			ESessionStatus::AdoptionAdvanceRejected,
			TEXT("Caller adoption authority returned a malformed advance result."));
	}
	if (Advanced.IsSuccess())
	{
		if (!Advanced.GetReceipt().Matches(AdvanceRequest)
			|| !Advanced.GetState().MatchesRequest(AdvanceRequest))
		{
			return Finish(
				ESessionStatus::AdoptionAdvanceRejected,
				TEXT("Caller adoption authority success did not match the exact request."));
		}
		Result.bAuthorityCurrent = true;
		return Finish(
			Advanced.GetStatus() == EAdvanceStatus::AlreadyCurrent
				? ESessionStatus::Replayed
				: ESessionStatus::Adopted,
			TEXT("Trusted terminal journal is authorized for caller adoption."));
	}
	if (Advanced.GetStatus() == EAdvanceStatus::Conflict)
	{
		return Finish(
			ESessionStatus::AdoptionAdvanceConflict,
			TEXT("Caller adoption compare-and-advance conflicted."));
	}
	if (Advanced.GetStatus() == EAdvanceStatus::Rejected
		|| Advanced.GetStatus() == EAdvanceStatus::Unavailable)
	{
		return Finish(
			ESessionStatus::AdoptionAdvanceRejected,
			TEXT("Caller adoption authority did not accept the advance."));
	}

	const auto Recheck = AdoptionAuthority.Read(
		AdoptionDomainId, LineageId);
	Result.AdoptionAuthorityRecheckStatus = Recheck.GetStatus();
	if (Recheck.IsValid()
		&& Recheck.GetStatus() == EReadStatus::Current
		&& Recheck.GetState().MatchesRequest(AdvanceRequest))
	{
		Result.bAuthorityCurrent = true;
		return Finish(
			ESessionStatus::AdoptedAfterAuthorityRecheck,
			TEXT("Unknown adoption outcome resolved by one exact authority re-read."));
	}
	return Finish(
		ESessionStatus::AdoptionOutcomeUnresolved,
		TEXT("One exact authority re-read could not resolve the terminal adoption outcome."));
}
