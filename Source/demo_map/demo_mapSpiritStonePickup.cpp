#include "demo_mapSpiritStonePickup.h"

#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapSpiritStoneTypes.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

Ademo_mapSpiritStonePickup::Ademo_mapSpiritStonePickup()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetSphereRadius(46.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0, 0, 82));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetText(FText::FromString(TEXT("灵石 +20")));
	Label->SetTextRenderColor(FColor(70, 230, 255));
	Label->SetWorldSize(32.0f);
}

bool Ademo_mapSpiritStonePickup::CanInteract(const APlayerController* Controller) const
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return !bCommitted && Pawn
		&& FVector::Dist(Pawn->GetActorLocation(), GetInteractionLocation()) <= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
}

FText Ademo_mapSpiritStonePickup::GetInteractionPrompt(const APlayerController* Controller) const
{
	return CanInteract(Controller)
		? FText::FromString(FString::Printf(TEXT("交互：拾取灵石 +%lld（风险）"), Value))
		: FText::GetEmpty();
}

Fdemo_mapItemOperationResult Ademo_mapSpiritStonePickup::RequestInteract(APlayerController* Controller)
{
	if (!CanInteract(Controller))
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InteractionOutOfRange, TEXT("Fixed Spirit Stone is unavailable or out of range."));
	UGameInstance* Instance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	Udemo_mapProfileSessionSubsystem* Session = Instance ? Instance->GetSubsystem<Udemo_mapProfileSessionSubsystem>() : nullptr;
	if (!Session)
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::RunNotActive, TEXT("Profile session is unavailable."));
	if (ExpectedRunId.IsValid() && Session->GetSnapshot().ActiveRunId != ExpectedRunId)
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Spirit Stone Pickup belongs to a different Run and remains unconsumed."));
	}
	const Fdemo_mapSpiritStonePickupResult Result = Session->CollectSpiritStone(PickupId, SourceId, Value);
	if (!Result.IsCommitted())
		return Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvariantViolation, Result.Diagnostic);
	bCommitted = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	Destroy();
	return Fdemo_mapItemOperationResult::Success(FGuid(), NAME_None, NAME_None, GetSourceId());
}

FVector Ademo_mapSpiritStonePickup::GetInteractionLocation() const
{
	return GetActorLocation();
}

void Ademo_mapSpiritStonePickup::FocusChanged(bool bFocused)
{
	if (Label) Label->SetTextRenderColor(bFocused ? FColor::Yellow : FColor(70, 230, 255));
}

FName Ademo_mapSpiritStonePickup::GetPickupId() const { return PickupId; }
FName Ademo_mapSpiritStonePickup::GetSourceId() const { return SourceId; }
int64 Ademo_mapSpiritStonePickup::GetValue() const { return Value; }

bool Ademo_mapSpiritStonePickup::InitializeRuntimePickup(
	FName InPickupId,
	FName InSourceId,
	int64 InValue,
	FGuid InExpectedRunId)
{
	if (bCommitted || InPickupId.IsNone() || InSourceId.IsNone()
		|| InValue < 15 || InValue > 120 || !InExpectedRunId.IsValid())
	{
		return false;
	}
	PickupId = InPickupId;
	SourceId = InSourceId;
	Value = InValue;
	ExpectedRunId = InExpectedRunId;
	if (Label)
	{
		Label->SetText(FText::FromString(FString::Printf(TEXT("灵石 +%lld"), Value)));
	}
	return true;
}
