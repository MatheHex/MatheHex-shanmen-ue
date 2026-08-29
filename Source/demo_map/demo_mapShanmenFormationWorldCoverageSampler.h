#pragma once

#include "CoreMinimal.h"
#include "ShanmenWorldEntityRegistry.h"
#include "demo_mapShanmenFormationAreaProvider.h"

class AActor;
class UWorld;

enum class Edemo_mapShanmenFormationWorldCoverageStatus : uint8
{
	Sampled,
	AreaInvalid,
	WorldInvalid,
	RegistryInactive,
	RunMismatch,
	SourceSetEmpty,
	ActorUnavailable,
	ActorWorldMismatch,
	ActorUnregistered,
	DuplicateEntity,
	LocationInvalid,
	CoverageRejected
};

/**
 * One immediate World-to-pure-rule projection result.
 *
 * Queries and nested coverage receipts contain no UObject pointers. Sources are
 * caller-owned and are never retained by the stateless sampler.
 */
struct Fdemo_mapShanmenFormationWorldCoverageResult
{
	Edemo_mapShanmenFormationWorldCoverageStatus Status =
		Edemo_mapShanmenFormationWorldCoverageStatus::AreaInvalid;
	FString Diagnostic;
	TArray<Fdemo_mapShanmenFormationAreaQuery> Queries;
	Fdemo_mapShanmenFormationCoverageResult Coverage;

	bool IsSuccess() const;
};

/**
 * Narrow synchronous bridge from live registered Actors to P8.5 queries.
 *
 * The caller owns cadence and the explicit Actor subset. This bridge performs
 * no discovery, Tick, timer, overlap, effect, damage, registry mutation, or
 * pointer retention.
 */
class Fdemo_mapShanmenFormationWorldCoverageSampler
{
public:
	static Fdemo_mapShanmenFormationWorldCoverageResult Sample(
		UWorld* World,
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const TArray<AActor*>& SourceActors);
};
