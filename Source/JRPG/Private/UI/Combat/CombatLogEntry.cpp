#include "UI/Combat/CombatLogEntry.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "TimerManager.h"

void UCombatLogEntry::SetupLog(const FString& Message, UTexture2D* Icon)
{
    if (Text_LogMessage)
    {
        Text_LogMessage->SetText(FText::FromString(Message));
    }

    if (Img_SkillIcon)
    {
        if (Icon)
        {
            Img_SkillIcon->SetBrushFromTexture(Icon);
            Img_SkillIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            Img_SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if(UWorld * World = GetWorld())
    {
        World->GetTimerManager().SetTimer(AutoDestroyTimerHandle, this, &UCombatLogEntry::RemoveLogEntry, 5.0f, false);
    }
}

void UCombatLogEntry::RemoveLogEntry()
{
    RemoveFromParent();
}