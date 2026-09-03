// Source/JRPGCombat/Private/Combat/Battle/BattleSessionSubsystem.cpp
#include "Combat/Battle/BattleSessionSubsystem.h"

#include "EngineUtils.h"
#include "Combat/Battle/BasicCombatSubsystem.h"
#include "Combat/Battle/CombatActionComponent.h"
#include "Combat/Battle/CombatZoneSettingDataAsset.h"

#include "Combat/Characters/CombatCharacterActor.h"
#include "Combat/Characters/CombatParticipantInterface.h"
#include "Combat/Characters/PartySubsystem.h"

#include "Combat/Items/CombatItemExecutionSubsystem.h"
#include "Combat/Session/CombatZoneActor.h"
#include "Combat/Stats/HPComponent.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"

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

#if __has_include("Combat/Items/InventorySubsystem.h")
#include "Combat/Items/InventorySubsystem.h"
#define JRPG_HAS_INVENTORY 1
#else
#define JRPG_HAS_INVENTORY 0
#endif

static double BattleNow()
{
	return FPlatformTime::Seconds();
}

UBasicCombatSubsystem* UBattleSessionSubsystem::GetBasicCombat()const
{
	return GetWorld() ?GetWorld()->GetSubsystem<UBasicCombatSubsystem>() : nullptr;
}

void UBattleSessionSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UBasicCombatSubsystem* BC = InWorld.GetSubsystem<UBasicCombatSubsystem>())
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
	ExclusiveModeOwners.Reset();
	ActivePresentedActors.Reset();
}

void UBattleSessionSubsystem::SetPhase(EBattlePhase NewPhase)
{
	Snapshot.Phase = NewPhase;
	OnBattlePhaseChanged.Broadcast(NewPhase);
}

FBattleParticipantSlot* UBattleSessionSubsystem::FindParticipantMutable(AActor* Actor)
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

const FBattleParticipantSlot* UBattleSessionSubsystem::FindParticipant(AActor* Actor) const
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

bool UBattleSessionSubsystem::BuildParticipants(const FBattleSessionConfig &Config)
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
	else if (Config.bPullPartyFromPartySubsystemIfPlayerSideEmpty && GetWorld() && GetWorld()->GetGameInstance())
	{
		if (UPartySubsystem* Party = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
		{
			Party->GetPartyMembers(PlayerActors);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BattleSessionSub::BuildParticipants : PlayerSide None."))
	}

	for (const TWeakObjectPtr<AActor> &W : Config.EnemySide)
	{
		if (AActor* A = W.Get())
		{
			EnemyActors.Add(A);
			UE_LOG(LogTemp, Warning, TEXT("BattleSessionSub::BuildParticipants : Add to Enemy"))
		}
	}

	if (PlayerActors.Num() <= 0 || EnemyActors.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleSessionSub::BuildParticipants : 플레이어 파티 또는 적의 개수가 0 이하임."))
		return false;
	}

	TSet<AActor*> Dedup;
	int32 AlivePlayerCount = 0;
	int32 AliveEnemyCount = 0;

	auto AddSide = [this, &Dedup, &AlivePlayerCount, &AliveEnemyCount](const TArray<AActor*> &InActors)
	{
		for (AActor *A : InActors)
		{
			if (!A || Dedup.Contains(A)) continue;

			ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(A);
			if (!P) continue;

			UHPComponent* HP = P->GetHP();
			if (!HP||HP->IsDead())
			{
				UE_LOG(LogTemp, Warning, TEXT("BattleSessionSub::BuildParticipants : 사망/HP 없음으로 참가자 제외 - %s"), *GetNameSafe(A));
				continue;
			}

			FBattleParticipantSlot Slot;
			Slot.Actor = A;
			Slot.Team = P->GetCombatTeam();
			Slot.bAlive = true;
			Slot.NextActionAllowedReal = 0.0;
			Participants.Add(Slot);

			if (Slot.Team == ECombatTeam::Player)
			{
				++AlivePlayerCount;
			}
			else if (Slot.Team == ECombatTeam::Enemy)
			{
				++AliveEnemyCount;
			}

			Dedup.Add(A);
		}
	};

	AddSide(PlayerActors);
	AddSide(EnemyActors);

	if (AlivePlayerCount <= 0 || AliveEnemyCount <= 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("BattleSessionSub::BuildParticipants : 생존 참가자 부족. AlivePlayers=%d AliveEnemies=%d TotalParticipants=%d"),
			AlivePlayerCount, AliveEnemyCount, Participants.Num());
		return false;
	}

	return true;
}

