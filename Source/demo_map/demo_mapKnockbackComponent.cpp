#include "demo_mapKnockbackComponent.h"
#include "demo_mapCombatDisplacement.h"
#include "demo_mapPlayerHealthComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

Udemo_mapKnockbackComponent::Udemo_mapKnockbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

Udemo_mapKnockbackComponent*
Udemo_mapKnockbackComponent::FindOrCreate(ACharacter* Character)
{
	if (!Character)
	{
		return nullptr;
	}
	if (Udemo_mapKnockbackComponent* Existing =
		Character->FindComponentByClass<Udemo_mapKnockbackComponent>())
	{
		return Existing;
	}
	Udemo_mapKnockbackComponent* Created =
		NewObject<Udemo_mapKnockbackComponent>(
			Character,
			TEXT("EnemySkillKnockback"));
	if (!Created)
	{
		return nullptr;
	}
	Character->AddInstanceComponent(Created);
	Created->RegisterComponent();
	return Created;
}

double Udemo_mapKnockbackComponent::GetNow() const
{
	return GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
}

bool Udemo_mapKnockbackComponent::IsRequestStructValid(
	bool bAlreadyActive,
	const Fdemo_mapKnockbackIntent& Intent)
{
	FVector Direction;
	return !bAlreadyActive
		&& FMath::IsFinite(Intent.Distance)
		&& FMath::IsFinite(Intent.Duration)
		&& Intent.Distance > 0.0f
		&& Intent.Duration > 0.0f
		&& Fdemo_mapCombatDisplacement::NormalizePlanarDirection(
			Intent.PlanarDirection,
			Direction);
}

bool Udemo_mapKnockbackComponent::TryStart(
	const Fdemo_mapKnockbackIntent& Intent)
{
	if (bActive)
	{
		++RejectedWhileActiveCount;
		return false;
	}
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	FVector Normalized;
	if (!Character
		|| !IsRequestStructValid(false, Intent)
		|| !Fdemo_mapCombatDisplacement::NormalizePlanarDirection(
			Intent.PlanarDirection,
			Normalized))
	{
		return false;
	}
	Direction = Normalized;
	RequestedDistance = Intent.Distance;
	Duration = Intent.Duration;
	ResolvedDistance = 0.0f;
	StartTime = GetNow();
	bActive = true;
	++AcceptedCount;
	if (UCharacterMovementComponent* Movement =
		Character->GetCharacterMovement())
	{
		SavedMovementMode = Movement->MovementMode;
		SavedCustomMovementMode = Movement->CustomMovementMode;
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	return true;
}

void Udemo_mapKnockbackComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bActive)
	{
		return;
	}
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		Cancel(false);
		return;
	}
	const float Alpha = FMath::Clamp(
		static_cast<float>(
			(GetNow() - StartTime)
			/ FMath::Max(
				static_cast<double>(Duration),
				UE_DOUBLE_SMALL_NUMBER)),
		0.0f,
		1.0f);
	const float Desired = RequestedDistance * Alpha;
	const float Step = FMath::Max(0.0f, Desired - ResolvedDistance);
	if (Step > KINDA_SMALL_NUMBER)
	{
		const Fdemo_mapCombatDisplacementResult Move =
			Fdemo_mapCombatDisplacement::MoveCharacterSwept(
				Character,
				Direction,
				Step);
		ResolvedDistance += Move.ResolvedDistance;
		if (Move.bBlocked)
		{
			Finish();
			return;
		}
	}
	if (Alpha >= 1.0f)
	{
		Finish();
	}
}

void Udemo_mapKnockbackComponent::Finish()
{
	Cancel(true);
}

void Udemo_mapKnockbackComponent::Cancel(bool bRestoreMovement)
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (bRestoreMovement && Character)
	{
		const Udemo_mapPlayerHealthComponent* Health =
			Character->FindComponentByClass<Udemo_mapPlayerHealthComponent>();
		if (!Health || !Health->IsDefeated())
		{
			if (UCharacterMovementComponent* Movement =
				Character->GetCharacterMovement())
			{
				Movement->SetMovementMode(
					SavedMovementMode,
					SavedCustomMovementMode);
				Movement->StopMovementImmediately();
			}
		}
	}
}

void Udemo_mapKnockbackComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Cancel(false);
	Super::EndPlay(EndPlayReason);
}
