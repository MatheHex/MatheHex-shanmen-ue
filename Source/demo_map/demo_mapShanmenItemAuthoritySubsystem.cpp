#include "demo_mapShanmenItemAuthoritySubsystem.h"

#include "Misc/Paths.h"

namespace
{
	FString CanonicalStorageRoot(const FString& Root)
	{
		FString Canonical = FPaths::ConvertRelativePathToFull(
			Root.TrimStartAndEnd());
		FPaths::NormalizeDirectoryName(Canonical);
		return Canonical;
	}
}

void Udemo_mapShanmenItemAuthoritySubsystem::Initialize(
	FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AuthorityService = MakeUnique<FShanmenItemAuthorityService>();
	LifecycleState = Edemo_mapShanmenItemAuthorityLifecycleState::Unbound;
	BoundStorageRoot.Reset();
	BoundOwnerId.Invalidate();
}

void Udemo_mapShanmenItemAuthoritySubsystem::Deinitialize()
{
	AuthorityService.Reset();
	LifecycleState = Edemo_mapShanmenItemAuthorityLifecycleState::Uninitialized;
	BoundStorageRoot.Reset();
	BoundOwnerId.Invalidate();
	Super::Deinitialize();
}

FShanmenContentStamp
Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp()
{
	FShanmenContentStamp Content;
	Content.Version = FName(TEXT("Shanmen.Items.0.0.10"));
	Content.Digest = TEXT("Shanmen.Items.LegacyAuthority.Schema1.v1");
	return Content;
}

bool Udemo_mapShanmenItemAuthoritySubsystem::EstablishOrValidateBinding(
	const Fdemo_mapProfileStorageContext& ProfileStorage,
	const FGuid& OwnerId,
	Fdemo_mapShanmenItemAuthorityBindResult& OutResult)
{
	if (!IsInGameThread())
	{
		OutResult.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::InvalidRequest;
		OutResult.Diagnostic =
			TEXT("Shanmen item authority binding is restricted to the Game Thread.");
		return false;
	}
	if (!AuthorityService
		|| LifecycleState
			== Edemo_mapShanmenItemAuthorityLifecycleState::Uninitialized)
	{
		OutResult.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::InvalidRequest;
		OutResult.Diagnostic =
			TEXT("Shanmen item authority has no initialized GameInstance owner.");
		return false;
	}
	const FString CanonicalRoot = CanonicalStorageRoot(
		ProfileStorage.RootDirectory);
	if (ProfileStorage.RootDirectory.TrimStartAndEnd().IsEmpty()
		|| CanonicalRoot.IsEmpty()
		|| !OwnerId.IsValid())
	{
		OutResult.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::InvalidRequest;
		OutResult.Diagnostic =
			TEXT("Shanmen item authority requires one explicit Profile storage root and valid OwnerId.");
		return false;
	}
	if (BoundOwnerId.IsValid()
		&& (BoundOwnerId != OwnerId
			|| !BoundStorageRoot.Equals(
				CanonicalRoot, ESearchCase::IgnoreCase)))
	{
		OutResult.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::BindingMismatch;
		OutResult.Diagnostic =
			TEXT("The GameInstance item authority is already bound to a different Profile storage identity.");
		return false;
	}
	if (!BoundOwnerId.IsValid())
	{
		BoundOwnerId = OwnerId;
		BoundStorageRoot = CanonicalRoot;
	}
	return true;
}

Fdemo_mapShanmenItemAuthorityBindResult
Udemo_mapShanmenItemAuthoritySubsystem::ApplyStartResult(
	const FShanmenItemAuthorityStartResult& Start,
	const bool bLegacyInputsRead)
{
	Fdemo_mapShanmenItemAuthorityBindResult Result;
	Result.AuthorityStart = Start;
	Result.Diagnostic = Start.Diagnostic;
	Result.bLegacyInputsRead = bLegacyInputsRead;
	switch (Start.Status)
	{
	case EShanmenItemAuthorityStartStatus::OpenedExisting:
		LifecycleState = Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
		Result.Status = Edemo_mapShanmenItemAuthorityBindStatus::OpenedExisting;
		break;
	case EShanmenItemAuthorityStartStatus::RecoveredExisting:
		LifecycleState = Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
		Result.Status = Edemo_mapShanmenItemAuthorityBindStatus::RecoveredExisting;
		break;
	case EShanmenItemAuthorityStartStatus::CreatedFromMigration:
		LifecycleState = Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
		Result.Status = Edemo_mapShanmenItemAuthorityBindStatus::CreatedFromLegacy;
		break;
	case EShanmenItemAuthorityStartStatus::AlreadyReady:
		LifecycleState = Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
		Result.Status = Edemo_mapShanmenItemAuthorityBindStatus::AlreadyReady;
		break;
	case EShanmenItemAuthorityStartStatus::MigrationRequired:
	case EShanmenItemAuthorityStartStatus::MigrationNotAuthorized:
	case EShanmenItemAuthorityStartStatus::InvalidRequest:
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::WaitingForStableLegacy;
		Result.Status = Start.Status
			== EShanmenItemAuthorityStartStatus::MigrationRequired
			? Edemo_mapShanmenItemAuthorityBindStatus::WaitingForStableLegacy
			: Edemo_mapShanmenItemAuthorityBindStatus::LegacyRejected;
		break;
	case EShanmenItemAuthorityStartStatus::PersistenceFailure:
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired;
		Result.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::PersistenceFailure;
		break;
	case EShanmenItemAuthorityStartStatus::RepositoryLoadFailure:
	default:
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired;
		Result.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::RecoveryRequired;
		break;
	}
	return Result;
}

