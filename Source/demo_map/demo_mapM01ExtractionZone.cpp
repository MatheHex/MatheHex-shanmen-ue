#include "demo_mapM01ExtractionZone.h"
#include "demo_mapGameMode.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapM01ExtractionZone::Ademo_mapM01ExtractionZone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->InitSphereRadius(150.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	MarkerMesh->SetupAttachment(SceneRoot);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetRelativeScale3D(FVector(1.8f, 1.8f, 0.12f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Shape.Succeeded()) MarkerMesh->SetStaticMesh(Shape.Object);
	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Status"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
	StatusText->SetHorizontalAlignment(EHTA_Center);
	StatusText->SetWorldSize(26.0f);
}

void Ademo_mapM01ExtractionZone::Configure(Edemo_mapM01ExitType InExitType)
{
	ExitType = InExitType;
	bAuthoredForM01 = false;
	SetProjectionActive(true);
	RefreshPresentation();
}

void Ademo_mapM01ExtractionZone::ConfigureAuthored(Edemo_mapM01ExitType InExitType)
{
	ExitType = InExitType;
	bAuthoredForM01 = true;
	Tags.AddUnique(TEXT("M01_GENERATED"));
	Tags.AddUnique(Fdemo_mapM01ExtractionAuthority::ExitId(ExitType));
	RefreshPresentation();
}

void Ademo_mapM01ExtractionZone::ConfigureAuthoredRegular() { ConfigureAuthored(Edemo_mapM01ExitType::Regular); }
void Ademo_mapM01ExtractionZone::ConfigureAuthoredBoss() { ConfigureAuthored(Edemo_mapM01ExitType::Boss); }
void Ademo_mapM01ExtractionZone::ConfigureAuthoredDiscardSpatial() { ConfigureAuthored(Edemo_mapM01ExitType::DiscardSpatial); }

void Ademo_mapM01ExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	SetProjectionActive(!bAuthoredForM01);
}

void Ademo_mapM01ExtractionZone::SetProjectionActive(bool bActive)
{
	bProjectionActive = bActive;
	SetActorHiddenInGame(!bActive);
	SetActorTickEnabled(bActive);
	if (InteractionSphere)
	{
		InteractionSphere->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (!bActive && bLastInRange)
	{
		if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			Mode->SetM01ExtractionInRange(ExitType, false);
		}
		bLastInRange = false;
	}
}

void Ademo_mapM01ExtractionZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bProjectionActive) return;
	Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const bool bInRange = Pawn
		&& FVector::Dist(Pawn->GetActorLocation(), GetInteractionLocation())
			<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
	if (Mode && bInRange != bLastInRange)
	{
		bLastInRange = bInRange;
		Mode->SetM01ExtractionInRange(ExitType, bInRange);
	}
	RefreshPresentation();
}

void Ademo_mapM01ExtractionZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bLastInRange)
	{
		if (Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
		{
			Mode->SetM01ExtractionInRange(ExitType, false);
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool Ademo_mapM01ExtractionZone::CanInteract(const APlayerController* Controller) const
{
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	const Fdemo_mapM01ExtractionSnapshot Snapshot = Mode
		? Mode->GetM01ExtractionSnapshot(ExitType)
		: Fdemo_mapM01ExtractionSnapshot();
	return bProjectionActive && Pawn && Mode && Mode->IsM01ExtractionFoundationActive()
		&& Snapshot.State != Edemo_mapM01ExtractionState::Unavailable
		&& Snapshot.State != Edemo_mapM01ExtractionState::Completed
		&& FVector::Dist(Pawn->GetActorLocation(), GetInteractionLocation())
			<= Fdemo_mapWorldInteractionRules::InteractionRangeUU;
}

FText Ademo_mapM01ExtractionZone::GetInteractionPrompt(const APlayerController* Controller) const
{
	const Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	const Fdemo_mapM01ExtractionSnapshot Snapshot = Mode
		? Mode->GetM01ExtractionSnapshot(ExitType)
		: Fdemo_mapM01ExtractionSnapshot();
	const FString Key = Fdemo_mapInputBindingSettings::Get()
		.GetKey(Fdemo_mapInputActionIds::Interact).GetDisplayName().ToString();
	if (Snapshot.State == Edemo_mapM01ExtractionState::CountingDown)
	{
		return FText::FromString(FString::Printf(TEXT("%s · %.1f 秒（离区/受伤将取消）"),
			*Fdemo_mapM01ExtractionAuthority::ExitLabel(ExitType), Snapshot.RemainingSeconds));
	}
	if (!Snapshot.bConditionSatisfied)
	{
		return FText::FromString(ExitType == Edemo_mapM01ExitType::Boss
			? TEXT("Boss撤离锁定 · 击败本局 M01 主 Boss")
			: FString::Printf(TEXT("[%s] 弃置空间道具包并开始撤离"), *Key));
	}
	return FText::FromString(FString::Printf(TEXT("[%s] 开始%s（3秒）"),
		*Key, *Fdemo_mapM01ExtractionAuthority::ExitLabel(ExitType)));
}

Fdemo_mapItemOperationResult Ademo_mapM01ExtractionZone::RequestInteract(APlayerController* Controller)
{
	Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (Mode) Mode->SetM01ExtractionInRange(ExitType, true);
	return Mode
		? Mode->RequestM01Extraction(ExitType, Controller)
		: Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::InteractionBlocked,
			TEXT("M01 extraction authority is unavailable."));
}

FVector Ademo_mapM01ExtractionZone::GetInteractionLocation() const
{
	return InteractionSphere->GetComponentLocation();
}

void Ademo_mapM01ExtractionZone::FocusChanged(bool bFocused)
{
	MarkerMesh->SetRenderCustomDepth(bFocused);
}

void Ademo_mapM01ExtractionZone::RefreshPresentation()
{
	const Ademo_mapGameMode* Mode = GetWorld() ? Cast<Ademo_mapGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (!Mode || !StatusText) return;
	const Fdemo_mapM01ExtractionSnapshot Snapshot = Mode->GetM01ExtractionSnapshot(ExitType);
	const FLinearColor ExitColor = ExitType == Edemo_mapM01ExitType::Regular
		? FLinearColor(0.08f, 0.90f, 0.42f)
		: (ExitType == Edemo_mapM01ExitType::DiscardSpatial
			? FLinearColor(1.0f, 0.46f, 0.05f)
			: FLinearColor(0.85f, 0.08f, 1.0f));
	const FLinearColor Color = Snapshot.State == Edemo_mapM01ExtractionState::CountingDown
		? FLinearColor(1.0f, 0.75f, 0.08f)
		: (Snapshot.bConditionSatisfied ? ExitColor : ExitColor * 0.30f);
	StatusText->SetText(FText::FromString(Mode->GetM01ExtractionStatus(ExitType)));
	StatusText->SetTextRenderColor(Color.ToFColor(true));
	MarkerMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color));
	MarkerMesh->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), FVector(Color));
	MarkerMesh->SetRelativeScale3D(ExitType == Edemo_mapM01ExitType::DiscardSpatial
		? FVector(2.35f, 1.45f, 0.12f)
		: (ExitType == Edemo_mapM01ExitType::Boss
			? FVector(2.25f, 2.25f, 0.18f)
			: FVector(1.80f, 1.80f, 0.12f)));
}
