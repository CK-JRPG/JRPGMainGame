#pragma once

#include "CoreMinimal.h"
#include "ChainAttackTypes.generated.h"

UENUM()
enum class EChainAttackState : uint8
{
	Idle,
	Entering,
	Active,
	Finishing
};

USTRUCT()
struct FChainAttackConfig
{
	GENERATED_BODY()

	// 비어 있으면 PartySubsystem의 파티 3인 사용
	UPROPERTY(EditAnywhere) TArray<TWeakObjectPtr<AActor>> ChainMembers;

	UPROPERTY(EditAnywhere) int32 StartingChainPoints = 3;
	UPROPERTY(EditAnywhere) float BaseDamageMultiplier = 1.25f;
	UPROPERTY(EditAnywhere) float BonusDamagePerStep = 0.15f;
	UPROPERTY(EditAnywhere) float GroggyPowerBonus = 10.f;

	// 끝나면 원래 턴을 소모하고 넘길지
	UPROPERTY(EditAnywhere) bool bConsumeBattleTurnOnEnd = true;
};

USTRUCT()
struct FChainAttackSnapshot
{
	GENERATED_BODY()

	UPROPERTY() FGuid BattleSessionId;
	UPROPERTY() FGuid ChainId;

	UPROPERTY() EChainAttackState State = EChainAttackState::Idle;

	UPROPERTY() int32 RemainingChainPoints = 0;
	UPROPERTY() int32 StepIndex = 0;

	UPROPERTY() float CurrentDamageMultiplier = 1.f;

	UPROPERTY() TWeakObjectPtr<AActor> ChainStarter;
	UPROPERTY() TWeakObjectPtr<AActor> CurrentActor;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChainAttackStarted, const FChainAttackSnapshot& /*Snapshot*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChainAttackStepResolved, const FChainAttackSnapshot& /*Snapshot*/, AActor* /*ActingActor*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChainAttackEnded, const FChainAttackSnapshot& /*Snapshot*/);
