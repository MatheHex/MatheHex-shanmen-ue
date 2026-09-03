#pragma once

#include "CoreMinimal.h"
#include "ShanmenDivineSenseScan.h"
#include "ShanmenWorldEntityRegistry.h"

class AActor;
class UWorld;

/** Why one synchronous Divine Sense World sample did not publish a receipt. */
enum class Edemo_mapShanmenDivineSenseWorldObservationStatus : uint8
{
	Resolved,
	RuntimeInputInvalid,
	SampleBudgetInvalid,
	SampleBudgetExceeded,
	WorldInvalid,
	RegistryInactive,
	RunMismatch,
	SourceActorUnavailable,
	SourceActorWorldMismatch,
	SourceActorUnregistered,
	SourceIdentityMismatch,
	SourceLocationInvalid,
	SubjectActorUnavailable,
	SubjectActorWorldMismatch,
	SubjectActorUnregistered,
	DuplicateSubjectEntity,
	SubjectLocationInvalid,
	SubjectEvidenceUnavailable,
	SubjectEvidenceInvalid,
	ObservationRejected,
	ResolutionRejected
};

/** Caller-owned metadata/visibility evidence for one already selected Actor. */
struct Fdemo_mapShanmenDivineSenseWorldSubjectEvidence
{
	FGameplayTagContainer SubjectTags;
	bool bHasLineOfSight = false;
	int64 AuthorityRevision = INDEX_NONE;

	bool IsValid() const
	{
		return !SubjectTags.IsEmpty() && AuthorityRevision >= 0;
	}
};

/**
 * Injected read-only seam for tags, line of sight, and authority revision.
 *
 * The adapter invokes this exactly once per canonical unique subject after the
 * complete Actor batch passes structural validation. Implementations may read
 * the supplied live objects but must not retain them.
 */
class Idemo_mapShanmenDivineSenseWorldEvidenceProvider
{
public:
	virtual ~Idemo_mapShanmenDivineSenseWorldEvidenceProvider() = default;

	virtual bool TryCaptureSubjectEvidence(
		UWorld* World,
		const FShanmenDivineSenseScanRequest& Request,
		const AActor* SourceActor,
		const AActor* SubjectActor,
		const FGuid& SubjectEntityId,
		const FVector& SourceLocation,
		const FVector& SubjectLocation,
		Fdemo_mapShanmenDivineSenseWorldSubjectEvidence& OutEvidence) const = 0;
};

/** Pointer-free outcome of one bounded synchronous World sample. */
struct Fdemo_mapShanmenDivineSenseWorldObservationResult
{
	Edemo_mapShanmenDivineSenseWorldObservationStatus Status =
		Edemo_mapShanmenDivineSenseWorldObservationStatus::RuntimeInputInvalid;
	FString Diagnostic;
	int32 SubjectActorBudget = INDEX_NONE;
	int32 ObservedSubjectCount = 0;
	FShanmenDivineSenseScanReceipt Receipt;

	bool IsSuccess() const;
};

/**
 * Stateless bridge from one explicit registered Actor set to the P19.0 scan.
 *
 * The caller owns cadence and discovery. This adapter samples each transform
 * once, canonicalizes provider invocation order, retains no UObject pointer,
 * and performs no implicit Actor enumeration, retry, Tick, resource mutation,
 * target mutation, persistence, or presentation work.
 */
class Fdemo_mapShanmenDivineSenseWorldObservationAdapter
{
public:
	static Fdemo_mapShanmenDivineSenseWorldObservationResult SampleAndResolve(
		UWorld* World,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		AActor* SourceActor,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		int32 ScanOrdinal,
		int32 SubjectActorBudget,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);
};
