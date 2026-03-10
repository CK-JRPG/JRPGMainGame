// Source/JRPGCombat/Public/Combat/AI/CombatAIScorer.h

#pragma once

#include "CoreMinimal.h"
#include "Combat/AI/CombatAIActionTypes.h"
#include "Combat/AI/CombatAIContext.h"
#include "CombatAIScorer.generated.h"

USTRUCT()
struct FSkillAIMeta
{
	GENERATED_BODY()

	UPROPERTY() bool bIsHeal = false;
	UPROPERTY() bool bIsCleanse = false;
	UPROPERTY() bool bIsTaunt = false;
	UPROPERTY() bool bIsBuff = false;
	UPROPERTY() bool bIsDebuff = false;
	UPROPERTY() bool bIsBreak = false;
	UPROPERTY() bool bIsHighDps = false;
};

// NOTE: 이 부분은 프로젝트의 SkillDataAsset/SkillComponent 구현에 맞춰 채우면 됨.
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FGetSkillAIMetaDelegate, class USkillComponent* /*SkillComp*/, FName /*SkillId*/, FSkillAIMeta& /*OutMeta*/);

UCLASS()
class JRPG_API UCombatAIScorer :public UObject
{
	GENERATED_BODY()
 
public:
	void Initialize(FGetSkillAIMetaDelegate InMetaResolver);
 
	float ScoreAction(const UCombatAIContext &Ctx,const FJRPGCombatAIAction &Action) const;
 
private:
	FGetSkillAIMetaDelegate MetaResolver;
 
	float ScoreRoleLogic(const UCombatAIContext &Ctx,const FJRPGCombatAIAction &A,const FSkillAIMeta &Meta) const;
	float ScoreSPOpportunity(const UCombatAIContext &Ctx,const FJRPGCombatAIAction &A,const FSkillAIMeta &Meta) const;
 
	static float SoftCapPenalty(float Value,float SoftCap);
};