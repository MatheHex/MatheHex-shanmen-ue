#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapShanmenSpiritShieldHUDTone : uint8
{
	Invalid,
	Stable,
	Low,
	Depleted
};

/**
 * Pure MainHUD projection of the authoritative short Spirit Shield session.
 *
 * The caller supplies one frozen read of the product capacity and the shared
 * Combat Run timeline. This value owns no shield state, clock, World, Actor or
 * timer; it only validates and formats what the existing authorities expose.
 */
class Fdemo_mapShanmenSpiritShieldHUDPresentation
{
public:
	static bool TryProject(
		bool bSessionActive,
		float AvailableCapacity,
		float MaximumCapacity,
		int64 CurrentTick,
		int64 DeadlineTick,
		int64 TicksPerSecond,
		const FString& ActivationKeyLabel,
		Fdemo_mapShanmenSpiritShieldHUDPresentation& OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSpiritShieldHUDPresentation& Other) const;
	Edemo_mapShanmenSpiritShieldHUDTone GetTone() const { return Tone; }
	float GetAvailableCapacity() const { return AvailableCapacity; }
	float GetMaximumCapacity() const { return MaximumCapacity; }
	int64 GetRemainingTicks() const { return RemainingTicks; }
	double GetRemainingSeconds() const { return RemainingSeconds; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenSpiritShieldHUDTone Tone =
		Edemo_mapShanmenSpiritShieldHUDTone::Invalid;
	float AvailableCapacity = -1.0f;
	float MaximumCapacity = -1.0f;
	int64 RemainingTicks = INDEX_NONE;
	double RemainingSeconds = -1.0;
	FString DisplayText;
};
