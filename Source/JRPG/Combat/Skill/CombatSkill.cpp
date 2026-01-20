#include "CombatSkill.h"

bool UCombatSkill::CanExecute(AActor* User, AActor* Target) const
{
	return User && Target;
}

void UCombatSkill::Execute(AActor* User, AActor* Target)
{
}
