#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatRole : uint8
{
	Defender,
	Attacker,
	Supporter
};

UENUM(BlueprintType)
enum class EBehaviorPresetType : uint8
{
	Basic,
	Aggressive,
	Defensive
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	TargetAttack,
	AoE,
	Melee
};