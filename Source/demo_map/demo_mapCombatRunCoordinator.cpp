#include "demo_mapCombatRunCoordinator.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapHeavyEnemyCharacter.h"
#include "demo_mapM01BossCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRangedEnemyCharacter.h"

namespace
{
	bool IsSuccessfulBinding(EShanmenWorldBindingResult Result)
	{
		return Result == EShanmenWorldBindingResult::Bound
			|| Result == EShanmenWorldBindingResult::AlreadyBound;
	}

	bool IsExpectedM01ProductActor(
		const Fdemo_mapM01EnemyDefinition& Definition,
		const AActor* EnemyActor)
	{
		switch (Definition.Archetype)
		{
		case Edemo_mapM01EnemyArchetype::StandardSkirmisher:
		case Edemo_mapM01EnemyArchetype::EliteStalker:
			return EnemyActor->IsA<Ademo_mapEnemyCharacter>();
		case Edemo_mapM01EnemyArchetype::StandardRanged:
			return EnemyActor->IsA<Ademo_mapRangedEnemyCharacter>();
		case Edemo_mapM01EnemyArchetype::StandardBruiser:
		case Edemo_mapM01EnemyArchetype::EliteBulwark:
			return EnemyActor->IsA<Ademo_mapHeavyEnemyCharacter>();
		case Edemo_mapM01EnemyArchetype::BossMain:
			return EnemyActor->IsA<Ademo_mapM01BossCharacter>();
		default:
			return false;
		}
	}
}

FName Fdemo_mapCombatRunCoordinator::PlayerSpawnSourceId()
{
	return TEXT("Spawn.Player.Primary");
}

FGuid Fdemo_mapCombatRunCoordinator::MakeM01EnemyEntityId(
	const FGuid& RunId,
	const Fdemo_mapM01EnemyDefinition& Definition)
{
	if (!Definition.IsValid())
	{
		return FGuid();
	}
	return FShanmenWorldEntityIdFactory::MakeEntityId(
		RunId,
		Definition.SpawnMarkerId,
		0);
}

bool Fdemo_mapCombatRunCoordinator::TryBeginRun(
	const FGuid& RunId,
	APawn* PlayerPawn,
	Udemo_mapPlayerHealthComponent* PlayerHealth,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!RunId.IsValid() || !PlayerPawn || !PlayerHealth
		|| PlayerHealth->GetOwner() != PlayerPawn)
	{
		OutDiagnostic =
			TEXT("Combat Run binding requires one valid Run, Pawn, and Pawn-owned health component.");
		return false;
	}

	const FGuid ExpectedEntityId =
		FShanmenWorldEntityIdFactory::MakeEntityId(
			RunId,
			PlayerSpawnSourceId(),
			0);
	if (!ExpectedEntityId.IsValid())
	{
		OutDiagnostic = TEXT("Player EntityId derivation failed closed.");
		return false;
	}
	if (PlayerHealth->IsCombatEntityBound()
		&& PlayerHealth->GetCombatEntityId() != ExpectedEntityId)
	{
		OutDiagnostic =
			TEXT("Player health is still bound to a different Run identity.");
		return false;
	}

	if (IsActive())
	{
		if (GetRunId() != RunId
			|| PlayerEntityId != ExpectedEntityId
			|| BoundPlayerPawn.Get() != PlayerPawn
			|| BoundPlayerHealth.Get() != PlayerHealth)
		{
			OutDiagnostic =
				TEXT("A live combat Run coordinator cannot switch Run or player host.");
			return false;
		}
		if (!IsReady())
		{
			OutDiagnostic =
				TEXT("The existing combat Run binding is no longer internally consistent.");
			return false;
		}
		OutDiagnostic = TEXT("Combat Run binding already active.");
		return true;
	}

	FShanmenWorldEntityRegistry PreparedRegistry;
	if (!PreparedRegistry.TryBeginRun(RunId)
		|| !IsSuccessfulBinding(PreparedRegistry.BindObject(
			RunId,
			PlayerPawn,
			ExpectedEntityId))
		|| !IsSuccessfulBinding(PreparedRegistry.BindObject(
			RunId,
			PlayerHealth,
			ExpectedEntityId)))
	{
		OutDiagnostic = TEXT("World Entity Registry rejected the player aliases.");
		return false;
	}

	UPrimitiveComponent* PlayerRoot =
		Cast<UPrimitiveComponent>(PlayerPawn->GetRootComponent());
	if (PlayerRoot
		&& !IsSuccessfulBinding(PreparedRegistry.BindObject(
			RunId,
			PlayerRoot,
			ExpectedEntityId)))
	{
		OutDiagnostic =
			TEXT("World Entity Registry rejected the player collision-root alias.");
		return false;
	}
	if (!PlayerHealth->TryBindCombatEntity(ExpectedEntityId))
	{
		OutDiagnostic =
			TEXT("Player vitality host rejected the stable World EntityId.");
		return false;
	}

	EntityRegistry = MoveTemp(PreparedRegistry);
	PlayerEntityId = ExpectedEntityId;
	BoundPlayerPawn = PlayerPawn;
	BoundPlayerHealth = PlayerHealth;
	BoundPlayerRoot = PlayerRoot;
	OutDiagnostic = FString::Printf(
		TEXT("Combat Run bound: RunId=%s PlayerEntityId=%s."),
		*RunId.ToString(EGuidFormats::DigitsWithHyphens),
		*PlayerEntityId.ToString(EGuidFormats::DigitsWithHyphens));
	return IsReady();
}

