#pragma once

#include "CoreMinimal.h"
#include "ShanmenThrownWeaponArcPreview.h"
#include "demo_mapShanmenThrownWeaponArcChoiceProjection.h"
#include "demo_mapShanmenThrownWeaponProductController.h"

/**
 * Immutable geometry-only configuration for one Arc preview composition.
 *
 * Capture projects the existing product-owned Arc envelope down to the values
 * required by planning. It owns no item authority, reservation, input device,
 * World, Actor, renderer, or launch state.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration
{
public:
	static bool TryCapture(
		const FShanmenCombatActionSnapshot& Action,
		const Fdemo_mapShanmenThrownWeaponProductCapture& Product,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& OutConfiguration);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Other) const;
	const FGuid& GetConfigurationId() const { return ConfigurationId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const Fdemo_mapShanmenThrownWeaponArcProductPolicy& GetArcProductPolicy()
		const
	{
		return ArcProductPolicy;
	}
	double GetMaximumLaunchSpeed() const { return MaximumLaunchSpeed; }
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetChoicePolicy() const
	{
		return ChoicePolicy;
	}
	int32 GetSegmentCount() const { return SegmentCount; }

private:
	FGuid ConfigurationId;
	FShanmenCombatActionSnapshot Action;
	Fdemo_mapShanmenThrownWeaponArcProductPolicy ArcProductPolicy;
	double MaximumLaunchSpeed = 0.0;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy ChoicePolicy;
	int32 SegmentCount = 0;
};

enum class Edemo_mapShanmenThrownWeaponArcPreviewCompositionStatus : uint8
{
	Invalid,
	ConfigurationRejected,
	ChoiceUnavailable,
	BasisUnavailable,
	ProjectionRejected,
	PlanRejected,
	PreviewRejected,
	Composed
};

/** Immutable audit evidence for one synchronous Arc preview composition. */
class Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult
{
public:
	bool IsValid() const;
	bool IsComposed() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult& Other)
		const;
	Edemo_mapShanmenThrownWeaponArcPreviewCompositionStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetChoiceStateReadCount() const { return ChoiceStateReadCount; }
	int32 GetBasisSampleCount() const { return BasisSampleCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration&
	GetConfiguration() const
	{
		return Configuration;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& GetBasis() const
	{
		return Basis;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult&
	GetProjection() const
	{
		return Projection;
	}
	const FShanmenThrownWeaponArcPlanResult& GetPlanResult() const
	{
		return PlanResult;
	}
	const FShanmenThrownWeaponArcPreview& GetPreview() const
	{
		return Preview;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewComposition;

	Edemo_mapShanmenThrownWeaponArcPreviewCompositionStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewCompositionStatus::Invalid;
	FString Diagnostic;
	int32 ChoiceStateReadCount = 0;
	int32 BasisSampleCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Configuration;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
	Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Projection;
	FShanmenThrownWeaponArcPlanResult PlanResult;
	FShanmenThrownWeaponArcPreview Preview;
};

/**
 * Pure device/UI-neutral composition seam.
 *
 * A valid configuration reads the current choice at most once, then samples
 * the caller-owned source basis at most once. It delegates geometry to the
 * existing projector, planner, and preview sampler without consuming or
 * launching the represented item.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewComposition
{
public:
	using FReadCurrentChoice = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceState()>;
	using FSampleSourceBasis = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis()>;

	static Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Compose(
		const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Configuration,
		FReadCurrentChoice ReadCurrentChoice,
		FSampleSourceBasis SampleSourceBasis);
};
