#include "demo_mapSearchContainerActor.h"
#include "demo_mapV3ProgressionManager.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapWorldPresentation.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapSearchContainerActor::Ademo_mapSearchContainerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Interaction"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(55.0f, 45.0f, 38.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Shape.Succeeded())
	{
		Mesh->SetStaticMesh(Shape.Object);
	}
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetWorldSize(24.0f);
	Label->SetTextRenderColor(FColor::Yellow);
}

bool Ademo_mapSearchContainerActor::InitializeSearchContainer(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	Edemo_mapRuntimeContainerKind InKind,
	FName InStableSourceId,
	const TArray<Fdemo_mapRuntimeContainerSeedEntry>& Seed)
{
	bCommittedRewardSourcePendingReconciliation = false;
	if (!InManager || !InItems || !InRunId.IsValid() || IsContainerInitialized())
	{
		return false;
	}
	Manager = InManager;
	Items = InItems;
	StableSourceId = InStableSourceId;
	FGuid NewContainerId;
	do
	{
		NewContainerId = FGuid::NewGuid();
	}
	while (!NewContainerId.IsValid());

	TArray<Fdemo_mapContainerMaterializationRequest> Requests;
	Requests.Reserve(Seed.Num());
	for (const Fdemo_mapRuntimeContainerSeedEntry& Entry : Seed)
	{
		Fdemo_mapContainerMaterializationRequest Request;
		Request.DefinitionId = Entry.DefinitionId;
		Request.Quantity = Entry.StackCount;
		Request.RewardEventKind = Entry.RewardEventKind;
		Request.RewardEventId = Entry.RewardEventId;
		Request.RewardValueMultiplierBps =
			Entry.RewardValueMultiplierBps;
		Request.RewardSourceRoleId = Entry.RewardSourceRoleId;
		Request.RareRewardEventId = Entry.RareRewardEventId;
		Request.RareRewardPolicyId = Entry.RareRewardPolicyId;
		Request.RareRewardTierId = Entry.RareRewardTierId;
		Request.RareRewardBonusValue = Entry.RareRewardBonusValue;
		Request.AffixSet = Entry.AffixSet;
		Requests.Add(Request);
	}
	TArray<Fdemo_mapRuntimeContainerResolvedSeedEntry> Resolved;
	TArray<FGuid> InstanceIds;
	const Fdemo_mapItemOperationResult Materialized =
		InItems->MaterializeContainerItemsAtomically(
			NewContainerId,
			Requests,
			[this, &Seed, &Resolved, NewContainerId, InRunId, InKind](
				const TArray<FGuid>& CreatedIds,
				FString& OutDiagnostic)
			{
				if (CreatedIds.Num() != Seed.Num())
				{
					OutDiagnostic =
						TEXT("Runtime Container materialization returned an incomplete item batch.");
					return false;
				}
				Resolved.Reset();
				for (int32 Index = 0; Index < Seed.Num(); ++Index)
				{
					const Fdemo_mapRuntimeContainerSeedEntry& Entry =
						Seed[Index];
					Fdemo_mapRuntimeContainerResolvedSeedEntry ResolvedEntry;
					ResolvedEntry.Section = Entry.Section;
					ResolvedEntry.SlotIndex = Entry.SlotIndex;
					ResolvedEntry.ItemInstanceId = CreatedIds[Index];
					ResolvedEntry.DefinitionId = Entry.DefinitionId;
					ResolvedEntry.StackCount = Entry.StackCount;
					ResolvedEntry.SearchDurationSeconds =
						Fdemo_mapSearchContainerPrototypeConfig::GetSearchSeconds(
							InKind,
							Entry.Section);
					Resolved.Add(ResolvedEntry);
				}
				return ContainerAuthority.Initialize(
					NewContainerId,
					InRunId,
					InKind,
					Resolved,
					OutDiagnostic,
					bPlayerDepositAllowed);
			},
			InstanceIds);
	if (!Materialized.bSuccess)
	{
		LastDiagnostic = Materialized.Diagnostic;
		return false;
	}
	RefreshContainerPresentation();
	return true;
}

