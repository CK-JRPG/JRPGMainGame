#include "Combat/AI/CombatAIActionSelectorComponent.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Battle/CombatTargetingSubsystem.h"

#include "Combat/Presentation/CombatPresentationComponent.h"
#include "Combat/Skills/SkillComponent.h"
#include "JRPG/Public/Combat/Skills/SkillDataAsset.h"

#include "JRPG/Public/Combat/Stats/HPComponent.h"
#include "JRPG/Public/Combat/Stats/APComponent.h"
#include "JRPG/Public/Combat/SP/SPComponent.h"
#include "Combat/Characters/CombatParticipantInterface.h"

UCombatAIActionSelectorComponent::UCombatAIActionSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick =true;
	PrimaryComponentTick.TickInterval =0.05f;
}

void UCombatAIActionSelectorComponent::BeginPlay()
{
	Super::BeginPlay();

	SkillComp = GetOwner() ? GetOwner()->FindComponentByClass<USkillComponent>() : nullptr;
	PresentationComp = GetOwner() ? GetOwner()->FindComponentByClass<UCombatPresentationComponent>() : nullptr;
}

void UCombatAIActionSelectorComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	if (!bAutoDriveEnemyAI || !GetOwner()) 
		return;

	ThinkAccumulator += DeltaTime;
	if (ThinkAccumulator<ThinkIntervalSec) 
		return;
	
	ThinkAccumulator = 0.f;

	UBattleSessionSubsystem* Battle = GetBattle();
	
	if (!Battle || !Battle->IsBattleActive()) return;
	if (Battle->GetPhase() != EBattlePhase::Active) return;
	if (!Battle->CanActorExecuteAction(GetOwner())) return;

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(GetOwner());
	if (!P || P->GetCombatTeam() == ECombatTeam::Neutral) return;

	ThinkAndAct();
}

UBattleSessionSubsystem* UCombatAIActionSelectorComponent::GetBattle() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

UCombatTargetingSubsystem* UCombatAIActionSelectorComponent::GetTargeting() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UCombatTargetingSubsystem>() : nullptr;
}

float UCombatAIActionSelectorComponent::GetHPRatio(AActor* Actor)const
{
	if (!Actor)
		return 1.f;

	if (ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor))
	{
		if (UHPComponent* HP = P->GetHP())
		{
			const float Max = FMath::Max(1.f,HP->GetMaxHP());
			return HP->GetHP() / Max;
		}
	}
	return 1.f;
}

bool UCombatAIActionSelectorComponent::CanAffordSkill(const USkillDataAsset &Skill) const
{
	if (!GetOwner())
		return false;

	UAPComponent*AP =GetOwner()->FindComponentByClass<UAPComponent>();
	USPComponent*SP =GetOwner()->FindComponentByClass<USPComponent>();
	
	if (!AP || !SP) 
		return false;
	if (Skill.APCost > 0 && !AP->CanConsume(Skill.APCost)) 
		return false;
	if (Skill.SPCost > 0 && SP->GetSP() < Skill.SPCost) 
		return false;

	if (SkillComp.IsValid()&&SkillComp->GetCooldownRemaining(Skill.SkillId) > 0.f)
		return false;

	return true;
}

