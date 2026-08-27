#pragma once

#include "CoreMinimal.h"

#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenLegacyItemWriteFence.h"

class Fdemo_map0909BSectWarehouseService;
class Udemo_mapProfileSessionSubsystem;

/** Explicit product-level outcome of one existing-first authority cutover. */
enum class Edemo_mapShanmenItemCutoverStatus : uint8
{
	OpenedExisting,
	CreatedFromLegacy,
	AlreadyReady,
	LegacyNotStable,
	LegacyRejected,
	RecoveryRequired,
	InvalidRequest,
	SafetyFenceFailure
};

struct Fdemo_mapShanmenItemCutoverResult
{
	Edemo_mapShanmenItemCutoverStatus Status =
		Edemo_mapShanmenItemCutoverStatus::InvalidRequest;
	FString Diagnostic;
	Fdemo_mapShanmenItemAuthorityBindResult AuthorityBind;
	Fdemo_mapShanmenLegacyItemWriteFenceProbe Fence;
	int32 SourceProfileGeneration = INDEX_NONE;
	int32 SourceCodeBPersistentRevision = INDEX_NONE;
	bool bProfileSourceRead = false;
	bool bCodeBSourceRead = false;

	bool IsReady() const
	{
		return Status == Edemo_mapShanmenItemCutoverStatus::OpenedExisting
			|| Status
				== Edemo_mapShanmenItemCutoverStatus::CreatedFromLegacy
			|| Status == Edemo_mapShanmenItemCutoverStatus::AlreadyReady;
	}
};

/**
 * The sole explicit product cutover sequence.
 *
 * It first adopts an existing authority without touching legacy inputs.  Only
 * a proven missing document permits stable Profile and Code B copies to be
 * read, validated, and published.  Success is not returned until the shared
 * durable write fence observes the new authority marker.
 */
class Fdemo_mapShanmenItemCutoverCoordinator
{
public:
	static Fdemo_mapShanmenItemCutoverResult Execute(
		const Fdemo_mapProfileStorageContext& ProfileStorage,
		const FGuid& OwnerId,
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Udemo_mapProfileSessionSubsystem& ProfileSession,
		const Fdemo_map0909BSectWarehouseService& Warehouse);
};
