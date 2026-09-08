#include "demo_mapShanmenControlledWeaponFlightReadModel.h"

namespace
{
	using EFlightPhase = Edemo_mapShanmenControlledWeaponFlightPhase;
	using FFlightReadModel =
		Fdemo_mapShanmenControlledWeaponFlightReadModel;

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool IsVisiblePhase(const EFlightPhase Phase)
	{
		return Phase == EFlightPhase::Orbiting
			|| Phase == EFlightPhase::Directed
			|| Phase == EFlightPhase::Returning
			|| Phase == EFlightPhase::Redeployed;
	}
}

bool FFlightReadModel::TryCapture(
	const FGuid& RequestedRunId,
	const FGuid& RequestedItemInstanceId,
	const FGuid& RequestedActivationId,
	const EFlightPhase RequestedPhase,
	const FVector& RequestedWeaponLocation,
	const FVector& RequestedReturnAnchor,
	FFlightReadModel& OutReadModel)
{
	OutReadModel = FFlightReadModel();
	FFlightReadModel Candidate;
	Candidate.RunId = RequestedRunId;
	Candidate.ItemInstanceId = RequestedItemInstanceId;
	Candidate.ActivationId = RequestedActivationId;
	Candidate.Phase = RequestedPhase;
	Candidate.WeaponLocation = RequestedWeaponLocation;
	Candidate.ReturnAnchor = RequestedReturnAnchor;
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutReadModel = Candidate;
	return true;
}

bool FFlightReadModel::IsValid() const
{
	return RunId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ActivationId.IsValid()
		&& IsVisiblePhase(Phase)
		&& IsFiniteVector(WeaponLocation)
		&& IsFiniteVector(ReturnAnchor)
		&& (Phase != EFlightPhase::Redeployed || IsAtReturnAnchor());
}

bool FFlightReadModel::Matches(const FFlightReadModel& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& RunId == Other.RunId
		&& ItemInstanceId == Other.ItemInstanceId
		&& ActivationId == Other.ActivationId
		&& Phase == Other.Phase
		&& WeaponLocation == Other.WeaponLocation
		&& ReturnAnchor == Other.ReturnAnchor;
}

bool FFlightReadModel::MatchesProduct(
	const FGuid& ExpectedRunId,
	const FGuid& ExpectedItemInstanceId) const
{
	return IsValid()
		&& RunId == ExpectedRunId
		&& ItemInstanceId == ExpectedItemInstanceId;
}

bool FFlightReadModel::IsAtReturnAnchor(const float Tolerance) const
{
	return FMath::IsFinite(Tolerance)
		&& Tolerance >= 0.0f
		&& IsFiniteVector(WeaponLocation)
		&& IsFiniteVector(ReturnAnchor)
		&& WeaponLocation.Equals(ReturnAnchor, Tolerance);
}
