#include "demo_mapShanmenDivineSenseWorldObservationAdapter.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	struct FSubjectWorldSample
	{
		AActor* Actor = nullptr;
		FGuid EntityId;
		FVector Location = FVector::ZeroVector;
	};

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	double CanonicalZero(double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FVector CanonicalVector(const FVector& Value)
	{
		return FVector(
			CanonicalZero(Value.X),
			CanonicalZero(Value.Y),
			CanonicalZero(Value.Z));
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool SampleComesBefore(
		const FSubjectWorldSample& Left,
		const FSubjectWorldSample& Right)
	{
		return GuidDigits(Left.EntityId) < GuidDigits(Right.EntityId);
	}

	Fdemo_mapShanmenDivineSenseWorldObservationResult Reject(
		Edemo_mapShanmenDivineSenseWorldObservationStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenDivineSenseWorldObservationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenDivineSenseWorldObservationResult::IsSuccess() const
{
	return Status
			== Edemo_mapShanmenDivineSenseWorldObservationStatus::Resolved
		&& SubjectActorBudget >= 0
		&& ObservedSubjectCount >= 0
		&& ObservedSubjectCount <= SubjectActorBudget
		&& Receipt.IsValid()
		&& Receipt.NumReveals() <= ObservedSubjectCount;
}

Fdemo_mapShanmenDivineSenseWorldObservationResult
Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* SourceActor,
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenDivineSenseDefinition& Definition,
	int32 ScanOrdinal,
	int32 SubjectActorBudget,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!Action.IsValid()
		|| !Definition.IsValid()
		|| Action.GetActionDefinitionId()
			!= Definition.GetActionDefinitionId()
		|| ScanOrdinal < 0)
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				RuntimeInputInvalid,
			TEXT("World observation requires one valid Divine Sense action, definition, and ordinal."));
	}
	if (SubjectActorBudget < 0)
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SampleBudgetInvalid,
			TEXT("Divine Sense World sampling requires a non-negative explicit Actor budget."));
	}
	if (SubjectActors.Num() > SubjectActorBudget)
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SampleBudgetExceeded,
			TEXT("The caller-supplied Actor set exceeds this sample's explicit budget."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::WorldInvalid,
			TEXT("Divine Sense sampling requires one live caller-owned World."));
	}
	if (!EntityRegistry.GetRunId().IsValid())
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				RegistryInactive,
			TEXT("Divine Sense sampling requires one active entity registry."));
	}
	if (EntityRegistry.GetRunId() != Action.GetRunId())
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::RunMismatch,
			TEXT("Divine Sense action and entity registry must belong to the same Run."));
	}
	if (!::IsValid(SourceActor) || SourceActor->IsActorBeingDestroyed())
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SourceActorUnavailable,
			TEXT("The Divine Sense source Actor must remain live for the synchronous sample."));
	}
	if (SourceActor->GetWorld() != World)
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SourceActorWorldMismatch,
			TEXT("The Divine Sense source Actor must belong to the supplied World."));
	}

	FGuid SourceEntityId;
	if (!EntityRegistry.TryResolveObject(
			Action.GetRunId(), SourceActor, INDEX_NONE, SourceEntityId))
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SourceActorUnregistered,
			TEXT("The Divine Sense source Actor requires an exact stable registry binding."));
	}
	if (SourceEntityId != Action.GetSourceEntityId())
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SourceIdentityMismatch,
			TEXT("The source Actor binding must match the frozen action source."));
	}
	const FVector SourceLocation = CanonicalVector(
		SourceActor->GetActorLocation());
	if (!IsFiniteVector(SourceLocation))
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				SourceLocationInvalid,
			TEXT("The sampled Divine Sense source location must be finite."));
	}

	FShanmenDivineSenseScanRequest Request;
	if (!FShanmenDivineSenseScanRequest::TryCapture(
			Action, Definition, SourceLocation, ScanOrdinal, Request))
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				RuntimeInputInvalid,
			TEXT("P19.0 rejected the frozen Divine Sense World request."));
	}

	TArray<FSubjectWorldSample> Samples;
	Samples.Reserve(SubjectActors.Num());
	TSet<FGuid> SubjectEntityIds;
	SubjectEntityIds.Reserve(SubjectActors.Num());
	for (AActor* SubjectActor : SubjectActors)
	{
		if (!::IsValid(SubjectActor)
			|| SubjectActor->IsActorBeingDestroyed())
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					SubjectActorUnavailable,
				TEXT("Every supplied subject Actor must remain live for the complete synchronous sample."));
		}
		if (SubjectActor->GetWorld() != World)
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					SubjectActorWorldMismatch,
				TEXT("One Divine Sense sample cannot mix Actors from another World."));
		}

		FGuid SubjectEntityId;
		if (!EntityRegistry.TryResolveObject(
				Action.GetRunId(),
				SubjectActor,
				INDEX_NONE,
				SubjectEntityId))
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					SubjectActorUnregistered,
				TEXT("Every supplied subject Actor requires an exact stable registry binding."));
		}
		if (SubjectEntityIds.Contains(SubjectEntityId))
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					DuplicateSubjectEntity,
				TEXT("One World sample accepts one live Actor transform per stable subject entity."));
		}
		const FVector SubjectLocation = CanonicalVector(
			SubjectActor->GetActorLocation());
		if (!IsFiniteVector(SubjectLocation))
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					SubjectLocationInvalid,
				TEXT("Every sampled Divine Sense subject location must be finite."));
		}

		SubjectEntityIds.Add(SubjectEntityId);
		Samples.Add({ SubjectActor, SubjectEntityId, SubjectLocation });
	}
	Samples.Sort(SampleComesBefore);

	TArray<FShanmenDivineSenseObservation> Observations;
	Observations.Reserve(Samples.Num());
	for (const FSubjectWorldSample& Sample : Samples)
	{
		Fdemo_mapShanmenDivineSenseWorldSubjectEvidence Evidence;
		if (!EvidenceProvider.TryCaptureSubjectEvidence(
				World,
				Request,
				SourceActor,
				Sample.Actor,
				Sample.EntityId,
				SourceLocation,
				Sample.Location,
				Evidence))
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					SubjectEvidenceUnavailable,
				TEXT("The injected authority could not capture one subject's tags, visibility, and revision."));
		}
		if (!Evidence.IsValid())
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					SubjectEvidenceInvalid,
				TEXT("The injected subject evidence must have tags and a non-negative authority revision."));
		}

		FShanmenDivineSenseObservationCapture Capture;
		Capture.SubjectEntityId = Sample.EntityId;
		Capture.WorldLocation = Sample.Location;
		Capture.SubjectTags = Evidence.SubjectTags;
		Capture.bHasLineOfSight = Evidence.bHasLineOfSight;
		Capture.AuthorityRevision = Evidence.AuthorityRevision;
		FShanmenDivineSenseObservation Observation;
		if (!FShanmenDivineSenseObservation::TryCapture(
				Request, Capture, Observation))
		{
			return Reject(
				Edemo_mapShanmenDivineSenseWorldObservationStatus::
					ObservationRejected,
				TEXT("P19.0 rejected one canonical World observation."));
		}
		Observations.Add(MoveTemp(Observation));
	}

	FShanmenDivineSenseScanReceipt Receipt;
	if (!FShanmenDivineSenseResolver::TryResolve(
			Request, Observations, Receipt))
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				ResolutionRejected,
			TEXT("P19.0 rejected the complete canonical World observation batch."));
	}

	Fdemo_mapShanmenDivineSenseWorldObservationResult Result;
	Result.Status =
		Edemo_mapShanmenDivineSenseWorldObservationStatus::Resolved;
	Result.Diagnostic =
		TEXT("Registered live Actors produced one immutable Divine Sense scan receipt.");
	Result.SubjectActorBudget = SubjectActorBudget;
	Result.ObservedSubjectCount = Observations.Num();
	Result.Receipt = MoveTemp(Receipt);
	if (!Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenDivineSenseWorldObservationStatus::
				ResolutionRejected,
			TEXT("Divine Sense World sampling produced an invalid staged result."));
	}
	return Result;
}
