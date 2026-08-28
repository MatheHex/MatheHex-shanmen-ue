#include "demo_mapCombatRunCoordinator.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldHitAdapter.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
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

	Idemo_mapCombatVitalityHost* ResolveM01VitalityHost(AActor* EnemyActor)
	{
		return EnemyActor
			? Cast<Idemo_mapCombatVitalityHost>(EnemyActor)
			: nullptr;
	}

	bool TryBuildProductBasicSwordDefinition(
		FShanmenBasicSwordDefinition& OutDefinition)
	{
		FShanmenBasicSwordDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Weapon.Main");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Basic01.Product.r1");
		Capture.BaseDamage = 0.0f;
		Capture.AttackPowerCoefficient = 1.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		return FShanmenBasicSwordDefinition::TryCapture(
			Capture,
			OutDefinition);
	}

	bool TryBuildProductBasicSwordAction(
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		const FGuid& SourceItemInstanceId,
		uint64 ActivationSequence,
		FShanmenCombatActionSnapshot& OutAction)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		// The local player entity is also the action owner for this product
		// slice. Persistent profile identity remains outside CombatCore.
		Capture.OwnerId = PlayerEntityId;
		Capture.SourceEntityId = PlayerEntityId;
		Capture.SourceItemInstanceId = SourceItemInstanceId;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P4.5");
		Capture.Content.Digest =
			TEXT("Shanmen.BasicSword.ProductTrajectory.r1");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			RunId,
			PlayerEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		return FShanmenCombatActionSnapshot::TryCapture(Capture, OutAction);
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
		Idemo_mapCombatVitalityHost* ExpectedVitalityHost =
			ResolveM01VitalityHost(EnemyActor);
		if (Existing->Actor.Get() != EnemyActor
			|| Existing->SpawnMarkerId != Definition.SpawnMarkerId
			|| !ExpectedVitalityHost)
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

	Idemo_mapCombatVitalityHost* VitalityHost =
		ResolveM01VitalityHost(EnemyActor);
	if (!VitalityHost || !VitalityHost->TryBindCombatEntity(EntityId))
	{
		OutDiagnostic =
			TEXT("M01 vitality host rejected its authored World EntityId.");
		return false;
	}

	EntityRegistry = MoveTemp(PreparedRegistry);
	FM01EnemyBinding& Added = M01EnemyBindings.Add(EntityId);
	Added.SpawnMarkerId = Definition.SpawnMarkerId;
	Added.Actor = EnemyActor;
	Added.CollisionRoot = CollisionRoot;
	OutDiagnostic = FString::Printf(
		TEXT("M01 enemy registered: SpawnMarkerId=%s EntityId=%s Vitality=%d."),
		*Definition.SpawnMarkerId.ToString(),
		*EntityId.ToString(EGuidFormats::DigitsWithHyphens),
		1);
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
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get());
		if (VitalityHost
			&& VitalityHost->IsCombatEntityBound()
			&& VitalityHost->GetCombatEntityId() != Pair.Key)
		{
			OutDiagnostic =
				TEXT("An M01 vitality host no longer owns its authored Run identity.");
			return false;
		}
	}

	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get());
		if (VitalityHost
			&& !VitalityHost->TryEndCombatEntityBinding(Pair.Key))
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
	NextPlayerBasicSwordActivationSequence = 1;
	OutDiagnostic = TEXT("Combat Run identities released.");
	return true;
}

void Fdemo_mapCombatRunCoordinator::Reset()
{
	for (const TPair<FGuid, FM01EnemyBinding>& Pair : M01EnemyBindings)
	{
		if (Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get()))
		{
			VitalityHost->TryEndCombatEntityBinding(Pair.Key);
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
	NextPlayerBasicSwordActivationSequence = 1;
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
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(Pair.Value.Actor.Get());
		if (VitalityHost
			&& VitalityHost->IsCombatEntityBound()
			&& VitalityHost->GetCombatEntityId() == Pair.Key)
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
	AActor* TargetEnemy)
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
	Idemo_mapCombatVitalityHost* VitalityHost =
		ResolveM01VitalityHost(TargetEnemy);
	if (!VitalityHost
		|| !VitalityHost->IsCombatEntityBound()
		|| VitalityHost->GetCombatEntityId() != TargetEntityId)
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

	Delivery.CommitResult = VitalityHost->CommitCombatImpact(Command);
	Delivery.Error = Delivery.CommitResult.IsSuccess()
		? Edemo_mapCombatImpactDeliveryError::None
		: Edemo_mapCombatImpactDeliveryError::CommitRejected;
	return Delivery;
}

