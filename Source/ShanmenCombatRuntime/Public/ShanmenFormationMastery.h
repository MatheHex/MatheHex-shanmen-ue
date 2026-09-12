#pragma once

#include "CoreMinimal.h"

#include "ShanmenFormationMastery.generated.h"

/** Operation tiers frozen by the 0.0.10 Formation Mastery direction. */
UENUM(BlueprintType)
enum class EShanmenFormationMasteryTier : uint8
{
	Invalid,
	Beginner,
	Intermediate,
	Master
};

/**
 * Distinct material-delivery operations unlocked by Formation Mastery.
 * Range, timing, item selection and world execution remain external.
 */
UENUM(BlueprintType)
enum class EShanmenFormationMaterialDeliveryMode : uint8
{
	Invalid,
	ProximityFill,
	RemoteThrow,
	ScatterFormation
};

/**
 * Immutable, pure capability policy for one Formation Mastery tier.
 * Higher tiers retain all lower-tier operations; this type owns no tuning.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationMasteryPolicy
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		EShanmenFormationMasteryTier Tier,
		FShanmenFormationMasteryPolicy& OutPolicy);
	static bool IsTierValid(EShanmenFormationMasteryTier Tier);
	static bool IsDeliveryModeValid(
		EShanmenFormationMaterialDeliveryMode Mode);

	bool IsValid() const;
	bool CanUseDeliveryMode(
		EShanmenFormationMaterialDeliveryMode Mode) const;
	EShanmenFormationMaterialDeliveryMode GetHighestUnlockedDeliveryMode() const;
	EShanmenFormationMasteryTier GetTier() const { return Tier; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	EShanmenFormationMasteryTier Tier =
		EShanmenFormationMasteryTier::Invalid;
};
