#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator.h"

/**
 * Stateless device/UI-neutral composition for the three visible Arc edits.
 *
 * Each entry consumes one existing P20.20 read result and delegates directly
 * to the matching P20.21 stale-safe request capture. It adds no identity,
 * authoritative state, revision, route, retry, or physical input ownership.
 */
struct Fdemo_mapShanmenThrownWeaponArcEditingInteractionComposition
{
	static bool TryComposeTargetRequest(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
		const FVector2D& RawTargetIntent,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);
	static bool TryComposeApexAdjustmentRequest(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
		double RawNormalizedDelta,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);
	static bool TryComposeTargetClearRequest(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);
};
