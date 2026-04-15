#include "UI/Combat/CombatTagSwapWidget.h"

#include "UI/Combat/CombatTagSwapSlotWidget.h"

void UCombatTagSwapWidget::UpdateSwapUI(UCombatPartySlotViewModel* LeftVM, UCombatPartySlotViewModel* RightVM)
{
	if (Slot_Q && LeftVM)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCombatTagSwapWidget::UpdateSwapUI : Slot Q"));
		Slot_Q->BindSwapData(LeftVM, TEXT("Q"));
	}

	if (Slot_E && RightVM)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCombatTagSwapWidget::UpdateSwapUI : Slot E"));
		Slot_E->BindSwapData(RightVM, TEXT("E"));
	}
}
