#include "demo_mapShanmenCombatConditionComponent.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenMeridianShockTreatmentRecoveryProof.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsAcceptedApplyMutation(
		const Edemo_mapExactModifierMutationStatus Status)
	{
		return Status == Edemo_mapExactModifierMutationStatus::Applied
			|| Status == Edemo_mapExactModifierMutationStatus::ApplyReplayed;
	}

	bool IsAcceptedRemoveMutation(
		const Edemo_mapExactModifierMutationStatus Status)
	{
		return Status == Edemo_mapExactModifierMutationStatus::Removed
			|| Status == Edemo_mapExactModifierMutationStatus::RemoveReplayed;
	}
}

bool Fdemo_mapShanmenCombatConditionApplicationReceipt::IsValid() const
{
	return ApplicationId.IsValid()
		&& RunId.IsValid()
		&& TargetEntityId.IsValid()
		&& TimelineId.IsValid()
		&& ImpactId.IsValid()
		&& ResolutionId.IsValid()
		&& !DefinitionId.IsNone()
		&& DefinitionId
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& AppliedAtTick >= 0
		&& ExpiryTick > AppliedAtTick
		&& ConditionRevision > 0
		&& ApplicationId
			== Udemo_mapShanmenCombatConditionComponent::MakeApplicationId(
				RunId,
				TargetEntityId,
				TimelineId,
				ImpactId,
				ResolutionId);
}

bool Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const FGuid& RequestedItemInstanceId,
	const FName RequestedItemDefinitionId,
	const int64 RequestedConditionRevision,
	Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenCombatConditionTreatmentIntent();
	Fdemo_mapShanmenCombatConditionTreatmentIntent Candidate;
	Candidate.RunId = RequestedRunId;
	Candidate.TargetEntityId = RequestedTargetEntityId;
	Candidate.TimelineId = RequestedTimelineId;
	Candidate.ItemInstanceId = RequestedItemInstanceId;
	Candidate.ItemDefinitionId = RequestedItemDefinitionId;
	Candidate.ConditionDefinitionId =
		Udemo_mapShanmenCombatConditionComponent::MeridianShockDefinitionId();
	Candidate.ExpectedConditionRevision = RequestedConditionRevision;
	Candidate.TreatmentId =
		Udemo_mapShanmenCombatConditionComponent::MakeTreatmentId(
			Candidate.RunId,
			Candidate.TargetEntityId,
			Candidate.TimelineId,
			Candidate.ItemInstanceId,
			Candidate.ItemDefinitionId,
			Candidate.ExpectedConditionRevision);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutIntent = Candidate;
	return true;
}

bool Fdemo_mapShanmenCombatConditionTreatmentIntent::IsValid() const
{
	return TreatmentId.IsValid()
		&& RunId.IsValid()
		&& TargetEntityId.IsValid()
		&& TimelineId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ItemDefinitionId
			== Fdemo_mapItemIds::MeridianStabilizingPillLevel1
		&& ConditionDefinitionId
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& ExpectedConditionRevision > 0
		&& TreatmentId
			== Udemo_mapShanmenCombatConditionComponent::MakeTreatmentId(
				RunId,
				TargetEntityId,
				TimelineId,
				ItemInstanceId,
				ItemDefinitionId,
				ExpectedConditionRevision);
}

bool Fdemo_mapShanmenCombatConditionTreatmentReceipt::Matches(
	const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent) const
{
	return Intent.IsValid()
		&& TreatmentId == Intent.GetTreatmentId()
		&& RunId == Intent.GetRunId()
		&& TargetEntityId == Intent.GetTargetEntityId()
		&& TimelineId == Intent.GetTimelineId()
		&& ItemInstanceId == Intent.GetItemInstanceId()
		&& ItemDefinitionId == Intent.GetItemDefinitionId()
		&& ConditionDefinitionId == Intent.GetConditionDefinitionId()
		&& ConditionRevisionBefore
			== Intent.GetExpectedConditionRevision();
}

