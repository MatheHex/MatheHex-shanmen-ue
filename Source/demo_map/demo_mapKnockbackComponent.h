#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapKnockbackComponent.generated.h"

UCLASS(ClassGroup=(Combat))
class Udemo_mapKnockbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapKnockbackComponent();
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static Udemo_mapKnockbackComponent* FindOrCreate(class ACharacter* Character);
	static bool IsRequestStructValid(
		bool bAlreadyActive,
		const Fdemo_mapKnockbackIntent& Intent);
	bool TryStart(const Fdemo_mapKnockbackIntent& Intent);
	void Cancel(bool bRestoreMovement);

	bool IsActive() const { return bActive; }
	float GetResolvedDistance() const { return ResolvedDistance; }
	int32 GetAcceptedCount() const { return AcceptedCount; }
	int32 GetRejectedWhileActiveCount() const
	{
		return RejectedWhileActiveCount;
	}

private:
	void Finish();
	double GetNow() const;

	bool bActive = false;
	FVector Direction = FVector::ZeroVector;
	float RequestedDistance = 0.0f;
	float ResolvedDistance = 0.0f;
	float Duration = 0.0f;
	double StartTime = 0.0;
	EMovementMode SavedMovementMode = MOVE_Walking;
	uint8 SavedCustomMovementMode = 0;
	int32 AcceptedCount = 0;
	int32 RejectedWhileActiveCount = 0;
};
