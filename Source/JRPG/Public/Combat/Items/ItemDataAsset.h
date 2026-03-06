// Source/JRPGCombat/Public/Combat/Items/ItemDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/ItemModifierTypes.h"
#include "ItemDataAsset.generated.h"

UCLASS()
class JRPGCOMBAT_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 필수
	UPROPERTY(EditAnywhere) FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere) EItemType ItemType = EItemType::Augment;

	UPROPERTY(EditAnywhere) FText DisplayName;
	UPROPERTY(EditAnywhere, Multiline) FText Description;

	UPROPERTY(EditAnywhere) EItemRarity Rarity = EItemRarity::Common;
	UPROPERTY(EditAnywhere) EItemSourcePolicy SourcePolicy = EItemSourcePolicy::Universal;

	// 레벨 요구(장착 조건)
	UPROPERTY(EditAnywhere) int32 RequiredLevel = 0;

	// 인벤/거래 플래그
	UPROPERTY(EditAnywhere) bool bSellable = true;
	UPROPERTY(EditAnywhere) bool bDiscardable = true;
	UPROPERTY(EditAnywhere) bool bLockable = true;

	// 가격 기준값(상점에서 ReferenceValue로 사용)
	UPROPERTY(EditAnywhere) int32 BaseValue = 0;

	// 스택
	UPROPERTY(EditAnywhere) int32 MaxStack = 1;

	// --- Augment 전용 ---
	// 어떤 슬롯에 장착 가능한지 (기본 All)
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/JRPGCombat.EAugmentSlotMask"))
	int32 EquipSlotMask = (int32)EAugmentSlotMask::All;

	// RoleRestriction(옵션). None이면 제한 없음.
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/JRPGCombat.ECombatRoleMask"))
	int32 RoleRestrictionMask = (int32)ECombatRoleMask::None;

	// RoleEfficiency(기본). 제한이 없을 때 역할별 효율 배율을 적용.
	UPROPERTY(EditAnywhere) FRoleEfficiency RoleEfficiency;

	// UniqueEquipGroup(옵션): 같은 그룹은 한 캐릭터에 중복 장착 불가
	UPROPERTY(EditAnywhere) FName UniqueEquipGroup = NAME_None;

	// 효과 리스트
	UPROPERTY(EditAnywhere) TArray<FAugmentEffect> EffectList;

public:
	bool IsAugment() const { return ItemType == EItemType::Augment; }
	bool IsKeyItem() const { return ItemType == EItemType::KeyItem; }

	bool IsRoleAllowed(ECombatRole Role) const
	{
		const ECombatRoleMask Mask = (ECombatRoleMask)RoleRestrictionMask;
		if (Mask == ECombatRoleMask::None) return true;
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