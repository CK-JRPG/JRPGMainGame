// Source/JRPGCombat/Public/Combat/Battle/BattleSessionTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "BattleSessionTypes.generated.h"

UENUM()
enum class EBattleFlowState : uint8
{
	Idle,
	Starting,
	PlayerTurn,
	EnemyTurn,
	ResolvingAction,
	Victory,
	Defeat,
	Ended
};

UENUM()
enum class EBattleEndReason : uint8
{
	Victory,
	Defeat,
	Aborted
};

USTRUCT()
struct FBattleRewardBundle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) int32 BaseExpReward =0;
	UPROPERTY(EditAnywhere) int32 GoldReward =0;
	UPROPERTY(EditAnywhere) int32 BondBPReward =0;
};

USTRUCT()
struct FBattleParticipantSlot
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() ECombatTeam Team = ECombatTeam::Neutral;

	UPROPERTY() bool bAlive = true;
	UPROPERTY() float CachedSpeed = 10.f;
};

USTRUCT()
struct FTurnOrderEntry
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() float Initiative = 0.f;
};

USTRUCT()
struct FBattleSessionConfig
{
	GENERATED_BODY()

// 비어 있으면 PartySubsystem에서 3인 파티를 가져옴
	UPROPERTY(EditAnywhere) TArray<TWeakObjectPtr<AActor>> PlayerSide;
	UPROPERTY(EditAnywhere) TArray<TWeakObjectPtr<AActor>> EnemySide;

	UPROPERTY(EditAnywhere) bool bPullPartyFromPartySubsystemIfPlayerSideEmpty = true;
	UPROPERTY(EditAnywhere) bool bAutoBegin = true;

	UPROPERTY(EditAnywhere) bool bRestoreAPOnTurnStart = true;
	UPROPERTY(EditAnywhere) bool bSkipDeadCombatants = true;

	UPROPERTY(EditAnywhere) bool bEndBattleOnAllEnemiesDefeated = true;
	UPROPERTY(EditAnywhere) bool bEndBattleOnAllPlayersDefeated = true;

	UPROPERTY(EditAnywhere) FBattleRewardBundle VictoryRewards;
	
	UPROPERTY(EditAnywhere) bool bEnableCombatClamp = false;
	UPROPERTY(EditAnywhere) FVector CombatClampCenter = FVector::ZeroVector;
	UPROPERTY(EditAnywhere) float CombatClampRadius = 0.f;
};

USTRUCT()
struct FBattleSessionSnapshot
{
	GENERATED_BODY()

	UPROPERTY() FGuid SessionId;
	UPROPERTY() EBattleFlowState FlowState = EBattleFlowState::Idle;

	UPROPERTY() int32 Round = 0;
	UPROPERTY() int32 TurnIndex = -1;

	UPROPERTY() TWeakObjectPtr<AActor> CurrentTurnActor;
};

USTRUCT()
struct FBattleActorRuntimeState
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() ECombatTeam Team = ECombatTeam::Neutral;
	UPROPERTY() bool bAlive = false;

	UPROPERTY() bool bActionLocked = false;
	UPROPERTY() FName ActionLockReason = NAME_None;

	UPROPERTY() bool bPresentedActionActive = false;
	UPROPERTY() float RemainingRecoverySec = 0.f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleStarted, const FBattleSessionSnapshot &/*Snapshot*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBattleEnded, const FBattleSessionSnapshot &/*Snapshot*/, EBattleEndReason /*Reason*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTurnStarted, AActor* /*Actor*/, int32 /*Round*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTurnEnded, AActor* /*Actor*/, int32 /*Round*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleStateChanged, EBattleFlowState /*NewState*/);