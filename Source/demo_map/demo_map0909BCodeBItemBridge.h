#pragma once

#include "CoreMinimal.h"
#include "CodeB/demo_mapCodeBLoadoutSelection.h"

class Ademo_mapGameMode;

/**
 * Formal one-way bridge boundary. P6 observes only a coordinator-confirmed
 * M01 activation; it never decides Code A deployment success or failure.
 */
class Fdemo_map0909BCodeBItemBridge
{
public:
	static FString ObserveConfirmedActivation(
		Ademo_mapGameMode& GameMode,
		const FGuid& StartAttemptId,
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FCodeBLoadoutSelection& LoadoutSelection);
	static bool VerifyNoActiveRunSession(
		const FString& StorageRoot,
		const FGuid& OwnerId,
		FString& OutDiagnostic);
};
