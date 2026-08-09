#pragma once

#include "CoreMinimal.h"

/** Unified player outgoing damage snapshot boundary. */
struct Fdemo_mapPlayerCombat
{
	static float CaptureOutgoingDamage(const AActor* SourceActor, float AttackCoefficient = 1.0f);
	static float CaptureCooldownMultiplier(const AActor* SourceActor);
	static float CaptureEffectiveCooldown(
		const AActor* SourceActor,
		float BaseCooldownSeconds);
};
