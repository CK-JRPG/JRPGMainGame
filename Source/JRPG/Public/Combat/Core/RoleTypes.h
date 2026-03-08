// Source/JRPGCombat/Public/Combat/Core/RoleTypes.h
#pragma once


#include "CoreMinimal.h"
#include "RoleTypes.generated.h"

UENUM()
enum class EPartyRole : uint8
{
	Attacker, Defender, Supporter, Healer
};
