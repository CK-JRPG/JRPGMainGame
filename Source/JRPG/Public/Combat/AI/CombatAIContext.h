// Source/JRPGCombat/Public/Combat/AI/CombatAIContext.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Combat/AI/CombatAITypes.h"
#include "CombatAIContext.generated.h"

// Forward decl (우리 구조의 다른 시스템)
class UBattleSessionSubsystem;
class UTacticalModeSubsystem;
class UTrinityChainSubsystem;

class UHPComponent;
class UAPComponent;
class USkillComponent;
class UThreatComponent;
class UStatusComponent;
class UGroggyComponent;
 
USTRUCT()
struct FCombatAIReservationSnapshot
{
	GENERATED_BODY()
	
	UPROPERTY() bool bHasReservation = false;
	UPROPERTY() FName ReservedSkillId = NAME_None;
	// 필요 시 타겟 스냅샷/ActorId 확장
};

UCLASS()
class JRPG_API UCombatAIContext :public UObject
{
	GENERATED_BODY()

public:
	// Owner
	UPROPERTY() TWeakObjectPtr<AActor> Owner;

	// Subsystems
	UPROPERTY() TObjectPtr<UBattleSessionSubsystem> Session = nullptr;
	UPROPERTY() TObjectPtr<UTacticalModeSubsystem> Tactical = nullptr;
	UPROPERTY() TObjectPtr<UTrinityChainSubsystem> Chain = nullptr;

	// Components
	UPROPERTY() TObjectPtr<UHPComponent> HP = nullptr;
	UPROPERTY() TObjectPtr<UAPComponent> AP = nullptr;
	UPROPERTY() TObjectPtr<USkillComponent> Skill = nullptr;
	UPROPERTY() TObjectPtr<UThreatComponent> Threat = nullptr;// Enemy only
	UPROPERTY() TObjectPtr<UStatusComponent> Status = nullptr;
	UPROPERTY() TObjectPtr<UGroggyComponent> Groggy = nullptr;

	// Cached flags
	UPROPERTY() float NowReal = 0.f;
	UPROPERTY() bool bSessionActive = false;
	UPROPERTY() bool bInputLocked = false;
	UPROPERTY() bool bChainActive = false;
	UPROPERTY() bool bEnemySuppressed = false;

	UPROPERTY() FCombatAIReservationSnapshot Reservation;

public:
	static UCombatAIContext* Build(AActor*InOwner);

	// Common
	bool IsAlive() const;
	float GetHPPercent() const;

	bool IsCCBlocked() const;// Status 기반
	bool IsGroggyStunned() const;// GroggyPhase==Stunned
	bool IsRising() const;

	// Party
	AActor* GetPrimaryTarget() const;

	// Enemy
	AActor* GetThreatTarget() const;

	// Participants (세션 권위)
	void GetPartyMembers(TArray<AActor*> &Out) const;
	void GetEnemies(TArray<AActor*> &Out) const;

private:
	void PullSubsystemsAndComponents();
	void PullDerivedFlags();
	void PullReservationSnapshot();
};