#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "demo_mapItemTypes.h"
#include "demo_mapInteractable.generated.h"

/** The established V3 interaction distance, centralized without changing behavior. */
struct Fdemo_mapWorldInteractionRules
{
	static constexpr float InteractionRangeUU = 350.0f;
};

UINTERFACE(MinimalAPI)
class Udemo_mapInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Read-only focus surface. Implementations may only request authoritative transactions. */
class DEMO_MAP_API Idemo_mapInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(const APlayerController* Controller) const = 0;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const = 0;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) = 0;
	virtual FVector GetInteractionLocation() const = 0;
	virtual int32 GetInteractionPriority() const { return 0; }
	virtual void FocusChanged(bool bFocused) {}
};
