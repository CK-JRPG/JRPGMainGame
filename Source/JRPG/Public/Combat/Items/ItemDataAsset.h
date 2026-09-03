// Source/JRPG/Public/Combat/Items/ItemDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/ItemModifierTypes.h"
#include "Combat/Items/ItemUseTypes.h"
#include "ItemDataAsset.generated.h"

UCLASS()
class JRPG_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere)
	EItemType ItemType = EItemType::Augment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere)
	EItemRarity Rarity = EItemRarity::Common;
	UPROPERTY(EditAnywhere)
	EItemSourcePolicy SourcePolicy = EItemSourcePolicy::Universal;

	UPROPERTY(EditAnywhere)
	int32 RequiredLevel = 0;

	UPROPERTY(EditAnywhere)
	bool bSellable = true;
	UPROPERTY(EditAnywhere)
	bool bDiscardable = true;
	UPROPERTY(EditAnywhere)
	bool bLockable = true;

	UPROPERTY(EditAnywhere)
	int32 BaseValue = 0;
	UPROPERTY(EditAnywhere)
	int32 MaxStack = 1;

	// ---- Augment ----
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/JRPG.EAugmentSlotMask"))
	int32 EquipSlotMask = (int32)EAugmentSlotMask::All;

	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/JRPG.EPartyRoleMask"))
	int32 RoleRestrictionMask = (int32)EPartyRoleMask::None;


	UPROPERTY(EditAnywhere)
	FRoleEfficiency RoleEfficiency;
	UPROPERTY(EditAnywhere)
	FName UniqueEquipGroup = NAME_None;
	UPROPERTY(EditAnywhere)
	TArray<FAugmentEffect> EffectList;

	// ---- Consumable ----
	UPROPERTY(EditAnywhere, Category = "Consumable")
	EItemUseTargeting Targeting = EItemUseTargeting::Self;

	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bUsableInCombat = true;

	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bUsableOutOfCombat = true;

	UPROPERTY(EditAnywhere, Category = "Consumable")
	bool bUsableDuringChainSequence = false;

	UPROPERTY(EditAnywhere, Category = "Consumable")
	TArray<FConsumableEffect> UseEffects;

	// ---- Enhance 확장 “예약” ----
	UPROPERTY(EditAnywhere, Category = "Enhance")
	FName EnhancementGroup = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Enhance")
	int32 MaxEnhanceLevel = 0;

public:
	bool IsWeapon() const { return ItemType == EItemType::Weapon; }
	bool IsAugment() const { return ItemType == EItemType::Augment; }
	bool IsConsumable() const { return ItemType == EItemType::Consumable; }
	bool IsKeyItem() const { return ItemType == EItemType::KeyItem; }

	bool IsRoleAllowed(EJRPGPartyRole Role) const
	{
		const EPartyRoleMask Mask = (EPartyRoleMask)RoleRestrictionMask;
		if (Mask == EPartyRoleMask::None) return true;
		return EnumHasAnyFlags(Mask, RoleToMask(Role));
	}
	
	bool IsSlotAllowed(EAugmentEquipSlot Slot) const
	{
		const EAugmentSlotMask Mask = (EAugmentSlotMask)EquipSlotMask;
		switch (Slot)
		{
		case EAugmentEquipSlot::AugmentSlot1: return EnumHasAnyFlags(Mask, EAugmentSlotMask::AugmentSlot1);
		case EAugmentEquipSlot::AugmentSlot2: return EnumHasAnyFlags(Mask, EAugmentSlotMask::AugmentSlot2);
		case EAugmentEquipSlot::AugmentSlot3: return EnumHasAnyFlags(Mask, EAugmentSlotMask::AugmentSlot3);
		default: return false;
		}
	}
};
