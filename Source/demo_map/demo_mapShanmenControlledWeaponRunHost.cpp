#include "demo_mapShanmenControlledWeaponRunHost.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"

namespace
{
	constexpr int64 MaximumDirectedTimelineTicksPerPump = 300;

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

bool Fdemo_mapShanmenControlledWeaponDirectedTimelineResult::IsSuccess()
	const
{
	if (Error
			!= Edemo_mapShanmenControlledWeaponDirectedTimelineError::None
		|| !RunId.IsValid()
		|| !TimelineId.IsValid()
		|| StartTick < 0
		|| EndTick < StartTick
		|| RequestedTickCount < 0
		|| RequestedTickCount != EndTick - StartTick
		|| RequestedTickCount > MaximumDirectedTimelineTicksPerPump
		|| MovementTickCount < 0
		|| MovementTickCount > RequestedTickCount
		|| MovementCount < MovementTickCount
		|| BlockingContactCount < 0
		|| static_cast<int64>(BlockingContactCount) > MovementCount
		|| DeliveredImpactCount < 0
		|| DeliveredImpactCount > BlockingContactCount
		|| TerminalizedCount != BlockingContactCount
		|| FallbackInterruptedCount < 0
		|| FallbackInterruptedCount > TerminalizedCount
		|| FailedItemInstanceId.IsValid()
		|| Diagnostic.IsEmpty())
	{
		return false;
	}
	return MovementTickCount > 0
		|| (MovementCount == 0
			&& BlockingContactCount == 0
			&& DeliveredImpactCount == 0
			&& TerminalizedCount == 0
			&& FallbackInterruptedCount == 0);
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

bool Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch::
IsFullyCaptured() const
{
	if (!RunId.IsValid()
		|| !SourceEntityId.IsValid()
		|| RequestedCount <= 0
		|| CapturedCount != RequestedCount
		|| Entries.Num() != RequestedCount)
	{
		return false;
	}

	FGuid PreviousItemInstanceId;
	for (const Fdemo_mapShanmenControlledWeaponDefenseReadinessEntry& Entry :
		Entries)
	{
		const FShanmenCombatActionSnapshot& Action =
			Entry.Readiness.GetRuntime().GetAction();
		if (!Entry.IsValid()
			|| Action.GetRunId() != RunId
			|| Action.GetSourceEntityId() != SourceEntityId
			|| (PreviousItemInstanceId.IsValid()
				&& !GuidLess(
					PreviousItemInstanceId, Entry.ItemInstanceId)))
		{
			return false;
		}
		PreviousItemInstanceId = Entry.ItemInstanceId;
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

bool Fdemo_mapShanmenControlledWeaponThreatFinalizationResult::
IsFinalized() const
{
	if (!ItemInstanceId.IsValid()
		|| !Evidence.IsCaptured()
		|| !Presence.IsValid()
		|| !Consumption.IsSuccess())
	{
		return false;
	}

	const FShanmenControlledWeaponThreatPolicyReceipt& Policy =
		Presence.GetPolicy();
	const FShanmenCombatActionSnapshot& Action =
		Policy.GetEmission().GetContext().GetAction();
	if (Action.GetSourceItemInstanceId() != ItemInstanceId
		|| Consumption.GetRunId() != Action.GetRunId()
		|| Evidence.ExpectedTargetCount != Policy.GetTargets().Num()
		|| Evidence.TargetEvidence.Num() != Policy.GetTargets().Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Policy.GetTargets().Num(); ++Index)
	{
		const FShanmenControlledWeaponThreatTargetEvidence& TargetEvidence =
			Evidence.TargetEvidence[Index];
		const FShanmenControlledWeaponThreatTargetReceipt& Target =
			Policy.GetTargets()[Index];
		if (TargetEvidence.GetTargetEntityId()
				!= Target.GetCandidate().TargetEntityId
			|| TargetEvidence.GetTargetTags() != Target.GetTargetTags())
		{
			return false;
		}
	}

	const TArray<FShanmenControlledWeaponThreatPresenceIntent>& Intents =
		Presence.GetIntents();
	const TArray<FShanmenControlledWeaponThreatPresenceConsumeReceipt>&
		Receipts = Consumption.GetReceipts();
	if (Consumption.GetStatus()
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp)
	{
		return Intents.IsEmpty() && Receipts.IsEmpty();
	}
	if (Receipts.Num() != Intents.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Intents.Num(); ++Index)
	{
		if (Intents[Index].GetSourceItemInstanceId() != ItemInstanceId
			|| Receipts[Index].GetIntent().GetIntentId()
				!= Intents[Index].GetIntentId()
			|| Receipts[Index].GetSourceEntityId()
				!= Action.GetSourceEntityId())
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleBatchEntry::
IsSuccessful() const
{
	return ItemInstanceId.IsValid()
		&& Finalization.IsFinalized()
		&& Finalization.GetItemInstanceId() == ItemInstanceId;
}

bool Fdemo_mapShanmenControlledWeaponThreatSampleBatch::
IsFullyFinalized() const
{
	if (!RunId.IsValid()
		|| AttemptedCount <= 0
		|| FinalizedCount != AttemptedCount
		|| Entries.Num() != AttemptedCount)
	{
		return false;
	}

	FGuid PreviousItemInstanceId;
	for (const Fdemo_mapShanmenControlledWeaponThreatSampleBatchEntry& Entry :
		Entries)
	{
		if (!Entry.IsSuccessful()
			|| Entry.Finalization.GetPresence().GetPolicy().GetEmission()
				.GetContext().GetAction().GetRunId() != RunId
			|| (PreviousItemInstanceId.IsValid()
				&& !GuidLess(PreviousItemInstanceId, Entry.ItemInstanceId)))
		{
			return false;
		}
		PreviousItemInstanceId = Entry.ItemInstanceId;
	}
	return true;
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
		if (!FShanmenControlledWeaponThreatPresenceAuthority::TryCreate(
				Candidate.RunId,
				Candidate.SourceEntityId,
				Candidate.ThreatPresenceAuthority))
		{
			Result.Error =
				Edemo_mapShanmenControlledWeaponHostAttachError::
				ProductStartRejected;
			return Result;
		}
	}
	Candidate.Controllers.Add(Result.ItemInstanceId, MoveTemp(Controller));
	if (!Candidate.ThreatPresenceAuthority.TryRegisterItemActivation(
			Result.ItemInstanceId, Result.ActivationId))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponHostAttachError::ProductStartRejected;
		return Result;
	}
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
		|| !BoundSourceActor
		|| !ThreatPresenceAuthority.IsValid()
		|| ThreatPresenceAuthority.GetRunId() != RunId
		|| ThreatPresenceAuthority.GetSourceEntityId() != SourceEntityId
		|| ThreatPresenceAuthority.NumRegisteredItemActivations()
			!= Controllers.Num())
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
			|| ThreatPresenceAuthority.GetRegisteredActivationId(
				ItemInstanceId) != Action.GetActivationId()
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

bool Fdemo_mapShanmenControlledWeaponRunHost::
TryCaptureOrbitDefenseReadinessInOrder(
	const TArray<FGuid>& ItemInstanceIds,
	Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch& OutBatch) const
{
	OutBatch = Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch();
	if (!IsValid() || ItemInstanceIds.IsEmpty())
	{
		return false;
	}

	TArray<FGuid> Ordered = ItemInstanceIds;
	Ordered.Sort(GuidLess);
	FGuid PreviousItemInstanceId;
	for (const FGuid& ItemInstanceId : Ordered)
	{
		if (!ItemInstanceId.IsValid()
			|| (PreviousItemInstanceId.IsValid()
				&& PreviousItemInstanceId == ItemInstanceId))
		{
			return false;
		}
		PreviousItemInstanceId = ItemInstanceId;
	}

	OutBatch.RunId = RunId;
	OutBatch.SourceEntityId = SourceEntityId;
	OutBatch.RequestedCount = Ordered.Num();
	OutBatch.Entries.Reserve(Ordered.Num());
	for (const FGuid& ItemInstanceId : Ordered)
	{
		const Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Controllers.Find(ItemInstanceId);
		Fdemo_mapShanmenControlledWeaponDefenseReadinessEntry Entry;
		Entry.ItemInstanceId = ItemInstanceId;
		if (!Controller
			|| !Controller->TryCaptureOrbitDefenseReadiness(
				Entry.Readiness))
		{
			OutBatch =
				Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch();
			return false;
		}
		++OutBatch.CapturedCount;
		OutBatch.Entries.Add(MoveTemp(Entry));
	}

	if (!OutBatch.IsFullyCaptured())
	{
		OutBatch = Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::
IsOrbitDefenseReadinessCurrent(
	const Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch& Batch) const
{
	if (!IsValid()
		|| !Batch.IsFullyCaptured()
		|| Batch.RunId != RunId
		|| Batch.SourceEntityId != SourceEntityId)
	{
		return false;
	}

	for (const Fdemo_mapShanmenControlledWeaponDefenseReadinessEntry& Entry :
		Batch.Entries)
	{
		const Fdemo_mapShanmenControlledWeaponProductController* Controller =
			Controllers.Find(Entry.ItemInstanceId);
		if (!Controller
			|| !Controller->IsOrbitDefenseReadinessCurrent(
				Entry.Readiness))
		{
			return false;
		}
	}
	return true;
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

bool Fdemo_mapShanmenControlledWeaponRunHost::
TryConsumeOrbitThreatPresence(
	const FGuid& ItemInstanceId,
	const FShanmenControlledWeaponThreatPresenceReceipt& Presence,
	FShanmenControlledWeaponThreatPresenceConsumeResult& OutResult)
{
	OutResult = FShanmenControlledWeaponThreatPresenceConsumeResult();
	if (!PresenceMatchesController(ItemInstanceId, Presence))
	{
		return false;
	}

	FShanmenControlledWeaponThreatPresenceAuthority Candidate =
		ThreatPresenceAuthority;
	OutResult = Candidate.Consume(Presence);
	if (!OutResult.IsSuccess() || !Candidate.IsValid())
	{
		return false;
	}
	ThreatPresenceAuthority = MoveTemp(Candidate);
	return IsValid();
}

bool Fdemo_mapShanmenControlledWeaponRunHost::TrySampleOrbitThreat(
	const FGuid& ItemInstanceId,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const TArray<Fdemo_mapShanmenControlledWeaponOrbitThreatContact>& Contacts,
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult& OutResult)
{
	OutResult = Fdemo_mapShanmenControlledWeaponThreatFinalizationResult();
	if (!IsValid()
		|| !CoordinatorMatches(Coordinator)
		|| !Controllers.Contains(ItemInstanceId))
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponRunHost Candidate = *this;
	FShanmenWorldHitContext Context;
	if (!Candidate.TryBeginOrbitThreatWindow(ItemInstanceId, Context))
	{
		return false;
	}

	TArray<AActor*> TargetActors;
	TargetActors.Reserve(Contacts.Num());
	for (const Fdemo_mapShanmenControlledWeaponOrbitThreatContact& Contact :
		Contacts)
	{
		const Fdemo_mapShanmenControlledWeaponOrbitThreatResult Projected =
			Candidate.ProjectOrbitThreatOverlap(
				ItemInstanceId,
				Coordinator,
				Contact.Overlap,
				Contact.ContactLocation,
				Contact.ContactNormal);
		AActor* TargetActor = Contact.Overlap.GetActor();
		if (!Projected.IsProjected() || !TargetActor)
		{
			return false;
		}
		TargetActors.Add(TargetActor);
	}

	FShanmenDetectorEmissionReceipt Emission;
	if (!Candidate.TryEndOrbitThreatWindow(ItemInstanceId, Emission))
	{
		return false;
	}

	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Result;
	if (!Candidate.TryFinalizeOrbitThreatSample(
			ItemInstanceId,
			Coordinator,
			Emission,
			TargetActors,
			Result)
		|| !Result.IsFinalized()
		|| !Candidate.IsValid())
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	OutResult = MoveTemp(Result);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::
TrySampleOrbitThreatsInOrder(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>& Requests,
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch& OutBatch)
{
	OutBatch = Fdemo_mapShanmenControlledWeaponThreatSampleBatch();
	if (!IsValid()
		|| !CoordinatorMatches(Coordinator)
		|| Requests.IsEmpty())
	{
		return false;
	}

	TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>
		OrderedRequests = Requests;
	OrderedRequests.Sort([](
		const Fdemo_mapShanmenControlledWeaponThreatSampleRequest& Left,
		const Fdemo_mapShanmenControlledWeaponThreatSampleRequest& Right)
	{
		return GuidLess(Left.ItemInstanceId, Right.ItemInstanceId);
	});
	for (int32 Index = 0; Index < OrderedRequests.Num(); ++Index)
	{
		const Fdemo_mapShanmenControlledWeaponThreatSampleRequest& Request =
			OrderedRequests[Index];
		if (!Request.IsValid()
			|| !Controllers.Contains(Request.ItemInstanceId)
			|| (Index > 0
				&& OrderedRequests[Index - 1].ItemInstanceId
					== Request.ItemInstanceId))
		{
			return false;
		}
	}

	Fdemo_mapShanmenControlledWeaponRunHost Candidate = *this;
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch Batch;
	Batch.RunId = RunId;
	Batch.AttemptedCount = OrderedRequests.Num();
	Batch.Entries.Reserve(OrderedRequests.Num());
	for (const Fdemo_mapShanmenControlledWeaponThreatSampleRequest& Request :
		OrderedRequests)
	{
		Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Finalization;
		if (!Candidate.TrySampleOrbitThreat(
				Request.ItemInstanceId,
				Coordinator,
				Request.Contacts,
				Finalization))
		{
			return false;
		}

		Fdemo_mapShanmenControlledWeaponThreatSampleBatchEntry& Entry =
			Batch.Entries.AddDefaulted_GetRef();
		Entry.ItemInstanceId = Request.ItemInstanceId;
		Entry.Finalization = MoveTemp(Finalization);
		++Batch.FinalizedCount;
	}

	if (!Batch.IsFullyFinalized() || !Candidate.IsValid())
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	OutBatch = MoveTemp(Batch);
	return true;
}

bool Fdemo_mapShanmenControlledWeaponRunHost::
TryFinalizeOrbitThreatSample(
	const FGuid& ItemInstanceId,
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenDetectorEmissionReceipt& Emission,
	const TArray<AActor*>& TargetActors,
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult& OutResult)
{
	OutResult = Fdemo_mapShanmenControlledWeaponThreatFinalizationResult();
	Fdemo_mapShanmenControlledWeaponThreatEvidenceCaptureResult Evidence;
	FShanmenControlledWeaponThreatPolicyReceipt Policy;
	if (!TryEvaluateOrbitThreatActors(
			ItemInstanceId,
			Coordinator,
			Emission,
			TargetActors,
			Evidence,
			Policy))
	{
		return false;
	}

	FShanmenControlledWeaponThreatPresenceReceipt Presence;
	if (!TryBuildOrbitThreatPresenceIntents(
			ItemInstanceId, Policy, Presence)
		|| !PresenceMatchesController(ItemInstanceId, Presence))
	{
		return false;
	}

	FShanmenControlledWeaponThreatPresenceAuthority Candidate =
		ThreatPresenceAuthority;
	FShanmenControlledWeaponThreatPresenceConsumeResult Consumption =
		Candidate.Consume(Presence);
	Fdemo_mapShanmenControlledWeaponThreatFinalizationResult Result;
	Result.ItemInstanceId = ItemInstanceId;
	Result.Evidence = MoveTemp(Evidence);
	Result.Presence = MoveTemp(Presence);
	Result.Consumption = MoveTemp(Consumption);
	if (!Candidate.IsValid() || !Result.IsFinalized())
	{
		return false;
	}

	ThreatPresenceAuthority = MoveTemp(Candidate);
	OutResult = MoveTemp(Result);
	return true;
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

Fdemo_mapShanmenControlledWeaponDirectedTimelineResult
Fdemo_mapShanmenControlledWeaponRunHost::AdvanceDirectedFixedTicks(
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
	const int64 AdvancedTicks,
	Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenControlledWeaponDirectedTimelineResult Result;
	Result.RunId = Coordinator.IsReady() ? Coordinator.GetRunId() : FGuid();
	Result.TimelineId = TimelineSample.GetTimelineId();
	Result.EndTick = TimelineSample.GetCurrentTick();
	Result.RequestedTickCount = AdvancedTicks;
	if (!TimelineSample.IsValid()
		|| AdvancedTicks < 0
		|| AdvancedTicks > Result.EndTick)
	{
		Result.Diagnostic =
			TEXT("Directed flight requires a valid current timeline sample and non-negative tick delta.");
		return Result;
	}
	Result.StartTick = Result.EndTick - AdvancedTicks;
	if (AdvancedTicks > MaximumDirectedTimelineTicksPerPump)
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::
				TickBudgetExceeded;
		Result.Diagnostic = FString::Printf(
			TEXT("Directed flight rejected %lld catch-up ticks; the bounded per-pump maximum is %lld."),
			static_cast<long long>(AdvancedTicks),
			static_cast<long long>(MaximumDirectedTimelineTicksPerPump));
		return Result;
	}
	if (!Coordinator.IsReady()
		|| TimelineSample.GetTimelineId()
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				Coordinator.GetRunId()))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::
				CoordinatorMismatch;
		Result.Diagnostic =
			TEXT("Directed flight timeline does not belong to the canonical Combat Run.");
		return Result;
	}
	if (AdvancedTicks == 0)
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::None;
		Result.Diagnostic =
			TEXT("No whole canonical tick advanced; directed flight remained unchanged.");
		return Result;
	}
	if (IsEmpty())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::None;
		Result.Diagnostic =
			TEXT("No controlled weapon is bound to the active Run.");
		return Result;
	}
	if (!IsValid())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::HostInvalid;
		Result.Diagnostic =
			TEXT("Directed flight rejected an invalid non-empty controlled-weapon Host.");
		return Result;
	}
	if (!CoordinatorMatches(Coordinator))
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::
				CoordinatorMismatch;
		Result.Diagnostic =
			TEXT("Controlled-weapon Host and Combat Run identities do not match.");
		return Result;
	}

	auto CountDirected = [this]()
	{
		int32 Count = 0;
		for (const FGuid& ItemInstanceId : GetOrderedItemInstanceIds())
		{
			const Fdemo_mapShanmenControlledWeaponProductController* Controller =
				FindController(ItemInstanceId);
			Count += Controller && Controller->IsDirected() ? 1 : 0;
		}
		return Count;
	};
	if (CountDirected() == 0)
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::None;
		Result.Diagnostic =
			TEXT("No controlled weapon is in directed flight for these ticks.");
		return Result;
	}

	const float FixedStepSeconds = 1.0f / static_cast<float>(
		Fdemo_mapShanmenCombatRunFixedTimeline::CanonicalTicksPerSecond());
	for (int64 TickOffset = 0; TickOffset < AdvancedTicks; ++TickOffset)
	{
		if (CountDirected() == 0)
		{
			break;
		}

		Fdemo_mapShanmenControlledWeaponHostMovementBatch Batch;
		if (!TryAdvanceDirectedInOrder(FixedStepSeconds, Batch)
			|| !Batch.IsFullyAdvanced())
		{
			Result.Error =
				Edemo_mapShanmenControlledWeaponDirectedTimelineError::
					MovementRejected;
			Result.Diagnostic = FString::Printf(
				TEXT("Directed movement rejected canonical tick %lld."),
				static_cast<long long>(Result.StartTick + TickOffset + 1));
			return Result;
		}
		++Result.MovementTickCount;
		Result.MovementCount += Batch.AdvancedCount;

		for (const Fdemo_mapShanmenControlledWeaponHostMovementEntry& Entry :
			Batch.Entries)
		{
			if (!Entry.Movement.bBlockingHit)
			{
				continue;
			}
			++Result.BlockingContactCount;
			bool bContactClosed = false;
			FShanmenWorldHitContext Context;
			if (Entry.BlockingHit.bBlockingHit
				&& TryBeginContactWindow(Entry.ItemInstanceId, Context))
			{
				const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult Delivery =
					ResolveSweepContact(
						Entry.ItemInstanceId, Coordinator, Entry.BlockingHit);
				Result.DeliveredImpactCount += Delivery.IsDelivered() ? 1 : 0;
				bContactClosed = TryEndContactWindow(Entry.ItemInstanceId);
			}

			bool bTerminalized = false;
			if (bContactClosed)
			{
				const Fdemo_mapShanmenControlledWeaponProductController* Controller =
					FindController(Entry.ItemInstanceId);
				const int64 ExpectedSequence = Controller
					? Controller->GetSession().GetExecution()
						.GetNextCommandSequence()
					: INDEX_NONE;
				FShanmenControlledWeaponCommandReceipt Recall;
				FShanmenActionTransitionReceipt Recovery;
				FShanmenActionTransitionReceipt Completed;
				bTerminalized = Controller
					&& TryRecallAndComplete(
						Entry.ItemInstanceId,
						ExpectedSequence,
						Recall,
						Recovery,
						Completed);
			}
			if (!bTerminalized)
			{
				const Fdemo_mapShanmenControlledWeaponProductController* Controller =
					FindController(Entry.ItemInstanceId);
				if (Controller && Controller->GetSession().IsTerminal())
				{
					bTerminalized = true;
				}
				else if (Controller && Controller->IsActive())
				{
					FShanmenActionTransitionReceipt Interrupted;
					bTerminalized = TryInterrupt(
						Entry.ItemInstanceId, Interrupted);
					Result.FallbackInterruptedCount += bTerminalized ? 1 : 0;
				}
			}
			if (!bTerminalized)
			{
				Result.Error =
					Edemo_mapShanmenControlledWeaponDirectedTimelineError::
						ContactLifecycleRejected;
				Result.FailedItemInstanceId = Entry.ItemInstanceId;
				Result.Diagnostic = FString::Printf(
					TEXT("Blocking contact for item %s could not reach a terminal lifecycle."),
					*Entry.ItemInstanceId.ToString(
						EGuidFormats::DigitsWithHyphens));
				return Result;
			}
			++Result.TerminalizedCount;
		}
	}

	Result.Error =
		Edemo_mapShanmenControlledWeaponDirectedTimelineError::None;
	Result.Diagnostic = Result.BlockingContactCount > 0
		? TEXT("Directed flight advanced on canonical ticks and every blocking item reached its existing terminal lifecycle.")
		: TEXT("Directed flight advanced on canonical ticks without blocking contact.");
	if (!Result.IsSuccess() || !IsValid())
	{
		Result.Error =
			Edemo_mapShanmenControlledWeaponDirectedTimelineError::
				ContactLifecycleRejected;
		Result.Diagnostic =
			TEXT("Directed flight produced an invalid audit or Host state.");
	}
	return Result;
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
	const FGuid ActivationId = Controller->GetSession().GetActionRuntime()
		.GetAction().GetActivationId();
	if (!Candidate.ThreatPresenceAuthority.TryRetireItemActivation(
			ItemInstanceId, ActivationId))
	{
		return false;
	}
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

bool Fdemo_mapShanmenControlledWeaponRunHost::PresenceMatchesController(
	const FGuid& ItemInstanceId,
	const FShanmenControlledWeaponThreatPresenceReceipt& Presence) const
{
	const Fdemo_mapShanmenControlledWeaponProductController* Controller =
		IsValid() ? Controllers.Find(ItemInstanceId) : nullptr;
	if (!Controller
		|| !Controller->IsOrbiting()
		|| Controller->HasActiveContactWindow()
		|| !Presence.IsValid())
	{
		return false;
	}

	const FShanmenCombatActionSnapshot& PresenceAction =
		Presence.GetPolicy().GetEmission().GetContext().GetAction();
	const FShanmenCombatActionSnapshot& ControllerAction =
		Controller->GetSession().GetActionRuntime().GetAction();
	return Controller->GetSession().GetExecution()
			.IsLatestCompletedOrbitThreatEmission(
				Presence.GetPolicy().GetEmission())
		&& PresenceAction.GetRunId() == RunId
		&& PresenceAction.GetOwnerId() == ControllerAction.GetOwnerId()
		&& PresenceAction.GetActivationId()
			== ControllerAction.GetActivationId()
		&& PresenceAction.GetSourceEntityId() == SourceEntityId
		&& PresenceAction.GetSourceItemInstanceId() == ItemInstanceId
		&& PresenceAction.GetActionDefinitionId()
			== ControllerAction.GetActionDefinitionId()
		&& PresenceAction.GetContent().Version
			== ControllerAction.GetContent().Version
		&& PresenceAction.GetContent().Digest
			== ControllerAction.GetContent().Digest
		&& PresenceAction.GetSourceTags()
			== ControllerAction.GetSourceTags();
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
