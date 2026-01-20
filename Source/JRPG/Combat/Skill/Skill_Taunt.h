#pragma once
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "Skill_Taunt.generated.h"

UCLASS()
class JRPG_API USkill_Taunt : public UCombatSkill
{
	GENERATED_BODY()

public:
	USkill_Taunt();

	UPROPERTY(EditDefaultsOnly, Category="Taunt") float ForceDurationSec = 3.0f;
	UPROPERTY(EditDefaultsOnly, Category="Taunt") float ThreatBoost = 100000.f;

	virtual void Execute(AActor* User, AActor* Target) override;
};
