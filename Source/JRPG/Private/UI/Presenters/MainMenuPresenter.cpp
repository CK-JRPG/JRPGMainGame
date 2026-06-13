#include "UI/Presenters/MainMenuPresenter.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuPresenter::Initialize(UWorld* World, TSubclassOf<UMainMenuUIWidget> WidgetClass)
{
	if (!World || !WidgetClass) return;

	MainMenuWidget = CreateWidget<UMainMenuUIWidget>(World, WidgetClass);
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(100);
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		MainMenuWidget->OnTabChanged.AddUObject(this, &UMainMenuPresenter::HandleTabChanged);
	}
}

void UMainMenuPresenter::Shutdown()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
}

void UMainMenuPresenter::ToggleMenu()
{
	//UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::ToggleMenu"));
	bIsOpen ? CloseMenu() : OpenMenu();
}

void UMainMenuPresenter::OpenMenu()
{
	//UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::OpenMenu : Try Open Main Menu"));
	if (!MainMenuWidget || bIsOpen) return;
	bIsOpen = true;
	MainMenuWidget->SetVisibility(ESlateVisibility::Visible);

	HandleTabChanged(EMainMenuTab::Map);
	//UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::OpenMenu : Open Main Menu"));

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(MainMenuWidget->GetWorld(), 0))
	{
		PC->SetPause(true);
		PC->SetInputMode(FInputModeGameAndUI());
		PC->bShowMouseCursor = true;
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::OpenMenu : PlayerController is Not Invalid"));
	}
}

void UMainMenuPresenter::CloseMenu()
{
	//UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::OpenMenu : Try Close Main Menu"));
	if (!MainMenuWidget || !bIsOpen) return;
	bIsOpen = false;
	MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(MainMenuWidget->GetWorld(), 0))
	{
		PC->SetPause(false);
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void UMainMenuPresenter::HandleTabChanged(EMainMenuTab Tab)
{
	if (MainMenuWidget) MainMenuWidget->SetActiveTab(Tab);
	OnTabSelected.Broadcast(Tab);
}
