#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "CombatAPComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAPChanged, int32 /*Current*/, int32 /*Max*/);

/**
 * AP(행동 포인트) SSOT(캐릭터 단위)
 * - 스킬 시스템의 Validate/Commit에서 사용
 * - Tick regen은 옵션(RealTime 기반)
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UCombatAPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatAPComponent();

	UPROPERTY(EditAnywhere, Category="JRPG|AP") int32 MaxAP = 100;
	UPROPERTY(VisibleAnywhere, Category="JRPG|AP") int32 CurrentAP = 100;

	// regen (optional)
	UPROPERTY(EditAnywhere, Category="JRPG|AP") bool bEnableRegen = false;
	UPROPERTY(EditAnywhere, Category="JRPG|AP") float RegenPerSecond = 5.f;

	FOnAPChanged OnAPChanged;

	bool CanSpend(int32 Cost) const { return Cost <= CurrentAP; }
	FJRPGOpResult Spend(int32 Cost, FName ReasonTag);
	FJRPGOpResult Gain(int32 Amount, FName ReasonTag);
	void SetFullAP();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	double LastRealTime = 0.0;
};