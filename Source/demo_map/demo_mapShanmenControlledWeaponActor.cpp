#include "demo_mapShanmenControlledWeaponActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

Ademo_mapShanmenControlledWeaponActor::
Ademo_mapShanmenControlledWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<UBoxComponent>(
		TEXT("ControlledWeaponCollision"));
	SetRootComponent(Collision);
	Collision->InitBoxExtent(FVector(40.0f, 4.0f, 2.0f));
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetNotifyRigidBodyCollision(true);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ControlledWeaponVisual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(0.8f, 0.08f, 0.04f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Visual->SetStaticMesh(CubeMesh.Object);
	}
}

bool Ademo_mapShanmenControlledWeaponActor::TryBindProductIdentity(
	const FGuid& InRunId,
	const FGuid& InItemInstanceId,
	AActor* InSourceActor)
{
	if (IsProductBound())
	{
		return IsProductBoundTo(
			InRunId, InItemInstanceId, InSourceActor);
	}
	if (RunId.IsValid()
		|| ItemInstanceId.IsValid()
		|| SourceActor
		|| !InRunId.IsValid()
		|| !InItemInstanceId.IsValid()
		|| !::IsValid(InSourceActor)
		|| InSourceActor == this
		|| !Collision
		|| GetRootComponent() != Collision)
	{
		return false;
	}

	RunId = InRunId;
	ItemInstanceId = InItemInstanceId;
	SourceActor = InSourceActor;
	SetOwner(InSourceActor);
	if (APawn* SourcePawn = Cast<APawn>(InSourceActor))
	{
		SetInstigator(SourcePawn);
	}
	Collision->IgnoreActorWhenMoving(InSourceActor, true);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return IsProductBoundTo(
		InRunId, InItemInstanceId, InSourceActor);
}

bool Ademo_mapShanmenControlledWeaponActor::IsProductBound() const
{
	return IsProductBoundTo(RunId, ItemInstanceId, SourceActor.Get());
}

bool Ademo_mapShanmenControlledWeaponActor::IsProductBoundTo(
	const FGuid& ExpectedRunId,
	const FGuid& ExpectedItemInstanceId,
	const AActor* ExpectedSourceActor) const
{
	return ExpectedRunId.IsValid()
		&& ExpectedItemInstanceId.IsValid()
		&& ::IsValid(ExpectedSourceActor)
		&& ExpectedSourceActor != this
		&& RunId == ExpectedRunId
		&& ItemInstanceId == ExpectedItemInstanceId
		&& SourceActor == ExpectedSourceActor
		&& GetOwner() == ExpectedSourceActor
		&& Collision
		&& Collision->GetOwner() == this
		&& GetRootComponent() == Collision;
}

void Ademo_mapShanmenControlledWeaponActor::ActivateProductCollision()
{
	check(IsProductBound());
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void Ademo_mapShanmenControlledWeaponActor::DeactivateProductCollision()
{
	if (Collision)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
