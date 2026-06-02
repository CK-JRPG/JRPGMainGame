#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UCombatPartySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (PB_HPBar) TargetHPPercent = PB_HPBar->GetPercent();
    if (PB_HPCatchUpBar) CurrentCatchUpPercent = PB_HPCatchUpBar->GetPercent();
}

void UCombatPartySlotWidget::UpdateName(const FString& Name) {
    if (Text_Name) Text_Name->SetText(FText::FromString(Name));
}

void UCombatPartySlotWidget::UpdateHP(float Percent, const FString& Text) {
	if (PB_HPBar->GetPercent() > Percent)
	{
		// 피해를 입은 경우
		PB_HPCatchUpBar->SetFillColorAndOpacity(FLinearColor::Red);

		if (PB_HPBar) PB_HPBar->SetPercent(Percent);

		TargetHPPercent = Percent;

		if (Percent >= CurrentCatchUpPercent)
		{
			CurrentCatchUpPercent = Percent;
			if (PB_HPCatchUpBar) PB_HPCatchUpBar->SetPercent(CurrentCatchUpPercent);

			if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CatchUpTimerHandle);
			return;
		}

		if (GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(CatchUpTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(
				CatchUpTimerHandle,
				this,
				&UCombatPartySlotWidget::OnCatchUpDamageTimerTick,
				UpdateInterval,
				true
			);
		}
	}
	else if(PB_HPBar->GetPercent() < Percent)
	{
		// 힐을 받은 경우
		PB_HPCatchUpBar->SetFillColorAndOpacity(FLinearColor::Red);

		if (PB_HPCatchUpBar) PB_HPCatchUpBar->SetPercent(Percent);

		TargetHPPercent = Percent;

		if (Percent >= CurrentCatchUpPercent)
		{
			CurrentCatchUpPercent = Percent;
			if (PB_HPBar) PB_HPBar->SetPercent(CurrentCatchUpPercent);

			if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CatchUpTimerHandle);
			return;
		}

		if (GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(CatchUpTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(
				CatchUpTimerHandle,
				this,
				&UCombatPartySlotWidget::OnCatchUpHealTimerTick,
				UpdateInterval,
				true
			);
		}
	}

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

void UCombatPartySlotWidget::OnCatchUpDamageTimerTick()
{
	CurrentCatchUpPercent = FMath::FInterpTo(CurrentCatchUpPercent, TargetHPPercent, UpdateInterval, CatchUpSpeed);

	if (PB_HPCatchUpBar)
	{
		PB_HPCatchUpBar->SetPercent(CurrentCatchUpPercent);
	}

	if (FMath::IsNearlyEqual(CurrentCatchUpPercent, TargetHPPercent, 0.001f))
	{
		CurrentCatchUpPercent = TargetHPPercent;
		if (PB_HPCatchUpBar) PB_HPCatchUpBar->SetPercent(CurrentCatchUpPercent);

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(CatchUpTimerHandle);
		}
	}
}

void UCombatPartySlotWidget::OnCatchUpHealTimerTick()
{
	CurrentCatchUpPercent = FMath::FInterpTo(CurrentCatchUpPercent, TargetHPPercent, UpdateInterval, CatchUpSpeed);

	if (PB_HPBar)
	{
		PB_HPBar->SetPercent(CurrentCatchUpPercent);
	}

	if (FMath::IsNearlyEqual(CurrentCatchUpPercent, TargetHPPercent, 0.001f))
	{
		CurrentCatchUpPercent = TargetHPPercent;
		if (PB_HPBar) PB_HPBar->SetPercent(CurrentCatchUpPercent);

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(CatchUpTimerHandle);
		}
	}
}