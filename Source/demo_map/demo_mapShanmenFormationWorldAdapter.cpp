#include "demo_mapShanmenFormationWorldAdapter.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FGuid MakePlacementId(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent)
	{
		if (!Intent.RunId.IsValid() || !Intent.OwnerId.IsValid()
			|| !Intent.DeploymentId.IsValid()
			|| Intent.AnchorDefinitionId.IsNone()
			|| !Intent.AnchorInstanceId.IsValid()
			|| !IsFiniteVector(Intent.WorldLocation)
			|| !Intent.AttemptId.IsValid()
			|| !Intent.FulfillmentId.IsValid()
			|| !Intent.DeploymentReceiptId.IsValid()
			|| Intent.AuthorityRevision < 0
			|| !Intent.Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacement.r1"),
			{
				GuidDigits(Intent.RunId),
				GuidDigits(Intent.OwnerId),
				GuidDigits(Intent.DeploymentId),
				Intent.AnchorDefinitionId.ToString(),
				GuidDigits(Intent.AnchorInstanceId),
				GuidDigits(Intent.AttemptId),
				GuidDigits(Intent.FulfillmentId),
				GuidDigits(Intent.DeploymentReceiptId),
				FString::FromInt(Intent.AuthorityRevision),
				Intent.Content.Version.ToString(),
				Intent.Content.Digest
			});
	}

	FGuid MakePlacementReceiptId(
		const FGuid& PlacementId,
		const FString& ActorClassPath)
	{
		if (!PlacementId.IsValid() || ActorClassPath.IsEmpty())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacementReceipt.r1"),
			{ GuidDigits(PlacementId), ActorClassPath });
	}

	FGuid MakeTeardownReceiptId(
		const FGuid& DeploymentId,
		const Edemo_mapShanmenFormationSessionState TerminalState,
		const int32 CommittedAnchorCount)
	{
		if (!DeploymentId.IsValid()
			|| (TerminalState
					!= Edemo_mapShanmenFormationSessionState::Cancelled
				&& TerminalState
					!= Edemo_mapShanmenFormationSessionState::Ended)
			|| CommittedAnchorCount < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldTeardown.r1"),
			{
				GuidDigits(DeploymentId),
				FString::FromInt(static_cast<int32>(TerminalState)),
				FString::FromInt(CommittedAnchorCount)
			});
	}

	bool IntentsMatch(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Left,
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.PlacementId == Right.PlacementId
			&& Left.RunId == Right.RunId
			&& Left.OwnerId == Right.OwnerId
			&& Left.DeploymentId == Right.DeploymentId
			&& Left.AnchorDefinitionId == Right.AnchorDefinitionId
			&& Left.AnchorInstanceId == Right.AnchorInstanceId
			&& Left.WorldLocation.Equals(Right.WorldLocation, KINDA_SMALL_NUMBER)
			&& Left.AttemptId == Right.AttemptId
			&& Left.FulfillmentId == Right.FulfillmentId
			&& Left.DeploymentReceiptId == Right.DeploymentReceiptId
			&& Left.AuthorityRevision == Right.AuthorityRevision
			&& Left.Content.Version == Right.Content.Version
			&& Left.Content.Digest == Right.Content.Digest;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakePlacementReceipt(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent,
		const FString& ActorClassPath)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Receipt.Intent = Intent;
		Receipt.ActorClassPath = ActorClassPath;
		Receipt.PlacementTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
				Intent.PlacementId);
		Receipt.DeploymentTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Intent.DeploymentId);
		Receipt.ReceiptId = MakePlacementReceiptId(
			Intent.PlacementId, ActorClassPath);
		return Receipt;
	}

	TArray<AActor*> FindTaggedActors(UWorld& World, const FName Tag)
	{
		TArray<AActor*> Actors;
		if (Tag.IsNone())
		{
			return Actors;
		}
		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			AActor* Actor = *It;
			if (IsValid(Actor) && !Actor->IsActorBeingDestroyed()
				&& Actor->ActorHasTag(Tag))
			{
				Actors.Add(Actor);
			}
		}
		return Actors;
	}

	bool ActorMatches(
		const AActor& Actor,
		UClass& ExpectedClass,
		const Fdemo_mapShanmenFormationAnchorPlacementReceipt& Receipt)
	{
		return Receipt.IsValid()
			&& Actor.GetClass() == &ExpectedClass
			&& Actor.GetActorLocation().Equals(
				Receipt.Intent.WorldLocation, KINDA_SMALL_NUMBER)
			&& Actor.ActorHasTag(Receipt.PlacementTag)
			&& Actor.ActorHasTag(Receipt.DeploymentTag);
	}

	Fdemo_mapShanmenFormationWorldResult Reject(
		const Edemo_mapShanmenFormationWorldStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenFormationAnchorPlacementIntent* Intent = nullptr)
	{
		Fdemo_mapShanmenFormationWorldResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		if (Intent)
		{
			Result.Intent = *Intent;
		}
		return Result;
	}
}

