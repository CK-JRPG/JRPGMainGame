#include "UI/Exploration/ExplorationUIWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "UI/ViewModels/CombatViewModels.h"
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

void UExplorationUIWidget::SetPartyStatusMode(int32 Mode)
{
	if (!Widget_PartyStatus) return;

	if (Mode == 0) // 숨김
	{
		Widget_PartyStatus->SetVisibility(ESlateVisibility::Hidden);
		// (필요 시 지역명도 숨김 처리)
	}
	else if (Mode == 1) // 전투 후 회복 모드 (Tab 텍스트 등은 가리고 체력바만 보여주는 연출)
	{
		Widget_PartyStatus->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PlayPartyStatusAnim(false);
	}
	else if (Mode == 2) // Tab 전체 정보 모드
	{
		Widget_PartyStatus->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PlayPartyStatusAnim(true);
		PlayRegionNameAnimation();
	}
}

void UExplorationUIWidget::ShowInteraction(bool bShow, const FString& Text)
{
	if (Overlay_Interaction && Text_InteractionAction)
	{
		Overlay_Interaction->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Text_InteractionAction->SetText(FText::FromString(Text));
	}
}

void UExplorationUIWidget::ShowDialogue(bool bShow, const FString& Speaker, const FString& Text)
{
	if (Overlay_Dialogue && Text_DialogueSpeaker && Text_DialogueContent)
	{
		Overlay_Dialogue->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		Text_DialogueSpeaker->SetText(FText::FromString(Speaker));
		Text_DialogueContent->SetText(FText::FromString(Text));
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

	// 최대 5개 유지. 5개(또는 그 이상)라면 가장 위에 있는(오래된) 말풍선을 퇴장시킵니다.
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