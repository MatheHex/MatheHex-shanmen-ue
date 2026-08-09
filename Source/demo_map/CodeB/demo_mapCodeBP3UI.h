// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CodeB/demo_mapCodeBP3.h"
#include "CodeB/demo_mapCodeBP4.h"
#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

#include "demo_mapCodeBP3UI.generated.h"

class UEditableTextBox;
class UVerticalBox;
class UCodeBP3InventoryWidget;

/** P10-only display policy for the existing production P3/P4 Host. */
struct FCodeBP3NormalContainerPresentation
{
	FGuid TargetContainerId;
	FString Title;
	FCodeBNormalContainerProjection Projection;
	/** The manager/service starts the durable search and returns its fresh projection. */
	TFunction<bool(const FGuid&, FCodeBNormalContainerProjection&, FString&)> BeginItemSearch;
};

/** P12-only display policy for the existing production P3/P4 Host. */
struct FCodeBP3BodyContainerPresentation
{
	FGuid TargetContainerId;
	FString Title;
	FCodeBBodyContainerProjection Projection;
	/** The manager/service starts the durable body search and returns its fresh projection. */
	TFunction<bool(const FGuid&, FCodeBBodyContainerProjection&, FString&)> BeginItemSearch;
};

/** P13 keeps the P3/P4 host projection-only; all writes enter a Store-owned binding service callback. */
struct FCodeBP3HotbarPresentation
{
	FCodeBHotbarProjection Projection;
	TFunction<bool(FCodeBHotbarProjection&, FString&)> Refresh;
	TFunction<bool(const FGuid&, int32, FCodeBHotbarProjection&, FString&)> Bind;
	TFunction<bool(int32, FCodeBHotbarProjection&, FString&)> Unbind;
};

/** P14's one actual ground-drop target. It carries no item authority itself. */
struct FCodeBP3GroundDropPresentation
{
	TFunction<bool(const demo_map_code_b::FCodeBP4DragPayload&, FString&)> RequestDrop;
};

/** P14's transient one-record target. It is a projection of the P6 WorldDrop graph. */
struct FCodeBP3WorldDropPresentation
{
	FGuid TargetContainerId;
	FString Title;
};

/** UMG carrier for P4's stable, read-only drag descriptor. */
UCLASS()
class DEMO_MAP_API UCodeBP4DragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	void Configure(const demo_map_code_b::FCodeBP4DragPayload& InPayload, UCodeBP3InventoryWidget* InOwnerWidget) { Payload = InPayload; OwnerWidget = InOwnerWidget; }
	const demo_map_code_b::FCodeBP4DragPayload& GetPayload() const { return Payload; }

protected:
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;

private:
	demo_map_code_b::FCodeBP4DragPayload Payload;
	TWeakObjectPtr<UCodeBP3InventoryWidget> OwnerWidget;
};

/** Reusable stable grid-cell button.  Its address is display-only and is refreshed from a P2 projection. */
UCLASS()
class DEMO_MAP_API UCodeBP3CellButton : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UCodeBP3InventoryWidget* InOwnerWidget, const demo_map_code_b::FCodeBP3SlotAddress& InAddress);
	const demo_map_code_b::FCodeBP3SlotAddress& GetAddress() const { return Address; }
	void SetCellContent(UWidget* InContent);
	void SetCellColor(const FLinearColor& InColor);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InPointerEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildButton();

	TWeakObjectPtr<UCodeBP3InventoryWidget> OwnerWidget;
	UPROPERTY(Transient)
	TObjectPtr<UButton> InnerButton;
	demo_map_code_b::FCodeBP3SlotAddress Address;
};

/** Explicit P13 reference action control. It does not carry an ItemId or any P1 write capability. */
UCLASS()
class DEMO_MAP_API UCodeBP3HotbarActionButton : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UCodeBP3InventoryWidget* InOwnerWidget, int32 InSlotIndex, bool bInUnbindAction);
	void SetLabel(const FString& InLabel, bool bEnabled);

private:
	void BuildButton();
	UFUNCTION()
	void OnClicked();

	TWeakObjectPtr<UCodeBP3InventoryWidget> OwnerWidget;
	UPROPERTY(Transient)
	TObjectPtr<UButton> InnerButton;
	int32 SlotIndex = INDEX_NONE;
	bool bUnbindAction = false;
};

/** Visible P6-only drag surface. It writes only from NativeOnDrop through its Host callback. */
UCLASS()
class DEMO_MAP_API UCodeBP3GroundDropZone : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UCodeBP3InventoryWidget* InOwnerWidget);

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildZone();
	TWeakObjectPtr<UCodeBP3InventoryWidget> OwnerWidget;
	UPROPERTY(Transient)
	TObjectPtr<class UBorder> InnerBorder;
};

