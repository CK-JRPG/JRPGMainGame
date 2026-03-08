// Source/JRPGCombat/Public/Combat/Items/CombatUsableItemDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "GameplayTagContainer.h"
#include "Combat/Skills/JRPGSkillTypes.h"
#include "CombatUsableItemDataAsset.generated.h"

class UStatusEffectDataAsset;

UCLASS()
class JRPG_API UCombatUsableItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	// 아이템도 스킬과 같은 타겟 규칙을 재사용
	UPROPERTY(EditAnywhere)
	ESkillTargetType TargetType = ESkillTargetType::AllySingle;

	UPROPERTY(EditAnywhere)
	bool bConsumeOnUse = true;
	UPROPERTY(EditAnywhere)
	bool bBattleOnly = true;

	// 회복/자원
	UPROPERTY(EditAnywhere)
	float HealHP = 0.f;
	UPROPERTY(EditAnywhere)
	int32 RestoreAP = 0;
	UPROPERTY(EditAnywhere)
	int32 GrantSP = 0;

	// 공격형 아이템(폭탄/투척류 확장)
	UPROPERTY(EditAnywhere)
	float FlatDamage = 0.f;
	UPROPERTY(EditAnywhere)
	float FlatGroggyDamage = 0.f;
	UPROPERTY(EditAnywhere)
	float FlatThreatToUserOnTarget = 0.f;

	// 버프/디버프 부여
	UPROPERTY(EditAnywhere)
	TObjectPtr<UStatusEffectDataAsset> ApplyStatus = nullptr;
	UPROPERTY(EditAnywhere)
	float StatusChance = 1.f;
	UPROPERTY(EditAnywhere)
	int32 StatusStacks = 1;

	// 정화/해제
	// 예: Status.Group.CC, Status.Group.Debuff, Status.Group.DoT
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer DispelAnyTags;
	UPROPERTY(EditAnywhere)
	int32 DispelRemoveCount = 0; // <=0 means all

	bool IsValidItem() const
	{
		return !ItemId.IsNone();
	}

	bool HasAnyRuntimeEffect() const
	{
		return HealHP > 0.f
			|| RestoreAP > 0
			|| GrantSP > 0
			|| FlatDamage > 0.f
			|| FlatGroggyDamage > 0.f
			|| FlatThreatToUserOnTarget != 0.f
			|| ApplyStatus != nullptr
			|| DispelAnyTags.Num() > 0;
	}
};
