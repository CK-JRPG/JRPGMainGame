#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/Chain/ChainTypes.h"
#include "BattleSessionSubsystem.generated.h"

UENUM()
enum class EJRPGCombatPhase :uint8
{
	Idle,
	Running,
	Ending,
	Cleanup
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatPhaseChanged, EJRPGCombatPhase/*Prev*/, EJRPGCombatPhase/*New*/ );

UCLASS()
class JRPGCOMBAT_API UBattleSessionSubsystem :public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Phase
	FOnCombatPhaseChanged OnCombatPhaseChanged;

	bool IsCombatRunning() const { return Phase== EJRPGCombatPhase::Running; }
	EJRPGCombatPhase GetPhase() const { return Phase; }

	void SetPhase(EJRPGCombatPhase NewPhase);

	// Participants (최소 구현)
	void RegisterParticipant(AActor *Actor,bool bIsPlayerParty);
	bool IsParticipant(AActor *Actor) const;

	void SetPrimaryTarget(AActor *Target);
	AActor* GetPrimaryTarget() const {return PrimaryTarget.Get(); }

	void GetPartyActors(TArray<AActor*> &OutParty)const;

	// --- Player input lock (Tactical/Chain 등) ---
	void PushPlayerInputLock(FName OwnerTag);
	void PopPlayerInputLock(FName OwnerTag);
	bool IsPlayerInputLocked()const {return PlayerInputLockOwners.Num()>0; }

	// --- Enemy suppression (Chain 등) ---
	void PushEnemySuppression(FName OwnerTag,EEnemySuppressionScope Scope);
	void PopEnemySuppression(FName OwnerTag);

	bool IsEnemySuppressed() const {return EnemySuppressionOwners.Num()>0; }
	EEnemySuppressionScope GetCurrentSuppressionScope() const {return CurrentSuppressionScope; }

	// AI gate
	bool CanEnemyStartAttack() const
	{
		if (!IsEnemySuppressed()) return true;
		return CurrentSuppressionScope == EEnemySuppressionScope::GateOnly; // StopOnly/StopAndGate면 공격 금지
	}

	// Damage/Status gate helper
	bool ShouldGateEnemyToAlly(AActor *Instigator,AActor *Victim)const;

	// Team helpers (프로젝트 팀/팩션 시스템 붙기 전까지는 Tag 기반)
	static bool IsEnemyActor(const AActor*A) {return A && A->ActorHasTag("Team.Enemy"); }
	static bool IsPlayerActor(const AActor*A) {return A && A->ActorHasTag("Team.Player"); }

private:
	EJRPGCombatPhase Phase = EJRPGCombatPhase::Idle;

	TSet<TWeakObjectPtr<AActor>> Participants;
	TSet<TWeakObjectPtr<AActor>> PlayerParty;

	TWeakObjectPtr<AActor> PrimaryTarget;

	// Input lock owner stack
	TArray<FName> PlayerInputLockOwners;

	// Enemy suppression owner stack
	struct FSuppOwner
	{
		FName OwnerTag = NAME_None;
		EEnemySuppressionScope Scope = EEnemySuppressionScope::StopAndGate;
	};
	TArray<FSuppOwner> EnemySuppressionOwners;
	EEnemySuppressionScope CurrentSuppressionScope = EEnemySuppressionScope::StopAndGate;

	void RecomputeSuppressionScope();
};