#include "demo_mapShanmenControlledWeaponWorldThreatSampler.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenWorldEntityRegistry.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenControlledWeaponActor.h"
#include "demo_mapShanmenControlledWeaponWorldLifecycle.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	struct FResolvedWorldContact
	{
		FGuid TargetEntityId;
		FString ComponentPath;
		Fdemo_mapShanmenControlledWeaponOrbitThreatContact Contact;
	};

	bool ContactLess(
		const FResolvedWorldContact& Left,
		const FResolvedWorldContact& Right)
	{
		if (Left.TargetEntityId != Right.TargetEntityId)
		{
			return GuidLess(Left.TargetEntityId, Right.TargetEntityId);
		}
		if (Left.Contact.Overlap.ItemIndex
			!= Right.Contact.Overlap.ItemIndex)
		{
			return Left.Contact.Overlap.ItemIndex
				< Right.Contact.Overlap.ItemIndex;
		}
		return Left.ComponentPath < Right.ComponentPath;
	}

	Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Reject(
		const Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus Status,
		const Fdemo_mapShanmenControlledWeaponWorldThreatSampler& Owner,
		const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Result;
		Result.Status = Status;
		Result.RunId = Owner.GetRunId();
		Result.ItemInstanceId = Owner.GetItemInstanceId();
		Result.ObservedTick = TimelineSample.IsValid()
			? TimelineSample.GetCurrentTick()
			: INDEX_NONE;
		Result.ScheduledTick = Owner.GetNextScheduledTick();
		Result.SampleSequence = Owner.NumCommittedSamples();
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult::
IsSampled() const
{
	return Status
			== Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::Sampled
		&& RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& IntentId.IsValid()
		&& ObservedTick >= 0
		&& ScheduledTick >= 0
		&& SampleSequence >= 0
		&& RawOverlapCount >= RoutedContactCount
		&& RoutedContactCount >= 0
		&& Route.IsAccepted()
		&& Route.IntentId == IntentId
		&& Route.RunId == RunId
		&& Route.SampleSequence == SampleSequence
		&& Route.ItemCount == 1;
}

bool Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult::IsNoOp() const
{
	return (Status
			== Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::NotDue
		|| Status
			== Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				NotOrbiting)
		&& RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ObservedTick >= 0
		&& ScheduledTick >= 0
		&& SampleSequence >= 0
		&& !IntentId.IsValid()
		&& RawOverlapCount == 0
		&& RoutedContactCount == 0
		&& !Route.IsAccepted();
}

FGuid Fdemo_mapShanmenControlledWeaponWorldThreatSampler::MakeIntentId(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTimelineId,
	const FGuid& RequestedItemInstanceId,
	const int64 SampleSequence,
	const int64 ScheduledTick)
{
	if (!RequestedRunId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| !RequestedItemInstanceId.IsValid()
		|| SampleSequence < 0
		|| ScheduledTick < 0)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.ControlledWeapon.WorldThreatSample.Intent.r1"),
		{
			GuidDigits(RequestedRunId),
			GuidDigits(RequestedTimelineId),
			GuidDigits(RequestedItemInstanceId),
			LexToString(SampleSequence),
			LexToString(ScheduledTick)
		});
}

