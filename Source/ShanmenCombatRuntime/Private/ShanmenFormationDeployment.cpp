#include "ShanmenFormationDeployment.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	double CanonicalZero(const double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
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

	bool TryCanonicalForward(
		const FVector& Value,
		FVector& OutForward)
	{
		OutForward = FVector::ZeroVector;
		if (!IsFiniteVector(Value))
		{
			return false;
		}
		const FVector Planar(Value.X, Value.Y, 0.0);
		if (Planar.IsNearlyZero())
		{
			return false;
		}
		OutForward = Planar.GetSafeNormal();
		if (!IsFiniteVector(OutForward)
			|| !FMath::IsNearlyEqual(OutForward.SizeSquared(), 1.0))
		{
			OutForward = FVector::ZeroVector;
			return false;
		}
		OutForward.X = CanonicalZero(OutForward.X);
		OutForward.Y = CanonicalZero(OutForward.Y);
		OutForward.Z = 0.0;
		return true;
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	FGuid MakeDeploymentId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenFormationDiagramDefinition& Diagram,
		const FVector& Origin,
		const FVector& Forward)
	{
		if (!Action.IsValid() || !Diagram.IsValid()
			|| !IsFiniteVector(Origin) || !IsFiniteVector(Forward))
		{
			return FGuid();
		}
		TArray<FString> Parts = {
				GuidDigits(Action.GetRunId()),
				GuidDigits(Action.GetOwnerId()),
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				Action.GetActionDefinitionId().ToString(),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest,
				Diagram.GetDiagramDefinitionId().ToString(),
				DoubleBits(Origin.X),
				DoubleBits(Origin.Y),
				DoubleBits(Origin.Z),
				DoubleBits(Forward.X),
				DoubleBits(Forward.Y),
				DoubleBits(Forward.Z),
				FString::FromInt(Diagram.GetAnchors().Num())
			};
		for (const FShanmenFormationAnchorDefinition& Anchor :
			Diagram.GetAnchors())
		{
			Parts.Append({
				FString::FromInt(Anchor.GetOrder()),
				Anchor.GetAnchorDefinitionId().ToString(),
				DoubleBits(Anchor.GetRelativeOffset().X),
				DoubleBits(Anchor.GetRelativeOffset().Y),
				DoubleBits(Anchor.GetRelativeOffset().Z),
				FString::FromInt(Anchor.GetRequirements().Num())
			});
			for (const FShanmenFormationMaterialRequirement& Requirement :
				Anchor.GetRequirements())
			{
				Parts.Append({
					FString::FromInt(Requirement.GetOrder()),
					Requirement.GetMaterialDefinitionId().ToString(),
					FString::FromInt(Requirement.GetQuantity())
				});
			}
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.Deployment.r1"), Parts);
	}

	FGuid MakeAnchorInstanceId(
		const FGuid& DeploymentId,
		const FShanmenFormationAnchorDefinition& Anchor)
	{
		if (!DeploymentId.IsValid() || !Anchor.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.AnchorInstance.r1"),
			{
				GuidDigits(DeploymentId),
				FString::FromInt(Anchor.GetOrder()),
				Anchor.GetAnchorDefinitionId().ToString()
			});
	}

	FVector MakeAnchorWorldLocation(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& RelativeOffset)
	{
		const FVector Right = FVector::CrossProduct(
			FVector::UpVector, Forward);
		return Origin
			+ Forward * RelativeOffset.X
			+ Right * RelativeOffset.Y
			+ FVector::UpVector * RelativeOffset.Z;
	}

	FGuid MakeSimpleReceiptId(
		const FGuid& DeploymentId,
		const EShanmenFormationDeploymentEventKind Kind)
	{
		if (!DeploymentId.IsValid()
			|| Kind == EShanmenFormationDeploymentEventKind::CommitAnchor)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.DeploymentEvent.r1"),
			{
				GuidDigits(DeploymentId),
				FString::FromInt(static_cast<int32>(Kind))
			});
	}

	FGuid MakeCommitReceiptId(
		const FShanmenFormationAnchorFulfillmentEvidence& Evidence)
	{
		if (!Evidence.IsValid())
		{
			return FGuid();
		}
		TArray<FShanmenFormationMaterialFulfillmentLine> CanonicalLines =
			Evidence.Lines;
		CanonicalLines.Sort([](
			const FShanmenFormationMaterialFulfillmentLine& Left,
			const FShanmenFormationMaterialFulfillmentLine& Right)
		{
			if (Left.MaterialDefinitionId != Right.MaterialDefinitionId)
			{
				return Left.MaterialDefinitionId.LexicalLess(
					Right.MaterialDefinitionId);
			}
			const FString LeftId = GuidDigits(Left.ItemInstanceId);
			const FString RightId = GuidDigits(Right.ItemInstanceId);
			if (LeftId != RightId)
			{
				return LeftId < RightId;
			}
			return Left.Quantity < Right.Quantity;
		});

		TArray<FString> Parts = {
			GuidDigits(Evidence.DeploymentId),
			Evidence.AnchorDefinitionId.ToString(),
			GuidDigits(Evidence.FulfillmentId),
			GuidDigits(Evidence.RunId),
			GuidDigits(Evidence.OwnerId),
			Evidence.Content.Version.ToString(),
			Evidence.Content.Digest,
			FString::FromInt(Evidence.AuthorityRevision)
		};
		Parts.Add(FString::FromInt(CanonicalLines.Num()));
		for (const FShanmenFormationMaterialFulfillmentLine& Line :
			CanonicalLines)
		{
			Parts.Append({
				Line.MaterialDefinitionId.ToString(),
				GuidDigits(Line.ItemInstanceId),
				FString::FromInt(Line.Quantity)
			});
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.AnchorCommit.r1"), Parts);
	}

	bool ContentMatches(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}
}

