// Source/JRPGCombat/Private/Combat/Battle/BattleSessionSubsystem.cpp
#include "Combat/Battle/CombatBattleSessionSubsystem.h"

#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Battle/CombatActionComponent.h"

#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/PartySubsystem.h"

#include "Combat/Items/CombatItemExecutionSubsystem.h"
#include "Combat/Stats/CombatHPComponent.h"

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

static double BattleNow()
{
	return FPlatformTime::Seconds();
}

UBasicCombatSubsystem* UCombatBattleSessionSubsystem::GetBasicCombat()const
{
	return GetWorld() ?GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr;
}

void UCombatBattleSessionSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UBasicCombatSubsystem* BC = InWorld.GetSubsystem<UBasicCombatSubsystem>())
	{
		BC->OnCombatantDefeated.AddUObject(this, &UCombatBattleSessionSubsystem::HandleCombatantDefeated);
	}
}

void UCombatBattleSessionSubsystem::ResetSessionState()
{
	bBattleActive = false;
	ActiveConfig = FBattleSessionConfig();
	Snapshot = FBattleSessionSnapshot();
	Participants.Reset();
	ExclusiveModeOwners.Reset();
	ActivePresentedActors.Reset();
}

void UCombatBattleSessionSubsystem::SetPhase(EBattlePhase NewPhase)
{
	Snapshot.Phase = NewPhase;
	OnBattlePhaseChanged.Broadcast(NewPhase);
}

FBattleParticipantSlot* UCombatBattleSessionSubsystem::FindParticipantMutable(AActor* Actor)
{
	if (!Actor)
		return nullptr;

	for (FBattleParticipantSlot &Slot :Participants)
	{
		if (Slot.Actor.Get() == Actor)
		{
			return &Slot;
		}
	}
	return nullptr;
}

const FBattleParticipantSlot* UCombatBattleSessionSubsystem::FindParticipant(AActor* Actor) const
{
	if (!Actor)
		return nullptr;

	for (const FBattleParticipantSlot &Slot : Participants)
	{
		if (Slot.Actor.Get() == Actor)
		{
			return &Slot;
		}
	}
	return nullptr;
}