bool Fdemo_mapShanmenControlledWeaponWorldThreatSampler::TryBegin(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTimelineId,
	const Fdemo_mapShanmenControlledWeaponWorldLifecycle& Lifecycle,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| !Lifecycle.IsActive()
		|| Lifecycle.GetRunId() != RequestedRunId
		|| !Lifecycle.GetItemInstanceId().IsValid()
		|| Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
			RequestedRunId) != RequestedTimelineId)
	{
		OutDiagnostic =
			TEXT("World threat cadence requires one exact active World lifecycle and canonical Run timeline.");
		return false;
	}
	if (!IsEmpty())
	{
		if (RunId == RequestedRunId
			&& TimelineId == RequestedTimelineId
			&& ItemInstanceId == Lifecycle.GetItemInstanceId())
		{
			OutDiagnostic =
				TEXT("World threat cadence is already active for this exact flying sword.");
			return true;
		}
		OutDiagnostic =
			TEXT("World threat cadence rejects a second active Run or item.");
		return false;
	}

	RunId = RequestedRunId;
	ItemInstanceId = Lifecycle.GetItemInstanceId();
	TimelineId = RequestedTimelineId;
	NextScheduledTick = 0;
	CommittedSampleCount = 0;
	OutDiagnostic =
		TEXT("Canonical flying-sword World threat cadence began at timeline tick zero.");
	return IsValid();
}

Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult
Fdemo_mapShanmenControlledWeaponWorldThreatSampler::TrySample(
	UWorld* World,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
	const Fdemo_mapShanmenControlledWeaponWorldLifecycle& Lifecycle,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenControlledWeaponRunHost& Host,
	Fdemo_mapShanmenControlledWeaponThreatSampleRouter& Router)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::OwnerInvalid,
			*this,
			TimelineSample,
			TEXT("World threat cadence owner contains invalid retained state."));
	}
	if (IsEmpty())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::OwnerInactive,
			*this,
			TimelineSample,
			TEXT("World threat cadence requires one active flying sword."));
	}
	if (!::IsValid(World)
		|| !TimelineSample.IsValid()
		|| !Lifecycle.IsActive()
		|| !Coordinator.IsReady()
		|| !Host.IsValid()
		|| !Router.IsValid())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				DependenciesUnavailable,
			*this,
			TimelineSample,
			TEXT("World threat sample requires live World, timeline, lifecycle, Coordinator, Host, and Router state."));
	}
	if (Lifecycle.GetRunId() != RunId
		|| Lifecycle.GetItemInstanceId() != ItemInstanceId
		|| Coordinator.GetRunId() != RunId
		|| Host.GetRunId() != RunId
		|| Host.GetSourceEntityId() != Coordinator.GetPlayerEntityId())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::RunMismatch,
			*this,
			TimelineSample,
			TEXT("World threat sample identities do not name one Run and item."));
	}
	if (TimelineSample.GetTimelineId() != TimelineId)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				TimelineMismatch,
			*this,
			TimelineSample,
			TEXT("World threat sample belongs to another combat timeline."));
	}
	if (Router.GetNextSampleSequence() != CommittedSampleCount
		|| (!Router.IsEmpty() && Router.GetRunId() != RunId))
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				RouterMismatch,
			*this,
			TimelineSample,
			TEXT("World cadence and P6.22 Router watermarks diverged."));
	}
	if (CommittedSampleCount == MAX_int64
		|| TimelineSample.GetCurrentTick()
			> MAX_int64 - CanonicalSampleIntervalTicks())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				SequenceExhausted,
			*this,
			TimelineSample,
			TEXT("World threat cadence cannot advance without integer overflow."));
	}
	if (TimelineSample.GetCurrentTick() < NextScheduledTick)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::NotDue,
			*this,
			TimelineSample,
			TEXT("Canonical World threat cadence has not reached its next tick."));
	}

	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Host.FindController(ItemInstanceId);
	if (!Controller || !Controller->IsOrbiting())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::NotOrbiting,
			*this,
			TimelineSample,
			TEXT("Near-body threat sampling is inactive while the flying sword is not orbiting."));
	}

	Ademo_mapShanmenControlledWeaponActor* WeaponActor =
		Lifecycle.GetWeaponActor();
	UBoxComponent* Collision = WeaponActor
		? WeaponActor->GetCollisionComponent()
		: nullptr;
	AActor* SourceActor = WeaponActor
		? WeaponActor->GetSourceActor()
		: nullptr;
	const FVector QueryLocation = Collision
		? Collision->GetComponentLocation()
		: FVector::ZeroVector;
	const FVector QueryExtent = Collision
		? Collision->GetScaledBoxExtent()
		: FVector::ZeroVector;
	if (!::IsValid(WeaponActor)
		|| WeaponActor->IsActorBeingDestroyed()
		|| !::IsValid(SourceActor)
		|| WeaponActor->GetWorld() != World
		|| SourceActor->GetWorld() != World
		|| !Collision
		|| !Collision->IsRegistered()
		|| Collision->GetCollisionEnabled() != ECollisionEnabled::QueryOnly
		|| !IsFiniteVector(QueryLocation)
		|| !IsFiniteVector(QueryExtent)
		|| QueryExtent.GetMin() <= 0.0)
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				CollisionUnavailable,
			*this,
			TimelineSample,
			TEXT("Canonical flying-sword collision geometry is unavailable for sampling."));
	}

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ShanmenControlledWeaponWorldThreat),
		false,
		WeaponActor);
	QueryParams.AddIgnoredActor(SourceActor);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		QueryLocation,
		Collision->GetComponentQuat(),
		ObjectTypes,
		FCollisionShape::MakeBox(QueryExtent),
		QueryParams);

	TArray<FResolvedWorldContact> Resolved;
	Resolved.Reserve(Overlaps.Num());
	const FShanmenWorldEntityRegistry& Registry =
		Coordinator.GetEntityRegistry();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		UPrimitiveComponent* TargetComponent = Overlap.GetComponent();
		FGuid TargetEntityId;
		if (!::IsValid(TargetActor)
			|| TargetActor->IsActorBeingDestroyed()
			|| TargetActor->GetWorld() != World
			|| TargetActor == WeaponActor
			|| TargetActor == SourceActor
			|| !Registry.TryResolveEntityId(
				RunId,
				EShanmenWorldContactSource::Overlap,
				TargetActor,
				TargetComponent,
				Overlap.ItemIndex,
				TargetEntityId)
			|| !TargetEntityId.IsValid()
			|| TargetEntityId == Host.GetSourceEntityId())
		{
			continue;
		}

		const FVector ContactLocation = TargetActor->GetActorLocation();
		FVector ContactNormal = ContactLocation - QueryLocation;
		if (!IsFiniteVector(ContactLocation)
			|| !IsFiniteVector(ContactNormal))
		{
			continue;
		}
		if (!ContactNormal.Normalize())
		{
			ContactNormal = FVector::UpVector;
		}

		FResolvedWorldContact& Contact = Resolved.AddDefaulted_GetRef();
		Contact.TargetEntityId = TargetEntityId;
		Contact.ComponentPath = TargetComponent
			? TargetComponent->GetPathName(TargetActor)
			: FString();
		Contact.Contact.Overlap = Overlap;
		Contact.Contact.ContactLocation = ContactLocation;
		Contact.Contact.ContactNormal = ContactNormal;
	}
	Resolved.Sort(ContactLess);

	Fdemo_mapShanmenControlledWeaponThreatSampleRequest Request;
	Request.ItemInstanceId = ItemInstanceId;
	FGuid PreviousTargetEntityId;
	for (FResolvedWorldContact& Contact : Resolved)
	{
		if (Contact.TargetEntityId == PreviousTargetEntityId)
		{
			continue;
		}
		PreviousTargetEntityId = Contact.TargetEntityId;
		Request.Contacts.Add(MoveTemp(Contact.Contact));
	}

	const int64 ObservedTick = TimelineSample.GetCurrentTick();
	const int64 ScheduledTick = NextScheduledTick;
	const int64 SampleSequence = CommittedSampleCount;
	const FGuid IntentId = MakeIntentId(
		RunId,
		TimelineId,
		ItemInstanceId,
		SampleSequence,
		ScheduledTick);
	Fdemo_mapShanmenControlledWeaponThreatSampleIntent Intent;
	if (!IntentId.IsValid()
		|| !Fdemo_mapShanmenControlledWeaponThreatSampleIntent::TryCapture(
			IntentId,
			RunId,
			SampleSequence,
			Coordinator,
			{ Request },
			Intent))
	{
		Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Result =
			Reject(
				Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
					IntentCaptureRejected,
				*this,
				TimelineSample,
				TEXT("Registered World contacts could not form one immutable P6.22 intent."));
		Result.IntentId = IntentId;
		Result.RawOverlapCount = Overlaps.Num();
		Result.RoutedContactCount = Request.Contacts.Num();
		return Result;
	}

	const int64 Remainder =
		ObservedTick % CanonicalSampleIntervalTicks();
	const int64 Advance = CanonicalSampleIntervalTicks() - Remainder;
	const int64 CandidateNextScheduledTick = ObservedTick + Advance;
	const int64 CandidateCommittedSampleCount = CommittedSampleCount + 1;
	Fdemo_mapShanmenControlledWeaponWorldThreatSampler OwnerCandidate = *this;
	OwnerCandidate.NextScheduledTick = CandidateNextScheduledTick;
	OwnerCandidate.CommittedSampleCount = CandidateCommittedSampleCount;
	if (!OwnerCandidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::ResultInvalid,
			*this,
			TimelineSample,
			TEXT("World threat cadence could not stage its next bounded watermark."));
	}

	Fdemo_mapShanmenControlledWeaponRunHost HostCandidate = Host;
	Fdemo_mapShanmenControlledWeaponThreatSampleRouter RouterCandidate = Router;
	Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Result;
	Result.RunId = RunId;
	Result.ItemInstanceId = ItemInstanceId;
	Result.IntentId = IntentId;
	Result.ObservedTick = ObservedTick;
	Result.ScheduledTick = ScheduledTick;
	Result.SampleSequence = SampleSequence;
	Result.RawOverlapCount = Overlaps.Num();
	Result.RoutedContactCount = Request.Contacts.Num();
	Result.Route = RouterCandidate.TryRoute(
		HostCandidate, Coordinator, Intent);
	if (!Result.Route.IsAccepted())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::RouteRejected;
		Result.Diagnostic = Result.Route.Diagnostic.IsEmpty()
			? TEXT("P6.22 rejected the canonical World threat sample.")
			: Result.Route.Diagnostic;
		return Result;
	}

	if (!HostCandidate.IsValid()
		|| !RouterCandidate.IsValid()
		|| Result.Route.IntentId != IntentId
		|| Result.Route.RunId != RunId
		|| Result.Route.SampleSequence != SampleSequence
		|| RouterCandidate.GetNextSampleSequence()
			!= CandidateCommittedSampleCount)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::ResultInvalid;
		Result.Diagnostic =
			TEXT("Accepted World threat sample failed final cadence validation.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::Sampled;
	Result.Diagnostic = Request.Contacts.IsEmpty()
		? TEXT("Canonical cadence routed one empty, zero-effect orbit threat sample.")
		: TEXT("Canonical cadence routed registered near-body contacts through P6.22.");
	if (!Result.IsSampled())
	{
		return Reject(
			Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::ResultInvalid,
			*this,
			TimelineSample,
			TEXT("Staged World threat sample is internally inconsistent."));
	}

	Host = MoveTemp(HostCandidate);
	Router = MoveTemp(RouterCandidate);
	*this = MoveTemp(OwnerCandidate);
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponWorldThreatSampler::IsValid() const
{
	if (!RunId.IsValid()
		&& !ItemInstanceId.IsValid()
		&& !TimelineId.IsValid())
	{
		return NextScheduledTick == 0 && CommittedSampleCount == 0;
	}
	return RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& TimelineId.IsValid()
		&& TimelineId
			== Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId)
		&& NextScheduledTick >= 0
		&& NextScheduledTick % CanonicalSampleIntervalTicks() == 0
		&& CommittedSampleCount >= 0
		&& CommittedSampleCount
			<= NextScheduledTick / CanonicalSampleIntervalTicks();
}

bool Fdemo_mapShanmenControlledWeaponWorldThreatSampler::IsEmpty() const
{
	return IsValid()
		&& !RunId.IsValid()
		&& !ItemInstanceId.IsValid()
		&& !TimelineId.IsValid();
}

bool Fdemo_mapShanmenControlledWeaponWorldThreatSampler::IsActiveForRun(
	const FGuid& ExpectedRunId) const
{
	return IsValid()
		&& !IsEmpty()
		&& ExpectedRunId.IsValid()
		&& RunId == ExpectedRunId;
}

void Fdemo_mapShanmenControlledWeaponWorldThreatSampler::Reset()
{
	RunId.Invalidate();
	ItemInstanceId.Invalidate();
	TimelineId.Invalidate();
	NextScheduledTick = 0;
	CommittedSampleCount = 0;
}
