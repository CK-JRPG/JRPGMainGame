#include "UI/Combat/CombatTargetInfoWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UCombatTargetInfoWidget::UpdateTargetName(const FString& Name) {
    if (Text_TargetName) Text_TargetName->SetText(FText::FromString(Name));
}
void UCombatTargetInfoWidget::UpdateTargetHP(float Percent) {
    if (PB_TargetHP) PB_TargetHP->SetPercent(Percent);
}
void UCombatTargetInfoWidget::UpdateGroggyState(bool bGroggy) {
    if (PB_GroggyGauge) PB_GroggyGauge->SetPercent(bGroggy ? 1.0f : 0.0f);
}