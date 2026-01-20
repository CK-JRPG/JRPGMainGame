#pragma once
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "Skill_ThreatDown.generated.h"

UCLASS()
class JRPG_API USkill_ThreatDown : public UCombatSkill
{
	GENERATED_BODY()

public:
	USkill_ThreatDown();

	UPROPERTY(EditDefaultsOnly, Category="ThreatDown") float ReduceAmount = 200.f;

	virtual void Execute(AActor* User, AActor* Target) override;
};
