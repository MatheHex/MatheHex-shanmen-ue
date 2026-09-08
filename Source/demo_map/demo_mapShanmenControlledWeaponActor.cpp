#include "demo_mapShanmenControlledWeaponActor.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "demo_mapShanmenControlledWeaponWorldAdapter.h"
#include "demo_mapShanmenControlledWeaponWorldThreatSampler.h"

namespace
{
	const FLinearColor IdleSwordColor(0.20f, 0.48f, 0.95f);
	const FLinearColor DirectedSwordColor(1.00f, 0.48f, 0.08f);
	const FLinearColor ReturningSwordColor(0.48f, 0.32f, 1.00f);
	const FLinearColor RedeployedSwordColor(0.72f, 1.00f, 0.88f);
	const FLinearColor ThreatSwordColor(0.05f, 1.00f, 0.72f);
}

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
	VisualMaterial = Visual->CreateDynamicMaterialInstance(0);

	ThreatCueLight = CreateDefaultSubobject<UPointLightComponent>(
		TEXT("ControlledWeaponThreatCueLight"));
	ThreatCueLight->SetupAttachment(Collision);
	ThreatCueLight->SetLightColor(ThreatSwordColor);
	ThreatCueLight->SetIntensity(2200.0f);
	ThreatCueLight->SetAttenuationRadius(180.0f);
	ThreatCueLight->SetVisibility(false);
	RefreshThreatPresenceCue();
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
		|| GetRootComponent() != Collision
		|| !IsThreatPresenceCueStateValid())
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
		&& GetRootComponent() == Collision
		&& IsThreatPresenceCueStateValid()
		&& IsFlightPresentationStateValid()
		&& IsCommittedImpactFeedbackStateValid();
}

bool Ademo_mapShanmenControlledWeaponActor::TryPresentThreatPresenceCue(
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult& Sample)
{
	if (!Sample.IsSampled()
		|| !IsProductBoundTo(
			Sample.RunId, Sample.ItemInstanceId, SourceActor.Get()))
	{
		return false;
	}

	if (LastThreatPresenceCueSampleSequence != INDEX_NONE)
	{
		if (Sample.SampleSequence < LastThreatPresenceCueSampleSequence)
		{
			return false;
		}
		if (Sample.SampleSequence == LastThreatPresenceCueSampleSequence)
		{
			return LastThreatPresenceCueIntentId == Sample.IntentId
				&& ThreatPresenceCueContactCount
					== Sample.RoutedContactCount
				&& bThreatPresenceCueActive
					== (Sample.RoutedContactCount > 0)
				&& IsThreatPresenceCueStateValid();
		}
	}

	LastThreatPresenceCueIntentId = Sample.IntentId;
	LastThreatPresenceCueSampleSequence = Sample.SampleSequence;
	ThreatPresenceCueContactCount = Sample.RoutedContactCount;
	bThreatPresenceCueActive = ThreatPresenceCueContactCount > 0;
	RefreshThreatPresenceCue();
	return IsThreatPresenceCueStateValid();
}

bool Ademo_mapShanmenControlledWeaponActor::TryClearThreatPresenceCue(
	const FGuid& ExpectedRunId,
	const FGuid& ExpectedItemInstanceId)
{
	if (!IsProductBoundTo(
			ExpectedRunId, ExpectedItemInstanceId, SourceActor.Get()))
	{
		return false;
	}
	ThreatPresenceCueContactCount = 0;
	bThreatPresenceCueActive = false;
	RefreshThreatPresenceCue();
	return IsThreatPresenceCueStateValid();
}

bool Ademo_mapShanmenControlledWeaponActor::IsThreatPresenceCueVisualActive()
	const
{
	return ThreatCueLight && ThreatCueLight->IsVisible();
}

bool Ademo_mapShanmenControlledWeaponActor::TryPresentFlightReadModel(
	const Fdemo_mapShanmenControlledWeaponFlightReadModel& ReadModel)
{
	if (!ReadModel.IsValid()
		|| !IsProductBoundTo(
			ReadModel.GetRunId(),
			ReadModel.GetItemInstanceId(),
			SourceActor.Get())
		|| !ReadModel.GetWeaponLocation().Equals(
			GetActorLocation(), KINDA_SMALL_NUMBER))
	{
		return false;
	}

	if (FlightReadModel.Matches(ReadModel))
	{
		return true;
	}
	if (PresentedImpactActivationId.IsValid()
		&& PresentedImpactActivationId != ReadModel.GetActivationId())
	{
		ClearCommittedImpactFeedback();
	}
	FlightReadModel = ReadModel;
	RefreshThreatPresenceCue();
	return IsFlightPresentationStateValid()
		&& IsCommittedImpactFeedbackStateValid();
}

