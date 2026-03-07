#include "Combat/Skills/SkillComponent.h"

#include "Combat/Characters/Stats/CombatStatsComponent.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Status/StatusEffectDataAsset.h"

USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void USkillComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Stats = GetOwner() ? GetOwner()->FindComponentByClass<UCombatStatsComponent>() : nullptr;
	HP = GetOwner() ? GetOwner()->FindComponentByClass<UHPComponent>() : nullptr;
	AP = GetOwner() ? GetOwner()->FindComponentByClass<UAPComponent>() : nullptr;
	SP = GetOwner() ? GetOwner()->FindComponentByClass<USPComponent>() : nullptr;

	for (USkillDataAsset *S : KnownSkills)
	{
		if (S && S->IsValidSkill() && !Cooldowns.Contains(S->SkillId))
			Cooldowns.Add(S->SkillId, 0.f);
	}
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	for (auto &KV : Cooldowns)
	{
		if (KV.Value > 0.f)
			KV.Value = FMath::Max(0.f, KV.Value - DeltaTime);
	}
}

USkillDataAsset* USkillComponent::FindSkill(FName SkillId)const
{
	for (USkillDataAsset *S : KnownSkills)
		if (S && S->SkillId == SkillId)
			return S;
	
	return nullptr;
}

bool USkillComponent::HasSkill(FName SkillId) const
{
	return FindSkill(SkillId) != nullptr;
}

void USkillComponent::LearnSkill(USkillDataAsset *Skill)
{
	if (!Skill || !Skill->IsValidSkill())
		return;
	
	if (HasSkill(Skill->SkillId))
		return;
	
	KnownSkills.Add(Skill);
	Cooldowns.Add(Skill->SkillId, 0.f);
}

float USkillComponent::GetCooldownRemaining(FName SkillId) const
{
	const float *V = Cooldowns.Find(SkillId);
	return V ? *V : 0.f;
}

FSkillCastResult USkillComponent::ValidateCast(const USkillDataAsset &Skill, const TArray<AActor*> &Targets) const
{
	if (!AP.IsValid() || !SP.IsValid())
		return FSkillCastResult::Fail("Reject.MissingResource");
	
	if (Skill.APCost>0 && !AP->CanConsume(Skill.APCost))
		return FSkillCastResult::Fail("Reject.NotEnoughAP");
	
	if (Skill.SPCost>0 && SP->GetSP() < Skill.SPCost)
		return FSkillCastResult::Fail("Reject.NotEnoughSP");

	if (GetCooldownRemaining(Skill.SkillId) > 0.f)
		return FSkillCastResult::Fail("Reject.Cooldown");

	// 최소: 대상 0이면 실패(자기 자신 스킬도 Targets에 자기 포함해서 호출)
	if (Targets.Num() <= 0)
		return FSkillCastResult::Fail("Reject.InvalidTarget");

	return FSkillCastResult::Ok();
}

float USkillComponent::ComputeDamageAgainst(AActor*Target,const USkillDataAsset&Skill)const
{
	const float Atk = Stats.IsValid() ? Stats->GetSnapshot().Attack : 10.f;

	float TDef = 5.f;
	if (Target)
	{
		if (UCombatStatsComponent *TS = Target->FindComponentByClass<UCombatStatsComponent>())
			TDef = TS->GetSnapshot().Defense;
	}

	const float Raw = Skill.BasePower + Atk * Skill.AttackScale - TDef * Skill.DefenseScale;
	return FMath::Max(1.f,Raw);
}

void USkillComponent::ApplySkillEffects(const USkillDataAsset &Skill,const TArray<AActor*> &Targets)
{
	for (AActor*T :Targets)
	{
		if (!T)continue;

		// Damage
		if (Skill.BasePower > 0.f || Skill.AttackScale != 0.f)
		{
			if (UHPComponent *THP = T->FindComponentByClass<UHPComponent>())
			{
				const float Dmg =ComputeDamageAgainst(T, Skill);
				THP->ApplyDamage(Dmg, GetOwner(), Skill.SkillId);
			}
		}

		// Heal
		if (Skill.HealPower>0.f)
		{
			if (UHPComponent *THP = T->FindComponentByClass<UHPComponent>())
			{
				THP->Heal(Skill.HealPower, GetOwner(), Skill.SkillId);
			}
		}

		// Status
		if (Skill.ApplyStatus&& FMath::FRand() <= FMath::Clamp(Skill.StatusChance, 0.f, 1.f))
		{
			if (UStatusEffectComponent *SEC = T->FindComponentByClass<UStatusEffectComponent>())
			{
				SEC->ApplyStatus(Skill.ApplyStatus, GetOwner(), Skill.StatusStacks, Skill.SkillId);
			}
		}
	}
}

FSkillCastResult USkillComponent::CastSkill(FNameSkillId,constTArray<AActor*>&Targets,FName)
{
	USkillDataAsset*Skill =FindSkill(SkillId);
	if (!Skill)return FSkillCastResult::Fail("Reject.SkillNotFound");

	const FSkillCastResultV =ValidateCast(*Skill,Targets);
	if (!V.bOk)returnV;

	// consume
	if (Skill->APCost > 0 && !AP->Consume(Skill->APCost,SkillId))
		return FSkillCastResult::Fail("Reject.NotEnoughAP");
	
	if (Skill->SPCost > 0 && !SP->ConsumeSP(Skill->SPCost, SkillId))
	{
		AP->Restore(Skill->APCost,"Rollback");
		return FSkillCastResult::Fail("Reject.NotEnoughSP");
	}

	// set cooldown
	Cooldowns.FindOrAdd(SkillId) = FMath::Max(0.f, Skill->CooldownSec);

	// apply effects
	ApplySkillEffects(*Skill, Targets);

	OnSkillCast.Broadcast(SkillId, GetOwner(), Targets.Num());
	return FSkillCastResult::Ok();
}