bool Fdemo_mapShanmenFormationAnchorPlacementIntent::IsValid() const
{
	return PlacementId.IsValid()
		&& PlacementId == MakePlacementId(*this);
}

bool Fdemo_mapShanmenFormationAnchorPlacementReceipt::IsValid() const
{
	return ReceiptId.IsValid() && Intent.IsValid()
		&& !ActorClassPath.IsEmpty()
		&& PlacementTag
			== Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
				Intent.PlacementId)
		&& DeploymentTag
			== Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Intent.DeploymentId)
		&& ReceiptId
			== MakePlacementReceiptId(Intent.PlacementId, ActorClassPath);
}

bool Fdemo_mapShanmenFormationWorldTeardownReceipt::IsValid() const
{
	return ReceiptId.IsValid() && DeploymentId.IsValid()
		&& CommittedAnchorCount >= 0 && RemovedActorCount >= 0
		&& ReceiptId == MakeTeardownReceiptId(
			DeploymentId, TerminalState, CommittedAnchorCount);
}

bool Fdemo_mapShanmenFormationWorldResult::IsPlacementSuccess() const
{
	return (Status == Edemo_mapShanmenFormationWorldStatus::Placed
			|| Status == Edemo_mapShanmenFormationWorldStatus::Replayed
			|| Status == Edemo_mapShanmenFormationWorldStatus::Adopted)
		&& Intent.IsValid() && PlacementReceipt.IsValid();
}

bool Fdemo_mapShanmenFormationWorldResult::IsTeardownSuccess() const
{
	return (Status
			== Edemo_mapShanmenFormationWorldStatus::TeardownComplete
			|| Status
				== Edemo_mapShanmenFormationWorldStatus::TeardownReplayed)
		&& TeardownReceipt.IsValid();
}

bool Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
	const Fdemo_mapShanmenFormationProductSession& Session,
	const FName AnchorDefinitionId,
	Fdemo_mapShanmenFormationAnchorPlacementIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenFormationAnchorPlacementIntent();
	if (!Session.IsValid() || AnchorDefinitionId.IsNone())
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAnchorAudit* Audit =
		Session.GetAnchorAudits().FindByPredicate(
			[AnchorDefinitionId](
				const Fdemo_mapShanmenFormationAnchorAudit& Candidate)
			{
				return Candidate.AnchorDefinitionId == AnchorDefinitionId;
			});
	const FShanmenFormationAnchorProgress* Progress =
		Session.GetDeployment().GetAnchors().FindByPredicate(
			[AnchorDefinitionId](
				const FShanmenFormationAnchorProgress& Candidate)
			{
				return Candidate.GetAnchorDefinitionId()
					== AnchorDefinitionId;
			});
	if (!Audit || !Audit->IsValid() || !Progress
		|| !Progress->IsCommitted())
	{
		return false;
	}

	OutIntent.RunId = Session.GetCorrelation().ActiveRunId;
	OutIntent.OwnerId = Session.GetCorrelation().OwnerId;
	OutIntent.DeploymentId = Session.GetDeployment().GetDeploymentId();
	OutIntent.AnchorDefinitionId = AnchorDefinitionId;
	OutIntent.AnchorInstanceId = Progress->GetAnchorInstanceId();
	OutIntent.WorldLocation = Progress->GetWorldLocation();
	OutIntent.AttemptId = Audit->AttemptId;
	OutIntent.FulfillmentId = Audit->Material.Evidence.FulfillmentId;
	OutIntent.DeploymentReceiptId = Audit->DeploymentReceipt.GetReceiptId();
	OutIntent.AuthorityRevision = Audit->Material.Evidence.AuthorityRevision;
	OutIntent.Content = Audit->Material.Evidence.Content;
	OutIntent.PlacementId = MakePlacementId(OutIntent);
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenFormationAnchorPlacementIntent();
		return false;
	}
	return true;
}

FName Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
	const FGuid& PlacementId)
{
	return PlacementId.IsValid()
		? FName(*FString::Printf(
			TEXT("Shanmen.Formation.Placement.%s"),
			*GuidDigits(PlacementId)))
		: NAME_None;
}