bool UCombatBattleSessionSubsystem::BuildParticipants(const FBattleSessionConfig &Config)
{
	Participants.Reset();

	TArray<AActor*> PlayerActors;
	TArray<AActor*> EnemyActors;

	if (Config.PlayerSide.Num() > 0)
	{
		for (const TWeakObjectPtr<AActor> &W : Config.PlayerSide)
		{
			if (AActor *A = W.Get())
			{
				PlayerActors.Add(A);
			}
		}
	}
	else if (Config.bPullPartyFromPartySubsystemIfPlayerSideEmpty&&GetWorld() && GetWorld()->GetGameInstance())
	{
		if (UPartySubsystem* Party = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
		{
			Party->GetPartyMembers(PlayerActors);
		}
	}

	for (const TWeakObjectPtr<AActor> &W : Config.EnemySide)
	{
		if (AActor* A = W.Get())
		{
			EnemyActors.Add(A);
		}
	}

	if (PlayerActors.Num() <= 0 || EnemyActors.Num() <= 0)
	{
		return false;
	}

	TSet<AActor*> Dedup;

	auto AddSide = [this, &Dedup](const TArray<AActor*> &InActors)
	{
		for (AActor *A : InActors)
		{
			if (!A || Dedup.Contains(A)) continue;

			ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(A);
			if (!P) continue;

			UCombatHPComponent* HP = P->GetHP();
			if (!HP||HP->IsDead()) continue;

			FBattleParticipantSlot Slot;
			Slot.Actor = A;
			Slot.Team = P->GetCombatTeam();
			Slot.bAlive = true;
			Slot.NextActionAllowedReal = 0.0;
			Participants.Add(Slot);

			Dedup.Add(A);
		}
	};

	AddSide(PlayerActors);
	AddSide(EnemyActors);

	return Participants.Num()>=2;
}

bool UCombatBattleSessionSubsystem::StartBattle(const FBattleSessionConfig &Config, FGuid &OutSessionId)
{
	if (bBattleActive)
	{
		return false;
	}

	ResetSessionState();

	if (!BuildParticipants(Config))
	{
		ResetSessionState();
		return false;
	}

	bBattleActive = true;
	ActiveConfig = Config;

	Snapshot.SessionId = FGuid::NewGuid();
	SetPhase(EBattlePhase::Starting);

	RebuildSnapshotCounts();

	OutSessionId = Snapshot.SessionId;
	OnBattleStarted.Broadcast(Snapshot);

	SetPhase(EBattlePhase::Active);
	return true;
}

void UCombatBattleSessionSubsystem::AbortBattle(FName)
{
	if (!bBattleActive)
		return;
	
	EndBattle(EBattleEndReason::Aborted);
}

bool UCombatBattleSessionSubsystem::GetCombatClamp(FVector &OutCenter, float &OutRadius) const
{
	if (!bBattleActive) return false;
	if (!ActiveConfig.bEnableCombatClamp) return false;
	if (ActiveConfig.CombatClampRadius <= 0.f) return false;

	OutCenter =ActiveConfig.CombatClampCenter;
	OutRadius =ActiveConfig.CombatClampRadius;
	return true;
}

bool UCombatBattleSessionSubsystem::EnterExclusiveMode(FName ModeTag)
{
	if (!bBattleActive || Snapshot.Phase!= EBattlePhase::Active)
		return false;
	
	if (ModeTag.IsNone())
		return false;

	const bool bAdded =ExclusiveModeOwners.Add(ModeTag) > 0;
	if (bAdded)
	{
		Snapshot.bExclusiveMode = true;
		Snapshot.ExclusiveModeTag = ModeTag;
		OnExclusiveModeChanged.Broadcast(true,ModeTag);
	}
	return bAdded;
}

void UCombatBattleSessionSubsystem::ExitExclusiveMode(FName ModeTag)
{
	if (ModeTag.IsNone())
		return;

	const bool bRemoved =ExclusiveModeOwners.Remove(ModeTag) > 0;
	if (!bRemoved)
		return;

	Snapshot.bExclusiveMode = (ExclusiveModeOwners.Num() > 0);
	Snapshot.ExclusiveModeTag = Snapshot.bExclusiveMode ? *ExclusiveModeOwners.CreateConstIterator() : NAME_None;
	OnExclusiveModeChanged.Broadcast(Snapshot.bExclusiveMode,Snapshot.ExclusiveModeTag);
}

bool UCombatBattleSessionSubsystem::IsParticipantAlive(AActor* Actor)const
{
	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	if (!Slot||!Slot->bAlive)
		return false;

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor);
	if (!P)
		return false;

	UCombatHPComponent* HP = P->GetHP();
	return HP && !HP->IsDead();
}

ECombatTeam UCombatBattleSessionSubsystem::GetParticipantTeam(AActor* Actor)const
{
	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	return Slot ? Slot->Team : ECombatTeam::Neutral;
}

bool UCombatBattleSessionSubsystem::IsActorActionLocked(AActor* Actor)const
{
	if (!Actor)
		return true;
	
	if (!IsParticipantAlive(Actor))
		return true;

	if (ActivePresentedActors.Contains(Actor))
	{
		return true;
	}

	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	if (!Slot)
		return true;

	return BattleNow() < Slot->NextActionAllowedReal;
}

float UCombatBattleSessionSubsystem::GetActorRemainingRecoverySec(AActor* Actor)const
{
	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	if (!Slot)
		return 0.f;

	const double Now = BattleNow();
	return (float) FMath::Max(0.0,Slot->NextActionAllowedReal-Now);
}

bool UCombatBattleSessionSubsystem::CanActorExecuteAction(AActor* Actor)const
{
	if (!bBattleActive)	return false;
	if (Snapshot.Phase != EBattlePhase::Active) return false;
	if (!Actor)	return false;
	if (!IsParticipantAlive(Actor)) return false;
	if (ExclusiveModeOwners.Num() > 0) return false;
	if (IsActorActionLocked(Actor)) return false;

	returntrue;
}

