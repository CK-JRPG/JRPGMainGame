#include "Combat/Characters/CharacterRuntimeStateAdapter.h"

#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/SP/SPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/Stats/HPComponent.h"

bool FCharacterRuntimeStateAdapter::Capture(const ACombatCharacterActor* Actor, FCharacterRuntimeState& OutState)
{
	if (!IsValid(Actor) || !Actor->HPComp || !Actor->APComp || !Actor->SPComp)
	{
		return false;
	}

	FCharacterRuntimeState Captured;
	Captured.HP = Actor->HPComp->GetHP();
	Captured.MaxHP = Actor->HPComp->GetMaxHP();
	Captured.AP = Actor->APComp->GetAP();
	Captured.MaxAP = Actor->APComp->GetMaxAP();
	Captured.SP = Actor->SPComp->GetSP();
	Captured.MaxSP = Actor->SPComp->GetMaxSP();

	if (!Captured.IsValid())
	{
		return false;
	}

	OutState = Captured;
	return true;
}

bool FCharacterRuntimeStateAdapter::Restore(ACombatCharacterActor* Actor, const FCharacterRuntimeState& State)
{
	if (!IsValid(Actor) || !Actor->HPComp || !Actor->APComp || !Actor->SPComp)
	{
		return false;
	}

	if (!State.IsValid())
	{
		return false;
	}

	// Import API는 damage/death/consume 등의 gameplay delegate를 발생시키지 않습니다.
	Actor->HPComp->ImportRuntimeState(State.MaxHP, State.HP);
	Actor->APComp->ImportRuntimeState(State.MaxAP, State.AP);
	Actor->SPComp->ImportRuntimeState(State.MaxSP, State.SP);
	return true;
}
