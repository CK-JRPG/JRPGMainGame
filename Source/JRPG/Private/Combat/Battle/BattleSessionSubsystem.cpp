
// Source/JRPGCombat/Private/Combat/Battle/BattleSessionSubsystem.cpp
#include "Combat/Battle/BattleSessionSubsystem.h"

#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/Stats/CombatStatsComponent.h"
#include "Combat/Characters/PartySubsystem.h"

#include "Combat/Stats/HPComponent.h"
#include "Combat/Stats/APComponent.h"

#if __has_include("Combat/Progression/Leveling/LevelingSubsystem.h")
	#include "Combat/Progression/Leveling/LevelingSubsystem.h"
	#define JRPG_HAS_LEVELING 1
#else
	#define JRPG_HAS_LEVELING 0
#endif

#if __has_include("Combat/Progression/Bond/BondSubsystem.h")
	#include "Combat/Progression/Bond/BondSubsystem.h"
	#define JRPG_HAS_BOND 1
#else
	#define JRPG_HAS_BOND 0
#endif

#if __has_include("Combat/Items/EconomySubsystem.h")
	#include "Combat/Items/EconomySubsystem.h"
	#define JRPG_HAS_ECONOMY 1
#else
	#define JRPG_HAS_ECONOMY 0
#endif

UBasicCombatSubsystem* UBattleSessionSubsystem::GetBasicCombat()const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr;
}

void UBattleSessionSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UBasicCombatSubsystem *BC = InWorld.GetSubsystem<UBasicCombatSubsystem>())
	{
		BC->OnCombatantDefeated.AddUObject(this, &UBattleSessionSubsystem::HandleCombatantDefeated);
	}
}

void UBattleSessionSubsystem::ResetSessionState()
{
	bBattleActive = false;
	ActiveConfig = FBattleSessionConfig();

	Snapshot = FBattleSessionSnapshot();
	Participants.Reset();
	TurnOrder.Reset();
}

void UBattleSessionSubsystem::SetFlowState(EBattleFlowState NewState)
{
	Snapshot.FlowState = NewState;
	OnBattleStateChanged.Broadcast(NewState);
}

bool UBattleSessionSubsystem::BuildParticipants(const FBattleSessionConfig &Config)
{
	Participants.Reset();

	TArray<AActor*> PlayerActors;
	TArray<AActor*> EnemyActors;

	// Player side
	if (Config.PlayerSide.Num()>0)
	{
		for (const TWeakObjectPtr<AActor> &W : Config.PlayerSide)
		{
			if (AActor*A =W.Get())
				PlayerActors.Add(A);
		}
	}
	else if (Config.bPullPartyFromPartySubsystemIfPlayerSideEmpty)
	{
		if (GetWorld()&&GetWorld()->GetGameInstance())
		{
			if (UPartySubsystem*Party =GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
			{
				Party->GetPartyMembers(PlayerActors);
			}
		}
	}

	// Enemy side
	for (const TWeakObjectPtr<AActor>&W : Config.EnemySide)
	{
		if (AActor *A = W.Get())
			EnemyActors.Add(A);
	}

	if (PlayerActors.Num() <=0 || EnemyActors.Num() <= 0)
		return false;

	TSet<AActor*> Dedup;

	auto AddSide = [this, &Dedup](const TArray<AActor*> &InActors)
	{
		for (AActor *A : InActors)
		{
			if (!A || Dedup.Contains(A)) 
				continue;

			ICombatParticipantInterface *P = Cast<ICombatParticipantInterface>(A);
			if (!P)
				continue;

			UHPComponent *HP = P->GetHP();
			if (!HP||HP->IsDead())
				continue;

			FBattleParticipantSlot Slot;
			Slot.Actor =A;
			Slot.Team =P->GetCombatTeam();
			Slot.bAlive =true;
			Slot.CachedSpeed =GetParticipantSpeed(A);

			Participants.Add(Slot);
			Dedup.Add(A);
		}
	};

	AddSide(PlayerActors);
	AddSide(EnemyActors);

	return Participants.Num() >= 2;
}

float UBattleSessionSubsystem::GetParticipantSpeed(AActor *Actor) const
{
	if (!Actor)
		return 10.f;

	if (UCombatStatsComponent *Stats = Actor->FindComponentByClass<UCombatStatsComponent>())
		return Stats->GetSnapshot().Speed;

	return 10.f;
}

bool UBattleSessionSubsystem::IsParticipantAlive(AActor*Actor)const
{
	if (!Actor)
		return false;

	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Actor.Get() == Actor)
		{
			if (!S.bAlive)
				return false;

			if (ICombatParticipantInterface*P =Cast<ICombatParticipantInterface>(Actor))
			{
				if (UHPComponent *HP = P->GetHP())
					return !HP->IsDead();
			}
			return true;
		}
	}
	return false;
}

