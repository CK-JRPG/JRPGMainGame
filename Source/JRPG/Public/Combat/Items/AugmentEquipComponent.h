// Source/JRPGCombat/Public/Combat/Items/AugmentEquipComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/ItemModifierTypes.h"
#include "AugmentEquipComponent.generated.h"

class UInventorySubsystem;
class UItemDatabaseAsset;
class UItemCapSettingsDataAsset;
class UItemDataAsset;

DECLARE_MULTICAST_DELEGATE_FiveParams(FOnAugmentEquipped, FName/*CharacterId*/, EAugmentEquipSlot/*Slot*/, FName/*OldItemId*/, FName/*NewItemId*/, FName/*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAugmentEquipRejected, FName/*CharacterId*/, FName/*ItemId*/, FName/*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnModifierSetChanged, FName/*CharacterId*/);

// 장착 슬롯에는 "인벤 인스턴스"가 들어간다(해제 시 인벤으로 되돌림 -> 인벤 공간 필요)
USTRUCT()
struct FAugmentSlotState
{
	GENERATED_BODY()

	UPROPERTY() bool bOccupied = false;
	UPROPERTY() FGuid InstanceId;
	UPROPERTY() FName ItemId = NAME_None;
};

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class JRPGCOMBAT_API UAugmentEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAugmentEquipComponent();

	UPROPERTY(EditAnywhere, Category = "Augment") ECombatRole Role = ECombatRole::Attacker;

	// 캐릭터 식별(저장/이벤트)
	UPROPERTY(EditAnywhere, Category = "Augment") FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Augment") TObjectPtr<UItemDatabaseAsset> ItemDB = nullptr;
	UPROPERTY(EditAnywhere, Category = "Augment") TObjectPtr<UItemCapSettingsDataAsset> CapSettings = nullptr;

	// RequiredLevel 체크용(있으면 사용)
	UPROPERTY(EditAnywhere, Category = "Augment") TScriptInterface<ICombatLevelProvider> LevelProvider;

	// Events (SSOT)
	FOnAugmentEquipped OnAugmentEquipped;
	FOnAugmentEquipRejected OnAugmentEquipRejected;
	FOnModifierSetChanged OnModifierSetChanged;

	// Query
	bool IsSlotOccupied(EAugmentEquipSlot Slot) const;
	FAugmentSlotState GetSlotState(EAugmentEquipSlot Slot) const;

	bool HasUniqueGroupEquipped(FName UniqueGroup) const;

	// Modifier set (Stats/Skill 계산용)
	const FAugmentModifierSet& GetCachedModifierSet() const { return CachedMods; }
	void RebuildModifierSet();

	// API
	FItemOp TryEquipFromInventory(UInventorySubsystem* Inventory, FGuid InstanceId, EAugmentEquipSlot Slot);
	FItemOp TryUnequipToInventory(UInventorySubsystem* Inventory, EAugmentEquipSlot Slot);

private:
	UPROPERTY() TMap<EAugmentEquipSlot, FAugmentSlotState> Slots;
	UPROPERTY() FAugmentModifierSet CachedMods;

	const UItemDataAsset* FindDef(FName ItemId) const;

	FItemOp ValidateEquip(const UItemDataAsset* Def, EAugmentEquipSlot Slot) const;

	// Apply one effect into accumulator with role efficiency + cap
	void ApplyEffectWithCaps(const UItemDataAsset* Def, const FAugmentEffect& E, float RoleEff);

	float ApplyCapPct(FName CapGroup, float PctSum) const;
};