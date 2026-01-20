#include "SkillComponent.h"
#include "SkillDataAsset.h"
#include "CombatSkill.h"
#include "JRPG/Combat/Stats/APComponent.h"
#include "JRPG/Combat/Skill/Skill_BasicAttack.h"

USkillComponent::USkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USkillComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeSkills();
}

void USkillComponent::AddRuntimeSkill(UCombatSkill* Skill)
{
    if (!Skill) return;
    RuntimeSkills.Add(Skill);
    Cooldowns.Add(Skill, FCooldownEntry{0.f});
}

void USkillComponent::InitializeSkills()
{
    RuntimeSkills.Reset();
    Cooldowns.Reset();
    ReservedSkill = nullptr;

    for (USkillDataAsset* Asset : SkillAssets)
    {
        if (!Asset || !Asset->SkillClass) continue;
        UCombatSkill* Skill = NewObject<UCombatSkill>(this, Asset->SkillClass);
        AddRuntimeSkill(Skill);
    }

    BasicAttack = NewObject<USkill_BasicAttack>(this, USkill_BasicAttack::StaticClass());
    AddRuntimeSkill(BasicAttack);
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    for (auto& It : Cooldowns)
        It.Value.Remaining = FMath::Max(0.f, It.Value.Remaining - DeltaTime);
}

UAPComponent* USkillComponent::GetAP() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UAPComponent>() : nullptr;
}

bool USkillComponent::IsOnCooldown(const UCombatSkill* Skill) const
{
    const FCooldownEntry* Entry = Cooldowns.Find(const_cast<UCombatSkill*>(Skill));
    return Entry ? Entry->Remaining > 0.f : false;
}

float USkillComponent::GetCooldownRemaining(const UCombatSkill* Skill) const
{
    const FCooldownEntry* Entry = Cooldowns.Find(const_cast<UCombatSkill*>(Skill));
    return Entry ? Entry->Remaining : 0.f;
}

UCombatSkill* USkillComponent::FindSkillById(FName SkillId) const
{
    for (UCombatSkill* S : RuntimeSkills)
        if (S && S->SkillId == SkillId) return S;
    return nullptr;
}

FName USkillComponent::GetReservedSkillId() const
{
    return ReservedSkill ? ReservedSkill->SkillId : NAME_None;
}

void USkillComponent::ReduceAllCooldownsByPercent(float Percent)
{
    const float P = FMath::Clamp(Percent, 0.f, 1.f);
    if (P <= 0.f) return;

    for (auto& It : Cooldowns)
        It.Value.Remaining = FMath::Max(0.f, It.Value.Remaining * (1.f - P));
}

bool USkillComponent::TryExecuteSkill(UCombatSkill* Skill, AActor* Target)
{
    if (!Skill || !GetOwner() || !Target) return false;
    if (!Skill->CanExecute(GetOwner(), Target)) return false;
    if (IsOnCooldown(Skill)) return false;

    UAPComponent* AP = GetAP();
    if (!AP) return false;

    if (Skill->APCost > 0 && !AP->CanSpend(Skill->APCost)) return false;
    if (Skill->APCost > 0) AP->Spend(Skill->APCost);

    Skill->Execute(GetOwner(), Target);
    Cooldowns.FindOrAdd(Skill).Remaining = Skill->CooldownSec;
    return true;
}

int32 USkillComponent::GetSkillSlotCount() const
{
    int32 Count = 0;
    for (UCombatSkill* S : RuntimeSkills)
        if (S && S != BasicAttack) Count++;
    return Count;
}

UCombatSkill* USkillComponent::GetSkillBySlot(int32 Slot1Based) const
{
    if (Slot1Based <= 0) return nullptr;

    int32 Index = 0;
    for (UCombatSkill* S : RuntimeSkills)
    {
        if (!S || S == BasicAttack) continue;
        Index++;
        if (Index == Slot1Based) return S;
    }
    return nullptr;
}
