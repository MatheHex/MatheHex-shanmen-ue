#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
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
	 * Projects current authority into visibility. Null, invalid, closed,
	 * released, or depleted Sessions fail closed.
	 */
	static bool Synchronize(
		AActor* Owner,
		const Fdemo_mapShanmenSpiritShieldProductSession* Session,
		UStaticMesh* ShellMesh,
		UMaterialInterface* ShellMaterial);

	static bool IsVisible(const AActor* Owner);
	static bool IsGeometryValid(const AActor* Owner);
	static bool HasCanonicalMaterialColor(const AActor* Owner);
	static FLinearColor GetCueColor(const AActor* Owner);

private:
	static UStaticMeshComponent* FindShell(const AActor* Owner);
	static UPointLightComponent* FindCueLight(const AActor* Owner);
	static void SetActive(AActor* Owner, bool bActive);
};
