#include "CombatAIComponent.h"
#include "JRPG/Combat/AI/BehaviorPresetDataAsset.h"
#include "JRPG/Combat/CombatTags.h"
#include "JRPG/Combat/Stats/HealthComponent.h"
#include "JRPG/Combat/Stats/APComponent.h"
#include "JRPG/Combat/Skill/SkillComponent.h"
#include "JRPG/Combat/Skill/CombatSkill.h"
#include "TimerManager.h"
#include "Engine/World.h"

UCombatAIComponent::UCombatAIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatAIComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UCombatAIComponent::StartAI()
{
    if (!GetWorld() || !bEnabled) return;
    GetWorld()->GetTimerManager().SetTimer(DecisionTimer, this, &UCombatAIComponent::Decide, DecisionIntervalSec, true);
}

void UCombatAIComponent::StopAI()
{
    if (!GetWorld()) return;
    GetWorld()->GetTimerManager().ClearTimer(DecisionTimer);
}

UHealthComponent* UCombatAIComponent::GetHP() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
}

UAPComponent* UCombatAIComponent::GetAP() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UAPComponent>() : nullptr;
}

USkillComponent* UCombatAIComponent::GetSkills() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<USkillComponent>() : nullptr;
}

bool UCombatAIComponent::IsEmergencySelf() const
{
    if (!Preset) return false;
    if (UHealthComponent* HP = GetHP())
        return HP->GetHPRatio() <= Preset->EmergencySelfHPRatio;
    return false;
}

float UCombatAIComponent::ScoreSkill(const UCombatSkill* Skill, bool bEmergencySelf) const
{
    if (!Preset || !Skill) return -FLT_MAX;

    float Score = 0.f;
    for (const FGameplayTag& Tag : Skill->Tags)
        Score += Preset->GetScore(Tag);

    if (bEmergencySelf && Skill->Tags.HasTagExact(CombatTags::Skill_MitSelf))
        Score += 50.f;

    const bool bIsHighDps = Skill->Tags.HasTagExact(CombatTags::Skill_DpsHigh);
    if (bIsTargeted && bIsHighDps && !Preset->bAllowDpsHighWhenTargeted)
        Score -= 9999.f;

    return Score;
}

void UCombatAIComponent::Decide()
{
    if (!bEnabled || !Preset) return;

    USkillComponent* SkillComp = GetSkills();
    UAPComponent* AP = GetAP();
    if (!SkillComp || !AP) return;

    AActor* Target = MainTarget.Get();
    if (!Target) return;

    // 1) 예약 스킬 최우선
    if (SkillComp->HasReservedSkill())
    {
        UCombatSkill* Reserved = SkillComp->GetReservedSkill();
        if (SkillComp->TryExecuteSkill(Reserved, Target))
        {
            SkillComp->ClearReservedSkill();
            return;
        }

        if (UCombatSkill* Basic = SkillComp->GetBasicAttack())
            SkillComp->TryExecuteSkill(Basic, Target);
        return;
    }

    // 2) 프리셋 점수 기반 선택
    const bool bEmergencySelf = IsEmergencySelf();

    UCombatSkill* BestSkill = nullptr;
    float BestScore = -FLT_MAX;

    for (UCombatSkill* Skill : SkillComp->GetSkills())
    {
        if (!Skill) continue;

        const int32 RemainingAfter = AP->CurrentAP - Skill->APCost;
        if (RemainingAfter < Preset->MinAPReserve) continue;

        if (bIsTargeted && Preset->bForceDpsCutWhenTargeted)
        {
            if (Skill->Tags.HasTagExact(CombatTags::Skill_DpsHigh))
                continue;
        }

        const float Score = ScoreSkill(Skill, bEmergencySelf);
        if (Score > BestScore)
        {
            BestScore = Score;
            BestSkill = Skill;
        }
    }

    if (BestSkill)
    {
        if (!SkillComp->TryExecuteSkill(BestSkill, Target))
        {
            if (UCombatSkill* Basic = SkillComp->GetBasicAttack())
                SkillComp->TryExecuteSkill(Basic, Target);
        }
    }
}
