#pragma once

#include "CoreMinimal.h"
#include "CombatDebugTypes.generated.h"

UENUM()
enum class ECombatDebugCategory : uint8
{
	System,
	Session,
	Turn,
	Action,
	Skill,
	Item,
	Tactical,
	Chain,
	SP,
	Motion,
	Status,
	Groggy,
	Threat,
	Presentation,
	AI
};

USTRUCT()
struct FCombatDebugEntry
{
	GENERATED_BODY()

	UPROPERTY() double RealTime = 0.0;
	UPROPERTY() float WorldTime = 0.f;
	UPROPERTY() ECombatDebugCategory Category = ECombatDebugCategory::System;
	UPROPERTY() FName Tag = NAME_None;

	UPROPERTY() FString Message;
	UPROPERTY() FString InstigatorName;
	UPROPERTY() FString TargetName;

	UPROPERTY() FLinearColor Color = FLinearColor::White;
};