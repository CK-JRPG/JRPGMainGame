#include "UI/Combat/CombatActionPaletteWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

void UCombatActionPaletteWidget::BindPlayerCharacter(AActor* PlayerActor)
{
    if (!PlayerActor) return;

    if (UHPComponent* HPComp = PlayerActor->FindComponentByClass<UHPComponent>())
    {
        CachedHPComp = HPComp;
        HPComp->OnHPChanged.AddUObject(this, &UCombatActionPaletteWidget::OnPlayerHPChanged);
        
        OnPlayerHPChanged(HPComp->GetHP(), HPComp->GetHP(), NAME_None);
    }

    if (UAPComponent* APComp = PlayerActor->FindComponentByClass<UAPComponent>())
    {
        CachedAPComp = APComp;
        APComp->OnAPChanged.AddUObject(this, &UCombatActionPaletteWidget::OnPlayerAPChanged);
        OnPlayerAPChanged(APComp->GetAP(), APComp->GetAP(), NAME_None);
    }

    if (USPComponent* SPComp = PlayerActor->FindComponentByClass<USPComponent>())
    {
        CachedSPComp = SPComp;
        SPComp->OnSPChanged.AddUObject(this, &UCombatActionPaletteWidget::OnPlayerSPChanged);
        OnPlayerSPChanged(SPComp->GetSP(), SPComp->GetSP(), NAME_None);
    }
}

void UCombatActionPaletteWidget::NativeDestruct()
{
    if (CachedHPComp.IsValid()) CachedHPComp->OnHPChanged.RemoveAll(this);
    if (CachedAPComp.IsValid()) CachedAPComp->OnAPChanged.RemoveAll(this);
    if (CachedSPComp.IsValid()) CachedSPComp->OnSPChanged.RemoveAll(this);
    Super::NativeDestruct();
}

void UCombatActionPaletteWidget::OnPlayerHPChanged(float OldHP, float NewHP, FName Reason)
{
    if (CachedHPComp.IsValid() && PB_HPBar && Text_HP)
    {
        float MaxHP = CachedHPComp->MaxHP;
        if (MaxHP > 0.f) PB_HPBar->SetPercent(NewHP / MaxHP);
        Text_HP->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(NewHP), FMath::RoundToInt(MaxHP))));
    }
}

void UCombatActionPaletteWidget::OnPlayerAPChanged(int32 OldAP, int32 NewAP, FName Reason)
{
    if (CachedAPComp.IsValid() && PB_APBar && Text_AP)
    {
        int32 MaxAP = CachedAPComp->MaxAP;
        if (MaxAP > 0) PB_APBar->SetPercent(static_cast<float>(NewAP) / MaxAP);
        Text_AP->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), NewAP, MaxAP)));
    }
}

void UCombatActionPaletteWidget::OnPlayerSPChanged(int32 OldSP, int32 NewSP, FName Reason)
{
    if (CachedSPComp.IsValid() && PB_SPBar && Text_SP)
    {
        int32 MaxSP = CachedSPComp->MaxSP;
        if (MaxSP > 0) PB_SPBar->SetPercent(static_cast<float>(NewSP) / MaxSP);
        Text_SP->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), NewSP, MaxSP)));
    }
}