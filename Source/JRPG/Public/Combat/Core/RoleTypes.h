// Source/JRPGCombat/Public/Combat/Core/RoleTypes.h
#pragma once

#include "CoreMinimal.h"
#include "RoleTypes.generated.h"

UENUM()
enum class EJRPGPartyRole : uint8
{
	Attacker, Defender, Supporter, Healer
};

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EPartyRoleMask : uint8
{
	None = 0 UMETA(Hidden),
	Attacker = 1 << 0,
	Defender = 1 << 1,
	Supporter = 1 << 2,
	Healer = 1 << 3,
	All = Attacker | Defender | Supporter | Healer
};

ENUM_CLASS_FLAGS(EPartyRoleMask);

FORCEINLINE EPartyRoleMask RoleToMask(EJRPGPartyRole Role)
{
	switch (Role)
	{
	case EJRPGPartyRole::Attacker:	return EPartyRoleMask::Attacker;
	case EJRPGPartyRole::Defender:	return EPartyRoleMask::Defender;
	case EJRPGPartyRole::Supporter: return EPartyRoleMask::Supporter;
	case EJRPGPartyRole::Healer:	return EPartyRoleMask::Healer;
	default:						return EPartyRoleMask::None;
	}
}
