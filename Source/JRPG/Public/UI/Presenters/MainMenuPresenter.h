#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UI/MainMenu/MainMenuUIWidget.h"
#include "MainMenuPresenter.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMenuTabSelected, EMainMenuTab);

UCLASS()
class JRPG_API UMainMenuPresenter : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(UWorld* World, TSubclassOf<UMainMenuUIWidget> WidgetClass);
	void Shutdown();

	void ToggleMenu();
	void OpenMenu();
	void CloseMenu();
	
	bool IsMenuOpen() const { return bIsOpen; }
	UMainMenuUIWidget* GetWidget() const { return MainMenuWidget; }

	FOnMenuTabSelected OnTabSelected;

private:
	UPROPERTY() TObjectPtr<UMainMenuUIWidget> MainMenuWidget;
	bool bIsOpen = false;

	void HandleTabChanged(EMainMenuTab Tab);
};