bool UBattleSessionSubsystem::StartBattle(const FBattleSessionConfig& Config, const FEncounterContext& InEncounterCtx, FGuid& OutSessionId)
{
	if (bBattleActive || bEndingBattle)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleSessionSubsystem : 배틀 세션이 이미 시작 되었거나 종료 처리 중임"));
		return false;
	}

	ResetSessionState();

	if (!BuildParticipants(Config))
	{
		ResetSessionState();
		UE_LOG(LogTemp, Error, TEXT("BattleSessionSubsystem : Config 인자가 제대로 등록되지 않음(인자 확인)"));

		return false;
	}

	bBattleActive = true;
	ActiveConfig = Config;

	// Zone 생성 (FEncounterContext 기반, 플레이어 중심)
	CreateCombatZone(InEncounterCtx);
	EnsureCombatNavMeshBounds(InEncounterCtx);

	Snapshot.SessionId = InEncounterCtx.EncounterToken.IsValid() ? InEncounterCtx.EncounterToken : FGuid::NewGuid();
	SetPhase(EBattlePhase::Starting);

	RebuildSnapshotCounts();

	OutSessionId = Snapshot.SessionId;
	OnBattleStarted.Broadcast(Snapshot);

	SetPhase(EBattlePhase::Active);
	return true;
}

void UBattleSessionSubsystem::AbortBattle(FName)
{
	if (!bBattleActive)
		return;
	
	EndBattle(EBattleEndReason::Aborted);
}

bool UBattleSessionSubsystem::GetCombatClamp(FVector &OutCenter, float &OutRadius) const
{
	if (!bBattleActive) return false;
	if (!ActiveConfig.bEnableCombatClamp) return false;
	if (ActiveConfig.CombatClampRadius <= 0.f) return false;

	OutCenter =ActiveConfig.CombatClampCenter;
	OutRadius =ActiveConfig.CombatClampRadius;
	return true;
}

bool UBattleSessionSubsystem::EnterExclusiveMode(FName ModeTag)
{
	if (!bBattleActive || Snapshot.Phase != EBattlePhase::Active)
		return false;
	
	if (ModeTag.IsNone())
		return false;

	
	bool bAlreadyInSet = false;
	
	ExclusiveModeOwners.Add(ModeTag, &bAlreadyInSet);
	const bool bAdded = !bAlreadyInSet;
	//에러 : 이항 '>' 연산자 없음. const bool bAdded = ExclusiveModeOwners.Add(ModeTag) > 0;
	//위 코드로 고침(Set 전용)
	if (bAdded)
	{
		Snapshot.bExclusiveMode = true;
		Snapshot.ExclusiveModeTag = ModeTag;
		OnExclusiveModeChanged.Broadcast(true,ModeTag);
	}
	return bAdded;
}

void UBattleSessionSubsystem::ExitExclusiveMode(FName ModeTag)
{
	if (ModeTag.IsNone())
		return;

	const bool bRemoved = ExclusiveModeOwners.Remove(ModeTag) > 0;
	if (!bRemoved)
		return;

	Snapshot.bExclusiveMode = (ExclusiveModeOwners.Num() > 0);
	Snapshot.ExclusiveModeTag = Snapshot.bExclusiveMode ? *ExclusiveModeOwners.CreateConstIterator() : NAME_None;
	OnExclusiveModeChanged.Broadcast(Snapshot.bExclusiveMode,Snapshot.ExclusiveModeTag);
}

bool UBattleSessionSubsystem::IsParticipantAlive(AActor* Actor)const
{
	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	if (!Slot||!Slot->bAlive)
		return false;

	ICombatParticipantInterface* P = Cast<ICombatParticipantInterface>(Actor);
	if (!P)
		return false;

	UHPComponent* HP = P->GetHP();
	return HP && !HP->IsDead();
}

