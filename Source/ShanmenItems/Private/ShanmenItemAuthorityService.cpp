#include "ShanmenItemAuthorityService.h"

#include "Misc/ScopeLock.h"

FShanmenItemMigrationAuthorization
FShanmenItemMigrationAuthorization::Explicit(const FGuid& MigrationId)
{
	FShanmenItemMigrationAuthorization Result;
	Result.AuthorizedMigrationId = MigrationId;
	return Result;
}

bool FShanmenItemMigrationAuthorization::Permits(
	const FShanmenItemMigrationEvidence& Evidence) const
{
	return AuthorizedMigrationId.IsValid()
		&& Evidence.MigrationId.IsValid()
		&& AuthorizedMigrationId == Evidence.MigrationId;
}

bool FShanmenItemAuthorityStartResult::IsReady() const
{
	return Status == EShanmenItemAuthorityStartStatus::OpenedExisting
		|| Status == EShanmenItemAuthorityStartStatus::RecoveredExisting
		|| Status == EShanmenItemAuthorityStartStatus::CreatedFromMigration
		|| Status == EShanmenItemAuthorityStartStatus::AlreadyReady;
}

bool FShanmenItemDurableCommandResult::IsDurable() const
{
	return Receipt.IsValid()
		&& (Status == EShanmenItemDurableCommandStatus::Persisted
		|| Status == EShanmenItemDurableCommandStatus::Replayed
		|| Status == EShanmenItemDurableCommandStatus::RejectedAndPersisted
		|| Status == EShanmenItemDurableCommandStatus::RejectedWithoutMutation
		|| Status == EShanmenItemDurableCommandStatus::ResolvedAfterReopen);
}

bool FShanmenItemDurableCommandResult::IsCommandSuccess() const
{
	return IsDurable() && Receipt.IsSuccess();
}

FShanmenItemAuthorityStartResult FShanmenItemAuthorityService::StartExisting(
	const FShanmenItemStorageContext& InStorage)
{
	FScopeLock Lock(&Mutex);
	return StartLocked(InStorage, nullptr, nullptr, nullptr);
}

FShanmenItemAuthorityStartResult
FShanmenItemAuthorityService::StartFromAuthorizedMigration(
	const FShanmenItemStorageContext& InStorage,
	const FShanmenItemMigrationAuthorization& Authorization,
	const FShanmenItemAuthoritySnapshot& MigrationCandidate,
	const FShanmenItemMigrationEvidence& Migration)
{
	FScopeLock Lock(&Mutex);
	return StartLocked(
		InStorage, &Authorization, &MigrationCandidate, &Migration);
}

