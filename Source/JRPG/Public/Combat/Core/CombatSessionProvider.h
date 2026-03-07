// Source/JRPGCombat/Public/Combat/Core/CombatSessionProvider.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatSessionProvider.generated.h"

UINTERFACE(MinimalAPI)
class UCombatSessionProvider : public UInterface
{
	GENERATED_BODY()
};

class ICombatSessionProvider
{
	GENERATED_BODY()
public:
	virtual bool IsCombatActive() const = 0; // 전투 중인가?
	virtual bool IsChainSequenceActive() const = 0; // 체인 시퀀스(제노블식 별도 전투) 중인가?
};