bool Ademo_mapSearchContainerActor::InitializeCommittedSearchContainer(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	Edemo_mapRuntimeContainerKind InKind,
	const Fdemo_mapPersistentGeneratedRewardSource& Source)
{
	bCommittedRewardSourcePendingReconciliation = false;
	if (!InManager || !InItems || !InRunId.IsValid()
		|| IsContainerInitialized() || !Source.ContainerId.IsValid()
		|| !Source.Receipt.IsValid() || Source.Receipt.RunId != InRunId
		|| Source.Entries.Num() != Source.Receipt.PlannedStacks.Num())
	{
		LastDiagnostic = TEXT("Committed reward source identity or item projection is invalid.");
		return false;
	}
	Manager = InManager;
	Items = InItems;
	StableSourceId = Source.Receipt.StableSourceRoleId;
	TArray<Fdemo_mapContainerMaterializationRequest> Requests;
	TArray<FGuid> CommittedIds;
	TArray<Fdemo_mapRuntimeContainerResolvedSeedEntry> Resolved;
	Requests.Reserve(Source.Entries.Num());
	CommittedIds.Reserve(Source.Entries.Num());
	Resolved.Reserve(Source.Entries.Num());
	for (const Fdemo_mapPersistentGeneratedRewardSourceEntry& Entry :
		Source.Entries)
	{
		Fdemo_mapContainerMaterializationRequest Request;
		Request.DefinitionId = Entry.Item.ItemDefinitionId;
		Request.Quantity = Entry.Item.StackCount;
		Request.RewardEventKind = Entry.Item.RewardEventKind;
		Request.RewardEventId = Entry.Item.RewardEventId;
		Request.RewardValueMultiplierBps = Entry.Item.RewardValueMultiplierBps;
		Request.RewardSourceRoleId = Entry.Item.RewardSourceRoleId;
		Request.RareRewardEventId = Entry.Item.RareRewardEventId;
		Request.RareRewardPolicyId = Entry.Item.RareRewardPolicyId;
		Request.RareRewardTierId = Entry.Item.RareRewardTierId;
		Request.RareRewardBonusValue = Entry.Item.RareRewardBonusValue;
		Request.AffixSet = Entry.Item.AffixSet;
		Requests.Add(MoveTemp(Request));
		CommittedIds.Add(Entry.Item.ItemInstanceId);
		Fdemo_mapRuntimeContainerResolvedSeedEntry ResolvedEntry;
		ResolvedEntry.Section = Entry.Section;
		ResolvedEntry.SlotIndex = Entry.SlotIndex;
		ResolvedEntry.ItemInstanceId = Entry.Item.ItemInstanceId;
		ResolvedEntry.DefinitionId = Entry.Item.ItemDefinitionId;
		ResolvedEntry.StackCount = Entry.Item.StackCount;
		ResolvedEntry.SearchDurationSeconds =
			Fdemo_mapSearchContainerPrototypeConfig::GetSearchSeconds(
				InKind, Entry.Section);
		Resolved.Add(MoveTemp(ResolvedEntry));
	}
	TArray<FGuid> MaterializedIds;
	const Fdemo_mapItemOperationResult Materialized =
		InItems->MaterializeCommittedContainerItemsAtomically(
			Source.ContainerId, Requests, CommittedIds,
			[this, &Resolved, &CommittedIds, &Source, InRunId, InKind](
				const TArray<FGuid>& CreatedIds, FString& OutDiagnostic)
			{
				if (CreatedIds.Num() != Resolved.Num()
					|| CreatedIds != CommittedIds)
				{
					OutDiagnostic = TEXT("Committed reward source changed its durable item identity order.");
					return false;
				}
				return ContainerAuthority.Initialize(
					Source.ContainerId, InRunId, InKind, Resolved,
					OutDiagnostic, bPlayerDepositAllowed);
			},
			MaterializedIds);
	if (!Materialized.bSuccess)
	{
		LastDiagnostic = Materialized.Diagnostic;
		return false;
	}
	RefreshContainerPresentation();
	return true;
}

