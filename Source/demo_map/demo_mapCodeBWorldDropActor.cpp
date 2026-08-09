#include "demo_mapCodeBWorldDropActor.h"

#include "demo_mapV3ProgressionManager.h"

#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"

Ademo_mapCodeBWorldDropActor::Ademo_mapCodeBWorldDropActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(42.0f, 42.0f, 24.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetWorldSize(20.0f);
	Label->SetText(FText::FromString(TEXT("GROUND DROP")));
	Label->SetTextRenderColor(FColor(235, 178, 86));
}

void Ademo_mapCodeBWorldDropActor::ConfigureCodeBWorldDrop(const FCodeBWorldDropProjection& Projection)
{
	OwnerId = Projection.OwnerId;
	RunInstanceId = Projection.RunInstanceId;
	WorldDropId = Projection.WorldDropId;
	SetActorTransform(Projection.FloorTransform);
}

void Ademo_mapCodeBWorldDropActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Ademo_mapV3ProgressionManager* Manager = ResolveManager())
	{
		Manager->NotifyCodeBWorldDropActorEndPlay(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool Ademo_mapCodeBWorldDropActor::CanInteract(const APlayerController* Controller) const
{
	return Controller && OwnerId.IsValid() && RunInstanceId.IsValid() && WorldDropId.IsValid()
		&& ResolveManager() != nullptr;
}

FText Ademo_mapCodeBWorldDropActor::GetInteractionPrompt(const APlayerController* Controller) const
{
	return FText::FromString(TEXT("[G] 查看地面物品"));
}

Fdemo_mapItemOperationResult Ademo_mapCodeBWorldDropActor::RequestInteract(APlayerController* Controller)
{
	if (Ademo_mapV3ProgressionManager* Manager = ResolveManager())
	{
		return Manager->RequestCodeBWorldDropInteract(this);
	}
	return Fdemo_mapItemOperationResult::Failure(
		Edemo_mapItemResultCode::InteractionBlocked, TEXT("Code B world-drop manager is unavailable."));
}

void Ademo_mapCodeBWorldDropActor::FocusChanged(const bool bFocused)
{
	if (Label) Label->SetTextRenderColor(bFocused ? FColor(255, 236, 110) : FColor(235, 178, 86));
}

Ademo_mapV3ProgressionManager* Ademo_mapCodeBWorldDropActor::ResolveManager() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<Ademo_mapV3ProgressionManager> It(World); It; ++It) return *It;
	}
	return nullptr;
}