bool FShanmenFormationMaterialRequirement::IsValid() const
{
	return Order >= 0 && !MaterialDefinitionId.IsNone() && Quantity > 0;
}

bool FShanmenFormationAnchorDefinition::IsValid() const
{
	if (Order < 0 || AnchorDefinitionId.IsNone()
		|| !IsFiniteVector(RelativeOffset) || Requirements.IsEmpty())
	{
		return false;
	}
	TSet<FName> DefinitionIds;
	for (int32 Index = 0; Index < Requirements.Num(); ++Index)
	{
		const FShanmenFormationMaterialRequirement& Requirement =
			Requirements[Index];
		if (!Requirement.IsValid() || Requirement.GetOrder() != Index
			|| DefinitionIds.Contains(
				Requirement.GetMaterialDefinitionId()))
		{
			return false;
		}
		DefinitionIds.Add(Requirement.GetMaterialDefinitionId());
	}
	return true;
}

FName FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Formation.Deploy");
}

bool FShanmenFormationDiagramDefinition::TryCapture(
	const FShanmenFormationDiagramCapture& Capture,
	FShanmenFormationDiagramDefinition& OutDefinition)
{
	OutDefinition = FShanmenFormationDiagramDefinition();
	if (Capture.ActionDefinitionId != CanonicalActionDefinitionId()
		|| Capture.DiagramDefinitionId.IsNone()
		|| Capture.Anchors.IsEmpty())
	{
		return false;
	}

	TArray<FShanmenFormationAnchorCapture> OrderedAnchors = Capture.Anchors;
	OrderedAnchors.Sort([](
		const FShanmenFormationAnchorCapture& Left,
		const FShanmenFormationAnchorCapture& Right)
	{
		if (Left.Order != Right.Order)
		{
			return Left.Order < Right.Order;
		}
		return Left.AnchorDefinitionId.LexicalLess(
			Right.AnchorDefinitionId);
	});

	OutDefinition.ActionDefinitionId = Capture.ActionDefinitionId;
	OutDefinition.DiagramDefinitionId = Capture.DiagramDefinitionId;
	TSet<FName> AnchorIds;
	for (int32 AnchorIndex = 0;
		AnchorIndex < OrderedAnchors.Num();
		++AnchorIndex)
	{
		const FShanmenFormationAnchorCapture& Source =
			OrderedAnchors[AnchorIndex];
		if (Source.Order != AnchorIndex
			|| Source.AnchorDefinitionId.IsNone()
			|| !IsFiniteVector(Source.RelativeOffset)
			|| Source.Requirements.IsEmpty()
			|| AnchorIds.Contains(Source.AnchorDefinitionId))
		{
			OutDefinition = FShanmenFormationDiagramDefinition();
			return false;
		}
		AnchorIds.Add(Source.AnchorDefinitionId);

		FShanmenFormationAnchorDefinition Anchor;
		Anchor.Order = Source.Order;
		Anchor.AnchorDefinitionId = Source.AnchorDefinitionId;
		Anchor.RelativeOffset = Source.RelativeOffset;
		Anchor.RelativeOffset.X = CanonicalZero(Anchor.RelativeOffset.X);
		Anchor.RelativeOffset.Y = CanonicalZero(Anchor.RelativeOffset.Y);
		Anchor.RelativeOffset.Z = CanonicalZero(Anchor.RelativeOffset.Z);
		TArray<FShanmenFormationMaterialRequirementCapture>
			OrderedRequirements = Source.Requirements;
		OrderedRequirements.Sort([](
			const FShanmenFormationMaterialRequirementCapture& Left,
			const FShanmenFormationMaterialRequirementCapture& Right)
		{
			if (Left.Order != Right.Order)
			{
				return Left.Order < Right.Order;
			}
			return Left.MaterialDefinitionId.LexicalLess(
				Right.MaterialDefinitionId);
		});

		TSet<FName> MaterialIds;
		for (int32 RequirementIndex = 0;
			RequirementIndex < OrderedRequirements.Num();
			++RequirementIndex)
		{
			const FShanmenFormationMaterialRequirementCapture&
				RequirementSource = OrderedRequirements[RequirementIndex];
			if (RequirementSource.Order != RequirementIndex
				|| RequirementSource.MaterialDefinitionId.IsNone()
				|| RequirementSource.Quantity <= 0
				|| MaterialIds.Contains(
					RequirementSource.MaterialDefinitionId))
			{
				OutDefinition = FShanmenFormationDiagramDefinition();
				return false;
			}
			MaterialIds.Add(RequirementSource.MaterialDefinitionId);

			FShanmenFormationMaterialRequirement Requirement;
			Requirement.Order = RequirementSource.Order;
			Requirement.MaterialDefinitionId =
				RequirementSource.MaterialDefinitionId;
			Requirement.Quantity = RequirementSource.Quantity;
			Anchor.Requirements.Add(Requirement);
		}
		OutDefinition.Anchors.Add(Anchor);
	}

	if (!OutDefinition.IsValid())
	{
		OutDefinition = FShanmenFormationDiagramDefinition();
		return false;
	}
	return true;
}

