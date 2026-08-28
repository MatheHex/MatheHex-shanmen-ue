#include "demo_mapPlayerHealthComponent.h"
#include "demo_map.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "TimerManager.h"

Udemo_mapPlayerHealthComponent::Udemo_mapPlayerHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void Udemo_mapPlayerHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ProcessedRestoreHealthReceiptIds.Reset();
	CombatVitalityLedger.Reset();
	if (AttributeComponent.IsValid() && AttributeChangedHandle.IsValid())
	{
		AttributeComponent->OnAttributeChanged.Remove(AttributeChangedHandle);
		AttributeChangedHandle.Reset();
	}
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &Udemo_mapPlayerHealthComponent::HandleOwnerDamaged);
	}
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageFeedbackTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void Udemo_mapPlayerHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	BindAttributes();
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &Udemo_mapPlayerHealthComponent::HandleOwnerDamaged);
	}
}

bool Udemo_mapPlayerHealthComponent::ShouldDodge(float DodgeChance, float NormalizedRoll)
{
	if (!FMath::IsFinite(DodgeChance) || DodgeChance <= 0.0f) return false;
	if (DodgeChance >= 1.0f) return true;
	if (!FMath::IsFinite(NormalizedRoll) || NormalizedRoll < 0.0f || NormalizedRoll >= 1.0f) return false;
	return NormalizedRoll < DodgeChance;
}

void Udemo_mapPlayerHealthComponent::BindAttributes()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr) return;
	Udemo_mapAttributeComponent* Attributes = Owner->FindComponentByClass<Udemo_mapAttributeComponent>();
	if (Attributes == nullptr)
	{
		Attributes = NewObject<Udemo_mapAttributeComponent>(Owner, TEXT("RuntimePlayerAttributes"));
		if (Attributes != nullptr) Attributes->RegisterComponent();
	}
	BindAttributeComponent(Attributes, true);
}

bool Udemo_mapPlayerHealthComponent::BindAttributeComponent(
	Udemo_mapAttributeComponent* Attributes,
	bool bInitializeCurrentHealth)
{
	if (Attributes == nullptr)
	{
		return false;
	}
	if (AttributeComponent.Get() != Attributes)
	{
		if (AttributeComponent.IsValid() && AttributeChangedHandle.IsValid())
		{
			AttributeComponent->OnAttributeChanged.Remove(AttributeChangedHandle);
			AttributeChangedHandle.Reset();
		}
		AttributeComponent = Attributes;
	}
	if (!AttributeChangedHandle.IsValid())
	{
		AttributeChangedHandle =
			Attributes->OnAttributeChanged.AddUObject(
				this,
				&Udemo_mapPlayerHealthComponent::HandleAttributeChanged);
	}
	ApplyMaxHealthFromAttributes(bInitializeCurrentHealth);
	return true;
}

void Udemo_mapPlayerHealthComponent::ApplyMaxHealthFromAttributes(bool bInitial)
{
	float FinalMaxHealth = 5.0f;
	if (AttributeComponent.IsValid()) AttributeComponent->GetFinalValue(Fdemo_mapAttributeIds::MaxHealth, FinalMaxHealth);
	const float NewMaximum = static_cast<float>(FMath::Max(1, FMath::RoundToInt(FinalMaxHealth)));
	const float NewCurrent = bInitial
		? NewMaximum
		: FMath::Clamp(CurrentVitality, 0.0f, NewMaximum);
	if (!TryCommitVitalityState(NewCurrent, NewMaximum))
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.0.10 P4.2: rejected desynchronized MaxHealth mutation."));
	}
}

bool Udemo_mapPlayerHealthComponent::TryBindCombatEntity(const FGuid& TargetEntityId)
{
	if (!TargetEntityId.IsValid())
	{
		return false;
	}
	if (CombatVitalityLedger.IsValid())
	{
		return CombatVitalityLedger.GetTargetEntityId() == TargetEntityId
			&& CombatVitalityLedger.IsSynchronized(CurrentVitality, MaximumVitality);
	}

	FShanmenVitalityCommitLedger NewLedger;
	if (!FShanmenVitalityCommitLedger::TryCreate(
		TargetEntityId,
		CurrentVitality,
		MaximumVitality,
		0,
		NewLedger))
	{
		return false;
	}
	CombatVitalityLedger = MoveTemp(NewLedger);
	return true;
}

