#include "UI/Combat/CombatPartyRosterWidget.h"

#include "Combat/Characters/CombatCharacterActor.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "Components/PanelWidget.h"

void UCombatPartyRosterWidget::InitializePartyFromActors(const TArray<ACombatCharacterActor*>& PartyActors)
{
	if (!PartyListContainer || !PartySlotClass) return;

	PartyListContainer->ClearChildren();

	for (ACombatCharacterActor* Actor : PartyActors)
	{
		if (!IsValid(Actor)) continue;

		UCombatPartySlotWidget* NewSlot = CreateWidget<UCombatPartySlotWidget>(GetWorld(), PartySlotClass);
		if (NewSlot)
		{
			NewSlot->BindPartyMember(Actor);
			PartyListContainer->AddChild(NewSlot);
		}
	}
}