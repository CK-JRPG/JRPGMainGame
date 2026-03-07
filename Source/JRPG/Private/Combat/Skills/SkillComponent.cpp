#include "Combat/Skills/SkillComponent.h"

#include "Combat/Battle/CombatFormulaLibrary.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"
#include "Combat/SP/SPComponent.h"

#include "Combat/Status/StatusEffectComponent.h"
#include "Combat/Status/StatusEffectDataAsset.h"
#include "Combat/Groggy/GroggyComponent.h"
#include "Combat/Threat/ThreatComponent.h"

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

	for (USkillDataAsset*S : KnownSkills)
	{
		if (S && S->IsValidSkill() && !Cooldowns.Contains(S->SkillId))
			Cooldowns.Add(S->SkillId, 0.f);
	}
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	for (auto &KV : Cooldowns)
	{
		if (KV.Value>0.f) 
			KV.Value = FMath::Max(0.f,KV.Value - DeltaTime);
	}
}

USkillDataAsset* USkillComponent::GetSkillDef(FName SkillId) const
{
	for (USkillDataAsset *S : KnownSkills)
	{
		if (S&&S->SkillId == SkillId)
			return S;
	}
	return nullptr;
}

bool USkillComponent::HasSkill(FName SkillId) const
{
	return GetSkillDef(SkillId) != nullptr;
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

bool USkillComponent::IsHostileTarget(AActor *Target)const
{
	if (!GetOwner() || !Target)returnfalse;

	ICombatParticipantInterface *A =Cast<ICombatParticipantInterface>(GetOwner());
	ICombatParticipantInterface *T =Cast<ICombatParticipantInterface>(Target);
	
	if (!A||!T)
		return false;

	const ECombatTeam TA = A->GetCombatTeam();
	const ECombatTeam TT = T->GetCombatTeam();
	if (TA == ECombatTeam::Neutral || TT == ECombatTeam::Neutral)
		return false;
	
	return TA != TT;
}

FSkillCastResult USkillComponent::ValidateCast(const USkillDataAsset &Skill,const TArray<AActor*> &Targets) const
{
	if (!AP.IsValid() || !SP.IsValid())
		return FSkillCastResult::Fail("Reject.MissingResource");
	
	if (Skill.APCost > 0 && !AP->CanConsume(Skill.APCost))
		return FSkillCastResult::Fail("Reject.NotEnoughAP");
	
	if (Skill.SPCost > 0 && SP->GetSP() < Skill.SPCost)
		return FSkillCastResult::Fail("Reject.NotEnoughSP");
	
	if (GetCooldownRemaining(Skill.SkillId) > 0.f)
		return FSkillCastResult::Fail("Reject.Cooldown");
	
	if (Targets.Num() <= 0)
		return FSkillCastResult::Fail("Reject.InvalidTarget");
	
	if (bHasPrepared)
		return FSkillCastResult::Fail("Reject.SkillAlreadyPrepared");
	
	return FSkillCastResult::Ok();
}

FSkillCastResult USkillComponent::PrepareSkillCast(FName SkillId,const TArray<AActor*>&Targets, bool bFromTacticalReservation, FName ReasonTag)
{
	USkillDataAsset*Skill =GetSkillDef(SkillId);
	if (!Skill)
		return FSkillCastResult::Fail("Reject.SkillNotFound");

	const FSkillCastResult V = ValidateCast(*Skill, Targets);
	
	if (!V.bOk)
		return V;

	if (Skill->APCost > 0 && !AP->Consume(Skill->APCost, SkillId))
		return FSkillCastResult::Fail("Reject.NotEnoughAP");
	
	if (Skill->SPCost > 0 && !SP->ConsumeSP(Skill->SPCost, SkillId))
	{
		AP->Restore(Skill->APCost,"Rollback");
		return FSkillCastResult::Fail("Reject.NotEnoughSP");
	}

	Cooldowns.FindOrAdd(SkillId) = FMath::Max(0.f,Skill->CooldownSec);

	Prepared = FPreparedSkillCast();
	Prepared.Skill =Skill;
	Prepared.CommittedAP =Skill->APCost;
	Prepared.CommittedSP =Skill->SPCost;
	Prepared.bFromTacticalReservation =bFromTacticalReservation;
	Prepared.ReasonTag =ReasonTag;

	for (AActor *T :Targets)
	{
		if (T)
			Prepared.Targets.Add(T);
	}

	bHasPrepared =true;
	return FSkillCastResult::Ok();
}

void USkillComponent::ApplySkillEffects(const USkillDataAsset&Skill, const TArray<AActor*>&Targets, bool bFromTacticalReservation)
{
	const float Atk = Stats.IsValid() ? Stats->GetSnapshot().Attack : 10.f;
	const float CritRate = Stats.IsValid() ? Stats->GetSnapshot().CritRate : 0.f;
	const float CritBonus = Stats.IsValid() ? Stats->GetSnapshot().CritDamage : 0.f;

	for (AActor *T :Targets)
	{
		if (!T) 
			continue;

		UHPComponent* THP = T->FindComponentByClass<UHPComponent>();
		UCombatStatsComponent* TStats = T->FindComponentByClass<UCombatStatsComponent>();
		UThreatComponent* TThreat = T->FindComponentByClass<UThreatComponent>();
		UGroggyComponent* TGroggy = T->FindComponentByClass<UGroggyComponent>();
		UStatusEffectComponent *TStatus = T->FindComponentByClass<UStatusEffectComponent>();

		float DamageDone = 0.f;

		if (THP && (Skill.BasePower>0.f || Skill.AttackScale > 0.f))
		{
			const float Def = TStats ? TStats->GetSnapshot().Defense : 5.f;

			const FDamageBreakdown B = UCombatFormulaLibrary::BuildDamage(
			Atk,Def,
			Skill.BasePower,
			Skill.AttackScale,
			Skill.DefenseScale,
			1.0f,
			Skill.bAllowCrit,
			CritRate,
			CritBonus,
			Skill.VarianceMin,
			Skill.VarianceMax,
			Skill.GroggyPower,
			1.0f
						);

			DamageDone = B.FinalDamage;
			THP->ApplyDamage(DamageDone,GetOwner(), Skill.SkillId);

			if (IsHostileTarget(T))
			{
				if (TGroggy&&Skill.GroggyPower > 0.f)
				{
					TGroggy->AddGroggyDamage(B.GroggyDamage,GetOwner(), Skill.SkillId);
				}
				if (TThreat)
				{
					const float Threat = FMath::Max(0.f,Skill.ThreatBase+DamageDone* FMath::Max(0.f,Skill.ThreatFromDamageMul));
					if (Threat > 0.f)
						TThreat->AddThreat(GetOwner(),Threat, Skill.SkillId);
				}
			}
		}

		if (THP && Skill.HealPower > 0.f)
		{
			THP->Heal(Skill.HealPower,GetOwner(), Skill.SkillId);
		}

		if (TStatus && Skill.ApplyStatus&& FMath::FRand()<= FMath::Clamp(Skill.StatusChance,0.f,1.f))
		{
			TStatus->ApplyStatus(Skill.ApplyStatus,GetOwner(), Skill.StatusStacks, Skill.SkillId);
		}
	}

	OnSkillResolvedDetailed.Broadcast(Skill.SkillId, GetOwner(), Targets.Num(), bFromTacticalReservation);
}

FSkillCastResult USkillComponent::ResolvePreparedSkillCast()
{
	if (!bHasPrepared || !Prepared.Skill)
		return FSkillCastResult::Fail("Reject.NoPreparedSkill");

	TArray<AActor*>Targets;
	for (const TWeakObjectPtr<AActor>&W : Prepared.Targets)
	{
		if (AActor*A =W.Get())Targets.Add(A);
	}

	ApplySkillEffects(*Prepared.Skill,Targets, Prepared.bFromTacticalReservation);
	OnSkillCast.Broadcast(Prepared.Skill->SkillId, GetOwner(), Targets.Num());

	Prepared = FPreparedSkillCast();
	bHasPrepared = false;

	return FSkillCastResult::Ok();
}

void USkillComponent::CancelPreparedSkillCast(bool bRefundCost, FName)
{
	if (!bHasPrepared || !Prepared.Skill)return;

	if (bRefundCost)
	{
		if (Prepared.CommittedAP>0 && AP.IsValid()) 
			AP->Restore(Prepared.CommittedAP,"Refund.SkillAP");
		
		if (Prepared.CommittedSP>0 && SP.IsValid()) 
			SP->AddSP(Prepared.CommittedSP,"Refund.SkillSP");
		
		Cooldowns.FindOrAdd(Prepared.Skill->SkillId) = 0.f;
	}

	Prepared = FPreparedSkillCast();
	bHasPrepared = false;
}

bool USkillComponent::HasPreparedSkillCast()const
{
	return bHasPrepared && Prepared.Skill != nullptr;
}

FSkillCastResult USkillComponent::CastSkill(FName SkillId,const TArray<AActor*> &Targets,FName ReasonTag)
{
	const FSkillCastResult P = PrepareSkillCast(SkillId, Targets, false, ReasonTag);
	if (!P.bOk) 
		return P;
	return ResolvePreparedSkillCast();
}