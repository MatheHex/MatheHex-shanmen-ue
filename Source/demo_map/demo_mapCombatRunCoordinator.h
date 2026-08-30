#pragma once

#include "CoreMinimal.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenVitalityAuthority.h"
#include "ShanmenWorldEntityRegistry.h"
#include "demo_mapCombatVitalityHost.h"

class AActor;
class APawn;
class UPrimitiveComponent;
class UObject;
class Udemo_mapPlayerHealthComponent;
struct Fdemo_mapM01EnemyDefinition;
struct FShanmenControlledWeaponImpactReceipt;
struct FShanmenThrownWeaponImpactReceipt;
struct FHitResult;
enum class Edemo_mapM01BossAttack : uint8;

/** Product-bound failures that occur before or around a canonical vitality commit. */
enum class Edemo_mapCombatImpactDeliveryError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidImpactReceipt,
	RunMismatch,
	SourceMismatch,
	SourceNotRegistered,
	TargetMismatch,
	TargetNotRegistered,
	TargetNotVitalityBound,
	CommandConstructionFailed,
	CommitRejected,
	ResourceCoordinationRejected
};

/** One product delivery attempt. The nested commit result remains the authority receipt. */
struct Fdemo_mapCombatImpactDeliveryResult
{
	Edemo_mapCombatImpactDeliveryError Error =
		Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
	FShanmenVitalityCommitResult CommitResult;

	bool IsSuccess() const
	{
		return Error == Edemo_mapCombatImpactDeliveryError::None
			&& CommitResult.IsSuccess();
	}
};

/** Product enemy-attack families whose identity and formula are frozen here. */
enum class Edemo_mapM01EnemyAttackFamily : uint8
{
	None,
	BasicMelee,
	StandardMeleeDash,
	EnhancedMeleeDash,
	StandardRangedProjectile,
	HeavySector,
	BossSweep,
	BossCharge,
	BossVolleyProjectile
};

/** Frozen pure-kernel receipt for one authored M01 enemy attack contact. */
struct Fdemo_mapM01EnemyAttackImpactReceipt
{
public:
	bool IsValid() const;
	Edemo_mapM01EnemyAttackFamily GetFamily() const { return Family; }
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class Fdemo_mapCombatRunCoordinator;
	Edemo_mapM01EnemyAttackFamily Family =
		Edemo_mapM01EnemyAttackFamily::None;
	FShanmenImpactRequest Request;
	FShanmenImpactResult Result;
};

enum class Edemo_mapM01EnemyAttackExecutionError : uint8
{
	None,
	CoordinatorNotReady,
	SourceNotRegistered,
	TargetMismatch,
	InvalidDamage,
	InvalidSkillProfile,
	InvalidActivationSequence,
	InvalidContact,
	InvalidHitOrdinal,
	SequenceExhausted,
	ActionConstructionFailed,
	RuntimeStartFailed,
	VitalitySnapshotFailed,
	DefenseSnapshotFailed,
	ResourceDefensePreparationFailed,
	ImpactResolutionFailed,
	DeliveryRejected,
	RuntimeCompletionFailed
};

/** Auditable result for one real M01 enemy attack contact decision. */
struct Fdemo_mapM01EnemyAttackExecutionResult
{
	Edemo_mapM01EnemyAttackExecutionError Error =
		Edemo_mapM01EnemyAttackExecutionError::CoordinatorNotReady;
	FGuid ActivationId;
	Fdemo_mapM01EnemyAttackImpactReceipt Impact;
	Fdemo_mapCombatImpactDeliveryResult Delivery;

	bool IsExecuted() const
	{
		return Error == Edemo_mapM01EnemyAttackExecutionError::None
			&& ActivationId.IsValid()
			&& Impact.IsValid()
			&& Delivery.IsSuccess();
	}

	/** Positive only for the first mutation; replay and prevention return zero. */
	float GetNewlyCommittedDamage() const
	{
		return IsExecuted()
			&& Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			? Delivery.CommitResult.Receipt.GetAppliedDamage()
			: 0.0f;
	}

	bool DidNewCommitDefeatTarget() const
	{
		return IsExecuted()
			&& Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			&& Delivery.CommitResult.Receipt.GetVitalityAfter() <= 0.0f;
	}
};

/** Player overlap-skill families whose canonical identity is frozen here. */
enum class Edemo_mapPlayerShapeSkillFamily : uint8
{
	None,
	GroundCircle,
	SelfSector
};

