// Source/JRPGCombat/Private/Combat/AI/CombatAIActionSelectorComponent.cpp
#include "Combat/AI/CombatAIActionSelectorComponent.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/CombatTargetingSubsystem.h"

#include "Combat/Skills/SkillComponent.h"
#include "Combat/Skills/SkillDataAsset.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

#include "Combat/Characters/CombatParticipantInterface.h"

#include "Engine/World.h"
#include "TimerManager.h"

UCombatAIActionSelectorComponent::UCombatAIActionSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatAIActionSelectorComponent::BeginPlay()
{
	Super::BeginPlay();

	SkillComp = GetOwner() ? GetOwner()->FindComponentByClass<USkillComponent>() : nullptr;

	if (UBattleSessionSubsystem *Battle = GetBattle())
	{
		Battle->OnTurnStarted.AddUObject(this, &UCombatAIActionSelectorComponent::HandleTurnStarted);
	}
}

void UCombatAIActionSelectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ThinkTimer);
	}

	if (UBattleSessionSubsystem *Battle =GetBattle())
	{
		Battle->OnTurnStarted.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

UBattleSessionSubsystem* UCombatAIActionSelectorComponent::GetBattle() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

UCombatTargetingSubsystem* UCombatAIActionSelectorComponent::GetTargeting() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTargetingSubsystem>() : nullptr;
}

float UCombatAIActionSelectorComponent::GetHPRatio(AActor *Actor) const
{
	if (!Actor)
		return 1.f;

	if (ICombatParticipantInterface *P =Cast<ICombatParticipantInterface>(Actor))
	{
		if (UHPComponent *HP = P->GetHP())
		{
			const float Max = FMath::Max(1.f,HP->GetMaxHP());
			return HP->GetHP() / Max;
		}
	}
	return 1.f;
}

bool UCombatAIActionSelectorComponent::CanAffordSkill(const USkillDataAsset &Skill) const
{
	if (!GetOwner())return false;

	UAPComponent *AP = GetOwner()->FindComponentByClass<UAPComponent>();
	USPComponent *SP = GetOwner()->FindComponentByClass<USPComponent>();
	if (!AP || !SP)return false;

	if (Skill.APCost>0 && !AP->CanConsume(Skill.APCost))
		return false;
	
	if (Skill.SPCost>0 && SP->GetSP() < Skill.SPCost)
		return false;

	if (SkillComp.IsValid() && SkillComp->GetCooldownRemaining(Skill.SkillId) > 0.f)
		return false;

	return true;
}

void UCombatAIActionSelectorComponent::HandleTurnStarted(AActor *Actor, int32)
{
	if (!bAutoDriveEnemyTurns) 
		return;
	
	if (!GetOwner() || Actor != GetOwner())
		return;

	ICombatParticipantInterface *P =Cast<ICombatParticipantInterface>(GetOwner());
	
	if (!P)
		return;
	
	if (P->GetCombatTeam()!= ECombatTeam::Enemy)
		return;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(ThinkTimer,this, &UCombatAIActionSelectorComponent::ThinkAndAct,ThinkDelaySec,false);
	}
}

USkillDataAsset* UCombatAIActionSelectorComponent::PickBestHealSkill(TArray<AActor*> &OutTargets) const
{
	OutTargets.Reset();
	if (!SkillComp.IsValid())return nullptr;

	float LowestObserved =1.f;
	TArray<AActor*> Allies;

	if (UBattleSessionSubsystem*Battle =GetBattle())
	{
		Battle->GetAlliesFor(GetOwner(),Allies);
	}
	Allies.AddUnique(GetOwner());

	for (AActor *A : Allies)
	{
		LowestObserved = FMath::Min(LowestObserved,GetHPRatio(A));
	}

	if (LowestObserved>HealThresholdRatio)
		return nullptr;

	float BestScore =- FLT_MAX;
	USkillDataAsset *BestSkill = nullptr;
	UCombatTargetingSubsystem *Targeting =GetTargeting();
	
	if (!Targeting)	
		return nullptr;

	for (USkillDataAsset*Skill :SkillComp->KnownSkills)
	{
		if (!Skill || !Skill->IsValidSkill())	continue;
		if (Skill->HealPower <= 0.f)			continue;
		if (!CanAffordSkill(*Skill))			continue;

		const FTargetingResult T = Targeting->ResolvePreferredTargetsForSkill(GetOwner(), Skill);
		if (!T.bOk || T.Targets.Num() <= 0) 
			continue;

		TArray<AActor*>Resolved;
		for (const TWeakObjectPtr<AActor> &W : T.Targets)
		{
			if (AActor*A = W.Get())Resolved.Add(A);
		}
		if (Resolved.Num() <= 0)
			continue;

		float MissingWeight = 0.f;
		for (AActor *A : Resolved)
		{
			MissingWeight += (1.f-GetHPRatio(A));
		}

		float Score = Skill-> HealPower * 1.5f + MissingWeight * 50.f;
		if (LowestObserved <= EmergencyHealThresholdRatio)
		{
			Score += 30.f;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestSkill = Skill;
			OutTargets = Resolved;
		}
	}

	return BestSkill;
}