USkillDataAsset* UCombatAIActionSelectorComponent::PickBestHealSkill(TArray<AActor*>& OutTargets) const
{
	if (!SkillComp.IsValid())
		return nullptr;

	UCombatTargetingSubsystem* Targeting = GetTargeting();
	if (!Targeting)
		return nullptr;

	// 아군 중 HP가 HealThresholdRatio 이하인 대상이 있는지 먼저 확인
	UBattleSessionSubsystem* Battle = GetBattle();
	if (!Battle)
		return nullptr;

	TArray<AActor*> Allies;
	Battle->GetAlliesFor(GetOwner(), Allies);
	if (!Allies.Contains(GetOwner()))
		Allies.Add(GetOwner());

	bool bNeedHeal = false;
	for (AActor* Ally : Allies)
	{
		if (GetHPRatio(Ally) <= HealThresholdRatio)
		{
			bNeedHeal = true;
			break;
		}
	}

	if (!bNeedHeal)
		return nullptr;

	// KnownSkills에서 힐 스킬 중 사용 가능한 최고의 스킬 선택
	USkillDataAsset* BestSkill = nullptr;
	float BestHealPower = 0.f;
	TArray<AActor*> BestTargets;

	for (USkillDataAsset* Skill : SkillComp->KnownSkills)
	{
		if (!Skill || Skill->HealPower <= 0.f)
			continue;

		if (!CanAffordSkill(*Skill))
			continue;

		const FTargetingResult Result = Targeting->ResolvePreferredTargetsForSkill(GetOwner(), Skill);
		if (!Result.bOk || Result.Targets.Num() <= 0)
			continue;

		if (Skill->HealPower > BestHealPower)
		{
			BestHealPower = Skill->HealPower;
			BestSkill = Skill;
			BestTargets.Reset();
			for (const TWeakObjectPtr<AActor>& T : Result.Targets)
			{
				if (T.IsValid())
					BestTargets.Add(T.Get());
			}
		}
	}

	if (BestSkill)
	{
		OutTargets = MoveTemp(BestTargets);
	}

	return BestSkill;
}

USkillDataAsset* UCombatAIActionSelectorComponent::PickBestOffensiveSkill(TArray<AActor*>& OutTargets) const
{
	if (!SkillComp.IsValid())
		return nullptr;

	UCombatTargetingSubsystem* Targeting = GetTargeting();
	if (!Targeting)
		return nullptr;

	USkillDataAsset* BestSkill = nullptr;
	float BestScore = 0.f;
	TArray<AActor*> BestTargets;

	for (USkillDataAsset* Skill : SkillComp->KnownSkills)
	{
		if (!Skill || Skill->BasePower <= 0.f)
			continue;

		if (!CanAffordSkill(*Skill))
			continue;

		const FTargetingResult Result = Targeting->ResolvePreferredTargetsForSkill(GetOwner(), Skill);
		if (!Result.bOk || Result.Targets.Num() <= 0)
			continue;

		// BasePower * AttackScale 기반 점수, AoE는 타겟 수 보너스
		const float Score = Skill->BasePower * Skill->AttackScale * FMath::Max(1, Result.Targets.Num());

		if (Score > BestScore)
		{
			BestScore = Score;
			BestSkill = Skill;
			BestTargets.Reset();
			for (const TWeakObjectPtr<AActor>& T : Result.Targets)
			{
				if (T.IsValid())
					BestTargets.Add(T.Get());
			}
		}
	}

	if (BestSkill)
	{
		OutTargets = MoveTemp(BestTargets);
	}

	return BestSkill;
}

// PickBestHealSkill / PickBestOffensiveSkill는 이전 버전 그대로 사용 가능

void UCombatAIActionSelectorComponent::ThinkAndAct()
{
	UBattleSessionSubsystem* Battle = GetBattle();
	UCombatTargetingSubsystem* Targeting = GetTargeting();
	
	if (!Battle || !Targeting || !PresentationComp.IsValid()) 
		return;

	TArray<AActor*> HealTargets;
	if (USkillDataAsset* HealSkill = PickBestHealSkill(HealTargets))
	{
		const FSkillCastResult R = PresentationComp->TryPresentSkill(HealSkill->SkillId,HealTargets,false);
		if (R.bOk)
			return;
	}

	TArray<AActor*> OffensiveTargets;
	if (USkillDataAsset* OffensiveSkill = PickBestOffensiveSkill(OffensiveTargets))
	{
		const FSkillCastResult R =PresentationComp->TryPresentSkill(OffensiveSkill->SkillId,OffensiveTargets,false);
		if (R.bOk) 
			return;
	}

	const FTargetingResult BasicTarget = Targeting->ResolvePreferredBasicAttackTarget(GetOwner());
	if (BasicTarget.bOk && BasicTarget.Targets.Num() > 0)
	{
		if (AActor* Target = BasicTarget.Targets[0].Get())
		{
			const FCombatActionResult R = PresentationComp->TryPresentBasicAttack(Target);
			if (R.bOk)
				return;
		}
	}
}