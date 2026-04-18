#include "Combat/Items/WeaponEquipComponent.h"

#include "Combat/Items/ItemDataAsset.h"

FItemOp UWeaponEquipComponent::EquipWeapon(UInventorySubsystem* Inventory, FGuid InstanceId)
{
	if (!Inventory)
	{
		return FItemOp::Fail("Equip.InstanceInvalid");
	}

	FItemInstance Instance;
	if (!Inventory->TryGetInstance(InstanceId, Instance))
	{
		return FItemOp::Fail("Equip.InstanceInvalid");
	}

	const UItemDataAsset* Def = Inventory->FindDef(Instance.ItemId);
	if (!Def || Def->ItemType != EItemType::Weapon)
	{
		return FItemOp::Fail("Equip.SlotInvalid");
	}

	if (Def->RequiredLevel > 0)
	{
		// TODO: 캐릭터 레벨 제공자 연결 후 적용
	}

	const FGuid OldWeapon = WeaponInstanceId;
	if (WeaponInstanceId.IsValid())
	{
		FItemOp UnequipResult = UnequipWeapon(Inventory);
		if (!UnequipResult.bOk)
		{
			return UnequipResult;
		}
	}

	const FItemOp RemoveResult = Inventory->RemoveItemByInstance(InstanceId, 1, "Inv.Equip");
	if (!RemoveResult.bOk)
	{
		return RemoveResult;
	}

	WeaponInstanceId = InstanceId;
	EquippedWeaponInstance = Instance;
	Inventory->NotifyEquipped(WeaponInstanceId);
	OnEquipmentChanged.Broadcast(CharacterId, EEquipmentSlotType::Weapon, OldWeapon, WeaponInstanceId, "Inv.Equip");
	return FItemOp::Ok();
}

FItemOp UWeaponEquipComponent::UnequipWeapon(UInventorySubsystem* Inventory)
{
	if (!Inventory)
	{
		return FItemOp::Fail("Equip.InstanceInvalid");
	}

	if (!WeaponInstanceId.IsValid())
	{
		return FItemOp::Fail("Equip.InstanceInvalid");
	}

	const FGuid OldWeapon = WeaponInstanceId;
	const FItemOp RestoreResult = Inventory->RestoreInstance(EquippedWeaponInstance, "Inv.Unequip");
	if (!RestoreResult.bOk)
	{
		return RestoreResult;
	}

	WeaponInstanceId.Invalidate();
	EquippedWeaponInstance = FItemInstance();
	Inventory->NotifyUnequipped(OldWeapon);
	OnEquipmentChanged.Broadcast(CharacterId, EEquipmentSlotType::Weapon, OldWeapon, FGuid(), "Inv.Unequip");
	return FItemOp::Ok();
}