bool FShanmenFormationDiagramDefinition::IsValid() const
{
	if (ActionDefinitionId != CanonicalActionDefinitionId()
		|| DiagramDefinitionId.IsNone() || Anchors.IsEmpty())
	{
		return false;
	}
	TSet<FName> AnchorIds;
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		const FShanmenFormationAnchorDefinition& Anchor = Anchors[Index];
		if (!Anchor.IsValid() || Anchor.GetOrder() != Index
			|| AnchorIds.Contains(Anchor.GetAnchorDefinitionId()))
		{
			return false;
		}
		AnchorIds.Add(Anchor.GetAnchorDefinitionId());
	}
	return true;
}

const FShanmenFormationAnchorDefinition*
FShanmenFormationDiagramDefinition::FindAnchor(
	const FName AnchorDefinitionId) const
{
	return Anchors.FindByPredicate(
		[AnchorDefinitionId](
			const FShanmenFormationAnchorDefinition& Candidate)
		{
			return Candidate.GetAnchorDefinitionId() == AnchorDefinitionId;
		});
}

bool FShanmenFormationMaterialFulfillmentLine::IsValid() const
{
	return ItemInstanceId.IsValid()
		&& !MaterialDefinitionId.IsNone()
		&& Quantity > 0;
}

bool FShanmenFormationAnchorFulfillmentEvidence::IsValid() const
{
	if (!FulfillmentId.IsValid() || !RunId.IsValid() || !OwnerId.IsValid()
		|| !DeploymentId.IsValid() || AnchorDefinitionId.IsNone()
		|| !Content.IsValid() || AuthorityRevision < 0 || Lines.IsEmpty())
	{
		return false;
	}
	TSet<FGuid> ItemIds;
	for (const FShanmenFormationMaterialFulfillmentLine& Line : Lines)
	{
		if (!Line.IsValid() || ItemIds.Contains(Line.ItemInstanceId))
		{
			return false;
		}
		ItemIds.Add(Line.ItemInstanceId);
	}
	return true;
}

bool FShanmenFormationDeploymentReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !DeploymentId.IsValid()
		|| Sequence < 0 || CommittedAnchorCount < 0
		|| (StateBefore == StateAfter
			&& EventKind
				!= EShanmenFormationDeploymentEventKind::CommitAnchor)
		|| StateBefore == EShanmenFormationDeploymentState::Uninitialized
		|| StateAfter == EShanmenFormationDeploymentState::Uninitialized)
	{
		return false;
	}

	switch (EventKind)
	{
	case EShanmenFormationDeploymentEventKind::Begin:
		return StateBefore == EShanmenFormationDeploymentState::Planned
			&& StateAfter == EShanmenFormationDeploymentState::Deploying
			&& AnchorDefinitionId.IsNone()
			&& !FulfillmentId.IsValid()
			&& AuthorityRevision == INDEX_NONE
			&& CommittedAnchorCount == 0;

	case EShanmenFormationDeploymentEventKind::CommitAnchor:
		return StateBefore == EShanmenFormationDeploymentState::Deploying
			&& (StateAfter == EShanmenFormationDeploymentState::Deploying
				|| StateAfter == EShanmenFormationDeploymentState::Active)
			&& !AnchorDefinitionId.IsNone()
			&& FulfillmentId.IsValid()
			&& AuthorityRevision >= 0
			&& CommittedAnchorCount > 0;

	case EShanmenFormationDeploymentEventKind::Cancel:
		return (StateBefore == EShanmenFormationDeploymentState::Planned
				|| StateBefore == EShanmenFormationDeploymentState::Deploying)
			&& StateAfter == EShanmenFormationDeploymentState::Cancelled
			&& AnchorDefinitionId.IsNone()
			&& !FulfillmentId.IsValid()
			&& AuthorityRevision == INDEX_NONE;

	case EShanmenFormationDeploymentEventKind::End:
		return StateBefore == EShanmenFormationDeploymentState::Active
			&& StateAfter == EShanmenFormationDeploymentState::Ended
			&& AnchorDefinitionId.IsNone()
			&& !FulfillmentId.IsValid()
			&& AuthorityRevision == INDEX_NONE;
	}
	return false;
}

bool FShanmenFormationDeployment::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenFormationDiagramDefinition& Diagram,
	const FVector& Origin,
	const FVector& Forward,
	FShanmenFormationDeployment& OutDeployment)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	const FShanmenFormationDiagramDefinition FrozenDiagram = Diagram;
	OutDeployment.Reset();
	FVector CanonicalForward;
	if (!FrozenAction.IsValid() || !FrozenDiagram.IsValid()
		|| FrozenAction.GetActionDefinitionId()
			!= FrozenDiagram.GetActionDefinitionId()
		|| !IsFiniteVector(Origin)
		|| !TryCanonicalForward(Forward, CanonicalForward))
	{
		return false;
	}

	OutDeployment.Action = FrozenAction;
	OutDeployment.Diagram = FrozenDiagram;
	OutDeployment.Origin = Origin;
	OutDeployment.Forward = CanonicalForward;
	OutDeployment.DeploymentId = MakeDeploymentId(
		FrozenAction, FrozenDiagram, Origin, CanonicalForward);
	if (!OutDeployment.DeploymentId.IsValid())
	{
		OutDeployment.Reset();
		return false;
	}

	for (const FShanmenFormationAnchorDefinition& Anchor :
		FrozenDiagram.GetAnchors())
	{
		FShanmenFormationAnchorProgress Progress;
		Progress.AnchorDefinitionId = Anchor.GetAnchorDefinitionId();
		Progress.AnchorInstanceId = MakeAnchorInstanceId(
			OutDeployment.DeploymentId, Anchor);
		Progress.WorldLocation = MakeAnchorWorldLocation(
			Origin, CanonicalForward, Anchor.GetRelativeOffset());
		if (!Progress.AnchorInstanceId.IsValid()
			|| !IsFiniteVector(Progress.WorldLocation))
		{
			OutDeployment.Reset();
			return false;
		}
		OutDeployment.Anchors.Add(Progress);
	}
	OutDeployment.State = EShanmenFormationDeploymentState::Planned;
	return OutDeployment.IsValid();
}

