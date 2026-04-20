#include "UI/Combat/CombatUIWidget.h"
#include "Components/TextBlock.h"

void UCombatUIWidget::PlaySkillAnnouncer(const FString& SkillName)
{
    if (Text_SkillAnnouncer)
    {
        Text_SkillAnnouncer->SetText(FText::FromString(SkillName));
    }

    if (Anim_SkillAnnouncer)
    {
        PlayAnimation(Anim_SkillAnnouncer);
    }
}
