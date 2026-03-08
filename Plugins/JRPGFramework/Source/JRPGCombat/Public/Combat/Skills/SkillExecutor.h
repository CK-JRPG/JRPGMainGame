#pragma once

#include "CoreMinimal.h"
#include "Combat/Skills/JRPGSkillTypes.h"

class UJRPGSkillDataAsset;
class UJRPGSkillComponent;
class UCombatAPComponent;
class UCombatHPComponent;
class UStatusComponent;
class UCombatGroggyComponent;
class UCombatThreatComponent;
class UJRPGCombatMotionComponent;


/**
 * SkillExecutor: 실제 실행 파이프라인
 * - SkillComponent는 SSOT 진입점/쿨다운 저장소
 * - Executor는 "검증/커밋/적용"을 담당(확장 안전)
 */

class JRPGCOMBAT_API FSkillExecutor
{
public:
	static FJRPGSkillResult Execute(UJRPGSkillComponent& SkillComp, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req);

private:
	
	// 유효성 검사 요청
	static bool ValidateRequest(UJRPGSkillComponent& SkillComp, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, FJRPGReason& OutReason, int32& OutAPCost);

	// 비용(AP) 검사
	static bool CommitCosts(UJRPGSkillComponent& SkillComp, int32 APCost, const FJRPGSkillRequest& Req, FJRPGReason& OutReason);
	
	static void ApplyEffects(UJRPGSkillComponent& SkillComp, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, FJRPGSkillResult& InOutResult);

	static void BuildTargetList(const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, TArray<AActor*>& OutTargets);

	// effect handlers
	static void ApplyDamageOrHeal(AActor* Caster, AActor* Target, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, bool bHeal, FJRPGSkillResult& InOutResult);
	static void ApplyStatus(AActor* Caster, AActor* Target, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult);
	static void RemoveStatus(AActor* Target, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult);
	static void AddGroggy(AActor* Target, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult);
	static void AddThreat(AActor* Target, AActor* Caster, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult);
	static void RequestMotion(AActor* Caster, AActor* PrimaryTarget, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult);
	static void GrantSP(AActor* Caster, const UJRPGSkillDataAsset& Skill, const FJRPGSkillRequest& Req, const FJRPGSkillEffectEntry& E, FJRPGSkillResult& InOutResult);

	static FVector ResolveMotionDirection(AActor* Caster, AActor* PrimaryTarget, const FJRPGSkillMotionEffect& Motion);

};