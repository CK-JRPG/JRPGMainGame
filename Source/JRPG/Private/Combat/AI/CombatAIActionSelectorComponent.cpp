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
	if (!P || P->GetCombatTeam() != ECombatTeam::Enemy) return;

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