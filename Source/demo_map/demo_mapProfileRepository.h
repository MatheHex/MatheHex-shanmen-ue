#pragma once

#include "CoreMinimal.h"
#include "demo_mapPersistentProfileTypes.h"

class FJsonObject;

/** Pure-data persistence boundary. It is intentionally not connected to startup or runtime item ownership. */
class Fdemo_mapProfileRepository
{
public:
	Fdemo_mapPersistentProfile CreateFreshProfile() const;
	bool ValidateProfile(const Fdemo_mapPersistentProfile& Profile, FString* OutError = nullptr) const;
	Fdemo_mapProfileSaveResult SaveProfile(Fdemo_mapPersistentProfile& Profile, const Fdemo_mapProfileStorageContext& Storage) const;
	Fdemo_mapProfileLoadResult LoadExistingProfile(const Fdemo_mapProfileStorageContext& Storage) const;
	Fdemo_mapProfileLoadResult LoadOrCreateDefaultProfile(const Fdemo_mapProfileStorageContext& Storage) const;

private:
	enum class EReadKind : uint8 { Valid, LegacySchema, Missing, ReadFailure, ParseFailure, FutureSchema, InvalidData };
	struct FReadResult
	{
		EReadKind Kind = EReadKind::ReadFailure;
		FString Diagnostic;
		TArray<uint8> Bytes;
		Fdemo_mapPersistentProfile Profile;
	};

	bool SerializeProfile(const Fdemo_mapPersistentProfile& Profile, TArray<uint8>& OutBytes, FString& OutError) const;
	FReadResult ReadProfile(const FString& Path) const;
	FReadResult DeserializeProfile(const TArray<uint8>& Bytes) const;
	bool ProfilesMatchForCommit(const Fdemo_mapPersistentProfile& Expected, const Fdemo_mapPersistentProfile& Actual) const;
	Fdemo_mapProfileLoadResult RecoverFromBackup(const Fdemo_mapProfileStorageContext& Storage, const FReadResult& Backup, Edemo_mapProfileLoadStatus SuccessStatus, const FString& QuarantinedPath) const;
	FString PreserveCorruptPrimary(const Fdemo_mapProfileStorageContext& Storage, const TArray<uint8>& Bytes, FString& OutError) const;
	bool ShouldFail(const Fdemo_mapProfileStorageContext& Storage, Edemo_mapProfileFailureStage Stage) const;
};
