#pragma once
#include "CoreMinimal.h"

enum class EShanmenDemo20EnemyKind : uint8 { Melee, Ranged, Elite };

/** Derived world content only. Health revisions and item quantities live in their existing authorities. */
struct FShanmenDemo20EnemySpec
{
	EShanmenDemo20EnemyKind Kind = EShanmenDemo20EnemyKind::Melee;
	FVector Spawn;
	float Health = 78.f, Damage = 18.f, Range = 240.f, Speed = 155.f, Windup = .85f, WarningRadius = 240.f;
	FVector Scale = FVector(.8f,.8f,1.3f);
	FLinearColor Color = FLinearColor(.48f,.25f,.14f);
	FString Name() const;
};

struct FShanmenDemo20Encounters
{
	static constexpr int32 Count = 3;
	static constexpr int32 CurrentRevision = 2;
	/** r1 preserves exact legacy slots; r2 draws each ordinary zone independently, with one fixed elite. */
	static bool Build(const FGuid& Run, int32 Revision, FShanmenDemo20EnemySpec (&Out)[Count]);
	static FGuid ContentId(int32 Revision);
	static int32 RevisionForContent(const FGuid& Content);
};
