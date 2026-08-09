#include "demo_mapCodeBNormalContainerActor.h"

#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"
#include "demo_mapV3ProgressionManager.h"

#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

Ademo_mapCodeBNormalContainerActor::Ademo_mapCodeBNormalContainerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(58.0f, 58.0f, 46.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetWorldSize(24.0f);
	Label->SetText(FText::FromString(TEXT("BASIC CACHE")));
	Label->SetTextRenderColor(FColor(92, 205, 255));
}

void Ademo_mapCodeBNormalContainerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (PendingActionId.IsValid())
	{
		if (Ademo_mapV3ProgressionManager* Manager = ResolveManager())
		{
			if (!Manager->IsCodeBNormalContainerActionStillValid(this, PendingActionId))
			{
				Manager->InterruptCodeBNormalContainerAction(this, TEXT("DistanceLostOrRunInvalid"));
				ClearPendingAction();
			}
		}
		else
		{
			ClearPendingAction();
		}
	}
}

void Ademo_mapCodeBNormalContainerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Ademo_mapV3ProgressionManager* Manager = ResolveManager())
	{
		// The manager also closes an already-open page; destruction must not leave
		// a transferable target projection behind just because no timer is pending.
		Manager->InterruptCodeBNormalContainerAction(this, TEXT("ActorDestroyed"));
	}
	ClearPendingAction();
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapCodeBNormalContainerActor::ScheduleOpenCompletion(const FGuid& ActionId, const float DurationSeconds)
{
	PendingActionId = ActionId;
	bPendingActionIsSearch = false;
	GetWorldTimerManager().SetTimer(PendingActionTimer, this, &Ademo_mapCodeBNormalContainerActor::CompletePendingAction, DurationSeconds, false);
}

void Ademo_mapCodeBNormalContainerActor::ScheduleSearchCompletion(const FGuid& ActionId, const float DurationSeconds)
{
	PendingActionId = ActionId;
	bPendingActionIsSearch = true;
	GetWorldTimerManager().SetTimer(PendingActionTimer, this, &Ademo_mapCodeBNormalContainerActor::CompletePendingAction, DurationSeconds, false);
}

void Ademo_mapCodeBNormalContainerActor::ClearPendingAction()
{
	GetWorldTimerManager().ClearTimer(PendingActionTimer);
	PendingActionId.Invalidate();
	bPendingActionIsSearch = false;
}

bool Ademo_mapCodeBNormalContainerActor::CanInteract(const APlayerController* Controller) const
{
	return Controller != nullptr && ResolveManager() != nullptr && !MapTargetIdentity.IsNone();
}

FText Ademo_mapCodeBNormalContainerActor::GetInteractionPrompt(const APlayerController* Controller) const
{
	return FText::FromString(TEXT("[G] 打开普通储物箱"));
}

Fdemo_mapItemOperationResult Ademo_mapCodeBNormalContainerActor::RequestInteract(APlayerController* Controller)
{
	if (Ademo_mapV3ProgressionManager* Manager = ResolveManager())
	{
		return Manager->RequestCodeBNormalContainerInteract(this);
	}
	return Fdemo_mapItemOperationResult::Failure(
		Edemo_mapItemResultCode::InteractionBlocked,
		TEXT("Code B normal-container manager is unavailable."));
}

FVector Ademo_mapCodeBNormalContainerActor::GetInteractionLocation() const
{
	return GetActorLocation();
}

void Ademo_mapCodeBNormalContainerActor::FocusChanged(const bool bFocused)
{
	if (Label)
	{
		Label->SetTextRenderColor(bFocused ? FColor(255, 236, 110) : FColor(92, 205, 255));
	}
}

Ademo_mapV3ProgressionManager* Ademo_mapCodeBNormalContainerActor::ResolveManager() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<Ademo_mapV3ProgressionManager> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void Ademo_mapCodeBNormalContainerActor::CompletePendingAction()
{
	const FGuid ActionId = PendingActionId;
	const bool bSearch = bPendingActionIsSearch;
	ClearPendingAction();
	if (Ademo_mapV3ProgressionManager* Manager = ResolveManager())
	{
		Manager->CompleteCodeBNormalContainerAction(this, ActionId, bSearch);
	}
}
