#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillDataAsset.generated.h"

class UCombatSkill;

UCLASS()
class JRPG_API USkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Skill") TSubclassOf<UCombatSkill> SkillClass;
};