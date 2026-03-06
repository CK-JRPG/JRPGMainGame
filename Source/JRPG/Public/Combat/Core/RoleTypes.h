// Source/JRPGCombat/Public/Combat/Core/RoleTypes.h
#pragma once

#include "CoreMinimal.h"
#include "RoleTypes.generated.h"

UENUM()
enum class EPartyRole : uint8
{
	Defender,
	Attacker,
	Supporter
};

UENUM(meta = (Bitflags))
enum class EPartyRoleMask : uint8
{
	None      = 0,
	Defender  = 1 << 0,
	Attacker  = 1 << 1,
	Supporter = 1 << 2,
	All       = Defender | Attacker | Supporter
};
ENUM_CLASS_FLAGS(EPartyRoleMask);

inline EPartyRoleMask RoleToMask(EPartyRole Role)
{
	switch (Role)
	{
	case EPartyRole::Defender: return EPartyRoleMask::Defender;
	case EPartyRole::Attacker: return EPartyRoleMask::Attacker;
	case EPartyRole::Supporter: return EPartyRoleMask::Supporter;
	default: return EPartyRoleMask::None;
	}
}