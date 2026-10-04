#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ShanmenDemo20Session.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenDemo20World.generated.h"

class UStaticMeshComponent;
class UShanmenDemo20Widget;

/** Fresh engine-primitive avatar: no dependency on any previous Demo Blueprint. */
UCLASS()
class AShanmenDemo20Character : public ACharacter
{
	GENERATED_BODY()
public:
	AShanmenDemo20Character();
	virtual void Tick(float DeltaSeconds) override;
private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Robe;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Blade;
};

UCLASS()
class AShanmenDemo20Controller : public APlayerController
{
	GENERATED_BODY()
public:
	AShanmenDemo20Controller();
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaSeconds) override;
	void ApplySurface(bool bGameplay);
	FVector GetMoveDirection() const { return MoveDirection; }
private:
	void Attack();
	void Evade();
	void Interact();
	void ToggleMenu();
	void ToggleInventory();
	FVector MoveDirection = FVector::ZeroVector;
};

/** Demo-specific composition only; damage and vitality remain existing domain types. */
UCLASS()
class AShanmenDemo20GameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AShanmenDemo20GameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void StartTrial();
	void ReturnToPreparation();
	void TogglePause();
	void PauseForFocusLoss();
	void Attack();
	void Evade();
	void Interact();
	void SetGuard(bool bHeld);
	void LeaveTrial();
	void ToggleInventory();
	bool IsInventoryOpen() const { return bInventoryOpen; }
	bool IsProfileReady() const { return bProfileReady; }
	bool TryCaptureItems(FShanmenItemAuthoritySnapshot& Out) const;
	FShanmenItemDurableCommandResult EditItemGrid(const FShanmenItemGridRequest& Intent);
	bool IsPlaying() const { return Session.GetPhase() == EShanmenDemo20Phase::Active && !bPaused; }
	bool IsPaused() const { return bPaused; }
	bool IsWorldReady() const { return bWorldReady; }
	const FShanmenDemo20Session& GetSession() const { return Session; }
	FString GetNotice() const { return Notice; }
	static FVector ExitLocation() { return FVector(-760.f, 0.f, 0.f); }
	static FVector SentinelLocation(int32 Index);
private:
	bool BuildArena();
	AActor* AddShape(const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision, bool bCylinder = false);
	void RefreshSurface();
	void UpdateSentinels(float DeltaSeconds);
	FShanmenDemo20Session Session;
	UPROPERTY() TObjectPtr<UShanmenDemo20Widget> Screen;
	UPROPERTY() TArray<TObjectPtr<AActor>> ArenaActors;
	UPROPERTY() TArray<TObjectPtr<AActor>> Sentinels;
	UPROPERTY() TArray<TObjectPtr<AActor>> Warnings;
	UPROPERTY() TObjectPtr<AActor> ExitMarker;
	float SentinelClocks[3] = {0.f, 0.f, 0.f};
	float NoticeTime = 0.f;
	FString Notice;
	bool bPaused = false;
	bool bWorldReady = false;
	bool bProfileReady = false;
	bool bInventoryOpen = false;
	FIntPoint LastViewportPixels = FIntPoint::ZeroValue;
};
