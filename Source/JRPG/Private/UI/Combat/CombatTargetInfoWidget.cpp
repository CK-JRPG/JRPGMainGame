#include "UI/Combat/CombatTargetInfoWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Groggy/GroggyComponent.h"

void UCombatTargetInfoWidget::SetTarget(AActor* TargetActor)
{
    NativeDestruct();

    if (!TargetActor)
    {
       SetVisibility(ESlateVisibility::Hidden);
       return;
    }
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (Text_TargetName)
    {
       Text_TargetName->SetText(FText::FromString(TargetActor->GetName()));
    }

    if (UHPComponent* HPComp = TargetActor->FindComponentByClass<UHPComponent>())
    {
       CachedHPComp = HPComp;
       HPComp->OnHPChanged.AddUObject(this, &UCombatTargetInfoWidget::OnTargetHPChanged);
        
       OnTargetHPChanged(HPComp->GetHP(), HPComp->GetHP(), NAME_None);
    }

    if (UGroggyComponent* GroggyComp = TargetActor->FindComponentByClass<UGroggyComponent>())
    {
       CachedGroggyComp = GroggyComp;
       GroggyComp->OnGroggyStateChanged.AddUObject(this, &UCombatTargetInfoWidget::OnTargetGroggyChanged);
        
       bool bInitialGroggyState = false; 
       
       OnTargetGroggyChanged(bInitialGroggyState);
    }
}

void UCombatTargetInfoWidget::NativeDestruct()
{
    if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
    if (CachedGroggyComp.IsValid()) CachedGroggyComp->OnGroggyStateChanged.RemoveAll(this);
    Super::NativeDestruct();
}

void UCombatTargetInfoWidget::OnTargetHPChanged(float OldHP, float NewHP, FName Reason)
{
    if (CachedHPComp.IsValid() && PB_TargetHP && CachedHPComp->MaxHP > 0.f)
    {
        PB_TargetHP->SetPercent(NewHP / CachedHPComp->MaxHP);
    }
}

void UCombatTargetInfoWidget::OnTargetGroggyChanged(bool bGroggy)
{
    if (PB_GroggyGauge)
    {
        PB_GroggyGauge->SetPercent(bGroggy ? 1.0f : 0.0f);
    }
}