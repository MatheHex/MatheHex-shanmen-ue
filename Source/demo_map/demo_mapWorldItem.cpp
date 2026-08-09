#include "demo_mapWorldItem.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapWorldPresentation.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapWorldItem::Ademo_mapWorldItem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->InitSphereRadius(42.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.42f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Shape.Succeeded()) Mesh->SetStaticMesh(Shape.Object);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(SceneRoot);
	Label->SetRelativeLocation(FVector(0, 0, 58));
	Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Label->SetWorldSize(22.0f);
}

void Ademo_mapWorldItem::RefreshPresentation()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const Udemo_mapItemSubsystem* Items = GameInstance ? GameInstance->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr;
	const Fdemo_mapItemInstance* Instance = Items ? Items->GetAuthority().FindInstance(InstanceId) : nullptr;
	const Fdemo_mapItemDefinition* Definition = Instance ? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId) : nullptr;
	if (!Definition) { Label->SetText(FText::FromString(TEXT("INVALID ITEM"))); return; }
	Label->SetText(Fdemo_mapWorldPresentation::MakeItemWorldLabel(Definition->WorldLabelName, Instance->Quantity));
	const FLinearColor Color = Definition->CategoryId == Fdemo_mapItemIds::WeaponCategory ? FLinearColor(0.95f, 0.28f, 0.18f) :
		Definition->CategoryId == Fdemo_mapItemIds::ArmorCategory ? FLinearColor(0.20f, 0.55f, 1.0f) :
		Definition->CategoryId == Fdemo_mapItemIds::AccessoryCategory ? FLinearColor(0.65f, 0.25f, 1.0f) :
		Definition->CategoryId == Fdemo_mapItemIds::SpatialRingCategory ? FLinearColor(0.83f, 0.38f, 1.0f) :
		Definition->CategoryId == Fdemo_mapItemIds::LootCategory ? FLinearColor(1.0f, 0.75f, 0.05f) : FLinearColor(0.20f, 0.90f, 0.35f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color));
	Label->SetTextRenderColor(Color.ToFColor(true));
	SetActorTickEnabled(!Fdemo_mapWorldPresentation::FaceLabelToCamera(Label));
}

void Ademo_mapWorldItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Fdemo_mapWorldPresentation::FaceLabelToCamera(Label)) SetActorTickEnabled(false);
}

FString Ademo_mapWorldItem::GetWorldLabelText() const { return Label ? Label->Text.ToString() : FString(); }
FVector Ademo_mapWorldItem::GetWorldLabelForwardVector() const { return Label ? Label->GetForwardVector() : FVector::ZeroVector; }

bool Ademo_mapWorldItem::CanInteract(const APlayerController* Controller) const
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const Udemo_mapItemSubsystem* Items = GetGameInstance() ? GetGameInstance()->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr;
	const Fdemo_mapItemInstance* Instance = Items ? Items->GetAuthority().FindInstance(InstanceId) : nullptr;
	return Pawn != nullptr && Instance != nullptr && Instance->OwnershipState == Edemo_mapItemOwnershipState::World && Items->IsWorldActorBound(InstanceId, this) && FVector::Dist(Pawn->GetActorLocation(), GetInteractionLocation()) <= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
}

FText Ademo_mapWorldItem::GetInteractionPrompt(const APlayerController* Controller) const
{
	const Udemo_mapItemSubsystem* Items = GetGameInstance() ? GetGameInstance()->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr;
	const int32 BundleMembers = Items ? Items->GetSpatialBundleMemberCount(InstanceId) : 0;
	if (BundleMembers > 0)
	{
		return FText::FromString(FString::Printf(TEXT("[G] 原子回收空间道具包（%d件）"), BundleMembers));
	}
	const Fdemo_mapItemInstance* Instance = Items ? Items->GetAuthority().FindInstance(InstanceId) : nullptr;
	const Fdemo_mapItemDefinition* Definition = Instance ? Fdemo_mapItemDefinitions::Find(Instance->DefinitionId) : nullptr;
	return Definition ? FText::FromString(FString::Printf(TEXT("[G] 拾取：%s ×%d"), *Definition->DisplayName.ToString(), Instance->Quantity)) : FText::FromString(TEXT("[G] 无效物品"));
}

Fdemo_mapItemOperationResult Ademo_mapWorldItem::RequestInteract(APlayerController* Controller)
{
	Udemo_mapItemSubsystem* Items = GetGameInstance() ? GetGameInstance()->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr;
	return Items ? Items->PickupWorldItem(this, Controller) : Fdemo_mapItemOperationResult::Failure(Edemo_mapItemResultCode::InvalidWorldBinding, TEXT("Item subsystem is unavailable."), InstanceId);
}

FVector Ademo_mapWorldItem::GetInteractionLocation() const { return InteractionSphere->GetComponentLocation(); }
void Ademo_mapWorldItem::FocusChanged(bool bFocused) { Mesh->SetRenderCustomDepth(bFocused); }

void Ademo_mapWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Udemo_mapItemSubsystem* Items = GetGameInstance() ? GetGameInstance()->GetSubsystem<Udemo_mapItemSubsystem>() : nullptr) Items->NotifyWorldActorEndPlay(InstanceId, this);
	Super::EndPlay(EndPlayReason);
}