FName Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
	const FGuid& DeploymentId)
{
	return DeploymentId.IsValid()
		? FName(*FString::Printf(
			TEXT("Shanmen.Formation.Deployment.%s"),
			*GuidDigits(DeploymentId)))
		: NAME_None;
}

Fdemo_mapShanmenFormationWorldResult
Fdemo_mapShanmenFormationWorldAdapter::TryPlaceCommittedAnchor(
	UWorld* World,
	TSubclassOf<AActor> ActorClass,
	const Fdemo_mapShanmenFormationProductSession& Session,
	const FName AnchorDefinitionId)
{
	if (!Session.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::SessionInvalid,
			TEXT("World placement requires one valid formation product session."));
	}
	if (Session.IsTerminal())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::SessionTerminal,
			TEXT("A terminal formation cannot publish new world placement."));
	}
	if (AnchorDefinitionId.IsNone())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::IntentInvalid,
			TEXT("World placement requires one explicit anchor identity."));
	}
	const bool bCommitted = Session.GetAnchorAudits().ContainsByPredicate(
		[AnchorDefinitionId](
			const Fdemo_mapShanmenFormationAnchorAudit& Candidate)
		{
			return Candidate.AnchorDefinitionId == AnchorDefinitionId;
		});
	if (!bCommitted)
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::AnchorNotCommitted,
			TEXT("Only an anchor with an immutable P8.2 commit audit may enter the World."));
	}
	Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
	if (!BuildPlacementIntent(Session, AnchorDefinitionId, Intent))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::IntentInvalid,
			TEXT("Committed anchor evidence could not produce a canonical placement intent."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::WorldInvalid,
			TEXT("Formation placement requires one live World."), &Intent);
	}
	UClass* RawActorClass = ActorClass.Get();
	if (!RawActorClass
		|| RawActorClass->HasAnyClassFlags(
			CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::ActorClassInvalid,
			TEXT("Formation placement requires one concrete Actor class."),
			&Intent);
	}
	if (!IsValid() || bTeardownComplete
		|| (BoundDeploymentId.IsValid()
			&& BoundDeploymentId != Intent.DeploymentId)
		|| (BoundWorld.IsValid() && BoundWorld.Get() != World))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::AdapterConflict,
			TEXT("One transient adapter cannot cross World or deployment ownership."),
			&Intent);
	}

	const FString ActorClassPath = RawActorClass->GetPathName();
	const Fdemo_mapShanmenFormationAnchorPlacementReceipt ExpectedReceipt =
		MakePlacementReceipt(Intent, ActorClassPath);
	if (!ExpectedReceipt.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::IntentInvalid,
			TEXT("The placement receipt failed deterministic self-validation."),
			&Intent);
	}

	FPlacementRecord* Existing = FindRecord(Intent.PlacementId);
	if (Existing
		&& (!IntentsMatch(Existing->Intent, Intent)
			|| Existing->Receipt.ActorClassPath != ActorClassPath))
	{
		return Reject(
			Existing->Receipt.ActorClassPath != ActorClassPath
				? Edemo_mapShanmenFormationWorldStatus::ActorClassConflict
				: Edemo_mapShanmenFormationWorldStatus::PlacementConflict,
			TEXT("The placement identity already belongs to different immutable evidence."),
			&Intent);
	}

	const TArray<AActor*> TaggedActors = FindTaggedActors(
		*World, ExpectedReceipt.PlacementTag);
	if (TaggedActors.Num() > 1)
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::DuplicatePlacementActors,
			TEXT("Multiple live Actors claim one deterministic formation placement."),
			&Intent);
	}
	if (TaggedActors.Num() == 1)
	{
		AActor* TaggedActor = TaggedActors[0];
		if (TaggedActor->GetClass() != RawActorClass)
		{
			return Reject(
				Edemo_mapShanmenFormationWorldStatus::ActorClassConflict,
				TEXT("The existing placement tag belongs to another Actor class."),
				&Intent);
		}
		if (!ActorMatches(*TaggedActor, *RawActorClass, ExpectedReceipt))
		{
			return Reject(
				Edemo_mapShanmenFormationWorldStatus::PlacementConflict,
				TEXT("The tagged Actor drifted from its canonical location or deployment."),
				&Intent);
		}
		if (Existing && Existing->bRemoved)
		{
			return Reject(
				Edemo_mapShanmenFormationWorldStatus::PlacementConflict,
				TEXT("A removed placement identity cannot be resurrected."),
				&Intent);
		}
		const bool bExactReplay = Existing
			&& Existing->Actor.Get() == TaggedActor;
		if (!Existing)
		{
			FPlacementRecord& Adopted = Placements.AddDefaulted_GetRef();
			Adopted.Intent = Intent;
			Adopted.Receipt = ExpectedReceipt;
			Adopted.ActorClass = ActorClass;
			Adopted.Actor = TaggedActor;
		}
		else
		{
			Existing->Actor = TaggedActor;
		}
		BoundDeploymentId = Intent.DeploymentId;
		BoundWorld = World;
		Fdemo_mapShanmenFormationWorldResult Result;
		Result.Status = bExactReplay
			? Edemo_mapShanmenFormationWorldStatus::Replayed
			: Edemo_mapShanmenFormationWorldStatus::Adopted;
		Result.Diagnostic = bExactReplay
			? TEXT("The exact live placement replayed without spawning.")
			: TEXT("The deterministic tagged Actor was adopted after adapter reconstruction.");
		Result.Intent = Intent;
		Result.PlacementReceipt = ExpectedReceipt;
		Result.Actor = TaggedActor;
		return Result;
	}

	if (Existing)
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::ActorUnavailable,
			TEXT("The recorded Actor is unavailable; exact replay will not silently duplicate it."),
			&Intent);
	}

	FActorSpawnParameters Parameters;
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Spawned = World->SpawnActor<AActor>(
		RawActorClass,
		FTransform(FRotator::ZeroRotator, Intent.WorldLocation),
		Parameters);
	if (!::IsValid(Spawned))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::SpawnRejected,
			TEXT("The World rejected the exact committed anchor spawn."),
			&Intent);
	}
	Spawned->Tags.AddUnique(ExpectedReceipt.PlacementTag);
	Spawned->Tags.AddUnique(ExpectedReceipt.DeploymentTag);
	if (!ActorMatches(*Spawned, *RawActorClass, ExpectedReceipt))
	{
		World->DestroyActor(Spawned, true, true);
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::SpawnRejected,
			TEXT("Spawn postconditions failed; the transient Actor was removed."),
			&Intent);
	}

	FPlacementRecord& Record = Placements.AddDefaulted_GetRef();
	Record.Intent = Intent;
	Record.Receipt = ExpectedReceipt;
	Record.ActorClass = ActorClass;
	Record.Actor = Spawned;
	BoundDeploymentId = Intent.DeploymentId;
	BoundWorld = World;

	Fdemo_mapShanmenFormationWorldResult Result;
	Result.Status = Edemo_mapShanmenFormationWorldStatus::Placed;
	Result.Diagnostic =
		TEXT("One committed formation anchor was projected into the World.");
	Result.Intent = Intent;
	Result.PlacementReceipt = ExpectedReceipt;
	Result.Actor = Spawned;
	return Result;
}

