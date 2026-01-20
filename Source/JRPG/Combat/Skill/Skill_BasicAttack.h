#pragma once
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "Skill_BasicAttack.generated.h"

UCLASS()
class JRPG_API USkill_BasicAttack : public UCombatSkill
{
	GENERATED_BODY()

public:
	USkill_BasicAttack();

	UPROPERTY(EditDefaultsOnly, Category="Basic") float Damage = 10.f;
	UPROPERTY(EditDefaultsOnly, Category="Basic") float CooldownReducePercentOnHit = 0.10f;
	UPROPERTY(EditDefaultsOnly, Category="Basic") float ThreatOnHitMultiplier = 1.0f;

	virtual void Execute(AActor* User, AActor* Target) override;
};
