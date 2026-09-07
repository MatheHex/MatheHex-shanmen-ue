#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.h"

/**
 * GameMode-owned runtime binding between the active Arc-preview composition
 * owner and the current MainHUD renderer surface.
 *
 * A permanently-live fallback surface keeps the non-owning presentation
 * owner safe across HUD creation/destruction. Every replacement uses the
 * existing ownership ticket and handoff protocol; visible recreation first
 * restores the exact physical cursor and never synthesizes a second logical
 * preview command. No Tick or DrawHUD polling is involved.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding() = default;
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding(
		const Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding&) =
		delete;
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding& operator=(
		const Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding&) =
		delete;

	static constexpr int32 CanonicalSegmentCount = 8;

	bool TryInitialize(
		const FGuid& FallbackSurfaceInstanceId,
		FString& OutDiagnostic);
	bool TryAttachHUD(
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter& Surface,
		FString& OutDiagnostic);
	bool TryDetachHUD(
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter& Surface,
		FString& OutDiagnostic);
	bool TryBeginRun(const FGuid& RunId, FString& OutDiagnostic);
	bool TryUpdate(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult&
			OutResult,
		FString& OutDiagnostic);
	bool TryEndRun(
		const FGuid& ExpectedRunId,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);

	bool IsInitialized() const { return FallbackSurface.IsInitialized(); }
	bool IsValid() const;
	bool IsActive() const { return Owner.IsActive(); }
	bool HasAttachedHUD() const { return AttachedHUDSurface != nullptr; }
	bool IsBoundToHUD() const
	{
		return BoundSurface != nullptr
			&& BoundSurface == AttachedHUDSurface;
	}
	const FGuid& GetRunId() const { return Owner.GetRunId(); }
	FGuid GetBoundSurfaceInstanceId() const
	{
		return BoundSurface != nullptr
			? BoundSurface->GetSurfaceInstanceId()
			: FGuid();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return Owner.GetState();
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
	GetBoundSurfaceCursor() const
	{
		return BoundSurface != nullptr
			? BoundSurface->GetSurfaceCursor()
			: Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState();
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter&
	GetFallbackSurface() const
	{
		return FallbackSurface;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner&
	GetOwner() const
	{
		return Owner;
	}

private:
	using FSurface =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;

	bool TryMoveOwnerTo(FSurface& NewSurface, FString& OutDiagnostic);
	bool TryApplyUpdate(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult&
			OutResult,
		FString& OutDiagnostic);

	FSurface FallbackSurface;
	FSurface* AttachedHUDSurface = nullptr;
	FSurface* BoundSurface = nullptr;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner Owner;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition
		OwnershipTransition;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff
		OwnerHandoff;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery
		OwnerHandoffRecovery;
	bool bOperationInProgress = false;
};