/** Frozen pure-kernel receipt for one player shape-skill target. */
struct Fdemo_mapPlayerShapeSkillImpactReceipt
{
public:
	bool IsValid() const;
	Edemo_mapPlayerShapeSkillFamily GetFamily() const { return Family; }
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class Fdemo_mapCombatRunCoordinator;
	Edemo_mapPlayerShapeSkillFamily Family =
		Edemo_mapPlayerShapeSkillFamily::None;
	FShanmenImpactRequest Request;
	FShanmenImpactResult Result;
};

enum class Edemo_mapPlayerShapeSkillExecutionError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidFamily,
	InvalidDamage,
	SequenceExhausted,
	ActionConstructionFailed,
	RuntimeStartFailed,
	VitalitySnapshotFailed,
	ImpactResolutionFailed,
	DeliveryRejected,
	RuntimeCompletionFailed
};

/** Auditable summary for one real player overlap-skill action. */
struct Fdemo_mapPlayerShapeSkillExecutionResult
{
	Edemo_mapPlayerShapeSkillExecutionError Error =
		Edemo_mapPlayerShapeSkillExecutionError::CoordinatorNotReady;
	Edemo_mapPlayerShapeSkillFamily Family =
		Edemo_mapPlayerShapeSkillFamily::None;
	FGuid ActivationId;
	int32 WorldContactCount = 0;
	int32 ResolvedCandidateCount = 0;
	int32 DeliveredImpactCount = 0;
	int32 CommittedImpactCount = 0;
	int32 AlreadyCommittedImpactCount = 0;
	TArray<FGuid> OrderedTargetEntityIds;
	TArray<Fdemo_mapPlayerShapeSkillImpactReceipt> Impacts;

	bool IsExecuted() const
	{
		return Error == Edemo_mapPlayerShapeSkillExecutionError::None
			&& Family != Edemo_mapPlayerShapeSkillFamily::None
			&& ActivationId.IsValid();
	}

	bool AppliedDamage() const
	{
		return IsExecuted() && CommittedImpactCount > 0;
	}
};

enum class Edemo_mapPlayerProjectileLaunchError : uint8
{
	None,
	CoordinatorNotReady,
	SourceMismatch,
	InvalidDamage,
	SequenceExhausted,
	ActionConstructionFailed
};

/** Frozen launch identity reserved before a player projectile enables collision. */
struct Fdemo_mapPlayerProjectileLaunchResult
{
	Edemo_mapPlayerProjectileLaunchError Error =
		Edemo_mapPlayerProjectileLaunchError::CoordinatorNotReady;
	uint64 ActivationSequence = 0;
	FGuid ActivationId;
	float RawDamage = 0.0f;

	bool IsPrepared() const
	{
		return Error == Edemo_mapPlayerProjectileLaunchError::None
			&& ActivationSequence > 0
			&& ActivationId.IsValid()
			&& FMath::IsFinite(RawDamage)
			&& RawDamage > 0.0f;
	}
};

/** Run-owned deterministic identity reserved for one exact physical throw. */
struct Fdemo_mapPlayerThrownWeaponActionReservation
{
	uint64 ActivationSequence = 0;
	FGuid ActivationId;
	FGuid RunId;
	FGuid SourceEntityId;
	FGuid SourceItemInstanceId;

	bool IsValid() const;
};

/** Frozen pure-kernel receipt for one player Straight Projectile contact. */
struct Fdemo_mapPlayerProjectileImpactReceipt
{
public:
	bool IsValid() const;
	const FShanmenImpactRequest& GetRequest() const { return Request; }
	const FShanmenImpactResult& GetResult() const { return Result; }

private:
	friend class Fdemo_mapCombatRunCoordinator;
	FShanmenImpactRequest Request;
	FShanmenImpactResult Result;
};

enum class Edemo_mapPlayerProjectileImpactError : uint8
{
	None,
	CoordinatorNotReady,
	SourceMismatch,
	InvalidLaunchIdentity,
	InvalidDamage,
	InvalidContact,
	TargetNotRegistered,
	ActionConstructionFailed,
	RuntimeStartFailed,
	CandidateConstructionFailed,
	VitalitySnapshotFailed,
	ImpactResolutionFailed,
	DeliveryRejected,
	RuntimeCompletionFailed
};

/** Auditable result for one real player Straight Projectile hostile contact. */
struct Fdemo_mapPlayerProjectileImpactResult
{
	Edemo_mapPlayerProjectileImpactError Error =
		Edemo_mapPlayerProjectileImpactError::CoordinatorNotReady;
	uint64 ActivationSequence = 0;
	FGuid ActivationId;
	Fdemo_mapPlayerProjectileImpactReceipt Impact;
	Fdemo_mapCombatImpactDeliveryResult Delivery;