bool Fdemo_mapShanmenCombatConditionTreatmentReceipt::IsValid() const
{
	return TreatmentId.IsValid()
		&& RunId.IsValid()
		&& TargetEntityId.IsValid()
		&& TimelineId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ItemDefinitionId
			== Fdemo_mapItemIds::MeridianStabilizingPillLevel1
		&& ConditionDefinitionId
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& TreatedAtTick >= 0
		&& ConditionRevisionBefore > 0
		&& ConditionRevisionBefore < MAX_int64
		&& ConditionRevisionAfter == ConditionRevisionBefore + 1
		&& TreatmentId
			== Udemo_mapShanmenCombatConditionComponent::MakeTreatmentId(
				RunId,
				TargetEntityId,
				TimelineId,
				ItemInstanceId,
				ItemDefinitionId,
				ConditionRevisionBefore);
}

Udemo_mapShanmenCombatConditionComponent::
	Udemo_mapShanmenCombatConditionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void Udemo_mapShanmenCombatConditionComponent::OnComponentDestroyed(
	const bool bDestroyingHierarchy)
{
	Reset();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

FName Udemo_mapShanmenCombatConditionComponent::MeridianShockDefinitionId()
{
	static const FName Value(TEXT("Condition.Injury.MeridianShock.Minor.r1"));
	return Value;
}

FName
Udemo_mapShanmenCombatConditionComponent::MeridianShockModifierSourceId()
{
	static const FName Value(
		TEXT("Condition.Injury.MeridianShock.Minor.MoveSpeed.r1"));
	return Value;
}

FGuid Udemo_mapShanmenCombatConditionComponent::MakeApplicationId(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const FGuid& ImpactId,
	const FGuid& ResolutionId)
{
	if (!RequestedRunId.IsValid()
		|| !RequestedTargetEntityId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| !ImpactId.IsValid()
		|| !ResolutionId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.Condition.Application.r1"),
		{
			GuidDigits(RequestedRunId),
			GuidDigits(RequestedTargetEntityId),
			GuidDigits(RequestedTimelineId),
			GuidDigits(ImpactId),
			GuidDigits(ResolutionId),
			MeridianShockDefinitionId().ToString()
		});
}

FGuid Udemo_mapShanmenCombatConditionComponent::MakeTreatmentId(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const FGuid& RequestedItemInstanceId,
	const FName RequestedItemDefinitionId,
	const int64 ExpectedConditionRevision)
{
	if (!RequestedRunId.IsValid()
		|| !RequestedTargetEntityId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| !RequestedItemInstanceId.IsValid()
		|| RequestedItemDefinitionId.IsNone()
		|| ExpectedConditionRevision <= 0)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.Condition.MeridianShock.Treatment.r1"),
		{
			GuidDigits(RequestedRunId),
			GuidDigits(RequestedTargetEntityId),
			GuidDigits(RequestedTimelineId),
			GuidDigits(RequestedItemInstanceId),
			RequestedItemDefinitionId.ToString(),
			FString::Printf(
				TEXT("%lld"),
				static_cast<long long>(ExpectedConditionRevision)),
			MeridianShockDefinitionId().ToString()
		});
}

Fdemo_mapModifierHandle
Udemo_mapShanmenCombatConditionComponent::MakeMeridianShockModifierHandle(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId)
{
	Fdemo_mapModifierHandle Handle;
	if (RequestedRunId.IsValid() && RequestedTargetEntityId.IsValid())
	{
		Handle.Value = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.Condition.Modifier.r1"),
			{
				GuidDigits(RequestedRunId),
				GuidDigits(RequestedTargetEntityId),
				MeridianShockDefinitionId().ToString(),
				Fdemo_mapAttributeIds::MoveSpeed.ToString()
			});
	}
	return Handle;
}

Fdemo_mapModifierSpec
Udemo_mapShanmenCombatConditionComponent::MakeMeridianShockModifierSpec()
{
	Fdemo_mapModifierSpec Spec;
	Spec.SourceId = MeridianShockModifierSourceId();
	Spec.AttributeId = Fdemo_mapAttributeIds::MoveSpeed;
	Spec.Operation = Edemo_mapModifierOperation::Multiply;
	Spec.Value = MeridianShockMoveSpeedMultiplier();
	Spec.Priority = 500;
	return Spec;
}

