#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ThreatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnThreatTargetChangedNative, AActor* /*Old*/, AActor* /*New*/);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JRPG_API UThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UThreatComponent();

	UPROPERTY(EditAnywhere, Category="Threat") float TargetSwitchThresholdRatio = 1.2f;
	UPROPERTY(EditAnywhere, Category="Threat") float TargetUpdateIntervalSec = 1.0f;

	FOnThreatTargetChangedNative OnTargetChanged;

	void InitThreat(const TArray<AActor*>& PartyMembers);

	void AddThreat(AActor* Target, float Amount);
	void ReduceThreat(AActor* Target, float Amount);

	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }
	float GetThreatOf(AActor* Target) const;

	void ForceTarget(AActor* NewTarget, float DurationSec);

protected:
	virtual void BeginPlay() override;

private:
	TMap<TWeakObjectPtr<AActor>, float> ThreatTable;
	TWeakObjectPtr<AActor> CurrentTarget;

	TWeakObjectPtr<AActor> ForcedTarget;
	double ForcedTargetEndRealTime = 0.0;

	FTimerHandle UpdateTimer;

	void UpdateTarget();
	bool IsForceActive() const;
};
