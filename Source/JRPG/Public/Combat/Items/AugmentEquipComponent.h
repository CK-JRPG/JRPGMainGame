// Source/JRPGCombat/Public/Combat/Items/AugmentEquipComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Combat/Core/RoleTypes.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/ItemModifierTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/CombatLevelProvider.h"
#include "Combat/Items/ItemSaveTypes.h"

#include "AugmentEquipComponent.generated.h"

class UItemDatabaseAsset;
class UItemCapSettingsDataAsset;
class UItemDataAsset;

USTRUCT()
struct FAugmentSlotState
{
	GENERATED_BODY()

	UPROPERTY() bool bOccupied = false;
	UPROPERTY() FItemInstance StoredInstance;
};

DECLARE_MULTICAST_DELEGATE_FiveParams(FOnAugmentEquipped, FName /*CharId*/, EAugmentEquipSlot /*Slot*/, FName /*Old*/, FName /*New*/, FName /*Reason*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAugmentEquipRejected, FName /*CharId*/, FName /*ItemId*/, FName /*Reason*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnModifierSetChanged, FName /*CharId*/);

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPG_API UAugmentEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAugmentEquipComponent();

	UPROPERTY(EditAnywhere, Category = "Augment") EPartyRole Role = EPartyRole::Attacker;
	UPROPERTY(EditAnywhere, Category = "Augment") FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Augment") TObjectPtr<UItemDatabaseAsset> ItemDB = nullptr;
	UPROPERTY(EditAnywhere, Category = "Augment") TObjectPtr<UItemCapSettingsDataAsset> CapSettings = nullptr;

	UPROPERTY(EditAnywhere, Category = "Augment") TScriptInterface<ICombatLevelProvider> LevelProvider;

	FOnAugmentEquipped OnAugmentEquipped;
	FOnAugmentEquipRejected OnAugmentEquipRejected;
	FOnModifierSetChanged OnModifierSetChanged;

	bool IsSlotOccupied(EAugmentEquipSlot Slot) const;
	FAugmentSlotState GetSlotState(EAugmentEquipSlot Slot) const;

	const FAugmentModifierSet& GetCachedModifierSet() const { return CachedMods; }
	void RebuildModifierSet();

	FItemOp TryEquipFromInventory(UInventorySubsystem* Inventory, FGuid InstanceId, EAugmentEquipSlot Slot);
	FItemOp TryUnequipToInventory(UInventorySubsystem* Inventory, EAugmentEquipSlot Slot);

	// Save/Load
	void ExportSaveData(FAugmentEquipSaveData& Out) const;
	FItemOp ImportSaveData(UInventorySubsystem* Inventory, const FAugmentEquipSaveData& In);

private:
	UPROPERTY() TMap<EAugmentEquipSlot, FAugmentSlotState> Slots;
	UPROPERTY() FAugmentModifierSet CachedMods;

	const UItemDataAsset* FindDef(FName ItemId) const;

	FItemOp ValidateEquip(const UItemDataAsset* Def, EAugmentEquipSlot Slot) const;
	bool HasUniqueGroupEquipped(FName UniqueGroup) const;

	void ApplyEffectWithCaps(const UItemDataAsset* Def, const FAugmentEffect& E, float RoleEff);
	float ApplyCapPct(FName CapGroup, float PctSum) const;
};