Fdemo_mapRuntimeContainerResult Ademo_mapSearchContainerActor::SubmitContainerIntent(
	const Fdemo_mapRuntimeContainerIntent& Intent,
	bool bPlayerAlive,
	bool bInRange)
{
	const Edemo_mapRuntimeContainerActionKind RequestedAction = Intent.Action;
	Fdemo_mapRuntimeContainerResult Result = ContainerAuthority.SubmitIntent(
		Intent,
		bPlayerAlive,
		bInRange,
		[this](FGuid ItemInstanceId)
		{
			return Items.IsValid()
				? Items->TransferContainerItemToInventory(
					ContainerAuthority.GetContainerId(),
					ItemInstanceId)
				: Fdemo_mapItemOperationResult::Failure(
					Edemo_mapItemResultCode::RunNotActive,
					TEXT("Item Authority is unavailable for Container Take."),
					ItemInstanceId);
		});
	LastDiagnostic = Result.Diagnostic;
	if (Result.bSuccess
		&& (RequestedAction == Edemo_mapRuntimeContainerActionKind::BeginOpen
			|| RequestedAction == Edemo_mapRuntimeContainerActionKind::BeginSearch))
	{
		ScheduleActiveAction();
	}
	else if (!ContainerAuthority.IsActionActive())
	{
		GetWorldTimerManager().ClearTimer(ActiveActionTimer);
		ActionDurationSeconds = 0.0f;
	}
	RefreshContainerPresentation();
	if (Manager.IsValid())
	{
		Manager->RefreshSearchContainerWidget();
	}
	return Result;
}

Fdemo_mapSearchContainerDropResult
Ademo_mapSearchContainerActor::SubmitPlayerDrop(
	const Fdemo_mapSearchContainerDropIntent& Intent)
{
	Fdemo_mapSearchContainerDropResult Result;
	if (!Items.IsValid())
	{
		Result.Operation = Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Item Authority is unavailable for search drag."),
			Intent.ExpectedSourceItemInstanceId);
		Result.Diagnostic = Result.Operation.Diagnostic;
		return Result;
	}
	Result = Items->ExecuteSearchContainerDrop(Intent, ContainerAuthority);
	LastDiagnostic = Result.Diagnostic;
	RefreshContainerPresentation();
	if (Manager.IsValid())
	{
		Manager->RefreshSearchContainerWidget();
	}
	return Result;
}

Fdemo_mapRuntimeContainerResult Ademo_mapSearchContainerActor::ReleaseOpeningHold()
{
	if (!ContainerAuthority.IsOpening())
	{
		return Fdemo_mapRuntimeContainerResult::Success(
			ContainerAuthority.GetContainerId(),
			ContainerAuthority.GetRevision(),
			ContainerAuthority.GetRevision());
	}
	return CancelContainerAction(TEXT("Opening cancelled because the interaction key was released."));
}

Fdemo_mapRuntimeContainerResult Ademo_mapSearchContainerActor::CancelContainerAction(
	const FString& Diagnostic)
{
	GetWorldTimerManager().ClearTimer(ActiveActionTimer);
	ActionDurationSeconds = 0.0f;
	Fdemo_mapRuntimeContainerResult Result =
		ContainerAuthority.CancelActiveAction(Diagnostic);
	LastDiagnostic = Result.Diagnostic;
	RefreshContainerPresentation();
	if (Manager.IsValid())
	{
		Manager->RefreshSearchContainerWidget();
	}
	return Result;
}

void Ademo_mapSearchContainerActor::ScheduleActiveAction()
{
	GetWorldTimerManager().ClearTimer(ActiveActionTimer);
	SetActorTickEnabled(true);
	ActionDurationSeconds = ContainerAuthority.GetActiveActionDuration();
	ActionStartSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (ActionDurationSeconds <= 0.0f)
	{
		CompleteTimedAction();
		return;
	}
	GetWorldTimerManager().SetTimer(
		ActiveActionTimer,
		this,
		&Ademo_mapSearchContainerActor::CompleteTimedAction,
		ActionDurationSeconds,
		false);
}

void Ademo_mapSearchContainerActor::CompleteTimedAction()
{
	const bool bWasOpening = ContainerAuthority.IsOpening();
	const Fdemo_mapRuntimeContainerResult Result =
		ContainerAuthority.CompleteActiveAction();
	ActionDurationSeconds = 0.0f;
	LastDiagnostic = Result.Diagnostic;
	RefreshContainerPresentation();
	if (!Result.bSuccess || !Manager.IsValid())
	{
		return;
	}
	if (bWasOpening)
	{
		Manager->OpenSearchContainer(this);
	}
	else
	{
		Manager->RefreshSearchContainerWidget();
	}
}

