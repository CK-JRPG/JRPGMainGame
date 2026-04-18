#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "JRPGCombatLevelProvider.generated.h"

UINTERFACE(MinimalAPI)
class UJRPGCombatLevelProvider : public UInterface { GENERATED_BODY() };
 
class IJRPGCombatLevelProvider
{
	GENERATED_BODY()

public:
	virtual int32 GetCharacterLevel(const AActor* Character) const = 0;
	virtual int32 GetPartyLevel() const = 0;
};