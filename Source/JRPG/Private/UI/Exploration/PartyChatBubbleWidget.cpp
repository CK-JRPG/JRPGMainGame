#include "UI/Exploration/PartyChatBubbleWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPartyChatBubbleWidget::InitChatMessage(const FPartyChatMsg& Msg)
{
	if (Text_Message)
	{
		Text_Message->SetText(FText::FromString(Msg.Message));
	}

	if (Image_Profile && Msg.SpeakerIcon)
	{
		Image_Profile->SetBrushFromTexture(Msg.SpeakerIcon);
	}
}

void UPartyChatBubbleWidget::ForceDismiss()
{
	if (!bIsDismissing)
	{
		bIsDismissing = true;
		PlayOutroAnimation(); // 블루프린트에 퇴장 지시
	}
}