bool FShanmenFormationDeployment::IsValid() const
{
	if (!Action.IsValid() || !Diagram.IsValid()
		|| Action.GetActionDefinitionId() != Diagram.GetActionDefinitionId()
		|| !DeploymentId.IsValid() || !IsFiniteVector(Origin)
		|| !IsFiniteVector(Forward)
		|| State == EShanmenFormationDeploymentState::Uninitialized
		|| Anchors.Num() != Diagram.GetAnchors().Num()
		|| CommittedAnchorCount < 0
		|| CommittedAnchorCount > Anchors.Num()
		|| NextReceiptSequence != Receipts.Num()
		|| DeploymentId != MakeDeploymentId(Action, Diagram, Origin, Forward))
	{
		return false;
	}

	FVector CanonicalForward;
	if (!TryCanonicalForward(Forward, CanonicalForward)
		|| CanonicalForward != Forward)
	{
		return false;
	}

	int32 CountedCommitted = 0;
	TSet<FGuid> AnchorInstanceIds;
	for (int32 Index = 0; Index < Anchors.Num(); ++Index)
	{
		const FShanmenFormationAnchorDefinition& Definition =
			Diagram.GetAnchors()[Index];
		const FShanmenFormationAnchorProgress& Progress = Anchors[Index];
		if (Progress.AnchorDefinitionId != Definition.GetAnchorDefinitionId()
			|| Progress.AnchorInstanceId
				!= MakeAnchorInstanceId(DeploymentId, Definition)
			|| Progress.WorldLocation
				!= MakeAnchorWorldLocation(
					Origin, Forward, Definition.GetRelativeOffset())
			|| AnchorInstanceIds.Contains(Progress.AnchorInstanceId))
		{
			return false;
		}
		AnchorInstanceIds.Add(Progress.AnchorInstanceId);
		if (Progress.bCommitted)
		{
			++CountedCommitted;
			if (!Progress.Fulfillment.IsValid()
				|| Progress.Fulfillment.DeploymentId != DeploymentId
				|| Progress.Fulfillment.AnchorDefinitionId
					!= Progress.AnchorDefinitionId
				|| !Progress.CommitReceiptId.IsValid()
				|| Progress.CommitReceiptId
					!= MakeCommitReceiptId(Progress.Fulfillment))
			{
				return false;
			}
		}
		else if (Progress.Fulfillment.IsValid()
			|| Progress.CommitReceiptId.IsValid())
		{
			return false;
		}
	}
	if (CountedCommitted != CommittedAnchorCount)
	{
		return false;
	}

	EShanmenFormationDeploymentState SimulatedState =
		EShanmenFormationDeploymentState::Planned;
	int32 SimulatedCommitted = 0;
	TSet<FName> SimulatedAnchors;
	for (int32 Index = 0; Index < Receipts.Num(); ++Index)
	{
		const FShanmenFormationDeploymentReceipt& Receipt = Receipts[Index];
		if (!Receipt.IsValid() || Receipt.Sequence != Index
			|| Receipt.DeploymentId != DeploymentId
			|| Receipt.StateBefore != SimulatedState)
		{
			return false;
		}
		switch (Receipt.EventKind)
		{
		case EShanmenFormationDeploymentEventKind::Begin:
			if (Receipt.ReceiptId != MakeSimpleReceiptId(
					DeploymentId, Receipt.EventKind))
			{
				return false;
			}
			break;

		case EShanmenFormationDeploymentEventKind::CommitAnchor:
		{
			const FShanmenFormationAnchorProgress* Progress =
				Anchors.FindByPredicate(
					[&Receipt](
						const FShanmenFormationAnchorProgress& Candidate)
					{
						return Candidate.AnchorDefinitionId
							== Receipt.AnchorDefinitionId;
					});
			if (!Progress || !Progress->bCommitted
				|| SimulatedAnchors.Contains(Receipt.AnchorDefinitionId)
				|| Progress->CommitReceiptId != Receipt.ReceiptId
				|| Progress->Fulfillment.FulfillmentId
					!= Receipt.FulfillmentId
				|| Progress->Fulfillment.AuthorityRevision
					!= Receipt.AuthorityRevision)
			{
				return false;
			}
			SimulatedAnchors.Add(Receipt.AnchorDefinitionId);
			++SimulatedCommitted;
			break;
		}

		case EShanmenFormationDeploymentEventKind::Cancel:
		case EShanmenFormationDeploymentEventKind::End:
			if (Receipt.ReceiptId != MakeSimpleReceiptId(
					DeploymentId, Receipt.EventKind))
			{
				return false;
			}
			break;
		}
		if (Receipt.CommittedAnchorCount != SimulatedCommitted)
		{
			return false;
		}
		EShanmenFormationDeploymentState ExpectedAfter = Receipt.StateAfter;
		if (Receipt.EventKind
			== EShanmenFormationDeploymentEventKind::CommitAnchor)
		{
			ExpectedAfter = SimulatedCommitted == Anchors.Num()
				? EShanmenFormationDeploymentState::Active
				: EShanmenFormationDeploymentState::Deploying;
		}
		if (Receipt.StateAfter != ExpectedAfter)
		{
			return false;
		}
		SimulatedState = Receipt.StateAfter;
	}

	if (SimulatedState != State || SimulatedCommitted != CommittedAnchorCount)
	{
		return false;
	}
	if (State == EShanmenFormationDeploymentState::Planned)
	{
		return Receipts.IsEmpty() && CommittedAnchorCount == 0;
	}
	if (State == EShanmenFormationDeploymentState::Deploying)
	{
		return CommittedAnchorCount < Anchors.Num();
	}
	if (State == EShanmenFormationDeploymentState::Active
		|| State == EShanmenFormationDeploymentState::Ended)
	{
		return CommittedAnchorCount == Anchors.Num();
	}
	return State == EShanmenFormationDeploymentState::Cancelled;
}

