#include "UI/Combat/CombatActionPaletteWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCombatActionPaletteWidget::UpdateSPUI(float Percent, const FString& Text)
{
    if (PB_SPBar) PB_SPBar->SetPercent(Percent);
    if (Text_SP) Text_SP->SetText(FText::FromString(Text));
}

void UCombatActionPaletteWidget::UpdateHP(float Percent, const FString& Text)
{
    if (PB_HPBar) PB_HPBar->SetPercent(Percent);
    if (Text_HP) Text_HP->SetText(FText::FromString(Text));
}

void UCombatActionPaletteWidget::UpdateAP(float Percent)
{
    if (PB_APBar) PB_APBar->SetPercent(Percent);
}