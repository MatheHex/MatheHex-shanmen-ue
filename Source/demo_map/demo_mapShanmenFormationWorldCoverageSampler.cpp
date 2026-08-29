#include "demo_mapShanmenFormationWorldCoverageSampler.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool QueryLess(
		const Fdemo_mapShanmenFormationAreaQuery& Left,
		const Fdemo_mapShanmenFormationAreaQuery& Right)
	{
		return GuidDigits(Left.SubjectEntityId)
			< GuidDigits(Right.SubjectEntityId);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	Fdemo_mapShanmenFormationWorldCoverageResult Reject(
		const Edemo_mapShanmenFormationWorldCoverageStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationWorldCoverageResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationWorldCoverageResult::IsSuccess() const
{
	if (Status != Edemo_mapShanmenFormationWorldCoverageStatus::Sampled
		|| Queries.IsEmpty() || !Coverage.IsSuccess()
		|| Coverage.Receipt.Memberships.Num() != Queries.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Queries.Num(); ++Index)
	{
		const Fdemo_mapShanmenFormationAreaQuery& Query = Queries[Index];
		const Fdemo_mapShanmenFormationMembershipReceipt& Membership =
			Coverage.Receipt.Memberships[Index];
		if (!Query.IsValid()
			|| (Index > 0 && !QueryLess(Queries[Index - 1], Query))
			|| Membership.SubjectEntityId != Query.SubjectEntityId
			|| Membership.WorldLocation != Query.WorldLocation)
		{
			return false;
		}
	}
	return true;
}

Fdemo_mapShanmenFormationWorldCoverageResult
Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
	UWorld* World,
	const Fdemo_mapShanmenFormationAreaSnapshot& Area,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	const TArray<AActor*>& SourceActors)
{
	if (!Area.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldCoverageStatus::AreaInvalid,
			TEXT("World coverage requires one valid frozen formation area."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldCoverageStatus::WorldInvalid,
			TEXT("World coverage requires one live caller-owned World."));
	}
	if (!EntityRegistry.GetRunId().IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldCoverageStatus::RegistryInactive,
			TEXT("World coverage requires one active entity registry."));
	}
	if (EntityRegistry.GetRunId() != Area.RunId)
	{
		return Reject(
			Edemo_mapShanmenFormationWorldCoverageStatus::RunMismatch,
			TEXT("Area and entity registry must belong to the same Run."));
	}
	if (SourceActors.IsEmpty())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldCoverageStatus::SourceSetEmpty,
			TEXT("The cadence owner must provide an explicit Actor subset."));
	}

	TArray<Fdemo_mapShanmenFormationAreaQuery> Queries;
	Queries.Reserve(SourceActors.Num());
	TSet<FGuid> EntityIds;
	for (AActor* Actor : SourceActors)
	{
		if (!::IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			return Reject(
				Edemo_mapShanmenFormationWorldCoverageStatus::ActorUnavailable,
				TEXT("Every source Actor must remain live for the complete synchronous sample."));
		}
		if (Actor->GetWorld() != World)
		{
			return Reject(
				Edemo_mapShanmenFormationWorldCoverageStatus::ActorWorldMismatch,
				TEXT("One coverage sample cannot mix Actors from another World."));
		}

		FGuid EntityId;
		if (!EntityRegistry.TryResolveObject(
				Area.RunId, Actor, INDEX_NONE, EntityId))
		{
			return Reject(
				Edemo_mapShanmenFormationWorldCoverageStatus::ActorUnregistered,
				TEXT("Every source Actor requires an exact stable registry binding."));
		}
		if (EntityIds.Contains(EntityId))
		{
			return Reject(
				Edemo_mapShanmenFormationWorldCoverageStatus::DuplicateEntity,
				TEXT("One sample accepts one live Actor location per stable entity."));
		}
		const FVector WorldLocation = Actor->GetActorLocation();
		if (!IsFiniteVector(WorldLocation))
		{
			return Reject(
				Edemo_mapShanmenFormationWorldCoverageStatus::LocationInvalid,
				TEXT("A sampled Actor location must be finite."));
		}
		EntityIds.Add(EntityId);
		Queries.Add({ EntityId, WorldLocation });
	}
	Queries.Sort(QueryLess);

	const Fdemo_mapShanmenFormationCoverageResult Coverage =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(Area, Queries);
	if (!Coverage.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldCoverageStatus::CoverageRejected,
			TEXT("P8.5 rejected the canonical World query batch."));
	}

	Fdemo_mapShanmenFormationWorldCoverageResult Result;
	Result.Status = Edemo_mapShanmenFormationWorldCoverageStatus::Sampled;
	Result.Diagnostic =
		TEXT("Registered live Actors produced one immutable P8.5 coverage receipt.");
	Result.Queries = MoveTemp(Queries);
	Result.Coverage = Coverage;
	return Result;
}
