#include "UI/Combat/CombatPartyRosterWidget.h"
#include "Components/PanelWidget.h"

void UCombatPartyRosterWidget::ClearRoster() {
    if (PartyListContainer) PartyListContainer->ClearChildren();
}
void UCombatPartyRosterWidget::AddPartySlot(UUserWidget* SlotWidget) {
    if (PartyListContainer && SlotWidget) PartyListContainer->AddChild(SlotWidget);
}