bool Udemo_mapPlayerHealthComponent::TryCaptureCombatVitalitySnapshot(
	FShanmenTargetVitalitySnapshot& OutSnapshot) const
{
	return CombatVitalityLedger.TryCaptureSnapshot(
		CurrentVitality,
		MaximumVitality,
		OutSnapshot);
}

FShanmenVitalityCommitResult Udemo_mapPlayerHealthComponent::CommitCombatImpact(
	const FShanmenVitalityCommitCommand& Command)
{
	FShanmenVitalityCommitResult Result = CombatVitalityLedger.Commit(
		Command,
		CurrentVitality,
		MaximumVitality);
	if (Result.Status == EShanmenVitalityCommitStatus::Committed
		&& Result.Receipt.GetAppliedDamage() > 0.0f)
	{
		PublishAppliedDamage(Result.Receipt.GetAppliedDamage());
	}
	return Result;
}

bool Udemo_mapPlayerHealthComponent::TryCommitVitalityState(
	float NewCurrentVitality,
	float NewMaximumVitality)
{
	if (!FMath::IsFinite(NewCurrentVitality)
		|| !FMath::IsFinite(NewMaximumVitality)
		|| NewMaximumVitality <= 0.0f
		|| NewCurrentVitality < 0.0f
		|| NewCurrentVitality > NewMaximumVitality)
	{
		return false;
	}
	if (CombatVitalityLedger.IsValid())
	{
		return CombatVitalityLedger.TryCommitExternalMutation(
			CurrentVitality,
			MaximumVitality,
			NewCurrentVitality,
			NewMaximumVitality);
	}
	CurrentVitality = NewCurrentVitality;
	MaximumVitality = NewMaximumVitality;
	return true;
}

void Udemo_mapPlayerHealthComponent::HandleAttributeChanged(const Fdemo_mapAttributeChange& Change)
{
	if (Change.AttributeId == Fdemo_mapAttributeIds::MaxHealth) ApplyMaxHealthFromAttributes(false);
}

void Udemo_mapPlayerHealthComponent::HandleOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	ApplyIncomingDamage(Damage);
}

int32 Udemo_mapPlayerHealthComponent::ResolveAppliedDamage(
	float RawDamage,
	float FlatDamageReduction)
{
	if (!FMath::IsFinite(RawDamage)
		|| !FMath::IsFinite(FlatDamageReduction)
		|| RawDamage <= 0.0f)
	{
		return 0;
	}
	return FMath::Max(
		0,
		FMath::FloorToInt(
			FMath::Max(0.0f, RawDamage - FMath::Max(0.0f, FlatDamageReduction))));
}

int32 Udemo_mapPlayerHealthComponent::ApplyIncomingDamage(float RawDamage)
{
	if (bIsDefeated || RawDamage <= 0.0f)
	{
		return 0;
	}

	float DodgeChance = 0.0f;
	if (AttributeComponent.IsValid()) AttributeComponent->GetFinalValue(Fdemo_mapAttributeIds::DodgeChance, DodgeChance);
	const bool bDodged = DodgeChance >= 1.0f || (DodgeChance > 0.0f && ShouldDodge(DodgeChance, FMath::FRand()));
	if (bDodged)
	{
		UE_LOG(Logdemo_map, Log, TEXT("0.3.1.0: player dodged incoming damage."));
		return 0;
	}

	float FlatDamageReduction = 0.0f;
	if (AttributeComponent.IsValid())
	{
		AttributeComponent->GetFinalValue(
			Fdemo_mapAttributeIds::FlatDamageReduction,
			FlatDamageReduction);
	}
	const int32 AppliedDamage =
		ResolveAppliedDamage(RawDamage, FlatDamageReduction);
	if (AppliedDamage == 0)
	{
		return 0;
	}

	const float BeforeVitality = CurrentVitality;
	const float AfterVitality = FMath::Max(
		0.0f,
		CurrentVitality - static_cast<float>(AppliedDamage));
	if (!TryCommitVitalityState(AfterVitality, MaximumVitality))
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.0.10 P4.2: rejected desynchronized legacy damage mutation."));
		return 0;
	}
	PublishAppliedDamage(BeforeVitality - AfterVitality);
	return AppliedDamage;
}

