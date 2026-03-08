#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Skills/JRPGSkillTypes.h"
#include "JRPGSkillDataAsset.generated.h"

/**
 * 스킬 정의(데이터 기반)
 * - C++ 로직은 SkillExecutor가 수행
 * - DA는 비용/쿨/타겟팅/이펙트만 소유(확장 안전)
 */
UCLASS()
class JRPGCOMBAT_API UJRPGSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="Skill") FName SkillId = NAME_None;
	UPROPERTY(EditDefaultsOnly, Category="Skill") FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category="Skill") EJRPGSkillTargeting Targeting = EJRPGSkillTargeting::SingleActor;
	UPROPERTY(EditDefaultsOnly, Category="Skill") FJRPGSkillCost Cost;
	UPROPERTY(EditDefaultsOnly, Category="Skill") FJRPGSkillCooldown Cooldown;

	UPROPERTY(EditDefaultsOnly, Category="Skill") TArray<FJRPGSkillEffectEntry> Effects;
	
	UPROPERTY(EditDefaultsOnly, Category = "Chain") bool bChainEligible = false;
	UPROPERTY(EditDefaultsOnly, Category = "Chain") int32 ChainTPBase = 0;
	/** 간단 정합성 체크(에디터/런타임 모두 사용 가능) */
	bool Validate(FJRPGReason& OutReason) const;
};