// Source/JRPGCombat/Private/Combat/Battle/CombatTargetingSubsystem.cpp
#include "Combat/Battle/CombatTargetingSubsystem.h"

#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Stats/HPComponent.h"
#include "Combat/Threat/ThreatComponent.h"

UBattleSessionSubsystem* UCombatTargetingSubsystem :: GetBattle()const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBattleSessionSubsystem>() : nullptr;
}

bool UCombatTargetingSubsystem::IsAliveCombatant(AActor *Actor)const
{
	if (!Actor)
		return false;

	if (ICombatParticipantInterface *P =Cast<ICombatParticipantInterface>(Actor))
	{
		if (UHPComponent *HP = P->GetHP())
			return !HP->IsDead();
	}
	return false;
}

bool UCombatTargetingSubsystem::IsSameTeam(AActor *A,AActor *B)const
{
	if (!A||!B)
		return false;

	ICombatParticipantInterface *PA = Cast<ICombatParticipantInterface>(A);
	ICombatParticipantInterface *PB = Cast<ICombatParticipantInterface>(B);
	
	if (!PA||!PB) 
		return false;

	const ECombatTeam TA = PA->GetCombatTeam();
	const ECombatTeam TB = PB->GetCombatTeam();
	if (TA == ECombatTeam::Neutral || TB == ECombatTeam::Neutral) 
		return false;

	return TA == TB;
}

bool UCombatTargetingSubsystem::IsEnemyTeam(AActor *A,AActor *B) const
{
	if (!A||!B)
		return false;

	ICombatParticipantInterface *PA =Cast<ICombatParticipantInterface>(A);
	ICombatParticipantInterface *PB =Cast<ICombatParticipantInterface>(B);
	
	if (!PA||!PB)
		return false;

	const ECombatTeam TA = PA->GetCombatTeam();
	const ECombatTeam TB = PB->GetCombatTeam();
	if (TA == ECombatTeam::Neutral || TB == ECombatTeam::Neutral)
		return false;

	return TA != TB;
}

float UCombatTargetingSubsystem::GetHPRatio(AActor *Actor) const
{
	if (!Actor)return 1.f;

	if (ICombatParticipantInterface *P = Cast<ICombatParticipantInterface>(Actor))
	{
		if (UHPComponent *HP = P->GetHP())
		{
			const float Max = FMath::Max(1.f,HP->GetMaxHP());
			return HP->GetHP() / Max;
		}
	}
	return 1.f;
}

AActor* UCombatTargetingSubsystem::PickTopThreatTarget(AActor *Requester, const TArray<AActor*> &Candidates)const
{
	if (!Requester) 
		return nullptr;

	if (UThreatComponent *Threat = Requester->FindComponentByClass<UThreatComponent>())
	{
		if (AActor *Top = Threat->GetTopThreatSource())
		{
			if (Candidates.Contains(Top) && IsAliveCombatant(Top))
				return Top;
		}
	}
	return nullptr;
}

AActor* UCombatTargetingSubsystem::PickLowestHPActor(const TArray<AActor*> &Candidates)const
{
	float BestRatio = FLT_MAX;
	AActor *Best = nullptr;

	for (AActor *A :Candidates)
	{
		if (!A || !IsAliveCombatant(A))
			continue;

		const float Ratio = GetHPRatio(A);
		if (Ratio < BestRatio)
		{
			BestRatio =Ratio;
			Best = A;
		}
	}
	return Best;
}

AActor* UCombatTargetingSubsystem::PickLowestHPAllyIncludingSelf(AActor *Requester, const TArray<AActor*> &Allies) const
{
	TArray<AActor*> Pool = Allies;
	if (Requester && !Pool.Contains(Requester))
		Pool.Add(Requester);

	return PickLowestHPActor(Pool);
}

void UCombatTargetingSubsystem::GetAlliesIncludingSelf(AActor *Requester,TArray<AActor*> &Out) const
{
	Out.Reset();
	if (!Requester)
		return;

	if (UBattleSessionSubsystem *Battle = GetBattle())
	{
		Battle->GetAlliesFor(Requester,Out);
	}
	if (!Out.Contains(Requester))
	{
		Out.Add(Requester);
	}
}

FTargetValidationResult UCombatTargetingSubsystem::ValidateBasicAttackTarget(AActor*Requester,AActor*Target)const
{
	if (!Requester || !Target)
		return FTargetValidationResult::Fail("Reject.InvalidTarget");
	
	if (Requester == Target)
		return FTargetValidationResult::Fail("Reject.InvalidTarget");
	
	if (!IsAliveCombatant(Requester))
		return FTargetValidationResult::Fail("Reject.AttackerDead");
	
	if (!IsAliveCombatant(Target))
		return FTargetValidationResult::Fail("Reject.TargetDead");
	
	if (!IsEnemyTeam(Requester,Target))
		return FTargetValidationResult::Fail("Reject.FriendlyTarget");

	return FTargetValidationResult::Ok();
}

