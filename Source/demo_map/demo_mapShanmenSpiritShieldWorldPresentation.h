#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct Fdemo_mapShanmenCombatRunTimelineSample;
class Fdemo_mapShanmenSpiritShieldProductSession;

/**
 * Stateless adapter from the sole Spirit Shield product Session to cues owned
 * by the currently controlled Pawn. It never caches capacity, time, or combat
 * identity and can therefore be safely re-run every controller tick.
 */
class Fdemo_mapShanmenSpiritShieldWorldPresentation
{
public:
	/** Installs the two collisionless cue components exactly once. */
	static bool EnsureInstalled(
		AActor* Owner,
		UStaticMesh* ShellMesh,
		UMaterialInterface* ShellMaterial);

	/**
	 * Projects current authority and one frozen Run-timeline sample. Null,
	 * mismatched, pre-start, expired, closed, released, or depleted inputs fail
	 * closed; expiry never waits for a later Session mutation.
	 */
	static bool Synchronize(
		AActor* Owner,
		const Fdemo_mapShanmenSpiritShieldProductSession* Session,
		const Fdemo_mapShanmenCombatRunTimelineSample* TimelineSample,
		UStaticMesh* ShellMesh,
		UMaterialInterface* ShellMaterial);

	static bool IsVisible(const AActor* Owner);
	static bool IsGeometryValid(const AActor* Owner);
	static bool HasCanonicalMaterialColor(const AActor* Owner);
	static bool HasStableAppearance(const AActor* Owner);
	static bool HasLowCapacityAppearance(const AActor* Owner);
	static bool HasCapacityScale(
		const AActor* Owner,
		float AvailableCapacity,
		float MaximumCapacity);
	static bool HasLifetimeRadius(
		const AActor* Owner,
		int64 CurrentTick,
		int64 StartTick,
		int64 DeadlineTick);
	static FLinearColor GetCueColor(const AActor* Owner);
	static float GetCueIntensity(const AActor* Owner);
	static float GetCueAttenuationRadius(const AActor* Owner);
	static FVector GetShellScale(const AActor* Owner);

private:
	static UStaticMeshComponent* FindShell(const AActor* Owner);
	static UPointLightComponent* FindCueLight(const AActor* Owner);
	static bool HasAppearance(
		const AActor* Owner,
		const FLinearColor& Color,
		float CueIntensity);
	static void SetAppearance(
		AActor* Owner,
		const FLinearColor& Color,
		float CueIntensity);
	static void SetCueAttenuationRadius(AActor* Owner, float Radius);
	static void SetShellScale(AActor* Owner, const FVector& Scale);
	static void SetActive(AActor* Owner, bool bActive);
};
