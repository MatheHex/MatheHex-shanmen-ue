#pragma once

#include "CoreMinimal.h"

/** Read-only product phase exposed to flying-sword presentation consumers. */
enum class Edemo_mapShanmenControlledWeaponFlightPhase : uint8
{
	Invalid,
	Orbiting,
	Directed,
	Returning,
	Redeployed
};

/**
 * Immutable observation of one authoritative controlled-weapon activation.
 *
 * The model owns no gameplay transition, time, movement, collision, inventory,
 * or combat authority. Product code captures it after the frame owner has
 * finished its work; Actor and HUD presentation may only consume the result.
 */
class Fdemo_mapShanmenControlledWeaponFlightReadModel
{
public:
	static bool TryCapture(
		const FGuid& RunId,
		const FGuid& ItemInstanceId,
		const FGuid& ActivationId,
		Edemo_mapShanmenControlledWeaponFlightPhase Phase,
		const FVector& WeaponLocation,
		const FVector& ReturnAnchor,
		Fdemo_mapShanmenControlledWeaponFlightReadModel& OutReadModel);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenControlledWeaponFlightReadModel& Other) const;
	bool MatchesProduct(
		const FGuid& ExpectedRunId,
		const FGuid& ExpectedItemInstanceId) const;
	bool IsAtReturnAnchor(float Tolerance = KINDA_SMALL_NUMBER) const;

	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	const FGuid& GetActivationId() const { return ActivationId; }
	Edemo_mapShanmenControlledWeaponFlightPhase GetPhase() const
	{
		return Phase;
	}
	const FVector& GetWeaponLocation() const { return WeaponLocation; }
	const FVector& GetReturnAnchor() const { return ReturnAnchor; }

private:
	FGuid RunId;
	FGuid ItemInstanceId;
	FGuid ActivationId;
	Edemo_mapShanmenControlledWeaponFlightPhase Phase =
		Edemo_mapShanmenControlledWeaponFlightPhase::Invalid;
	FVector WeaponLocation = FVector::ZeroVector;
	FVector ReturnAnchor = FVector::ZeroVector;
};
