// Source/JRPGCombat/Public/Combat/Items/ItemTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ItemTypes.generated.h"

// Item System SSOT:
// - ItemType: Augment / Consumable / Material / KeyItem(Quest) (판매/장착 불가)
// - Role: Defender / Attacker / Supporter
// - EquipSlot: AugmentSlot1~3

UENUM()
enum class EItemType : uint8
{
	Augment,
	Consumable,
	Material,
	KeyItem
};

UENUM()
enum class EItemSourcePolicy : uint8
{
	Universal,
	ShopOnly,
	LootOnly,
	ExploreOnly
};

UENUM()
enum class ECombatRole : uint8
{
	Defender,
	Attacker,
	Supporter
};

UENUM()
enum class EAugmentEquipSlot : uint8
{
	AugmentSlot1,
	AugmentSlot2,
	AugmentSlot3
};

UENUM()
enum class EItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UENUM()
enum class EItemOpResult : uint8
{
	Success,
	Rejected
};

USTRUCT()
struct FItemOp
{
	GENERATED_BODY()

	UPROPERTY() bool bOk = false;
	UPROPERTY() EItemOpResult Result = EItemOpResult::Rejected;
	UPROPERTY() FName ReasonTag = NAME_None;

	static FItemOp Ok()
	{
		FItemOp O;
		O.bOk = true;
		O.Result = EItemOpResult::Success;
		return O;
	}

	static FItemOp Fail(FName Reason)
	{
		FItemOp O;
		O.bOk = false;
		O.Result = EItemOpResult::Rejected;
		O.ReasonTag = Reason;
		return O;
	}
};

// 역할 마스크(선택적으로 RoleRestriction에 사용)
UENUM(meta = (Bitflags))
enum class ECombatRoleMask : uint8
{
	None = 0,
	Defender = 1 << 0,
	Attacker = 1 << 1,
	Supporter = 1 << 2,
	All = Defender | Attacker | Supporter
};
ENUM_CLASS_FLAGS(ECombatRoleMask);

// 슬롯 마스크
UENUM(meta = (Bitflags))
enum class EAugmentSlotMask : uint8
{
	None = 0,
	AugmentSlot1 = 1 << 0,
	AugmentSlot2 = 1 << 1,
	AugmentSlot3 = 1 << 2,
	All = AugmentSlot1 | AugmentSlot2 | AugmentSlot3
};
ENUM_CLASS_FLAGS(EAugmentSlotMask);

inline ECombatRoleMask RoleToMask(ECombatRole Role)
{
	switch (Role)
	{
	case ECombatRole::Defender: return ECombatRoleMask::Defender;
	case ECombatRole::Attacker: return ECombatRoleMask::Attacker;
	case ECombatRole::Supporter: return ECombatRoleMask::Supporter;
	default: return ECombatRoleMask::None;
	}
}