#include "UI/Combat/CombatUIWidget.h"
#include "UI/Combat/CombatActionPaletteWidget.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatTagSwapWidget.h"
#include "UI/Combat/CombatTargetInfoWidget.h"
#include "UI/Combat/CombatEncounterOverlayWidget.h"
#include "Components/CanvasPanel.h"
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

void UCombatUIWidget::SetCombatPanelsVisible(bool bVisible)
{
    const ESlateVisibility PanelVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden;

    if (TargetInfoPanel)
    {
        TargetInfoPanel->SetVisibility(PanelVisibility);
    }

    //if (PartyRosterPanel)
    //{
    //    PartyRosterPanel->SetVisibility(PanelVisibility);
    //}

    if (TagSwapPanel)
    {
        TagSwapPanel->SetVisibility(PanelVisibility);
    }

    if (ActionPalettePanel)
    {
        ActionPalettePanel->SetVisibility(PanelVisibility);
    }

    if (Canvas_Damage)
    {
        Canvas_Damage->SetVisibility(PanelVisibility);
    }
}

void UCombatUIWidget::ShowEncounterOverlay()
{
    if (EncounterOverlayPanel)
    {
        EncounterOverlayPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (Anim_EncounterIntro)
    {
        PlayAnimation(Anim_EncounterIntro);
    }
}

void UCombatUIWidget::HideEncounterOverlay()
{
    if (Anim_EncounterIntro)
    {
        StopAnimation(Anim_EncounterIntro);
    }

    if (EncounterOverlayPanel)
    {
        EncounterOverlayPanel->SetVisibility(ESlateVisibility::Hidden);
    }
}
