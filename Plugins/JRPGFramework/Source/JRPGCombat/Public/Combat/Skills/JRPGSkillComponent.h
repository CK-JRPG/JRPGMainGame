#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "Combat/Skills/JRPGSkillTypes.h"
#include "JRPGSkillComponent.generated.h"

class UJRPGSkillDataAsset;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSkillExecuted, const FJRPGSkillRequest& /*Req*/, const FJRPGSkillResult& /*Result*/);

/**
 * 스킬 SSOT(캐릭터 단위)
 * - RequestUseSkill 단일 진입점
 * - 쿨다운/글로벌 쿨다운 저장소
 */
UCLASS(ClassGroup=(JRPG), meta=(BlueprintSpawnableComponent))
class JRPGCOMBAT_API UJRPGSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJRPGSkillComponent();

	/** 스킬 DB(간단 버전): SkillId -> DataAsset */
	UPROPERTY(EditDefaultsOnly, Category="JRPG|Skill")
	TMap<FName, TObjectPtr<UJRPGSkillDataAsset>> SkillDB;

	const UJRPGSkillDataAsset *GetSkillAsset(FName SkillId) const;
	/** 이벤트 */
	FOnSkillExecuted OnSkillExecuted;

	// --- Public API ---
	FJRPGSkillResult RequestUseSkill(const FJRPGSkillRequest& Req);
	bool CanUseReservedSkill(FName SkillId, FJRPGReason &OutReason) const;

	void TickTacticalReservation(float RealDelta);

	bool IsOnCooldown(FName SkillId) const;
	bool IsOnGlobalCooldown() const { return GlobalCooldownRemaining > 0.f; }

	float GetCooldownRemaining(FName SkillId) const;
	float GetGlobalCooldownRemaining() const { return GlobalCooldownRemaining; }

	// 스킬 시스템 확장을 위한 공용 함수(Executor가 호출)
	void StartCooldown(FName SkillId, float CooldownSec);
	void StartGlobalCooldown(float Sec);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// RealTime 기반 쿨다운(기획서 일관성: 슬로모 영향 최소)
	double LastRealTime = 0.0;

	UPROPERTY(Transient)
	TMap<FName, float> CooldownRemaining; // seconds

	UPROPERTY(Transient)
	float GlobalCooldownRemaining = 0.f;

	const UJRPGSkillDataAsset* FindSkill(FName SkillId) const;

	void TickCooldowns(float RealDelta);
};