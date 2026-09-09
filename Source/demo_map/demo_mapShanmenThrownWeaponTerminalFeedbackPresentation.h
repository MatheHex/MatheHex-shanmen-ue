#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponRunHost.h"

enum class Edemo_mapShanmenThrownWeaponTerminalFeedbackKind : uint8
{
	Invalid,
	Impact,
	BlockingMiss,
	RangeExpired,
	FlightTimeExpired,
	Interrupted
};

/**
 * Immutable, renderer-neutral projection of one authoritative thrown-weapon
 * terminal receipt. It never resolves damage, advances gameplay, or owns time.
 */
class Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponTerminalReceipt& Receipt,
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation&
			OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation& Other)
		const;
	Edemo_mapShanmenThrownWeaponTerminalFeedbackKind GetKind() const
	{
		return Kind;
	}
	const FGuid& GetLaunchId() const { return LaunchId; }
	float GetAppliedDamage() const { return AppliedDamage; }
	bool DidDefeatTarget() const { return bDefeatedTarget; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenThrownWeaponTerminalFeedbackKind Kind =
		Edemo_mapShanmenThrownWeaponTerminalFeedbackKind::Invalid;
	FGuid LaunchId;
	float AppliedDamage = 0.0f;
	bool bDefeatedTarget = false;
	FString DisplayText;
};
