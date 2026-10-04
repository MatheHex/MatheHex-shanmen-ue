#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShanmenDemo20Widget.generated.h"

class AShanmenDemo20GameMode;
class UBorder;
class UButton;
class UProgressBar;
class UTextBlock;

/** Presentation only: consumes the session, invokes the product host, never mutates vitality. */
UCLASS()
class UShanmenDemo20Widget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializeForDemo(AShanmenDemo20GameMode* InHost);
	void Refresh();
protected:
	virtual void NativeOnInitialized() override;
private:
	void Build();
	UFUNCTION() void Primary();
	UFUNCTION() void Secondary();
	TWeakObjectPtr<AShanmenDemo20GameMode> Host;
	UPROPERTY() TObjectPtr<UBorder> Modal;
	UPROPERTY() TObjectPtr<UProgressBar> Health;
	UPROPERTY() TObjectPtr<UTextBlock> HealthLabel;
	UPROPERTY() TObjectPtr<UTextBlock> Objective;
	UPROPERTY() TObjectPtr<UTextBlock> Notice;
	UPROPERTY() TObjectPtr<UTextBlock> Defense;
	UPROPERTY() TObjectPtr<UTextBlock> Heading;
	UPROPERTY() TObjectPtr<UTextBlock> Body;
	UPROPERTY() TObjectPtr<UTextBlock> PrimaryLabel;
	UPROPERTY() TObjectPtr<UTextBlock> SecondaryLabel;
	UPROPERTY() TObjectPtr<UButton> PrimaryButton;
	UPROPERTY() TObjectPtr<UButton> SecondaryButton;
};
