#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FProof =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof;
	using FRotation =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation;
	using FCompletion =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion;
	using FAdoption =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using FJournalRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;
	using EDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using ERecordKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;
	using FBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;

	bool JournalsMatch(const FJournal& Left, const FJournal& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.GetJournalId() != Right.GetJournalId()
			|| Left.GetRecordCount() != Right.GetRecordCount())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.GetRecordCount(); ++Index)
		{
			FJournalRecord LeftRecord;
			FJournalRecord RightRecord;
			if (!Left.TryGetRecordAt(Index, LeftRecord)
				|| !Right.TryGetRecordAt(Index, RightRecord)
				|| !LeftRecord.Matches(RightRecord))
			{
				return false;
			}
		}
		return true;
	}

	bool IsExactRecoveryExtension(
		const FJournal& PendingJournal,
		const FJournal& TerminalJournal,
		const FCompletion& Completion)
	{
		if (!PendingJournal.IsValid()
			|| PendingJournal.GetLatestDisposition()
				!= EDisposition::CheckpointPending
			|| !TerminalJournal.IsValid()
			|| TerminalJournal.GetLatestDisposition()
				!= EDisposition::RecoveryCommitted
			|| TerminalJournal.GetRecordCount()
				!= PendingJournal.GetRecordCount() + 1
			|| !Completion.IsValid())
		{
			return false;
		}
		for (int32 Index = 0; Index < PendingJournal.GetRecordCount(); ++Index)
		{
			FJournalRecord PendingRecord;
			FJournalRecord TerminalRecord;
			if (!PendingJournal.TryGetRecordAt(Index, PendingRecord)
				|| !TerminalJournal.TryGetRecordAt(Index, TerminalRecord)
				|| !PendingRecord.Matches(TerminalRecord))
			{
				return false;
			}
		}
		FJournalRecord Latest;
		return TerminalJournal.TryGetLatestRecord(Latest)
			&& Latest.IsValid()
			&& Latest.GetKind() == ERecordKind::RecoveryCommitted
			&& Latest.GetCheckpointId()
				== Completion.GetRequest()
					.GetAdmissionRequest()
					.GetExpectedCheckpointId()
			&& Latest.GetRecoveryReceiptId()
				== Completion.GetRecoveryReceiptId();
	}

	bool HasOneCanonicalAuthorityChain(
		const FRotation& FirstRotation,
		const FCompletion& IntermediateCompletion,
		const FAdoption& IntermediateAdoption,
		const FRotation& SecondRotation)
	{
		const auto& FirstAdoptionRequest =
			FirstRotation.GetRequest().GetAdoption().GetRequest();
		const auto& FirstCompletionRequest =
			FirstAdoptionRequest.GetCompletionRequest();
		const auto& FirstAdmissionRequest =
			FirstCompletionRequest.GetAdmissionRequest();
		const auto& IntermediateCompletionRequest =
			IntermediateCompletion.GetRequest();
		const auto& IntermediateAdmissionRequest =
			IntermediateCompletionRequest.GetAdmissionRequest();
		const auto& IntermediateAdoptionRequest =
			IntermediateAdoption.GetRequest();
		const auto& SecondAdoption =
			SecondRotation.GetRequest().GetAdoption();
		const auto& SecondAdoptionRequest = SecondAdoption.GetRequest();
		const auto& SecondCompletionRequest =
			SecondAdoptionRequest.GetCompletionRequest();
		const auto& SecondAdmissionRequest =
			SecondCompletionRequest.GetAdmissionRequest();

		return FirstAdmissionRequest.GetLineageId()
				== IntermediateAdmissionRequest.GetLineageId()
			&& IntermediateAdmissionRequest.GetLineageId()
				== SecondAdmissionRequest.GetLineageId()
			&& FirstAdmissionRequest.GetAuthorityDomainId()
				== IntermediateAdmissionRequest.GetAuthorityDomainId()
			&& IntermediateAdmissionRequest.GetAuthorityDomainId()
				== SecondAdmissionRequest.GetAuthorityDomainId()
			&& FirstCompletionRequest.GetCompletionAuthorityDomainId()
				== IntermediateCompletionRequest.GetCompletionAuthorityDomainId()
			&& IntermediateCompletionRequest.GetCompletionAuthorityDomainId()
				== SecondCompletionRequest.GetCompletionAuthorityDomainId()
			&& FirstAdoptionRequest.GetAdoptionAuthorityDomainId()
				== IntermediateAdoptionRequest.GetAdoptionAuthorityDomainId()
			&& IntermediateAdoptionRequest.GetAdoptionAuthorityDomainId()
				== SecondAdoptionRequest.GetAdoptionAuthorityDomainId();
	}

	bool IsExactAdjacentCycle(
		const FRotation& FirstRotation,
		const FCompletion& IntermediateCompletion,
		const FAdoption& IntermediateAdoption,
		const FRotation& SecondRotation)
	{
		if (!FirstRotation.IsValid() || !IntermediateCompletion.IsValid()
			|| !IntermediateAdoption.IsValid() || !SecondRotation.IsValid())
		{
			return false;
		}

		const int32 InitialGeneration = FirstRotation.GetSourceGeneration();
		const int32 IntermediateGeneration =
			IntermediateCompletion.GetGeneration();
		const int32 FinalGeneration = SecondRotation.GetTargetGeneration();
		const FJournal& FirstPendingJournal =
			FirstRotation.GetPendingJournal();
		const FJournal& IntermediateTerminalJournal =
			IntermediateCompletion.GetTerminalJournal();
		const FAdoption& SecondAdoption =
			SecondRotation.GetRequest().GetAdoption();

		return InitialGeneration >= 1
			&& FirstRotation.GetTargetGeneration()
				== InitialGeneration + 1
			&& IntermediateGeneration
				== FirstRotation.GetTargetGeneration()
			&& IntermediateAdoption.GetGeneration()
				== IntermediateGeneration
			&& IntermediateAdoption.GetNextGeneration()
				== IntermediateGeneration + 1
			&& SecondRotation.GetSourceGeneration()
				== IntermediateGeneration
			&& FinalGeneration == IntermediateGeneration + 1
			&& FinalGeneration <= FBundle::MaximumGeneration()
			&& IntermediateCompletion.GetSourceBundleId()
				== FirstRotation.GetPendingBundle().GetBundleId()
			&& JournalsMatch(
				IntermediateCompletion.GetSourceJournal(),
				FirstPendingJournal)
			&& IsExactRecoveryExtension(
				FirstPendingJournal,
				IntermediateTerminalJournal,
				IntermediateCompletion)
			&& IntermediateAdoption.MatchesCompletion(
				IntermediateCompletion)
			&& IntermediateAdoption.GetSourceJournalId()
				== FirstPendingJournal.GetJournalId()
			&& IntermediateAdoption.GetTerminalJournalId()
				== IntermediateTerminalJournal.GetJournalId()
			&& SecondAdoption.GetAdoptionId()
				== IntermediateAdoption.GetAdoptionId()
			&& SecondAdoption.MatchesCompletion(IntermediateCompletion)
			&& JournalsMatch(
				SecondRotation.GetSourceTerminalJournal(),
				IntermediateTerminalJournal)
			&& FirstRotation.GetRequest()
					.GetNewCheckpoint()
					.GetCheckpointId()
				!= SecondRotation.GetRequest()
					.GetNewCheckpoint()
					.GetCheckpointId()
			&& HasOneCanonicalAuthorityChain(
				FirstRotation,
				IntermediateCompletion,
				IntermediateAdoption,
				SecondRotation);
	}

	FGuid MakeProofId(
		const FRotation& FirstRotation,
		const FCompletion& IntermediateCompletion,
		const FAdoption& IntermediateAdoption,
		const FRotation& SecondRotation)
	{
		if (!IsExactAdjacentCycle(
				FirstRotation,
				IntermediateCompletion,
				IntermediateAdoption,
				SecondRotation))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.r1"),
			{
				FirstRotation.GetRotationId().ToString(EGuidFormats::Digits),
				FString::FromInt(FirstRotation.GetSourceGeneration()),
				FString::FromInt(FirstRotation.GetTargetGeneration()),
				FirstRotation.GetPendingBundle().GetBundleId().ToString(
					EGuidFormats::Digits),
				IntermediateCompletion.GetCompletionId().ToString(
					EGuidFormats::Digits),
				IntermediateCompletion.GetCompletionDigest().ToString(
					EGuidFormats::Digits),
				IntermediateCompletion.GetRecoveryReceiptId().ToString(
					EGuidFormats::Digits),
				IntermediateAdoption.GetAdoptionId().ToString(
					EGuidFormats::Digits),
				SecondRotation.GetRequest().GetRequestId().ToString(
					EGuidFormats::Digits),
				SecondRotation.GetRotationId().ToString(
					EGuidFormats::Digits),
				FString::FromInt(SecondRotation.GetTargetGeneration()),
				SecondRotation.GetPendingBundle().GetBundleId().ToString(
					EGuidFormats::Digits)
			});
	}
}

