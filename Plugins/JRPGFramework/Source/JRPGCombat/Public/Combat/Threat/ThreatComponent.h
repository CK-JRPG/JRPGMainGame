#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCoreApiTypes.h"
#include "ThreatTypes.h"
#include "ThreatComponent.generated.h"

/**
 * 어그로 SSOT(적 1체당 1개가 일반적)
 * - 단순 ThreatMap + CurrentTarget
 * - TargetLock(벽꿍/그랩 등에서 잠깐 고정) 제공
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThreatComponent();

	FOnThreatTargetChanged OnTargetChanged;

	FJRPGOpResult AddThreat(AActor* Target, float Amount, FName ReasonTag);
	AActor* GetCurrentTarget() const { return LockedTarget ? LockedTarget.Get() : CurrentTarget.Get(); }

	// Lock target for a duration (seconds). Duration<=0 -> unlock
	FJRPGOpResult LockTarget(AActor* Target, float Duration, FName ReasonTag);
	void UnlockTarget(FName ReasonTag);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TMap<TWeakObjectPtr<AActor>, float> ThreatMap;
	UPROPERTY(Transient) TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient) TWeakObjectPtr<AActor> LockedTarget;
	FTimerHandle LockTimer;

	void RecomputeTarget();
};