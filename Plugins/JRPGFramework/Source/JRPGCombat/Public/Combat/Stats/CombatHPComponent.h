#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "Combat/Core/CombatDamageTypes.h"
#include "CombatHPComponent.generated.h"

class UJRPGCombatMotionComponent;
class UStatusComponent;
class UGroggyComponent;
class UCombatThreatComponent;
class UCombatBattleSessionSubsystem;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float /*Current*/, float /*Max*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDamaged, const FCombatDamageSpec& /*Spec*/, const FCombatDamageResult& /*Result*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealed, const FCombatDamageSpec& /*Spec*/, const FCombatDamageResult& /*Result*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDied, AActor* /*Killer*/, FName /*ReasonTag*/);

/**
 * 기본 전투 HP SSOT(캐릭터 단위)
 * - ApplyDamageSpec 단일 진입점
 * - 피해/회복/사망 이벤트
 * - 전투 이동(HitMove), 상태이상, 그로기, 어그로, SP를 “옵션 연동” (존재하면 적용)
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UCombatHPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatHPComponent();

	// Tunables
	UPROPERTY(EditAnywhere, Category="JRPG|HP") float MaxHP = 100.f;
	UPROPERTY(VisibleAnywhere, Category="JRPG|HP") float CurrentHP = 100.f;

	UPROPERTY(EditAnywhere, Category="JRPG|HP") bool bCanOverheal = false;
	UPROPERTY(EditAnywhere, Category="JRPG|HP") float OverhealMaxMultiplier = 1.0f; // 1.0=불가

	// Events
	FOnHPChanged OnHPChanged;
	FOnDamaged OnDamaged;
	FOnHealed OnHealed;
	FOnDied OnDied;

	// Public API
	bool IsDead() const { return bDead; }

	void SetFullHP();
	void SetHP(float NewHP);

	/** 기본 전투 단일 진입점: Damage/Heal 모두 Spec 하나로 처리 */
	FCombatDamageResult ApplyDamageSpec(const FCombatDamageSpec& Spec);

protected:
	virtual void BeginPlay() override;

private:
	bool bDead = false;

	// cache components (optional)
	UPROPERTY(Transient) TObjectPtr<UJRPGCombatMotionComponent> CombatMotion = nullptr;
	UPROPERTY(Transient) TObjectPtr<UStatusComponent> Status = nullptr;
	UPROPERTY(Transient) TObjectPtr<UGroggyComponent> Groggy = nullptr;
	UPROPERTY(Transient) TObjectPtr<UCombatThreatComponent> Threat = nullptr;

	void CacheOptionalComponents();

	// internal helpers
	bool CheckCombatActiveIfRequired(const FCombatDamageSpec& Spec, FJRPGReason& OutReason) const;

	void ApplyHitReactionIfNeeded(const FCombatDamageSpec& Spec);
	void ApplyStatusIfNeeded(const FCombatDamageSpec& Spec);
	void ApplyGroggyIfNeeded(const FCombatDamageSpec& Spec);
	void ApplyThreatIfNeeded(const FCombatDamageSpec& Spec);

	void RouteSPEvents(const FCombatDamageSpec& Spec, const FCombatDamageResult& Result);

	FCombatDamageResult ApplyDamageInternal(const FCombatDamageSpec& Spec);
	FCombatDamageResult ApplyHealInternal(const FCombatDamageSpec& Spec);

	void Kill(AActor* Killer, FName ReasonTag);
};