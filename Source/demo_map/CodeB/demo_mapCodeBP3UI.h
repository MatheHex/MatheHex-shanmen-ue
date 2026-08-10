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
class UScrollBox;
class UVerticalBox;
class UCodeBP3InventoryWidget;

/** P10-only display policy for the existing production P3/P4 Host. */
struct FCodeBP3NormalContainerPresentation
{
	FGuid TargetContainerId;
	FString Title;
	FCodeBNormalContainerProjection Projection;
	/** The manager/service starts the durable search and returns its fresh projection. */
	TFunction<bool(const demo_map_code_b::FCodeBP3SearchLocator&, FCodeBNormalContainerProjection&, FString&)> BeginItemSearch;
};

/** P12-only display policy for the existing production P3/P4 Host. */
struct FCodeBP3BodyContainerPresentation
{
	FGuid TargetContainerId;
	FString Title;
	FCodeBBodyContainerProjection Projection;
	/** P38 transient page/focus lifecycle identity; never persisted as BodyTarget truth. */
	uint32 TargetOpenGeneration = 0;
	/** The manager/service starts the durable body search and returns its fresh projection. */
	TFunction<bool(const demo_map_code_b::FCodeBP3SearchLocator&, FCodeBBodyContainerProjection&, FString&)> BeginItemSearch;
};

/** P13 keeps the P3/P4 host projection-only; all writes enter a Store-owned binding service callback. */
struct FCodeBP3HotbarPresentation
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FCodeBHotbarProjection Projection;
	TFunction<bool(FCodeBHotbarProjection&, FString&)> Refresh;
	TFunction<bool(const FGuid&, int32, FCodeBHotbarProjection&, FString&)> Bind;
	TFunction<bool(int32, FCodeBHotbarProjection&, FString&)> Unbind;
};

/** P23 policy/identity supplied by the product host; no item graph is stored here. */
struct FCodeBP3WorkspacePresentation
{
	demo_map_code_b::FCodeBP3InventoryWorkspaceContext Context;
	TFunction<demo_map_code_b::ECodeBP3WorkspaceWriteGate()> ResolveWriteGate;
	TFunction<int32()> ResolveSessionRevision;
};

/** P14's one actual ground-drop target. It carries no item authority itself. */
struct FCodeBP3GroundDropPresentation
{
	TFunction<bool(const demo_map_code_b::FCodeBP4DragPayload&, FString&)> RequestDrop;
};

