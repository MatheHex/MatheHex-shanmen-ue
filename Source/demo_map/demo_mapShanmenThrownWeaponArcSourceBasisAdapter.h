#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcChoiceInputComposition.h"

class AActor;

enum class Edemo_mapShanmenThrownWeaponArcSourceBasisStatus : uint8
{
	Invalid,
	Sampled,
	SourceUnavailable,
	TransformInvalid,
	BasisRejected
};

/** Immutable evidence from one canonical source Actor transform snapshot. */
class Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult
{
public:
	bool IsValid() const;
	bool IsSampled() const;
	Edemo_mapShanmenThrownWeaponArcSourceBasisStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetTransformSampleCount() const { return TransformSampleCount; }
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& GetBasis() const
	{
		return Basis;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter;

	Edemo_mapShanmenThrownWeaponArcSourceBasisStatus Status =
		Edemo_mapShanmenThrownWeaponArcSourceBasisStatus::Invalid;
	FString Diagnostic;
	int32 TransformSampleCount = 0;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
};

enum class Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus : uint8
{
	Invalid,
	InputCompletedBeforeSourceSample,
	SourceSampleRejected,
	Composed,
	CompositionProtocolRejected
};

/** Audit result for one lazy source-basis-to-choice composition route. */
class Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetRouteInvocationCount() const { return RouteInvocationCount; }
	int32 GetSourceSampleRequestCount() const
	{
		return SourceSampleRequestCount;
	}
	const Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult&
	GetSourceSample() const
	{
		return SourceSample;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult&
	GetComposition() const
	{
		return Composition;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter;

	Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus Status =
		Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus::Invalid;
	FString Diagnostic;
	int32 RouteInvocationCount = 0;
	int32 SourceSampleRequestCount = 0;
	Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult SourceSample;
	Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult Composition;
};

/**
 * Stateless lazy adapter from one caller-supplied source Actor to P20.13.
 *
 * The downstream InputAdapter remains responsible for proving that the Actor
 * is the canonical player source in the active World and Run. This adapter is
 * invoked only after that route requests geometry, snapshots the Actor
 * transform once, and retains no UObject pointer or world authority.
 */
class Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter
{
public:
	using FRouteWithBasis = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponArcChoiceInputCompositionResult(
			TFunctionRef<Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>)>;

	static Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult Sample(
		AActor* SourceActor);

	Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult Route(
		AActor* SourceActor,
		FRouteWithBasis RouteWithBasis) const;

private:
	static Fdemo_mapShanmenThrownWeaponArcSourceBasisSampleResult RejectSample(
		Edemo_mapShanmenThrownWeaponArcSourceBasisStatus Status,
		const TCHAR* Diagnostic,
		int32 TransformSampleCount = 0);
};
