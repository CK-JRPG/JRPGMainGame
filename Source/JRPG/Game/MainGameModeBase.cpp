#include "MainGameModeBase.h"

#include "JRPG/Player/CombatPlayerController.h"

AMainGameModeBase::AMainGameModeBase()
{
	PlayerControllerClass = ACombatPlayerController::StaticClass();
}
