#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ShanmenDemo20Session.h"
#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenDemo20Sources.h"
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
	virtual void GetPlayerViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	void ApplySurface(bool bGameplay);
	FVector GetMoveDirection() const { return MoveDirection; }
private:
	void Attack();
	void Evade();
	void UseMedicine();
	void Interact();
	void ToggleMenu();
	void ToggleInventory();
	FVector MoveDirection = FVector::ZeroVector;
	bool bPausedViewLogged = false;
	bool bPausedCameraReady = false;
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
	void UseMedicine();
	void Interact();
	void SetGuard(bool bHeld);
	void LeaveTrial();
	void ToggleInventory();
	bool IsInventoryOpen() const { return bInventoryOpen; }
	bool IsSourceSurfaceOpen() const { return SourceSearch.IsActive() || bSourcePreviewOpen; }
	bool IsSearchingSource() const { return SourceSearch.IsActive(); }
	FString GetSourceHeading() const { return FShanmenDemo20Sources::Name(SourceSearch.IsActive()?SourceSearch.Role:SourcePreview.GetPlan().SourceRoleId); }
	FString GetSourceBody() const;
	void CloseSourceSurface();
	bool IsProfileReady() const { return bProfileReady; }
	bool IsExpedition() const { return bExpeditionMode; }
	FString GetExplorationArea() const;
	float GetExtractionProgress() const { return ExtractionClock / 3.f; }
	bool IsTerminalConfirmed() const { return bTerminalConfirmed; }
	bool TryCaptureItems(FShanmenItemAuthoritySnapshot& Out) const;
	FShanmenItemDurableCommandResult EditItemGrid(const FShanmenItemGridRequest& Intent);
	FString ReplenishBasicEquipment();
	bool IsPlaying() const { return Session.GetPhase() == EShanmenDemo20Phase::Active && !bPaused && !bInventoryOpen && !IsSourceSurfaceOpen(); }
	bool IsPaused() const { return bPaused; }
	bool IsWorldReady() const { return bWorldReady; }
	const FShanmenDemo20Session& GetSession() const { return Session; }
	FString GetNotice() const { return Notice; }
	int32 GetCarryMedicine() const { return CarryMedicine; }
	int32 GetSecureMedicine() const { return SecureMedicine; }
	bool IsMedicinePending() const { return WorldCheckpoint.Medicine.IsSet() || (bCheckpointPending && PendingCheckpoint.Medicine.IsSet()); }
	static FVector ExitLocation() { return FVector(-760.f, 0.f, 0.f); }
	static FVector SentinelLocation(int32 Index);
protected:
	bool bExpeditionMode = false;
private:
	bool BuildArena();
	AActor* AddShape(const FVector& Location, const FVector& Scale, const FLinearColor& Color, bool bCollision, bool bCylinder = false);
	void RefreshSurface();
	void UpdateSentinels(float DeltaSeconds);
	bool BuildExpedition();
	void StartExpedition();
	void RestoreExpeditionOnOpen();
	void TickExpedition(float DeltaSeconds);
	void UpdateExpeditionEnemies(float DeltaSeconds);
	bool SaveExpedition(const FShanmenDemo20Session& Candidate);
	bool RetryExpeditionCheckpoint();
	bool ResolvePendingMedicine();
	void RefreshMedicineProjection();
	void ApplyExpeditionProjection();
	bool FinalizeExpedition();
	void ApplySourceProjection();
	bool TryInteractSource();
	void TickSourceSearch(float Delta);
	void ConfirmSourceSearch(FName SourceRole);
	FShanmenDemo20WorldCheckpoint CaptureWorld(const FShanmenDemo20Session& Candidate) const;
	FShanmenDemo20Session Session;
	UPROPERTY() TObjectPtr<UShanmenDemo20Widget> Screen;
	UPROPERTY() TArray<TObjectPtr<AActor>> ArenaActors;
	UPROPERTY() TArray<TObjectPtr<AActor>> Sentinels;
	UPROPERTY() TArray<TObjectPtr<AActor>> Warnings;
	UPROPERTY() TArray<TObjectPtr<AActor>> SourceMarkers;
	UPROPERTY() TObjectPtr<AActor> ExitMarker;
	float SentinelClocks[3] = {0.f, 0.f, 0.f};
	float NoticeTime = 0.f;
	FString Notice;
	bool bPaused = false;
	bool bWorldReady = false;
	bool bProfileReady = false;
	bool bInventoryOpen = false;
	FIntPoint LastViewportPixels = FIntPoint::ZeroValue;
	FShanmenDemo20WorldCheckpoint WorldCheckpoint, PendingCheckpoint;
	bool bCheckpointPending = false, bTerminalConfirmed = false;
	FString WorldProfileRoot;
	float CheckpointClock = 0.f, ExtractionClock = 0.f;
	bool bExtracting = false;
	int32 CarryMedicine = 0, SecureMedicine = 0;
	FShanmenDemo20Search SourceSearch;
	FShanmenItemGeneratedSourceReceipt SourcePreview;
	bool bSourcePreviewOpen = false;
	FGuid SourceLayoutRun;
};

/** Formal exploration map; shares existing Demo20 UI/input, not the practice map. */
UCLASS()
class AShanmenDemo20ExpeditionGameMode : public AShanmenDemo20GameMode
{
	GENERATED_BODY()
public:
	AShanmenDemo20ExpeditionGameMode() { bExpeditionMode = true; }
};