bool Udemo_mapShanmenCombatConditionComponent::TryBegin(
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	Udemo_mapAttributeComponent* RequestedAttributeComponent,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid()
		|| !RequestedRunId.IsValid()
		|| !RequestedTargetEntityId.IsValid()
		|| !RequestedTimelineId.IsValid()
		|| RequestedTimelineId
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				RequestedRunId)
		|| !::IsValid(RequestedAttributeComponent)
		|| !RequestedAttributeComponent->IsAttributeRegistered(
			Fdemo_mapAttributeIds::MoveSpeed))
	{
		OutDiagnostic =
			TEXT("Combat condition begin requires valid Run, target, canonical timeline and attribute authority.");
		return false;
	}
	if (!IsEmpty())
	{
		if (RunId == RequestedRunId
			&& TargetEntityId == RequestedTargetEntityId
			&& TimelineId == RequestedTimelineId
			&& AttributeComponent.Get() == RequestedAttributeComponent)
		{
			OutDiagnostic =
				TEXT("Combat condition authority is already active for this Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("Combat condition authority rejects a second active Run.");
		return false;
	}

	const Fdemo_mapModifierHandle CandidateHandle =
		MakeMeridianShockModifierHandle(
			RequestedRunId,
			RequestedTargetEntityId);
	if (!CandidateHandle.IsValid()
		|| !IsAcceptedRemoveMutation(
			RequestedAttributeComponent->EnsureModifierRemoved(
				MakeMeridianShockModifierSpec(),
				CandidateHandle)))
	{
		OutDiagnostic =
			TEXT("Combat condition begin could not establish an empty exact modifier slot.");
		return false;
	}

	RunId = RequestedRunId;
	TargetEntityId = RequestedTargetEntityId;
	TimelineId = RequestedTimelineId;
	AttributeComponent = RequestedAttributeComponent;
	MeridianShockModifierHandle = CandidateHandle;
	LastObservedTick = 0;
	MeridianShockExpiryTick = INDEX_NONE;
	ConditionRevision = 0;
	bMeridianShockActive = false;
	ProcessedApplications.Reset();
	ProcessedTreatments.Reset();
	OutDiagnostic =
		TEXT("Combat condition authority began empty at canonical tick zero.");
	return IsValid();
}

Fdemo_mapShanmenCombatConditionAdvanceResult
Udemo_mapShanmenCombatConditionComponent::TryAdvance(
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	Fdemo_mapShanmenCombatConditionAdvanceResult Result;
	Result.ConditionRevision = ConditionRevision;
	if (!IsValid() || IsEmpty() || !AttributeComponent.IsValid())
	{
		return Result;
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId() != TimelineId)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::TimelineMismatch;
		return Result;
	}
	if (TimelineSample.GetCurrentTick() < LastObservedTick)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::StaleTimeline;
		return Result;
	}

	const int64 ObservedTick = TimelineSample.GetCurrentTick();
	if (!bMeridianShockActive || ObservedTick < MeridianShockExpiryTick)
	{
		LastObservedTick = ObservedTick;
		Result.Status =
			Edemo_mapShanmenCombatConditionAdvanceStatus::Observed;
		Result.Error = Edemo_mapShanmenCombatConditionError::None;
		Result.ObservedTick = LastObservedTick;
		return Result;
	}
	if (ConditionRevision == MAX_int64)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::RevisionExhausted;
		return Result;
	}
	if (!IsAcceptedRemoveMutation(
			AttributeComponent->EnsureModifierRemoved(
				MakeMeridianShockModifierSpec(),
				MeridianShockModifierHandle)))
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::ModifierRejected;
		return Result;
	}

	bMeridianShockActive = false;
	MeridianShockExpiryTick = INDEX_NONE;
	LastObservedTick = ObservedTick;
	++ConditionRevision;
	Result.Status = Edemo_mapShanmenCombatConditionAdvanceStatus::Expired;
	Result.Error = Edemo_mapShanmenCombatConditionError::None;
	Result.ObservedTick = LastObservedTick;
	Result.ConditionRevision = ConditionRevision;
	return Result;
}

