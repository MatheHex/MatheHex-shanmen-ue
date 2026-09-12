#include "ShanmenFormationMastery.h"

bool FShanmenFormationMasteryPolicy::TryCreate(
	const EShanmenFormationMasteryTier Tier,
	FShanmenFormationMasteryPolicy& OutPolicy)
{
	OutPolicy = FShanmenFormationMasteryPolicy();
	if (!IsTierValid(Tier))
	{
		return false;
	}

	OutPolicy.Tier = Tier;
	return OutPolicy.IsValid();
}

bool FShanmenFormationMasteryPolicy::IsTierValid(
	const EShanmenFormationMasteryTier Tier)
{
	return Tier == EShanmenFormationMasteryTier::Beginner
		|| Tier == EShanmenFormationMasteryTier::Intermediate
		|| Tier == EShanmenFormationMasteryTier::Master;
}

bool FShanmenFormationMasteryPolicy::IsDeliveryModeValid(
	const EShanmenFormationMaterialDeliveryMode Mode)
{
	return Mode == EShanmenFormationMaterialDeliveryMode::ProximityFill
		|| Mode == EShanmenFormationMaterialDeliveryMode::RemoteThrow
		|| Mode == EShanmenFormationMaterialDeliveryMode::ScatterFormation;
}

bool FShanmenFormationMasteryPolicy::IsValid() const
{
	return IsTierValid(Tier);
}

bool FShanmenFormationMasteryPolicy::CanUseDeliveryMode(
	const EShanmenFormationMaterialDeliveryMode Mode) const
{
	if (!IsValid() || !IsDeliveryModeValid(Mode))
	{
		return false;
	}

	switch (Tier)
	{
	case EShanmenFormationMasteryTier::Beginner:
		return Mode
			== EShanmenFormationMaterialDeliveryMode::ProximityFill;
	case EShanmenFormationMasteryTier::Intermediate:
		return Mode
			== EShanmenFormationMaterialDeliveryMode::ProximityFill
			|| Mode
				== EShanmenFormationMaterialDeliveryMode::RemoteThrow;
	case EShanmenFormationMasteryTier::Master:
		return true;
	default:
		return false;
	}
}

EShanmenFormationMaterialDeliveryMode
FShanmenFormationMasteryPolicy::GetHighestUnlockedDeliveryMode() const
{
	switch (Tier)
	{
	case EShanmenFormationMasteryTier::Beginner:
		return EShanmenFormationMaterialDeliveryMode::ProximityFill;
	case EShanmenFormationMasteryTier::Intermediate:
		return EShanmenFormationMaterialDeliveryMode::RemoteThrow;
	case EShanmenFormationMasteryTier::Master:
		return EShanmenFormationMaterialDeliveryMode::ScatterFormation;
	default:
		return EShanmenFormationMaterialDeliveryMode::Invalid;
	}
}
