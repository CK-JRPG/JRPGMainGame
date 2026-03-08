#pragma once

#include "CoreMinimal.h"
#include "JRPGCore/Public/JRPGCoreApiTypes.h"
#include "StatusTypes.generated.h"

UENUM()
enum class EStatusStackPolicy : uint8
{
	RefreshDuration,   // 이미 있으면 지속시간 갱신
	AddStack,          // 스택 증가
	IgnoreIfExists     // 이미 있으면 무시
};

USTRUCT()
struct FStatusSpec
{
	GENERATED_BODY()

	UPROPERTY() FName StatusId = NAME_None;
	UPROPERTY() float Duration = 0.f;        // <=0 이면 DA의 기본값 사용
	UPROPERTY() int32 StackDelta = 1;
	UPROPERTY() FName SourceTag = NAME_None; // "CombatMotion.WallSlam" 등
	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr;
};

USTRUCT()
struct FActiveStatus
{
	GENERATED_BODY()

	UPROPERTY() FName StatusId = NAME_None;
	UPROPERTY() float Remaining = 0.f;
	UPROPERTY() int32 Stacks = 1;
	UPROPERTY() bool bIsCC = false;
	UPROPERTY() FName LastSourceTag = NAME_None;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnStatusApplied, FName /*StatusId*/, const FActiveStatus& /*State*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatusRemoved, FName /*StatusId*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCCStateChanged, bool /*bNowCC*/);
