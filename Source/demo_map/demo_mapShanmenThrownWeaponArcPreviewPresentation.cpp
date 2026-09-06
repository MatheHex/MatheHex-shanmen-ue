#include "demo_mapShanmenThrownWeaponArcPreviewPresentation.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EMode =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationMode;
	using EProjectStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus;
	using EReduceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool IsKnownMode(const EMode Mode)
	{
		return Mode == EMode::Hidden || Mode == EMode::Visible;
	}

	bool IsPreviewChoice(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice)
	{
		return Choice.IsValid()
			&& Choice.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Choice.HasArcTargetIntent();
	}

	FGuid MakeSegmentId(
		const FGuid& SourcePreviewId,
		const int32 Index,
		const FVector& Start,
		const FVector& End)
	{
		if (!SourcePreviewId.IsValid()
			|| Index < 0
			|| !IsFiniteVector(Start)
			|| !IsFiniteVector(End))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationSegment.r1"),
			{
				GuidDigits(SourcePreviewId),
				FString::FromInt(Index),
				DoubleBits(Start.X),
				DoubleBits(Start.Y),
				DoubleBits(Start.Z),
				DoubleBits(End.X),
				DoubleBits(End.Y),
				DoubleBits(End.Z)
			});
	}

	bool SameScope(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Left,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Right)
	{
		return Left.GetRunId() == Right.GetRunId()
			&& Left.GetPlayerEntityId() == Right.GetPlayerEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId();
	}

	FGuid MakeStateId(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State)
	{
		if (!IsKnownMode(State.GetMode())
			|| !State.GetRunId().IsValid()
			|| !State.GetPlayerEntityId().IsValid()
			|| !State.GetSourceItemInstanceId().IsValid()
			|| !State.GetChoiceState().IsValid())
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			FString::FromInt(static_cast<int32>(State.GetMode())),
			GuidDigits(State.GetRunId()),
			GuidDigits(State.GetPlayerEntityId()),
			GuidDigits(State.GetSourceItemInstanceId()),
			GuidDigits(State.GetChoiceState().GetStateId()),
			FString::Printf(
				TEXT("%llu"),
				static_cast<unsigned long long>(State.GetChoiceRevision()))
		};
		if (State.GetMode() == EMode::Visible)
		{
			if (!State.GetSourceProductRequestId().IsValid()
				|| !State.GetSourcePreviewActivationId().IsValid()
				|| !State.GetSourcePreview().IsValid())
			{
				return FGuid();
			}
			Parts.Append({
				GuidDigits(State.GetSourceProductRequestId()),
				GuidDigits(State.GetSourcePreviewActivationId()),
				GuidDigits(State.GetSourcePreview().GetPreviewId()),
				FString::FromInt(State.NumSegments())
			});
			for (const auto& Segment : State.GetSegments())
			{
				if (!Segment.IsValid())
				{
					return FGuid();
				}
				Parts.Add(GuidDigits(Segment.GetSegmentId()));
			}
		}
		else
		{
			Parts.Append({TEXT("NONE"), TEXT("NONE"), TEXT("NONE"), TEXT("0")});
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewPresentationState.r1"),
			Parts);
	}

}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment::IsValid()
	const
{
	return SegmentId.IsValid()
		&& SourcePreviewId.IsValid()
		&& Index >= 0
		&& IsFiniteVector(Start)
		&& IsFiniteVector(End)
		&& SegmentId == MakeSegmentId(SourcePreviewId, Index, Start, End);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& SegmentId == Other.SegmentId
		&& SourcePreviewId == Other.SourcePreviewId
		&& Index == Other.Index
		&& Start == Other.Start
		&& End == Other.End;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState::
TryRehydrateVisible(
	const FGuid& ExpectedPresentationStateId,
	const FGuid& InRunId,
	const FGuid& InPlayerEntityId,
	const FGuid& InSourceItemInstanceId,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& InChoiceState,
	const FGuid& InSourceProductRequestId,
	const FGuid& InSourcePreviewActivationId,
	const FShanmenThrownWeaponArcPreview& InSourcePreview,
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& OutState)
{
	OutState = Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState();
	if (!ExpectedPresentationStateId.IsValid()
		|| !InRunId.IsValid() || !InPlayerEntityId.IsValid()
		|| !InSourceItemInstanceId.IsValid() || !InChoiceState.IsValid()
		|| !InSourceProductRequestId.IsValid()
		|| !InSourcePreviewActivationId.IsValid()
		|| !InSourcePreview.IsValid())
	{
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Candidate;
	Candidate.Mode = EMode::Visible;
	Candidate.RunId = InRunId;
	Candidate.PlayerEntityId = InPlayerEntityId;
	Candidate.SourceItemInstanceId = InSourceItemInstanceId;
	Candidate.ChoiceState = InChoiceState;
	Candidate.SourceProductRequestId = InSourceProductRequestId;
	Candidate.SourcePreviewActivationId = InSourcePreviewActivationId;
	Candidate.SourcePreview = InSourcePreview;
	Candidate.Segments.Reserve(InSourcePreview.GetSegmentCount());
	const TArray<FVector>& Positions = InSourcePreview.GetPositions();
	for (int32 Index = 0; Index < InSourcePreview.GetSegmentCount(); ++Index)
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment Segment;
		Segment.SourcePreviewId = InSourcePreview.GetPreviewId();
		Segment.Index = Index;
		Segment.Start = Positions[Index];
		Segment.End = Positions[Index + 1];
		Segment.SegmentId = MakeSegmentId(
			Segment.SourcePreviewId,
			Segment.Index,
			Segment.Start,
			Segment.End);
		Candidate.Segments.Add(MoveTemp(Segment));
	}
	Candidate.ApexPosition = InSourcePreview.GetApexPosition();
	Candidate.PlannedLandingPosition =
		InSourcePreview.GetPlannedLandingPosition();
	Candidate.FlightTimeSeconds = InSourcePreview.GetFlightTimeSeconds();
	Candidate.PresentationStateId = MakeStateId(Candidate);
	if (Candidate.PresentationStateId != ExpectedPresentationStateId
		|| !Candidate.IsVisible())
	{
		return false;
	}
	OutState = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState::IsEmpty() const
{
	return !PresentationStateId.IsValid()
		&& Mode == EMode::Invalid
		&& !RunId.IsValid()
		&& !PlayerEntityId.IsValid()
		&& !SourceItemInstanceId.IsValid()
		&& !ChoiceState.IsValid()
		&& !SourceProductRequestId.IsValid()
		&& !SourcePreviewActivationId.IsValid()
		&& !SourcePreview.GetPreviewId().IsValid()
		&& !SourcePreview.GetPlan().IsValid()
		&& SourcePreview.GetSegmentCount() == 0
		&& SourcePreview.GetPositions().IsEmpty()
		&& SourcePreview.GetApexPosition().IsZero()
		&& SourcePreview.GetPlannedLandingPosition().IsZero()
		&& SourcePreview.GetFlightTimeSeconds() == 0.0
		&& Segments.IsEmpty()
		&& ApexPosition.IsZero()
		&& PlannedLandingPosition.IsZero()
		&& FlightTimeSeconds == 0.0;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState::IsValid() const
{
	if (!PresentationStateId.IsValid()
		|| !IsKnownMode(Mode)
		|| !RunId.IsValid()
		|| !PlayerEntityId.IsValid()
		|| !SourceItemInstanceId.IsValid()
		|| !ChoiceState.IsValid())
	{
		return false;
	}

	if (Mode == EMode::Hidden)
	{
		return !IsPreviewChoice(ChoiceState)
			&& !SourceProductRequestId.IsValid()
			&& !SourcePreviewActivationId.IsValid()
			&& !SourcePreview.IsValid()
			&& Segments.IsEmpty()
			&& ApexPosition.IsZero()
			&& PlannedLandingPosition.IsZero()
			&& FlightTimeSeconds == 0.0
			&& PresentationStateId == MakeStateId(*this);
	}

	if (!IsPreviewChoice(ChoiceState)
		|| !SourceProductRequestId.IsValid()
		|| !SourcePreviewActivationId.IsValid()
		|| !SourcePreview.IsValid()
		|| Segments.Num() != SourcePreview.GetSegmentCount()
		|| ApexPosition != SourcePreview.GetApexPosition()
		|| PlannedLandingPosition
			!= SourcePreview.GetPlannedLandingPosition()
		|| FlightTimeSeconds != SourcePreview.GetFlightTimeSeconds())
	{
		return false;
	}

	const FShanmenCombatActionSnapshot& Action =
		SourcePreview.GetPlan().GetRequest().GetAction();
	if (Action.GetRunId() != RunId
		|| Action.GetOwnerId() != PlayerEntityId
		|| Action.GetSourceEntityId() != PlayerEntityId
		|| Action.GetSourceItemInstanceId() != SourceItemInstanceId
		|| Action.GetActivationId() != SourcePreviewActivationId)
	{
		return false;
	}

	const TArray<FVector>& Positions = SourcePreview.GetPositions();
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const auto& Segment = Segments[Index];
		if (!Segment.IsValid()
			|| Segment.GetSourcePreviewId()
				!= SourcePreview.GetPreviewId()
			|| Segment.GetIndex() != Index
			|| Segment.GetStart() != Positions[Index]
			|| Segment.GetEnd() != Positions[Index + 1])
		{
			return false;
		}
	}
	return PresentationStateId == MakeStateId(*this);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState::IsVisible()
	const
{
	return IsValid() && Mode == EMode::Visible;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState::IsHidden()
	const
{
	return IsValid() && Mode == EMode::Hidden;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& PresentationStateId == Other.PresentationStateId;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult::
	IsProjected() const
{
	return Status == EProjectStatus::Projected
		&& !Diagnostic.IsEmpty()
		&& CompositionCount == 1
		&& Composition.IsComposed()
		&& State.IsVisible();
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Reject(
	const EProjectStatus Status,
	const TCHAR* Diagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
	const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult& Bridge,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis)
{
	if (!Bridge.IsCaptured())
	{
		return Reject(
			EProjectStatus::BridgeRejected,
			TEXT("Arc preview presentation requires one captured product bridge."));
	}
	if (!SourceBasis.IsValid())
	{
		return Reject(
			EProjectStatus::BasisRejected,
			TEXT("Arc preview presentation requires one immutable source basis."));
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult Result;
	Result.CompositionCount = 1;
	Result.Composition =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			Bridge.GetCapture().GetConfiguration(),
			[&Bridge]() { return Bridge.GetChoiceState(); },
			[&SourceBasis]() { return SourceBasis; });
	if (!Result.Composition.IsComposed())
	{
		Result.Status = EProjectStatus::CompositionRejected;
		Result.Diagnostic = Result.Composition.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview presentation geometry composition failed.")
			: Result.Composition.GetDiagnostic();
		return Result;
	}

	const auto& Capture = Bridge.GetCapture();
	const FShanmenThrownWeaponArcPreview& Preview =
		Result.Composition.GetPreview();
	const FShanmenCombatActionSnapshot& Action =
		Preview.GetPlan().GetRequest().GetAction();
	if (!Result.Composition.GetConfiguration().Matches(
			Capture.GetConfiguration())
		|| !Result.Composition.GetChoiceState().Matches(
			Bridge.GetChoiceState())
		|| Action.GetRunId() != Bridge.GetLifecycleRunId()
		|| Action.GetOwnerId() != Bridge.GetPlayerEntityId()
		|| Action.GetSourceEntityId() != Bridge.GetPlayerEntityId()
		|| Action.GetSourceItemInstanceId()
			!= Bridge.GetSourceItemInstanceId()
		|| Action.GetActivationId() != Capture.GetPreviewActivationId())
	{
		Result.Status = EProjectStatus::IdentityMismatch;
		Result.Diagnostic =
			TEXT("Arc preview bridge and composed geometry do not share one identity scope.");
		return Result;
	}

	Result.State.Mode = EMode::Visible;
	Result.State.RunId = Bridge.GetLifecycleRunId();
	Result.State.PlayerEntityId = Bridge.GetPlayerEntityId();
	Result.State.SourceItemInstanceId = Bridge.GetSourceItemInstanceId();
	Result.State.ChoiceState = Bridge.GetChoiceState();
	Result.State.SourceProductRequestId = Bridge.GetPreviewRequestId();
	Result.State.SourcePreviewActivationId =
		Capture.GetPreviewActivationId();
	Result.State.SourcePreview = Preview;
	Result.State.Segments.Reserve(Preview.GetSegmentCount());
	const TArray<FVector>& Positions = Preview.GetPositions();
	for (int32 Index = 0; Index < Preview.GetSegmentCount(); ++Index)
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment Segment;
		Segment.SourcePreviewId = Preview.GetPreviewId();
		Segment.Index = Index;
		Segment.Start = Positions[Index];
		Segment.End = Positions[Index + 1];
		Segment.SegmentId = MakeSegmentId(
			Segment.SourcePreviewId,
			Segment.Index,
			Segment.Start,
			Segment.End);
		Result.State.Segments.Add(MoveTemp(Segment));
	}
	Result.State.ApexPosition = Preview.GetApexPosition();
	Result.State.PlannedLandingPosition =
		Preview.GetPlannedLandingPosition();
	Result.State.FlightTimeSeconds = Preview.GetFlightTimeSeconds();
	Result.State.PresentationStateId = MakeStateId(Result.State);
	if (!Result.State.IsVisible())
	{
		Result.Status = EProjectStatus::StateRejected;
		Result.Diagnostic =
			TEXT("Arc preview presentation state failed deterministic validation.");
		return Result;
	}

	Result.Status = EProjectStatus::Projected;
	Result.Diagnostic =
		TEXT("Projected one captured Arc preview to renderer-neutral segments.");
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult::
	IsApplied() const
{
	return (Status == EReduceStatus::Replaced
			|| Status == EReduceStatus::Cleared)
		&& !Diagnostic.IsEmpty()
		&& State.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult::
	IsReplaced() const
{
	return IsApplied() && Status == EReduceStatus::Replaced
		&& State.IsVisible();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult::
	IsCleared() const
{
	return IsApplied() && Status == EReduceStatus::Cleared
		&& State.IsHidden();
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult::
	IsDuplicate() const
{
	return Status == EReduceStatus::Duplicate
		&& !Diagnostic.IsEmpty()
		&& State.IsValid();
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Reject(
	const EReduceStatus Status,
	const TCHAR* Diagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& PreviousState,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& CandidateState)
{
	if (!CandidateState.IsVisible())
	{
		return Reject(
			EReduceStatus::CandidateRejected,
			TEXT("Arc preview replacement requires one valid visible candidate."));
	}
	if (PreviousState.IsEmpty())
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Result;
		Result.Status = EReduceStatus::Replaced;
		Result.Diagnostic = TEXT("Installed the first Arc preview presentation.");
		Result.State = CandidateState;
		return Result;
	}
	if (!PreviousState.IsValid())
	{
		return Reject(
			EReduceStatus::PreviousStateInvalid,
			TEXT("Consumer-owned previous Arc preview state is invalid."));
	}
	if (!SameScope(PreviousState, CandidateState))
	{
		return Reject(
			EReduceStatus::IdentityMismatch,
			TEXT("Arc preview replacement rejects a foreign Run or item scope."));
	}
	if (CandidateState.Matches(PreviousState))
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Result;
		Result.Status = EReduceStatus::Duplicate;
		Result.Diagnostic = TEXT("Arc preview presentation is already current.");
		Result.State = PreviousState;
		return Result;
	}
	if (CandidateState.GetChoiceRevision() < PreviousState.GetChoiceRevision())
	{
		return Reject(
			EReduceStatus::StaleRevision,
			TEXT("Arc preview replacement rejects an older choice revision."));
	}
	if (CandidateState.GetChoiceRevision() == PreviousState.GetChoiceRevision())
	{
		return Reject(
			EReduceStatus::RevisionConflict,
			TEXT("One choice revision cannot name two Arc preview presentations."));
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Result;
	Result.Status = EReduceStatus::Replaced;
	Result.Diagnostic = TEXT("Replaced Arc preview with a newer choice revision.");
	Result.State = CandidateState;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult
Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& PreviousState,
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice)
{
	if (PreviousState.IsEmpty())
	{
		return Reject(
			EReduceStatus::PreviousStateRequired,
			TEXT("Arc preview clear requires one consumer-owned previous state."));
	}
	if (!PreviousState.IsValid())
	{
		return Reject(
			EReduceStatus::PreviousStateInvalid,
			TEXT("Consumer-owned previous Arc preview state is invalid."));
	}
	if (!CurrentChoice.IsValid())
	{
		return Reject(
			EReduceStatus::ChoiceRejected,
			TEXT("Arc preview clear requires one valid current choice."));
	}
	if (PreviousState.IsHidden()
		&& CurrentChoice.Matches(PreviousState.GetChoiceState()))
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Result;
		Result.Status = EReduceStatus::Duplicate;
		Result.Diagnostic = TEXT("Arc preview presentation is already cleared.");
		Result.State = PreviousState;
		return Result;
	}
	if (CurrentChoice.GetRevision() < PreviousState.GetChoiceRevision())
	{
		return Reject(
			EReduceStatus::StaleRevision,
			TEXT("Arc preview clear rejects an older choice revision."));
	}
	if (CurrentChoice.GetRevision() == PreviousState.GetChoiceRevision())
	{
		return Reject(
			EReduceStatus::RevisionConflict,
			TEXT("Arc preview clear requires a newer choice revision."));
	}
	if (IsPreviewChoice(CurrentChoice))
	{
		return Reject(
			EReduceStatus::ClearNotRequired,
			TEXT("A newer Arc choice with a target requires replacement, not clear."));
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Result;
	Result.State.Mode = EMode::Hidden;
	Result.State.RunId = PreviousState.GetRunId();
	Result.State.PlayerEntityId = PreviousState.GetPlayerEntityId();
	Result.State.SourceItemInstanceId =
		PreviousState.GetSourceItemInstanceId();
	Result.State.ChoiceState = CurrentChoice;
	Result.State.PresentationStateId = MakeStateId(Result.State);
	if (!Result.State.IsHidden())
	{
		return Reject(
			EReduceStatus::StateRejected,
			TEXT("Arc preview hidden tombstone failed deterministic validation."));
	}
	Result.Status = EReduceStatus::Cleared;
	Result.Diagnostic =
		TEXT("Cleared Arc preview and retained the newer choice revision.");
	return Result;
}