bool UCombatBattleSessionSubsystem::BeginPresentedAction(AActor* Actor,FName ReasonTag)
{
	if (!CanActorExecuteAction(Actor))
	{
		return false;
	}

	ActivePresentedActors.Add(Actor, ReasonTag);
	Snapshot.ActivePresentedActionCount = ActivePresentedActors.Num();

	OnActorActionLockChanged.Broadcast(Actor, true, ReasonTag);
	return true;
}

bool UCombatBattleSessionSubsystem::CanActorResolvePresentedAction(AActor* Actor) const
{
	if (!bBattleActive || Snapshot.Phase != EBattlePhase::Active) 
		return false;
	
	if (!Actor)
		return false;
	
	if (!IsParticipantAlive(Actor))
		return false;
	
	return ActivePresentedActors.Contains(Actor);
}

void UCombatBattleSessionSubsystem::SetActorActionRecovery(AActor *Actor,float RecoverySec,FName ReasonTag)
{
	if (!Actor)
		return;

	if (FBattleParticipantSlot* Slot = FindParticipantMutable(Actor))
	{
		Slot->NextActionAllowedReal = BattleNow() + FMath::Max(0.f,RecoverySec);
		OnActorActionLockChanged.Broadcast(Actor, RecoverySec > 0.f ,ReasonTag);
	}
}

void UCombatBattleSessionSubsystem::CompletePresentedAction(AActor* Actor,FName ReasonTag,float RecoverySec)
{
	if (!Actor)
		return;

	const bool bRemoved = ActivePresentedActors.Remove(Actor) > 0;
	Snapshot.ActivePresentedActionCount = ActivePresentedActors.Num();

	if (bRemoved)
	{
		SetActorActionRecovery(Actor, RecoverySec,ReasonTag);
		OnActorActionLockChanged.Broadcast(Actor, false, ReasonTag);
	}

	CheckBattleEndAndResolve();
}

void UCombatBattleSessionSubsystem::AbortPresentedAction(AActor* Actor,FName ReasonTag,bool bClearRecovery)
{
	if (!Actor)return;

	const bool bRemoved =ActivePresentedActors.Remove(Actor) > 0;
	Snapshot.ActivePresentedActionCount = ActivePresentedActors.Num();

	if (bClearRecovery)
	{
		if (FBattleParticipantSlot* Slot = FindParticipantMutable(Actor))
		{
			Slot->NextActionAllowedReal =0.0;
		}
	}

	if (bRemoved)
	{
		OnActorActionLockChanged.Broadcast(Actor,false,ReasonTag);
	}
}

FCombatActionResult UCombatBattleSessionSubsystem::TryExecuteBasicAttack(AActor* Attacker, AActor* Target)
{
	if (!CanActorExecuteAction(Attacker))
	{
		return FCombatActionResult::Fail("Reject.ActionLocked");
	}

	UCombatActionComponent* Action = Attacker ? Attacker->FindComponentByClass<UCombatActionComponent>() : nullptr;
	if (!Action)
	{
		return FCombatActionResult::Fail("Reject.NoActionComponent");
	}

	const FCombatActionResult R = Action->TryBasicAttack(Target);
	if (R.bOk)
	{
		SetActorActionRecovery(Attacker,ActiveConfig.DefaultActionRecoverySec,"Battle.BasicAttack");
		CheckBattleEndAndResolve();
	}
	return R;
}

FSkillCastResult UCombatBattleSessionSubsystem::TryExecuteSkill(AActor* Attacker,FName SkillId,const TArray<AActor*> &Targets)
{
	if (!CanActorExecuteAction(Attacker))
	{
		return FSkillCastResult::Fail("Reject.ActionLocked");
	}

	UCombatActionComponent* Action = Attacker ? Attacker->FindComponentByClass<UCombatActionComponent>() : nullptr;
	if (!Action)
	{
		return FSkillCastResult::Fail("Reject.NoActionComponent");
	}

	const FSkillCastResult R = Action->TryCastSkill(SkillId, Targets);
	if (R.bOk)
	{
		SetActorActionRecovery(Attacker,ActiveConfig.DefaultActionRecoverySec,"Battle.Skill");
		CheckBattleEndAndResolve();
	}
	return R;
}

