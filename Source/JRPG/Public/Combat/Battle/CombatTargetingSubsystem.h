// Source/JRPGCombat/Public/Combat/Battle/CombatTargetingSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Battle/CombatTargetingTypes.h"
#include "Combat/Skills/SkillDataAsset.h"

#include "CombatTargetingSubsystem.generated.h"

UCLASS()
class JRPG_API UCombatTargetingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 기본 공격용 “선호 타겟”
	FTargetingResult ResolvePreferredBasicAttackTarget(AActor *Requester) const;

	// 스킬용 “선호 타겟 세트”
	FTargetingResult ResolvePreferredTargetsForSkill(AActor *Requester,const USkillDataAsset *Skill) const;

	// 사용자가 고른 타겟이 합법적인지 검사
	FTargetValidationResult ValidateSkillTargets(AActor *Requester,const USkillDataAsset *Skill,const TArray<AActor*> &Targets) const;

	FTargetValidationResult ValidateBasicAttackTarget(AActor *Requester,AActor *Target)const;

private:
	class UBattleSessionSubsystem* GetBattle()const;

	bool IsAliveCombatant(AActor *Actor) const;
	bool IsSameTeam(AActor *A,AActor *B) const;
	bool IsEnemyTeam(AActor *A,AActor *B) const;

	float GetHPRatio(AActor *Actor) const;

	AActor *PickTopThreatTarget(AActor *Requester,const TArray<AActor*> &Candidates) const;
	AActor *PickLowestHPActor(const TArray<AActor*> &Candidates) const;
	AActor *PickLowestHPAllyIncludingSelf(AActor *Requester,const TArray<AActor*> &Allies) const;

	void GetAlliesIncludingSelf(AActor *Requester,TArray<AActor*> &Out) const;
};