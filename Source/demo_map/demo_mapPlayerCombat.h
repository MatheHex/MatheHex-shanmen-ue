#pragma once

#include "CoreMinimal.h"

/** Unified player outgoing damage snapshot boundary. */
struct Fdemo_mapPlayerCombat
{
	/** Captures the authoritative derived attribute before an action starts. */
	static float CaptureAttackPower(const AActor* SourceActor);
	static float CaptureOutgoingDamage(const AActor* SourceActor, float AttackCoefficient = 1.0f);
	static float CaptureCooldownMultiplier(const AActor* SourceActor);
	static float CaptureEffectiveCooldown(
		const AActor* SourceActor,
		float BaseCooldownSeconds);
};