/** Actual native UMG P3 page.  All writes route to the P3 controller and then P2 ApplicationService. */
UCLASS()
class DEMO_MAP_API UCodeBP3InventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCodeBP3InventoryWidget(const FObjectInitializer& ObjectInitializer);
	void InitializeForHost(class UCodeBP3UIHostSubsystem* InHost);
	void RefreshFromController();
	/** Returns the currently mounted production cell for a stable P2 address.  Read-only support for the Slate input smoke harness. */
	UCodeBP3CellButton* FindMountedCell(const FGuid& ContainerId, int32 SlotIndex) const;
	/** Returns the currently mounted production controls used by the real-input lifecycle smoke. */
	UButton* GetMountedCloseButton() const { return CloseButton; }
	void HandleCellActivated(UCodeBP3CellButton* CellButton);
	void HandleHotbarSlotAction(int32 SlotIndex, bool bUnbindAction);
	void BeginP4PointerGesture(const demo_map_code_b::FCodeBP3SlotAddress& Address);
	void TraceP4Input(const FString& EventName, const demo_map_code_b::FCodeBP3SlotAddress& Address, const FString& Detail = FString(), bool bSubmittedCommand = false);
	bool BeginP4Drag(UCodeBP3CellButton* CellButton, demo_map_code_b::FCodeBP4DragPayload& OutPayload);
	void HandleP4DragCancelled(UCodeBP4DragOperation* Operation);
	bool HasP4Preview() const { return P4Preview.IsSet(); }
	int32 GetP4CommandCount() const { return CurrentP4SubmittedCommandCount; }
	int32 GetP4CallCount() const { return CurrentP4P2CallCount; }
	int32 GetP4RevisionBefore() const { return CurrentP4RevisionBefore; }
	void HandleP4DragEnter(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation);
	void HandleP4DragLeave(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation);
	bool HandleP4Drop(UCodeBP3CellButton* CellButton, UCodeBP4DragOperation* Operation);
	bool HandleGroundDropZoneDrop(UCodeBP4DragOperation* Operation);
	void HandleP4ContextMenu(UCodeBP3CellButton* CellButton);
	void SetP4PreviewCapture(const demo_map_code_b::FCodeBP4DragPayload& Payload, const demo_map_code_b::FCodeBP3SlotAddress& Target);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildLayout();
	void BuildPageContents();
	UVerticalBox* AddPanel(UVerticalBox* Parent, const FString& Title);
	void AddContainerSection(UVerticalBox* Parent, const FString& Title, const demo_map_code_b::FCodeBP2ContainerView& Container, int32 Columns);
	void AddNormalContainerSection(UVerticalBox* Parent, const demo_map_code_b::FCodeBP2ContainerView& Container);
	void AddBodyContainerSection(UVerticalBox* Parent, const demo_map_code_b::FCodeBP2ContainerView& Container);
	void AddBodyEquipmentContainerSection(UVerticalBox* Parent, const demo_map_code_b::FCodeBP2ContainerView& Container, FName SlotSemantic);
	void AddHotbarPlaceholders(UVerticalBox* Parent);
	void AddGroundDropZone(UVerticalBox* Parent);
	void AddDetailAndActions(UVerticalBox* Parent);
	void AddP4ContextActions(UVerticalBox* Parent);
	const demo_map_code_b::FCodeBP2ContainerView* FindRole(FName Role) const;
	bool IsSelectedItemInEquipmentSlot() const;
	bool IsSelectedItemEquipable() const;
	FLinearColor GetNormalCellColor(const demo_map_code_b::FCodeBP3SlotAddress& Address) const;
	demo_map_code_b::FCodeBP4InteractionController* GetP4Controller();

	UFUNCTION()
	void OnEquipGuidanceClicked();
	UFUNCTION()
	void OnUnequipGuidanceClicked();
	UFUNCTION()
	void OnContextDetailClicked();
	UFUNCTION()
	void OnCloseClicked();

	TWeakObjectPtr<class UCodeBP3UIHostSubsystem> Host;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PageContents;
	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> SplitQuantityBox;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UCodeBP3CellButton>> MountedCells;
	TUniquePtr<demo_map_code_b::FCodeBP4InteractionController> P4Controller;
	TOptional<demo_map_code_b::FCodeBP3SlotAddress> ContextMenuAddress;
	TOptional<demo_map_code_b::FCodeBP3SlotAddress> P4PreviewAddress;
	TOptional<demo_map_code_b::FCodeBP4DropPreview> P4Preview;
	int32 ContextMenuRevision = INDEX_NONE;
	uint64 NextP4GestureId = 0;
	uint64 CurrentP4GestureId = 0;
	int32 CurrentP4SubmittedCommandCount = 0;
	int32 CurrentP4P2CallCount = 0;
	int32 CurrentP4RevisionBefore = INDEX_NONE;
	bool bLayoutBuilt = false;
};

