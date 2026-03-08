// Source/JRPGCombat/Public/Combat/Battle/BattleSessionTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Characters/CombatTeamTypes.h"
#include "BattleSessionTypes.generated.h"

UENUM()
enum class EBattlePhase :uint8
{
	Idle,
	Starting,
	Active,
	Ending,
	Cleanup
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

	UPROPERTY(EditAnywhere)int32 BaseExpReward = 0;
	UPROPERTY(EditAnywhere)int32 GoldReward = 0;
	UPROPERTY(EditAnywhere)int32 BondBPReward = 0;
};

USTRUCT()
struct FBattleParticipantSlot
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	UPROPERTY() ECombatTeam Team = ECombatTeam::Neutral;
	UPROPERTY() bool bAlive = true;

	// 실시간 전투용
	UPROPERTY() double NextActionAllowedReal = 0.0;
	UPROPERTY() bool bActionLocked = false;
	UPROPERTY() FName ActionLockReason = NAME_None;
};

USTRUCT()
struct FBattleSessionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) TArray<TWeakObjectPtr<AActor>> PlayerSide;
	UPROPERTY(EditAnywhere) TArray<TWeakObjectPtr<AActor>> EnemySide;

	UPROPERTY(EditAnywhere) bool bPullPartyFromPartySubsystemIfPlayerSideEmpty = true;
	UPROPERTY(EditAnywhere) bool bEndBattleOnAllEnemiesDefeated = true;
	UPROPERTY(EditAnywhere) bool bEndBattleOnAllPlayersDefeated = true;

	UPROPERTY(EditAnywhere)float DefaultActionRecoverySec = 0.25f;

// Clamp
	UPROPERTY(EditAnywhere)bool bEnableCombatClamp = false;
	UPROPERTY(EditAnywhere)FVector CombatClampCenter = FVector::ZeroVector;
	UPROPERTY(EditAnywhere)float CombatClampRadius = 0.f;

	UPROPERTY(EditAnywhere)FBattleRewardBundle VictoryRewards;
};

USTRUCT()
struct FBattleSessionSnapshot
{
	GENERATED_BODY()

	UPROPERTY() FGuid SessionId;
	UPROPERTY() EBattlePhase Phase = EBattlePhase::Idle;

	UPROPERTY() int32 AlivePlayers = 0;
	UPROPERTY() int32 AliveEnemies = 0;
	UPROPERTY() int32 ActivePresentedActionCount = 0;

	UPROPERTY() bool bExclusiveMode = false;
	UPROPERTY() FName ExclusiveModeTag = NAME_None;
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