Fdemo_mapShanmenCombatConditionApplicationResult
Udemo_mapShanmenCombatConditionComponent::TryApplyMeridianShock(
	const FShanmenVitalityCommitReceipt& VitalityReceipt,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	Fdemo_mapShanmenCombatConditionApplicationResult Result;
	if (!IsValid() || IsEmpty() || !AttributeComponent.IsValid())
	{
		return Result;
	}
	if (!VitalityReceipt.IsValid())
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::InvalidReceipt;
		return Result;
	}
	if (VitalityReceipt.GetTargetEntityId() != TargetEntityId)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::TargetMismatch;
		return Result;
	}
	if (!TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId() != TimelineId)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::TimelineMismatch;
		return Result;
	}
	if (TimelineSample.GetCurrentTick() < LastObservedTick)
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::StaleTimeline;
		return Result;
	}

	const Fdemo_mapShanmenCombatConditionAdvanceResult Advance =
		TryAdvance(TimelineSample);
	if (!Advance.IsSuccess())
	{
		Result.Error = Advance.Error;
		return Result;
	}
	if (VitalityReceipt.GetAppliedDamage() <= 0.0f)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::NoCommittedDamage;
		return Result;
	}
	if (const FProcessedApplication* Existing =
		ProcessedApplications.Find(VitalityReceipt.GetImpactId()))
	{
		if (Existing->ResolutionId != VitalityReceipt.GetResolutionId())
		{
			Result.Error =
				Edemo_mapShanmenCombatConditionError::ImpactConflict;
			return Result;
		}
		Result.Status =
			Edemo_mapShanmenCombatConditionApplicationStatus::AlreadyApplied;
		Result.Error = Edemo_mapShanmenCombatConditionError::None;
		Result.Receipt = Existing->Receipt;
		return Result;
	}

	const int64 AppliedAtTick = TimelineSample.GetCurrentTick();
	if (AppliedAtTick > MAX_int64 - MeridianShockDurationTicks()
		|| ConditionRevision == MAX_int64)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::RevisionExhausted;
		return Result;
	}
	const int64 CandidateExpiry =
		AppliedAtTick + MeridianShockDurationTicks();
	const int64 EffectiveExpiry = bMeridianShockActive
		? FMath::Max(MeridianShockExpiryTick, CandidateExpiry)
		: CandidateExpiry;
	const int64 CandidateRevision = ConditionRevision + 1;

	Fdemo_mapShanmenCombatConditionApplicationReceipt Receipt;
	Receipt.RunId = RunId;
	Receipt.TargetEntityId = TargetEntityId;
	Receipt.TimelineId = TimelineId;
	Receipt.ImpactId = VitalityReceipt.GetImpactId();
	Receipt.ResolutionId = VitalityReceipt.GetResolutionId();
	Receipt.DefinitionId = MeridianShockDefinitionId();
	Receipt.AppliedAtTick = AppliedAtTick;
	Receipt.ExpiryTick = EffectiveExpiry;
	Receipt.ConditionRevision = CandidateRevision;
	Receipt.ApplicationId = MakeApplicationId(
		RunId,
		TargetEntityId,
		TimelineId,
		Receipt.ImpactId,
		Receipt.ResolutionId);
	if (!Receipt.IsValid())
	{
		Result.Error = Edemo_mapShanmenCombatConditionError::InvalidReceipt;
		return Result;
	}

	const bool bWasActive = bMeridianShockActive;
	if (!IsAcceptedApplyMutation(
			AttributeComponent->EnsureModifierApplied(
				MakeMeridianShockModifierSpec(),
				MeridianShockModifierHandle)))
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionError::ModifierRejected;
		return Result;
	}

	bMeridianShockActive = true;
	MeridianShockExpiryTick = EffectiveExpiry;
	ConditionRevision = CandidateRevision;
	FProcessedApplication& Processed =
		ProcessedApplications.Add(Receipt.ImpactId);
	Processed.ResolutionId = Receipt.ResolutionId;
	Processed.Receipt = Receipt;
	Result.Status = bWasActive
		? Edemo_mapShanmenCombatConditionApplicationStatus::Refreshed
		: Edemo_mapShanmenCombatConditionApplicationStatus::Applied;
	Result.Error = Edemo_mapShanmenCombatConditionError::None;
	Result.Receipt = Receipt;
	return Result;
}

