#pragma once

#include "CoreMinimal.h"
#include "SPTypes.generated.h"

USTRUCT()
struct FJRPGSPState
{
	GENERATED_BODY()

	UPROPERTY() int32 Current = 0;
	UPROPERTY() int32 Max = 100;
};

USTRUCT()
struct FJRPGSPGainEvent
{
	GENERATED_BODY()

	UPROPERTY() int32 Amount = 0;
	UPROPERTY() FName SourceTag = NAME_None;      // "CombatMotion.WallSlam" 등
	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr;
};