#pragma once

#include "CoreMinimal.h"
#include "JRPGCoreApiTypes.h"
#include "CombatTimeTypes.generated.h"

UENUM()
enum class ECombatTimeMode : uint8
{
	Normal,
	Slow,
	Frozen
};

UENUM()
enum class ECombatTimePriority : uint8
{
	Low =0,
	Medium =1,
	High =2,
	Critical =3
};

USTRUCT()
struct FCombatTimeHandle
{
	GENERATED_BODY()

	UPROPERTY() uint64 Value =0;

	bool IsValid() const { return Value != 0; }
	void Invalidate() { Value = 0; }
};

USTRUCT()
struct FCombatTimeRequest
{
	GENERATED_BODY()

	UPROPERTY() ECombatTimeMode Mode = ECombatTimeMode::Slow;
	UPROPERTY() ECombatTimePriority Priority = ECombatTimePriority::Medium;

	UPROPERTY() FName OwnerTag = NAME_None;

	// Slow scale (0.01 ~ 1.0)
	UPROPERTY() float TimeScale =0.15f;

	// RealTime duration (<= 5 recommended for tactical)
	UPROPERTY() float DurationRealSec =1.0f;

	UPROPERTY() float BlendInSec =0.10f;
	UPROPERTY() float BlendOutSec =0.12f;
};

USTRUCT()
struct FCombatTimeResult
{
	GENERATED_BODY()

	UPROPERTY() FJRPGOpResult Op = FJRPGOpResult::Ok();
	UPROPERTY() FCombatTimeHandle Handle;
};