Fdemo_mapShanmenCombatConditionTreatmentResult
Udemo_mapShanmenCombatConditionComponent::TryTreatMeridianShock(
	const Fdemo_mapShanmenCombatConditionTreatmentIntent& Intent,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample)
{
	Fdemo_mapShanmenCombatConditionTreatmentResult Result;
	if (!IsValid() || IsEmpty() || !AttributeComponent.IsValid())
	{
		return Result;
	}
	if (!Intent.IsValid())
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::InvalidIntent;
		return Result;
	}
	if (Intent.GetRunId() != RunId)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::RunMismatch;
		return Result;
	}
	if (Intent.GetTargetEntityId() != TargetEntityId)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::TargetMismatch;
		return Result;
	}
	if (Intent.GetTimelineId() != TimelineId
		|| !TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId() != TimelineId)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::TimelineMismatch;
		return Result;
	}
	if (TimelineSample.GetCurrentTick() < LastObservedTick)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::StaleTimeline;
		return Result;
	}

	if (const FProcessedTreatment* Existing =
		ProcessedTreatments.Find(Intent.GetTreatmentId()))
	{
		if (!Existing->Intent.IsValid()
			|| !Existing->Receipt.IsValid()
			|| !Existing->Receipt.Matches(Intent))
		{
			Result.Error =
				Edemo_mapShanmenCombatConditionTreatmentError::
					TreatmentConflict;
			return Result;
		}
		Result.Status =
			Edemo_mapShanmenCombatConditionTreatmentStatus::AlreadyTreated;
		Result.Error = Edemo_mapShanmenCombatConditionTreatmentError::None;
		Result.Receipt = Existing->Receipt;
		return Result;
	}

	const Fdemo_mapShanmenCombatConditionAdvanceResult Advance =
		TryAdvance(TimelineSample);
	if (!Advance.IsSuccess())
	{
		Result.Error = Advance.Error
			== Edemo_mapShanmenCombatConditionError::StaleTimeline
			? Edemo_mapShanmenCombatConditionTreatmentError::StaleTimeline
			: Edemo_mapShanmenCombatConditionTreatmentError::
				ComponentNotReady;
		return Result;
	}
	if (!bMeridianShockActive)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::ConditionInactive;
		return Result;
	}
	if (Intent.GetExpectedConditionRevision() != ConditionRevision)
	{
		Result.Error = Edemo_mapShanmenCombatConditionTreatmentError::
			ConditionRevisionMismatch;
		return Result;
	}
	if (ConditionRevision == MAX_int64)
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::RevisionExhausted;
		return Result;
	}
	Fdemo_mapShanmenCombatConditionTreatmentReceipt Receipt;
	Receipt.TreatmentId = Intent.GetTreatmentId();
	Receipt.RunId = Intent.GetRunId();
	Receipt.TargetEntityId = Intent.GetTargetEntityId();
	Receipt.TimelineId = Intent.GetTimelineId();
	Receipt.ItemInstanceId = Intent.GetItemInstanceId();
	Receipt.ItemDefinitionId = Intent.GetItemDefinitionId();
	Receipt.ConditionDefinitionId = Intent.GetConditionDefinitionId();
	Receipt.TreatedAtTick = TimelineSample.GetCurrentTick();
	Receipt.ConditionRevisionBefore = ConditionRevision;
	Receipt.ConditionRevisionAfter = ConditionRevision + 1;
	if (!Receipt.IsValid() || !Receipt.Matches(Intent))
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::InvalidIntent;
		return Result;
	}
	if (!IsAcceptedRemoveMutation(
		AttributeComponent->EnsureModifierRemoved(
			MakeMeridianShockModifierSpec(),
			MeridianShockModifierHandle)))
	{
		Result.Error =
			Edemo_mapShanmenCombatConditionTreatmentError::ModifierRejected;
		return Result;
	}

	bMeridianShockActive = false;
	MeridianShockExpiryTick = INDEX_NONE;
	LastObservedTick = TimelineSample.GetCurrentTick();
	ConditionRevision = Receipt.GetConditionRevisionAfter();
	FProcessedTreatment& Processed =
		ProcessedTreatments.Add(Receipt.GetTreatmentId());
	Processed.Intent = Intent;
	Processed.Receipt = Receipt;
	Result.Status =
		Edemo_mapShanmenCombatConditionTreatmentStatus::Treated;
	Result.Error = Edemo_mapShanmenCombatConditionTreatmentError::None;
	Result.Receipt = Receipt;
	return Result;
}

