#pragma once

#include "CoreMinimal.h"
#include "Combat/Characters/CharacterRuntimeStateTypes.h"

class ACombatCharacterActor;

/** Source Combat Actor와 영속 Runtime State 사이의 유일한 변환 경계입니다. */
class JRPG_API FCharacterRuntimeStateAdapter
{
public:
	static bool Capture(const ACombatCharacterActor* Actor, FCharacterRuntimeState& OutState);
	static bool Restore(ACombatCharacterActor* Actor, const FCharacterRuntimeState& State);
};