ECombatTeam UBattleSessionSubsystem::GetParticipantTeam(AActor* Actor)const
{
	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	return Slot ? Slot->Team : ECombatTeam::Neutral;
}

bool UBattleSessionSubsystem::IsActorActionLocked(AActor* Actor)const
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

float UBattleSessionSubsystem::GetActorRemainingRecoverySec(AActor* Actor)const
{
	const FBattleParticipantSlot* Slot = FindParticipant(Actor);
	if (!Slot)
		return 0.f;

	const double Now = BattleNow();
	return (float) FMath::Max(0.0,Slot->NextActionAllowedReal-Now);
}

bool UBattleSessionSubsystem::CanActorExecuteAction(AActor* Actor)const
{
	if (!bBattleActive)	return false;
	if (Snapshot.Phase != EBattlePhase::Active) return false;
	if (!Actor)	return false;
	if (!IsParticipantAlive(Actor)) return false;
	if (ExclusiveModeOwners.Num() > 0) return false;
	if (IsActorActionLocked(Actor)) return false;

	return true;
}

bool UBattleSessionSubsystem::BeginPresentedAction(AActor* Actor,FName ReasonTag)
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

bool UBattleSessionSubsystem::CanActorResolvePresentedAction(AActor* Actor) const
{
	if (!bBattleActive || Snapshot.Phase != EBattlePhase::Active) 
		return false;
	
	if (!Actor)
		return false;
	
	if (!IsParticipantAlive(Actor))
		return false;
	
	return ActivePresentedActors.Contains(Actor);
}

void UBattleSessionSubsystem::SetActorActionRecovery(AActor *Actor,float RecoverySec,FName ReasonTag)
{
	if (!Actor)
		return;

	if (FBattleParticipantSlot* Slot = FindParticipantMutable(Actor))
	{
		Slot->NextActionAllowedReal = BattleNow() + FMath::Max(0.f,RecoverySec);
		OnActorActionLockChanged.Broadcast(Actor, RecoverySec > 0.f ,ReasonTag);
	}
}

void UBattleSessionSubsystem::CompletePresentedAction(AActor* Actor,FName ReasonTag,float RecoverySec)
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

void UBattleSessionSubsystem::AbortPresentedAction(AActor* Actor,FName ReasonTag,bool bClearRecovery)
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

FCombatActionResult UBattleSessionSubsystem::TryExecuteBasicAttack(AActor* Attacker, AActor* Target)
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

FSkillCastResult UBattleSessionSubsystem::TryExecuteSkill(AActor* Attacker,FName SkillId,const TArray<AActor*> &Targets)
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

FCombatItemUseResult UBattleSessionSubsystem::TryUseCombatItem(AActor*User,FName ItemId,const TArray<AActor*> &Targets,bool bFromTacticalReservation)
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

void UBattleSessionSubsystem::FinishCurrentTurn(FName ReasonTag)
{
	if (!bBattleActive || Snapshot.Phase != EBattlePhase::Active)
		return;
	
	for (auto It = ActivePresentedActors.CreateIterator(); It; ++It)
	{
		if (AActor* Actor = It.Key().Get())
		{
			OnActorActionLockChanged.Broadcast(Actor, false, ReasonTag);
		}
	}
	
	ActivePresentedActors.Reset();
	Snapshot.ActivePresentedActionCount = 0;
	CheckBattleEndAndResolve();
}

void UBattleSessionSubsystem::GetAliveParticipants(TArray<AActor*> &Out) const
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

void UBattleSessionSubsystem::GetAliveParticipantsByTeam(ECombatTeam Team,TArray<AActor*> &Out)const
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

void UBattleSessionSubsystem::GetOpponentsFor(AActor*Actor,TArray<AActor*>&Out)const
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

void UBattleSessionSubsystem::GetAlliesFor(AActor* Actor,TArray<AActor*> &Out)const
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

void UBattleSessionSubsystem::RebuildSnapshotCounts()
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

void UBattleSessionSubsystem::GetParticipantRuntimeStates(TArray<FBattleActorRuntimeState>& OutStates) const
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

bool UBattleSessionSubsystem::AreAllEnemiesDefeated() const
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

