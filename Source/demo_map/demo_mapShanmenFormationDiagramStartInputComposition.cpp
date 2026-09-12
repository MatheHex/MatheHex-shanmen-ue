#include "demo_mapShanmenFormationDiagramStartInputComposition.h"

namespace
{
	using ECompositionStatus =
		Edemo_mapShanmenFormationDiagramStartInputCompositionStatus;
	using EInputStatus = Edemo_mapShanmenFormationStartInputStatus;

	bool IsInputPreflightStatus(const EInputStatus Status)
	{
		switch (Status)
		{
		case EInputStatus::GameplayBlocked:
		case EInputStatus::LifecycleUnavailable:
		case EInputStatus::RunUnavailable:
		case EInputStatus::OwnerUnavailable:
		case EInputStatus::EventIdentityInvalid:
			return true;
		default:
			return false;
		}
	}

	bool SelectedAccessMatchesInput(
		const Fdemo_mapShanmenFormationDiagramStartInputCompositionResult&
			Result)
	{
		if (!Result.GetAccess().IsSelected()
			|| !Result.GetInput().Sample.IsValid())
		{
			return false;
		}

		const Fdemo_mapShanmenFormationDiagramSelection& AccessSelection =
			Result.GetAccess().Selection.Selection;
		const Fdemo_mapShanmenFormationDiagramSelection& InputSelection =
			Result.GetInput().Sample.GetSelection();
		return Result.GetInput().OwnerId
				== Result.GetAccess().RequestedOwnerId
			&& AccessSelection.GetSelectionId()
				== InputSelection.GetSelectionId()
			&& AccessSelection.GetCatalogId()
				== InputSelection.GetCatalogId()
			&& AccessSelection.GetKnowledgeSnapshotId()
				== InputSelection.GetKnowledgeSnapshotId()
			&& AccessSelection.GetOwnerId() == InputSelection.GetOwnerId()
			&& AccessSelection.GetKnowledgeAuthorityRevision()
				== InputSelection.GetKnowledgeAuthorityRevision()
			&& AccessSelection.GetDiagram().GetDiagramDefinitionId()
				== InputSelection.GetDiagram().GetDiagramDefinitionId()
			&& InputSelection.GetDiagram().GetDiagramDefinitionId()
				== Result.GetAccess().RequestedDiagramDefinitionId;
	}
}

