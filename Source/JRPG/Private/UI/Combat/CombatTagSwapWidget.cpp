#include "UI/Combat/CombatTagSwapWidget.h"

#include "UI/Combat/CombatTagSwapSlotWidget.h"

void UCombatTagSwapWidget::UpdateSwapUI(UCombatPartySlotViewModel* LeftVM, UCombatPartySlotViewModel* RightVM)
{
	if (Slot_Q && LeftVM)
	{
		Slot_Q->SetVisibility(ESlateVisibility::Visible);
		Slot_Q->BindSwapData(LeftVM, TEXT("Q"));
	}
	else
	{
		Slot_Q->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Slot_E && RightVM)
	{
		Slot_E->SetVisibility(ESlateVisibility::Visible);
		Slot_E->BindSwapData(RightVM, TEXT("E"));
	}
	else
	{
		Slot_E->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCombatTagSwapWidget::InitailizeSwapUI()
{
	Slot_Q->SetVisibility(ESlateVisibility::Collapsed);
	Slot_E->SetVisibility(ESlateVisibility::Collapsed);
}
