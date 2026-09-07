#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.h"

/**
 * Concrete renderer surface owned by Ademo_mapHUD.
 *
 * The adapter stores only the visible renderer cursor. Delivery ordering,
 * replay, Run ownership and recovery remain in the existing presentation
 * owner/consumer stack. A caller supplies the physical surface identity so a
 * recreated HUD can never silently reuse an older instance identity.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter final
	: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter() = default;
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter(
		const Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter&) =
		delete;
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter& operator=(
		const Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter&) =
		delete;

	static FName StableConsumerDefinitionId();

	bool TryInitialize(
		const FGuid& SurfaceInstanceId,
		FString& OutDiagnostic);
	bool IsInitialized() const;
	bool IsValid() const;
	/**
	 * Restores one exact authoritative visible cursor onto a newly-created
	 * physical HUD surface before the existing ownership protocol adopts it.
	 * This is a renderer reconstruction operation, not a logical presentation
	 * command, and therefore emits no command or delivery receipt.
	 */
	bool TryRehydrateVisibleForHandoff(
		const FGuid& ExpectedRunId,
		FName ExpectedConsumerDefinitionId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			AuthoritativeVisibleState,
		FString& OutDiagnostic);
	/** Rolls back only the exact unadopted cursor restored by the method above. */
	bool TryDiscardRehydratedVisibleForHandoff(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ExpectedVisibleState,
		FString& OutDiagnostic);

	virtual FGuid GetSurfaceInstanceId() const override
	{
		return SurfaceInstanceId;
	}
	virtual FName GetConsumerDefinitionId() const override
	{
		return ConsumerDefinitionId;
	}
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
	GetSurfaceCursor() const override
	{
		return SurfaceCursor;
	}
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	Show(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		override;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	Replace(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		override;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	Hide(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		override;
	virtual
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse
	ClearToEmpty(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit) override;
	virtual
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse
	RetireForHandoff(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket) override;

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	ApplyMutation(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind
			ExpectedKind);

	FGuid SurfaceInstanceId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};