ECombatTeam UBattleSessionSubsystem::GetParticipantTeam(AActor *Actor)const
{
	if (!Actor) 
		return ECombatTeam::Neutral;

	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Actor.Get() == Actor)
			return S.Team;
	}
	return ECombatTeam::Neutral;
}

void UBattleSessionSubsystem::BuildTurnOrderForRound()
{
	TurnOrder.Reset();

	for (FBattleParticipantSlot &S :Participants)
	{
		AActor*A =S.Actor.Get();
		
		if (!A)
			continue;
		
		if (!IsParticipantAlive(A))
			continue;

		S.CachedSpeed = GetParticipantSpeed(A);

		FTurnOrderEntry E;
		E.Actor =A;
		E.Initiative =S.CachedSpeed;
		TurnOrder.Add(E);
	}

	TurnOrder.Sort([](const FTurnOrderEntry &L, const FTurnOrderEntry &R)
		{
	if (!FMath::IsNearlyEqual(L.Initiative,R.Initiative,0.001f))
		return L.Initiative>R.Initiative;

		AActor *LA = L.Actor.Get();
		AActor *RA = R.Actor.Get();
		const FString LN = LA ? LA->GetName() : FString();
		const FString RN = RA ? RA->GetName() : FString();
		return LN < RN;
			});
}

bool UBattleSessionSubsystem::StartBattle(constFBattleSessionConfig &Config,FGuid &OutSessionId)
{
	if (bBattleActive)
		return false;

	ResetSessionState();

	if (!BuildParticipants(Config))
	{
		ResetSessionState();
		return false;
	}

	bBattleActive = true;
	ActiveConfig = Config;

	Snapshot.SessionId = FGuid::NewGuid();
	Snapshot.Round = 1;
	Snapshot.TurnIndex = -1;

	SetFlowState(EBattleFlowState::Starting);

	BuildTurnOrderForRound();

	OutSessionId = Snapshot.SessionId;
	OnBattleStarted.Broadcast(Snapshot);

	if (Config.bAutoBegin)
	{
		return BeginNextTurn();
	}

	return true;
}

void UBattleSessionSubsystem::AbortBattle(FName)
{
if (!bBattleActive)return;
EndBattle(EBattleEndReason::Aborted);
}

bool UBattleSessionSubsystem::BeginNextTurn()
{
	if (!bBattleActive)
		return false;
	if (CheckBattleEndAndResolve())
		return false;

	// 다음 인덱스
	int32 StartIdx = Snapshot.TurnIndex + 1;

	// 새 라운드
	if (TurnOrder.Num() <= 0 || StartIdx >= TurnOrder.Num())
	{
		Snapshot.Round++;
		Snapshot.TurnIndex = -1;
		BuildTurnOrderForRound();
		StartIdx  0;
	}

	for (int32 i = StartIdx; i<TurnOrder.Num(); ++i)
	{
		AActor*A =TurnOrder[i].Actor.Get();
		
		if (!A)
			continue;
		
		if (!IsParticipantAlive(A) && ActiveConfig.bSkipDeadCombatants)
			continue;

		Snapshot.TurnIndex = i;
		Snapshot.CurrentTurnActor = A;

		// 턴 시작 시 AP 회복
		if (ActiveConfig.bRestoreAPOnTurnStart)
		{
			if (UAPComponent*AP =A->FindComponentByClass<UAPComponent>())
			{
				AP-> Restore(AP->GetMaxAP(),"Battle.TurnStartAP");
			}
		}

		const ECombatTeamTeam = GetParticipantTeam(A);
		SetFlowState(Team == ECombatTeam::Player ? EBattleFlowState::PlayerTurn : EBattleFlowState::EnemyTurn);

		OnTurnStarted.Broadcast(A,Snapshot.Round);
		return true;
	}

	// 유효한 턴이 없으면 새 라운드 재시도
	BuildTurnOrderForRound();
	if (TurnOrder.Num()<=0)
	{
		return CheckBattleEndAndResolve() ? false : false;
	}

	Snapshot.TurnIndex = -1;
	return BeginNextTurn();
}

