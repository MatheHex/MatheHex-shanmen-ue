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
	const FVector SpiritShieldFullScale(1.35f, 1.35f, 2.10f);
	const FVector SpiritShieldMinimumScale(1.20f, 1.20f, 1.90f);
	const FLinearColor SpiritShieldStableColor(0.08f, 0.68f, 1.0f);
	const FLinearColor SpiritShieldLowCapacityColor(1.0f, 0.46f, 0.05f);
	constexpr float SpiritShieldShellOpacity = 0.16f;
	constexpr float SpiritShieldStableCueIntensity = 1800.0f;
	constexpr float SpiritShieldLowCapacityCueIntensity = 2600.0f;
	constexpr float SpiritShieldCueAttenuationRadius = 220.0f;
	constexpr float SpiritShieldLowCapacityFraction = 0.25f;

	FVector CalculateCapacityScale(
		float AvailableCapacity,
		float MaximumCapacity)
	{
		const float CapacityFraction = FMath::Clamp(
			AvailableCapacity / MaximumCapacity, 0.0f, 1.0f);
		return FMath::Lerp(
			SpiritShieldMinimumScale,
			SpiritShieldFullScale,
			CapacityFraction);
	}

	bool IsBoundedShellScale(const FVector& Scale)
	{
		return FMath::IsFinite(Scale.X)
			&& FMath::IsFinite(Scale.Y)
			&& FMath::IsFinite(Scale.Z)
			&& Scale.X >= SpiritShieldMinimumScale.X - KINDA_SMALL_NUMBER
			&& Scale.Y >= SpiritShieldMinimumScale.Y - KINDA_SMALL_NUMBER
			&& Scale.Z >= SpiritShieldMinimumScale.Z - KINDA_SMALL_NUMBER
			&& Scale.X <= SpiritShieldFullScale.X + KINDA_SMALL_NUMBER
			&& Scale.Y <= SpiritShieldFullScale.Y + KINDA_SMALL_NUMBER
			&& Scale.Z <= SpiritShieldFullScale.Z + KINDA_SMALL_NUMBER;
	}
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
		Shell->SetRelativeScale3D(SpiritShieldFullScale);
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
				TEXT("Color"), SpiritShieldStableColor);
			DynamicMaterial->SetVectorParameterValue(
				TEXT("BaseColor"), SpiritShieldStableColor);
			DynamicMaterial->SetVectorParameterValue(
				TEXT("Emissive"), SpiritShieldStableColor);
			DynamicMaterial->SetScalarParameterValue(
				TEXT("Opacity"), SpiritShieldShellOpacity);
		}

		Owner->AddInstanceComponent(CueLight);
		CueLight->SetupAttachment(Owner->GetRootComponent());
		CueLight->SetRelativeLocation(FVector::ZeroVector);
		CueLight->SetLightColor(SpiritShieldStableColor, false);
		CueLight->SetIntensity(SpiritShieldStableCueIntensity);
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
	const float AvailableCapacity = Session
		? Session->GetAvailableCapacity()
		: 0.0f;
	const bool bShouldBeVisible = Session
		&& Session->IsValid()
		&& Session->IsActive()
		&& AvailableCapacity > 0.0f;
	bool bLowCapacity = false;
	float MaximumCapacity = 0.0f;
	if (bShouldBeVisible)
	{
		MaximumCapacity = Session->GetSession()
			.GetCapacityAuthority().GetMaximumCapacity();
		if (!FMath::IsFinite(MaximumCapacity) || MaximumCapacity <= 0.0f
			|| !FMath::IsFinite(AvailableCapacity)
			|| AvailableCapacity > MaximumCapacity)
		{
			SetShellScale(Owner, SpiritShieldFullScale);
			SetAppearance(
				Owner,
				SpiritShieldStableColor,
				SpiritShieldStableCueIntensity);
			SetActive(Owner, false);
			return false;
		}
		bLowCapacity = AvailableCapacity
			<= MaximumCapacity * SpiritShieldLowCapacityFraction;
	}
	SetShellScale(
		Owner,
		bShouldBeVisible
			? CalculateCapacityScale(AvailableCapacity, MaximumCapacity)
			: SpiritShieldFullScale);
	SetAppearance(
		Owner,
		bLowCapacity
			? SpiritShieldLowCapacityColor
			: SpiritShieldStableColor,
		bLowCapacity
			? SpiritShieldLowCapacityCueIntensity
			: SpiritShieldStableCueIntensity);
	SetActive(Owner, bShouldBeVisible);
	return IsVisible(Owner) == bShouldBeVisible
		&& (!bShouldBeVisible
			|| ((bLowCapacity
					? HasLowCapacityAppearance(Owner)
					: HasStableAppearance(Owner))
				&& HasCapacityScale(
					Owner, AvailableCapacity, MaximumCapacity)));
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
		&& IsBoundedShellScale(Shell->GetRelativeScale3D())
		&& Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0));
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
			.Equals(SpiritShieldStableColor, KINDA_SMALL_NUMBER);
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::HasStableAppearance(
	const AActor* Owner)
{
	return HasAppearance(
		Owner, SpiritShieldStableColor, SpiritShieldStableCueIntensity);
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::
	HasLowCapacityAppearance(const AActor* Owner)
{
	return HasAppearance(
		Owner,
		SpiritShieldLowCapacityColor,
		SpiritShieldLowCapacityCueIntensity);
}

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::HasCapacityScale(
	const AActor* Owner,
	float AvailableCapacity,
	float MaximumCapacity)
{
	const UStaticMeshComponent* Shell = FindShell(Owner);
	return Shell
		&& FMath::IsFinite(AvailableCapacity)
		&& FMath::IsFinite(MaximumCapacity)
		&& AvailableCapacity > 0.0f
		&& MaximumCapacity > 0.0f
		&& AvailableCapacity <= MaximumCapacity
		&& Shell->GetRelativeScale3D().Equals(
			CalculateCapacityScale(AvailableCapacity, MaximumCapacity),
			KINDA_SMALL_NUMBER);
}

FLinearColor Fdemo_mapShanmenSpiritShieldWorldPresentation::GetCueColor(
	const AActor* Owner)
{
	const UPointLightComponent* CueLight = FindCueLight(Owner);
	return CueLight
		? CueLight->GetLightColor()
		: FLinearColor::Transparent;
}

float Fdemo_mapShanmenSpiritShieldWorldPresentation::GetCueIntensity(
	const AActor* Owner)
{
	const UPointLightComponent* CueLight = FindCueLight(Owner);
	return CueLight ? CueLight->Intensity : 0.0f;
}

FVector Fdemo_mapShanmenSpiritShieldWorldPresentation::GetShellScale(
	const AActor* Owner)
{
	const UStaticMeshComponent* Shell = FindShell(Owner);
	return Shell ? Shell->GetRelativeScale3D() : FVector::ZeroVector;
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

bool Fdemo_mapShanmenSpiritShieldWorldPresentation::HasAppearance(
	const AActor* Owner,
	const FLinearColor& Color,
	float CueIntensity)
{
	const UStaticMeshComponent* Shell = FindShell(Owner);
	UMaterialInstanceDynamic* DynamicMaterial = Shell
		? Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0))
		: nullptr;
	const UPointLightComponent* CueLight = FindCueLight(Owner);
	const FLinearColor QuantizedCueColor(Color.ToFColor(false));
	return DynamicMaterial && CueLight
		&& DynamicMaterial->K2_GetVectorParameterValue(TEXT("Color"))
			.Equals(Color, KINDA_SMALL_NUMBER)
		&& CueLight->GetLightColor().Equals(
			QuantizedCueColor, KINDA_SMALL_NUMBER)
		&& FMath::IsNearlyEqual(CueLight->Intensity, CueIntensity);
}

void Fdemo_mapShanmenSpiritShieldWorldPresentation::SetAppearance(
	AActor* Owner,
	const FLinearColor& Color,
	float CueIntensity)
{
	if (UStaticMeshComponent* Shell = FindShell(Owner))
	{
		if (UMaterialInstanceDynamic* DynamicMaterial =
			Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0)))
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
			DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), Color);
		}
	}
	if (UPointLightComponent* CueLight = FindCueLight(Owner))
	{
		CueLight->SetLightColor(Color, false);
		CueLight->SetIntensity(CueIntensity);
	}
}

void Fdemo_mapShanmenSpiritShieldWorldPresentation::SetShellScale(
	AActor* Owner,
	const FVector& Scale)
{
	if (UStaticMeshComponent* Shell = FindShell(Owner))
	{
		Shell->SetRelativeScale3D(Scale);
	}
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