Fdemo_mapBasicSwordProductExecutionResult
Fdemo_mapCombatRunCoordinator::ExecutePlayerBasicSwordSweep(
	const FGuid& SourceItemInstanceId,
	float AttackPower,
	const TArray<FHitResult>& WorldHits)
{
	Fdemo_mapBasicSwordProductExecutionResult ProductResult;
	ProductResult.WorldContactCount = WorldHits.Num();
	if (!IsReady() || NextPlayerBasicSwordActivationSequence == MAX_uint64)
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::CoordinatorNotReady;
		return ProductResult;
	}
	if (!SourceItemInstanceId.IsValid())
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::InvalidSourceItem;
		return ProductResult;
	}

	FShanmenBasicSwordOffenseSnapshot Offense;
	if (!FShanmenBasicSwordOffenseSnapshot::TryCapture(
			AttackPower,
			Offense))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::InvalidOffense;
		return ProductResult;
	}

	FShanmenCombatActionSnapshot Action;
	if (!TryBuildProductBasicSwordAction(
			GetRunId(),
			PlayerEntityId,
			SourceItemInstanceId,
			NextPlayerBasicSwordActivationSequence,
			Action))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::ActionConstructionFailed;
		return ProductResult;
	}

	FShanmenBasicSwordDefinition Definition;
	if (!TryBuildProductBasicSwordDefinition(Definition))
	{
		ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
			DefinitionConstructionFailed;
		return ProductResult;
	}
	FShanmenBasicSwordExecution SwordExecution;
	if (!FShanmenBasicSwordExecution::TryCreate(
			Action,
			Definition,
			Offense,
			SwordExecution))
	{
		ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
			ExecutionConstructionFailed;
		return ProductResult;
	}

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt Transition;
	if (!FShanmenActionOrchestrator::TryStart(
			Action,
			ActionRuntime,
			Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::RuntimeStartFailed;
		return ProductResult;
	}
	ProductResult.ActivationId = Action.GetActivationId();
	++NextPlayerBasicSwordActivationSequence;

	FShanmenWorldHitContext HitContext;
	if (!SwordExecution.TryBeginEmission(ActionRuntime, HitContext))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::EmissionStartFailed;
		return ProductResult;
	}

	for (const FHitResult& WorldHit : WorldHits)
	{
		AActor* TargetEnemy = WorldHit.GetActor();
		Idemo_mapCombatVitalityHost* VitalityHost =
			ResolveM01VitalityHost(TargetEnemy);
		if (!VitalityHost || !VitalityHost->IsCombatEntityBound())
		{
			continue;
		}

		FShanmenHitCandidate Candidate;
		if (!FShanmenWorldHitAdapter::TryFromSweep(
				HitContext,
				WorldHit,
				EntityRegistry,
				Candidate))
		{
			continue;
		}
		++ProductResult.ResolvedCandidateCount;

		FShanmenTargetVitalitySnapshot Vitality;
		if (!VitalityHost->TryCaptureCombatVitalitySnapshot(Vitality))
		{
			continue;
		}
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenBasicSwordImpactReceipt Impact;
		if (!SwordExecution.TryResolveCandidate(
				ActionRuntime,
				Candidate,
				Vitality,
				Defense,
				Impact))
		{
			continue;
		}

		const Fdemo_mapCombatImpactDeliveryResult Delivery =
			DeliverBasicSwordImpactToM01Enemy(Impact, TargetEnemy);
		if (!Delivery.IsSuccess())
		{
			SwordExecution.EndEmissionForTermination();
			ActionRuntime.TryInterrupt(
				EShanmenCombatActionPhase::Active,
				Transition);
			ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
				DeliveryRejected;
			return ProductResult;
		}
		++ProductResult.DeliveredImpactCount;
		if (Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::Committed)
		{
			++ProductResult.CommittedImpactCount;
		}
		else if (Delivery.CommitResult.Status
			== EShanmenVitalityCommitStatus::AlreadyCommitted)
		{
			++ProductResult.AlreadyCommittedImpactCount;
		}
	}

	if (!SwordExecution.TryEndEmission(ActionRuntime))
	{
		ProductResult.Error =
			Edemo_mapBasicSwordProductExecutionError::EmissionEndFailed;
		return ProductResult;
	}
	if (!ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active,
			Transition)
		|| !ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery,
			Transition))
	{
		ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::
			RuntimeCompletionFailed;
		return ProductResult;
	}

	ProductResult.Error = Edemo_mapBasicSwordProductExecutionError::None;
	return ProductResult;
}