/**
 * Default-off GameInstance host for the P3 development page.  It creates no P2 fixture until CodeB.P3.Open.
 * Closing the page keeps the isolated repository alive; subsystem deinitialization releases it and restores input.
 */
UCLASS()
class DEMO_MAP_API UCodeBP3UIHostSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	bool OpenPage();
	/** Formal product entry; never constructs or resets the development fixture. */
	bool OpenProfilePage(
		demo_map_code_b::FCodeBRepository& Repository,
		const demo_map_code_b::FCodeBP2PlayerLayout& Layout,
		demo_map_code_b::FCodeBP3UIController::FProfileCommit Commit,
		TFunction<void()> OnClosed = nullptr,
		bool bActiveRunPresentation = false,
		const FCodeBP3NormalContainerPresentation* NormalContainerPresentation = nullptr,
		const FCodeBP3BodyContainerPresentation* BodyContainerPresentation = nullptr,
		const FCodeBP3HotbarPresentation* HotbarPresentation = nullptr,
		const FCodeBP3GroundDropPresentation* GroundDropPresentation = nullptr,
		const FCodeBP3WorldDropPresentation* WorldDropPresentation = nullptr);
	bool IsNormalContainerPresentation(const FGuid& ContainerId) const;
	bool HasNormalContainerPresentation() const { return NormalContainerPresentation.IsSet(); }
	bool IsNormalContainerItemHidden(const FGuid& ItemId) const;
	bool IsNormalContainerItemSearching(const FGuid& ItemId) const;
	bool IsNormalContainerSlotProtected(const FGuid& ContainerId, int32 SlotIndex) const;
	bool RequestNormalContainerItemSearch(const FGuid& ContainerId, int32 SlotIndex, FString& OutError);
	void UpdateNormalContainerProjection(const FCodeBNormalContainerProjection& Projection);
	bool IsBodyContainerPresentation(const FGuid& ContainerId) const;
	bool IsBodyEquipmentContainerPresentation(const FGuid& ContainerId) const;
	bool HasBodyContainerPresentation() const { return BodyContainerPresentation.IsSet(); }
	bool IsBodyContainerItemHidden(const FGuid& ItemId) const;
	bool IsBodyContainerItemSearching(const FGuid& ItemId) const;
	bool IsBodyContainerSlotProtected(const FGuid& ContainerId, int32 SlotIndex) const;
	bool RequestBodyContainerItemSearch(const FGuid& ContainerId, int32 SlotIndex, FString& OutError);
	void UpdateBodyContainerProjection(const FCodeBBodyContainerProjection& Projection);
	const FCodeBP3HotbarPresentation* GetHotbarPresentation() const
	{
		return HotbarPresentation.IsSet() ? &HotbarPresentation.GetValue() : nullptr;
	}
	bool HasGroundDropPresentation() const { return GroundDropPresentation.IsSet(); }
	bool RequestGroundDrop(const demo_map_code_b::FCodeBP4DragPayload& Payload, FString& OutError);
	bool IsWorldDropPresentation(const FGuid& ContainerId) const
	{
		return WorldDropPresentation.IsSet() && WorldDropPresentation->TargetContainerId == ContainerId;
	}
	bool RequestHotbarBindFromSelectedItem(int32 SlotIndex, FString& OutError);
	bool RequestHotbarUnbind(int32 SlotIndex, FString& OutError);
	void RefreshHotbarProjection();
	void ClosePage();
	void ResetDevelopmentFixture();
	void RefreshActivePage();
	void SetP4PreviewCapture(const demo_map_code_b::FCodeBP4DragPayload& Payload, const demo_map_code_b::FCodeBP3SlotAddress& Target);
	bool IsHostEnabled() const;
	demo_map_code_b::FCodeBP3UIController* GetController() const { return Controller.Get(); }
	UCodeBP3InventoryWidget* GetActiveWidget() const { return ActiveWidget.Get(); }

private:
	class APlayerController* GetPlayerController() const;
	void CaptureInput(class APlayerController* PlayerController, UCodeBP3InventoryWidget* Widget);
	void RestoreInput(class APlayerController* PlayerController);

	TUniquePtr<demo_map_code_b::FCodeBP3UIController> Controller;
	TWeakObjectPtr<UCodeBP3InventoryWidget> ActiveWidget;
	bool bInputCaptured = false;
	bool bPreviousShowMouseCursor = false;
	TFunction<void()> ProfilePageClosed;
	TOptional<FCodeBP3NormalContainerPresentation> NormalContainerPresentation;
	TOptional<FCodeBP3BodyContainerPresentation> BodyContainerPresentation;
	TOptional<FCodeBP3HotbarPresentation> HotbarPresentation;
	TOptional<FCodeBP3GroundDropPresentation> GroundDropPresentation;
	TOptional<FCodeBP3WorldDropPresentation> WorldDropPresentation;
};
