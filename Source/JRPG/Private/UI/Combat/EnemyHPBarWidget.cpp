#include "UI/Combat/EnemyHPBarWidget.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"

void UEnemyHPBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PB_HPBar) TargetHPPercent = PB_HPBar->GetPercent();
    if (PB_CatchUpBar) CurrentCatchUpPercent = PB_CatchUpBar->GetPercent();
}

void UEnemyHPBarWidget::UpdateHP(float Percent) 
{
	if (PB_HPBar)
	{
		PB_HPBar->SetPercent(Percent);
	}

	TargetHPPercent = Percent;

	if (Percent >= CurrentCatchUpPercent)
	{
		CurrentCatchUpPercent = Percent;
		if (PB_CatchUpBar) PB_CatchUpBar->SetPercent(CurrentCatchUpPercent);

		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CatchUpTimerHandle);
		return;
	}

	if (GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(CatchUpTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			CatchUpTimerHandle,
			this,
			&UEnemyHPBarWidget::OnCatchUpTimerTick,
			UpdateInterval,
			true
		);
	}
}

void UEnemyHPBarWidget::OnCatchUpTimerTick()
{
	CurrentCatchUpPercent = FMath::FInterpTo(CurrentCatchUpPercent, TargetHPPercent, UpdateInterval, CatchUpSpeed);

	if (PB_CatchUpBar)
	{
		PB_CatchUpBar->SetPercent(CurrentCatchUpPercent);
	}

	if (FMath::IsNearlyEqual(CurrentCatchUpPercent, TargetHPPercent, 0.001f))
	{
		CurrentCatchUpPercent = TargetHPPercent;
		if (PB_CatchUpBar) PB_CatchUpBar->SetPercent(CurrentCatchUpPercent);

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(CatchUpTimerHandle);
		}
	}
}
