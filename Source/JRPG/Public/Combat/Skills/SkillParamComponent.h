// Source/JRPGCombat/Public/Combat/Skills/SkillParamComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Items/ItemModifierTypes.h"
#include "SkillParamComponent.generated.h"

// SkillSystem은 최종 값을 계산(아이템은 직접 변경 X, modifier 제공)
// FinalRadius = BaseRadius * (1 + ΣAoERadiusPct) 같은 형태.

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPGCOMBAT_API USkillParamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void ApplyAugmentModifierSet(const FAugmentModifierSet& Mods);

	// TagKey가 NAME_None이면 "AoE.All" 같은 기본 키로 취급할 수도 있음
	float GetAoERadiusMultiplier(FName SkillTagKey) const;
	float GetBuffDurationMultiplier(FName SkillTagKey) const;
	float GetDebuffPotencyMultiplier(FName SkillTagKey) const;
	float GetBreakBuildUpMultiplier(FName SkillTagKey) const;

private:
	FAugmentModifierSet Cached;

	static float MultFromMap(const TMap<FName, float>& Map, FName Key);
};