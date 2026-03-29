#include "UI/Combat/CombatUIWidget.h"

#include "Combat/Characters/PartyActorSpawnSubsystem.h"
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
		// PartyActorSpawnSubsystem에서 모든 스폰된 전투 캐릭터를 가져옴
		if (UWorld* World = GetWorld())
		{
			if (UPartyActorSpawnSubsystem* SpawnSub = World->GetSubsystem<UPartyActorSpawnSubsystem>())
			{
				TArray<ACombatCharacterActor*> AllActors = SpawnSub->GetSpawnedActors();
				PartyRosterPanel->InitializePartyFromActors(AllActors);
			}
		}
	}
}