#include "demo_mapShanmenSpiritShieldWorldPresentation.h"

#include "demo_mapShanmenSpiritShieldProductSession.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const FName SpiritShieldShellName(TEXT("SpiritShieldShellVisual"));
	const FName SpiritShieldCueLightName(TEXT("SpiritShieldCueLight"));
	const FVector SpiritShieldShellScale(1.35f, 1.35f, 2.10f);
	const FLinearColor SpiritShieldEnergyColor(0.08f, 0.68f, 1.0f);
	const FLinearColor SpiritShieldQuantizedCueColor(
		SpiritShieldEnergyColor.ToFColor(false));
	constexpr float SpiritShieldShellOpacity = 0.16f;
	constexpr float SpiritShieldCueIntensity = 1800.0f;
	constexpr float SpiritShieldCueAttenuationRadius = 220.0f;
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::EnsureInstalled(
	AActor* Owner,
	UStaticMesh* ShellMesh,
	UMaterialInterface* ShellMaterial)
{
	if (!IsValid(Owner) || !Owner->GetRootComponent()
		|| !ShellMesh || !ShellMaterial)
	{
		return false;
	}

	UStaticMeshComponent* Shell = FindShell(Owner);
	UPointLightComponent* CueLight = FindCueLight(Owner);
	if ((Shell == nullptr) != (CueLight == nullptr))
	{
		SetActive(Owner, false);
		return false;
	}
	if (!Shell)
	{
		Shell = NewObject<UStaticMeshComponent>(
			Owner, SpiritShieldShellName, RF_Transient);
		CueLight = NewObject<UPointLightComponent>(
			Owner, SpiritShieldCueLightName, RF_Transient);
		if (!Shell || !CueLight)
		{
			return false;
		}

		Owner->AddInstanceComponent(Shell);
		Shell->SetupAttachment(Owner->GetRootComponent());
		Shell->SetRelativeLocation(FVector::ZeroVector);
		Shell->SetRelativeRotation(FRotator::ZeroRotator);
		Shell->SetRelativeScale3D(SpiritShieldShellScale);
		Shell->SetStaticMesh(ShellMesh);
		Shell->SetMaterial(0, ShellMaterial);
		Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shell->SetCollisionResponseToAllChannels(ECR_Ignore);
		Shell->SetGenerateOverlapEvents(false);
		Shell->SetCanEverAffectNavigation(false);
		Shell->SetCastShadow(false);
		Shell->SetTranslucentSortPriority(8);
		Shell->RegisterComponent();
		UMaterialInstanceDynamic* DynamicMaterial =
			Shell->CreateDynamicMaterialInstance(0);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(
				TEXT("Color"), SpiritShieldEnergyColor);
			DynamicMaterial->SetVectorParameterValue(
				TEXT("BaseColor"), SpiritShieldEnergyColor);
			DynamicMaterial->SetVectorParameterValue(
				TEXT("Emissive"), SpiritShieldEnergyColor);
			DynamicMaterial->SetScalarParameterValue(
				TEXT("Opacity"), SpiritShieldShellOpacity);
		}

		Owner->AddInstanceComponent(CueLight);
		CueLight->SetupAttachment(Owner->GetRootComponent());
		CueLight->SetRelativeLocation(FVector::ZeroVector);
		CueLight->SetLightColor(SpiritShieldEnergyColor, false);
		CueLight->SetIntensity(SpiritShieldCueIntensity);
		CueLight->SetAttenuationRadius(
			SpiritShieldCueAttenuationRadius);
		CueLight->SetCastShadows(false);
		CueLight->RegisterComponent();
		SetActive(Owner, false);
	}

	return IsGeometryValid(Owner);
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::Synchronize(
	AActor* Owner,
	const Fdemo_mapShanmenSpiritShieldProductSession* Session,
	UStaticMesh* ShellMesh,
	UMaterialInterface* ShellMaterial)
{
	if (!EnsureInstalled(Owner, ShellMesh, ShellMaterial))
	{
		SetActive(Owner, false);
		return false;
	}
	const bool bShouldBeVisible = Session
		&& Session->IsValid()
		&& Session->IsActive()
		&& Session->GetAvailableCapacity() > 0.0f;
	SetActive(Owner, bShouldBeVisible);
	return IsVisible(Owner) == bShouldBeVisible;
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::IsVisible(
	const AActor* Owner)
{
	const UStaticMeshComponent* Shell = FindShell(Owner);
	const UPointLightComponent* CueLight = FindCueLight(Owner);
	return Shell && CueLight && Shell->IsVisible() && CueLight->IsVisible();
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::IsGeometryValid(
	const AActor* Owner)
{
	const UStaticMeshComponent* Shell = FindShell(Owner);
	const UPointLightComponent* CueLight = FindCueLight(Owner);
	return IsValid(Owner)
		&& Owner->GetRootComponent()
		&& Shell && CueLight
		&& Shell->GetOwner() == Owner
		&& CueLight->GetOwner() == Owner
		&& Shell->GetAttachParent() == Owner->GetRootComponent()
		&& CueLight->GetAttachParent() == Owner->GetRootComponent()
		&& Shell->GetStaticMesh()
		&& Shell->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !Shell->GetGenerateOverlapEvents()
		&& !Shell->CanEverAffectNavigation()
		&& Shell->GetRelativeLocation().IsNearlyZero()
		&& Shell->GetRelativeRotation().Equals(
			FRotator::ZeroRotator, KINDA_SMALL_NUMBER)
		&& Shell->GetRelativeScale3D().Equals(
			SpiritShieldShellScale, KINDA_SMALL_NUMBER)
		&& HasCanonicalMaterialColor(Owner)
		&& CueLight->GetLightColor().Equals(
			SpiritShieldQuantizedCueColor, KINDA_SMALL_NUMBER);
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::
	HasCanonicalMaterialColor(const AActor* Owner)
{
	const UStaticMeshComponent* Shell = FindShell(Owner);
	UMaterialInstanceDynamic* DynamicMaterial = Shell
		? Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0))
		: nullptr;
	return DynamicMaterial
		&& DynamicMaterial->K2_GetVectorParameterValue(TEXT("Color"))
			.Equals(SpiritShieldEnergyColor, KINDA_SMALL_NUMBER);
}

FLinearColor Fdemo_mapShanmenSpiritShieldWorldPresentation::GetCueColor(
	const AActor* Owner)
{
	const UPointLightComponent* CueLight = FindCueLight(Owner);
	return CueLight
		? CueLight->GetLightColor()
		: FLinearColor::Transparent;
}

UStaticMeshComponent*
Fdemo_mapShanmenSpiritShieldWorldPresentation::FindShell(
	const AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}
	TInlineComponentArray<UStaticMeshComponent*> Components;
	Owner->GetComponents(Components);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component && Component->GetFName() == SpiritShieldShellName)
		{
			return Component;
		}
	}
	return nullptr;
}

UPointLightComponent*
Fdemo_mapShanmenSpiritShieldWorldPresentation::FindCueLight(
	const AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}
	TInlineComponentArray<UPointLightComponent*> Components;
	Owner->GetComponents(Components);
	for (UPointLightComponent* Component : Components)
	{
		if (Component && Component->GetFName() == SpiritShieldCueLightName)
		{
			return Component;
		}
	}
	return nullptr;
}

void Fdemo_mapShanmenSpiritShieldWorldPresentation::SetActive(
	AActor* Owner,
	bool bActive)
{
	if (UStaticMeshComponent* Shell = FindShell(Owner))
	{
		Shell->SetVisibility(bActive, false);
	}
	if (UPointLightComponent* CueLight = FindCueLight(Owner))
	{
		CueLight->SetVisibility(bActive);
	}
}
