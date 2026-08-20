#include "demo_mapCorpseContainerActor.h"
#include "demo_mapSearchContainerTypes.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapV3ProgressionManager.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

Ademo_mapCorpseContainerActor::Ademo_mapCorpseContainerActor()
{
	SetContainerMeshColor(FLinearColor(0.20f, 0.04f, 0.04f));
	if (UStaticMeshComponent* ContainerMesh = GetContainerMesh())
	{
		ContainerMesh->SetRelativeScale3D(FVector(1.25f, 0.55f, 0.25f));
	}
	SetContainerDisplayLabel(TEXT("CORPSE"));
}

void Ademo_mapCorpseContainerActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (CodeBBodyPendingActionId.IsValid())
	{
		if (Ademo_mapV3ProgressionManager* ProgressionManager = ResolveManager())
		{
			if (!ProgressionManager->IsCodeBBodyContainerActionStillValid(this, CodeBBodyPendingActionId))
			{
				ProgressionManager->InterruptCodeBBodyContainerAction(this, TEXT("DistanceLostOrRunInvalid"));
				ClearCodeBBodyPendingAction();
			}
		}
		else
		{
			ClearCodeBBodyPendingAction();
		}
	}
}

void Ademo_mapCorpseContainerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasCodeBBodyContainerInteraction())
	{
		if (Ademo_mapV3ProgressionManager* ProgressionManager = ResolveManager())
		{
			ProgressionManager->InterruptCodeBBodyContainerAction(this, TEXT("ActorDestroyed"));
		}
	}
	ClearCodeBBodyPendingAction();
	Super::EndPlay(EndPlayReason);
}

void Ademo_mapCorpseContainerActor::EnableCodeBBodyContainerInteraction(const FName InBodyTargetIdentity)
{
	CodeBBodyTargetIdentity = InBodyTargetIdentity;
}

void Ademo_mapCorpseContainerActor::ScheduleCodeBBodyOpenCompletion(
	const FGuid& ActionId,
	const float DurationSeconds)
{
	CodeBBodyPendingActionId = ActionId;
	bCodeBBodyPendingSearch = false;
	GetWorldTimerManager().SetTimer(
		CodeBBodyPendingActionTimer, this,
		&Ademo_mapCorpseContainerActor::CompleteCodeBBodyPendingAction,
		DurationSeconds, false);
}

void Ademo_mapCorpseContainerActor::ScheduleCodeBBodySearchCompletion(
	const FGuid& ActionId,
	const float DurationSeconds)
{
	CodeBBodyPendingActionId = ActionId;
	bCodeBBodyPendingSearch = true;
	GetWorldTimerManager().SetTimer(
		CodeBBodyPendingActionTimer, this,
		&Ademo_mapCorpseContainerActor::CompleteCodeBBodyPendingAction,
		DurationSeconds, false);
}

void Ademo_mapCorpseContainerActor::ClearCodeBBodyPendingAction()
{
	GetWorldTimerManager().ClearTimer(CodeBBodyPendingActionTimer);
	CodeBBodyPendingActionId.Invalidate();
	bCodeBBodyPendingSearch = false;
}

bool Ademo_mapCorpseContainerActor::CanInteract(const APlayerController* Controller) const
{
	return HasCodeBBodyContainerInteraction()
		? Controller != nullptr && ResolveManager() != nullptr && Super::CanInteract(Controller)
		: Super::CanInteract(Controller);
}

FText Ademo_mapCorpseContainerActor::GetInteractionPrompt(const APlayerController* Controller) const
{
	return HasCodeBBodyContainerInteraction()
		? FText::FromString(TEXT("[G] 搜查尸体 / SEARCH CORPSE"))
		: Super::GetInteractionPrompt(Controller);
}

Fdemo_mapItemOperationResult Ademo_mapCorpseContainerActor::RequestInteract(APlayerController* Controller)
{
	if (!HasCodeBBodyContainerInteraction())
	{
		return Super::RequestInteract(Controller);
	}
	if (Ademo_mapV3ProgressionManager* ProgressionManager = ResolveManager())
	{
		return ProgressionManager->RequestCodeBBodyContainerInteract(this);
	}
	return Fdemo_mapItemOperationResult::Failure(
		Edemo_mapItemResultCode::InteractionBlocked,
		TEXT("Code B body-container manager is unavailable."));
}

bool Ademo_mapCorpseContainerActor::InitializeCorpse(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	FGuid InLootSourceId)
{
	LootSourceId = InLootSourceId;
	LootTableId = NAME_None;
	return LootSourceId.IsValid()
		&& InitializeSearchContainer(
			InManager,
			InItems,
			InRunId,
			Edemo_mapRuntimeContainerKind::Corpse,
			FName(*LootSourceId.ToString(EGuidFormats::Digits)),
			Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed());
}

