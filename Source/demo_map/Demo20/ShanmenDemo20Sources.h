#pragma once
#include "CoreMinimal.h"
#include "ShanmenItemAuthorityService.h"

/** Product-only registry/generator. No inventory or acquired quantity lives here. */
struct FShanmenDemo20SourcePorts
{
	TFunction<FShanmenItemGeneratedSourceReadResult(const FGuid&,const FGuid&,FName)> Read;
	TFunction<FShanmenItemDurableCommandResult(const FShanmenItemGeneratedSourceRequest&)> Accept;
};
struct FShanmenDemo20Sources
{
	static constexpr int32 ChestCount=3;
	static FShanmenContentStamp ContentStamp();
	static FName ChestRole(int32 Index);
	static FName EnemyRole(int32 Index);
	static FString Name(FName Role);
	static bool IsRegistered(FName Role);
	static bool ChestPosition(const FGuid& Run, uint64 RunSeed, int32 Index, FVector& Out);
	static bool Build(const FShanmenItemGeneratedSourceReadResult& Read, uint64 RunSeed,
		FShanmenItemGeneratedSourceRequest& Out, FString& Reason);
	/** Read Accepted first; never regenerate a stored result or claim it as inventory. */
	static bool Resolve(const FGuid& Run, uint64 RunSeed, FName Role, const FShanmenDemo20SourcePorts& Ports,
		FShanmenItemGeneratedSourceReceipt& Out, FString& Reason);
	static FString Preview(const FShanmenItemGeneratedSourceReceipt& Receipt);
};

enum class EShanmenDemo20SearchStep : uint8 { Pending, Complete, Interrupted };
/** Only an unconfirmed UI timer. Admission at completion is the durable searched state. */
struct FShanmenDemo20Search
{
	FName Role=NAME_None;
	FVector Origin=FVector::ZeroVector;
	float Clock=0, Health=0;
	bool Begin(FName InRole, const FVector& Player, float InHealth);
	EShanmenDemo20SearchStep Advance(float Delta, const FVector& Player, float InHealth, bool Available);
	void Cancel() { *this=FShanmenDemo20Search(); }
	bool IsActive() const { return !Role.IsNone(); }
};
