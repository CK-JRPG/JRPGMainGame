#include "UI/Combat/CombatActionPaletteWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

//void UCombatActionPaletteWidget::UpdateSPUI(float Percent, const FString& Text)
//{
//    if (PB_SPBar) PB_SPBar->SetPercent(Percent);
//    if (Text_SP) Text_SP->SetText(FText::FromString(Text));
//}
//
//void UCombatActionPaletteWidget::UpdateHP(float Percent, const FString& Text)
//{
//    if (PB_HPBar) PB_HPBar->SetPercent(Percent);
//    if (Text_HP) Text_HP->SetText(FText::FromString(Text));
//}
//
void UCombatActionPaletteWidget::UpdateAP(float Percent)
{
    if (PB_APBar) PB_APBar->SetPercent(Percent);
}

void UCombatActionPaletteWidget::UpdateSkillList(const TArray<FString>& SkillNames)
{
    TArray<UTextBlock*> SkillTexts = { Text_Skill1, Text_Skill2, Text_Skill3, Text_Skill4 };
    TArray<UImage*> KeyImages = { Img_Key1, Img_Key2, Img_Key3, Img_Key4 };

    for (int32 i = 0; i < 4; ++i)
    {
        bool bHasSkill = SkillNames.IsValidIndex(i);

        if (SkillTexts[i])
        {
            if (bHasSkill)
            {
                SkillTexts[i]->SetText(FText::FromString(SkillNames[i]));
                SkillTexts[i]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            }
            else
            {
                SkillTexts[i]->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        if (KeyImages[i])
        {
            if (bHasSkill)
            {
                KeyImages[i]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            }
            else
            {
                KeyImages[i]->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

UCombatPartySlotWidget* UCombatActionPaletteWidget::GetPartySlot(int32 Index)
{
    if (Index == 0) return Slot_Party1;
    if (Index == 1) return Slot_Party2;
    if (Index == 2) return Slot_Party3;
    return nullptr;
}

void UCombatActionPaletteWidget::ClearAllPartySlots()
{
    if (Slot_Party1) Slot_Party1->SetVisibility(ESlateVisibility::Collapsed);
    if (Slot_Party2) Slot_Party2->SetVisibility(ESlateVisibility::Collapsed);
    if (Slot_Party3) Slot_Party3->SetVisibility(ESlateVisibility::Collapsed);
}

FName UCombatActionPaletteWidget::GetSelectedPartyMemberID_Implementation() const
{
    TArray<UCombatPartySlotWidget*> Slots = { Slot_Party1, Slot_Party2, Slot_Party3 };

    for (UCombatPartySlotWidget* S : Slots)
    {
        if (S && S->GetVisibility() == ESlateVisibility::SelfHitTestInvisible && S->IsHovered())
        {
            return S->GetCharacterID();
        }
    }

    return NAME_None;
}