bool Fdemo_mapShanmenFormationDiagramStartInputCompositionResult::IsValid()
	const
{
	if (Status == ECompositionStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| AccessInvocationCount < 0 || AccessInvocationCount > 1
		|| SpatialSampleCaptureCount < 0
		|| SpatialSampleCaptureCount > 1)
	{
		return false;
	}

	switch (Status)
	{
	case ECompositionStatus::InputCompletedBeforeAccess:
		return !bRouteProtocolViolation
			&& AccessInvocationCount == 0
			&& SpatialSampleCaptureCount == 0
			&& !Access.IsValid()
			&& Input.IsValid()
			&& Input.SampleCount == 0
			&& IsInputPreflightStatus(Input.Status);

	case ECompositionStatus::DiagramAccessRejected:
		return !bRouteProtocolViolation
			&& AccessInvocationCount == 1
			&& SpatialSampleCaptureCount == 0
			&& Access.IsValid()
			&& !Access.IsSelected()
			&& Input.IsValid()
			&& Input.Status == EInputStatus::SampleRejected
			&& Input.SampleCount == 1
			&& Input.LifecycleInvocationCount == 0;

	case ECompositionStatus::SpatialSampleRejected:
		return !bRouteProtocolViolation
			&& AccessInvocationCount == 1
			&& SpatialSampleCaptureCount == 1
			&& Access.IsSelected()
			&& Input.IsValid()
			&& Input.Status == EInputStatus::SampleRejected
			&& Input.SampleCount == 1
			&& Input.LifecycleInvocationCount == 0;

	case ECompositionStatus::Delegated:
		return !bRouteProtocolViolation
			&& AccessInvocationCount == 1
			&& SpatialSampleCaptureCount == 1
			&& Input.IsValid()
			&& (Input.Status == EInputStatus::Applied
				|| Input.Status == EInputStatus::LifecycleRejected)
			&& Input.SampleCount == 1
			&& Input.LifecycleInvocationCount == 1
			&& SelectedAccessMatchesInput(*this);

	case ECompositionStatus::RouteProtocolRejected:
		return bRouteProtocolViolation && !Input.IsAccepted();

	case ECompositionStatus::Invalid:
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationDiagramStartInputCompositionResult::IsAccepted()
	const
{
	return IsValid()
		&& Status == ECompositionStatus::Delegated
		&& Input.IsAccepted();
}

Fdemo_mapShanmenFormationDiagramStartInputCompositionResult
Fdemo_mapShanmenFormationDiagramStartInputComposition::Route(
	const bool bGameplayInputAllowed,
	const bool bLifecycleAvailable,
	const FGuid& RunId,
	const FGuid& OwnerId,
	const FGuid& InputEventId,
	const Fdemo_mapShanmenFormationDiagramCatalog& Catalog,
	const FName RequestedDiagramDefinitionId,
	const FVector& Origin,
	const FVector& Forward,
	FReadAuthority ReadAuthority,
	FRouteStart RouteLifecycle)
{
	Fdemo_mapShanmenFormationDiagramStartInputCompositionResult Result;
	auto SampleStart = [&Result, &Catalog, &OwnerId,
		RequestedDiagramDefinitionId, &Origin, &Forward, &ReadAuthority]()
	{
		++Result.AccessInvocationCount;
		if (Result.AccessInvocationCount != 1)
		{
			Result.bRouteProtocolViolation = true;
			return Fdemo_mapShanmenFormationStartInputSample();
		}

		Result.Access =
			Fdemo_mapShanmenFormationDiagramAccessAdapter::Resolve(
				Catalog,
				OwnerId,
				RequestedDiagramDefinitionId,
				ReadAuthority);
		if (!Result.Access.IsSelected())
		{
			return Fdemo_mapShanmenFormationStartInputSample();
		}

		++Result.SpatialSampleCaptureCount;
		Fdemo_mapShanmenFormationStartInputSample Sample;
		Fdemo_mapShanmenFormationStartInputSample::TryCapture(
			Result.Access.Selection.Selection,
			Origin,
			Forward,
			Sample);
		return Sample;
	};

	Result.Input = Fdemo_mapShanmenFormationInputAdapter::RouteStartInput(
		bGameplayInputAllowed,
		bLifecycleAvailable,
		RunId,
		OwnerId,
		InputEventId,
		SampleStart,
		RouteLifecycle);

	if (Result.bRouteProtocolViolation
		|| Result.AccessInvocationCount != Result.Input.SampleCount
		|| Result.SpatialSampleCaptureCount
			!= (Result.Access.IsSelected() ? 1 : 0))
	{
		Result.Status = ECompositionStatus::RouteProtocolRejected;
		Result.bRouteProtocolViolation = true;
		Result.Diagnostic =
			TEXT("Formation diagram start composition rejected a route protocol violation.");
	}
	else if (Result.AccessInvocationCount == 0)
	{
		if (Result.Input.IsValid()
			&& IsInputPreflightStatus(Result.Input.Status))
		{
			Result.Status = ECompositionStatus::InputCompletedBeforeAccess;
			Result.Diagnostic = Result.Input.Diagnostic;
		}
		else
		{
			Result.Status = ECompositionStatus::RouteProtocolRejected;
			Result.bRouteProtocolViolation = true;
			Result.Diagnostic =
				TEXT("Formation input route bypassed diagram access without a valid preflight result.");
		}
	}
	else if (!Result.Access.IsSelected())
	{
		if (Result.Access.IsValid()
			&& Result.Input.IsValid()
			&& Result.Input.Status == EInputStatus::SampleRejected)
		{
			Result.Status = ECompositionStatus::DiagramAccessRejected;
			Result.Diagnostic = Result.Access.Diagnostic;
		}
		else
		{
			Result.Status = ECompositionStatus::RouteProtocolRejected;
			Result.bRouteProtocolViolation = true;
			Result.Diagnostic =
				TEXT("Formation input route misreported rejected diagram access.");
		}
	}
	else if (Result.Input.Status == EInputStatus::SampleRejected)
	{
		Result.Status = ECompositionStatus::SpatialSampleRejected;
		Result.Diagnostic =
			TEXT("Selected formation diagram has an invalid spatial input sample.");
	}
	else if (Result.Input.IsValid()
		&& (Result.Input.Status == EInputStatus::Applied
			|| Result.Input.Status == EInputStatus::LifecycleRejected)
		&& SelectedAccessMatchesInput(Result))
	{
		Result.Status = ECompositionStatus::Delegated;
		Result.Diagnostic = Result.Input.Diagnostic;
	}
	else
	{
		Result.Status = ECompositionStatus::RouteProtocolRejected;
		Result.bRouteProtocolViolation = true;
		Result.Diagnostic =
			TEXT("Formation input route returned evidence inconsistent with selected diagram access.");
	}

	return Result;
}
