#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "StatusTypes.h"
#include "StatusComponent.generated.h"

class UStatusDataAsset;

/**
 * 상태이상 SSOT(캐릭터 단위)
 * - ApplyStatus 단일 진입점
 * - CC 여부를 통합적으로 관리(기획서: CC 체크는 단일 체크)
 * - Tick은 RealTime 기준(시간 슬로모 영향 최소화)
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusComponent();

	UPROPERTY(EditDefaultsOnly, Category="JRPG|Status")
	TObjectPtr<UStatusDataAsset> StatusDB = nullptr;

	FOnStatusApplied OnStatusApplied;
	FOnStatusRemoved OnStatusRemoved;
	FOnCCStateChanged OnCCStateChanged;

	FJRPGOpResult ApplyStatus(const FStatusSpec& Spec);
	FJRPGOpResult RemoveStatus(FName StatusId, FName ReasonTag);
	bool HasStatus(FName StatusId) const;

	bool IsCrowdControlled() const { return bAnyCC; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TMap<FName, FActiveStatus> Active;

	bool bAnyCC = false;
	double LastRealTime = 0.0;

	void RecomputeCC();
};
