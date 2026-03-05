#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.h"
#include "ThreatTypes.generated.h"

USTRUCT()
struct FThreatEntry
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<AActor> Target = nullptr;
	UPROPERTY() float Value = 0.f;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnThreatTargetChanged, AActor* /*Old*/, AActor* /*New*/);