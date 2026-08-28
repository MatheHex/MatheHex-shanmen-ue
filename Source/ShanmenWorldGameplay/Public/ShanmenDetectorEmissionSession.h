#pragma once

#include "CoreMinimal.h"
#include "ShanmenWorldHitAdapter.h"

/**
 * Authoritative ordinal and duplicate gate for one Action + Detector pair.
 *
 * One Begin/End emission represents one scheduled sweep, overlap sample, or
 * projectile contact event. Every target in that emission receives the same
 * ordinal; TargetEntityId keeps ImpactIds unique and target callback order is
 * therefore irrelevant. Repeated contacts for one target are accepted once.
 */
class SHANMENWORLDGAMEPLAY_API FShanmenDetectorEmissionSession
{
public:
	static bool TryStart(
		const FShanmenCombatActionSnapshot& Action,
		FName DetectorId,
		EShanmenHitDetectorKind DetectorKind,
		FShanmenDetectorEmissionSession& OutSession);

	bool IsValid() const;
	bool TryBeginEmission(FShanmenWorldHitContext& OutContext);
	bool TryAcceptCandidate(const FShanmenHitCandidate& Candidate);
	bool TryEndEmission();
	void Reset();

	bool IsEmissionActive() const { return bEmissionActive; }
	int64 GetNextEmissionOrdinal() const { return NextEmissionOrdinal; }
	int32 NumAcceptedTargets() const { return AcceptedTargetIds.Num(); }

private:
	FShanmenCombatActionSnapshot Action;
	FName DetectorId = NAME_None;
	EShanmenHitDetectorKind DetectorKind = EShanmenHitDetectorKind::Shape;
	int64 NextEmissionOrdinal = 0;
	bool bEmissionActive = false;
	TSet<FGuid> AcceptedTargetIds;
};
