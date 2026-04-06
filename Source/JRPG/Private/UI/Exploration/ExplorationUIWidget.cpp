#include "UI/Exploration/ExplorationUIWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"

void UExplorationUIWidget::UpdateQuestInfo(UTexture2D* QuestIcon, const FString& ObjectiveText)
{
	if (Text_QuestObjective)
	{
		Text_QuestObjective->SetText(FText::FromString(ObjectiveText));
	}

	if (Image_QuestIcon)
	{
		if (QuestIcon)
		{
			Image_QuestIcon->SetBrushFromTexture(QuestIcon);
			Image_QuestIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			// 아이콘이 없으면 기본 설정된 이미지를 쓰거나 숨깁니다.
			// Image_QuestIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}