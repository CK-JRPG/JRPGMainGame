#include "UI/Combat/CombatUIWidget.h"
#include "UI/Combat/CombatTargetInfoWidget.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatActionPaletteWidget.h"

void UCombatUIWidget::InitializeCombatState(AActor* PlayerActor)
{
	if (!PlayerActor) return;

	if (ActionPalettePanel)
	{
		ActionPalettePanel->BindPlayerCharacter(PlayerActor);
	}

	if (PartyRosterPanel)
	{
		PartyRosterPanel->InitializeParty(PlayerActor); 
	}
}