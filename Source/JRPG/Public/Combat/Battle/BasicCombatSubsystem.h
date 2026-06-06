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
	struct FHitStopRuntime
	{
		float OriginalDilation = 1.f;
		int32 Serial = 0;
	};

	TMap<TWeakObjectPtr<AActor>, FHitStopRuntime> ActiveHitStops;

	bool IsFriendlyTarget(AActor* Attacker, AActor* Target) const;
};