USkillDataAsset* UCombatAIActionSelectorComponent::PickBestOffensiveSkill(TArray<AActor*>&OutTargets)const
{
	OutTargets.Reset();
	
	if (!SkillComp.IsValid())
		return nullptr;

	float BestScore =- FLT_MAX;
	USkillDataAsset *BestSkill = nullptr;
	UCombatTargetingSubsystem *Targeting =GetTargeting();
	if (!Targeting)return nullptr;

	for (USkillDataAsset*Skill : SkillComp->KnownSkills)
	{
		if (!Skill || !Skill->IsValidSkill())	continue;
		if (Skill->HealPower>0.f)				continue;/ / 힐은 다른 단계에서 처리
		if (!CanAffordSkill(*Skill))			continue;

		const FTargetingResult T = Targeting->ResolvePreferredTargetsForSkill(GetOwner(),Skill);
		if (!T.bOk || T.Targets.Num() <= 0)
			continue;

		TArray<AActor*> Resolved;
		for (const TWeakObjectPtr<AActor> &W : T.Targets)
		{
			if (AActor *A = W.Get())
				Resolved.Add(A);
		}
		if (Resolved.Num() <= 0)
			continue;

		float Score = 0.f;
		Score += Skill->BasePower;
		Score += Skill->AttackScale * 20.f;
		Score += Skill->GroggyPower * 1.2f;
		Score += Skill->ThreatBase * 0.25f;
		Score += Skill->ApplyStatus ? 10.f : 0.f;
		Score += (Skill->TargetType == ESkillTargetType::EnemyAll) ? 15.f : 0.f;

		Score -= (float)Skill->APCost * 0.7f;
		Score -= (float)Skill->SPCost * 0.4f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestSkill = Skill;
			OutTargets = Resolved;
		}
	}

	return BestSkill;
}

void UCombatAIActionSelectorComponent::ThinkAndAct()
{
	UBattleSessionSubsystem *Battle = GetBattle();
	UCombatTargetingSubsystem *Targeting = GetTargeting();
	
	if (!Battle||!Targeting||!GetOwner())	 return;
	if (!Battle->IsBattleActive())			 return;
	if (!Battle->CanActorActNow(GetOwner())) return;

	// 1) 힐 필요하면 힐 우선
	TArray<AActor*> HealTargets;
	if (USkillDataAsset *HealSkill = PickBestHealSkill(HealTargets))
	{
		const FSkillCastResult R = Battle->TryExecuteSkill(GetOwner(), HealSkill->SkillId, HealTargets);
		
		if (R.bOk)
			return;
	}

	// 2) 공격 스킬
	TArray<AActor*> OffensiveTargets;
	if (USkillDataAsset *OffensiveSkill = PickBestOffensiveSkill(OffensiveTargets))
	{
		const FSkillCastResult R = Battle->TryExecuteSkill(GetOwner(), OffensiveSkill->SkillId, OffensiveTargets);
		
		if (R.bOk)
			return;
	}

	// 3) 기본 공격
	const FTargetingResult BasicTarget = Targeting->ResolvePreferredBasicAttackTarget(GetOwner());
	if (BasicTarget.bOk && BasicTarget.Targets.Num()>0)
	{
		if (AActor *Target = BasicTarget.Targets[0].Get())
		{
			const FCombatActionResult R =Battle->TryExecuteBasicAttack(GetOwner(),Target);
			if (R.bOk)
				return;
		}
	}

	// 4) 정말 아무 것도 못 하면 턴 종료
	Battle->FinishCurrentTurn("AI.NoValidAction");
}