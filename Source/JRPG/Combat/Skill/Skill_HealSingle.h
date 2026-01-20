#pragma once
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "Skill_HealSingle.generated.h"

UCLASS()
class JRPG_API USkill_HealSingle : public UCombatSkill
{
	GENERATED_BODY()

public:
	USkill_HealSingle();

	UPROPERTY(EditDefaultsOnly, Category="Heal") float HealAmount = 25.f;
	UPROPERTY(EditDefaultsOnly, Category="Heal") float HealThreatMultiplier = 0.5f;

	virtual void Execute(AActor* User, AActor* Target) override;
};