FShanmenItemAuthorityStartResult FShanmenItemAuthorityService::StartLocked(
	const FShanmenItemStorageContext& InStorage,
	const FShanmenItemMigrationAuthorization* Authorization,
	const FShanmenItemAuthoritySnapshot* MigrationCandidate,
	const FShanmenItemMigrationEvidence* Migration)
{
	FShanmenItemAuthorityStartResult Result;
	if (State == EShanmenItemAuthorityServiceState::Ready)
	{
		Result.Status = EShanmenItemAuthorityStartStatus::AlreadyReady;
		Result.Diagnostic = TEXT("Item authority service is already ready; its storage binding was not changed.");
		Result.DocumentGeneration = Document.SaveGeneration;
		return Result;
	}

	// Durable authority always wins. Migration arguments are intentionally not
	// inspected until this probe proves that no authority document exists.
	const FShanmenItemLoadResult Loaded = Store.LoadExisting(InStorage);
	Result.LoadStatus = Loaded.Status;
	Result.bDiskStateChanged = Loaded.bDiskStateChanged;
	if (Loaded.IsSuccess())
	{
		FString InstallError;
		if (!InstallDocumentLocked(Loaded.Document, InStorage, InstallError))
		{
			Result.Status = EShanmenItemAuthorityStartStatus::RepositoryLoadFailure;
			Result.Diagnostic = InstallError;
			State = EShanmenItemAuthorityServiceState::RecoveryRequired;
			return Result;
		}
		Result.Status = Loaded.Status == EShanmenItemLoadStatus::LoadedPrimary
			? EShanmenItemAuthorityStartStatus::OpenedExisting
			: EShanmenItemAuthorityStartStatus::RecoveredExisting;
		Result.Diagnostic = Loaded.Diagnostic;
		Result.DocumentGeneration = Document.SaveGeneration;
		return Result;
	}

	if (Loaded.Status != EShanmenItemLoadStatus::Missing)
	{
		State = EShanmenItemAuthorityServiceState::RecoveryRequired;
		Result.Status = EShanmenItemAuthorityStartStatus::PersistenceFailure;
		Result.Diagnostic = FString::Printf(
			TEXT("Durable authority could not be opened and legacy migration was not attempted: %s"),
			*Loaded.Diagnostic);
		return Result;
	}

	if (Authorization == nullptr
		|| MigrationCandidate == nullptr
		|| Migration == nullptr)
	{
		State = EShanmenItemAuthorityServiceState::Closed;
		Result.Status = EShanmenItemAuthorityStartStatus::MigrationRequired;
		Result.Diagnostic = TEXT("No durable authority exists; an explicitly authorized migration is required.");
		return Result;
	}
	if (!Authorization->Permits(*Migration))
	{
		State = EShanmenItemAuthorityServiceState::Closed;
		Result.Status = EShanmenItemAuthorityStartStatus::MigrationNotAuthorized;
		Result.Diagnostic = TEXT("Migration capability does not authorize the supplied MigrationId.");
		return Result;
	}

	const FShanmenItemOpenResult Opened = Store.OpenOrCreateFromMigration(
		*MigrationCandidate, *Migration, InStorage);
	Result.OpenStatus = Opened.Status;
	Result.bDiskStateChanged |= Opened.bDiskStateChanged;
	if (Opened.IsSuccess())
	{
		FString InstallError;
		if (!InstallDocumentLocked(Opened.Document, InStorage, InstallError))
		{
			Result.Status = EShanmenItemAuthorityStartStatus::RepositoryLoadFailure;
			Result.Diagnostic = InstallError;
			State = EShanmenItemAuthorityServiceState::RecoveryRequired;
			return Result;
		}
		switch (Opened.Status)
		{
		case EShanmenItemOpenStatus::CreatedFromMigration:
			Result.Status = EShanmenItemAuthorityStartStatus::CreatedFromMigration;
			break;
		case EShanmenItemOpenStatus::RecoveredExisting:
			Result.Status = EShanmenItemAuthorityStartStatus::RecoveredExisting;
			break;
		default:
			Result.Status = EShanmenItemAuthorityStartStatus::OpenedExisting;
			break;
		}
		Result.Diagnostic = Opened.Diagnostic;
		Result.DocumentGeneration = Document.SaveGeneration;
		return Result;
	}

	if (Opened.Status == EShanmenItemOpenStatus::InvalidRequest)
	{
		State = EShanmenItemAuthorityServiceState::Closed;
		Result.Status = EShanmenItemAuthorityStartStatus::InvalidRequest;
		Result.Diagnostic = Opened.Diagnostic;
		return Result;
	}

	// Another owner of this storage may have published between our missing
	// probe and create. Reopen once; whichever valid document is durable wins.
	const FShanmenItemLoadResult Reopened = Store.LoadExisting(InStorage);
	Result.LoadStatus = Reopened.Status;
	Result.bDiskStateChanged |= Reopened.bDiskStateChanged;
	if (Reopened.IsSuccess())
	{
		FString InstallError;
		if (InstallDocumentLocked(Reopened.Document, InStorage, InstallError))
		{
			Result.Status = Reopened.Status == EShanmenItemLoadStatus::LoadedPrimary
				? EShanmenItemAuthorityStartStatus::OpenedExisting
				: EShanmenItemAuthorityStartStatus::RecoveredExisting;
			Result.Diagnostic = TEXT("A durable authority appeared during migration startup and was adopted instead of reimporting legacy data.");
			Result.DocumentGeneration = Document.SaveGeneration;
			return Result;
		}
		Result.Status = EShanmenItemAuthorityStartStatus::RepositoryLoadFailure;
		Result.Diagnostic = InstallError;
		State = EShanmenItemAuthorityServiceState::RecoveryRequired;
		return Result;
	}

	State = EShanmenItemAuthorityServiceState::RecoveryRequired;
	Result.Status = EShanmenItemAuthorityStartStatus::PersistenceFailure;
	Result.Diagnostic = FString::Printf(
		TEXT("Authorized migration did not produce a verifiable authority document: %s; reopen: %s"),
		*Opened.Diagnostic, *Reopened.Diagnostic);
	return Result;
}

