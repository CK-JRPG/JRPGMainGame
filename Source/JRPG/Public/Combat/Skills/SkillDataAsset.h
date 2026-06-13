#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "Animation/AnimMontage.h"
#include "JRPG/Public/Combat/Skills/SkillTypes.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "Combat/Motion/CombatMotionTypes.h"
#include "SkillDataAsset.generated.h"

class UStatusEffectDataAsset;
class UNiagaraSystem;
class UCurveFloat;

UCLASS()
class JRPG_API USkillDataAsset :public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) FName SkillId = NAME_None;
	UPROPERTY(EditAnywhere) FText DisplayName;

	UPROPERTY(EditAnywhere) ESkillTargetType TargetType = ESkillTargetType::EnemySingle;

	UPROPERTY(EditAnywhere) int32 APCost = 0;
	UPROPERTY(EditAnywhere) int32 SPCost = 0;
	UPROPERTY(EditAnywhere) float CooldownSec = 0.f;

	UPROPERTY(EditAnywhere) float BasePower = 0.f;
	UPROPERTY(EditAnywhere) float AttackScale = 1.0f;
	UPROPERTY(EditAnywhere) float DefenseScale = 0.5f;
	UPROPERTY(EditAnywhere, Category="Damage|Directional") bool bEnableDirectionalDamageBonus = false;
	UPROPERTY(EditAnywhere, Category="Damage|Directional", meta=(ClampMin="-1.0", ClampMax="1.0", EditCondition="bEnableDirectionalDamageBonus"))
	float DirectionalBackDotThreshold = -0.5f;
	UPROPERTY(EditAnywhere, Category="Damage|Directional", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnableDirectionalDamageBonus"))
	float DirectionalSideDotThreshold = 0.5f;
	UPROPERTY(EditAnywhere, Category="Damage|Directional", meta=(ClampMin="1.0", EditCondition="bEnableDirectionalDamageBonus"))
	float DirectionalBackDamageMultiplier = 2.0f;
	UPROPERTY(EditAnywhere, Category="Damage|Directional", meta=(ClampMin="1.0", EditCondition="bEnableDirectionalDamageBonus"))
	float DirectionalSideDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere) bool bAllowCrit = true;
	UPROPERTY(EditAnywhere) float VarianceMin = 0.95f;
	UPROPERTY(EditAnywhere) float VarianceMax = 1.05f;

	UPROPERTY(EditAnywhere) float HealPower = 0.f;

	UPROPERTY(EditAnywhere) float GroggyPower = 0.f;
	UPROPERTY(EditAnywhere, Category="Threat") float ThreatBase = 0.f;
	UPROPERTY(EditAnywhere, Category="Threat") float ThreatFromDamageMul = 1.0f;
	UPROPERTY(EditAnywhere, Category="Threat") float ThreatFromHealMul = 0.85f;

	UPROPERTY(EditAnywhere) TObjectPtr<UStatusEffectDataAsset> ApplyStatus = nullptr;
	UPROPERTY(EditAnywhere) float StatusChance = 1.0f;
	UPROPERTY(EditAnywhere) int32 StatusStacks = 1;

	// Presentation
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimMontage> CastMontage = nullptr;
	UPROPERTY(EditAnywhere) ECombatResolveTiming ResolveTiming = ECombatResolveTiming::AnimNotifyWindow;
	UPROPERTY(EditAnywhere) FName StartCueTag = "Skill.Start";
	UPROPERTY(EditAnywhere) FName HitCueTag = "Skill.Hit";
	UPROPERTY(EditAnywhere) FName FinishCueTag = "Skill.Finish";
	UPROPERTY(EditAnywhere) bool bHasSkillMotion = false;
	UPROPERTY(EditAnywhere) FJRPGCombatMotionRequest SkillMotion;
	
	UPROPERTY(EditAnywhere, Category="Presentation|Camera")
	FCombatCameraShakeSpec CameraShake;

	UPROPERTY(EditAnywhere, Category="Presentation|VFX") TObjectPtr<UNiagaraSystem> HitNiagaraEffect = nullptr;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX") TObjectPtr<UNiagaraSystem> OnResolveTargetEffect = nullptr;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX") TObjectPtr<UNiagaraSystem> OnResolveGroundEffect = nullptr;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX", meta=(ClampMin="0.0")) float GroundEffectDuration = 0.f;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX") TObjectPtr<UCurveFloat> DamageToEffectScaleCurve = nullptr;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX", meta=(ClampMin="0.0")) float EffectScaleReferenceDamage = 100.f;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX", meta=(ClampMin="0.0")) float EffectScaleMultiplier = 0.25f;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX", meta=(ClampMin="1.0")) float MinEffectScale = 1.f;
	UPROPERTY(EditAnywhere, Category="Presentation|VFX", meta=(ClampMin="1.0")) float MaxEffectScale = 3.f;

	UPROPERTY(EditAnywhere) FGameplayTagContainer DispelAnyTags;
	UPROPERTY(EditAnywhere) int32 DispelRemoveCount = 0;// <=0 means all
	
	// AI utility tags (ex: Damage, Heal, Taunt, Buff, Debuff, Shield, Escape, AOE...)
	UPROPERTY(EditAnywhere, Category = "AI") FGameplayTagContainer AITags;

	bool IsValidSkill() const { return !SkillId.IsNone(); }
};
