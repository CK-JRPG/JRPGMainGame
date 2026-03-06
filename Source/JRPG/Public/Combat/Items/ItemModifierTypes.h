// Source/JRPGCombat/Public/Combat/Items/ItemModifierTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/Items/ItemTypes.h"
#include "ItemModifierTypes.generated.h"

// Augment EffectType SSOT (대표)
// (A) Stat modifiers: Attack/Defense/HP/BreakPower/HealingPower/ThreatMod
// (B) Skill param modifiers: AoERadiusPct, BuffDurationPct, DebuffPotencyPct, BreakBuildUpPct

UENUM()
enum class EAugmentEffectType : uint8
{
	// --- Stat (Flat/Pct)
	AttackFlat,
	AttackPct,
	DefenseFlat,
	DefensePct,
	HPFlat,
	HPPct,
	BreakPowerFlat,
	BreakPowerPct,
	HealingPowerFlat,
	HealingPowerPct,
	ThreatModPct,

	// --- Skill params (Pct)
	AoERadiusPct,
	BuffDurationPct,
	DebuffPotencyPct,
	BreakBuildUpPct
};

USTRUCT()
struct FAugmentEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EAugmentEffectType EffectType = EAugmentEffectType::AttackFlat;

	// Pct는 0.10 = +10% (UI 표기는 10%)
	UPROPERTY(EditAnywhere) float Value = 0.f;

	// Skill Param Modifier일 때만 사용: "Heal.Zone", "AoE.All" 같은 타겟 태그/키
	UPROPERTY(EditAnywhere) FName TargetSkillTag = NAME_None;

	// 같은 계열 상한 관리
	UPROPERTY(EditAnywhere) FName CapGroup = NAME_None;
};

USTRUCT()
struct FRoleEfficiency
{
	GENERATED_BODY()

// 1.0 = 100%
	UPROPERTY(EditAnywhere) float Defender = 1.0f;
	UPROPERTY(EditAnywhere) float Attacker = 1.0f;
	UPROPERTY(EditAnywhere) float Supporter = 1.0f;

	float Get(ECombatRole Role) const
	{
		switch (Role)
		{
			case ECombatRole::Defender: return Defender;
			case ECombatRole::Attacker: return Attacker;
			case ECombatRole::Supporter: return Supporter;
			default: return 1.0f;
		}
	}
};

// 최종 스탯 계산 규칙 SSOT:
// Final = (Base + ΣFlat) * (1 + ΣPct)
USTRUCT()
struct FStatModifierAccumulator
{
	GENERATED_BODY()

	UPROPERTY() float Flat = 0.f;
	UPROPERTY() float Pct = 0.f;
};

USTRUCT()
struct FSkillParamModifierAccumulator
{
	GENERATED_BODY()

	// TagKey -> PctSum (0.2 = +20%)
	UPROPERTY() TMap<FName, float> PctBySkillTag;
};

USTRUCT()
struct FAugmentModifierSet
{
	GENERATED_BODY()

	UPROPERTY() FStatModifierAccumulator Attack;
	UPROPERTY() FStatModifierAccumulator Defense;
	UPROPERTY() FStatModifierAccumulator HP;
	UPROPERTY() FStatModifierAccumulator BreakPower;
	UPROPERTY() FStatModifierAccumulator HealingPower;
	UPROPERTY() float ThreatModPct = 0.f;

	// Skill params
	UPROPERTY() FSkillParamModifierAccumulator AoERadius;
	UPROPERTY() FSkillParamModifierAccumulator BuffDuration;
	UPROPERTY() FSkillParamModifierAccumulator DebuffPotency;
	UPROPERTY() FSkillParamModifierAccumulator BreakBuildUp;
};