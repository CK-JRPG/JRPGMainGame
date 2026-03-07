#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Combat/Battle/BattleSessionTypes.h"
#include "Combat/Battle/BasicCombatTypes.h"
#include "Combat/Skills/SkillTypes.h"
#include "Combat/Items/CombatItemTypes.h"
#include "JRPG/Public/Combat/Skills/SkillTypes.h"

#include "BattleSessionSubsystem.generated.h"

class UBasicCombatSubsystem;
class UCombatActionComponent;

UCLASS()
class JRPG_API UBattleSessionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FOnBattleStarted OnBattleStarted;
	FOnBattleEnded OnBattleEnded;
	FOnTurnStarted OnTurnStarted;
	FOnTurnEnded OnTurnEnded;
	FOnBattleStateChanged OnBattleStateChanged;

	bool StartBattle(const FBattleSessionConfig &Config, FGuid &OutSessionId);
	void AbortBattle(FName ReasonTag);

	bool IsBattleActive() const { return bBattleActive; }
	const FBattleSessionSnapshot& GetSnapshot() const { return Snapshot; }

	AActor* GetCurrentTurnActor() const { return Snapshot.CurrentTurnActor.Get(); }
	EBattleFlowState GetFlowState() const { return Snapshot.FlowState; }

	// 외부 흐름 잠금(전술 모드 / 체인 어택)
	bool PauseFlow(FName ReasonTag);
	void ResumeFlow(FName ReasonTag);
	bool IsFlowPaused() const { return bFlowPaused; }

	bool BeginNextTurn();
	void FinishCurrentTurn(FName ReasonTag);

	bool CanActorActNow(AActor* Actor) const;

	FCombatActionResult TryExecuteBasicAttack(AActor* Attacker, AActor* Target);
	FSkillCastResult TryExecuteSkill(AActor* Attacker, FName SkillId, const TArray<AActor*> &Targets);

	void GetAliveParticipants(TArray<AActor*> &Out) const;
	void GetAliveParticipantsByTeam(ECombatTeam Team, TArray<AActor*> &Out) const;
	void GetOpponentsFor(AActor* Actor,TArray<AActor*> &Out) const;
	void GetAlliesFor(AActor* Actor,TArray<AActor*> &Out) const;
	
	bool BeginPresentedAction(AActor *Actor, FName ReasonTag);
	bool CanActorResolvePresentedAction(AActor *Actor) const;
	void CompletePresentedAction(AActor *Actor, FName ReasonTag);
	void AbortPresentedAction(AActor *Actor, FName ReasonTag);
	
	FCombatItemUseResult TryUseCombatItem (AActor* User, FName ItemId, const TArray<AActor*> &Targets, bool bFromTacticalReservation = false);

	bool GetCombatClamp(FVector& OutCenter, float& OutRadius) const;
	
protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY() bool bBattleActive = false;
	UPROPERTY() bool bFlowPaused = false;
	UPROPERTY() FName FlowPauseReason = NAME_None;

	UPROPERTY() FBattleSessionConfig ActiveConfig;
	UPROPERTY() FBattleSessionSnapshot Snapshot;

	UPROPERTY() TArray<FBattleParticipantSlot> Participants;
	UPROPERTY() TArray<FTurnOrderEntry> TurnOrder;

	UPROPERTY()TWeakObjectPtr<AActor>PresentedResolvingActor;
	
	UBasicCombatSubsystem* GetBasicCombat() const;

	void ResetSessionState();
	bool BuildParticipants(const FBattleSessionConfig& Config);
	void BuildTurnOrderForRound();

	void SetFlowState(EBattleFlowState NewState);

	bool IsParticipantAlive(AActor* Actor) const;
	ECombatTeam GetParticipantTeam(AActor* Actor) const;
	float GetParticipantSpeed(AActor* Actor) const;

	bool CheckBattleEndAndResolve();
	bool AreAllEnemiesDefeated() const;
	bool AreAllPlayersDefeated() const;

	void EndBattle(EBattleEndReason Reason);
	void GrantVictoryRewards();

	void HandleCombatantDefeated(AActor* Victim, AActor* Killer);
};
