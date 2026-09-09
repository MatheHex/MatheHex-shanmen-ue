#include "demo_mapInputBindingSettings.h"

#include "demo_mapInputActionRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

Fdemo_mapInputBindingSettings& Fdemo_mapInputBindingSettings::Get()
{
	static Fdemo_mapInputBindingSettings Instance;
	return Instance;
}

Fdemo_mapInputBindingSettings::Fdemo_mapInputBindingSettings()
	: Bindings(DefaultBindings())
{
}

TMap<FName, FKey> Fdemo_mapInputBindingSettings::DefaultBindings()
{
	TMap<FName, FKey> Result;
	for (const Fdemo_mapInputActionDefinition& Action : Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		Result.Add(Action.ActionId, Action.DefaultKey);
	return Result;
}

FString Fdemo_mapInputBindingSettings::GetConfigPath() const
{
	return FPaths::Combine(FPaths::GeneratedConfigDir(), TEXT("ShanmenInputBindings.ini"));
}

bool Fdemo_mapInputBindingSettings::ValidateKey(const FKey& Key, FString& OutDiagnostic)
{
	if (!Key.IsValid() || (Key.GetMenuCategory() != EKeys::NAME_KeyboardCategory && !Key.IsMouseButton()) || Key.IsGamepadKey()
		|| Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D())
	{
		OutDiagnostic = TEXT("Only known digital keyboard keys or mouse buttons are accepted.");
		return false;
	}
	if (Key == EKeys::Escape
		|| Key.IsModifierKey()
		|| Key == EKeys::LeftShift || Key == EKeys::RightShift
		|| Key == EKeys::LeftControl || Key == EKeys::RightControl
		|| Key == EKeys::LeftAlt || Key == EKeys::RightAlt
		|| Key == EKeys::LeftCommand || Key == EKeys::RightCommand
		|| Key == EKeys::MouseWheelAxis
		|| Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown)
	{
		OutDiagnostic = TEXT("Escape, modifiers, axes, and mouse-wheel inputs are reserved.");
		return false;
	}
	return true;
}

bool Fdemo_mapInputBindingSettings::ValidateBindings(const TMap<FName, FKey>& Candidate, FString& OutDiagnostic)
{
	const int32 RegistryCount =
		Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num();
	if (Candidate.Num() != RegistryCount)
	{
		OutDiagnostic = FString::Printf(
			TEXT("Input settings must define exactly the %d registry actions."),
			RegistryCount);
		return false;
	}
	TSet<FKey> Used;
	for (const Fdemo_mapInputActionDefinition& Action : Fdemo_mapInputActionRegistry::GetExactDefaultActions())
	{
		const FKey* Key = Candidate.Find(Action.ActionId);
		if (!Key || !ValidateKey(*Key, OutDiagnostic)) return false;
		if (Used.Contains(*Key))
		{
			OutDiagnostic = TEXT("Duplicate input keys are rejected atomically.");
			return false;
		}
		Used.Add(*Key);
	}
	return true;
}

bool Fdemo_mapInputBindingSettings::Serialize(const TMap<FName, FKey>& Candidate, FString& OutText) const
{
	OutText = TEXT("[ShanmenInputBindings]\nVersion=9\n");
	for (const Fdemo_mapInputActionDefinition& Action : Fdemo_mapInputActionRegistry::GetExactDefaultActions())
	{
		const FKey* Key = Candidate.Find(Action.ActionId);
		if (!Key) return false;
		OutText += FString::Printf(TEXT("%s=%s\n"), *Action.ActionId.ToString(), *Key->GetFName().ToString());
	}
	return true;
}

bool Fdemo_mapInputBindingSettings::Deserialize(
	const FString& Text,
	TMap<FName, FKey>& OutCandidate,
	FString& OutDiagnostic) const
{
	OutCandidate.Reset();
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, true);
	bool bSection = false;
	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		if (Line == TEXT("[ShanmenInputBindings]")) { bSection = true; continue; }
		if (!bSection || Line.IsEmpty() || Line.StartsWith(TEXT(";"))
			|| Line.StartsWith(TEXT("Version="))) continue;
		FString Left, Right;
		if (!Line.Split(TEXT("="), &Left, &Right) || !Fdemo_mapInputActionRegistry::Find(FName(*Left)))
		{
			OutDiagnostic = TEXT("Input config contains an unknown or malformed action.");
			return false;
		}
		OutCandidate.Add(FName(*Left), FKey(FName(*Right)));
	}
	if (!MergeMissingDefaults(OutCandidate, OutDiagnostic))
	{
		return false;
	}
	return ValidateBindings(OutCandidate, OutDiagnostic);
}

