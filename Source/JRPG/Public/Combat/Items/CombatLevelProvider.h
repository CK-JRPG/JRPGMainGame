// Source/JRPGCombat/Public/Combat/Items/CombatLevelProvider.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatLevelProvider.generated.h"

UINTERFACE(MinimalAPI)
class UCombatLevelProvider : public UInterface
{
	GENERATED_BODY()
};

class ICombatLevelProvider
{
	GENERATED_BODY()
public:
	// 파티 공유 레벨(레벨업 문서에서 SSOT) 또는 캐릭터 레벨을 반환
	virtual int32 GetCharacterLevel(const AActor * Character) const = 0;
};