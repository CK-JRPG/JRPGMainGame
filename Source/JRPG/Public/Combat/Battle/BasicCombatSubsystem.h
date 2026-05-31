#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "BasicCombatSubsystem.generated.h"

UCLASS()
class JRPG_API UBasicCombatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnBasicAttackResolved OnBasicAttackResolved;
	FOnCombatantDefeated OnCombatantDefeated;

	FCombatActionResult ExecuteBasicAttack(const FBasicAttackRequest& Req);
	void ApplyHitFeedback(AActor* Attacker, AActor* Target, float DamageAmount, bool bCritical, bool bSkillOrHeavyHit, FName SourceTag = NAME_None);

private:
	bool IsFriendlyTarget(AActor* Attacker, AActor* Target) const;
};