	bool IsExecuted() const
	{
		return Error == Edemo_mapPlayerProjectileImpactError::None
			&& ActivationSequence > 0
			&& ActivationId.IsValid()
			&& Impact.IsValid()
			&& Delivery.IsSuccess();
	}

	float GetNewlyCommittedDamage() const
	{
		return IsExecuted()
			&& Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			? Delivery.CommitResult.Receipt.GetAppliedDamage()
			: 0.0f;
	}
};

/** Product execution failures before a BasicSword action can close normally. */
enum class Edemo_mapBasicSwordProductExecutionError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidSourceItem,
	InvalidOffense,
	ActionConstructionFailed,
	RuntimeStartFailed,
	DefinitionConstructionFailed,
	ExecutionConstructionFailed,
	EmissionStartFailed,
	DeliveryRejected,
	EmissionEndFailed,
	RuntimeCompletionFailed
};

/** Auditable summary for one real player-input BasicSword trajectory sample. */
struct Fdemo_mapBasicSwordProductExecutionResult
{
	Edemo_mapBasicSwordProductExecutionError Error =
		Edemo_mapBasicSwordProductExecutionError::CoordinatorNotReady;
	FGuid ActivationId;
	int32 WorldContactCount = 0;
	int32 ResolvedCandidateCount = 0;
	int32 DeliveredImpactCount = 0;
	int32 CommittedImpactCount = 0;
	int32 AlreadyCommittedImpactCount = 0;

	bool IsExecuted() const
	{
		return Error == Edemo_mapBasicSwordProductExecutionError::None
			&& ActivationId.IsValid();
	}

	bool AppliedDamage() const
	{
		return IsExecuted() && CommittedImpactCount > 0;
	}
};

enum class Edemo_mapCombatRunEntityAliasStatus : uint8
{
	Bound,
	AlreadyBound,
	CoordinatorNotReady,
	RegisteredObjectUnavailable,
	AliasObjectUnavailable,
	BodyIndexInvalid,
	RegisteredObjectNotFound,
	AliasConflict,
	RegistryRejected,
	StateInvalid
};

/** Pointer-free receipt for one explicit alias derived from a registered object. */
struct Fdemo_mapCombatRunEntityAliasResult
{
	Edemo_mapCombatRunEntityAliasStatus Status =
		Edemo_mapCombatRunEntityAliasStatus::CoordinatorNotReady;
	FString Diagnostic;
	FGuid RunId;
	FGuid EntityId;
	uint32 RegisteredObjectUniqueId = 0;
	uint32 AliasObjectUniqueId = 0;
	int32 RegisteredBodyIndex = INDEX_NONE;
	int32 AliasBodyIndex = INDEX_NONE;
	EShanmenWorldBindingResult BindingResult =
		EShanmenWorldBindingResult::Invalid;
	int32 BindingCountBefore = 0;
	int32 BindingCountAfter = 0;
	bool bRegisteredObjectResolved = false;
	bool bAliasVerified = false;
	bool bRegistryUpdated = false;

	bool IsSuccess() const;
};

/**
 * Shared product bridge for one authority Run.
 *
 * The persistent player and every authored M01 enemy alias enter one World
 * Entity Registry. Player identity uses the fixed primary-player tuple. M01
 * identity uses only the authored SpawnMarkerId and ordinal zero; transient
 * actor addresses, object names, random loot ids, and spawn callback order
 * never participate. Every authored M01 product host binds the same canonical
 * vitality-host contract and ledger.
 */
class Fdemo_mapCombatRunCoordinator
{
public:
	static FName PlayerSpawnSourceId();
	static FGuid MakeM01EnemyEntityId(
		const FGuid& RunId,
		const Fdemo_mapM01EnemyDefinition& Definition);

