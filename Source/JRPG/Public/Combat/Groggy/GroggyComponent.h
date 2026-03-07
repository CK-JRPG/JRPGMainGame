#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GroggyComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGroggyStateChanged,bool/*bGroggy*/);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UGroggyComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UGroggyComponent();

	UPROPERTY(EditAnywhere) float MaxGauge = 100.f;
	UPROPERTY(VisibleAnywhere) float Gauge = 0.f;

	UPROPERTY(EditAnywhere) float GroggyDurationSec = 6.f;
	UPROPERTY(VisibleAnywhere) bool bGroggy = false;

	// 그로기 중 디버프 (StatsComponent에 주입)
	UPROPERTY(EditAnywhere) float DefenseMulWhileGroggy = 0.8f;
	UPROPERTY(EditAnywhere) float SpeedMulWhileGroggy = 0.85f;

	FOnGroggyStateChanged OnGroggyStateChanged;

	void AddGroggyDamage(float Amount, AActor* SourceActor, FName ReasonTag, bool bFromTacticalReservation = false);
	void ResetGauge(FName ReasonTag);

protected:
	virtual void BeginPlay()override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	UCLASS()
	class UGroggyModSourceObject : public UObject { GENERATED_BODY() };

	UPROPERTY() float RemainingGroggy = 0.f;
	UPROPERTY() TObjectPtr<UGroggyModSourceObject> ModSource = nullptr;

	TWeakObjectPtr<class UCombatStatsComponent> Stats;
	
	TWeakObjectPtr<AActor> LastBreakSource;
	bool bLastBreakFromTacticalReservation = false;

	void EnterGroggy();
	void ExitGroggy();
};