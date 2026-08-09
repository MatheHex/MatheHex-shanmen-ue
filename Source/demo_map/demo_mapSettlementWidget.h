#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "demo_mapItemTypes.h"
#include "demo_mapSettlementWidget.generated.h"

class UTextBlock;
class UButton;
class UScrollBox;
class Ademo_mapV3ProgressionManager;

/** Read-only value snapshot for one terminal V3 run event. */
UCLASS()
class Udemo_mapSettlementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForManager(Ademo_mapV3ProgressionManager* InManager);
	void SetSummary(
		const Fdemo_mapSettlementSummary& InSummary,
		FGuid InSettlementId = FGuid());
	const Fdemo_mapSettlementSummary& GetSummary() const { return Summary; }
	FGuid GetSettlementId() const { return SettlementId; }
	bool HasRenderedSummary() const { return bBuilt && Summary.bValid; }
#if !UE_BUILD_SHIPPING
	bool AutomationValidateContinuePath(FString& OutDiagnostic) const;
	bool AutomationClickContinue();
	bool AutomationPressBack(const FKey& Key);
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void HandleContinueClicked();
	bool DismissForNavigationKey(const FKey& Key);
	void BuildInterface();
	void RefreshText();
	Fdemo_mapSettlementSummary Summary;
	FGuid SettlementId;
	TWeakObjectPtr<Ademo_mapV3ProgressionManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RowsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TotalsText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> RowsScroll;
	UPROPERTY(Transient) TObjectPtr<UButton> ContinueButton;
	bool bBuilt = false;
};