	bool TryBeginRun(
		const FGuid& RunId,
		APawn* PlayerPawn,
		Udemo_mapPlayerHealthComponent* PlayerHealth,
		FString& OutDiagnostic);
	/** Registers the actor and collision root from its configured M01 identity. */
	bool TryRegisterM01Enemy(AActor* EnemyActor, FString& OutDiagnostic);
	bool TryEndRun(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsReady() const;
	bool IsActive() const { return EntityRegistry.GetRunId().IsValid(); }
	const FGuid& GetRunId() const { return EntityRegistry.GetRunId(); }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	/**
	 * Binds one caller-owned UObject as an alias of an already registered
	 * object. EntityId is resolved internally and can never be injected by the
	 * caller. The operation is copy-on-success and stores no object pointer.
	 */
	Fdemo_mapCombatRunEntityAliasResult TryBindEntityAlias(
		const UObject* RegisteredObject,
		int32 RegisteredBodyIndex,
		const UObject* AliasObject,
		int32 AliasBodyIndex = INDEX_NONE);
	int32 NumRegisteredM01Enemies() const { return M01EnemyBindings.Num(); }
	int32 NumVitalityBoundM01Enemies() const;
	const FShanmenWorldEntityRegistry& GetEntityRegistry() const
	{
		return EntityRegistry;
	}

	Fdemo_mapCombatImpactDeliveryResult DeliverBasicSwordImpactToPlayer(
		const FShanmenBasicSwordImpactReceipt& Impact);
	Fdemo_mapCombatImpactDeliveryResult DeliverBasicSwordImpactToM01Enemy(
		const FShanmenBasicSwordImpactReceipt& Impact,
		AActor* TargetEnemy);
	Fdemo_mapCombatImpactDeliveryResult DeliverPlayerShapeSkillImpactToM01Enemy(
		const Fdemo_mapPlayerShapeSkillImpactReceipt& Impact,
		AActor* TargetEnemy);
	Fdemo_mapCombatImpactDeliveryResult
	DeliverPlayerProjectileImpactToM01Enemy(
		const Fdemo_mapPlayerProjectileImpactReceipt& Impact,
		AActor* TargetEnemy);
	/** Delivers an already-resolved controlled-weapon receipt through the canonical vitality authority. */
	Fdemo_mapCombatImpactDeliveryResult
	DeliverControlledWeaponImpactToM01Enemy(
		const FShanmenControlledWeaponImpactReceipt& Impact,
		AActor* TargetEnemy);
	/** Delivers an already-resolved physical thrown-item receipt through canonical vitality. */
	Fdemo_mapCombatImpactDeliveryResult
	DeliverThrownWeaponImpactToM01Enemy(
		const FShanmenThrownWeaponImpactReceipt& Impact,
		AActor* TargetEnemy);
	Fdemo_mapCombatImpactDeliveryResult
	DeliverM01EnemyAttackImpactToPlayer(
		const Fdemo_mapM01EnemyAttackImpactReceipt& Impact,
		AActor* SourceEnemy);
	/**
	 * Resolves one already-authorized M01 basic melee contact. Geometry,
	 * cooldown, faction, and range stay with the product Actor; this boundary
	 * owns stable action identity, player defense capture, pure resolution, and
	 * the only player-vitality write.
	 */
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyBasicMeleeStrike(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		float RawDamage);
	/**
	 * Resolves one legal contact from an already-running authored melee dash.
	 * The skill runtime's Run-reset ActivationSerial is the stable action
	 * sequence, so repeated delivery of the same contact is idempotent.
	 */
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyMeleeDashContact(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		FName SkillProfileId,
		uint32 ActivationSerial,
		float RawDamage);
	/**
	 * Resolves one hostile contact from an authored M01 ranged projectile.
	 * ProjectileSequence is reserved before flight and reset only with the Run;
	 * the world contact remains geometry-only input to this canonical boundary.
	 */
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyRangedProjectileImpact(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		FName SkillProfileId,
		uint64 ProjectileSequence,
		float RawDamage,
		const FVector& ImpactLocation,
		const FVector& ImpactNormal);
	/**
	 * Resolves one authorized M01 heavy-sector contact. AttackSequence is
	 * reserved when the windup begins and therefore survives delayed resolve
	 * without borrowing the legacy ResolveCount diagnostic.
	 */
	Fdemo_mapM01EnemyAttackExecutionResult
	ExecuteM01EnemyHeavySectorAttack(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		uint64 AttackSequence,
		float RawDamage);
	/** Resolves one legal sweep or charge contact from the authored M01 Boss. */
	Fdemo_mapM01EnemyAttackExecutionResult ExecuteM01BossShapeAttack(
		AActor* SourceBoss,
		APawn* TargetPlayer,
		Edemo_mapM01BossAttack Attack,
		uint64 AttackSequence,
		float RawDamage);
	/**
	 * Resolves one projectile in an authored three-shot Boss volley. All three
	 * contacts share AttackSequence and differ only by ProjectileOrdinal 0..2.
	 */
	Fdemo_mapM01EnemyAttackExecutionResult ExecuteM01BossVolleyProjectileImpact(
		AActor* SourceBoss,
		APawn* TargetPlayer,
		uint64 AttackSequence,
		int32 ProjectileOrdinal,
		float RawDamage,
		const FVector& ImpactLocation,
		const FVector& ImpactNormal);
	/**
	 * Executes one complete player BasicSword action from an already sampled UE
	 * trajectory. Every accepted contact resolves through this Run's Registry;
	 * canonical vitality delivery is the only mutable damage path.
	 */
	Fdemo_mapBasicSwordProductExecutionResult ExecutePlayerBasicSwordSweep(
		const FGuid& SourceItemInstanceId,
		float AttackPower,
		const TArray<FHitResult>& WorldHits);
	/**
	 * Executes one already-authorized player overlap query. Geometry and faction
	 * policy stay with SkillComponent; this boundary owns Run-local identity,
	 * stable target order, pure resolution, and canonical vitality delivery.
	 */
	Fdemo_mapPlayerShapeSkillExecutionResult ExecutePlayerShapeSkill(
		Edemo_mapPlayerShapeSkillFamily Family,
		float RawDamage,
		const TArray<struct FOverlapResult>& WorldOverlaps,
		const FVector& ContactOrigin);
	/** Reserves deterministic identity after spawn and before collision starts. */
	Fdemo_mapPlayerProjectileLaunchResult PreparePlayerStraightProjectile(
		AActor* SourcePlayer,
		float RawDamage);
	/** Reserves the next Run-local action identity without choosing product data. */
	bool TryReservePlayerThrownWeaponAction(
		const FGuid& SourceItemInstanceId,
		Fdemo_mapPlayerThrownWeaponActionReservation& OutReservation,
		FString& OutDiagnostic);
	/** Resolves one already-authorized hostile projectile contact. */
	Fdemo_mapPlayerProjectileImpactResult
	ExecutePlayerStraightProjectileImpact(
		AActor* SourcePlayer,
		AActor* TargetEnemy,
		UPrimitiveComponent* TargetComponent,
		uint64 ActivationSequence,
		const FGuid& ExpectedActivationId,
		float RawDamage,
		const FVector& ImpactLocation,
		const FVector& ImpactNormal);
	uint64 GetNextPlayerBasicSwordActivationSequence() const
	{
		return NextPlayerBasicSwordActivationSequence;
	}
	uint64 GetNextPlayerShapeSkillActivationSequence(
		Edemo_mapPlayerShapeSkillFamily Family) const;
	uint64 GetNextPlayerStraightProjectileActivationSequence() const
	{
		return NextPlayerStraightProjectileActivationSequence;
	}
	uint64 GetNextPlayerThrownWeaponActivationSequence() const
	{
		return NextPlayerThrownWeaponActivationSequence;
	}

private:
	struct FM01EnemyBinding
	{
		FName SpawnMarkerId = NAME_None;
		FName SkillProfileId = NAME_None;
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UPrimitiveComponent> CollisionRoot;
	};

	Fdemo_mapM01EnemyAttackExecutionResult ExecuteM01EnemyAttack(
		AActor* SourceEnemy,
		APawn* TargetPlayer,
		float RawDamage,
		Edemo_mapM01EnemyAttackFamily Family,
		uint64 RequestedActivationSequence,
		int32 RequestedHitOrdinal,
		const FVector& RequestedHitLocation,
		const FVector& RequestedHitNormal);
	Fdemo_mapCombatImpactDeliveryResult
	DeliverResolvedPlayerImpactToM01Enemy(
		bool bImpactValid,
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Result,
		AActor* TargetEnemy);

	FShanmenWorldEntityRegistry EntityRegistry;
	FGuid PlayerEntityId;
	TWeakObjectPtr<APawn> BoundPlayerPawn;
	TWeakObjectPtr<Udemo_mapPlayerHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UPrimitiveComponent> BoundPlayerRoot;
	bool bPlayerRequiresResourceDefenseAuthority = false;
	TMap<FGuid, FM01EnemyBinding> M01EnemyBindings;
	TMap<FGuid, uint64> NextM01EnemyBasicMeleeActivationSequences;
	uint64 NextPlayerBasicSwordActivationSequence = 1;
	uint64 NextPlayerGroundCircleActivationSequence = 1;
	uint64 NextPlayerSelfSectorActivationSequence = 1;
	uint64 NextPlayerStraightProjectileActivationSequence = 1;
	uint64 NextPlayerThrownWeaponActivationSequence = 1;
};