bool FShanmenFormationDeployment::TryBeginDeployment(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenFormationDeploymentReceipt& OutReceipt)
{
	OutReceipt = FShanmenFormationDeploymentReceipt();
	const FGuid ReceiptId = MakeSimpleReceiptId(
		DeploymentId, EShanmenFormationDeploymentEventKind::Begin);
	if (const FShanmenFormationDeploymentReceipt* Existing =
		FindReceipt(ReceiptId))
	{
		if (!IsValid() || !MatchesActionRuntime(ActionRuntime))
		{
			return false;
		}
		OutReceipt = *Existing;
		return true;
	}
	if (!IsValid() || State != EShanmenFormationDeploymentState::Planned
		|| !MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates())
	{
		return false;
	}

	const FShanmenFormationDeployment Before = *this;
	OutReceipt.ReceiptId = ReceiptId;
	OutReceipt.DeploymentId = DeploymentId;
	OutReceipt.Sequence = NextReceiptSequence++;
	OutReceipt.EventKind = EShanmenFormationDeploymentEventKind::Begin;
	OutReceipt.StateBefore = State;
	State = EShanmenFormationDeploymentState::Deploying;
	OutReceipt.StateAfter = State;
	OutReceipt.CommittedAnchorCount = CommittedAnchorCount;
	Receipts.Add(OutReceipt);
	if (!OutReceipt.IsValid() || !IsValid())
	{
		*this = Before;
		OutReceipt = FShanmenFormationDeploymentReceipt();
		return false;
	}
	return true;
}