Fdemo_mapShanmenFormationWorldResult
Fdemo_mapShanmenFormationWorldAdapter::TryTeardownTerminal(
	UWorld* World,
	const Fdemo_mapShanmenFormationProductSession& Session)
{
	if (!Session.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::SessionInvalid,
			TEXT("World teardown requires one valid formation product session."));
	}
	if (!Session.IsTerminal())
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::TerminalRequired,
			TEXT("Formation Actors remain live until the product session is terminal."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::WorldInvalid,
			TEXT("Formation teardown requires the owning live World."));
	}
	const FGuid DeploymentId = Session.GetDeployment().GetDeploymentId();
	if (!IsValid()
		|| (BoundDeploymentId.IsValid()
			&& BoundDeploymentId != DeploymentId)
		|| (BoundWorld.IsValid() && BoundWorld.Get() != World))
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::AdapterConflict,
			TEXT("Terminal teardown cannot cross World or deployment ownership."));
	}
	if (bTeardownComplete)
	{
		Fdemo_mapShanmenFormationWorldResult Result;
		Result.Status =
			Edemo_mapShanmenFormationWorldStatus::TeardownReplayed;
		Result.Diagnostic =
			TEXT("The exact terminal teardown replayed its stable receipt.");
		Result.TeardownReceipt = TeardownReceipt;
		return Result;
	}

	BoundDeploymentId = DeploymentId;
	BoundWorld = World;
	TSet<AActor*> ActorsToRemove;
	for (FPlacementRecord& Record : Placements)
	{
		if (!Record.bRemoved && ::IsValid(Record.Actor.Get())
			&& Record.Actor->GetWorld() == World)
		{
			ActorsToRemove.Add(Record.Actor.Get());
		}
	}
	for (AActor* Tagged : FindTaggedActors(
		*World, MakeDeploymentTag(DeploymentId)))
	{
		ActorsToRemove.Add(Tagged);
	}

	int32 RemovedActorCount = 0;
	bool bRecoveryRequired = false;
	for (AActor* Actor : ActorsToRemove)
	{
		if (!::IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			continue;
		}
		if (World->DestroyActor(Actor, true, true)
			|| Actor->IsActorBeingDestroyed())
		{
			++RemovedActorCount;
		}
		else
		{
			bRecoveryRequired = true;
		}
	}
	TeardownRemovedActorCount += RemovedActorCount;
	for (FPlacementRecord& Record : Placements)
	{
		AActor* Actor = Record.Actor.Get();
		Record.bRemoved = !::IsValid(Actor)
			|| Actor->IsActorBeingDestroyed();
	}
	if (!FindTaggedActors(*World, MakeDeploymentTag(DeploymentId)).IsEmpty())
	{
		bRecoveryRequired = true;
	}
	if (bRecoveryRequired)
	{
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::TeardownRecoveryRequired,
			TEXT("Some formation Actors remain; replay terminal teardown to continue cleanup."));
	}

	for (FPlacementRecord& Record : Placements)
	{
		Record.bRemoved = true;
	}
	TeardownReceipt.DeploymentId = DeploymentId;
	TeardownReceipt.TerminalState = Session.GetState();
	TeardownReceipt.CommittedAnchorCount =
		Session.GetAnchorAudits().Num();
	TeardownReceipt.RemovedActorCount = TeardownRemovedActorCount;
	TeardownReceipt.ReceiptId = MakeTeardownReceiptId(
		DeploymentId,
		TeardownReceipt.TerminalState,
		TeardownReceipt.CommittedAnchorCount);
	if (!TeardownReceipt.IsValid())
	{
		TeardownReceipt = Fdemo_mapShanmenFormationWorldTeardownReceipt();
		return Reject(
			Edemo_mapShanmenFormationWorldStatus::TeardownRecoveryRequired,
			TEXT("Terminal cleanup completed but its deterministic receipt was invalid."));
	}
	bTeardownComplete = true;

	Fdemo_mapShanmenFormationWorldResult Result;
	Result.Status = Edemo_mapShanmenFormationWorldStatus::TeardownComplete;
	Result.Diagnostic =
		TEXT("All Actors for the terminal formation deployment were removed.");
	Result.TeardownReceipt = TeardownReceipt;
	return Result;
}

