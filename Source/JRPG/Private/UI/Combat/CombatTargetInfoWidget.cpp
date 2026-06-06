#include "UI/Combat/CombatTargetInfoWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"

void UCombatTargetInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PB_TargetHP) TargetHPPercent = PB_TargetHP->GetPercent();
    if (PB_CatchUpBar) CurrentCatchUpPercent = PB_CatchUpBar->GetPercent();
}

void UCombatTargetInfoWidget::UpdateTargetName(const FString& Name) {
    if (Text_TargetName) Text_TargetName->SetText(FText::FromString(Name));
}

void UCombatTargetInfoWidget::UpdateTargetHP(float Percent) 
{
    if (PB_TargetHP) PB_TargetHP->SetPercent(Percent);

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
			&UCombatTargetInfoWidget::OnCatchUpTimerTick,
			UpdateInterval,
			true
		);
	}

}

//void UCombatTargetInfoWidget::UpdateGroggyState(bool bGroggy) {
//    if (PB_GroggyGauge) PB_GroggyGauge->SetPercent(bGroggy ? 1.0f : 0.0f);
//}

void UCombatTargetInfoWidget::OnCatchUpTimerTick()
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