void UBattleSessionSubsystem::FinishCurrentTurn(FName)
{
	if (!bBattleActive)
		return;

	AActor *Current =Snapshot.CurrentTurnActor.Get();
	if (Current)
	{
		OnTurnEnded.Broadcast(Current,Snapshot.Round);
	}

	Snapshot.CurrentTurnActor =nullptr;

	if (!CheckBattleEndAndResolve())
	{
		BeginNextTurn();
	}
}

bool UBattleSessionSubsystem::CanActorActNow(AActor *Actor)const
{
	if (!bBattleActive || !Actor) 
		return false;
	
	if (Snapshot.CurrentTurnActor.Get() != Actor)
		return false;
	
	if (!IsParticipantAlive(Actor)) 
		return false;

	const ECombatTeam Team = GetParticipantTeam(Actor);
	
	if (Team == ECombatTeam::Player && Snapshot.FlowState != EBattleFlowState::PlayerTurn)
		return false;
	
	if (Team == ECombatTeam::Enemy && Snapshot.FlowState != EBattleFlowState::EnemyTurn)
		return false;

	return true;
}

FCombatActionResult UBattleSessionSubsystem::TryExecuteBasicAttack(AActor *Attacker,AActor *Target)
{
	if (!CanActorActNow(Attacker))
		return FCombatActionResult::Fail("Reject.NotCurrentTurn");

	UCombatActionComponent*Action =Attacker ?Attacker->FindComponentByClass<UCombatActionComponent>() :nullptr;
	if (!Action)
		return FCombatActionResult::Fail("Reject.NoActionComponent");

	SetFlowState(EBattleFlowState::ResolvingAction);

	FCombatActionResult R = Action->TryBasicAttack(Target);

	if (!R.bOk)
	{
		// 실패하면 원래 턴 상태로 복구
		const ECombatTeamTeam =GetParticipantTeam(Attacker);
		SetFlowState(Team == ECombatTeam::Player ? EBattleFlowState::PlayerTurn : EBattleFlowState::EnemyTurn);
		return R;
	}

	FinishCurrentTurn("Battle.BasicAttack");
	return R;
}

FSkillCastResult UBattleSessionSubsystem::TryExecuteSkill(AActor*Attacker,FName SkillId, const TArray<AActor *> &Targets)
{
	if (!CanActorActNow(Attacker))
		return FSkillCastResult::Fail("Reject.NotCurrentTurn");

	UCombatActionComponent*Action =Attacker ?Attacker->FindComponentByClass<UCombatActionComponent>() : nullptr;
	if (!Action)
		return FSkillCastResult::Fail("Reject.NoActionComponent");

	SetFlowState(EBattleFlowState::ResolvingAction);

	FSkillCastResult R = Action->TryCastSkill(SkillId,Targets);

	if (!R.bOk)
	{
		const ECombatTeam Team =GetParticipantTeam(Attacker);
		SetFlowState(Team== ECombatTeam::Player ? EBattleFlowState::PlayerTurn : EBattleFlowState::EnemyTurn);
		return R;
	}

	FinishCurrentTurn("Battle.Skill");
	return R;
}

void UBattleSessionSubsystem::GetAliveParticipants(TArray<AActor*>&Out)const
{
Out.Reset();
for (const FBattleParticipantSlot &S :Participants)
	{
if (AActor*A =S.Actor.Get())
		{
if (IsParticipantAlive(A))
Out.Add(A);
		}
	}
}

void UBattleSessionSubsystem::GetAliveParticipantsByTeam(ECombatTeam Team, TArray<AActor*> &Out) const
{
	Out.Reset();
	for (const FBattleParticipantSlot &S :Participants)
	{
		if (S.Team != Team)continue;
		if (AActor *A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
				Out.Add(A);
		}
	}
}

void UBattleSessionSubsystem::GetOpponentsFor(AActor *Actor, TArray<AActor*> &Out) const
{
	Out.Reset();
	const ECombatTeam MyTeam = GetParticipantTeam(Actor);
	if (MyTeam == ECombatTeam::Neutral)return;

	for (const FBattleParticipantSlot &S :Participants)
	{
		if (S.Team == MyTeam)continue;
		if (AActor *A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
				Out.Add(A);
		}
	}
}