#if !UE_BUILD_SHIPPING
Fdemo_mapRuntimeContainerResult Ademo_mapSearchContainerActor::CompleteActionForAutomation()
{
	GetWorldTimerManager().ClearTimer(ActiveActionTimer);
	const bool bWasOpening = ContainerAuthority.IsOpening();
	const Fdemo_mapRuntimeContainerResult Result =
		ContainerAuthority.CompleteActiveAction();
	ActionDurationSeconds = 0.0f;
	RefreshContainerPresentation();
	if (Result.bSuccess && Manager.IsValid() && bWasOpening)
	{
		Manager->OpenSearchContainer(this);
	}
	else if (Manager.IsValid())
	{
		Manager->RefreshSearchContainerWidget();
	}
	return Result;
}
#endif

Fdemo_mapRuntimeContainerSnapshot Ademo_mapSearchContainerActor::GetContainerSnapshot() const
{
	const Fdemo_mapItemAuthority* Authority =
		Items.IsValid() ? &Items->GetAuthority() : nullptr;
	if (!Authority)
	{
		return Fdemo_mapRuntimeContainerSnapshot();
	}
	Fdemo_mapRuntimeContainerSnapshot Snapshot =
		ContainerAuthority.BuildSnapshot(
		*Authority,
		GetCurrentActionProgress01(),
		Authority->GetUsedInventorySlots(),
		Authority->GetInventoryCapacity(),
		LastDiagnostic);
	Snapshot.SourceDisplayLabel = DisplayLabel;
	return Snapshot;
}

float Ademo_mapSearchContainerActor::GetCurrentActionProgress01() const
{
	if (!ContainerAuthority.IsActionActive())
	{
		return ContainerAuthority.GetState() == Edemo_mapRuntimeContainerState::Opened
			? 1.0f
			: 0.0f;
	}
	if (ActionDurationSeconds <= 0.0f || !GetWorld())
	{
		return 0.0f;
	}
	return FMath::Clamp(
		static_cast<float>((GetWorld()->GetTimeSeconds() - ActionStartSeconds) / ActionDurationSeconds),
		0.0f,
		1.0f);
}

bool Ademo_mapSearchContainerActor::IsPlayerInRange(
	const APlayerController* Controller) const
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn
		&& FVector::Dist(
			Pawn->GetActorLocation(),
			GetInteractionLocation())
			<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
}

bool Ademo_mapSearchContainerActor::IsPlayerAlive(
	const APlayerController* Controller) const
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const Udemo_mapPlayerHealthComponent* Health =
		Pawn ? Pawn->FindComponentByClass<Udemo_mapPlayerHealthComponent>() : nullptr;
	return Pawn && (!Health || !Health->IsDefeated());
}

bool Ademo_mapSearchContainerActor::CanInteract(
	const APlayerController* Controller) const
{
	return IsContainerInitialized()
		&& IsPlayerAlive(Controller)
		&& IsPlayerInRange(Controller)
		&& !ContainerAuthority.IsSearching();
}

FText Ademo_mapSearchContainerActor::GetInteractionPrompt(
	const APlayerController*) const
{
	switch (ContainerAuthority.GetState())
	{
	case Edemo_mapRuntimeContainerState::Closed:
		// Opening is committed by a single G press.  Keep the prompt aligned with
		// the actual input contract so players are not told to hold a key that the
		// interaction system intentionally releases immediately.
		return FText::FromString(TEXT("[G] 开启 / OPEN"));
	case Edemo_mapRuntimeContainerState::Opening:
		return FText::FromString(
			FString::Printf(
				TEXT("开启中 %.0f%%"),
				GetCurrentActionProgress01() * 100.0f));
	case Edemo_mapRuntimeContainerState::Opened:
		return FText::FromString(
			IsContainerEmpty()
				? TEXT("[G] 已取空 / EMPTY")
				: TEXT("[G] 查看容器 / OPEN"));
	default:
		return FText::FromString(TEXT("容器不可用"));
	}
}