bool FShanmenItemAuthorityService::InstallDocumentLocked(
	const FShanmenItemAuthorityDocument& InDocument,
	const FShanmenItemStorageContext& InStorage,
	FString& OutDiagnostic)
{
	FShanmenItemRepository Candidate;
	EShanmenItemTransactionError RepositoryError =
		EShanmenItemTransactionError::InvariantViolation;
	if (!Candidate.TryLoadSnapshot(InDocument.Authority, &RepositoryError))
	{
		OutDiagnostic = FString::Printf(
			TEXT("Durable authority passed document validation but repository load failed with error %d."),
			static_cast<int32>(RepositoryError));
		return false;
	}
	Storage = InStorage;
	Document = InDocument;
	Repository = MoveTemp(Candidate);
	State = EShanmenItemAuthorityServiceState::Ready;
	OutDiagnostic.Reset();
	return true;
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::ReserveDurable(
	const FShanmenItemReserveRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.Reserve(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::CommitDurable(
	const FShanmenItemReservationActionRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.Commit(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::CommitBatchDurable(
	const FShanmenItemReservationBatchRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.CommitBatch(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::StartPreparedRunDurable(
	const FShanmenItemRunStartRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.StartPreparedRun(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::AmendReservationPurposeDurable(
	const FShanmenItemReservationAmendRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.AmendReservationPurpose(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::CancelDurable(
	const FShanmenItemReservationActionRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.Cancel(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::ReleaseDeploymentDurable(
	const FShanmenItemReservationActionRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.ReleaseDeployment(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::ClaimPreparedRunDurable(
	const FShanmenItemRunClaimRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.ClaimPreparedRun(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::ConsumePreparedRunItemDurable(
	const FShanmenItemRunConsumeRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.ConsumePreparedRunItem(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::PreparePreparedRunQuantityIntentDurable(
	const FShanmenItemRunQuantityIntentRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.PreparePreparedRunQuantityIntent(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::FinalizePreparedRunQuantityIntentDurable(
	const FShanmenItemRunQuantityIntentFinalizeRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.FinalizePreparedRunQuantityIntent(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::CommitPreparedRunResourcesDurable(
	const FShanmenItemRunResourceCommitRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.CommitPreparedRunResources(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::PreparePreparedRunResourceIntentDurable(
	const FShanmenItemRunResourceIntentRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.PreparePreparedRunResourceIntent(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::FinalizePreparedRunResourceIntentDurable(
	const FShanmenItemRunResourceIntentFinalizeRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.FinalizePreparedRunResourceIntent(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::FinalizePreparedRunDurable(
	const FShanmenItemRunFinalizeRequest& Request)
{
	FScopeLock Lock(&Mutex);
	return ExecuteCommandLocked(
		[&Request](FShanmenItemRepository& MutableRepository)
		{
			return MutableRepository.FinalizePreparedRun(Request);
		});
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::ExecuteCommandLocked(
	TFunctionRef<FShanmenItemTransactionReceipt(FShanmenItemRepository&)>
		Command)
{
	FShanmenItemDurableCommandResult Result;
	if (State != EShanmenItemAuthorityServiceState::Ready)
	{
		Result.Status = State == EShanmenItemAuthorityServiceState::RecoveryRequired
			? EShanmenItemDurableCommandStatus::RecoveryRequired
			: EShanmenItemDurableCommandStatus::NotReady;
		Result.Diagnostic = State == EShanmenItemAuthorityServiceState::RecoveryRequired
			? TEXT("Item authority requires recovery before accepting commands.")
			: TEXT("Item authority is not open.");
		return Result;
	}

	const FShanmenItemAuthorityDocument BeforeDocument = Document;
	const FShanmenItemAuthoritySnapshot Before = Repository.CaptureSnapshot();
	const FShanmenItemTransactionReceipt Receipt = Command(Repository);
	const FShanmenItemAuthoritySnapshot After = Repository.CaptureSnapshot();
	const bool bRepositoryMutated = !(After == Before);

	Result.Receipt = Receipt;
	Result.bRepositoryMutated = bRepositoryMutated;
	const FShanmenItemSaveResult Saved = Store.SaveAuthority(
		Document, After, Storage);
	Result.SaveStatus = Saved.Status;
	Result.bDiskStateChanged = Saved.bDiskStateChanged;
	Result.DocumentGeneration = Document.SaveGeneration;
	if (Saved.IsSuccess())
	{
		if (bRepositoryMutated)
		{
			Result.Status = Receipt.IsSuccess()
				? EShanmenItemDurableCommandStatus::Persisted
				: EShanmenItemDurableCommandStatus::RejectedAndPersisted;
		}
		else
		{
			Result.Status = Receipt.IsSuccess()
				? EShanmenItemDurableCommandStatus::Replayed
				: EShanmenItemDurableCommandStatus::RejectedWithoutMutation;
		}
		Result.Diagnostic = Saved.Diagnostic;
		return Result;
	}

	return ReconcileFailedSaveLocked(
		Saved, BeforeDocument, After, Receipt, bRepositoryMutated);
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::ReconcileFailedSaveLocked(
	const FShanmenItemSaveResult& SaveResult,
	const FShanmenItemAuthorityDocument& BeforeDocument,
	const FShanmenItemAuthoritySnapshot& After,
	const FShanmenItemTransactionReceipt& Receipt,
	bool bRepositoryMutated)
{
	const FShanmenItemLoadResult Loaded = Store.LoadExisting(Storage);
	if (!Loaded.IsSuccess())
	{
		return EnterRecoveryRequiredLocked(
			SaveResult, Loaded, Receipt, bRepositoryMutated,
			FString::Printf(
				TEXT("Persistence failed and durable outcome could not be reopened: save=%s; reopen=%s"),
				*SaveResult.Diagnostic, *Loaded.Diagnostic));
	}

	if (Loaded.Document == BeforeDocument)
	{
		FString InstallError;
		if (!InstallDocumentLocked(Loaded.Document, Storage, InstallError))
		{
			return EnterRecoveryRequiredLocked(
				SaveResult, Loaded, Receipt, bRepositoryMutated, InstallError);
		}
		FShanmenItemDurableCommandResult Result;
		Result.Status =
			EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack;
		Result.Receipt = Receipt;
		Result.SaveStatus = SaveResult.Status;
		Result.ReconcileLoadStatus = Loaded.Status;
		Result.Diagnostic = FString::Printf(
			TEXT("Command was rolled back to the exact durable pre-command document: %s"),
			*SaveResult.Diagnostic);
		Result.bRepositoryMutated = bRepositoryMutated;
		Result.bDiskStateChanged = SaveResult.bDiskStateChanged
			|| Loaded.bDiskStateChanged;
		Result.DocumentGeneration = Document.SaveGeneration;
		return Result;
	}

	if (bRepositoryMutated && Loaded.Document.Authority == After)
	{
		FString InstallError;
		if (!InstallDocumentLocked(Loaded.Document, Storage, InstallError))
		{
			return EnterRecoveryRequiredLocked(
				SaveResult, Loaded, Receipt, bRepositoryMutated, InstallError);
		}
		FShanmenItemDurableCommandResult Result;
		Result.Status = EShanmenItemDurableCommandStatus::ResolvedAfterReopen;
		Result.Receipt = Receipt;
		Result.SaveStatus = SaveResult.Status;
		Result.ReconcileLoadStatus = Loaded.Status;
		Result.Diagnostic = TEXT("Persistence verification was ambiguous, but reopen proved the exact post-command authority is durable.");
		Result.bRepositoryMutated = true;
		Result.bDiskStateChanged = SaveResult.bDiskStateChanged
			|| Loaded.bDiskStateChanged;
		Result.DocumentGeneration = Document.SaveGeneration;
		return Result;
	}

	return EnterRecoveryRequiredLocked(
		SaveResult, Loaded, Receipt, bRepositoryMutated,
		TEXT("Reopened authority matches neither the exact pre-command document nor the command result; manual recovery is required."));
}

FShanmenItemDurableCommandResult
FShanmenItemAuthorityService::EnterRecoveryRequiredLocked(
	const FShanmenItemSaveResult& SaveResult,
	const FShanmenItemLoadResult& LoadResult,
	const FShanmenItemTransactionReceipt& Receipt,
	bool bRepositoryMutated,
	const FString& Diagnostic)
{
	State = EShanmenItemAuthorityServiceState::RecoveryRequired;
	FShanmenItemDurableCommandResult Result;
	Result.Status = EShanmenItemDurableCommandStatus::RecoveryRequired;
	Result.Receipt = Receipt;
	Result.SaveStatus = SaveResult.Status;
	Result.ReconcileLoadStatus = LoadResult.Status;
	Result.Diagnostic = Diagnostic;
	Result.bRepositoryMutated = bRepositoryMutated;
	Result.bDiskStateChanged = SaveResult.bDiskStateChanged
		|| LoadResult.bDiskStateChanged;
	Result.DocumentGeneration = Document.SaveGeneration;
	return Result;
}

EShanmenItemAuthorityServiceState
FShanmenItemAuthorityService::GetState() const
{
	FScopeLock Lock(&Mutex);
	return State;
}

bool FShanmenItemAuthorityService::TryGetDocument(
	FShanmenItemAuthorityDocument& OutDocument) const
{
	FScopeLock Lock(&Mutex);
	if (State != EShanmenItemAuthorityServiceState::Ready)
	{
		return false;
	}
	OutDocument = Document;
	return true;
}

bool FShanmenItemAuthorityService::TryCaptureSnapshot(
	FShanmenItemAuthoritySnapshot& OutSnapshot) const
{
	FScopeLock Lock(&Mutex);
	if (State != EShanmenItemAuthorityServiceState::Ready)
	{
		return false;
	}
	OutSnapshot = Repository.CaptureSnapshot();
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
void FShanmenItemAuthorityService::SetInjectedFailureForTests(
	EShanmenItemStoreFailureStage Stage)
{
	FScopeLock Lock(&Mutex);
	Storage.InjectedFailure = Stage;
}
#endif
