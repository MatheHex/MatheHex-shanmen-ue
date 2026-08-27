#pragma once

#include "CoreMinimal.h"

/** Builds replay-stable GUIDs from explicitly ordered canonical values. */
struct SHANMENCORE_API FShanmenDeterministicId
{
	static FGuid FromCanonicalString(FName Namespace, const FString& Payload);
	static FGuid FromCanonicalParts(FName Namespace, const TArray<FString>& Parts);
};