/** P31's transient exact-record target. It projects one member of the P6 WorldDrop Registry. */
struct FCodeBP3WorldDropPresentation
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FGuid WorldDropId;
	int32 Ordinal = 0;
	FGuid TargetContainerId;
	FGuid RootItemId;
	FGuid SpatialChildContainerId;
	FName MapRoute = NAME_None;
	int32 RecordRevision = INDEX_NONE;
	/** Read-only durable record family used only by the shared QuickTransfer resolver. */
	FString Provenance;
	uint32 TargetOpenGeneration = 0;
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
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
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
	bool bConsumedQuickTransfer = false;
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
	void HandleCellHover(const demo_map_code_b::FCodeBP3SlotAddress& Address, bool bHovered);
	void HandleQuickTransfer(UCodeBP3CellButton* CellButton);
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
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	enum class EInventorySectionKind : uint8
	{
		Plain,
		NormalTarget,
		BodyTarget
	};

	void BuildLayout();
	void BuildPageContents();
	UVerticalBox* AddPanel(UVerticalBox* Parent, const FString& Title);
	void AddInventorySection(UVerticalBox* Parent, const FString& Title, const demo_map_code_b::FCodeBP2ContainerView& Container, int32 Columns, EInventorySectionKind Kind);
	void AddContainerSection(UVerticalBox* Parent, const FString& Title, const demo_map_code_b::FCodeBP2ContainerView& Container, int32 Columns);
	void AddNormalContainerSection(UVerticalBox* Parent, const demo_map_code_b::FCodeBP2ContainerView& Container);
	void AddBodyContainerSection(UVerticalBox* Parent, const demo_map_code_b::FCodeBP2ContainerView& Container);
	void AddBodyEquipmentContainerSection(UVerticalBox* Parent, const demo_map_code_b::FCodeBP2ContainerView& Container, FName SlotSemantic);
	void AddSpatialContainerSection(UVerticalBox* Parent, const FString& EmptyTitle, const demo_map_code_b::FCodeBP2ContainerView* Container, const demo_map_code_b::FCodeBP2SlotView* ParentSlot);
	void AddHotbarPlaceholders(UVerticalBox* Parent);
	void AddGroundDropZone(UVerticalBox* Parent);
	void AddDetailAndActions(UVerticalBox* Parent);
	void AddP4ContextActions(UVerticalBox* Parent);
	const demo_map_code_b::FCodeBP2ContainerView* FindRole(FName Role) const;
	bool IsSelectedItemInEquipmentSlot() const;
	bool IsSelectedItemEquipable() const;
	const demo_map_code_b::FCodeBP2SlotView* FindSlot(const demo_map_code_b::FCodeBP3SlotAddress& Address) const;
	const demo_map_code_b::FCodeBP2SlotView* FindSpatialParent(const demo_map_code_b::FCodeBP2ContainerView& ChildContainer) const;
	demo_map_code_b::FCodeBP4DropPreview PreviewInventoryTransfer(const demo_map_code_b::FCodeBP4DragPayload& Payload, const demo_map_code_b::FCodeBP3SlotAddress& Target) const;
	bool CommitInventoryTransfer(const demo_map_code_b::FCodeBP4DragPayload& Payload, const demo_map_code_b::FCodeBP3SlotAddress& Target, const TCHAR* InputLabel);
	const demo_map_code_b::FCodeBP2ContainerView* ResolveQuickTransferDestination(const demo_map_code_b::FCodeBP4DragPayload& Payload) const;
	FLinearColor GetNormalCellColor(const demo_map_code_b::FCodeBP3SlotAddress& Address) const;
	demo_map_code_b::FCodeBP4InteractionController* GetP4Controller();

	UFUNCTION()
	void OnEquipGuidanceClicked();
	UFUNCTION()
	void OnUnequipGuidanceClicked();
	UFUNCTION()
	void OnOpenSplitClicked();
	UFUNCTION()
	void OnBeginSplitClicked();
	UFUNCTION()
	void OnCancelSplitClicked();
	UFUNCTION()
	void OnContextDetailClicked();
	UFUNCTION()
	void OnCloseClicked();

	TWeakObjectPtr<class UCodeBP3UIHostSubsystem> Host;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PageContents;
	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> PlayerScrollBox;
	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> TargetScrollBox;
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
	TOptional<demo_map_code_b::FCodeBP3SlotAddress> HoveredAddress;
	float PlayerScrollOffset = 0.0f;
	float TargetScrollOffset = 0.0f;
	int32 ContextMenuRevision = INDEX_NONE;
	uint64 NextP4GestureId = 0;
	uint64 CurrentP4GestureId = 0;
	int32 CurrentP4SubmittedCommandCount = 0;
	int32 CurrentP4P2CallCount = 0;
	int32 CurrentP4RevisionBefore = INDEX_NONE;
	FGuid SplitInputItemId;
	bool bLayoutBuilt = false;
	bool bSplitQuantityInputOpen = false;
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
		const FCodeBP3WorldDropPresentation* WorldDropPresentation = nullptr,
		const FCodeBP3WorkspacePresentation* WorkspacePresentation = nullptr);
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
	bool ResolveExternalCellState(
		const FGuid& ContainerId,
		int32 SlotIndex,
		demo_map_code_b::ECodeBP3CellState& OutState,
		demo_map_code_b::FCodeBP3SearchLocator& OutLocator) const;
	bool IsExternalTargetContainer(const FGuid& ContainerId) const;
	void PopulateAddressContext(demo_map_code_b::FCodeBP3SlotAddress& Address) const;
	void PopulateTransferContext(demo_map_code_b::FCodeBP4DragPayload& Payload) const;
	bool ValidateTransferContext(const demo_map_code_b::FCodeBP4DragPayload& Payload, FString& OutError);
	/** Shared P38 normal-drag / P39 frozen-target source and destination gate. */
	bool ValidateP38BodyEquipmentTransferContext(
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		const demo_map_code_b::FCodeBP3SlotAddress& Target,
		FString& OutError) const;
	bool IsP29PlayerQuickTransferSourceContainer(const FGuid& ContainerId) const;
	/** P35 exact current-child gate; BaseQuick is deliberately not accepted here. */
	bool IsCurrentActiveP17ChildContainer(const FGuid& ContainerId) const;
	/** P37 read-only gate for the accepted P32/P33 provenance family; authority remains in P1/P6. */
	bool IsP34StandardEquipmentWorldDropSource(const FGuid& ContainerId) const;
	/** P35 provenance gate shared by P35 normal Drag and P36's explicitly frozen Ctrl quick pickup. */
	bool IsP35ChildStandardEquipmentWorldDropSource(const FGuid& ContainerId) const;
	bool ValidateWorldDropTransferContext(
		const demo_map_code_b::FCodeBP4DragPayload& Payload,
		const demo_map_code_b::FCodeBP3SlotAddress& Target,
		FString& OutError) const;
	bool CreateSplitDraft(const demo_map_code_b::FCodeBP3SlotAddress& Source, int32 RequestedQuantity, FString& OutError);
	bool BeginInventoryDrag(
		demo_map_code_b::FCodeBP4InteractionController& Interaction,
		const demo_map_code_b::FCodeBP3SlotAddress& Source,
		demo_map_code_b::FCodeBP4DragPayload& OutPayload,
		FString& OutError);
	bool CancelSplitDraft(const FString& Reason = TEXT("拆分草稿已取消，未写入物品状态"));
	const demo_map_code_b::FCodeBP3SplitDraft* GetSplitDraft() const
	{
		return SplitDraft.IsSet() ? &SplitDraft.GetValue() : nullptr;
	}
	bool CanWriteWorkspace(FString& OutError);
	bool IsOutOfRaidWorkspace() const;
	bool ActivateQuickTransferDestination(const demo_map_code_b::FCodeBP3SlotAddress& Address);
	void UpdateWorkspaceHover(const demo_map_code_b::FCodeBP3SlotAddress& Address, bool bHovered);
	void UpdateWorkspaceSelection(const demo_map_code_b::FCodeBP3SlotAddress& Address);
	void UpdateWorkspaceScroll(float PlayerOffset, float TargetOffset);
	void ClearWorkspaceTransientState();
	const demo_map_code_b::FCodeBP3InventoryWorkspaceContext* GetWorkspaceContext() const
	{
		return WorkspacePresentation.IsSet() ? &WorkspacePresentation->Context : nullptr;
	}
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
	bool HasWorldDropPresentation() const { return WorldDropPresentation.IsSet(); }
	bool RequestHotbarBindFromAddress(const demo_map_code_b::FCodeBP3SlotAddress& Address, int32 SlotIndex, FString& OutError);
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
	TOptional<FCodeBP3WorkspacePresentation> WorkspacePresentation;
	TOptional<demo_map_code_b::FCodeBP3SplitDraft> SplitDraft;
	uint32 NextActiveDestinationOpenGeneration = 1;
	uint32 NextBodyTargetOpenGeneration = 1;
};
