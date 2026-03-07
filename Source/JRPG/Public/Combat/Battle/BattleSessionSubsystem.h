// Source/JRPGCombat/Public/Combat/Battle/BattleSessionSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Skills/SkillTypes.h"

#include "BattleSessionSubsystem.generated.h"

class UBasicCombatSubsystem;
class UCombatActionComponent;

UCLASS()
class JRPG_API UBattleSessionSubsystem :public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnBattleStarted OnBattleStarted;
	FOnBattleEnded OnBattleEnded;
	FOnTurnStarted OnTurnStarted;
	FOnTurnEnded OnTurnEnded;
	FOnBattleStateChanged OnBattleStateChanged;

	// ---- Session API ----
	bool StartBattle(constFBattleSessionConfig &Config,FGuid &OutSessionId);
	void AbortBattle(FNameReasonTag);

	bool IsBattleActive()const {return bBattleActive; }
	constFBattleSessionSnapshot& GetSnapshot()const {return Snapshot; }

	AActor* GetCurrentTurnActor()const {return Snapshot.CurrentTurnActor.Get(); }
	EBattleFlowState GetFlowState()const {return Snapshot.FlowState; }

	// ---- Turn API ----
	bool BeginNextTurn();
	void FinishCurrentTurn(FName ReasonTag);

	bool CanActorActNow(AActor *Actor) const;

	// ---- Action API ----
	FCombatActionResult TryExecuteBasicAttack(AActor *Attacker,AActor *Target);
	FSkillCastResult TryExecuteSkill(AActor *Attacker,FName SkillId, const TArray<AActor*> &Targets);

	// ---- Query API ----
	void GetAliveParticipants(TArray<AActor*> &Out) const;
	void GetAliveParticipantsByTeam(ECombatTeam Team,TArray<AActor*> &Out) const;
	void GetOpponentsFor(AActor *Actor,TArray<AActor*> &Out) const;
	void GetAlliesFor(AActor *Actor,TArray<AActor*> &Out) const;

protected:
	virtual void OnWorldBeginPlay(UWorld &InWorld) override;

private:
	UPROPERTY() bool bBattleActive = false;
	UPROPERTY() FBattleSessionConfig ActiveConfig;
	UPROPERTY() FBattleSessionSnapshot Snapshot;

	UPROPERTY() TArray<FBattleParticipantSlot> Participants;
	UPROPERTY() TArray<FTurnOrderEntry> TurnOrder;

	// ---- Internal ----
	UBasicCombatSubsystem* GetBasicCombat()const;

	void ResetSessionState();
	bool BuildParticipants(const FBattleSessionConfig &Config);
	void BuildTurnOrderForRound();

	void SetFlowState(EBattleFlowState NewState);

	bool IsParticipantAlive(AActor *Actor) const;
	ECombatTeam GetParticipantTeam(AActor *Actor) const;
	float GetParticipantSpeed(AActor *Actor) const;

	bool CheckBattleEndAndResolve();
	bool AreAllEnemiesDefeated() const;
	bool AreAllPlayersDefeated() const;

	void EndBattle(EBattleEndReason Reason);
	void GrantVictoryRewards();

	void HandleCombatantDefeated(AActor *Victim,AActor *Killer);
};