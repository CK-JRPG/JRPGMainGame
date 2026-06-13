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

	static constexpr float BasicAttackThreatGeneratedScalar = 0.55f;
	static constexpr float BasicHitStopSec = 0.045f;
	static constexpr float SkillHitStopSec = 0.065f;
	static constexpr float HeavyHitStopMinSec = 0.085f;
	static constexpr float HeavyHitStopMaxSec = 0.12f;
	static constexpr float HitStopTimeDilation = 0.02f;

	bool IsFriendlyTarget(AActor* Attacker, AActor* Target) const;
};
