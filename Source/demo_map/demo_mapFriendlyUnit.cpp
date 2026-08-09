#include "demo_mapFriendlyUnit.h"
#include "demo_map.h"
#include "demo_mapFactionComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapFriendlyUnit::Ademo_mapFriendlyUnit()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(true);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FriendlyMesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded()) Mesh->SetStaticMesh(SphereMesh.Object);
	Mesh->SetRelativeScale3D(FVector(0.85f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCanEverAffectNavigation(false);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FriendlyLabel"));
	Label->SetupAttachment(Mesh);
	Label->SetRelativeLocation(FVector(0, 0, 115));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(40.0f);
	Label->SetTextRenderColor(FColor::Green);
	Label->SetText(FText::FromString(TEXT("ALLY\n5 / 5")));
	GreenLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FriendlyGreenLight"));
	GreenLight->SetupAttachment(Mesh);
	GreenLight->SetLightColor(FLinearColor::Green);
	GreenLight->SetIntensity(1800.0f);
	GreenLight->SetAttenuationRadius(300.0f);
	FactionComponent = CreateDefaultSubobject<Udemo_mapFactionComponent>(TEXT("Faction"));
	FactionComponent->SetFaction(Edemo_mapFaction::Friendly);
}

float Ademo_mapFriendlyUnit::TakeDamage(float DamageAmount, FDamageEvent const&, AController*, AActor*)
{
	if (DamageAmount > 0.0f) UE_LOG(Logdemo_map, Error, TEXT("V2A: ERROR friendly received forbidden damage request %.2f."), DamageAmount);
	return 0.0f;
}