bool Fdemo_mapShanmenFormationWorldAdapter::IsValid() const
{
	if (Placements.IsEmpty() && !bTeardownComplete)
	{
		const bool bUnbound = !BoundDeploymentId.IsValid()
			&& !BoundWorld.IsValid();
		const bool bBoundForTeardownRetry = BoundDeploymentId.IsValid()
			&& BoundWorld.IsValid();
		return TeardownRemovedActorCount >= 0
			&& !TeardownReceipt.IsValid()
			&& (bUnbound || bBoundForTeardownRetry);
	}
	if (!BoundDeploymentId.IsValid() || !BoundWorld.IsValid()
		|| TeardownRemovedActorCount < 0)
	{
		return false;
	}
	TSet<FGuid> PlacementIds;
	for (const FPlacementRecord& Record : Placements)
	{
		if (!Record.Intent.IsValid() || !Record.Receipt.IsValid()
			|| Record.Intent.DeploymentId != BoundDeploymentId
			|| !Record.ActorClass
			|| Record.Receipt.ActorClassPath
				!= Record.ActorClass->GetPathName()
			|| PlacementIds.Contains(Record.Intent.PlacementId)
			|| (bTeardownComplete && !Record.bRemoved))
		{
			return false;
		}
		PlacementIds.Add(Record.Intent.PlacementId);
	}
	return bTeardownComplete
		? TeardownReceipt.IsValid()
			&& TeardownReceipt.DeploymentId == BoundDeploymentId
		: !TeardownReceipt.IsValid();
}

const Fdemo_mapShanmenFormationAnchorPlacementReceipt*
Fdemo_mapShanmenFormationWorldAdapter::FindReceipt(
	const FGuid& PlacementId) const
{
	const FPlacementRecord* Record = Placements.FindByPredicate(
		[&PlacementId](const FPlacementRecord& Candidate)
		{
			return Candidate.Intent.PlacementId == PlacementId;
		});
	return Record ? &Record->Receipt : nullptr;
}

Fdemo_mapShanmenFormationWorldAdapter::FPlacementRecord*
Fdemo_mapShanmenFormationWorldAdapter::FindRecord(
	const FGuid& PlacementId)
{
	return Placements.FindByPredicate(
		[&PlacementId](FPlacementRecord& Candidate)
		{
			return Candidate.Intent.PlacementId == PlacementId;
		});
}