bool Fdemo_mapCombatRunCoordinator::TryRegisterM01Enemy(
	AActor* EnemyActor,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsReady() || !EnemyActor)
	{
		OutDiagnostic =
			TEXT("M01 enemy registration requires an active combat Run and actor.");
		return false;
	}

	const Udemo_mapM01EnemyIdentityComponent* Identity =
		EnemyActor->FindComponentByClass<Udemo_mapM01EnemyIdentityComponent>();
	if (!Identity || !Identity->IsConfigured())
	{
		OutDiagnostic =
			TEXT("M01 enemy registration requires one configured authored identity component.");
		return false;
	}
	const Fdemo_mapM01EnemyDefinition& Definition = Identity->GetDefinition();
	if (!IsExpectedM01ProductActor(Definition, EnemyActor))
	{
		OutDiagnostic =
			TEXT("M01 authored archetype does not match the spawned product actor class.");
		return false;
	}
	const FGuid EntityId = MakeM01EnemyEntityId(GetRunId(), Definition);
	if (!EntityId.IsValid())
	{
		OutDiagnostic = TEXT("M01 authored EntityId derivation failed closed.");
		return false;
	}

	if (const FM01EnemyBinding* Existing = M01EnemyBindings.Find(EntityId))
	{
		const Ademo_mapEnemyCharacter* ExpectedVitalityHost =
			Cast<Ademo_mapEnemyCharacter>(EnemyActor);
		if (Existing->Actor.Get() != EnemyActor
			|| Existing->SpawnMarkerId != Definition.SpawnMarkerId
			|| Existing->VitalityHost.Get() != ExpectedVitalityHost)
		{
			OutDiagnostic =
				TEXT("M01 authored EntityId is already owned by a different product actor.");
			return false;
		}
		FGuid ResolvedEntityId;
		if (!EntityRegistry.TryResolveObject(
			GetRunId(), EnemyActor, INDEX_NONE, ResolvedEntityId)
			|| ResolvedEntityId != EntityId
			|| (ExpectedVitalityHost
				&& (!ExpectedVitalityHost->IsCombatEntityBound()
					|| ExpectedVitalityHost->GetCombatEntityId() != EntityId)))
		{
			OutDiagnostic =
				TEXT("Existing M01 enemy binding is no longer internally consistent.");
			return false;
		}
		OutDiagnostic = TEXT("M01 enemy identity already registered.");
		return true;
	}

	FShanmenWorldEntityRegistry PreparedRegistry = EntityRegistry;
	if (!IsSuccessfulBinding(PreparedRegistry.BindObject(
		GetRunId(), EnemyActor, EntityId)))
	{
		OutDiagnostic =
			TEXT("World Entity Registry rejected the M01 enemy actor alias.");
		return false;
	}
	UPrimitiveComponent* CollisionRoot =
		Cast<UPrimitiveComponent>(EnemyActor->GetRootComponent());
	if (CollisionRoot
		&& !IsSuccessfulBinding(PreparedRegistry.BindObject(
			GetRunId(), CollisionRoot, EntityId)))
	{
		OutDiagnostic =
			TEXT("World Entity Registry rejected the M01 collision-root alias.");
		return false;
	}

	Ademo_mapEnemyCharacter* VitalityHost =
		Cast<Ademo_mapEnemyCharacter>(EnemyActor);
	if (VitalityHost && !VitalityHost->TryBindCombatEntity(EntityId))
	{
		OutDiagnostic =
			TEXT("M01 melee vitality host rejected its authored World EntityId.");
		return false;
	}

	EntityRegistry = MoveTemp(PreparedRegistry);
	FM01EnemyBinding& Added = M01EnemyBindings.Add(EntityId);
	Added.SpawnMarkerId = Definition.SpawnMarkerId;
	Added.Actor = EnemyActor;
	Added.CollisionRoot = CollisionRoot;
	Added.VitalityHost = VitalityHost;
	OutDiagnostic = FString::Printf(
		TEXT("M01 enemy registered: SpawnMarkerId=%s EntityId=%s Vitality=%d."),
		*Definition.SpawnMarkerId.ToString(),
		*EntityId.ToString(EGuidFormats::DigitsWithHyphens),
		VitalityHost ? 1 : 0);
	return true;
}