bool Fdemo_mapInputBindingSettings::MergeMissingDefaults(
	TMap<FName, FKey>& Candidate,
	FString& OutDiagnostic)
{
	const TArray<Fdemo_mapInputActionDefinition>& Actions =
		Fdemo_mapInputActionRegistry::GetExactDefaultActions();
	// Forward-compatible migration preserves every existing user override. A
	// newly introduced default may already be occupied by an older override, so
	// use the first still-free registry default instead of rejecting the whole
	// otherwise-valid file or silently moving the user's existing action.
	for (const Fdemo_mapInputActionDefinition& Action : Actions)
	{
		if (Candidate.Contains(Action.ActionId))
		{
			continue;
		}
		FKey MigratedKey = Action.DefaultKey;
		if (Candidate.FindKey(MigratedKey))
		{
			MigratedKey = FKey();
			for (const Fdemo_mapInputActionDefinition& Fallback : Actions)
			{
				if (!Candidate.FindKey(Fallback.DefaultKey))
				{
					MigratedKey = Fallback.DefaultKey;
					break;
				}
			}
		}
		if (!MigratedKey.IsValid())
		{
			OutDiagnostic = FString::Printf(
				TEXT("No conflict-free registry key remains for missing action %s."),
				*Action.ActionId.ToString());
			return false;
		}
		Candidate.Add(Action.ActionId, MigratedKey);
	}
	return true;
}

Fdemo_mapInputBindingResult Fdemo_mapInputBindingSettings::Load()
{
	Fdemo_mapInputBindingResult Result;
	const FString Path = GetConfigPath();
	if (!IFileManager::Get().FileExists(*Path))
	{
		Bindings = DefaultBindings();
		bLoaded = true;
		Result.Status = Edemo_mapInputBindingStatus::NoOp;
		Result.Diagnostic = TEXT("No override config exists; exact defaults are active.");
		return Result;
	}
	FString Text;
	TMap<FName, FKey> Candidate;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !Deserialize(Text, Candidate, Result.Diagnostic))
	{
		Result.Status = Edemo_mapInputBindingStatus::LoadRejected;
		return Result;
	}
	Bindings = MoveTemp(Candidate);
	bLoaded = true;
	Result.Status = Edemo_mapInputBindingStatus::Applied;
	Result.Diagnostic = TEXT("Input overrides loaded from the isolated user config.");
	return Result;
}