void Udemo_mapPlayerHealthComponent::PublishAppliedDamage(float AppliedDamage)
{
	if (!FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f)
	{
		return;
	}
#if WITH_DEV_AUTOMATION_TESTS
	++PositiveDamageBroadcastCount;
#endif
	const int32 CompatibilityDamage = FMath::Max(1, FMath::CeilToInt(AppliedDamage));
	OnPlayerDamaged.Broadcast(CompatibilityDamage);
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("0.0.10 P4.2: player damaged; vitality=%.3f/%.3f."),
		CurrentVitality,
		MaximumVitality);
	if (AActor* Owner = GetOwner(); Owner != nullptr && GetWorld() != nullptr)
	{
		UPointLightComponent* Flash = Owner->FindComponentByClass<UPointLightComponent>();
		if (Flash == nullptr)
		{
			Flash = NewObject<UPointLightComponent>(Owner, TEXT("DamageFeedbackLight"));
			Flash->SetupAttachment(Owner->GetRootComponent());
			Flash->SetLightColor(FLinearColor::Red);
			Flash->SetAttenuationRadius(420.0f);
			Flash->RegisterComponent();
		}
		Flash->SetIntensity(9000.0f);
		GetWorld()->GetTimerManager().SetTimer(DamageFeedbackTimer, this, &Udemo_mapPlayerHealthComponent::ClearDamageFeedback, 0.22f, false);
	}
	if (CurrentVitality <= 0.0f)
	{
		EnterDefeatedState();
	}
}

int32 Udemo_mapPlayerHealthComponent::ApplyHealing(int32 RequestedHealing)
{
	if (RequestedHealing <= 0
		|| bIsDefeated
		|| CurrentVitality >= MaximumVitality)
	{
		return 0;
	}
	const float Before = CurrentVitality;
	const float After = FMath::Clamp(
		CurrentVitality + static_cast<float>(RequestedHealing),
		0.0f,
		MaximumVitality);
	if (!TryCommitVitalityState(After, MaximumVitality))
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.0.10 P4.2: rejected desynchronized healing mutation."));
		return 0;
	}
	return FMath::RoundToInt(After - Before);
}

bool Udemo_mapPlayerHealthComponent::ApplyRestoreHealthReceipt(
	const FGuid& ReceiptId,
	const int32 RestoreAmount,
	bool& bOutAlreadyProcessed)
{
	bOutAlreadyProcessed = false;
	if (!ReceiptId.IsValid() || RestoreAmount <= 0 || bIsDefeated)
	{
		return false;
	}
	if (ProcessedRestoreHealthReceiptIds.Contains(ReceiptId))
	{
		bOutAlreadyProcessed = true;
		return true;
	}
	// Capping remains Code A health authority. A receipt that reaches a now-full
	// living health pool is still consumed once and can be durably acknowledged.
	ApplyHealing(RestoreAmount);
	ProcessedRestoreHealthReceiptIds.Add(ReceiptId);
	return true;
}

void Udemo_mapPlayerHealthComponent::RestoreCurrentVitalityAfterItemUseRollback(
	float PreviousVitality)
{
	if (!TryCommitVitalityState(
		FMath::Clamp(PreviousVitality, 0.0f, MaximumVitality),
		MaximumVitality))
	{
		UE_LOG(Logdemo_map, Error, TEXT("0.0.10 P4.2: rejected desynchronized item rollback."));
	}
}

#if !UE_BUILD_SHIPPING
void Udemo_mapPlayerHealthComponent::SetCurrentHealthForAutomation(int32 NewHealth)
{
	TryCommitVitalityState(
		FMath::Clamp(static_cast<float>(NewHealth), 0.0f, MaximumVitality),
		MaximumVitality);
}
#endif

void Udemo_mapPlayerHealthComponent::EnterDefeatedState()
{
	if (bIsDefeated)
	{
		return;
	}

	bIsDefeated = true;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
		Character->GetCharacterMovement()->DisableMovement();
	}
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* Controller = Cast<APlayerController>(Pawn->GetController()))
		{
			Controller->SetIgnoreMoveInput(true);
			Controller->SetIgnoreLookInput(true);
		}
	}

	UE_LOG(Logdemo_map, Log, TEXT("T7: player defeated."));
	OnPlayerDefeated.Broadcast();
}

void Udemo_mapPlayerHealthComponent::ClearDamageFeedback()
{
	if (AActor* Owner = GetOwner())
	{
		if (UPointLightComponent* Flash = Owner->FindComponentByClass<UPointLightComponent>())
		{
			Flash->SetIntensity(0.0f);
		}
	}
}
