#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponProductSession.h"

enum class Edemo_mapShanmenThrownWeaponLaunchRejectionKind : uint8
{
	Invalid,
	ReleasePathBlocked
};

/**
 * Immutable, renderer-neutral feedback projected only from an exact
 * pre-publication release-path rejection. Other product failures fail closed
 * so input, item, protocol, or recovery errors cannot be presented as walls.
 */
class Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponSessionResult& Session,
		Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation&
			OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation& Other)
		const;
	Edemo_mapShanmenThrownWeaponLaunchRejectionKind GetKind() const
	{
		return Kind;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSelectionId() const { return SelectionId; }
	bool RequiresCancellationRecovery() const
	{
		return bRequiresCancellationRecovery;
	}
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenThrownWeaponLaunchRejectionKind Kind =
		Edemo_mapShanmenThrownWeaponLaunchRejectionKind::Invalid;
	FGuid RunId;
	FGuid SelectionId;
	bool bRequiresCancellationRecovery = false;
	FString DisplayText;
};
