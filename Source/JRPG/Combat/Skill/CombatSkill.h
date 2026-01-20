#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "JRPG/Combat/CombatTypes.h"
#include "CombatSkill.generated.h"

UCLASS(Abstract)
class JRPG_API UCombatSkill : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Skill") FName SkillId;
	UPROPERTY(EditDefaultsOnly, Category="Skill") FGameplayTagContainer Tags;
	UPROPERTY(EditDefaultsOnly, Category="Skill") int32 APCost = 1;
	UPROPERTY(EditDefaultsOnly, Category="Skill") float CooldownSec = 3.f;
	UPROPERTY(EditDefaultsOnly, Category="Skill") EAttackType AttackType = EAttackType::TargetAttack;
	UPROPERTY(EditDefaultsOnly, Category="Skill") bool bIgnoreRange = true;

	virtual bool CanExecute(AActor* User, AActor* Target) const;
	virtual void Execute(AActor* User, AActor* Target);
};