bool Ademo_mapCorpseContainerActor::InitializeM01PrototypeCorpse(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	FGuid InLootSourceId,
	FName InRewardSourceRoleId,
	FName InCorpseIdentity)
{
	if (!InLootSourceId.IsValid()
		|| InRewardSourceRoleId.IsNone()
		|| InCorpseIdentity.IsNone())
	{
		return false;
	}
	LootSourceId = InLootSourceId;
	LootTableId = NAME_None;
	RewardProjectionId = NAME_None;
	RewardSourceRoleId = InRewardSourceRoleId;
	CorpseIdentity = InCorpseIdentity;
	SetContainerDisplayLabel(FString::Printf(
		TEXT("M01 CORPSE\n%s"),
		*InCorpseIdentity.ToString()));
	return InitializeSearchContainer(
		InManager,
		InItems,
		InRunId,
		Edemo_mapRuntimeContainerKind::Corpse,
		InRewardSourceRoleId,
		Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed());
}

bool Ademo_mapCorpseContainerActor::InitializeFixedCorpse(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	FGuid InLootSourceId,
	FName InLootTableId)
{
	const Fdemo_mapFixedLootTableDefinition* Table =
		Fdemo_mapItemDefinitions::FindFixedLootProfile(InLootTableId);
	if (!InLootSourceId.IsValid()
		|| !Table
		|| Table->Kind != Edemo_mapRuntimeContainerKind::Corpse)
	{
		return false;
	}
	LootSourceId = InLootSourceId;
	LootTableId = InLootTableId;
	RewardSourceRoleId = NAME_None;
	CorpseIdentity = NAME_None;
	return InitializeSearchContainer(
		InManager,
		InItems,
		InRunId,
		Edemo_mapRuntimeContainerKind::Corpse,
		InLootTableId,
		Table->Entries);
}

bool Ademo_mapCorpseContainerActor::InitializeGeneratedCorpse(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	FGuid InLootSourceId,
	const Fdemo_mapRewardSourceProjection& Projection,
	const Fdemo_mapRewardSourceProjectionResult& Plan)
{
	if (!InLootSourceId.IsValid()
		|| !Projection.IsValid())
	{
		return false;
	}
	LootSourceId = InLootSourceId;
	LootTableId = Projection.FixedFallbackTableId;
	RewardProjectionId = Projection.ProjectionId;
	RewardSourceRoleId = Projection.StableSourceRoleId;
	CorpseIdentity = NAME_None;
	if (!Projection.SourceDisplayLabel.IsEmpty())
	{
		SetContainerDisplayLabel(Projection.SourceDisplayLabel);
	}
	Fdemo_mapPersistentGeneratedRewardSource ExistingSource;
	if (InManager && InManager->FindDurablyAcceptedRewardSource(
		InRunId, Projection.StableSourceRoleId, ExistingSource))
	{
		if (InitializeCommittedSearchContainer(
			InManager, InItems, InRunId,
			Edemo_mapRuntimeContainerKind::Corpse, ExistingSource))
		{
			return true;
		}
		MarkCommittedRewardSourcePendingReconciliation(
			TEXT("Durably accepted Corpse source is awaiting Runtime reconciliation without reroll."));
		return true;
	}
	if (!Plan.IsSuccess() || Plan.PlannedStacks.IsEmpty())
	{
		return false;
	}
	ProjectionResult = Plan;
	const Fdemo_mapProfileGeneratedRewardSourceResult Accepted =
		InManager->PrepareGeneratedRewardSource(
		InRunId, Projection.StableSourceRoleId,
		Fdemo_mapRewardSourceAcceptanceReceipt::FromProjection(
			Projection, Plan));
	if (!Accepted.IsDurablyCommitted())
	{
		return false;
	}
	if (Accepted.RequiresRuntimeReconciliation())
	{
		MarkCommittedRewardSourcePendingReconciliation(Accepted.Diagnostic);
		return true;
	}
	if (InitializeCommittedSearchContainer(
		InManager, InItems, InRunId,
		Edemo_mapRuntimeContainerKind::Corpse, Accepted.Source))
	{
		return true;
	}
	MarkCommittedRewardSourcePendingReconciliation(
		TEXT("Generated corpse source is durably accepted; Runtime projection is pending reconciliation without reroll."));
	return true;
}

bool Ademo_mapCorpseContainerActor::InitializeM01GeneratedCorpse(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId,
	FGuid InLootSourceId,
	const Fdemo_mapRewardSourceProjection& Projection,
	const Fdemo_mapRewardSourceProjectionResult& Plan,
	FName InCorpseIdentity)
{
	if (InCorpseIdentity.IsNone()
		|| !InitializeGeneratedCorpse(
			InManager, InItems, InRunId, InLootSourceId, Projection, Plan))
	{
		return false;
	}
	CorpseIdentity = InCorpseIdentity;
	SetContainerDisplayLabel(FString::Printf(
		TEXT("M01 CORPSE REWARD\n%s"), *InCorpseIdentity.ToString()));
	return true;
}

Ademo_mapV3ProgressionManager* Ademo_mapCorpseContainerActor::ResolveManager() const
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

void Ademo_mapCorpseContainerActor::CompleteCodeBBodyPendingAction()
{
	const FGuid ActionId = CodeBBodyPendingActionId;
	const bool bSearch = bCodeBBodyPendingSearch;
	ClearCodeBBodyPendingAction();
	if (Ademo_mapV3ProgressionManager* ProgressionManager = ResolveManager())
	{
		ProgressionManager->CompleteCodeBBodyContainerAction(this, ActionId, bSearch);
	}
}