bool Udemo_mapShanmenCombatConditionComponent::
	TryGetProcessedMeridianShockTreatment(
		const FGuid& TreatmentId,
		Fdemo_mapShanmenCombatConditionTreatmentReceipt& OutReceipt) const
{
	OutReceipt = Fdemo_mapShanmenCombatConditionTreatmentReceipt();
	if (!IsValid() || IsEmpty() || !TreatmentId.IsValid())
	{
		return false;
	}
	const FProcessedTreatment* Processed =
		ProcessedTreatments.Find(TreatmentId);
	if (!Processed
		|| !Processed->Intent.IsValid()
		|| !Processed->Receipt.IsValid()
		|| Processed->Intent.GetTreatmentId() != TreatmentId
		|| Processed->Receipt.GetTreatmentId() != TreatmentId
		|| !Processed->Receipt.Matches(Processed->Intent))
	{
		return false;
	}
	OutReceipt = Processed->Receipt;
	return true;
}

bool Udemo_mapShanmenCombatConditionComponent::
	TryRestoreProcessedMeridianShockTreatment(
		const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Proof,
		FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || IsEmpty() || !AttributeComponent.IsValid())
	{
		OutDiagnostic =
			TEXT("Treatment proof restore requires one valid active condition authority.");
		return false;
	}

	Fdemo_mapShanmenCombatConditionTreatmentIntent Intent;
	Fdemo_mapShanmenCombatConditionTreatmentReceipt Receipt;
	if (!Proof.TryRestore(Intent, Receipt)
		|| Receipt.GetRunId() != RunId
		|| Receipt.GetTargetEntityId() != TargetEntityId
		|| Receipt.GetTimelineId() != TimelineId)
	{
		OutDiagnostic =
			TEXT("Treatment proof does not belong to this Run condition authority.");
		return false;
	}

	if (const FProcessedTreatment* Existing =
		ProcessedTreatments.Find(Receipt.GetTreatmentId()))
	{
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof ExistingProof;
		if (!Existing->Intent.IsValid()
			|| !Existing->Receipt.IsValid()
			|| !Existing->Receipt.Matches(Intent)
			|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryCapture(
				Existing->Receipt,
				ExistingProof)
			|| !ExistingProof.Matches(Proof))
		{
			OutDiagnostic =
				TEXT("Treatment proof conflicts with existing condition history.");
			return false;
		}
		OutDiagnostic =
			TEXT("Exact processed treatment proof was already restored.");
		return true;
	}

	if (bMeridianShockActive
		|| LastObservedTick != 0
		|| ConditionRevision != 0
		|| !ProcessedApplications.IsEmpty()
		|| !ProcessedTreatments.IsEmpty())
	{
		OutDiagnostic =
			TEXT("Treatment proof restore refuses non-fresh or active condition state.");
		return false;
	}

	FProcessedTreatment& Processed =
		ProcessedTreatments.Add(Receipt.GetTreatmentId());
	Processed.Intent = Intent;
	Processed.Receipt = Receipt;
	LastObservedTick = Receipt.GetTreatedAtTick();
	ConditionRevision = Receipt.GetConditionRevisionAfter();
	if (!IsValid())
	{
		ProcessedTreatments.Reset();
		LastObservedTick = 0;
		ConditionRevision = 0;
		OutDiagnostic =
			TEXT("Treatment proof restore failed condition invariant validation.");
		return false;
	}
	OutDiagnostic =
		TEXT("Exact processed treatment proof restored into fresh condition history.");
	return true;
}

bool Udemo_mapShanmenCombatConditionComponent::
	TryCaptureMeridianShockStatus(
		Fdemo_mapShanmenCombatConditionStatusSnapshot& OutStatus) const
{
	OutStatus = Fdemo_mapShanmenCombatConditionStatusSnapshot();
	return IsValid()
		&& !IsEmpty()
		&& Fdemo_mapShanmenCombatConditionStatusSnapshot::TryCapture(
			RunId,
			TargetEntityId,
			TimelineId,
			LastObservedTick,
			MeridianShockExpiryTick,
			ConditionRevision,
			bMeridianShockActive,
			OutStatus);
}

