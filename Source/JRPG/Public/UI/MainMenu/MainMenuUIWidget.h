#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuUIWidget.generated.h"

class UWidgetSwitcher;
class UInventoryUIWidget;
class UButton;

UENUM(BlueprintType)
enum class EMainMenuTab : uint8
{
	Map = 0,
	CharacterInfo = 1,
	Inventory = 2,
	Settings = 3
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMainMenuTabChanged, EMainMenuTab);

UCLASS()
class JRPG_API UMainMenuUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void SetActiveTab(EMainMenuTab Tab);

	UFUNCTION(BlueprintCallable, Category = "JRPG|MainMenu")
	void RequestTabChanged(EMainMenuTab Tab);

	UFUNCTION(BlueprintCallable, Category = "JRPG|MainMenu")
	void RestartCurrentLevel();

	FOnMainMenuTabChanged OnTabChanged;

	UInventoryUIWidget* GetInventoryWidget() const { return Widget_Inventory; }
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInventoryUIWidget> Widget_Inventory;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Restart;
};
