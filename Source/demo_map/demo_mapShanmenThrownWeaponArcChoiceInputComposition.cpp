#include "demo_mapShanmenThrownWeaponArcChoiceInputComposition.h"

#include <limits>

namespace
{
	FVector RejectedTargetSentinel()
	{
		return FVector(std::numeric_limits<double>::quiet_NaN());
	}

	double RejectedApexSentinel()
	{
		return std::numeric_limits<double>::quiet_NaN();
	}

	bool InputSamplingMatchesCallbacks(
		const Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult&
			Result)
	{
		return Result.GetInput().bTargetSampled
			== (Result.GetTargetRequestCount() > 0)
			&& Result.GetInput().bApexClearanceSampled
				== (Result.GetApexRequestCount() > 0);
	}
}

bool Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult::IsValid() const
{
	if (Status
			== Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| RouteInvocationCount != 1
		|| BasisSampleCount < 0 || BasisSampleCount > 1
		|| TargetRequestCount < 0 || ApexRequestCount < 0)
	{
		return false;
	}

	using EStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;
	switch (Status)
	{
	case EStatus::InputCompletedBeforeProjection:
		return !bRouteProtocolViolation
			&& BasisSampleCount == 0
			&& TargetRequestCount == 0
			&& ApexRequestCount == 0
			&& !Projection.IsValid()
			&& Projection.GetStatus()
				== Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Invalid
			&& !Input.bTargetSampled
			&& !Input.bApexClearanceSampled;

	case EStatus::ProjectionRejected:
		return !bRouteProtocolViolation
			&& BasisSampleCount == 1
			&& TargetRequestCount == 1
			&& ApexRequestCount == 0
			&& !Projection.IsValid()
			&& Projection.GetStatus()
				!= Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Invalid
			&& Input.Status
				== Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable
			&& Input.bTargetSampled
			&& !Input.bApexClearanceSampled;

	case EStatus::Delegated:
		return !bRouteProtocolViolation
			&& BasisSampleCount == 1
			&& TargetRequestCount == 1
			&& (ApexRequestCount == 0 || ApexRequestCount == 1)
			&& Projection.IsValid()
			&& Input.bTargetSampled
			&& InputSamplingMatchesCallbacks(*this)
			&& (ApexRequestCount == 1
				|| Input.Status
					== Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable);

	case EStatus::RouteProtocolRejected:
		return bRouteProtocolViolation && !Input.IsAccepted();

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult::IsAccepted()
	const
{
	return IsValid()
		&& Status
			== Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus::Delegated
		&& Input.IsAccepted();
}

Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult
Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition::Route(
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy,
	TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()> SampleBasis,
	FRouteProjectedArc RouteProjectedArc) const
{
	Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult Result;
	auto SampleTarget = [&Result, &ChoiceState, &Policy, &SampleBasis]()
	{
		++Result.TargetRequestCount;
		if (Result.TargetRequestCount != 1)
		{
			Result.bRouteProtocolViolation = true;
			return RejectedTargetSentinel();
		}

		++Result.BasisSampleCount;
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis = SampleBasis();
		Result.Projection =
			Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
				ChoiceState, Basis, Policy);
		return Result.Projection.IsValid()
			? Result.Projection.GetTarget()
			: RejectedTargetSentinel();
	};
	auto SampleApex = [&Result]()
	{
		++Result.ApexRequestCount;
		if (Result.ApexRequestCount != 1
			|| Result.TargetRequestCount != 1
			|| !Result.Projection.IsValid())
		{
			Result.bRouteProtocolViolation = true;
			return RejectedApexSentinel();
		}
		return Result.Projection.GetApexClearance();
	};

	Result.RouteInvocationCount = 1;
	Result.Input = RouteProjectedArc(SampleTarget, SampleApex);
	if (!InputSamplingMatchesCallbacks(Result)
		|| Result.TargetRequestCount > 1
		|| Result.ApexRequestCount > 1
		|| (Result.ApexRequestCount > 0 && Result.TargetRequestCount != 1))
	{
		Result.bRouteProtocolViolation = true;
	}

	using EStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;
	if (Result.bRouteProtocolViolation)
	{
		Result.Status = EStatus::RouteProtocolRejected;
		Result.Diagnostic =
			TEXT("Arc choice composition rejected a route sampling protocol violation.");
	}
	else if (Result.TargetRequestCount == 0)
	{
		Result.Status = EStatus::InputCompletedBeforeProjection;
		Result.Diagnostic = Result.Input.Diagnostic.IsEmpty()
			? TEXT("Arc input completed before choice projection was required.")
			: Result.Input.Diagnostic;
	}
	else if (!Result.Projection.IsValid())
	{
		if (Result.Input.Status
				== Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable
			&& Result.Input.bTargetSampled
			&& !Result.Input.bApexClearanceSampled)
		{
			Result.Status = EStatus::ProjectionRejected;
			Result.Diagnostic = Result.Projection.GetDiagnostic();
		}
		else
		{
			Result.Status = EStatus::RouteProtocolRejected;
			Result.bRouteProtocolViolation = true;
			Result.Diagnostic =
				TEXT("Arc input route accepted or misreported a rejected projection.");
		}
	}
	else
	{
		Result.Status = EStatus::Delegated;
		Result.Diagnostic = Result.Input.Diagnostic.IsEmpty()
			? TEXT("Projected Arc choice delegated to the existing input route.")
			: Result.Input.Diagnostic;
	}
	return Result;
}