bool Udemo_mapShanmenCombatConditionComponent::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ExpectedRunId.IsValid() || !IsValid())
	{
		OutDiagnostic =
			TEXT("Combat condition end requires valid state and Run identity.");
		return false;
	}
	if (IsEmpty())
	{
		OutDiagnostic = TEXT("Combat condition authority is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic =
			TEXT("Combat condition authority rejects mismatched Run teardown.");
		return false;
	}
	if (bMeridianShockActive
		&& (!AttributeComponent.IsValid()
			|| !IsAcceptedRemoveMutation(
				AttributeComponent->EnsureModifierRemoved(
					MakeMeridianShockModifierSpec(),
					MeridianShockModifierHandle))))
	{
		OutDiagnostic =
			TEXT("Combat condition teardown could not remove its exact modifier.");
		return false;
	}
	ClearState();
	OutDiagnostic =
		TEXT("Combat condition authority ended and removed Run projections.");
	return true;
}

void Udemo_mapShanmenCombatConditionComponent::Reset()
{
	if (bMeridianShockActive && AttributeComponent.IsValid())
	{
		AttributeComponent->EnsureModifierRemoved(
			MakeMeridianShockModifierSpec(),
			MeridianShockModifierHandle);
	}
	ClearState();
}

void Udemo_mapShanmenCombatConditionComponent::ClearState()
{
	RunId.Invalidate();
	TargetEntityId.Invalidate();
	TimelineId.Invalidate();
	AttributeComponent.Reset();
	MeridianShockModifierHandle.Reset();
	LastObservedTick = INDEX_NONE;
	MeridianShockExpiryTick = INDEX_NONE;
	ConditionRevision = 0;
	bMeridianShockActive = false;
	ProcessedApplications.Reset();
	ProcessedTreatments.Reset();
}

bool Udemo_mapShanmenCombatConditionComponent::IsValid() const
{
	if (!RunId.IsValid()
		&& !TargetEntityId.IsValid()
		&& !TimelineId.IsValid())
	{
		return !AttributeComponent.IsValid()
			&& !MeridianShockModifierHandle.IsValid()
			&& LastObservedTick == INDEX_NONE
			&& MeridianShockExpiryTick == INDEX_NONE
			&& ConditionRevision == 0
			&& !bMeridianShockActive
			&& ProcessedApplications.IsEmpty()
			&& ProcessedTreatments.IsEmpty();
	}

	if (!RunId.IsValid()
		|| !TargetEntityId.IsValid()
		|| !TimelineId.IsValid()
		|| TimelineId
			!= Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId)
		|| !AttributeComponent.IsValid()
		|| !MeridianShockModifierHandle.IsValid()
		|| !(MeridianShockModifierHandle
			== MakeMeridianShockModifierHandle(RunId, TargetEntityId))
		|| LastObservedTick < 0
		|| ConditionRevision < 0
		|| (bMeridianShockActive
			? MeridianShockExpiryTick <= LastObservedTick
			: MeridianShockExpiryTick != INDEX_NONE))
	{
		return false;
	}

	for (const TPair<FGuid, FProcessedApplication>& Pair :
		ProcessedApplications)
	{
		const Fdemo_mapShanmenCombatConditionApplicationReceipt& Receipt =
			Pair.Value.Receipt;
		if (!Receipt.IsValid()
			|| Pair.Key != Receipt.GetImpactId()
			|| Pair.Value.ResolutionId != Receipt.GetResolutionId()
			|| Receipt.GetRunId() != RunId
			|| Receipt.GetTargetEntityId() != TargetEntityId
			|| Receipt.GetTimelineId() != TimelineId
			|| Receipt.GetConditionRevision() > ConditionRevision)
		{
			return false;
		}
	}
	for (const TPair<FGuid, FProcessedTreatment>& Pair :
		ProcessedTreatments)
	{
		if (!Pair.Value.Intent.IsValid()
			|| !Pair.Value.Receipt.IsValid()
			|| Pair.Key != Pair.Value.Intent.GetTreatmentId()
			|| Pair.Key != Pair.Value.Receipt.GetTreatmentId()
			|| !Pair.Value.Receipt.Matches(Pair.Value.Intent)
			|| Pair.Value.Intent.GetRunId() != RunId
			|| Pair.Value.Intent.GetTargetEntityId() != TargetEntityId
			|| Pair.Value.Intent.GetTimelineId() != TimelineId
			|| Pair.Value.Receipt.GetConditionRevisionAfter()
				> ConditionRevision)
		{
			return false;
		}
	}
	return true;
}

bool Udemo_mapShanmenCombatConditionComponent::IsEmpty() const
{
	return IsValid() && !RunId.IsValid();
}
