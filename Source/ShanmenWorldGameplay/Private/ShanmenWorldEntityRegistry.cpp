#include "ShanmenWorldEntityRegistry.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}
}

FGuid FShanmenWorldEntityIdFactory::MakeEntityId(
	const FGuid& RunId,
	FName SpawnSourceId,
	int32 SpawnOrdinal)
{
	if (!RunId.IsValid() || SpawnSourceId.IsNone() || SpawnOrdinal < 0)
	{
		return FGuid();
	}

	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.World.Entity.r1"),
		{
			GuidDigits(RunId),
			SpawnSourceId.ToString(),
			FString::FromInt(SpawnOrdinal)
		});
}

bool FShanmenWorldEntityRegistry::TryBeginRun(const FGuid& RunId)
{
	if (!RunId.IsValid())
	{
		return false;
	}
	if (ActiveRunId.IsValid())
	{
		return ActiveRunId == RunId;
	}

	ActiveRunId = RunId;
	return true;
}

bool FShanmenWorldEntityRegistry::TryEndRun(const FGuid& ExpectedRunId)
{
	if (!ActiveRunId.IsValid() || ExpectedRunId != ActiveRunId)
	{
		return false;
	}

	Reset();
	return true;
}

void FShanmenWorldEntityRegistry::Reset()
{
	Bindings.Reset();
	ActiveRunId.Invalidate();
}

EShanmenWorldBindingResult FShanmenWorldEntityRegistry::BindObject(
	const FGuid& ExpectedRunId,
	const UObject* WorldObject,
	const FGuid& EntityId,
	int32 BodyIndex)
{
	if (!WorldObject || !EntityId.IsValid() || BodyIndex < INDEX_NONE)
	{
		return EShanmenWorldBindingResult::Invalid;
	}
	if (!ActiveRunId.IsValid() || ExpectedRunId != ActiveRunId)
	{
		return EShanmenWorldBindingResult::RunMismatch;
	}

	const FObjectKey ObjectKey(WorldObject);
	FObjectBindings* Existing = Bindings.Find(ObjectKey);
	if (!Existing)
	{
		FObjectBindings& Added = Bindings.Add(ObjectKey);
		Added.EntityId = EntityId;
		if (BodyIndex == INDEX_NONE)
		{
			Added.bMatchAllBodies = true;
		}
		else
		{
			Added.ExactBodyIndices.Add(BodyIndex);
		}
		return EShanmenWorldBindingResult::Bound;
	}

	if (Existing->EntityId != EntityId)
	{
		return EShanmenWorldBindingResult::Conflict;
	}
	if (BodyIndex == INDEX_NONE)
	{
		if (Existing->bMatchAllBodies)
		{
			return EShanmenWorldBindingResult::AlreadyBound;
		}
		Existing->bMatchAllBodies = true;
		return EShanmenWorldBindingResult::Bound;
	}
	if (Existing->ExactBodyIndices.Contains(BodyIndex))
	{
		return EShanmenWorldBindingResult::AlreadyBound;
	}

	Existing->ExactBodyIndices.Add(BodyIndex);
	return EShanmenWorldBindingResult::Bound;
}

EShanmenWorldBindingResult FShanmenWorldEntityRegistry::UnbindObject(
	const FGuid& ExpectedRunId,
	const UObject* WorldObject)
{
	if (!WorldObject)
	{
		return EShanmenWorldBindingResult::Invalid;
	}
	if (!ActiveRunId.IsValid() || ExpectedRunId != ActiveRunId)
	{
		return EShanmenWorldBindingResult::RunMismatch;
	}

	return Bindings.Remove(FObjectKey(WorldObject)) > 0
		? EShanmenWorldBindingResult::Removed
		: EShanmenWorldBindingResult::NotFound;
}

bool FShanmenWorldEntityRegistry::TryResolveObject(
	const FGuid& ExpectedRunId,
	const UObject* WorldObject,
	int32 BodyIndex,
	FGuid& OutEntityId) const
{
	OutEntityId.Invalidate();
	if (!WorldObject
		|| BodyIndex < INDEX_NONE
		|| !ActiveRunId.IsValid()
		|| ExpectedRunId != ActiveRunId)
	{
		return false;
	}

	const FObjectBindings* Existing = Bindings.Find(FObjectKey(WorldObject));
	if (!Existing
		|| (!Existing->bMatchAllBodies
			&& (BodyIndex == INDEX_NONE || !Existing->ExactBodyIndices.Contains(BodyIndex))))
	{
		return false;
	}

	OutEntityId = Existing->EntityId;
	return OutEntityId.IsValid();
}

bool FShanmenWorldEntityRegistry::TryResolveEntityId(
	const FGuid& ExpectedRunId,
	EShanmenWorldContactSource,
	const AActor* Actor,
	const UPrimitiveComponent* Component,
	int32 BodyIndex,
	FGuid& OutEntityId) const
{
	OutEntityId.Invalidate();
	if (!ActiveRunId.IsValid() || ExpectedRunId != ActiveRunId)
	{
		return false;
	}

	FGuid ComponentEntityId;
	const bool bHasComponentBinding = Component
		&& TryResolveObject(ExpectedRunId, Component, BodyIndex, ComponentEntityId);

	FGuid ActorEntityId;
	const bool bHasActorBinding = Actor
		&& TryResolveObject(ExpectedRunId, Actor, BodyIndex, ActorEntityId);

	if (bHasComponentBinding && bHasActorBinding && ComponentEntityId != ActorEntityId)
	{
		return false;
	}
	if (bHasComponentBinding)
	{
		OutEntityId = ComponentEntityId;
		return true;
	}
	if (bHasActorBinding)
	{
		OutEntityId = ActorEntityId;
		return true;
	}
	return false;
}
