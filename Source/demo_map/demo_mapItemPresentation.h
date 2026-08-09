#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemUseTypes.h"
#include "demo_mapPersistentWarehouseTransaction.h"
#include "demo_mapItemPresentation.generated.h"

class UButton;
class UTextBlock;
class UDragDropOperation;

enum class Edemo_mapItemPresentationContext : uint8
{
	Warehouse,
	TeleportEquipment,
	TeleportQuickItems,
	TeleportSpatialStorage,
	TeleportWarehouse,
	RuntimeEquipment,
	RuntimeBaseQuickItems,
	RuntimeRingQuickItems,
	RuntimeSpatialStorage,
	/** P3: identified cells belonging to the active Search Container. */
	SearchContainerGrid,
	SearchContainerEquipment,
	SearchContainerBaseQuickItems,
	SearchContainerSpatialStorage,
	SearchContainerBody,
	/** P3: a non-owning Runtime UI target that projects a player item to World. */
	RuntimeWorldDrop
};

enum class Edemo_mapItemContextAction : uint8
{
	ViewDetails,
	Equip,
	Unequip,
	Move,
	ReturnToWarehouse,
	EquipToHotbar,
	MoveToBaseQuickItems,
	MoveToSpatialStorage,
	Use
};

enum class Edemo_mapHotbarSlotState : uint8
{
	Empty,
	Ready,
	Unavailable,
	Cooldown,
	InvalidBinding
};

struct Fdemo_mapHotbarSlotView
{
	int32 SlotNumber = INDEX_NONE;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FString IconLabel;
	FString StateLabel;
	int32 Quantity = 0;
	float CooldownRemaining = 0.0f;
	Edemo_mapHotbarSlotState State =
		Edemo_mapHotbarSlotState::Empty;

	bool IsOccupied() const
	{
		return ItemInstanceId.IsValid();
	}
};

struct Fdemo_mapUnifiedItemCellView
{
	int32 SlotIndex = INDEX_NONE;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FString IconLabel;
	FString LevelLabel;
	FString QualityLabel;
	int32 Quantity = 0;
	bool bOccupied = false;
	bool bSelected = false;
	bool bUnavailable = false;
	/** A cell may be clickable for search while remaining ineligible as a drag source. */
	bool bAllowDrag = true;
	bool bSearchMatch = true;
	bool bDragSource = false;
	bool bDragTarget = false;
	/** A drag is hovering an illegal destination; source stays authoritative. */
	bool bDragRejected = false;
	bool bJackpotOrSpecial = false;
	FString ShortcutLabel;
};

struct Fdemo_mapUnifiedItemDetailView
{
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FString DisplayName;
	FString IconLabel;
	FString LevelLabel;
	FString TypeLabel;
	FString QualityLabel;
	int32 Quantity = 0;
	FString Description;
	FString BaseAttributes;
	FString RandomAffixes;
	FString UseEffect;
	FString SellValue;
	FString AllowedPositions;
	FString TagsAndStatus;
	bool bValid = false;
};

struct Fdemo_mapResolvedItemActions
{
	TArray<Edemo_mapItemContextAction> Actions;

	bool Contains(Edemo_mapItemContextAction Action) const
	{
		return Actions.Contains(Action);
	}
};

struct Fdemo_mapWarehouseView
{
	static constexpr int32 CurrentLevel = 1;
	static constexpr int32 CurrentCapacity =
		Fdemo_mapPersistentWarehouseLayout::SlotCount;
	static constexpr int32 NextLevelCapacity = 42;

	TArray<Fdemo_mapUnifiedItemCellView> Slots;
	int32 UsedSlots = 0;
	int32 RemainingSlots = 0;
	FString NextLevelRequirement =
		TEXT("等待后续经济与建筑升级任务接入");
	bool bValid = false;
	FString Diagnostic;
};

