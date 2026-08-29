#include "demo_mapShanmenControlledWeaponRunHost.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"

namespace
{
	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return Left.ToString(EGuidFormats::Digits)
			< Right.ToString(EGuidFormats::Digits);
	}
}

bool Fdemo_mapShanmenControlledWeaponHostMovementBatch::
IsFullyAdvanced() const
{
	if (AttemptedCount <= 0
		|| AdvancedCount != AttemptedCount
		|| Entries.Num() != AttemptedCount)
	{
		return false;
	}
	for (const Fdemo_mapShanmenControlledWeaponHostMovementEntry& Entry :
		Entries)
	{
		if (!Entry.IsSuccessful())
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponHostOrbitBatch::IsFullyAdvanced() const
{
	if (AttemptedCount <= 0
		|| AdvancedCount != AttemptedCount
		|| Entries.Num() != AttemptedCount)
	{
		return false;
	}
	for (const Fdemo_mapShanmenControlledWeaponHostOrbitEntry& Entry :
		Entries)
	{
		if (!Entry.IsSuccessful())
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponOrbitFrameResult::IsValid() const
{
	const bool bFinitePositiveDelta =
		FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0.0f;
	const bool bEmptyBatch = Batch.AttemptedCount == 0
		&& Batch.AdvancedCount == 0
		&& Batch.Entries.IsEmpty();
	switch (Status)
	{
	case Edemo_mapShanmenControlledWeaponOrbitFrameStatus::NoOrbitingItems:
		return bFinitePositiveDelta
			&& OrbitingCount == 0
			&& bEmptyBatch
			&& ((BoundCount == 0 && !RunId.IsValid())
				|| (BoundCount > 0 && RunId.IsValid()));
	case Edemo_mapShanmenControlledWeaponOrbitFrameStatus::Advanced:
		return bFinitePositiveDelta
			&& RunId.IsValid()
			&& BoundCount >= OrbitingCount
			&& OrbitingCount > 0
			&& Batch.AttemptedCount == OrbitingCount
			&& Batch.IsFullyAdvanced();
	case Edemo_mapShanmenControlledWeaponOrbitFrameStatus::DeltaInvalid:
		return !bFinitePositiveDelta
			&& BoundCount >= 0
			&& OrbitingCount == 0
			&& bEmptyBatch;
	case Edemo_mapShanmenControlledWeaponOrbitFrameStatus::HostInvalid:
		return bFinitePositiveDelta
			&& BoundCount > 0
			&& OrbitingCount == 0
			&& bEmptyBatch;
	case Edemo_mapShanmenControlledWeaponOrbitFrameStatus::MovementRejected:
		if (!bFinitePositiveDelta
			|| !RunId.IsValid()
			|| BoundCount < OrbitingCount
			|| OrbitingCount <= 0
			|| Batch.IsFullyAdvanced())
		{
			return false;
		}
		return bEmptyBatch
			|| (Batch.AttemptedCount == OrbitingCount
				&& Batch.Entries.Num() == OrbitingCount
				&& Batch.AdvancedCount >= 0
				&& Batch.AdvancedCount < OrbitingCount);
	default:
		return false;
	}
}

Fdemo_mapShanmenControlledWeaponHostAttachResult
Fdemo_mapShanmenControlledWeaponRunHost::TryAttach(
	const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	AActor* RequestedSourceActor,
	AActor* RequestedWeaponActor,
	UPrimitiveComponent* RequestedWeaponCollisionRoot,
	const Fdemo_mapShanmenControlledWeaponMotionCapture& Motion)
{
	Fdemo_mapShanmenControlledWeaponHostAttachResult Result;
	if (!Coordinator.IsReady())
	{
		return Result;
	}
	if (!Prepared.IsPrepared())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::PreparedInvalid;
		return Result;
	}

	Result.ItemInstanceId = Prepared.Evidence.ItemInstanceId;
	Result.ActivationId = Prepared.Action.GetActivationId();
	if (!IsEmpty()
		&& !BindingMatches(Coordinator, RequestedSourceActor, Prepared))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::HostBindingMismatch;
		return Result;
	}
	if (Controllers.Contains(Result.ItemInstanceId))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::ItemAlreadyBound;
		return Result;
	}
	for (const TPair<FGuid,
		Fdemo_mapShanmenControlledWeaponProductController>& Pair : Controllers)
	{
		if (Pair.Value.GetSession().GetActionRuntime().GetAction()
				.GetActivationId() == Result.ActivationId)
		{
			Result.Error =
				Edemo_mapShanmenControlledWeaponHostAttachError::ActivationAlreadyBound;
			return Result;
		}
	}
	if (HasWeaponBinding(
		RequestedWeaponActor, RequestedWeaponCollisionRoot))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::WeaponActorAlreadyBound;
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponProductController Controller;
	Result.Product =
		Fdemo_mapShanmenControlledWeaponProductController::TryStart(
			Prepared,
			Coordinator,
			RequestedSourceActor,
			RequestedWeaponActor,
			RequestedWeaponCollisionRoot,
			Motion,
			Controller);
	if (!Result.Product.IsStarted())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::ProductStartRejected;
		return Result;
	}

	Fdemo_mapShanmenControlledWeaponRunHost Candidate = *this;
	if (Candidate.IsEmpty())
	{
		Candidate.RunId = Coordinator.GetRunId();
		Candidate.SourceEntityId = Coordinator.GetPlayerEntityId();
		Candidate.SourceActor = RequestedSourceActor;
	}
	Candidate.Controllers.Add(Result.ItemInstanceId, MoveTemp(Controller));
	if (!Candidate.IsValid())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::ProductStartRejected;
		return Result;
	}

	*this = MoveTemp(Candidate);
	Result.Error = Edemo_mapShanmenControlledWeaponHostAttachError::None;
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::IsValid() const
{
	AActor* BoundSourceActor = SourceActor.Get();
	if (Controllers.IsEmpty()
		|| !RunId.IsValid()
		|| !SourceEntityId.IsValid()
		|| !BoundSourceActor)
	{
		return false;
	}

	TSet<const AActor*> WeaponActors;
	TSet<const UPrimitiveComponent*> CollisionRoots;
	TSet<FGuid> ActivationIds;
	for (const TPair<FGuid,
		Fdemo_mapShanmenControlledWeaponProductController>& Pair : Controllers)
	{
		const FGuid& ItemInstanceId = Pair.Key;
		const Fdemo_mapShanmenControlledWeaponProductController& Controller =
			Pair.Value;
		const FShanmenCombatActionSnapshot& Action =
			Controller.GetSession().GetActionRuntime().GetAction();
		AActor* Weapon = Controller.GetWeaponActor();
		UPrimitiveComponent* Collision =
			Controller.GetWeaponCollisionRoot();
		if (!ItemInstanceId.IsValid()
			|| !Controller.IsValid()
			|| Controller.GetSession().GetEvidence().ItemInstanceId
				!= ItemInstanceId
			|| Action.GetSourceItemInstanceId() != ItemInstanceId
			|| Action.GetRunId() != RunId
			|| Action.GetSourceEntityId() != SourceEntityId
			|| Controller.GetSourceActor() != BoundSourceActor
			|| !Weapon
			|| !Collision
			|| WeaponActors.Contains(Weapon)
			|| CollisionRoots.Contains(Collision)
			|| ActivationIds.Contains(Action.GetActivationId()))
		{
			return false;
		}
		WeaponActors.Add(Weapon);
		CollisionRoots.Add(Collision);
		ActivationIds.Add(Action.GetActivationId());
	}
	return true;
}