FCombatItemUseResult UCombatBattleSessionSubsystem::TryUseCombatItem(AActor*User,FName ItemId,const TArray<AActor*> &Targets,bool bFromTacticalReservation)
{
	if (!CanActorExecuteAction(User))
	{
		return FCombatItemUseResult::Fail("Reject.ActionLocked");
	}

	UCombatItemExecutionSubsystem* ItemExec = GetWorld() ? GetWorld()->GetSubsystem<UCombatItemExecutionSubsystem>() : nullptr;
	if (!ItemExec)
	{
		return FCombatItemUseResult::Fail("Reject.NoItemExecutionSubsystem");
	}

	FCombatItemUseRequest Req;
	Req.User = User;
	Req.ItemId = ItemId;
	Req.bFromTacticalReservation = bFromTacticalReservation;
	Req.ReasonTag = "Battle.UseItem";
	for (AActor* T : Targets)
	{
		if (T)Req.Targets.Add(T);
	}

	const FCombatItemUseResult R = ItemExec->ExecuteUse(Req);
	if (R.bOk)
	{
		SetActorActionRecovery(User,ActiveConfig.DefaultActionRecoverySec,"Battle.Item");
		CheckBattleEndAndResolve();
	}
	return R;
}

void UCombatBattleSessionSubsystem::GetAliveParticipants(TArray<AActor*> &Out) const
{
	Out.Reset();
	for (const FBattleParticipantSlot &S :Participants)
	{
		if (AActor* A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
			{
				Out.Add(A);
			}
		}
	}
}

void UCombatBattleSessionSubsystem::GetAliveParticipantsByTeam(ECombatTeamTeam,TArray<AActor*> &Out)const
{
	Out.Reset();
	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Team != Team) continue;
		if (AActor*A =S.Actor.Get())
		{
			if (IsParticipantAlive(A))
			{
				Out.Add(A);
			}
		}
	}
}

void UCombatBattleSessionSubsystem::GetOpponentsFor(AActor*Actor,TArray<AActor*>&Out)const
{
	Out.Reset();
	const ECombatTeam MyTeam =GetParticipantTeam(Actor);
	if (MyTeam == ECombatTeam::Neutral)return;

	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Team == MyTeam) continue;
		if (AActor* A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
			{
				Out.Add(A);
			}
		}
	}
}

void UCombatBattleSessionSubsystem::GetAlliesFor(AActor* Actor,TArray<AActor*> &Out)const
{
	Out.Reset();
	const ECombatTeam MyTeam = GetParticipantTeam(Actor);
	if (MyTeam == ECombatTeam::Neutral)return;

	for (const FBattleParticipantSlot &S :Participants)
	{
		if (S.Team != MyTeam)continue;
		if (AActor* A = S.Actor.Get())
		{
			if (A != Actor && IsParticipantAlive(A))
			{
				Out.Add(A);
			}
		}
	}
}

void UCombatBattleSessionSubsystem::RebuildSnapshotCounts()
{
	int32 AlivePlayers = 0;
	int32 AliveEnemies = 0;

	for (const FBattleParticipantSlot &S : Participants)
	{
		if (!S.Actor.IsValid() || !IsParticipantAlive(S.Actor.Get()))
			continue;

		if (S.Team == ECombatTeam::Player) ++AlivePlayers;
		else if (S.Team == ECombatTeam::Enemy) ++AliveEnemies;
	}

	Snapshot.AlivePlayers = AlivePlayers;
	Snapshot.AliveEnemies = AliveEnemies;
	Snapshot.ActivePresentedActionCount = ActivePresentedActors.Num();
	Snapshot.bExclusiveMode = (ExclusiveModeOwners.Num()>0);
	Snapshot.ExclusiveModeTag = Snapshot.bExclusiveMode ? *ExclusiveModeOwners.CreateConstIterator() : NAME_None;
}

