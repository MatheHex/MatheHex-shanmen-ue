#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "demo_mapGameState.generated.h"

UENUM()
enum class Edemo_mapMissionPhase : uint8
{
	EliminateTargets,
	ReachExit,
	Complete
};

/** Owns the minimal runtime mission state for the top-down demo. */
UCLASS()
class Ademo_mapGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void InitializeMission(int32 InRequiredTargets = 3);
	bool RegisterTargetDestroyed();
	bool CanUseExit() const;
	bool TryCompleteMission();
	FString GetMissionText() const;

	int32 GetRequiredTargets() const { return RequiredTargets; }
	int32 GetDestroyedTargets() const { return DestroyedTargets; }
	Edemo_mapMissionPhase GetMissionPhase() const { return MissionPhase; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	int32 RequiredTargets = 3;

	UPROPERTY(VisibleAnywhere, Category = "Mission")
	int32 DestroyedTargets = 0;

	UPROPERTY(VisibleAnywhere, Category = "Mission")
	Edemo_mapMissionPhase MissionPhase = Edemo_mapMissionPhase::EliminateTargets;
};
