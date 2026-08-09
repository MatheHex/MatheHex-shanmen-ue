#pragma once

#include "CoreMinimal.h"
#include "demo_mapAttributeTypes.h"

struct Fdemo_mapAttributeIds
{
	static const FName Primary01;
	static const FName Primary02;
	static const FName Primary03;
	static const FName MaxHealth;
	static const FName MoveSpeed;
	static const FName AttackPower;
	static const FName DodgeChance;
	static const FName FlatDamageReduction;
	static const FName CooldownMultiplier;
};

/** Central registry and replaceable prototype formula boundary. */
struct Fdemo_mapAttributeDefinitions
{
	static constexpr float V2DefaultMoveSpeed = 600.0f;

	static const TArray<Fdemo_mapAttributeDefinition>& GetAll();
	static const Fdemo_mapAttributeDefinition* Find(FName AttributeId);
	static bool ValidateDefinitions(const TArray<Fdemo_mapAttributeDefinition>& Definitions, FString* OutError = nullptr);
	static float ComputePrototypeDerivedRaw(FName AttributeId, const TMap<FName, float>& PrimaryFinals);
};