void UCombatBattleSessionSubsystem::GetParticipantRuntimeStates(TArray<FBattleActorRuntimeState>& OutStates) const
{
	OutStates.Reset();

	const double Now = BattleNow();

	for (const FBattleParticipantSlot& Slot : Participants)
	{
		FBattleActorRuntimeState S;
		S.Actor = Slot.Actor;
		S.Team = Slot.Team;
		S.bAlive = Slot.Actor.IsValid() && IsParticipantAlive(Slot.Actor.Get());
		S.bPresentedActionActive = Slot.Actor.IsValid() && ActivePresentedActors.Contains(Slot.Actor);

		const float RemainingRecovery = (float)FMath::Max(0.0, Slot.NextActionAllowedReal - Now);
		S.RemainingRecoverySec = RemainingRecovery;

		S.bActionLocked = S.bPresentedActionActive || RemainingRecovery > 0.f;
		S.ActionLockReason = S.bPresentedActionActive
			? FName("PresentedAction")
			: (RemainingRecovery> 0.f ? FName("Recovery") : NAME_None);

		OutStates.Add(S);
	}
}

bool UCombatBattleSessionSubsystem::AreAllEnemiesDefeated() const
{
	bool bHasEnemy = false;
	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Team != ECombatTeam::Enemy)continue;
		bHasEnemy = true;

		if (AActor* A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
			{
				return false;
			}
		}
	}
	return bHasEnemy;
}

bool UCombatBattleSessionSubsystem::AreAllPlayersDefeated() const
{
	bool bHasPlayer = false;
	for (const FBattleParticipantSlot &S : Participants)
	{
		if (S.Team != ECombatTeam::Player)
			continue;
		bHasPlayer = true;

		if (AActor* A = S.Actor.Get())
		{
			if (IsParticipantAlive(A))
			{
				return false;
			}
		}
	}
	return bHasPlayer;
}

bool UCombatBattleSessionSubsystem::CheckBattleEndAndResolve()
{
	if (!bBattleActive)return false;

	RebuildSnapshotCounts();

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

void UCombatBattleSessionSubsystem::GrantVictoryRewards()
{
#if JRPG_HAS_LEVELING
	if (ActiveConfig.VictoryRewards.BaseExpReward > 0)
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (ULevelingSubsystem* L = GetWorld()->GetGameInstance()->GetSubsystem<ULevelingSubsystem>())
			{
				L->GrantCombatRewardExp(ActiveConfig.VictoryRewards.BaseExpReward,Snapshot.SessionId);
			}
		}
	}
#endif

#if JRPG_HAS_BOND
	if (ActiveConfig.VictoryRewards.BondBPReward > 0)
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (UBondSubsystem* Bond = GetWorld()->GetGameInstance()->GetSubsystem<UBondSubsystem>())
			{
				TArray<FName> PartyIds;
				if (UPartySubsystem* Party =GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
				{
					PartyIds = Party->GetPartyIds();
				}

				if (PartyIds.Num() == 3)
				{
					FBondAddRequest Req;
					Req.Source = EBondSource::CombatWin;
					Req.Participants =PartyIds;
					Req.BaseAmount =ActiveConfig.VictoryRewards.BondBPReward;
					Req.Context ="BattleVictory";
					Req.SourceTag ="Bond.CombatWin";
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
			if (UEconomySubsystem* Eco =GetWorld()->GetGameInstance()->GetSubsystem<UEconomySubsystem>())
			{
				Eco->AddGold(ActiveConfig.VictoryRewards.GoldReward,"Battle.Victory");
			}
		}
	}
#endif
}

void UCombatBattleSessionSubsystem::EndBattle(EBattleEndReason Reason)
{
	if (!bBattleActive)return;

	SetPhase(EBattlePhase::Ending);

	if (Reason == EBattleEndReason::Victory)
	{
		GrantVictoryRewards();
	}

	SetPhase(EBattlePhase::Cleanup);

	const FBattleSessionSnapshot FinalSnapshot = Snapshot;

	bBattleActive = false;
	OnBattleEnded.Broadcast(FinalSnapshot, Reason);

	ResetSessionState();
}

void UCombatBattleSessionSubsystem::HandleCombatantDefeated(AActor* Victim, AActor*)
{
	if (!bBattleActive || !Victim)return;

	if (FBattleParticipantSlot* Slot = FindParticipantMutable(Victim))
	{
		Slot->bAlive = false;
		Slot->bActionLocked = true;
		Slot->ActionLockReason = "Defeated";
	}

	ActivePresentedActors.Remove(Victim);
	CheckBattleEndAndResolve();
}