bool Fdemo_mapCombatRunCoordinator::TryEndRun(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ExpectedRunId.IsValid() || !IsActive()
		|| ExpectedRunId != GetRunId())
	{
		OutDiagnostic =
			TEXT("Combat Run end rejected a missing or mismatched RunId.");
		return false;
	}

	if (BoundPlayerHealth.IsValid()
		&& BoundPlayerHealth->IsCombatEntityBound()
		&& BoundPlayerHealth->GetCombatEntityId() != PlayerEntityId)
	{
		OutDiagnostic =
			TEXT("Player vitality host no longer owns the expected Run identity.");
		return false;
	}
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		if (Pair.Value.VitalityHost.IsValid()
			&& Pair.Value.VitalityHost->IsCombatEntityBound()
			&& Pair.Value.VitalityHost->GetCombatEntityId() != Pair.Key)
		{
			OutDiagnostic =
				TEXT("An M01 vitality host no longer owns its authored Run identity.");
			return false;
		}
	}

	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		if (Pair.Value.VitalityHost.IsValid()
			&& !Pair.Value.VitalityHost->TryEndCombatEntityBinding(Pair.Key))
		{
			OutDiagnostic =
				TEXT("An M01 vitality host rejected the exact Run identity release.");
			return false;
		}
	}
	if (BoundPlayerHealth.IsValid()
		&& !BoundPlayerHealth->TryEndCombatEntityBinding(PlayerEntityId))
	{
		OutDiagnostic =
			TEXT("Player vitality host rejected the exact Run identity release.");
		return false;
	}
	if (!EntityRegistry.TryEndRun(ExpectedRunId))
	{
		OutDiagnostic = TEXT("World Entity Registry rejected the exact Run end.");
		return false;
	}

	M01EnemyBindings.Reset();
	PlayerEntityId.Invalidate();
	BoundPlayerPawn.Reset();
	BoundPlayerHealth.Reset();
	BoundPlayerRoot.Reset();
	OutDiagnostic = TEXT("Combat Run identities released.");
	return true;
}

void Fdemo_mapCombatRunCoordinator::Reset()
{
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		if (Pair.Value.VitalityHost.IsValid())
		{
			Pair.Value.VitalityHost->TryEndCombatEntityBinding(Pair.Key);
		}
	}
	if (BoundPlayerHealth.IsValid() && PlayerEntityId.IsValid())
	{
		BoundPlayerHealth->TryEndCombatEntityBinding(PlayerEntityId);
	}
	EntityRegistry.Reset();
	M01EnemyBindings.Reset();
	PlayerEntityId.Invalidate();
	BoundPlayerPawn.Reset();
	BoundPlayerHealth.Reset();
	BoundPlayerRoot.Reset();
}

