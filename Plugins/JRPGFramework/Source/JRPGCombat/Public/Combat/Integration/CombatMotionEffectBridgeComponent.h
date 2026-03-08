#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Movement/JRPGCombatMotionTypes.h"
#include "CombatMotionEffectBridgeComponent.generated.h"

class UJRPGCombatMotionComponent;
class UStatusComponent;
class UGroggyComponent;
class UCombatThreatComponent;
class UCombatMotionImpactConfigDataAsset;

/**
 * CombatMotion 이벤트 → 상태이상/그로기/SP/어그로 적용 브릿지
 * - 이동 SSOT(CombatMotion)와 효과 SSOT(Status/Groggy/SP/Threat)를 분리
 * - 기본 전투(스킬/피격/체인)로 이어 붙일 때 가장 안정적인 결합 방식
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UCombatMotionEffectBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatMotionEffectBridgeComponent();

	// 데이터 기반 룰(없으면 기본값으로 처리)
	UPROPERTY(EditDefaultsOnly, Category="JRPG|CombatMotion|Impact")
	TObjectPtr<UCombatMotionImpactConfigDataAsset> ImpactConfig = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient) TObjectPtr<UJRPGCombatMotionComponent> Motion = nullptr;
	UPROPERTY(Transient) TObjectPtr<UStatusComponent> Status = nullptr;
	UPROPERTY(Transient) TObjectPtr<UGroggyComponent> Groggy = nullptr;
	UPROPERTY(Transient) TObjectPtr<UCombatThreatComponent> Threat = nullptr;

	// Handle -> Request 캐시(Ended/Cancelled 때 archetype 확인용)
	UPROPERTY(Transient) TMap<uint64, FCombatMotionRequest> RequestById;

	// Bind handlers
	void Bind();
	void Unbind();

	// delegates
	void HandleMotionStarted(const FCombatMotionHandle& H, const FCombatMotionRequest& Req);
	void HandleMotionEnded(const FCombatMotionHandle& H, FName EndReason);
	void HandleMotionCancelled(const FCombatMotionHandle& H, FName CancelReason);
	void HandleWallSlam(const FCombatMotionHandle& H, const FHitResult& Hit, FName WallSlamTag);

	// CC 변화 -> CombatMotion CancelPolicy(OnCC) 트리거
	void HandleCCChanged(bool bNowCC);

	// apply rule
	void ApplyImpactRule(ECombatMotionArchetype Archetype, const FCombatMotionRequest& Req, FName SourceTag);

	// defaults if no config
	FCombatMotionImpactRule DefaultRuleFor(ECombatMotionArchetype Archetype) const;
};