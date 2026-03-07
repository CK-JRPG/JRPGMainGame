#pragma once
#include "CoreMinimal.h"
#include "StatusEffectTypes.generated.h"

UENUM()
enum class EStatusStackPolicy : uint8
{
	RefreshDuration,
	AddStacksClamp,
	IgnoreIfExists
};

USTRUCT()
struct FPeriodicEffect
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere) float PeriodSec = 0.f;
	UPROPERTY(EditAnywhere) float DamagePerTick = 0.f;
	UPROPERTY(EditAnywhere) float HealPerTick = 0.f;
};