#pragma once
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "Skill_Damage.generated.h"

UCLASS()
class JRPG_API USkill_Damage : public UCombatSkill
{
	GENERATED_BODY()

public:
	USkill_Damage();

	UPROPERTY(EditDefaultsOnly, Category="Damage") float Damage = 25.f;

	virtual void Execute(AActor* User, AActor* Target) override;
};
