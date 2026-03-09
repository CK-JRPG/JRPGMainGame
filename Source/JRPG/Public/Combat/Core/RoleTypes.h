// Source/JRPGCombat/Public/Combat/Core/RoleTypes.h
#pragma once

#include "CoreMinimal.h"
#include "RoleTypes.generated.h"

UENUM()
enum class EJRPGPartyRole : uint8
{
	Attacker, Defender, Supporter, Healer
};
