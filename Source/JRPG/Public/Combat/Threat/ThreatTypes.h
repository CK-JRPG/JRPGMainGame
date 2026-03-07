#pragma once
#include "CoreMinimal.h"
#include "ThreatTypes.generated.h"

USTRUCT()
struct FThreatEntry
{
	GENERATED_BODY()
	UPROPERTY() TWeakObjectPtr<AActor> Source;
	UPROPERTY() float Threat = 0.f;
	UPROPERTY() double LastUpdateReal = 0.0;
};