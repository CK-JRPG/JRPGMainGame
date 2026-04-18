#include "UI/Presenters/MainMenuPresenter.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuPresenter::Initialize(UWorld* World, TSubclassOf<UMainMenuUIWidget> WidgetClass)
{
	if (!World || !WidgetClass) return;

	MainMenuWidget = CreateWidget<UMainMenuUIWidget>(World, WidgetClass);
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(100);
		MainMenuWidget->SetVisibility(ESlateVisibility::Hidden);
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
	bIsOpen ? CloseMenu() : OpenMenu();
}

void UMainMenuPresenter::OpenMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::OpenMenu()"));
	if (!MainMenuWidget || bIsOpen) return;
	UE_LOG(LogTemp, Warning, TEXT("MainMenuPresenter::OpenMenu()"));
	bIsOpen = true;
	MainMenuWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	HandleTabChanged(EMainMenuTab::Map);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(MainMenuWidget->GetWorld(), 0))
	{
		PC->SetPause(true);
		PC->SetInputMode(FInputModeGameAndUI());
		PC->bShowMouseCursor = true;
	}
}

void UMainMenuPresenter::CloseMenu()
{
	if (!MainMenuWidget || !bIsOpen) return;
	bIsOpen = false;
	MainMenuWidget->SetVisibility(ESlateVisibility::Hidden);

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
