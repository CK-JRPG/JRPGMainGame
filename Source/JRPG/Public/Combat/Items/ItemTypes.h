// Source/JRPGCombat/Public/Combat/Items/ItemTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Core/RoleTypes.h"
#include "ItemTypes.generated.h"

UENUM()
enum class EItemType : uint8
{
	Weapon,
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
enum class EItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UENUM()
enum class EAugmentEquipSlot : uint8
{
	AugmentSlot1,
	AugmentSlot2,
	AugmentSlot3
};

UENUM(meta = (Bitflags))
enum class EAugmentSlotMask : uint8
{
	None         = 0,
	AugmentSlot1 = 1 << 0,
	AugmentSlot2 = 1 << 1,
	AugmentSlot3 = 1 << 2,
	All          = AugmentSlot1 | AugmentSlot2 | AugmentSlot3
};
ENUM_CLASS_FLAGS(EAugmentSlotMask);

UENUM()
enum class EInventoryChangeType : uint8
{
	Added,
	Removed,
	Updated,
	Sorted,
	Filtered,
	Cleared
};

UENUM()
enum class EEquipmentSlotType : uint8
{
	Weapon,
	Augment1,
	Augment2,
	Augment3
};

USTRUCT()
struct FItemOp
{
	GENERATED_BODY()

	UPROPERTY() bool bOk = false;
	UPROPERTY() FName ReasonTag = NAME_None;

	static FItemOp Ok()
	{
		FItemOp O;
		O.bOk = true;
		return O;
	}
	static FItemOp Fail(FName Reason)
	{
		FItemOp O;
		O.bOk = false;
		O.ReasonTag = Reason;
		return O;
	}
};