Fdemo_mapItemOperationResult Ademo_mapSearchContainerActor::RequestInteract(
	APlayerController* Controller)
{
	if (!CanInteract(Controller))
	{
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionOutOfRange,
			TEXT("Runtime Container interaction is unavailable or out of range."));
	}
	if (IsContainerOpened())
	{
		if (Manager.IsValid())
		{
			Manager->OpenSearchContainer(this);
			return Fdemo_mapItemOperationResult::Success();
		}
		return Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			TEXT("Runtime Container manager is unavailable."));
	}
	Fdemo_mapRuntimeContainerIntent Intent;
	Intent.ExpectedRunId = ContainerAuthority.GetOwningRunId();
	Intent.ContainerId = ContainerAuthority.GetContainerId();
	Intent.ExpectedRevision = ContainerAuthority.GetRevision();
	Intent.Action = Edemo_mapRuntimeContainerActionKind::BeginOpen;
	const Fdemo_mapRuntimeContainerResult Result = SubmitContainerIntent(
		Intent,
		IsPlayerAlive(Controller),
		IsPlayerInRange(Controller));
	return Result.bSuccess
		? Fdemo_mapItemOperationResult::Success()
		: Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			Result.Diagnostic);
}

FVector Ademo_mapSearchContainerActor::GetInteractionLocation() const
{
	return InteractionBox->GetComponentLocation();
}

void Ademo_mapSearchContainerActor::FocusChanged(bool bFocused)
{
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(bFocused);
	}
}

void Ademo_mapSearchContainerActor::SetContainerDisplayLabel(const FString& InLabel)
{
	DisplayLabel = InLabel;
	RefreshContainerPresentation();
}

void Ademo_mapSearchContainerActor::SetContainerMeshColor(const FLinearColor& Color)
{
	if (Mesh)
	{
		if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		}
	}
}

void Ademo_mapSearchContainerActor::RefreshContainerPresentation()
{
	if (!Mesh || !Label)
	{
		return;
	}
	FString StateText = TEXT("CLOSED");
	FColor StateColor = FColor::Yellow;
	switch (ContainerAuthority.GetState())
	{
	case Edemo_mapRuntimeContainerState::Opening:
		StateText = FString::Printf(
			TEXT("OPENING %.0f%%"),
			GetCurrentActionProgress01() * 100.0f);
		StateColor = FColor(255, 190, 60);
		break;
	case Edemo_mapRuntimeContainerState::Opened:
		StateText = IsContainerEmpty() ? TEXT("EMPTY") : TEXT("OPENED");
		StateColor = FColor(130, 230, 150);
		break;
	default:
		break;
	}
	Label->SetText(FText::FromString(
		FString::Printf(TEXT("%s - %s"), *DisplayLabel, *StateText)));
	Label->SetTextRenderColor(StateColor);
	Mesh->SetRelativeScale3D(
		ContainerAuthority.GetState() == Edemo_mapRuntimeContainerState::Opened
			? FVector(1.0f, 0.75f, 0.30f)
			: FVector(1.0f, 0.75f, 0.55f));
}

void Ademo_mapSearchContainerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ContainerAuthority.IsActionActive())
	{
		RefreshContainerPresentation();
		if (Manager.IsValid())
		{
			Manager->RefreshSearchContainerWidget();
		}
	}
	if (Fdemo_mapWorldPresentation::FaceLabelToCamera(Label)
		&& !ContainerAuthority.IsActionActive())
	{
		SetActorTickEnabled(false);
	}
}

FString Ademo_mapSearchContainerActor::GetWorldLabelText() const
{
	return Label ? Label->Text.ToString() : FString();
}

bool Ademo_mapSearchContainerActor::HasActiveTimer() const
{
	return GetWorld() && GetWorldTimerManager().IsTimerActive(ActiveActionTimer);
}

void Ademo_mapSearchContainerActor::CleanupUnclaimedItems()
{
	if (bCleanupComplete)
	{
		return;
	}
	bCleanupComplete = true;
	GetWorldTimerManager().ClearTimer(ActiveActionTimer);
	ContainerAuthority.CleanupUnclaimed(
		[this](FGuid ItemInstanceId)
		{
			return Items.IsValid()
				? Items->DestroyContainerItem(
					ContainerAuthority.GetContainerId(),
					ItemInstanceId)
				: Fdemo_mapItemOperationResult::Failure(
					Edemo_mapItemResultCode::InstanceNotFound,
					TEXT("Runtime Container Item Authority is already unavailable."),
					ItemInstanceId);
		});
}

void Ademo_mapSearchContainerActor::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ActiveActionTimer);
	if (Manager.IsValid())
	{
		Manager->NotifySearchContainerEndPlay(this);
	}
	CleanupUnclaimedItems();
	Super::EndPlay(EndPlayReason);
}