void UBattleSessionSubsystem::GetAlliesFor(AActor *Actor,TArray<AActor*> &Out) const
{
	Out.Reset();
	const ECombatTeam MyTeam = GetParticipantTeam(Actor);
	if (MyTeam == ECombatTeam::Neutral) 
		return;

	for (const FBattleParticipantSlot &S :Participants)
	{
		if (S.Team != MyTeam)
			continue;
		
		if (AActor *A = S.Actor.Get())
		{
			if (A != Actor && IsParticipantAlive(A))
				Out.Add(A);
		}
	}
}


bool UBattleSessionSubsystem::AreAllEnemiesDefeated()const
{
	bool bHasEnemy = false;
	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Team != ECombatTeam::Enemy)
			continue;
		
		bHasEnemy = true;

		if (AActor*A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
				return false;
		}
	}
	return bHasEnemy;
}

bool UBattleSessionSubsystem::AreAllPlayersDefeated() const
{
	bool bHasPlayer = false;
	for (const FBattleParticipantSlot&S : Participants)
	{
		if (S.Team != ECombatTeam::Player)
			continue;
		
		bHasPlayer = true;

		if (AActor *A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
				return false;
		}
	}
	return bHasPlayer;
}

bool UBattleSessionSubsystem::CheckBattleEndAndResolve()
{
	if (!bBattleActive)
		return false;

	if (ActiveConfig.bEndBattleOnAllEnemiesDefeated && AreAllEnemiesDefeated())
	{
		EndBattle(EBattleEndReason::Victory);
		return true;
	}

	if (ActiveConfig.bEndBattleOnAllPlayersDefeated && AreAllPlayersDefeated())
	{
		EndBattle(EBattleEndReason::Defeat);
		return true;
	}

	return false;
}

void UBattleSessionSubsystem::GrantVictoryRewards()
{
#if JRPG_HAS_LEVELING
	if (ActiveConfig.VictoryRewards.BaseExpReward>0)
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (ULevelingSubsystem*L = GetWorld()->GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
			{
				L->GrantCombatRewardExp(ActiveConfig.VictoryRewards.BaseExpReward, Snapshot.SessionId);
			}
		}
	}
#endif

#if JRPG_HAS_BOND
	if (ActiveConfig.VictoryRewards.BondBPReward > 0)
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (UBondSubsystem *Bond = GetWorld()->GetGameInstance()->GetSubsystem<UBondSubsystem>())
			{
				TArray<FName> PartyIds;

				if (UPartySubsystem *Party =GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
				{
					PartyIds = Party->GetPartyIds();
				}

				if (PartyIds.Num() == 3)
				{
					FBondAddRequestReq;
					Req.Source = EBondSource::CombatWin;
					Req.Participants = PartyIds;
					Req.BaseAmount = ActiveConfig.VictoryRewards.BondBPReward;
					Req.Context = "BattleVictory";
					Req.SourceTag = "Bond.CombatWin";

					Bond->AddBondPoints(Req);
				}
			}
		}
	}
#endif

#if JRPG_HAS_ECONOMY
	if (ActiveConfig.VictoryRewards.GoldReward>0)
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (UEconomySubsystem*Eco = GetWorld()->GetGameInstance()->GetSubsystem<UEconomySubsystem>())
			{
				Eco->AddGold(ActiveConfig.VictoryRewards.GoldReward,"Battle.Victory");
			}
		}
	}
#endif
}

void UBattleSessionSubsystem::EndBattle(EBattleEndReason Reason)
{
	if (!bBattleActive)
		return;

	switch (Reason)
	{
	case EBattleEndReason::Victory:
		SetFlowState(EBattleFlowState::Victory);
		GrantVictoryRewards();
		break;
	case EBattleEndReason::Defeat:
		SetFlowState(EBattleFlowState::Defeat);
		break;
	case EBattleEndReason::Aborted:
	default:
		SetFlowState(EBattleFlowState::Ended);
		break;
	}

	const FBattleSessionSnapshot FinalSnapshot = Snapshot;

	bBattleActive = false;
	OnBattleEnded.Broadcast(FinalSnapshot, Reason);

	ResetSessionState();
}

void UBattleSessionSubsystem::HandleCombatantDefeated(AActor *Victim, AActor*)
{
	if (!bBattleActive || !Victim)
		return;

	for (FBattleParticipantSlot &S : Participants)
	{
		if (S.Actor.Get() == Victim)
		{
			S.bAlive = false;
			break;
		}
	}

	CheckBattleEndAndResolve();
}