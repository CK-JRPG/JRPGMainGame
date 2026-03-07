// Source/JRPGCombat/Private/Combat/Skills/SkillParamComponent.cpp
#include "Combat/Skills/SkillParamComponent.h"

void USkillParamComponent::ApplyAugmentModifierSet(const FAugmentModifierSet& Mods)
{
	Cached = Mods;
}

float USkillParamComponent::MultFromMap(const TMap<FName, float>& Map, FName Key)
{
	// 기본 키 fallback 지원(원하면 "AoE.All" 등으로 통일)
	const float Pct = Map.Contains(Key) ? Map[Key] : 0.f;
	return 1.0f + Pct;
}

float USkillParamComponent::GetAoERadiusMultiplier(FName SkillTagKey) const
{
	return MultFromMap(Cached.AoERadius.PctBySkillTag, SkillTagKey);
}

float USkillParamComponent::GetBuffDurationMultiplier(FName SkillTagKey) const
{
	return MultFromMap(Cached.BuffDuration.PctBySkillTag, SkillTagKey);
}

float USkillParamComponent::GetDebuffPotencyMultiplier(FName SkillTagKey) const
{
	return MultFromMap(Cached.DebuffPotency.PctBySkillTag, SkillTagKey);
}

float USkillParamComponent::GetBreakBuildUpMultiplier(FName SkillTagKey) const
{
	return MultFromMap(Cached.BreakBuildUp.PctBySkillTag, SkillTagKey);
}