struct Fdemo_mapItemPresentation
{
	static Fdemo_mapWarehouseView BuildWarehouseView(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot);
	static Fdemo_mapUnifiedItemCellView BuildCell(
		const Fdemo_mapProfilePreparationStashRow* Row,
		int32 SlotIndex);
	static Fdemo_mapUnifiedItemCellView BuildCell(
		const Fdemo_mapEntityItemSlotView& Cell);
	static Fdemo_mapUnifiedItemDetailView BuildDetail(
		const Fdemo_mapProfilePreparationStashRow& Row);
	static Fdemo_mapUnifiedItemDetailView BuildDetail(
		const Fdemo_mapItemInstance& Item);
	static Fdemo_mapResolvedItemActions ResolveActions(
		const Fdemo_mapProfilePreparationStashRow& Row,
		Edemo_mapItemPresentationContext Context);
	static Fdemo_mapResolvedItemActions ResolveRuntimeActions(
		const Fdemo_mapItemInstance& Item,
		Edemo_mapItemPresentationContext Context);
	static TArray<Fdemo_mapHotbarSlotView> BuildPreparationHotbar(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot);
	static TArray<Fdemo_mapHotbarSlotView> BuildRuntimeHotbar(
		const Fdemo_mapHotbarBindingSnapshot& Bindings,
		const Fdemo_mapItemAuthority& Authority,
		const Fdemo_mapItemUseCooldownSnapshot& Cooldown);
	static FString BuildHotbarSlotLabel(
		const Fdemo_mapHotbarSlotView& Slot,
		const FString& KeyLabel);
	static FString ActionLabel(Edemo_mapItemContextAction Action);
};

DECLARE_DELEGATE_OneParam(Fdemo_mapItemCellActivated, int32);
DECLARE_DELEGATE_FiveParams(
	Fdemo_mapItemCellDropped,
	int32 /*SourceSlotIndex*/,
	FGuid /*SourceItemId*/,
	Edemo_mapItemPresentationContext /*SourceContext*/,
	int32 /*TargetSlotIndex*/,
	Edemo_mapItemPresentationContext /*TargetContext*/);
DECLARE_DELEGATE_FourParams(
	Fdemo_mapItemCellContextRequested,
	int32 /*SourceSlotIndex*/,
	FGuid /*SourceItemId*/,
	Edemo_mapItemPresentationContext /*SourceContext*/,
	FVector2D /*ScreenPosition*/);
DECLARE_DELEGATE_OneParam(Fdemo_mapItemCellDoubleClicked, int32);

/** Native UMG transport only.  It carries identity, never an item copy. */
UCLASS()
class Udemo_mapItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	int32 SourceSlotIndex = INDEX_NONE;
	FGuid SourceItemInstanceId;
	Edemo_mapItemPresentationContext SourceContext =
		Edemo_mapItemPresentationContext::Warehouse;
};

/** Reusable display/input cell. It forwards a slot index and owns no item rule. */
UCLASS()
class Udemo_mapItemCellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeCell(
		const Fdemo_mapUnifiedItemCellView& InView,
		Fdemo_mapItemCellActivated InActivated,
		Edemo_mapItemPresentationContext InContext =
			Edemo_mapItemPresentationContext::Warehouse,
		Fdemo_mapItemCellDropped InDropped = Fdemo_mapItemCellDropped(),
		Fdemo_mapItemCellContextRequested InContextRequested =
			Fdemo_mapItemCellContextRequested(),
		Fdemo_mapItemCellDoubleClicked InDoubleClicked =
			Fdemo_mapItemCellDoubleClicked(),
		bool bInOpenNestedContainerOnClick = false);
	const Fdemo_mapUnifiedItemCellView& GetCellView() const
	{
		return View;
	}

protected:
	virtual void NativeOnInitialized() override;
	/**
	 * UButton consumes ordinary mouse-down before the outer UUserWidget sees it.
	 * Preview is therefore the only reliable real-mouse drag capture point.
	 */
	virtual FReply NativeOnPreviewMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

private:
	void BuildInterface();
	void RefreshText();
	bool CanAcceptDrop(const Udemo_mapItemDragDropOperation* Operation) const;

	UFUNCTION()
	void ClickCell();

	Fdemo_mapUnifiedItemCellView View;
	Fdemo_mapItemCellActivated Activated;
	Fdemo_mapItemCellDropped Dropped;
	Fdemo_mapItemCellContextRequested ContextRequested;
	Fdemo_mapItemCellDoubleClicked DoubleClicked;
	bool bOpenNestedContainerOnClick = false;
	Edemo_mapItemPresentationContext Context =
		Edemo_mapItemPresentationContext::Warehouse;
	UPROPERTY(Transient) TObjectPtr<UButton> CellButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MetaText;
	double LastLeftPressSeconds = -1.0;
	FVector2D LastLeftPressScreenPosition = FVector2D::ZeroVector;
	bool bBuilt = false;
};