Fdemo_mapInputBindingResult Fdemo_mapInputBindingSettings::CommitCandidate(
	const TMap<FName, FKey>& Candidate,
	Edemo_mapInputBindingStatus SuccessStatus,
	FName ActionId,
	const FKey& Key)
{
	Fdemo_mapInputBindingResult Result;
	Result.ActionId = ActionId;
	Result.Key = Key;
	if (!ValidateBindings(Candidate, Result.Diagnostic))
	{
		Result.Status = Result.Diagnostic.Contains(TEXT("Duplicate"))
			? Edemo_mapInputBindingStatus::DuplicateKey
			: Edemo_mapInputBindingStatus::InvalidKey;
		return Result;
	}
	if (Candidate.OrderIndependentCompareEqual(Bindings))
	{
		Result.Status = Edemo_mapInputBindingStatus::NoOp;
		Result.Diagnostic = TEXT("Input binding already matches; memory and file remain unchanged.");
		return Result;
	}
	FString Text;
	if (!Serialize(Candidate, Text))
	{
		Result.Status = Edemo_mapInputBindingStatus::FileWriteFailed;
		Result.Diagnostic = TEXT("Input config serialization failed.");
		return Result;
	}
	const FString Path = GetConfigPath();
	const FString TempPath = Path + TEXT(".tmp");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	if (!FFileHelper::SaveStringToFile(Text, *TempPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		Result.Status = Edemo_mapInputBindingStatus::FileWriteFailed;
		Result.Diagnostic = TEXT("Input config temporary write failed.");
		return Result;
	}
	FString VerifiedText;
	TMap<FName, FKey> Verified;
	if (!FFileHelper::LoadFileToString(VerifiedText, *TempPath)
		|| !Deserialize(VerifiedText, Verified, Result.Diagnostic)
		|| !Verified.OrderIndependentCompareEqual(Candidate))
	{
		Result.Status = Edemo_mapInputBindingStatus::FileVerificationFailed;
		return Result;
	}
	if (!IFileManager::Get().Move(*Path, *TempPath, true, false, true, true))
	{
		Result.Status = Edemo_mapInputBindingStatus::FileReplaceFailed;
		Result.Diagnostic = TEXT("Input config atomic replace failed.");
		return Result;
	}
	Bindings = Candidate;
	bLoaded = true;
	Result.Status = SuccessStatus;
	Result.Diagnostic = SuccessStatus == Edemo_mapInputBindingStatus::RestoredDefaults
		? TEXT("Exact default input bindings restored and committed.")
		: TEXT("Input override committed and ready for immediate rebuild.");
	return Result;
}

Fdemo_mapInputBindingResult Fdemo_mapInputBindingSettings::ApplyOverride(FName ActionId, const FKey& Key)
{
	Fdemo_mapInputBindingResult Result;
	Result.ActionId = ActionId;
	Result.Key = Key;
	if (!Fdemo_mapInputActionRegistry::Find(ActionId))
	{
		Result.Status = Edemo_mapInputBindingStatus::UnknownAction;
		Result.Diagnostic = TEXT("Input override action is not in the exact registry.");
		return Result;
	}
	FString KeyDiagnostic;
	if (!ValidateKey(Key, KeyDiagnostic))
	{
		Result.Status = Key == EKeys::Escape ? Edemo_mapInputBindingStatus::ReservedKey : Edemo_mapInputBindingStatus::InvalidKey;
		Result.Diagnostic = KeyDiagnostic;
		return Result;
	}
	TMap<FName, FKey> Candidate = Bindings;
	Candidate.Add(ActionId, Key);
	return CommitCandidate(Candidate, Edemo_mapInputBindingStatus::Applied, ActionId, Key);
}

Fdemo_mapInputBindingResult
Fdemo_mapInputBindingSettings::ApplyOverrideWithSwap(
	FName ActionId,
	const FKey& Key)
{
	Fdemo_mapInputBindingResult Result;
	Result.ActionId = ActionId;
	Result.Key = Key;
	if (!Fdemo_mapInputActionRegistry::Find(ActionId))
	{
		Result.Status = Edemo_mapInputBindingStatus::UnknownAction;
		Result.Diagnostic =
			TEXT("Input override action is not in the registry.");
		return Result;
	}
	FString KeyDiagnostic;
	if (!ValidateKey(Key, KeyDiagnostic))
	{
		Result.Status = Key == EKeys::Escape
			? Edemo_mapInputBindingStatus::ReservedKey
			: Edemo_mapInputBindingStatus::InvalidKey;
		Result.Diagnostic = KeyDiagnostic;
		return Result;
	}

	const FKey PreviousKey = GetKey(ActionId);
	if (PreviousKey == Key)
	{
		Result.Status = Edemo_mapInputBindingStatus::NoOp;
		Result.Diagnostic =
			TEXT("Input binding already matches; no file change was required.");
		return Result;
	}

	FName DisplacedAction = NAME_None;
	for (const TPair<FName, FKey>& Pair : Bindings)
	{
		if (Pair.Value == Key && Pair.Key != ActionId)
		{
			DisplacedAction = Pair.Key;
			break;
		}
	}

	TMap<FName, FKey> Candidate = Bindings;
	Candidate.Add(ActionId, Key);
	if (!DisplacedAction.IsNone())
	{
		Candidate.Add(DisplacedAction, PreviousKey);
	}
	Result = CommitCandidate(
		Candidate,
		Edemo_mapInputBindingStatus::Applied,
		ActionId,
		Key);
	if (Result.IsSuccess() && !DisplacedAction.IsNone())
	{
		Result.Diagnostic = FString::Printf(
			TEXT("Conflict resolved: %s now uses %s; %s moved to %s."),
			*ActionId.ToString(),
			*Key.GetDisplayName().ToString(),
			*DisplacedAction.ToString(),
			*PreviousKey.GetDisplayName().ToString());
	}
	return Result;
}

Fdemo_mapInputBindingResult Fdemo_mapInputBindingSettings::RestoreDefaults()
{
	return CommitCandidate(DefaultBindings(), Edemo_mapInputBindingStatus::RestoredDefaults, NAME_None, FKey());
}

FKey Fdemo_mapInputBindingSettings::GetKey(FName ActionId) const
{
	if (const FKey* Key = Bindings.Find(ActionId)) return *Key;
	if (const Fdemo_mapInputActionDefinition* Action = Fdemo_mapInputActionRegistry::Find(ActionId)) return Action->DefaultKey;
	return FKey();
}
