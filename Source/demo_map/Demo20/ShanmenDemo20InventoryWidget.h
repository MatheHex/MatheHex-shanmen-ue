#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShanmenItemTypes.h"
#include "ShanmenDemo20InventoryWidget.generated.h"
class AShanmenDemo20GameMode;

/** Read-only authority projection; edits are durable host intents, never local quantity writes. */
UCLASS()
class UShanmenDemo20InventoryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeForDemo(AShanmenDemo20GameMode* InHost);
	void RefreshProjection();
protected:
	virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&,
		int32, const FWidgetStyle&, bool) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseMove(const FGeometry&, const FPointerEvent&) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent&) override;
private:
	struct FBoard { FGuid Id; FVector2D Origin; int32 Width = 1; int32 Height = 1; bool Equipment = false; };
	TArray<FBoard> Boards() const;
	const FShanmenItemInstance* HitItem(FVector2D Point) const;
	const FBoard* HitBoard(FVector2D Point, const TArray<FBoard>& List) const;
	FShanmenItemGridRequest Request(FGuid ItemId) const;
	bool DropIntent(const FBoard& Board, FVector2D Point, FShanmenItemGridRequest& Intent) const;
	void UpdateDragPreview();
	void Submit(FShanmenItemGridRequest Intent);
	void Toolbar(int32 Index);
	bool FirstFit(FShanmenItemGridRequest& Intent, FGuid ContainerId, bool IgnoreOriginal = true) const;
	TWeakObjectPtr<AShanmenDemo20GameMode> Host;
	FShanmenItemAuthoritySnapshot Projection;
	FGuid Selected;
	FVector2D Cursor;
	FVector2D DragStart;
	FVector2D GrabOffset;
	bool Dragging = false;
	bool Rotated = false;
	bool PreviewValid = false;
	FString Feedback;
	FString LoadoutSummary;
};
