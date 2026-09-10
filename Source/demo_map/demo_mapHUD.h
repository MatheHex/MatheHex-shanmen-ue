#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.h"
#include "GameFramework/HUD.h"
#include "demo_mapHUD.generated.h"

struct Fdemo_mapShanmenSwordQiAvailabilityCommandResult;

/** Draws only the current GameState text; it owns no mission state. */
UCLASS()
class Ademo_mapHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

	/** Projects the existing typed Sword Qi command result into one HUD line. */
	static bool TryBuildSwordQiFeedback(
		const Fdemo_mapShanmenSwordQiAvailabilityCommandResult& Result,
		const FString& KeyLabel,
		FString& OutText,
		FLinearColor& OutColor);

	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter&
	GetThrownWeaponArcPreviewRendererAdapter()
	{
		return ThrownWeaponArcPreviewRendererAdapter;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter&
	GetThrownWeaponArcPreviewRendererAdapter() const
	{
		return ThrownWeaponArcPreviewRendererAdapter;
	}

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter
		ThrownWeaponArcPreviewRendererAdapter;
};
