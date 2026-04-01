#include "UI/Combat/CombatActionPaletteWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Combat/SP/SPComponent.h"

void UCombatActionPaletteWidget::BindPlayerCharacter(AActor* PlayerActor)
{
    if (!PlayerActor) return;

    if (USPComponent* SPComp = PlayerActor->FindComponentByClass<USPComponent>())
    {
        CachedSPComp = SPComp;
        SPComp->OnSPChanged.AddUObject(this, &UCombatActionPaletteWidget::OnPlayerSPChanged);
        OnPlayerSPChanged(SPComp->GetSP(), SPComp->GetSP(), NAME_None);
    }
}

void UCombatActionPaletteWidget::NativeDestruct()
{
    if (CachedSPComp.IsValid()) CachedSPComp->OnSPChanged.RemoveAll(this);
    Super::NativeDestruct();
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