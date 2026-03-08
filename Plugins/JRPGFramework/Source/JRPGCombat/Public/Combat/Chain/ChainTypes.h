#pragma once

#include "CoreMinimal.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "ChainTypes.generated.h"

UENUM()
enum class EChainState : uint8
{
	Idle,
	SelectionFrozen, // Freeze + UI pick
	ExecuteQueue,	 // Step 1->2->3
	Finisher,		 // Finisher skill
	Recover,		 // cleanup
	Aborted
};

UENUM()
enum class EChainEligibilityPolicy : uint8
{
	ChainOnlyEligible, // default recommended
	AllowAllOwnedSkills
};

UENUM()
enum class EEnemySuppressionScope : uint8
{
	StopOnly, // AI stop only
	GateOnly, // damage/status gate only
	StopAndGate
};

USTRUCT()
struct FChainToken
{
	GENERATED_BODY()

	UPROPERTY() 
	FGuid Guid;

	static FChainToken NewToken()
	{
		FChainToken T;
		T.Guid = FGuid::NewGuid();
		return T;
	}

	bool IsValid() const { return Guid.IsValid(); }
};

USTRUCT()
struct FChainStepPick
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Caster;
	UPROPERTY() FName SkillId = NAME_None;
	// 체인 문서 기본: Target은 PrimaryTarget 고정
};

USTRUCT()
struct FChainStepRuntime
{
	GENERATED_BODY()

	UPROPERTY() FChainStepPick Pick;
	UPROPERTY() bool bExecuted = false;
	UPROPERTY() bool bSucceeded = false;

	UPROPERTY() int32 TPGained = 0;
	UPROPERTY() FName FailReasonTag = NAME_None;
};

USTRUCT()
struct FChainContext
{
	GENERATED_BODY()

	UPROPERTY() FChainToken Token;

	UPROPERTY() TWeakObjectPtr<AActor> PrimaryTarget;

	UPROPERTY() TArray<TWeakObjectPtr<AActor>> PartyActors;// <=3

	UPROPERTY() EChainState State = EChainState::Idle;

	UPROPERTY() uint64 TimeHandleValue = 0;// CombatTimeHandle.Value (의존 최소화)

	UPROPERTY() double StartTimeReal = 0.0;
	UPROPERTY() double SelectionDeadlineReal = 0.0;

	UPROPERTY() int32 TPTotal = 0;

	UPROPERTY() bool bEnemySuppressed = false;
	UPROPERTY() bool bChainStunBonusArmed = false;

	UPROPERTY() int32 StepIndex = 0;// 0..2

	UPROPERTY() TArray<FChainStepRuntime> Steps;
};

USTRUCT()
struct FChainStepView
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<AActor> Caster;
	UPROPERTY() FName SkillId = NAME_None;
	UPROPERTY() bool bPicked = false;
	UPROPERTY() bool bCanPick = true;
	UPROPERTY() FName CantPickReason = NAME_None;

	UPROPERTY() bool bExecuted = false;
	UPROPERTY() bool bSucceeded = false;
	UPROPERTY() int32 TPGained = 0;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChainStateChanged, EChainState/*Prev*/, EChainState/*New*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnChainStepPickChanged, AActor*/*Caster*/, bool/*HasPick*/, FName/*SkillId*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnChainTimerUpdated, float/*ElapsedReal*/, float/*RemainingReal*/, float/*Normalized01*/, bool/*bActive*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnChainStepExecuted, int32/*StepIndex*/, AActor*/*Caster*/, FName/*SkillId*/, bool/*bSuccess*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnChainEnded, FChainToken/*Token*/, bool/*bAborted*/, FName/*ReasonTag*/);

/** AI/AI-SP 확장: 체인 자동 채우기(미선택 스텝) */
DECLARE_DELEGATE_RetVal_TwoParams(FName, FChainAutoPickSkillDelegate, AActor*/*Caster*/, AActor*/*PrimaryTarget*/);