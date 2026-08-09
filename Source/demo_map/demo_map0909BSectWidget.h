#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "demo_map0909BFrameworkTypes.h"
#include "demo_map0909BSectWidget.generated.h"

class Ademo_map0909BFrameworkHost;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/** New default AtSect presentation. It routes only through the 0.0.9B host. */
UCLASS()
class Udemo_map0909BSectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForFramework(Ademo_map0909BFrameworkHost* InHost);
	void RefreshPresentation(
		Edemo_map0909BTopState InState,
		const FString& InFeedback,
		const Fdemo_map0909BStartDiagnostic& InDiagnostic);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildInterface();
	void RefreshText();
	UButton* AddActionButton(const FString& Label);

	UFUNCTION()
	void ClickStartM01();
	UFUNCTION()
	void ClickOpenWarehouse();

	TWeakObjectPtr<Ademo_map0909BFrameworkHost> FrameworkHost;
	Edemo_map0909BTopState State = Edemo_map0909BTopState::AtSect;
	FString Feedback;
	Fdemo_map0909BStartDiagnostic Diagnostic;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Column;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FeedbackText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DiagnosticText;
	UPROPERTY(Transient) TObjectPtr<UButton> StartM01Button;
	UPROPERTY(Transient) TObjectPtr<UButton> WarehouseButton;
	bool bBuilt = false;
};
