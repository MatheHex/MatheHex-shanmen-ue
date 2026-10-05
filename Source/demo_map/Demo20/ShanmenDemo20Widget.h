#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShanmenDemo20Widget.generated.h"

class AShanmenDemo20GameMode;
class UBorder;
class UButton;
class UProgressBar;
class UTextBlock;
class UShanmenDemo20InventoryWidget;

/** Presentation only: consumes the session, invokes the product host, never mutates vitality. */
UCLASS()
class UShanmenDemo20Widget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeForDemo(AShanmenDemo20GameMode* InHost);
	void Refresh();
	void RefreshInventory();
	void FocusActiveSurface();
protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry&, const FKeyEvent&) override;
private:
	void Build();
	UFUNCTION() void Primary();
	UFUNCTION() void Secondary();
	UFUNCTION() void Inventory();
	TWeakObjectPtr<AShanmenDemo20GameMode> Host;
	UPROPERTY() TObjectPtr<UBorder> Modal;
	UPROPERTY() TObjectPtr<UProgressBar> Health;
	UPROPERTY() TObjectPtr<UTextBlock> HealthLabel;
	UPROPERTY() TObjectPtr<UTextBlock> Objective;
	UPROPERTY() TObjectPtr<UTextBlock> Notice;
	UPROPERTY() TObjectPtr<UTextBlock> Defense;
	UPROPERTY() TObjectPtr<UTextBlock> Medicine;
	UPROPERTY() TObjectPtr<UTextBlock> Heading;
	UPROPERTY() TObjectPtr<UTextBlock> Body;
	UPROPERTY() TObjectPtr<UTextBlock> PrimaryLabel;
	UPROPERTY() TObjectPtr<UTextBlock> SecondaryLabel;
	UPROPERTY() TObjectPtr<UButton> PrimaryButton;
	UPROPERTY() TObjectPtr<UButton> SecondaryButton;
	UPROPERTY() TObjectPtr<UButton> InventoryButton;
	UPROPERTY() TObjectPtr<UBorder> InventorySurface;
	UPROPERTY() TObjectPtr<UShanmenDemo20InventoryWidget> InventoryView;
};