bool FShanmenFormationDeployment::TryCommitAnchor(
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenFormationAnchorFulfillmentEvidence& Evidence,
	FShanmenFormationDeploymentReceipt& OutReceipt)
{
	OutReceipt = FShanmenFormationDeploymentReceipt();
	if (!Evidence.IsValid())
	{
		return false;
	}
	const FGuid ReceiptId = MakeCommitReceiptId(Evidence);
	if (const FShanmenFormationDeploymentReceipt* Existing =
		FindReceipt(ReceiptId))
	{
		if (!IsValid() || !MatchesActionRuntime(ActionRuntime))
		{
			return false;
		}
		OutReceipt = *Existing;
		return true;
	}
	if (!IsValid() || State != EShanmenFormationDeploymentState::Deploying
		|| !MatchesActionRuntime(ActionRuntime)
		|| !ActionRuntime.CanEmitCandidates()
		|| Evidence.RunId != Action.GetRunId()
		|| Evidence.OwnerId != Action.GetOwnerId()
		|| Evidence.DeploymentId != DeploymentId
		|| !ContentMatches(Evidence.Content, Action.GetContent()))
	{
		return false;
	}
	const FShanmenFormationAnchorDefinition* Anchor =
		Diagram.FindAnchor(Evidence.AnchorDefinitionId);
	FShanmenFormationAnchorProgress* Progress =
		FindAnchorProgress(Evidence.AnchorDefinitionId);
	if (!Anchor || !Progress || Progress->bCommitted
		|| !ValidateFulfillment(*Anchor, Evidence))
	{
		return false;
	}

	const FShanmenFormationDeployment Before = *this;
	const EShanmenFormationDeploymentState StateBefore = State;
	Progress->bCommitted = true;
	Progress->Fulfillment = Evidence;
	Progress->CommitReceiptId = ReceiptId;
	++CommittedAnchorCount;
	if (CommittedAnchorCount == Anchors.Num())
	{
		State = EShanmenFormationDeploymentState::Active;
	}

	OutReceipt.ReceiptId = ReceiptId;
	OutReceipt.DeploymentId = DeploymentId;
	OutReceipt.Sequence = NextReceiptSequence++;
	OutReceipt.EventKind =
		EShanmenFormationDeploymentEventKind::CommitAnchor;
	OutReceipt.StateBefore = StateBefore;
	OutReceipt.StateAfter = State;
	OutReceipt.AnchorDefinitionId = Evidence.AnchorDefinitionId;
	OutReceipt.FulfillmentId = Evidence.FulfillmentId;
	OutReceipt.AuthorityRevision = Evidence.AuthorityRevision;
	OutReceipt.CommittedAnchorCount = CommittedAnchorCount;
	Receipts.Add(OutReceipt);
	if (!OutReceipt.IsValid() || !IsValid())
	{
		*this = Before;
		OutReceipt = FShanmenFormationDeploymentReceipt();
		return false;
	}
	return true;
}

bool FShanmenFormationDeployment::TryCancel(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenFormationDeploymentReceipt& OutReceipt)
{
	OutReceipt = FShanmenFormationDeploymentReceipt();
	const FGuid ReceiptId = MakeSimpleReceiptId(
		DeploymentId, EShanmenFormationDeploymentEventKind::Cancel);
	if (const FShanmenFormationDeploymentReceipt* Existing =
		FindReceipt(ReceiptId))
	{
		if (!IsValid() || !MatchesActionRuntime(ActionRuntime))
		{
			return false;
		}
		OutReceipt = *Existing;
		return true;
	}
	if (!IsValid()
		|| (State != EShanmenFormationDeploymentState::Planned
			&& State != EShanmenFormationDeploymentState::Deploying)
		|| !MatchesActionRuntime(ActionRuntime))
	{
		return false;
	}

	const FShanmenFormationDeployment Before = *this;
	OutReceipt.ReceiptId = ReceiptId;
	OutReceipt.DeploymentId = DeploymentId;
	OutReceipt.Sequence = NextReceiptSequence++;
	OutReceipt.EventKind = EShanmenFormationDeploymentEventKind::Cancel;
	OutReceipt.StateBefore = State;
	State = EShanmenFormationDeploymentState::Cancelled;
	OutReceipt.StateAfter = State;
	OutReceipt.CommittedAnchorCount = CommittedAnchorCount;
	Receipts.Add(OutReceipt);
	if (!OutReceipt.IsValid() || !IsValid())
	{
		*this = Before;
		OutReceipt = FShanmenFormationDeploymentReceipt();
		return false;
	}
	return true;
}

