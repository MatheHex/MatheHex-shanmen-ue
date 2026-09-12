#include "demo_mapShanmenFormationProductAuthority.h"

#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& SameContent(Left.GetContent(), Right.GetContent())
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool RequirementsMatch(
		const FShanmenFormationMaterialRequirement& Left,
		const FShanmenFormationMaterialRequirement& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetOrder() == Right.GetOrder()
			&& Left.GetMaterialDefinitionId()
				== Right.GetMaterialDefinitionId()
			&& Left.GetQuantity() == Right.GetQuantity();
	}

	bool AnchorsMatch(
		const FShanmenFormationAnchorDefinition& Left,
		const FShanmenFormationAnchorDefinition& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.GetOrder() != Right.GetOrder()
			|| Left.GetAnchorDefinitionId() != Right.GetAnchorDefinitionId()
			|| Left.GetRelativeOffset() != Right.GetRelativeOffset()
			|| Left.GetRequirements().Num() != Right.GetRequirements().Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.GetRequirements().Num(); ++Index)
		{
			if (!RequirementsMatch(
					Left.GetRequirements()[Index],
					Right.GetRequirements()[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool DiagramsMatch(
		const FShanmenFormationDiagramDefinition& Left,
		const FShanmenFormationDiagramDefinition& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.GetActionDefinitionId() != Right.GetActionDefinitionId()
			|| Left.GetDiagramDefinitionId() != Right.GetDiagramDefinitionId()
			|| Left.GetActivationEnergyCost().GetCostId()
				!= Right.GetActivationEnergyCost().GetCostId()
			|| Left.GetAnchors().Num() != Right.GetAnchors().Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.GetAnchors().Num(); ++Index)
		{
			if (!AnchorsMatch(
					Left.GetAnchors()[Index], Right.GetAnchors()[Index]))
			{
				return false;
			}
		}
		return true;
	}
}

bool Fdemo_mapPlayerFormationActionReservation::IsValid() const
{
	FGameplayTagContainer ExpectedSourceTags;
	ExpectedSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	return ActivationSequence > 0
		&& Action.IsValid()
		&& Action.GetOwnerId().IsValid()
		&& !Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenFormationDiagramDefinition::
				CanonicalActionDefinitionId()
		&& Action.GetContent().IsValid()
		&& Action.GetSourceTags() == ExpectedSourceTags
		&& Action.GetActivationId() == FShanmenCombatIdFactory::MakeActivationId(
			Action.GetRunId(),
			Action.GetSourceEntityId(),
			Action.GetActionDefinitionId(),
			ActivationSequence);
}

bool Fdemo_mapShanmenFormationDeploymentCommand::TryCapture(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	const Fdemo_mapPlayerFormationActionReservation& RequestedReservation,
	const FShanmenFormationDiagramDefinition& RequestedDiagram,
	const FVector& RequestedOrigin,
	const FVector& RequestedForward,
	Fdemo_mapShanmenFormationDeploymentCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenFormationDeploymentCommand();
	if (!RequestedCorrelation.IsValid()
		|| !RequestedReservation.IsValid()
		|| !RequestedDiagram.IsValid()
		|| RequestedCorrelation.ActiveRunId
			!= RequestedReservation.GetAction().GetRunId()
		|| RequestedCorrelation.OwnerId
			!= RequestedReservation.GetAction().GetOwnerId()
		|| RequestedDiagram.GetActionDefinitionId()
			!= RequestedReservation.GetAction().GetActionDefinitionId()
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedForward)
		|| RequestedForward.IsNearlyZero())
	{
		return false;
	}

	OutCommand.Correlation = RequestedCorrelation;
	OutCommand.Reservation = RequestedReservation;
	OutCommand.Diagram = RequestedDiagram;
	OutCommand.Origin = RequestedOrigin;
	OutCommand.Forward = RequestedForward.GetSafeNormal();
	if (!OutCommand.IsValid())
	{
		OutCommand = Fdemo_mapShanmenFormationDeploymentCommand();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenFormationDeploymentCommand::IsValid() const
{
	return Correlation.IsValid()
		&& Reservation.IsValid()
		&& Diagram.IsValid()
		&& Correlation.ActiveRunId == GetAction().GetRunId()
		&& Correlation.OwnerId == GetAction().GetOwnerId()
		&& Diagram.GetActionDefinitionId()
			== GetAction().GetActionDefinitionId()
		&& IsFiniteVector(Origin)
		&& IsFiniteVector(Forward)
		&& Forward.IsNormalized();
}

bool Fdemo_mapShanmenFormationDeploymentCommand::Matches(
	const Fdemo_mapShanmenFormationDeploymentCommand& Other) const
{
	return IsValid() && Other.IsValid()
		&& Correlation == Other.Correlation
		&& Reservation.GetActivationSequence()
			== Other.Reservation.GetActivationSequence()
		&& ActionsMatch(GetAction(), Other.GetAction())
		&& DiagramsMatch(Diagram, Other.Diagram)
		&& Origin == Other.Origin
		&& Forward == Other.Forward;
}

bool Fdemo_mapShanmenFormationProductPreparationResult::IsReady() const
{
	return Status == Edemo_mapShanmenFormationProductPreparationStatus::Ready
		&& AuthorityRevision >= Correlation.LifecycleAuthorityRevision
		&& Correlation.IsValid()
		&& Reservation.IsValid()
		&& Command.IsValid()
		&& Command.GetCorrelation() == Correlation
		&& Command.GetReservation().GetActivationSequence()
			== Reservation.GetActivationSequence()
		&& ActionsMatch(Command.GetAction(), Reservation.GetAction());
}

Fdemo_mapShanmenFormationProductPreparationResult
Fdemo_mapShanmenFormationProductAuthority::PrepareDeployment(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FShanmenFormationDiagramDefinition& Diagram,
	const FVector& Origin,
	const FVector& Forward)
{
	Fdemo_mapShanmenFormationProductPreparationResult Result;
	if (!Diagram.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenFormationProductPreparationStatus::InvalidDiagram;
		Result.Diagnostic =
			TEXT("Formation deployment requires one valid authored diagram.");
		return Result;
	}
	if (!IsFiniteVector(Origin))
	{
		Result.Status =
			Edemo_mapShanmenFormationProductPreparationStatus::InvalidOrigin;
		Result.Diagnostic =
			TEXT("Formation deployment requires one finite sampled origin.");
		return Result;
	}
	if (!IsFiniteVector(Forward) || Forward.IsNearlyZero())
	{
		Result.Status =
			Edemo_mapShanmenFormationProductPreparationStatus::InvalidDirection;
		Result.Diagnostic =
			TEXT("Formation deployment requires one finite non-zero forward direction.");
		return Result;
	}
	if (Authority.GetLifecycleState()
		!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		Result.Status =
			Edemo_mapShanmenFormationProductPreparationStatus::AuthorityNotReady;
		Result.Diagnostic =
			TEXT("Formation deployment requires the ready ShanmenItems authority.");
		return Result;
	}
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Result.Correlation, &Result.Diagnostic))
	{
		Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::
			CorrelationUnavailable;
		return Result;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot)
		|| !Snapshot.Content.IsValid())
	{
		Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::
			SnapshotUnavailable;
		Result.Diagnostic =
			TEXT("Formation deployment could not freeze the current item-authority content stamp.");
		return Result;
	}
	Result.AuthorityRevision = Snapshot.AuthorityRevision;
	if (Authority.GetBoundOwnerId() != Result.Correlation.OwnerId
		|| Snapshot.AuthorityRevision
			< Result.Correlation.LifecycleAuthorityRevision)
	{
		Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::
			AuthorityCorrelationMismatch;
		Result.Diagnostic =
			TEXT("Formation deployment rejected a stale or foreign durable Run correlation.");
		return Result;
	}
	if (!Coordinator.IsReady()
		|| Coordinator.GetRunId() != Result.Correlation.ActiveRunId)
	{
		Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::
			CoordinatorNotReady;
		Result.Diagnostic =
			TEXT("Formation deployment requires the combat coordinator for the exact durable ActiveRun.");
		return Result;
	}
	if (!Coordinator.TryReservePlayerFormationAction(
			Result.Correlation,
			Snapshot.Content,
			Result.Reservation,
			Result.Diagnostic))
	{
		Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::
			ReservationRejected;
		return Result;
	}
	if (!Fdemo_mapShanmenFormationDeploymentCommand::TryCapture(
			Result.Correlation,
			Result.Reservation,
			Diagram,
			Origin,
			Forward,
			Result.Command))
	{
		Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::
			CommandRejected;
		Result.Diagnostic =
			TEXT("Reserved formation identity could not freeze a deployment command; the sequence remains consumed.");
		return Result;
	}

	Result.Status = Edemo_mapShanmenFormationProductPreparationStatus::Ready;
	Result.Diagnostic =
		TEXT("Durable item Run and combat Run prepared one formation deployment command.");
	return Result;
}
