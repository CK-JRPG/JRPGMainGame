// Source/JRPGCombat/Public/Combat/Core/PartyProvider.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PartyProvider.generated.h"

UINTERFACE(MinimalAPI)
class UPartyProvider : public UInterface
{
	GENERATED_BODY()
};

class IPartyProvider
{
	GENERATED_BODY()
public:
	virtual void GetPartyMembers(TArray<AActor*>& OutMembers) const = 0; // 파티 전체
};