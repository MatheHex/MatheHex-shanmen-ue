#include "demo_mapPlayerCombat.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"

float Fdemo_mapPlayerCombat::CaptureAttackPower(const AActor* SourceActor)
{
	float AttackPower = 1.0f;
	if (SourceActor != nullptr)
	{
		if (const Udemo_mapAttributeComponent* Attributes = SourceActor->FindComponentByClass<Udemo_mapAttributeComponent>())
		{
			Attributes->GetFinalValue(Fdemo_mapAttributeIds::AttackPower, AttackPower);
		}
	}
	return FMath::IsFinite(AttackPower)
		? FMath::Max(0.0f, AttackPower)
		: 0.0f;
}

float Fdemo_mapPlayerCombat::CaptureOutgoingDamage(
	const AActor* SourceActor,
	float AttackCoefficient)
{
	return CaptureAttackPower(SourceActor)
		* FMath::Max(0.0f, AttackCoefficient);
}

float Fdemo_mapPlayerCombat::CaptureCooldownMultiplier(
	const AActor* SourceActor)
{
	float Multiplier = 1.0f;
	if (SourceActor != nullptr)
	{
		if (const Udemo_mapAttributeComponent* Attributes =
			SourceActor->FindComponentByClass<
				Udemo_mapAttributeComponent>())
		{
			Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::CooldownMultiplier,
				Multiplier);
		}
	}
	return FMath::IsFinite(Multiplier) && Multiplier > 0.0f
		? Multiplier
		: 1.0f;
}

float Fdemo_mapPlayerCombat::CaptureEffectiveCooldown(
	const AActor* SourceActor,
	float BaseCooldownSeconds)
{
	return FMath::Max(0.0f, BaseCooldownSeconds)
		* CaptureCooldownMultiplier(SourceActor);
}
