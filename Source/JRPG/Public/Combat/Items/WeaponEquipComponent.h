#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Core/RoleTypes.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "WeaponEquipComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_FiveParams(FOnWeaponEquipmentChanged, FName /*CharacterId*/, EEquipmentSlotType /*Slot*/, FGuid /*OldInstanceId*/, FGuid /*NewInstanceId*/, FName /*ReasonTag*/);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UWeaponEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Weapon") FName CharacterId = NAME_None;
	UPROPERTY(EditAnywhere, Category = "Weapon") EJRPGPartyRole Role = EJRPGPartyRole::Attacker;

	FOnWeaponEquipmentChanged OnEquipmentChanged;

	UFUNCTION(BlueprintCallable)
	FItemOp EquipWeapon(UInventorySubsystem* Inventory, FGuid InstanceId);

	UFUNCTION(BlueprintCallable)
	FItemOp UnequipWeapon(UInventorySubsystem* Inventory);

	UFUNCTION(BlueprintPure)
	FGuid GetEquippedWeaponInstanceId() const { return WeaponInstanceId; }

private:
	UPROPERTY() FGuid WeaponInstanceId;
	UPROPERTY() FItemInstance EquippedWeaponInstance;
};