bool Fdemo_mapCombatRunCoordinator::IsReady() const
{
	if (!IsActive() || !PlayerEntityId.IsValid()
		|| !BoundPlayerPawn.IsValid() || !BoundPlayerHealth.IsValid()
		|| !BoundPlayerHealth->IsCombatEntityBound()
		|| BoundPlayerHealth->GetCombatEntityId() != PlayerEntityId)
	{
		return false;
	}

	FGuid ResolvedEntityId;
	if (!EntityRegistry.TryResolveObject(
		GetRunId(), BoundPlayerPawn.Get(), INDEX_NONE, ResolvedEntityId)
		|| ResolvedEntityId != PlayerEntityId
		|| !EntityRegistry.TryResolveObject(
			GetRunId(), BoundPlayerHealth.Get(), INDEX_NONE, ResolvedEntityId)
		|| ResolvedEntityId != PlayerEntityId)
	{
		return false;
	}

	return !BoundPlayerRoot.IsValid()
		|| (EntityRegistry.TryResolveObject(
			GetRunId(), BoundPlayerRoot.Get(), INDEX_NONE, ResolvedEntityId)
			&& ResolvedEntityId == PlayerEntityId);
}

int32 Fdemo_mapCombatRunCoordinator::NumVitalityBoundM01Enemies() const
{
	int32 Count = 0;
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		if (Pair.Value.VitalityHost.IsValid()
			&& Pair.Value.VitalityHost->IsCombatEntityBound()
			&& Pair.Value.VitalityHost->GetCombatEntityId() == Pair.Key)
		{
			++Count;
		}
	}
	return Count;
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverBasicSwordImpactToPlayer(
	const FShanmenBasicSwordImpactReceipt& Impact)
{
	Fdemo_mapCombatImpactDeliveryResult Delivery;
	if (!IsReady())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
		return Delivery;
	}
	if (!Impact.IsValid())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::InvalidImpactReceipt;
		return Delivery;
	}
	if (Impact.GetRequest().Action.GetRunId() != GetRunId())
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::RunMismatch;
		return Delivery;
	}
	if (Impact.GetRequest().Candidate.TargetEntityId != PlayerEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::TargetMismatch;
		return Delivery;
	}

	FShanmenVitalityCommitCommand Command;
	if (!FShanmenVitalityCommitCommand::TryCreate(
		Impact.GetRequest(), Impact.GetResult(), Command))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CommandConstructionFailed;
		return Delivery;
	}

	Delivery.CommitResult = BoundPlayerHealth->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapCombatRunCoordinator::DeliverBasicSwordImpactToM01Enemy(
	const FShanmenBasicSwordImpactReceipt& Impact,
	Ademo_mapEnemyCharacter* TargetEnemy)
{
	Fdemo_mapCombatImpactDeliveryResult Delivery;
	if (!IsReady())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
		return Delivery;
	}
	if (!Impact.IsValid())
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::InvalidImpactReceipt;
		return Delivery;
	}
	if (Impact.GetRequest().Action.GetRunId() != GetRunId())
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::RunMismatch;
		return Delivery;
	}
	if (Impact.GetRequest().Action.GetSourceEntityId() != PlayerEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::SourceMismatch;
		return Delivery;
	}
	if (!TargetEnemy)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}

	FGuid TargetEntityId;
	if (!EntityRegistry.TryResolveObject(
		GetRunId(), TargetEnemy, INDEX_NONE, TargetEntityId))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}
	if (Impact.GetRequest().Candidate.TargetEntityId != TargetEntityId)
	{
		Delivery.Error = Edemo_mapCombatImpactDeliveryError::TargetMismatch;
		return Delivery;
	}
	const FM01EnemyBinding* Binding = M01EnemyBindings.Find(TargetEntityId);
	if (!Binding || Binding->Actor.Get() != TargetEnemy)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotRegistered;
		return Delivery;
	}
	if (!Binding->VitalityHost.IsValid()
		|| Binding->VitalityHost.Get() != TargetEnemy
		|| !TargetEnemy->IsCombatEntityBound()
		|| TargetEnemy->GetCombatEntityId() != TargetEntityId)
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::TargetNotVitalityBound;
		return Delivery;
	}

	FShanmenVitalityCommitCommand Command;
	if (!FShanmenVitalityCommitCommand::TryCreate(
		Impact.GetRequest(), Impact.GetResult(), Command))
	{
		Delivery.Error =
			Edemo_mapCombatImpactDeliveryError::CommandConstructionFailed;
		return Delivery;
	}

	Delivery.CommitResult = TargetEnemy->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}
