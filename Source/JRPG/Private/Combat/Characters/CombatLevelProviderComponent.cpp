#include "Combat/Characters/CombatLevelProviderComponent.h"
#include "Combat/Progression/Leveling/LevelingSubsystem.h"

int32 UCombatLevelProviderComponent::GetCharacterLevel(const AActor* /*Character*/) const
{
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		if (ULevelingSubsystem* L = GetWorld()->GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
			return L->GetPartyLevel();
	}
	return 1;
}

int32 UCombatLevelProviderComponent::GetPartyLevel() const
{
	return GetCharacterLevel(nullptr);
}