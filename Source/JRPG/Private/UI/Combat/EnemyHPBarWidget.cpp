#include "UI/Combat/EnemyHPBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHPBarWidget::UpdateHP(float Percent)
{
	if (PB_HPBar) PB_HPBar->SetPercent(Percent);
}
