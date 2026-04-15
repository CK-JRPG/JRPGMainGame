#include "UI/Combat/CombatTagSwapWidget.h"

#include "UI/Combat/CombatTagSwapSlotWidget.h"

void UCombatTagSwapWidget::UpdateSwapUI(UCombatPartySlotViewModel* LeftVM, UCombatPartySlotViewModel* RightVM)
{
	if (Slot_Q && LeftVM)
	{
		Slot_Q->BindSwapData(LeftVM, TEXT("Q"));
	}

	if (Slot_E && RightVM)
	{
		Slot_E->BindSwapData(RightVM, TEXT("E"));
	}
}
