#include "demo_mapShanmenItemCutover.h"

#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapProfileSessionSubsystem.h"

namespace
{
	Edemo_mapShanmenItemCutoverStatus MapFailedBind(
		const Fdemo_mapShanmenItemAuthorityBindResult& Bind)
	{
		switch (Bind.Status)
		{
		case Edemo_mapShanmenItemAuthorityBindStatus::WaitingForStableLegacy:
		case Edemo_mapShanmenItemAuthorityBindStatus::LegacyNotStable:
			return Edemo_mapShanmenItemCutoverStatus::LegacyNotStable;
		case Edemo_mapShanmenItemAuthorityBindStatus::LegacyRejected:
		case Edemo_mapShanmenItemAuthorityBindStatus::BindingMismatch:
			return Edemo_mapShanmenItemCutoverStatus::LegacyRejected;
		case Edemo_mapShanmenItemAuthorityBindStatus::RecoveryRequired:
		case Edemo_mapShanmenItemAuthorityBindStatus::PersistenceFailure:
			return Edemo_mapShanmenItemCutoverStatus::RecoveryRequired;
		default:
			return Edemo_mapShanmenItemCutoverStatus::InvalidRequest;
		}
	}

	bool FinalizeReadyResult(
		const Fdemo_mapProfileStorageContext& ProfileStorage,
		const FGuid& OwnerId,
		Fdemo_mapShanmenItemCutoverResult& Result)
	{
		Result.Fence = Fdemo_mapShanmenLegacyItemWriteFence::Inspect(
			ProfileStorage.RootDirectory, OwnerId);
		if (!Result.Fence.IsRetired())
		{
			Result.Status =
				Edemo_mapShanmenItemCutoverStatus::SafetyFenceFailure;
			Result.Diagnostic = FString::Printf(
				TEXT("Authority reported Ready but the durable legacy write fence did not close: %s"),
				*Result.Fence.Diagnostic);
			return false;
		}
		return true;
	}
}

Fdemo_mapShanmenItemCutoverResult
Fdemo_mapShanmenItemCutoverCoordinator::Execute(
	const Fdemo_mapProfileStorageContext& ProfileStorage,
	const FGuid& OwnerId,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Udemo_mapProfileSessionSubsystem& ProfileSession,
	const Fdemo_map0909BSectWarehouseService& Warehouse)
{
	Fdemo_mapShanmenItemCutoverResult Result;
	if (!IsInGameThread()
		|| ProfileStorage.RootDirectory.IsEmpty()
		|| !OwnerId.IsValid())
	{
		Result.Diagnostic =
			TEXT("Item cutover requires the Game Thread, storage root, and OwnerId.");
		return Result;
	}

	Result.AuthorityBind = Authority.BindExisting(ProfileStorage, OwnerId);
	if (Result.AuthorityBind.IsReady())
	{
		Result.Status = Result.AuthorityBind.Status
			== Edemo_mapShanmenItemAuthorityBindStatus::AlreadyReady
			? Edemo_mapShanmenItemCutoverStatus::AlreadyReady
			: Edemo_mapShanmenItemCutoverStatus::OpenedExisting;
		Result.Diagnostic = Result.AuthorityBind.Diagnostic;
		FinalizeReadyResult(ProfileStorage, OwnerId, Result);
		return Result;
	}
	if (Result.AuthorityBind.Status
		!= Edemo_mapShanmenItemAuthorityBindStatus::WaitingForStableLegacy)
	{
		Result.Status = MapFailedBind(Result.AuthorityBind);
		Result.Diagnostic = Result.AuthorityBind.Diagnostic;
		Result.Fence = Fdemo_mapShanmenLegacyItemWriteFence::Inspect(
			ProfileStorage.RootDirectory, OwnerId);
		return Result;
	}

	Fdemo_mapPersistentProfile Profile;
	FString SourceDiagnostic;
	Result.bProfileSourceRead =
		ProfileSession.TryCaptureStableProfileForItemMigration(
			Profile, &SourceDiagnostic);
	if (!Result.bProfileSourceRead || Profile.ProfileId != OwnerId)
	{
		Result.Status = Edemo_mapShanmenItemCutoverStatus::LegacyNotStable;
		Result.Diagnostic = Result.bProfileSourceRead
			? TEXT("Stable Profile source does not match the requested OwnerId.")
			: SourceDiagnostic;
		return Result;
	}
	Result.SourceProfileGeneration = Profile.SaveGeneration;

	FCodeBOutOfRaidInventoryRecord CodeB;
	Result.bCodeBSourceRead =
		Warehouse.CaptureStableItemMigrationRecord(CodeB, SourceDiagnostic);
	if (!Result.bCodeBSourceRead || CodeB.OwnerId != OwnerId)
	{
		Result.Status = Edemo_mapShanmenItemCutoverStatus::LegacyNotStable;
		Result.Diagnostic = Result.bCodeBSourceRead
			? TEXT("Stable Code B source does not match the requested OwnerId.")
			: SourceDiagnostic;
		return Result;
	}
	Result.SourceCodeBPersistentRevision = CodeB.PersistentRevision;

	Result.AuthorityBind = Authority.BindFromStableLegacy(
		ProfileStorage, OwnerId, Profile, CodeB);
	if (!Result.AuthorityBind.IsReady())
	{
		Result.Status = MapFailedBind(Result.AuthorityBind);
		Result.Diagnostic = Result.AuthorityBind.Diagnostic;
		Result.Fence = Fdemo_mapShanmenLegacyItemWriteFence::Inspect(
			ProfileStorage.RootDirectory, OwnerId);
		return Result;
	}

	Result.Status = Result.AuthorityBind.Status
		== Edemo_mapShanmenItemAuthorityBindStatus::CreatedFromLegacy
		? Edemo_mapShanmenItemCutoverStatus::CreatedFromLegacy
		: Edemo_mapShanmenItemCutoverStatus::OpenedExisting;
	Result.Diagnostic = Result.AuthorityBind.Diagnostic;
	FinalizeReadyResult(ProfileStorage, OwnerId, Result);
	return Result;
}