bool Ademo_mapShanmenControlledWeaponActor::
TryPresentCommittedImpactFeedback(
	const Fdemo_mapShanmenControlledWeaponWorldDeliveryResult& Delivery)
{
	const FShanmenImpactRequest& Request = Delivery.Impact.GetRequest();
	const FShanmenVitalityCommitResult& CommitResult =
		Delivery.Delivery.CommitResult;
	const FShanmenVitalityCommitReceipt& CommitReceipt =
		CommitResult.Receipt;
	const float AppliedDamage = CommitReceipt.GetAppliedDamage();
	const float VitalityAfter = CommitReceipt.GetVitalityAfter();
	if (!Delivery.IsDelivered()
		|| CommitResult.Status != EShanmenVitalityCommitStatus::Committed
		|| !IsProductBoundTo(
			Request.Action.GetRunId(),
			Request.Action.GetSourceItemInstanceId(),
			SourceActor.Get())
		|| !Request.Action.GetActivationId().IsValid()
		|| Delivery.TargetEntityId != Request.Candidate.TargetEntityId
		|| CommitReceipt.GetImpactId() != Request.ImpactId
		|| CommitReceipt.GetTargetEntityId() != Delivery.TargetEntityId
		|| !FMath::IsFinite(AppliedDamage)
		|| AppliedDamage < 0.0f
		|| !FMath::IsFinite(VitalityAfter)
		|| VitalityAfter < 0.0f
		|| (FlightReadModel.IsValid()
			&& FlightReadModel.GetActivationId()
				!= Request.Action.GetActivationId()))
	{
		return false;
	}

	if (PresentedImpactId.IsValid())
	{
		return PresentedImpactId == Request.ImpactId
			&& PresentedImpactActivationId
				== Request.Action.GetActivationId()
			&& PresentedImpactTargetEntityId == Delivery.TargetEntityId
			&& PresentedImpactAppliedDamage == AppliedDamage
			&& PresentedImpactTargetVitalityAfter == VitalityAfter
			&& IsCommittedImpactFeedbackStateValid();
	}

	PresentedImpactId = Request.ImpactId;
	PresentedImpactActivationId = Request.Action.GetActivationId();
	PresentedImpactTargetEntityId = Delivery.TargetEntityId;
	PresentedImpactAppliedDamage = AppliedDamage;
	PresentedImpactTargetVitalityAfter = VitalityAfter;
	return IsCommittedImpactFeedbackStateValid();
}

FLinearColor
Ademo_mapShanmenControlledWeaponActor::GetResolvedPresentationColor() const
{
	if (bThreatPresenceCueActive)
	{
		return ThreatSwordColor;
	}
	if (!FlightReadModel.IsValid())
	{
		return IdleSwordColor;
	}

	switch (FlightReadModel.GetPhase())
	{
	case Edemo_mapShanmenControlledWeaponFlightPhase::Directed:
		return DirectedSwordColor;
	case Edemo_mapShanmenControlledWeaponFlightPhase::Returning:
		return ReturningSwordColor;
	case Edemo_mapShanmenControlledWeaponFlightPhase::Redeployed:
		return RedeployedSwordColor;
	case Edemo_mapShanmenControlledWeaponFlightPhase::Orbiting:
	default:
		return IdleSwordColor;
	}
}

bool Ademo_mapShanmenControlledWeaponActor::IsThreatPresenceCueStateValid()
	const
{
	const bool bHasCommittedSample =
		LastThreatPresenceCueSampleSequence != INDEX_NONE;
	return ThreatCueLight
		&& ThreatCueLight->GetAttachParent() == Collision
		&& ThreatPresenceCueContactCount >= 0
		&& bThreatPresenceCueActive
			== (ThreatPresenceCueContactCount > 0)
		&& IsThreatPresenceCueVisualActive() == bThreatPresenceCueActive
		&& (bHasCommittedSample
			? LastThreatPresenceCueSampleSequence >= 0
				&& LastThreatPresenceCueIntentId.IsValid()
			: !LastThreatPresenceCueIntentId.IsValid()
				&& ThreatPresenceCueContactCount == 0
				&& !bThreatPresenceCueActive);
}

bool Ademo_mapShanmenControlledWeaponActor::IsFlightPresentationStateValid()
	const
{
	return !FlightReadModel.IsValid()
		|| FlightReadModel.MatchesProduct(RunId, ItemInstanceId);
}

bool Ademo_mapShanmenControlledWeaponActor::
IsCommittedImpactFeedbackStateValid() const
{
	if (!PresentedImpactId.IsValid())
	{
		return !PresentedImpactActivationId.IsValid()
			&& !PresentedImpactTargetEntityId.IsValid()
			&& PresentedImpactAppliedDamage == 0.0f
			&& PresentedImpactTargetVitalityAfter == 0.0f;
	}
	return PresentedImpactActivationId.IsValid()
		&& PresentedImpactTargetEntityId.IsValid()
		&& FMath::IsFinite(PresentedImpactAppliedDamage)
		&& PresentedImpactAppliedDamage >= 0.0f
		&& FMath::IsFinite(PresentedImpactTargetVitalityAfter)
		&& PresentedImpactTargetVitalityAfter >= 0.0f
		&& (!FlightReadModel.IsValid()
			|| FlightReadModel.GetActivationId()
				== PresentedImpactActivationId);
}

void Ademo_mapShanmenControlledWeaponActor::ClearCommittedImpactFeedback()
{
	PresentedImpactId.Invalidate();
	PresentedImpactActivationId.Invalidate();
	PresentedImpactTargetEntityId.Invalidate();
	PresentedImpactAppliedDamage = 0.0f;
	PresentedImpactTargetVitalityAfter = 0.0f;
}

void Ademo_mapShanmenControlledWeaponActor::RefreshThreatPresenceCue()
{
	const FLinearColor Color = GetResolvedPresentationColor();
	if (VisualMaterial)
	{
		VisualMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		VisualMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
	if (ThreatCueLight)
	{
		ThreatCueLight->SetVisibility(bThreatPresenceCueActive);
	}
}

void Ademo_mapShanmenControlledWeaponActor::ActivateProductCollision()
{
	check(IsProductBound());
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void Ademo_mapShanmenControlledWeaponActor::DeactivateProductCollision()
{
	ThreatPresenceCueContactCount = 0;
	bThreatPresenceCueActive = false;
	FlightReadModel =
		Fdemo_mapShanmenControlledWeaponFlightReadModel();
	ClearCommittedImpactFeedback();
	RefreshThreatPresenceCue();
	if (Collision)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
