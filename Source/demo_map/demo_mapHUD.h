#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "demo_mapHUD.generated.h"

/** Draws only the current GameState text; it owns no mission state. */
UCLASS()
class Ademo_mapHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
