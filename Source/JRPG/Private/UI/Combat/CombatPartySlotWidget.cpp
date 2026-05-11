#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UCombatPartySlotWidget::UpdateName(const FString& Name) {
    if (Text_Name) Text_Name->SetText(FText::FromString(Name));
}
void UCombatPartySlotWidget::UpdateHP(float Percent, const FString& Text) {
    if (PB_HPBar) PB_HPBar->SetPercent(Percent);
    if (Text_HP) Text_HP->SetText(FText::FromString(Text));
}
void UCombatPartySlotWidget::UpdateAP(float Percent) {
    if (PB_APBar) PB_APBar->SetPercent(Percent);
}

void UCombatPartySlotWidget::SetIsActiveCharacter(bool bIsActive)
{
    float ScaleValue = bIsActive ? 1.04f : 0.8f;
    SetRenderScale(FVector2D(ScaleValue, ScaleValue));
}