bool FProof::TryCreate(
	const FRotation& InFirstRotation,
	const FCompletion& InIntermediateCompletion,
	const FAdoption& InIntermediateAdoption,
	const FRotation& InSecondRotation,
	FProof& OutProof,
	FString& OutDiagnostic)
{
	OutProof = {};
	OutDiagnostic.Reset();
	if (!IsExactAdjacentCycle(
			InFirstRotation,
			InIntermediateCompletion,
			InIntermediateAdoption,
			InSecondRotation))
	{
		OutDiagnostic = TEXT(
			"Multi-generation cycle proof requires one exact adjacent G to G+2 authority chain.");
		return false;
	}

	OutProof.FirstRotation = InFirstRotation;
	OutProof.IntermediateCompletion = InIntermediateCompletion;
	OutProof.IntermediateAdoption = InIntermediateAdoption;
	OutProof.SecondRotation = InSecondRotation;
	OutProof.ProofId = MakeProofId(
		InFirstRotation,
		InIntermediateCompletion,
		InIntermediateAdoption,
		InSecondRotation);
	if (!OutProof.IsValid())
	{
		OutProof = {};
		OutDiagnostic = TEXT(
			"Multi-generation cycle proof failed deterministic validation.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Multi-generation cycle proof bound two adjacent rotations through one exact completion and adoption.");
	return true;
}

bool FProof::IsValid() const
{
	return ProofId.IsValid()
		&& IsExactAdjacentCycle(
			FirstRotation,
			IntermediateCompletion,
			IntermediateAdoption,
			SecondRotation)
		&& ProofId == MakeProofId(
			FirstRotation,
			IntermediateCompletion,
			IntermediateAdoption,
			SecondRotation);
}

bool FProof::Matches(const FProof& Other) const
{
	return IsValid() && Other.IsValid() && ProofId == Other.ProofId;
}
