#include "demo_mapGameState.h"
#include "demo_map.h"

void Ademo_mapGameState::InitializeMission(int32 InRequiredTargets)
{
	RequiredTargets = FMath::Max(1, InRequiredTargets);
	DestroyedTargets = 0;
	MissionPhase = Edemo_mapMissionPhase::EliminateTargets;
	UE_LOG(Logdemo_map, Log, TEXT("T5: mission initialized; targets=0/%d."), RequiredTargets);
}

bool Ademo_mapGameState::RegisterTargetDestroyed()
{
	if (MissionPhase != Edemo_mapMissionPhase::EliminateTargets || DestroyedTargets >= RequiredTargets)
	{
		return false;
	}

	++DestroyedTargets;
	UE_LOG(Logdemo_map, Log, TEXT("T5: target destroyed; targets=%d/%d."), DestroyedTargets, RequiredTargets);
	if (DestroyedTargets == RequiredTargets)
	{
		MissionPhase = Edemo_mapMissionPhase::ReachExit;
		return true;
	}

	return false;
}

bool Ademo_mapGameState::CanUseExit() const
{
	return MissionPhase == Edemo_mapMissionPhase::ReachExit;
}

bool Ademo_mapGameState::TryCompleteMission()
{
	if (!CanUseExit())
	{
		return false;
	}

	MissionPhase = Edemo_mapMissionPhase::Complete;
	UE_LOG(Logdemo_map, Log, TEXT("T5: demo complete."));
	return true;
}

FString Ademo_mapGameState::GetMissionText() const
{
	switch (MissionPhase)
	{
	case Edemo_mapMissionPhase::EliminateTargets:
		return FString::Printf(TEXT("Destroy Targets: %d / %d"), DestroyedTargets, RequiredTargets);
	case Edemo_mapMissionPhase::ReachExit:
		return TEXT("Reach the Exit");
	case Edemo_mapMissionPhase::Complete:
		return TEXT("Demo Complete");
	default:
		return TEXT("Mission State Unavailable");
	}
}
