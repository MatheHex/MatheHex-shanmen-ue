#include "demo_mapPlayerCombatCoordinator.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	bool IsSuccessfulBinding(EShanmenWorldBindingResult Result)
	{
		return Result == EShanmenWorldBindingResult::Bound
			|| Result == EShanmenWorldBindingResult::AlreadyBound;
	}
}

FName Fdemo_mapPlayerCombatCoordinator::PlayerSpawnSourceId()
{
	return TEXT("Spawn.Player.Primary");
}

bool Fdemo_mapPlayerCombatCoordinator::TryBeginRun(
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
			TEXT("Player combat binding requires one valid Run, Pawn, and Pawn-owned health component.");
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
				TEXT("A live player combat coordinator cannot switch Run or product host.");
			return false;
		}
		if (!IsReady())
		{
			OutDiagnostic =
				TEXT("The existing player combat binding is no longer internally consistent.");
			return false;
		}
		OutDiagnostic = TEXT("Player combat Run binding already active.");
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
		TEXT("Player combat identity bound: RunId=%s EntityId=%s."),
		*RunId.ToString(EGuidFormats::DigitsWithHyphens),
		*PlayerEntityId.ToString(EGuidFormats::DigitsWithHyphens));
	return IsReady();
}

bool Fdemo_mapPlayerCombatCoordinator::TryEndRun(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ExpectedRunId.IsValid() || !IsActive()
		|| ExpectedRunId != GetRunId())
	{
		OutDiagnostic = TEXT("Player combat Run end rejected a missing or mismatched RunId.");
		return false;
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

	PlayerEntityId.Invalidate();
	BoundPlayerPawn.Reset();
	BoundPlayerHealth.Reset();
	BoundPlayerRoot.Reset();
	OutDiagnostic = TEXT("Player combat Run identity released.");
	return true;
}

void Fdemo_mapPlayerCombatCoordinator::Reset()
{
	if (BoundPlayerHealth.IsValid() && PlayerEntityId.IsValid())
	{
		BoundPlayerHealth->TryEndCombatEntityBinding(PlayerEntityId);
	}
	EntityRegistry.Reset();
	PlayerEntityId.Invalidate();
	BoundPlayerPawn.Reset();
	BoundPlayerHealth.Reset();
	BoundPlayerRoot.Reset();
}

bool Fdemo_mapPlayerCombatCoordinator::IsReady() const
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
			GetRunId(),
			BoundPlayerPawn.Get(),
			INDEX_NONE,
			ResolvedEntityId)
		|| ResolvedEntityId != PlayerEntityId
		|| !EntityRegistry.TryResolveObject(
			GetRunId(),
			BoundPlayerHealth.Get(),
			INDEX_NONE,
			ResolvedEntityId)
		|| ResolvedEntityId != PlayerEntityId)
	{
		return false;
	}

	return !BoundPlayerRoot.IsValid()
		|| (EntityRegistry.TryResolveObject(
			GetRunId(),
			BoundPlayerRoot.Get(),
			INDEX_NONE,
			ResolvedEntityId)
			&& ResolvedEntityId == PlayerEntityId);
}

Fdemo_mapCombatImpactDeliveryResult
Fdemo_mapPlayerCombatCoordinator::DeliverBasicSwordImpact(
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
			Impact.GetRequest(),
			Impact.GetResult(),
			Command))
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
