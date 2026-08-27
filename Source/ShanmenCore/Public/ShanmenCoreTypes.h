#pragma once

#include "CoreMinimal.h"

#include "ShanmenCoreTypes.generated.h"

/** Immutable content identity captured when an operation starts. */
USTRUCT(BlueprintType)
struct SHANMENCORE_API FShanmenContentStamp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Content")
	FName Version = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Content")
	FString Digest;

	bool IsValid() const
	{
		return !Version.IsNone() && !Digest.IsEmpty();
	}
};

/** Stable identity envelope for commands crossing 0.0.10 module boundaries. */
USTRUCT(BlueprintType)
struct SHANMENCORE_API FShanmenOperationContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Identity")
	FGuid RunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Identity")
	FGuid OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Identity")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Identity")
	FShanmenContentStamp Content;

	bool IsValid() const
	{
		return RunId.IsValid() && OwnerId.IsValid() && RequestId.IsValid() && Content.IsValid();
	}
};
