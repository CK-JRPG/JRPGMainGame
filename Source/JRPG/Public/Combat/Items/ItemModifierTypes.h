// Source/JRPGCombat/Public/Combat/Items/ItemModifierTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Core/RoleTypes.h"
#include "ItemModifierTypes.generated.h"

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
	UPROPERTY(EditAnywhere) float Value = 0.f; // Pct는 0.10 = +10%
	UPROPERTY(EditAnywhere) FName TargetSkillTag = NAME_None; // Skill tag key (e.g., "AoE.All")
	UPROPERTY(EditAnywhere) FName CapGroup = NAME_None; // 상한 그룹
};

USTRUCT()
struct FRoleEfficiency
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) float Defender = 1.0f;
	UPROPERTY(EditAnywhere) float Attacker = 1.0f;
	UPROPERTY(EditAnywhere) float Supporter = 1.0f;

	float Get(EJRPGPartyRole Role) const
	{
		switch (Role)
		{
		case EJRPGPartyRole::Defender: return Defender;
		case EJRPGPartyRole::Attacker: return Attacker;
		case EJRPGPartyRole::Supporter: return Supporter;
		default: return 1.0f;
		}
	}
};

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

	UPROPERTY() FSkillParamModifierAccumulator AoERadius;
	UPROPERTY() FSkillParamModifierAccumulator BuffDuration;
	UPROPERTY() FSkillParamModifierAccumulator DebuffPotency;
	UPROPERTY() FSkillParamModifierAccumulator BreakBuildUp;
};