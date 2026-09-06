#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.h"

/**
 * Immutable proof that one canonical recovery lineage completed two adjacent
 * pending-generation rotations through the existing completion and adoption
 * authorities.
 *
 * This object owns no authority, storage, surface or retry policy. It only
 * binds already-valid G -> G+1 and G+1 -> G+2 evidence and proves that the
 * intermediate completion/adoption is the exact bridge between them.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation&
			FirstRotation,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
			IntermediateCompletion,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
			IntermediateAdoption,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation&
			SecondRotation,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof&
			OutProof,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof&
			Other) const;

	int32 GetInitialGeneration() const
	{
		return FirstRotation.GetSourceGeneration();
	}
	int32 GetIntermediateGeneration() const
	{
		return IntermediateCompletion.GetGeneration();
	}
	int32 GetFinalGeneration() const
	{
		return SecondRotation.GetTargetGeneration();
	}
	const FGuid& GetProofId() const { return ProofId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation&
	GetFirstRotation() const
	{
		return FirstRotation;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion&
	GetIntermediateCompletion() const
	{
		return IntermediateCompletion;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption&
	GetIntermediateAdoption() const
	{
		return IntermediateAdoption;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation&
	GetSecondRotation() const
	{
		return SecondRotation;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation
		FirstRotation;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion
		IntermediateCompletion;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption
		IntermediateAdoption;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation
		SecondRotation;
	FGuid ProofId;
};
