#include "UI/MainMenu/MainMenuUIWidget.h"
#include "Components/WidgetSwitcher.h"

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
