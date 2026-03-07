#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PartySaveGame.generated.h"

UCLASS()
class JRPG_API UPartySaveGame :public USaveGame
{
	GENERATED_BODY()
	
public:
	UPROPERTY() TArray<FName> PartyIds;
	UPROPERTY() FName RestockKey = NAME_None;
};