bool FShanmenFormationDeployment::TryEnd(
	const FShanmenActionOrchestrator& ActionRuntime,
	FShanmenFormationDeploymentReceipt& OutReceipt)
{
	OutReceipt = FShanmenFormationDeploymentReceipt();
	const FGuid ReceiptId = MakeSimpleReceiptId(
		DeploymentId, EShanmenFormationDeploymentEventKind::End);
	if (const FShanmenFormationDeploymentReceipt* Existing =
		FindReceipt(ReceiptId))
	{
		if (!IsValid() || !MatchesActionRuntime(ActionRuntime))
		{
			return false;
		}
		OutReceipt = *Existing;
		return true;
	}
	if (!IsValid() || State != EShanmenFormationDeploymentState::Active
		|| !MatchesActionRuntime(ActionRuntime))
	{
		return false;
	}

	const FShanmenFormationDeployment Before = *this;
	OutReceipt.ReceiptId = ReceiptId;
	OutReceipt.DeploymentId = DeploymentId;
	OutReceipt.Sequence = NextReceiptSequence++;
	OutReceipt.EventKind = EShanmenFormationDeploymentEventKind::End;
	OutReceipt.StateBefore = State;
	State = EShanmenFormationDeploymentState::Ended;
	OutReceipt.StateAfter = State;
	OutReceipt.CommittedAnchorCount = CommittedAnchorCount;
	Receipts.Add(OutReceipt);
	if (!OutReceipt.IsValid() || !IsValid())
	{
		*this = Before;
		OutReceipt = FShanmenFormationDeploymentReceipt();
		return false;
	}
	return true;
}

void FShanmenFormationDeployment::Reset()
{
	*this = FShanmenFormationDeployment();
}

bool FShanmenFormationDeployment::MatchesActionRuntime(
	const FShanmenActionOrchestrator& ActionRuntime) const
{
	return ActionRuntime.IsValid()
		&& ActionsMatch(Action, ActionRuntime.GetAction());
}

const FShanmenFormationDeploymentReceipt*
FShanmenFormationDeployment::FindReceipt(const FGuid& ReceiptId) const
{
	return ReceiptId.IsValid()
		? Receipts.FindByPredicate(
			[&ReceiptId](
				const FShanmenFormationDeploymentReceipt& Candidate)
			{
				return Candidate.GetReceiptId() == ReceiptId;
			})
		: nullptr;
}

FShanmenFormationAnchorProgress*
FShanmenFormationDeployment::FindAnchorProgress(
	const FName AnchorDefinitionId)
{
	return Anchors.FindByPredicate(
		[AnchorDefinitionId](
			FShanmenFormationAnchorProgress& Candidate)
		{
			return Candidate.GetAnchorDefinitionId() == AnchorDefinitionId;
		});
}

bool FShanmenFormationDeployment::ValidateFulfillment(
	const FShanmenFormationAnchorDefinition& Anchor,
	const FShanmenFormationAnchorFulfillmentEvidence& Evidence) const
{
	if (!Anchor.IsValid() || !Evidence.IsValid()
		|| Evidence.AnchorDefinitionId != Anchor.GetAnchorDefinitionId())
	{
		return false;
	}
	TMap<FName, int64> FulfilledQuantities;
	for (const FShanmenFormationMaterialFulfillmentLine& Line :
		Evidence.Lines)
	{
		FulfilledQuantities.FindOrAdd(Line.MaterialDefinitionId) +=
			Line.Quantity;
	}
	if (FulfilledQuantities.Num() != Anchor.GetRequirements().Num())
	{
		return false;
	}
	for (const FShanmenFormationMaterialRequirement& Requirement :
		Anchor.GetRequirements())
	{
		const int64* Fulfilled = FulfilledQuantities.Find(
			Requirement.GetMaterialDefinitionId());
		if (!Fulfilled || *Fulfilled != Requirement.GetQuantity())
		{
			return false;
		}
	}
	return true;
}
