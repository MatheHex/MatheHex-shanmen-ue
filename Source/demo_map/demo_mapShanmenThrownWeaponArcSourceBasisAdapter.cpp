#include "demo_mapShanmenThrownWeaponArcSourceBasisAdapter.h"

#include "demo_mapShanmenThrownWeaponInputAdapter.h"

#include "GameFramework/Actor.h"

namespace
{
	using ESampleStatus =
		Edemo_mapShanmenThrownWeaponArcSourceBasisStatus;
	using ERouteStatus =
		Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus;
	using ECompositionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

}

bool Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult::IsValid() const
{
	if (Status == ESampleStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| TransformSampleCount < 0 || TransformSampleCount > 1)
	{
		return false;
	}
	if (Status == ESampleStatus::Sampled)
	{
		return TransformSampleCount == 1 && Basis.IsValid();
	}
	if (Basis.IsValid())
	{
		return false;
	}
	return Status == ESampleStatus::SourceUnavailable
		? TransformSampleCount == 0
		: TransformSampleCount == 1;
}

bool Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult::IsSampled() const
{
	return IsValid() && Status == ESampleStatus::Sampled;
}

bool Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult::IsValid() const
{
	if (Status == ERouteStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| RouteInvocationCount != 1
		|| SourceSampleRequestCount < 0
		|| SourceSampleRequestCount > 1
		|| !Composition.IsValid()
		|| Composition.GetBasisSampleCount() != SourceSampleRequestCount)
	{
		return false;
	}

	switch (Status)
	{
	case ERouteStatus::InputCompletedBeforeSourceSample:
		return SourceSampleRequestCount == 0
			&& !SourceSample.IsValid()
			&& Composition.GetStatus()
				== ECompositionStatus::InputCompletedBeforeProjection;

	case ERouteStatus::SourceSampleRejected:
		return SourceSampleRequestCount == 1
			&& SourceSample.IsValid()
			&& !SourceSample.IsSampled()
			&& Composition.GetStatus()
				== ECompositionStatus::ProjectionRejected;

	case ERouteStatus::Composed:
		return SourceSampleRequestCount == 1
			&& SourceSample.IsSampled()
			&& (Composition.GetStatus() == ECompositionStatus::Delegated
				|| Composition.GetStatus()
					== ECompositionStatus::ProjectionRejected);

	case ERouteStatus::CompositionProtocolRejected:
		return (SourceSampleRequestCount == 0
				? !SourceSample.IsValid()
				: SourceSample.IsValid())
			&& Composition.GetStatus()
				== ECompositionStatus::RouteProtocolRejected;

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult::IsAccepted() const
{
	return IsValid()
		&& Status == ERouteStatus::Composed
		&& Composition.IsAccepted();
}

Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult
Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::RejectSample(
	const ESampleStatus Status,
	const TCHAR* Diagnostic,
	const int32 TransformSampleCount)
{
	Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.TransformSampleCount = TransformSampleCount;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult
Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::Sample(
	AActor* SourceActor)
{
	if (!::IsValid(SourceActor) || SourceActor->IsActorBeingDestroyed())
	{
		return RejectSample(
			ESampleStatus::SourceUnavailable,
			TEXT("Arc source basis requires one live caller-supplied Actor."));
	}

	const FTransform Transform = SourceActor->GetActorTransform();
	const FVector Location = Transform.GetLocation();
	const FVector Forward = Transform.GetUnitAxis(EAxis::X);
	const FVector Right = Transform.GetUnitAxis(EAxis::Y);
	if (!IsFiniteVector(Location)
		|| !IsFiniteVector(Forward)
		|| !IsFiniteVector(Right))
	{
		return RejectSample(
			ESampleStatus::TransformInvalid,
			TEXT("Arc source Actor transform must remain finite."),
			1);
	}

	Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
	const FVector Origin = Location + FVector(
		0.0,
		0.0,
		Fdemo_mapShanmenThrownWeaponInputAdapter::GetLaunchOriginHeight());
	if (!Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			Origin, Forward, Right, Basis))
	{
		return RejectSample(
			ESampleStatus::BasisRejected,
			TEXT("Arc source Actor transform did not produce a canonical basis."),
			1);
	}

	Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult Result;
	Result.Status = ESampleStatus::Sampled;
	Result.Diagnostic =
		TEXT("Arc source basis sampled from one Actor transform snapshot.");
	Result.TransformSampleCount = 1;
	Result.Basis = Basis;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult
Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter::Route(
	AActor* SourceActor,
	FRouteWithBasis RouteWithBasis) const
{
	Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult Result;
	auto SampleBasis = [&Result, SourceActor]()
	{
		++Result.SourceSampleRequestCount;
		if (Result.SourceSampleRequestCount != 1)
		{
			return Fdemo_mapShanmenThrownWeaponArcChoiceBasis();
		}
		Result.SourceSample = Sample(SourceActor);
		return Result.SourceSample.IsSampled()
			? Result.SourceSample.GetBasis()
			: Fdemo_mapShanmenThrownWeaponArcChoiceBasis();
	};

	Result.RouteInvocationCount = 1;
	Result.Composition = RouteWithBasis(SampleBasis);
	if (Result.Composition.GetStatus()
		== ECompositionStatus::RouteProtocolRejected
		|| Result.Composition.GetBasisSampleCount()
			!= Result.SourceSampleRequestCount)
	{
		Result.Status = ERouteStatus::CompositionProtocolRejected;
		Result.Diagnostic = Result.Composition.GetDiagnostic().IsEmpty()
			? TEXT("Arc source route rejected a composition protocol violation.")
			: Result.Composition.GetDiagnostic();
	}
	else if (Result.SourceSampleRequestCount == 0)
	{
		Result.Status = ERouteStatus::InputCompletedBeforeSourceSample;
		Result.Diagnostic = Result.Composition.GetDiagnostic();
	}
	else if (!Result.SourceSample.IsSampled())
	{
		Result.Status = ERouteStatus::SourceSampleRejected;
		Result.Diagnostic = Result.SourceSample.GetDiagnostic();
	}
	else
	{
		Result.Status = ERouteStatus::Composed;
		Result.Diagnostic = Result.Composition.GetDiagnostic();
	}
	return Result;
}
