#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

enum class Edemo_mapInputBindingStatus : uint8
{
	Applied,
	RestoredDefaults,
	NoOp,
	UnknownAction,
	InvalidKey,
	ReservedKey,
	DuplicateKey,
	FileWriteFailed,
	FileVerificationFailed,
	FileReplaceFailed,
	LoadRejected
};

struct Fdemo_mapInputBindingResult
{
	Edemo_mapInputBindingStatus Status = Edemo_mapInputBindingStatus::LoadRejected;
	FString Diagnostic;
	FName ActionId = NAME_None;
	FKey Key;

	bool IsSuccess() const
	{
		return Status == Edemo_mapInputBindingStatus::Applied
			|| Status == Edemo_mapInputBindingStatus::RestoredDefaults
			|| Status == Edemo_mapInputBindingStatus::NoOp;
	}
};

class Fdemo_mapInputBindingSettings
{
public:
	static Fdemo_mapInputBindingSettings& Get();

	Fdemo_mapInputBindingResult Load();
	Fdemo_mapInputBindingResult ApplyOverride(FName ActionId, const FKey& Key);
	/** Applies a binding and deterministically swaps the displaced action to the previous key. */
	Fdemo_mapInputBindingResult ApplyOverrideWithSwap(FName ActionId, const FKey& Key);
	Fdemo_mapInputBindingResult RestoreDefaults();
	FKey GetKey(FName ActionId) const;
	const TMap<FName, FKey>& GetBindings() const { return Bindings; }
	FString GetConfigPath() const;
	bool IsLoaded() const { return bLoaded; }

	static bool ValidateKey(const FKey& Key, FString& OutDiagnostic);
	static bool ValidateBindings(const TMap<FName, FKey>& Candidate, FString& OutDiagnostic);

private:
	Fdemo_mapInputBindingSettings();
	static TMap<FName, FKey> DefaultBindings();
	Fdemo_mapInputBindingResult CommitCandidate(
		const TMap<FName, FKey>& Candidate,
		Edemo_mapInputBindingStatus SuccessStatus,
		FName ActionId,
		const FKey& Key);
	bool Serialize(const TMap<FName, FKey>& Candidate, FString& OutText) const;
	bool Deserialize(const FString& Text, TMap<FName, FKey>& OutCandidate, FString& OutDiagnostic) const;
	static bool MergeMissingDefaults(
		TMap<FName, FKey>& Candidate,
		FString& OutDiagnostic);

	TMap<FName, FKey> Bindings;
	bool bLoaded = false;
};