Fdemo_mapShanmenItemAuthorityBindResult
Udemo_mapShanmenItemAuthoritySubsystem::BindExisting(
	const Fdemo_mapProfileStorageContext& ProfileStorage,
	const FGuid& OwnerId)
{
	Fdemo_mapShanmenItemAuthorityBindResult Result;
	if (!EstablishOrValidateBinding(ProfileStorage, OwnerId, Result))
	{
		return Result;
	}
	if (LifecycleState
		== Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired)
	{
		Result.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::RecoveryRequired;
		Result.Diagnostic =
			TEXT("The bound item authority requires recovery; legacy fallback and rebinding are disabled.");
		return Result;
	}
	return ApplyStartResult(
		AuthorityService->StartExisting(
			FShanmenItemStorageContext::ForRoot(
				BoundStorageRoot, BoundOwnerId)),
		false);
}

Fdemo_mapShanmenItemAuthorityBindResult
Udemo_mapShanmenItemAuthoritySubsystem::BindFromStableLegacy(
	const Fdemo_mapProfileStorageContext& ProfileStorage,
	const FGuid& OwnerId,
	const Fdemo_mapPersistentProfile& CodeAProfile,
	const FCodeBOutOfRaidInventoryRecord& CodeBRecord)
{
	Fdemo_mapShanmenItemAuthorityBindResult Result;
	if (!EstablishOrValidateBinding(ProfileStorage, OwnerId, Result))
	{
		return Result;
	}
	if (LifecycleState
		== Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired)
	{
		Result.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::RecoveryRequired;
		Result.Diagnostic =
			TEXT("The bound item authority requires recovery; a legacy source cannot replace it.");
		return Result;
	}

	const FShanmenItemStorageContext Storage =
		FShanmenItemStorageContext::ForRoot(
			BoundStorageRoot, BoundOwnerId);
	const FShanmenItemAuthorityStartResult Existing =
		AuthorityService->StartExisting(Storage);
	if (Existing.Status
		!= EShanmenItemAuthorityStartStatus::MigrationRequired)
	{
		return ApplyStartResult(Existing, false);
	}

	Result.bLegacyInputsRead = true;
	if (CodeAProfile.ProfileId != BoundOwnerId
		|| CodeBRecord.OwnerId != BoundOwnerId)
	{
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::WaitingForStableLegacy;
		Result.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::LegacyRejected;
		Result.Diagnostic =
			TEXT("Stable legacy binding requires exact Profile, Code B, and requested Owner identity.");
		return Result;
	}
	if (CodeAProfile.ActiveRun.bHasActiveRun
		|| CodeBRecord.bHasActiveRunInventorySession)
	{
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::WaitingForStableLegacy;
		Result.Status =
			Edemo_mapShanmenItemAuthorityBindStatus::LegacyNotStable;
		Result.Diagnostic =
			TEXT("Stable legacy binding is unavailable while Code A or Code B owns an active Run.");
		return Result;
	}

	const Fdemo_mapShanmenItemMigrationResult Migration =
		Fdemo_mapShanmenItemMigration::BuildCandidate(
			CodeAProfile, CodeBRecord, ProductContentStamp());
	if (!Migration.IsSuccess())
	{
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::WaitingForStableLegacy;
		Result.Status =
			Migration.Error == Edemo_mapShanmenItemMigrationError::ActiveRunUnsupported
			? Edemo_mapShanmenItemAuthorityBindStatus::LegacyNotStable
			: Edemo_mapShanmenItemAuthorityBindStatus::LegacyRejected;
		Result.Diagnostic = Migration.Diagnostic;
		return Result;
	}

	const FShanmenItemMigrationEvidence Evidence =
		Migration.Receipt.ToPersistenceEvidence();
	Result = ApplyStartResult(
		AuthorityService->StartFromAuthorizedMigration(
			Storage,
			FShanmenItemMigrationAuthorization::Explicit(
				Migration.Receipt.MigrationId),
			Migration.Candidate,
			Evidence),
		true);
	Result.MigrationReceipt = Migration.Receipt;
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::RejectCommand(
	const FString& Diagnostic) const
{
	FShanmenItemDurableCommandResult Result;
	Result.Status = EShanmenItemDurableCommandStatus::NotReady;
	Result.Diagnostic = Diagnostic;
	return Result;
}

void Udemo_mapShanmenItemAuthoritySubsystem::SynchronizeCommandState(
	const FShanmenItemDurableCommandResult& Result)
{
	if (Result.Status == EShanmenItemDurableCommandStatus::RecoveryRequired
		|| !AuthorityService
		|| AuthorityService->GetState()
			== EShanmenItemAuthorityServiceState::RecoveryRequired)
	{
		LifecycleState =
			Edemo_mapShanmenItemAuthorityLifecycleState::RecoveryRequired;
	}
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::ReserveDurable(
	const FShanmenItemReserveRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Reserve requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->ReserveDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::CommitDurable(
	const FShanmenItemReservationActionRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Commit requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->CommitDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::CommitBatchDurable(
	const FShanmenItemReservationBatchRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("CommitBatch requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->CommitBatchDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::StartPreparedRunDurable(
	const FShanmenItemRunStartRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Atomic prepared Run start requires the Ready authority on the Game Thread."));
	}
	const FShanmenItemDurableCommandResult Result =
		AuthorityService->StartPreparedRunDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::AmendReservationPurposeDurable(
	const FShanmenItemReservationAmendRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("AmendReservationPurpose requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->AmendReservationPurposeDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::CancelDurable(
	const FShanmenItemReservationActionRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Cancel requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->CancelDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::ReleaseDeploymentDurable(
	const FShanmenItemReservationActionRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("ReleaseDeployment requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->ReleaseDeploymentDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::ClaimPreparedRunDurable(
	const FShanmenItemRunClaimRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Prepared Run claim requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->ClaimPreparedRunDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::ConsumePreparedRunItemDurable(
	const FShanmenItemRunConsumeRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Prepared Run item use requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->ConsumePreparedRunItemDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::PreparePreparedRunResourceIntentDurable(
	const FShanmenItemRunResourceIntentRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Prepared Run resource intent requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->PreparePreparedRunResourceIntentDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::FinalizePreparedRunResourceIntentDurable(
	const FShanmenItemRunResourceIntentFinalizeRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Prepared Run resource intent finalization requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->FinalizePreparedRunResourceIntentDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

FShanmenItemDurableCommandResult
Udemo_mapShanmenItemAuthoritySubsystem::FinalizePreparedRunDurable(
	const FShanmenItemRunFinalizeRequest& Request)
{
	if (!IsInGameThread() || !AuthorityService
		|| LifecycleState != Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return RejectCommand(
			TEXT("Prepared Run finalization requires the ready GameInstance item authority on the Game Thread."));
	}
	FShanmenItemDurableCommandResult Result =
		AuthorityService->FinalizePreparedRunDurable(Request);
	SynchronizeCommandState(Result);
	return Result;
}

bool Udemo_mapShanmenItemAuthoritySubsystem::TryCaptureSnapshot(
	FShanmenItemAuthoritySnapshot& OutSnapshot) const
{
	OutSnapshot = FShanmenItemAuthoritySnapshot();
	return IsInGameThread()
		&& AuthorityService
		&& LifecycleState == Edemo_mapShanmenItemAuthorityLifecycleState::Ready
		&& AuthorityService->TryCaptureSnapshot(OutSnapshot);
}

bool Udemo_mapShanmenItemAuthoritySubsystem::TryGetDocument(
	FShanmenItemAuthorityDocument& OutDocument) const
{
	OutDocument = FShanmenItemAuthorityDocument();
	return IsInGameThread()
		&& AuthorityService
		&& LifecycleState == Edemo_mapShanmenItemAuthorityLifecycleState::Ready
		&& AuthorityService->TryGetDocument(OutDocument);
}

#if WITH_DEV_AUTOMATION_TESTS
void Udemo_mapShanmenItemAuthoritySubsystem::SetInjectedFailureForAutomation(
	const EShanmenItemStoreFailureStage Stage)
{
	if (IsInGameThread() && AuthorityService)
	{
		AuthorityService->SetInjectedFailureForTests(Stage);
	}
}
#endif
