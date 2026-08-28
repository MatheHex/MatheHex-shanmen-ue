#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"

#include "ShanmenWorldHitAdapter.generated.h"

class AActor;
class UPrimitiveComponent;
struct FHitResult;
struct FOverlapResult;

/** The UE contact source that supplied geometric evidence. */
UENUM(BlueprintType)
enum class EShanmenWorldContactSource : uint8
{
	Sweep,
	Overlap,
	Projectile
};

/**
 * Frozen bridge context for one detector emission.
 *
 * HitOrdinal is assigned by the authoritative detector runtime. The adapter
 * deliberately does not infer it from callback or array order.
 */
USTRUCT(BlueprintType)
struct SHANMENWORLDGAMEPLAY_API FShanmenWorldHitContext
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		FName DetectorId,
		EShanmenHitDetectorKind DetectorKind,
		int32 HitOrdinal,
		FShanmenWorldHitContext& OutContext);

	bool IsValid() const;
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	FName GetDetectorId() const { return DetectorId; }
	EShanmenHitDetectorKind GetDetectorKind() const { return DetectorKind; }
	int32 GetHitOrdinal() const { return HitOrdinal; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|WorldGameplay", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|WorldGameplay", meta = (AllowPrivateAccess = "true"))
	FName DetectorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|WorldGameplay", meta = (AllowPrivateAccess = "true"))
	EShanmenHitDetectorKind DetectorKind = EShanmenHitDetectorKind::Shape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|WorldGameplay", meta = (AllowPrivateAccess = "true"))
	int32 HitOrdinal = INDEX_NONE;
};

/**
 * Injected authority for translating transient UE object references to stable
 * combat entity identity. The adapter never hashes pointers, object names, or
 * callback order.
 */
class SHANMENWORLDGAMEPLAY_API IShanmenWorldEntityResolver
{
public:
	virtual ~IShanmenWorldEntityResolver() = default;

	virtual bool TryResolveEntityId(
		const FGuid& ExpectedRunId,
		EShanmenWorldContactSource ContactSource,
		const AActor* Actor,
		const UPrimitiveComponent* Component,
		int32 BodyIndex,
		FGuid& OutEntityId) const = 0;
};

/** Converts UE world-query evidence into the engine-independent CombatCore candidate contract. */
struct SHANMENWORLDGAMEPLAY_API FShanmenWorldHitAdapter
{
	static bool TryFromSweep(
		const FShanmenWorldHitContext& Context,
		const FHitResult& Hit,
		const IShanmenWorldEntityResolver& EntityResolver,
		FShanmenHitCandidate& OutCandidate);

	static bool TryFromOverlap(
		const FShanmenWorldHitContext& Context,
		const FOverlapResult& Overlap,
		const FVector& ContactLocation,
		const FVector& ContactNormal,
		const IShanmenWorldEntityResolver& EntityResolver,
		FShanmenHitCandidate& OutCandidate);

	static bool TryFromProjectile(
		const FShanmenWorldHitContext& Context,
		const FHitResult& Hit,
		const IShanmenWorldEntityResolver& EntityResolver,
		FShanmenHitCandidate& OutCandidate);

	static bool IsCompatible(
		EShanmenWorldContactSource ContactSource,
		EShanmenHitDetectorKind DetectorKind);

private:
	static bool TryBuildCandidate(
		const FShanmenWorldHitContext& Context,
		EShanmenWorldContactSource ContactSource,
		const AActor* Actor,
		const UPrimitiveComponent* Component,
		int32 BodyIndex,
		const FVector& ContactLocation,
		const FVector& ContactNormal,
		const IShanmenWorldEntityResolver& EntityResolver,
		FShanmenHitCandidate& OutCandidate);
};
