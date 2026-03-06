#pragma once

#include "CoreMinimal.h"
#include "CombatTeamTypes.generated.h"

UENUM()
enum class ECombatTeam : uint8
{
	Player,
	Enemy,
	Neutral
};
