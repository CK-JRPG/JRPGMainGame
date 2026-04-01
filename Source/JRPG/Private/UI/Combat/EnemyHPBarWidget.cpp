#include "UI/Combat/EnemyHPBarWidget.h"

#include "Components/ProgressBar.h"
#include "Combat/Stats/HPComponent.h"

void UEnemyHPBarWidget::BindHPComponent(UHPComponent* InHPComp)
{
	if (!InHPComp || !PB_HPBar) return;

	CachedHPComp = InHPComp;

	InHPComp->OnHPChanged.AddUObject(this, &UEnemyHPBarWidget::OnHPChanged);

	OnHPChanged(InHPComp->GetHP(), InHPComp->GetHP(), NAME_None);
}

void UEnemyHPBarWidget::NativeDestruct()
{
	if (CachedHPComp.IsValid())
	{
		CachedHPComp->OnHPChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UEnemyHPBarWidget::OnHPChanged(float OldHP, float NewHP, FName Reason)
{
	if (CachedHPComp.IsValid() && PB_HPBar)
	{
		float MaxHP = CachedHPComp->MaxHP;
		if (MaxHP > 0.f)
		{
			PB_HPBar->SetPercent(NewHP / MaxHP);
		}
	}
}