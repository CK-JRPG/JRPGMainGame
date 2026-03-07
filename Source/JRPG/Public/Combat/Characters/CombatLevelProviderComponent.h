#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Items/CombatLevelProvider.h"
#include "CombatLevelProviderComponent.generated.h"

UCLASS(ClassGroup=(Combat),meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatLevelProviderComponent : public UActorComponent, public ICombatLevelProvider
{
	GENERATED_BODY()

public:
	virtual int32 GetCharacterLevel(const AActor* Character) const override;
	virtual int32 GetPartyLevel() const override;
};