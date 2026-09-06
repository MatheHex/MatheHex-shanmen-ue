#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.h"
#include "GameFramework/HUD.h"
#include "demo_mapHUD.generated.h"

/** Draws only the current GameState text; it owns no mission state. */
UCLASS()
class Ademo_mapHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

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
