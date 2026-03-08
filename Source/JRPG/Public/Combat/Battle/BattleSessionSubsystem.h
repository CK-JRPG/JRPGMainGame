// Source/JRPGCombat/Public/Combat/Battle/BattleSessionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "JRPG/Public/Combat/Battle/BattleSessionTypes.h"
#include "JRPG/Public/Combat/Battle/BasicCombatTypes.h"
#include "Combat/Skills/SkillTypes.h"
#include "Combat/Items/CombatItemTypes.h"

#include "BattleSessionSubsystem.generated.h"

class UBasicCombatSubsystem;

UCLASS()
class JRPG_API UBattleSessionSubsystem :public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnBattleStarted OnBattleStarted;
	FOnBattleEnded OnBattleEnded;
	FOnBattlePhaseChanged OnBattlePhaseChanged;
	FOnActorActionLockChanged OnActorActionLockChanged;
	FOnExclusiveModeChanged OnExclusiveModeChanged;

	bool StartBattle(const FBattleSessionConfig &Config, FGuid &OutSessionId);
	void AbortBattle(FName ReasonTag);

	bool IsBattleActive()const {return bBattleActive && Snapshot.Phase == EBattlePhase::Active; }
	const FBattleSessionSnapshot& GetSnapshot()const {return Snapshot; }
	EBattlePhase GetPhase()const {return Snapshot.Phase; }

	float GetDefaultActionRecoverySec()const {return ActiveConfig.DefaultActionRecoverySec; }

	// clamp
	bool GetCombatClamp(FVector&OutCenter,float&OutRadius)const;

	// exclusive mode: 체인/연출 전용 별도 시퀀스
	bool EnterExclusiveMode(FName ModeTag);
	void ExitExclusiveMode(FName ModeTag);
	bool IsExclusiveModeActive()const {return ExclusiveModeOwners.Num() > 0; }

	// action gating
	bool CanActorExecuteAction(AActor* Actor) const;
	bool IsActorActionLocked(AActor* Actor) const;
	float GetActorRemainingRecoverySec(AActor* Actor) const;

	bool BeginPresentedAction(AActor* Actor,FName ReasonTag);
	bool CanActorResolvePresentedAction(AActor* Actor) const;
	void CompletePresentedAction(AActor* Actor, FName ReasonTag, float RecoverySec = 0.25f);
	void AbortPresentedAction(AActor* Actor, FName ReasonTag, bool bClearRecovery = false);

	void SetActorActionRecovery(AActor* Actor,float RecoverySec,FName ReasonTag);

	// direct execution API (즉발 경로용)
	FCombatActionResult TryExecuteBasicAttack(AActor* Attacker,AActor* Target);
	FSkillCastResult TryExecuteSkill(AActor* Attacker, FName SkillId,const TArray<AActor*> &Targets);
	FCombatItemUseResult TryUseCombatItem(AActor* User, FName ItemId,const TArray<AActor*> &Targets,bool bFromTacticalReservation = false);

	// queries
	void GetAliveParticipants(TArray<AActor*>&Out)const;
	void GetAliveParticipantsByTeam(ECombatTeamTeam,TArray<AActor*>&Out)const;
	void GetOpponentsFor(AActor*Actor,TArray<AActor*>&Out)const;
	void GetAlliesFor(AActor*Actor,TArray<AActor*>&Out)const;

	bool GetCombatClamp(FVector& OutCenter, float& OutRadius) const;
	
	void GetParticipantRuntimeStates(TArray<FBattleActorRuntimeState>& OutStates) const;
	

protected:
	virtual void OnWorldBeginPlay(UWorld &InWorld) override;

private:
	bool bBattleActive = false;

	UPROPERTY() FBattleSessionConfig ActiveConfig;
	UPROPERTY() FBattleSessionSnapshot Snapshot;
	UPROPERTY() TArray<FBattleParticipantSlot> Participants;

	TSet<FName> ExclusiveModeOwners;
	TMap<TWeakObjectPtr<AActor>, FName> ActivePresentedActors;

	UBasicCombatSubsystem* GetBasicCombat() const;

	void ResetSessionState();
	void SetPhase(EBattlePhase NewPhase);
	bool BuildParticipants(const FBattleSessionConfig &Config);

	FBattleParticipantSlot* FindParticipantMutable(AActor* Actor);
	const FBattleParticipantSlot* FindParticipant(AActor* Actor) const;

	bool IsParticipantAlive(AActor* Actor) const;
	ECombatTeam GetParticipantTeam(AActor* Actor) const;

	void RebuildSnapshotCounts();
	bool AreAllEnemiesDefeated() const;
	bool AreAllPlayersDefeated() const;
	bool CheckBattleEndAndResolve();

	void GrantVictoryRewards();
	void EndBattle(EBattleEndReason Reason);

	void HandleCombatantDefeated(AActor* Victim, AActor* Killer);
};