FTargetValidationResult UCombatTargetingSubsystem::ValidateSkillTargets(AActor *Requester,const USkillDataAsset *Skill,const TArray<AActor*> &Targets) const
{
	if (!Requester||!Skill)
		return FTargetValidationResult::Fail("Reject.InvalidSkill");
	
	if (Targets.Num()<=0)
		return FTargetValidationResult::Fail("Reject.InvalidTarget");

	auto IsValidAlive = [this](AActor*A) {return A && IsAliveCombatant(A); };

	switch (Skill->TargetType)
	{
	case ESkillTargetType::Self:
		if (Targets.Num()!=1||Targets[0]!=Requester)
			return FTargetValidationResult::Fail("Reject.InvalidTarget");
		
		return FTargetValidationResult::Ok();

	case ESkillTargetType::EnemySingle:
		if (Targets.Num()!=1||!IsValidAlive(Targets[0])||!IsEnemyTeam(Requester,Targets[0]))
			return FTargetValidationResult::Fail("Reject.InvalidTarget");
		
		return FTargetValidationResult::Ok();

	case ESkillTargetType::EnemyAll:
		for (AActor*T :Targets)
		{
			if (!IsValidAlive(T)||!IsEnemyTeam(Requester,T))
				return FTargetValidationResult::Fail("Reject.InvalidTarget");
		}
		
		return FTargetValidationResult::Ok();

	case ESkillTargetType::AllySingle:
		if (Targets.Num() != 1 || !IsValidAlive(Targets[0]) || !IsSameTeam(Requester,Targets[0]))
			return FTargetValidationResult::Fail("Reject.InvalidTarget");
		
		return FTargetValidationResult::Ok();

	case ESkillTargetType::AllyAll:
		for (AActor*T :Targets)
		{
			if (!IsValidAlive(T)||!IsSameTeam(Requester,T))
				return FTargetValidationResult::Fail("Reject.InvalidTarget");
		}
		return FTargetValidationResult::Ok();

	default:
		return FTargetValidationResult::Fail("Reject.InvalidTarget");
	}
}

FTargetingResult UCombatTargetingSubsystem::ResolvePreferredBasicAttackTarget(AActor *Requester)const
{
	if (!Requester)
		return FTargetingResult::Fail("Reject.InvalidTarget");

	UBattleSessionSubsystem *Battle = GetBattle();
	if (!Battle||!Battle->IsBattleActive())
		return FTargetingResult::Fail("Reject.NoBattle");

	TArray<AActor*> Opponents;
	Battle->GetOpponentsFor(Requester,Opponents);
	if (Opponents.Num()<=0)
		return FTargetingResult::Fail("Reject.NoOpponent");

	AActor*Chosen =PickTopThreatTarget(Requester, Opponents);
	if (!Chosen)
	{
		Chosen =PickLowestHPActor(Opponents);
	}
	if (!Chosen)
		return FTargetingResult::Fail("Reject.NoOpponent");

	TArray<AActor*> One = {Chosen };
	return FTargetingResult::Ok(One);
}

FTargetingResult UCombatTargetingSubsystem::ResolvePreferredTargetsForSkill(AActor *Requester,const USkillDataAsset *Skill) const
{
	if (!Requester||!Skill)
		return FTargetingResult::Fail("Reject.InvalidSkill");

	UBattleSessionSubsystem *Battle =GetBattle();
	if (!Battle||!Battle->IsBattleActive())
		return FTargetingResult::Fail("Reject.NoBattle");

	TArray<AActor*> Targets;

	switch (Skill->TargetType)
	{
	case ESkillTargetType::Self:
		Targets.Add(Requester);
		break;

	case ESkillTargetType::EnemySingle:
		{
			TArray<AActor*> Opponents;
			Battle->GetOpponentsFor(Requester,Opponents);

			AActor *Chosen = PickTopThreatTarget(Requester,Opponents);
			if (!Chosen)
				Chosen = PickLowestHPActor(Opponents);
		
			if (!Chosen)
				return FTargetingResult::Fail("Reject.NoOpponent");

			Targets.Add(Chosen);
			break;
		}

	case ESkillTargetType::EnemyAll:
		Battle->GetOpponentsFor(Requester,Targets);
		if (Targets.Num() <= 0)
			return FTargetingResult::Fail("Reject.NoOpponent");
		break;

	case ESkillTargetType::AllySingle:
		{
			TArray<AActor*>Allies;
			GetAlliesIncludingSelf(Requester,Allies);
			
			AActor *Chosen = PickLowestHPAllyIncludingSelf(Requester,Allies);
			if (!Chosen)
				return FTargetingResult::Fail("Reject.NoAlly");
			Targets.Add(Chosen);
			break;
		}

	case ESkillTargetType::AllyAll:
		GetAlliesIncludingSelf(Requester,Targets);
		if (Targets.Num() <= 0)
			return FTargetingResult::Fail("Reject.NoAlly");
		break;

	default:
		return FTargetingResult::Fail("Reject.InvalidTarget");
	}

	const FTargetValidationResult V =ValidateSkillTargets(Requester,Skill,Targets);
	if (!V.bOk)
		return FTargetingResult::Fail(V.ReasonTag);

	return FTargetingResult::Ok(Targets);
}