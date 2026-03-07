#pragma once
#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "Animation/AnimMontage.h"
#include "Combat/Skills/SkillTypes.h"
#include "Combat/Presentation/CombatPresentationTypes.h"
#include "SkillDataAsset.generated.h"

class UStatusEffectDataAsset;

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

	UPROPERTY(EditAnywhere) bool bAllowCrit = true;
	UPROPERTY(EditAnywhere) float VarianceMin = 0.95f;
	UPROPERTY(EditAnywhere) float VarianceMax = 1.05f;

	UPROPERTY(EditAnywhere) float HealPower = 0.f;

	UPROPERTY(EditAnywhere) float GroggyPower = 0.f;
	UPROPERTY(EditAnywhere) float ThreatBase = 0.f;
	UPROPERTY(EditAnywhere) float ThreatFromDamageMul = 1.0f;

	UPROPERTY(EditAnywhere) TObjectPtr<UStatusEffectDataAsset> ApplyStatus = nullptr;
	UPROPERTY(EditAnywhere) float StatusChance = 1.0f;
	UPROPERTY(EditAnywhere) int32 StatusStacks = 1;

	// Presentation
	UPROPERTY(EditAnywhere) TObjectPtr<UAnimMontage> CastMontage = nullptr;
	UPROPERTY(EditAnywhere) ECombatResolveTiming ResolveTiming = ECombatResolveTiming::AnimNotifyWindow;
	UPROPERTY(EditAnywhere) FName StartCueTag = "Skill.Start";
	UPROPERTY(EditAnywhere) FName HitCueTag = "Skill.Hit";
	UPROPERTY(EditAnywhere) FName FinishCueTag = "Skill.Finish";

	bool IsValidSkill()const { return !SkillId.IsNone(); }
};