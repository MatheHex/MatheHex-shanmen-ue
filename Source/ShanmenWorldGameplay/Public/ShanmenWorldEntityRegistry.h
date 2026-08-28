#pragma once

#include "CoreMinimal.h"
#include "ShanmenWorldHitAdapter.h"
#include "UObject/ObjectKey.h"

#include "ShanmenWorldEntityRegistry.generated.h"

class UObject;

UENUM(BlueprintType)
enum class EShanmenWorldBindingResult : uint8
{
	Invalid,
	Bound,
	AlreadyBound,
	Removed,
	NotFound,
	RunMismatch,
	Conflict
};

/** Stable entity identity derived only from explicit Run and spawner inputs. */
struct SHANMENWORLDGAMEPLAY_API FShanmenWorldEntityIdFactory
{
	static FGuid MakeEntityId(
		const FGuid& RunId,
		FName SpawnSourceId,
		int32 SpawnOrdinal);
};

/**
 * Run-scoped mapping from transient UE objects to stable combat EntityIds.
 *
 * One object can expose a generic binding or exact physics-body aliases, but
 * all aliases for that object must agree on one EntityId. Several objects may
 * intentionally alias the same entity (for example an Actor and hit component).
 */
class SHANMENWORLDGAMEPLAY_API FShanmenWorldEntityRegistry final : public IShanmenWorldEntityResolver
{
public:
	bool TryBeginRun(const FGuid& RunId);
	bool TryEndRun(const FGuid& ExpectedRunId);
	void Reset();

	const FGuid& GetRunId() const { return ActiveRunId; }
	int32 NumObjectBindings() const { return Bindings.Num(); }

	EShanmenWorldBindingResult BindObject(
		const FGuid& ExpectedRunId,
		const UObject* WorldObject,
		const FGuid& EntityId,
		int32 BodyIndex = INDEX_NONE);

	EShanmenWorldBindingResult UnbindObject(
		const FGuid& ExpectedRunId,
		const UObject* WorldObject);

	bool TryResolveObject(
		const FGuid& ExpectedRunId,
		const UObject* WorldObject,
		int32 BodyIndex,
		FGuid& OutEntityId) const;

	virtual bool TryResolveEntityId(
		const FGuid& ExpectedRunId,
		EShanmenWorldContactSource ContactSource,
		const AActor* Actor,
		const UPrimitiveComponent* Component,
		int32 BodyIndex,
		FGuid& OutEntityId) const override;

private:
	struct FObjectBindings
	{
		FGuid EntityId;
		bool bMatchAllBodies = false;
		TSet<int32> ExactBodyIndices;
	};

	FGuid ActiveRunId;
	TMap<FObjectKey, FObjectBindings> Bindings;
};
