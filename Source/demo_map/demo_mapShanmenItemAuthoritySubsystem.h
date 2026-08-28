#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ShanmenItemAuthorityService.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapShanmenItemMigration.h"

#include "demo_mapShanmenItemAuthoritySubsystem.generated.h"

/** Product-visible lifecycle of the one GameInstance-owned 0.0.10 item authority. */
enum class Edemo_mapShanmenItemAuthorityLifecycleState : uint8
{
	Uninitialized,
	Unbound,
	WaitingForStableLegacy,
	Ready,
	RecoveryRequired
};

/** Explicit result of binding the product owner to one Profile storage identity. */
enum class Edemo_mapShanmenItemAuthorityBindStatus : uint8
{
	OpenedExisting,
	RecoveredExisting,
	CreatedFromLegacy,
	AlreadyReady,
	WaitingForStableLegacy,
	LegacyNotStable,
	LegacyRejected,
	BindingMismatch,
	InvalidRequest,
	PersistenceFailure,
	RecoveryRequired
};

struct Fdemo_mapShanmenItemAuthorityBindResult
{
	Edemo_mapShanmenItemAuthorityBindStatus Status =
		Edemo_mapShanmenItemAuthorityBindStatus::InvalidRequest;
	FString Diagnostic;
	FShanmenItemAuthorityStartResult AuthorityStart;
	Fdemo_mapShanmenItemMigrationReceipt MigrationReceipt;
	/** True only after the existing-authority probe proved the document absent. */
	bool bLegacyInputsRead = false;

	bool IsReady() const
	{
		return Status == Edemo_mapShanmenItemAuthorityBindStatus::OpenedExisting
			|| Status == Edemo_mapShanmenItemAuthorityBindStatus::RecoveredExisting
			|| Status == Edemo_mapShanmenItemAuthorityBindStatus::CreatedFromLegacy
			|| Status == Edemo_mapShanmenItemAuthorityBindStatus::AlreadyReady;
	}
};

/**
 * The sole product-level owner of FShanmenItemAuthorityService.
 *
 * GameInstance construction is deliberately storage-lazy. BindExisting probes
 * only the schema-1 authority. BindFromStableLegacy performs the same probe
 * first and reads its immutable legacy arguments only when no authority exists.
 * It refuses an active Code A or Code B Run and requires an exact deterministic
 * migration receipt before publishing generation one.
 */
UCLASS()
class Udemo_mapShanmenItemAuthoritySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	Fdemo_mapShanmenItemAuthorityBindResult BindExisting(
		const Fdemo_mapProfileStorageContext& ProfileStorage,
		const FGuid& OwnerId);
	Fdemo_mapShanmenItemAuthorityBindResult BindFromStableLegacy(
		const Fdemo_mapProfileStorageContext& ProfileStorage,
		const FGuid& OwnerId,
		const Fdemo_mapPersistentProfile& CodeAProfile,
		const FCodeBOutOfRaidInventoryRecord& CodeBRecord);

	FShanmenItemDurableCommandResult ReserveDurable(
		const FShanmenItemReserveRequest& Request);
	FShanmenItemDurableCommandResult CommitDurable(
		const FShanmenItemReservationActionRequest& Request);
	FShanmenItemDurableCommandResult CommitBatchDurable(
		const FShanmenItemReservationBatchRequest& Request);
	FShanmenItemDurableCommandResult StartPreparedRunDurable(
		const FShanmenItemRunStartRequest& Request);
	FShanmenItemDurableCommandResult AmendReservationPurposeDurable(
		const FShanmenItemReservationAmendRequest& Request);
	FShanmenItemDurableCommandResult CancelDurable(
		const FShanmenItemReservationActionRequest& Request);
	FShanmenItemDurableCommandResult ReleaseDeploymentDurable(
		const FShanmenItemReservationActionRequest& Request);
	FShanmenItemDurableCommandResult ClaimPreparedRunDurable(
		const FShanmenItemRunClaimRequest& Request);
	FShanmenItemDurableCommandResult FinalizePreparedRunDurable(
		const FShanmenItemRunFinalizeRequest& Request);

	Edemo_mapShanmenItemAuthorityLifecycleState GetLifecycleState() const
	{
		return LifecycleState;
	}
	FGuid GetBoundOwnerId() const { return BoundOwnerId; }
	const FString& GetBoundStorageRoot() const { return BoundStorageRoot; }
	bool TryCaptureSnapshot(FShanmenItemAuthoritySnapshot& OutSnapshot) const;
	bool TryGetDocument(FShanmenItemAuthorityDocument& OutDocument) const;

	/** Frozen production migration target; callers cannot substitute ad-hoc content. */
	static FShanmenContentStamp ProductContentStamp();

#if WITH_DEV_AUTOMATION_TESTS
	void SetInjectedFailureForAutomation(EShanmenItemStoreFailureStage Stage);
#endif

private:
	bool EstablishOrValidateBinding(
		const Fdemo_mapProfileStorageContext& ProfileStorage,
		const FGuid& OwnerId,
		Fdemo_mapShanmenItemAuthorityBindResult& OutResult);
	Fdemo_mapShanmenItemAuthorityBindResult ApplyStartResult(
		const FShanmenItemAuthorityStartResult& Start,
		bool bLegacyInputsRead);
	FShanmenItemDurableCommandResult RejectCommand(const FString& Diagnostic) const;
	void SynchronizeCommandState(const FShanmenItemDurableCommandResult& Result);

	TUniquePtr<FShanmenItemAuthorityService> AuthorityService;
	Edemo_mapShanmenItemAuthorityLifecycleState LifecycleState =
		Edemo_mapShanmenItemAuthorityLifecycleState::Uninitialized;
	FString BoundStorageRoot;
	FGuid BoundOwnerId;
};
