// Source/JRPGCombat/Public/Combat/Status/CombatStatusCleanseInterface.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CombatStatusCleanseInterface.generated.h"

UINTERFACE(MinimalAPI)
class UCombatStatusCleanseInterface : public UInterface
{
	GENERATED_BODY()
};

class JRPG_API ICombatStatusCleanseInterface
{
	GENERATED_BODY()

public:
	// MatchingAnyTags 중 하나라도 만족하는 상태를 제거한다.
	// RemoveCount <= 0 이면 전부 제거.
	virtual int32 RemoveStatusesByAnyTags(
		const FGameplayTagContainer& MatchingAnyTags,
		int32 RemoveCount,
		AActor* SourceActor,
		FName ReasonTag) = 0;

	virtual int32 CountStatusesByAnyTags(const FGameplayTagContainer& MatchingAnyTags) const = 0;
};
