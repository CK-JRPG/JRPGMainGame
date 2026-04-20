#include "UI/Combat/CombatTagSwapSlotWidget.h"

#include "UI/ViewModels/CombatViewModels.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCombatTagSwapSlotWidget::BindSwapData(UCombatPartySlotViewModel* InVM, const FString& KeyString)
{
	if (Text_KeyBadge)
	{
		Text_KeyBadge->SetText(FText::FromString(KeyString));
	}

	if (!InVM || !Img_Portrait) return;
	FName CharID = InVM->GetCharacterID();

	// TODO : 데이터 에셋/테이블에서 CharID로 초상화 연동
}
