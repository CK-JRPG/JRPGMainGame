#include "UI/MainMenu/MainMenuUIWidget.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Restart)
	{
		Button_Restart->OnClicked.RemoveAll(this);
		Button_Restart->OnClicked.AddDynamic(this, &UMainMenuUIWidget::RestartCurrentLevel);
	}
}

void UMainMenuUIWidget::NativeDestruct()
{
	if (Button_Restart)
	{
		Button_Restart->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UMainMenuUIWidget::SetActiveTab(EMainMenuTab Tab)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	}
}

void UMainMenuUIWidget::RequestTabChanged(EMainMenuTab Tab)
{
	OnTabChanged.Broadcast(Tab);
}

void UMainMenuUIWidget::RestartCurrentLevel()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			PC->SetPause(false);
		}

		const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(World, true));
		UGameplayStatics::OpenLevel(World, CurrentLevelName);
	}
}
