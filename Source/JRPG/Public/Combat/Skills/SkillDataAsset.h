#pragma once
#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "Combat/Skills/SkillTypes.h"
#include "SkillDataAsset.generated.h"

class UStatusEffectDataAsset;

UCLASS()
class JRPG_API USkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) FName SkillId = NAME_None;
	UPROPERTY(EditAnywhere) FText DisplayName;

	UPROPERTY(EditAnywhere) ESkillTargetType TargetType = ESkillTargetType::EnemySingle;

	UPROPERTY(EditAnywhere) int32 APCost = 0;
	UPROPERTY(EditAnywhere) int32 SPCost = 0;

	UPROPERTY(EditAnywhere) float CooldownSec = 0.f;

	// 효과(최소)
	UPROPERTY(EditAnywhere) float BasePower = 0.f;
	UPROPERTY(EditAnywhere) float AttackScale = 1.0f;
	UPROPERTY(EditAnywhere) float DefenseScale = 0.5f;

	UPROPERTY(EditAnywhere) float HealPower =0.f;

	UPROPERTY(EditAnywhere) TObjectPtr<UStatusEffectDataAsset> ApplyStatus = nullptr;
	UPROPERTY(EditAnywhere) float StatusChance = 1.0f;
	UPROPERTY(EditAnywhere) int32 StatusStacks = 1;

	bool IsValidSkill()const
	{
		return !SkillId.IsNone();
	}
};