bool UBattleSessionSubsystem::AreAllPlayersDefeated() const
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

bool UBattleSessionSubsystem::CheckBattleEndAndResolve()
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

void UBattleSessionSubsystem::GrantVictoryRewards()
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

				if (PartyIds.Num() >= 2 && PartyIds.Num() <= 3)
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
	if (ActiveConfig.VictoryRewards.GoldReward > 0)
	{
		if (GetWorld() && GetWorld()->GetGameInstance())
		{
			if (UEconomySubsystem* Eco = GetWorld()->GetGameInstance()->GetSubsystem<UEconomySubsystem>())
			{
				Eco->AddGold(ActiveConfig.VictoryRewards.GoldReward,"Battle.Victory");
			}
		}
	}
#endif


#if JRPG_HAS_INVENTORY
	if (ActiveConfig.VictoryRewards.ItemDrops.Num() > 0 && GetWorld() && GetWorld()->GetGameInstance())
	{
		UInventorySubsystem* Inv = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>();
		if (Inv)
		{
			for (const FBattleItemDrop& Drop : ActiveConfig.VictoryRewards.ItemDrops)
			{
				if (Drop.ItemId.IsNone() || Drop.Amount <= 0) continue;
				if (FMath::FRand() > FMath::Clamp(Drop.DropChance, 0.f, 1.f)) continue;

				Inv->AddItem(Drop.ItemId, Drop.Amount, "Battle.VictoryDrop");
			}
		}
	}
#endif
}

void UBattleSessionSubsystem::EndBattle(EBattleEndReason Reason)
{
	if (!bBattleActive || bEndingBattle)return;

	// OnBattleEnded listener가 동기적으로 새 세션을 시작해
	// 이전 세션의 Reset/Zone cleanup에 덮어쓰이지 않도록 보호한다.
	bEndingBattle = true;

	SetPhase(EBattlePhase::Ending);

	if (Reason == EBattleEndReason::Victory)
	{
		GrantVictoryRewards();
	}

	SetPhase(EBattlePhase::Cleanup);

	const FBattleSessionSnapshot FinalSnapshot = Snapshot;

	// Legacy EncounterTriggerActor does not own rematch recovery, so keep the
	// session-level fallback until that path is removed.
	if (Reason == EBattleEndReason::Defeat || Reason == EBattleEndReason::Aborted)
	{
		for (const FBattleParticipantSlot& Slot : Participants)
		{
			if (Slot.Team != ECombatTeam::Enemy)
			{
				continue;
			}

			if (ACombatCharacterActor* Enemy = Cast<ACombatCharacterActor>(Slot.Actor.Get()))
			{
				Enemy->ResetEnemyRuntimeForRematch(TEXT("Battle.RematchReset"));
			}
		}
	}

	bBattleActive = false;
	OnBattleEnded.Broadcast(FinalSnapshot, Reason);

	ResetSessionState();
	// Zone 정리 (BattleSession이 생성한 Zone 파괴)
	if (IsValid(SpawnedZone))
	{
		SpawnedZone->Destroy();
		SpawnedZone = nullptr;
	}

	if (IsValid(SpawnedCombatNavBounds))
	{
		SpawnedCombatNavBounds->Destroy();
		SpawnedCombatNavBounds = nullptr;
	}

	bEndingBattle = false;
}

void UBattleSessionSubsystem::HandleCombatantDefeated(AActor* Victim, AActor*)
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