int32 Fdemo_mapShanmenControlledWeaponRunHost::NumActive() const
{
	if (!IsValid())
	{
		return 0;
	}
	int32 Result = 0;
	for (const TPair<FGuid,
		Fdemo_mapShanmenControlledWeaponProductController>& Pair : Controllers)
	{
		Result += Pair.Value.IsActive() ? 1 : 0;
	}
	return Result;
}

int32 Fdemo_mapShanmenControlledWeaponRunHost::NumOrbiting() const
{
	if (!IsValid())
	{
		return 0;
	}
	int32 Result = 0;
	for (const TPair<FGuid,
		Fdemo_mapShanmenControlledWeaponProductController>& Pair : Controllers)
	{
		Result += Pair.Value.IsOrbiting() ? 1 : 0;
	}
	return Result;
}

TArray<FGuid>
Fdemo_mapShanmenControlledWeaponRunHost::GetOrderedItemInstanceIds() const
{
	TArray<FGuid> Result;
	Controllers.GetKeys(Result);
	Result.Sort(GuidLess);
	return Result;
}

const Fdemo_mapShanmenControlledWeaponProductController*
Fdemo_mapShanmenControlledWeaponRunHost::FindController(
	const FGuid& ItemInstanceId) const
{
	return ItemInstanceId.IsValid()
		? Controllers.Find(ItemInstanceId)
		: nullptr;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryLaunch(
	const FGuid& ItemInstanceId,
	int64 ExpectedSequence,
	const FVector& DesiredDirection,
	FShanmenControlledWeaponCommandReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponCommandReceipt();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller
		&& Controller->TryLaunch(
			ExpectedSequence, DesiredDirection, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryRedirect(
	const FGuid& ItemInstanceId,
	int64 ExpectedSequence,
	const FVector& DesiredDirection,
	FShanmenControlledWeaponCommandReceipt& OutReceipt)
{
	OutReceipt = FShanmenControlledWeaponCommandReceipt();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller
		&& Controller->TryRedirect(
			ExpectedSequence, DesiredDirection, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryAdvanceOrbitingInOrder(
	float DeltaSeconds,
	Fdemo_mapShanmenControlledWeaponHostOrbitBatch& OutBatch)
{
	OutBatch = Fdemo_mapShanmenControlledWeaponHostOrbitBatch();
	if (!IsValid()
		|| !FMath::IsFinite(DeltaSeconds)
		|| DeltaSeconds <= 0.0f)
	{
		return false;
	}

	const TArray<FGuid> OrderedActive =
		GetOrderedActiveItemInstanceIds();
	TArray<FGuid> OrderedOrbiting;
	for (const FGuid& ItemInstanceId : OrderedActive)
	{
		const Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Controllers.Find(ItemInstanceId);
		if (Controller && Controller->IsOrbiting())
		{
			if (DeltaSeconds > Controller->GetMotion().MaximumStepSeconds)
			{
				return false;
			}
			OrderedOrbiting.Add(ItemInstanceId);
		}
	}
	if (OrderedOrbiting.IsEmpty())
	{
		return false;
	}

	OutBatch.AttemptedCount = OrderedOrbiting.Num();
	OutBatch.Entries.Reserve(OrderedOrbiting.Num());
	for (const FGuid& ItemInstanceId : OrderedOrbiting)
	{
		Fdemo_mapShanmenControlledWeaponHostOrbitEntry Entry;
		Entry.ItemInstanceId = ItemInstanceId;
		Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Controllers.Find(ItemInstanceId);
		Entry.bAdvanced = Controller
			&& Controller->TryAdvanceOrbiting(
				DeltaSeconds, Entry.Movement);
		OutBatch.AdvancedCount += Entry.bAdvanced ? 1 : 0;
		OutBatch.Entries.Add(MoveTemp(Entry));
	}
	return OutBatch.IsFullyAdvanced();
}

Fdemo_mapShanmenControlledWeaponOrbitFrameResult
Fdemo_mapShanmenControlledWeaponRunHost::AdvanceOrbitingFrame(
	float DeltaSeconds)
{
	Fdemo_mapShanmenControlledWeaponOrbitFrameResult Result;
	Result.DeltaSeconds = DeltaSeconds;
	if (!IsEmpty())
	{
		Result.RunId = RunId;
		Result.BoundCount = NumBound();
	}
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponOrbitFrameStatus::DeltaInvalid;
		return Result;
	}
	if (IsEmpty())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponOrbitFrameStatus::NoOrbitingItems;
		return Result;
	}
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponOrbitFrameStatus::HostInvalid;
		return Result;
	}

	Result.OrbitingCount = NumOrbiting();
	if (Result.OrbitingCount == 0)
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponOrbitFrameStatus::NoOrbitingItems;
		return Result;
	}
	Result.Status = TryAdvanceOrbitingInOrder(
		DeltaSeconds, Result.Batch)
		? Edemo_mapShanmenControlledWeaponOrbitFrameStatus::Advanced
		: Edemo_mapShanmenControlledWeaponOrbitFrameStatus::MovementRejected;
	return Result;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryBeginOrbitThreatWindow(
	const FGuid& ItemInstanceId,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller
		&& Controller->TryBeginOrbitThreatWindow(OutContext);
}

Fdemo_mapShanmenControlledWeaponOrbitThreatResult
Fdemo_mapShanmenControlledWeaponRunHost::ProjectOrbitThreatOverlap(
	const FGuid& ItemInstanceId,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal)
{
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() && CoordinatorMatches(Coordinator)
			? Controllers.Find(ItemInstanceId)
			: nullptr;
	return Controller
		? Controller->ProjectOrbitThreatOverlap(
			Coordinator, Overlap, ContactLocation, ContactNormal)
		: Fdemo_mapShanmenControlledWeaponOrbitThreatResult();
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryEndOrbitThreatWindow(
	const FGuid& ItemInstanceId,
	FShanmenDetectorEmissionReceipt& OutReceipt)
{
	OutReceipt = FShanmenDetectorEmissionReceipt();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller && Controller->TryEndOrbitThreatWindow(OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryEndOrbitThreatWindow(
	const FGuid& ItemInstanceId)
{
	FShanmenDetectorEmissionReceipt Ignored;
	return TryEndOrbitThreatWindow(ItemInstanceId, Ignored);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryEvaluateOrbitThreatReceipt(
	const FGuid& ItemInstanceId,
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<FShanmenControlledWeaponThreatTargetEvidence>& TargetEvidence,
	FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller
		&& Controller->TryEvaluateOrbitThreatReceipt(
			Emission, TargetEvidence, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryEvaluateOrbitThreatActors(
	const FGuid& ItemInstanceId,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<AActor*>& TargetActors,
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult& OutEvidence,
	FShanmenControlledWeaponThreatPolicyReceipt& OutReceipt) const
{
	OutEvidence =
		Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult();
	OutReceipt = FShanmenControlledWeaponThreatPolicyReceipt();
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() && CoordinatorMatches(Coordinator)
			? Controllers.Find(ItemInstanceId)
			: nullptr;
	return Controller
		&& Controller->TryEvaluateOrbitThreatActors(
			Coordinator,
			Emission,
			TargetActors,
			OutEvidence,
			OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::
TryBuildOrbitThreatPresenceIntents(
	const FGuid& ItemInstanceId,
	const FShanmenControlledWeaponThreatPolicyReceipt& Policy,
	FShanmenControlledWeaponThreatPresenceReceipt& OutReceipt) const
{
	OutReceipt = FShanmenControlledWeaponThreatPresenceReceipt();
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller
		&& Controller->TryBuildOrbitThreatPresenceIntents(
			Policy, OutReceipt);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryAdvanceDirectedInOrder(
	float DeltaSeconds,
	Fdemo_mapShanmenControlledWeaponHostMovementBatch& OutBatch)
{
	OutBatch = Fdemo_mapShanmenControlledWeaponHostMovementBatch();
	if (!IsValid()
		|| !FMath::IsFinite(DeltaSeconds)
		|| DeltaSeconds <= 0.0f)
	{
		return false;
	}

	const TArray<FGuid> OrderedActive =
		GetOrderedActiveItemInstanceIds();
	TArray<FGuid> OrderedDirected;
	for (const FGuid& ItemInstanceId : OrderedActive)
	{
		const Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Controllers.Find(ItemInstanceId);
		if (Controller && Controller->IsDirected())
		{
			if (DeltaSeconds > Controller->GetMotion().MaximumStepSeconds)
			{
				return false;
			}
			OrderedDirected.Add(ItemInstanceId);
		}
	}
	if (OrderedDirected.IsEmpty())
	{
		return false;
	}

	OutBatch.AttemptedCount = OrderedDirected.Num();
	OutBatch.Entries.Reserve(OrderedDirected.Num());
	for (const FGuid& ItemInstanceId : OrderedDirected)
	{
		Fdemo_mapShanmenControlledWeaponHostMovementEntry Entry;
		Entry.ItemInstanceId = ItemInstanceId;
		Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Controllers.Find(ItemInstanceId);
		Entry.bAdvanced = Controller
			&& Controller->TryAdvanceDirected(
				DeltaSeconds, Entry.Movement, Entry.BlockingHit);
		OutBatch.AdvancedCount += Entry.bAdvanced ? 1 : 0;
		OutBatch.Entries.Add(MoveTemp(Entry));
	}
	return OutBatch.IsFullyAdvanced();
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryBeginContactWindow(
	const FGuid& ItemInstanceId,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller && Controller->TryBeginContactWindow(OutContext);
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponRunHost::ResolveSweepContact(
	const FGuid& ItemInstanceId,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FHitResult& Hit)
{
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() && CoordinatorMatches(Coordinator)
			? Controllers.Find(ItemInstanceId)
			: nullptr;
	return Controller
		? Controller->ResolveSweepContact(Coordinator, Hit)
		: Fdemo_mapShanmenControlledWeaponWorldDeliveryResult();
}

Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
Fdemo_mapShanmenControlledWeaponRunHost::ResolveOverlapContact(
	const FGuid& ItemInstanceId,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal)
{
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() && CoordinatorMatches(Coordinator)
			? Controllers.Find(ItemInstanceId)
			: nullptr;
	return Controller
		? Controller->ResolveOverlapContact(
			Coordinator, Overlap, ContactLocation, ContactNormal)
		: Fdemo_mapShanmenControlledWeaponWorldDeliveryResult();
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryEndContactWindow(
	const FGuid& ItemInstanceId)
{
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller && Controller->TryEndContactWindow();
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryRecallAndComplete(
	const FGuid& ItemInstanceId,
	int64 ExpectedSequence,
	FShanmenControlledWeaponCommandReceipt& OutRecall,
	FShanmenActionTransitionReceipt& OutRecovery,
	FShanmenActionTransitionReceipt& OutCompleted)
{
	OutRecall = FShanmenControlledWeaponCommandReceipt();
	OutRecovery = FShanmenActionTransitionReceipt();
	OutCompleted = FShanmenActionTransitionReceipt();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller
		&& Controller->TryRecallAndComplete(
			ExpectedSequence, OutRecall, OutRecovery, OutCompleted);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryInterrupt(
	const FGuid& ItemInstanceId,
	FShanmenActionTransitionReceipt& OutInterrupted)
{
	OutInterrupted = FShanmenActionTransitionReceipt();
	Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	return Controller && Controller->TryInterrupt(OutInterrupted);
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryInterruptAll(
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt>& OutReceipts)
{
	OutReceipts.Reset();
	if (!IsValid())
	{
		return false;
	}
	const TArray<FGuid> OrderedActive =
		GetOrderedActiveItemInstanceIds();
	if (OrderedActive.IsEmpty())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponRunHost Candidate = *this;
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt> Receipts;
	Receipts.Reserve(OrderedActive.Num());
	for (const FGuid& ItemInstanceId : OrderedActive)
	{
		Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Candidate.Controllers.Find(ItemInstanceId);
		Fdemo_mapShanmenControlledWeaponHostInterruptReceipt Receipt;
		Receipt.ItemInstanceId = ItemInstanceId;
		if (!Controller
			|| !Controller->TryInterrupt(Receipt.Interrupted)
			|| !Receipt.IsValid())
		{
			return false;
		}
		Receipts.Add(MoveTemp(Receipt));
	}
	if (!Candidate.IsValid())
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	OutReceipts = MoveTemp(Receipts);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TryRemoveTerminal(
	const FGuid& ItemInstanceId)
{
	if (!IsValid())
	{
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		Controllers.Find(ItemInstanceId);
	if (!Controller || !Controller->GetSession().IsTerminal())
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponRunHost Candidate = *this;
	Candidate.Controllers.Remove(ItemInstanceId);
	if (Candidate.Controllers.IsEmpty())
	{
		Candidate.Reset();
	}
	else if (!Candidate.IsValid())
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

void Fdemo_mapShanmenControlledWeaponRunHost::Reset()
{
	*this = Fdemo_mapShanmenControlledWeaponRunHost();
}

bool Fdemo_mapShanmenControlledWeaponRunHost::CoordinatorMatches(
	const Fdemo_mapCombatRunCoordinator& Coordinator) const
{
	if (!Coordinator.IsReady()
		|| Coordinator.GetRunId() != RunId
		|| Coordinator.GetPlayerEntityId() != SourceEntityId
		|| !SourceActor.IsValid())
	{
		return false;
	}

	FGuid ResolvedSourceEntityId;
	return Coordinator.GetEntityRegistry().TryResolveObject(
		RunId,
		SourceActor.Get(),
		INDEX_NONE,
		ResolvedSourceEntityId)
		&& ResolvedSourceEntityId == SourceEntityId;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::BindingMatches(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const AActor* RequestedSourceActor,
	const Fdemo_mapShanmenControlledWeaponPrepareResult& Prepared) const
{
	return CoordinatorMatches(Coordinator)
		&& RequestedSourceActor == SourceActor.Get()
		&& Prepared.IsPrepared()
		&& Prepared.Action.GetRunId() == RunId
		&& Prepared.Action.GetSourceEntityId() == SourceEntityId;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::HasWeaponBinding(
	const AActor* RequestedWeaponActor,
	const UPrimitiveComponent* RequestedCollisionRoot) const
{
	if (!RequestedWeaponActor || !RequestedCollisionRoot)
	{
		return false;
	}
	for (const TPair<FGuid,
		Fdemo_mapShanmenControlledWeaponProductController>& Pair : Controllers)
	{
		if (Pair.Value.GetWeaponActor() == RequestedWeaponActor
			|| Pair.Value.GetWeaponCollisionRoot() == RequestedCollisionRoot)
		{
			return true;
		}
	}
	return false;
}

TArray<FGuid>
Fdemo_mapShanmenControlledWeaponRunHost::GetOrderedActiveItemInstanceIds() const
{
	TArray<FGuid> Result;
	if (!IsValid())
	{
		return Result;
	}
	for (const TPair<FGuid,
		Fdemo_mapShanmenControlledWeaponProductController>& Pair : Controllers)
	{
		if (Pair.Value.IsActive())
		{
			Result.Add(Pair.Key);
		}
	}
	Result.Sort(GuidLess);
	return Result;
}
