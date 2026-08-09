#pragma once

#include "CoreMinimal.h"
#include "demo_mapSearchContainerActor.h"
#include "demo_mapRewardSourceProjection.h"
#include "demo_mapCorpseContainerActor.generated.h"

class Ademo_mapV3ProgressionManager;
class Udemo_mapItemSubsystem;

/** One explicitly marked P4 melee Enemy Death -> Corpse product slice. */
UCLASS()
class Ademo_mapCorpseContainerActor : public Ademo_mapSearchContainerActor
{
	GENERATED_BODY()

public:
	Ademo_mapCorpseContainerActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	bool InitializeCorpse(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId,
		FGuid InLootSourceId);
	bool InitializeM01PrototypeCorpse(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId,
		FGuid InLootSourceId,
		FName InRewardSourceRoleId,
		FName InCorpseIdentity);
	bool InitializeFixedCorpse(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId,
		FGuid InLootSourceId,
		FName InLootTableId);
	bool InitializeGeneratedCorpse(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId,
		FGuid InLootSourceId,
		const Fdemo_mapRewardSourceProjection& Projection,
		const Fdemo_mapRewardSourceProjectionResult& Plan);
	bool InitializeM01GeneratedCorpse(
		Ademo_mapV3ProgressionManager* InManager,
		Udemo_mapItemSubsystem* InItems,
		FGuid InRunId,
		FGuid InLootSourceId,
		const Fdemo_mapRewardSourceProjection& Projection,
		const Fdemo_mapRewardSourceProjectionResult& Plan,
		FName InCorpseIdentity);
	/** P12 transient routing only; the static identity remains the Code B durable gate. */
	void EnableCodeBBodyContainerInteraction(FName InBodyTargetIdentity);
	bool HasCodeBBodyContainerInteraction() const { return !CodeBBodyTargetIdentity.IsNone(); }
	FName GetCodeBBodyTargetIdentity() const { return CodeBBodyTargetIdentity; }
	void ScheduleCodeBBodyOpenCompletion(const FGuid& ActionId, float DurationSeconds);
	void ScheduleCodeBBodySearchCompletion(const FGuid& ActionId, float DurationSeconds);
	void ClearCodeBBodyPendingAction();
	FGuid GetLootSourceId() const { return LootSourceId; }
	FName GetLootTableId() const { return LootTableId; }
	FName GetRewardProjectionId() const { return RewardProjectionId; }
	FName GetRewardSourceRoleId() const { return RewardSourceRoleId; }
	FName GetCorpseIdentity() const { return CorpseIdentity; }
	bool UsesGeneratedReward() const { return !RewardProjectionId.IsNone(); }
	const Fdemo_mapRewardSourceProjectionResult& GetProjectionResult() const
	{
		return ProjectionResult;
	}

	virtual bool CanInteract(const APlayerController* Controller) const override;
	virtual FText GetInteractionPrompt(const APlayerController* Controller) const override;
	virtual Fdemo_mapItemOperationResult RequestInteract(APlayerController* Controller) override;

private:
	Ademo_mapV3ProgressionManager* ResolveManager() const;
	void CompleteCodeBBodyPendingAction();

	FGuid LootSourceId;
	FName LootTableId = NAME_None;
	FName RewardProjectionId = NAME_None;
	FName RewardSourceRoleId = NAME_None;
	FName CorpseIdentity = NAME_None;
	Fdemo_mapRewardSourceProjectionResult ProjectionResult;
	FName CodeBBodyTargetIdentity = NAME_None;
	FTimerHandle CodeBBodyPendingActionTimer;
	FGuid CodeBBodyPendingActionId;
	bool bCodeBBodyPendingSearch = false;
};
