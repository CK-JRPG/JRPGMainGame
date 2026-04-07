#include "UI/Exploration/ExplorationUIWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/Exploration/PartyChatBubbleWidget.h"
#include "Components/VerticalBox.h"

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

void UExplorationUIWidget::ShowRegionName(const FString& RegionName)
{
	if (Text_RegionName)
	{
		Text_RegionName->SetText(FText::FromString(RegionName));
		PlayRegionNameAnimation(); // BP 애니메이션 킥
	}
}

void UExplorationUIWidget::AddPartyChat(const FPartyChatMsg& Msg)
{
	if (!VBox_PartyChat || !ChatBubbleClass) return;

	// [요구사항] 최대 5개 유지. 5개(또는 그 이상)라면 가장 위에 있는(오래된) 말풍선을 퇴장시킵니다.
	// 제거된 위젯은 스스로 애니메이션 종료 후 자신을 배열과 부모에서 제거(RemoveFromParent)할 것입니다.
	if (VBox_PartyChat->GetChildrenCount() >= 5)
	{
		if (UPartyChatBubbleWidget* OldestBubble = Cast<UPartyChatBubbleWidget>(VBox_PartyChat->GetChildAt(0)))
		{
			OldestBubble->ForceDismiss();
		}
	}

	// 새 말풍선 생성 및 아래에 추가
	UPartyChatBubbleWidget* NewBubble = CreateWidget<UPartyChatBubbleWidget>(this, ChatBubbleClass);
	if (NewBubble)
	{
		NewBubble->InitChatMessage(Msg);
		VBox_PartyChat->AddChildToVerticalBox(NewBubble);
	}
}
