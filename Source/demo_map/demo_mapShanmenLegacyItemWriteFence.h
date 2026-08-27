#pragma once

#include "CoreMinimal.h"

struct Fdemo_mapPersistentProfile;

/** Durable reason that the 0.0.9B item writers are no longer authoritative. */
enum class Edemo_mapShanmenLegacyItemWriteFenceState : uint8
{
	Open,
	RetiredByPrimary,
	RetiredByBackup,
	InvalidIdentity
};

/** Read-only probe of the storage-level 0.0.10 cutover marker. */
struct Fdemo_mapShanmenLegacyItemWriteFenceProbe
{
	Edemo_mapShanmenLegacyItemWriteFenceState State =
		Edemo_mapShanmenLegacyItemWriteFenceState::InvalidIdentity;
	FString PrimaryPath;
	FString BackupPath;
	FString Diagnostic;

	bool IsRetired() const
	{
		return State
			== Edemo_mapShanmenLegacyItemWriteFenceState::RetiredByPrimary
			|| State
				== Edemo_mapShanmenLegacyItemWriteFenceState::RetiredByBackup;
	}
};

/**
 * Shared fail-closed fence for every legacy durable item writer.
 *
 * The verified 0.0.10 authority document is itself the cutover marker.  A
 * primary or backup file retires legacy writes even when its bytes later prove
 * corrupt: old authorities must never overwrite or silently reconstruct a new
 * authority that has entered recovery.
 */
class Fdemo_mapShanmenLegacyItemWriteFence
{
public:
	static Fdemo_mapShanmenLegacyItemWriteFenceProbe Inspect(
		const FString& StorageRoot,
		const FGuid& OwnerId);

	/**
	 * Allows Profile-only currency/progression saves after cutover, while
	 * rejecting any change to the legacy item graph, layouts, shop stock, or
	 * item-bearing Run projection.
	 */
	static bool PreservesRetiredProfileItems(
		const Fdemo_mapPersistentProfile& DurableBefore,
		const Fdemo_mapPersistentProfile& Candidate,
		FString* OutDiagnostic = nullptr);
};