void UBattleSessionSubsystem::CreateCombatZone(const FEncounterContext& InEncounterCtx)
{
	TSubclassOf<ACombatZoneActor> ZoneClass = nullptr;

	// DA에서 CombatZoneClass 가져오기
	if (IsValid(InEncounterCtx.ZoneSetting))
	{
		ZoneClass = InEncounterCtx.ZoneSetting->CombatZoneClass;
	}

	if (!ZoneClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleSessionSubsystem::CreateCombatZone : CombatZoneClass가 설정되지 않음 (ZoneSetting DA 확인 필요). Zone 생성 생략."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 플레이어(TriggerActor) 위치 기준으로 Zone 생성
	const FVector SpawnLocation = InEncounterCtx.ZoneCenter;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	SpawnedZone = GetWorld()->SpawnActor<ACombatZoneActor>(
		ZoneClass,
		SpawnLocation,
		SpawnRotation,
		Params
	);

	if (IsValid(SpawnedZone))
	{
		if (IsValid(InEncounterCtx.ZoneSetting))
		{
			const FVector ZoneExtent = InEncounterCtx.ZoneSetting->ZoneBoxExtent.GetAbs();
			SpawnedZone->SetZoneHalfHeight(ZoneExtent.Z);

			if (InEncounterCtx.ZoneSetting->ZoneShape == ECombatZoneShape::Sphere)
			{
				SpawnedZone->SetZoneRadius(FMath::Max(0.0f, InEncounterCtx.ZoneSetting->ZoneSphereRadius));
			}
			else
			{
				SpawnedZone->SetZoneRadius(FMath::Max(ZoneExtent.X, ZoneExtent.Y));
			}
		}

		UE_LOG(LogTemp, Log, TEXT("BattleSessionSubsystem::CreateCombatZone : CombatZoneActor 생성 완료 (Center=%s)"),
			*SpawnLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BattleSessionSubsystem::CreateCombatZone : CombatZoneActor 생성 실패"));
	}
}

void UBattleSessionSubsystem::EnsureCombatNavMeshBounds(const FEncounterContext& InEncounterCtx)
{
	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> ExistingNavBounds;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavMeshBoundsVolume::StaticClass(), ExistingNavBounds);
	if (ExistingNavBounds.Num() > 0)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			for (AActor* NavBoundsActor : ExistingNavBounds)
			{
				if (ANavMeshBoundsVolume* NavBounds = Cast<ANavMeshBoundsVolume>(NavBoundsActor))
				{
					NavSys->OnNavigationBoundsUpdated(NavBounds);
				}
			}
		}
		return;
	}

	FVector NavExtent(2500.f, 2500.f, 600.f);
	if (IsValid(InEncounterCtx.ZoneSetting))
	{
		const FVector ZoneExtent = InEncounterCtx.ZoneSetting->ZoneBoxExtent.GetAbs();
		const float Radius = InEncounterCtx.ZoneSetting->ZoneShape == ECombatZoneShape::Sphere
			? FMath::Max(0.f, InEncounterCtx.ZoneSetting->ZoneSphereRadius)
			: FMath::Max(ZoneExtent.X, ZoneExtent.Y);
		NavExtent.X = FMath::Max(NavExtent.X, Radius + 800.f);
		NavExtent.Y = FMath::Max(NavExtent.Y, Radius + 800.f);
		NavExtent.Z = FMath::Max(NavExtent.Z, ZoneExtent.Z + 400.f);
	}
	else if (ActiveConfig.CombatClampRadius > 0.f)
	{
		NavExtent.X = FMath::Max(NavExtent.X, ActiveConfig.CombatClampRadius + 800.f);
		NavExtent.Y = FMath::Max(NavExtent.Y, ActiveConfig.CombatClampRadius + 800.f);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedCombatNavBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(),
		InEncounterCtx.ZoneCenter,
		FRotator::ZeroRotator,
		Params);
	if (!IsValid(SpawnedCombatNavBounds))
	{
		UE_LOG(LogTemp, Error, TEXT("[PartyAI][NavSetup] FailedToSpawnCombatNavBounds Center=%s"), *InEncounterCtx.ZoneCenter.ToString());
		return;
	}

	// The default volume brush is roughly 200 uu wide, so scale converts the requested extent to a combat-sized nav bounds.
	SpawnedCombatNavBounds->SetActorScale3D(FVector(NavExtent.X / 100.f, NavExtent.Y / 100.f, NavExtent.Z / 100.f));
	SpawnedCombatNavBounds->SetActorLocation(InEncounterCtx.ZoneCenter);

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->OnNavigationBoundsUpdated(SpawnedCombatNavBounds);
		NavSys->Build();
	}

	UE_LOG(LogTemp, Log, TEXT("[PartyAI][NavSetup] SpawnedCombatNavBounds Center=%s Extent=%s"),
		*InEncounterCtx.ZoneCenter.ToString(),
		*NavExtent.ToString());
}
