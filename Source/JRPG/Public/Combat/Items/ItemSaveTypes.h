// Source/JRPGCombat/Public/Combat/Items/ItemSaveTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/InventoryTypes.h"
#include "ItemSaveTypes.generated.h"

USTRUCT()
struct FInventorySaveData
{
	GENERATED_BODY()

	UPROPERTY() int32 SaveVersion = 1;
	UPROPERTY() TArray<FItemInstance> Instances;
	UPROPERTY() TMap<FName, int32> LegacyStackItems;
	UPROPERTY() int32 Gold = 0;
	UPROPERTY() FName LastSortKey = "Rarity";
	UPROPERTY() FName LastFilter = NAME_None;
};

USTRUCT()
struct FEquipmentSaveData
{
	GENERATED_BODY()

	UPROPERTY() int32 SaveVersion = 1;
	UPROPERTY() TMap<FName, FEquipmentLoadout> CharacterLoadouts;
};

USTRUCT()
struct FAugmentEquipSaveData
{
	GENERATED_BODY()

	UPROPERTY() FName CharacterId = NAME_None;
	UPROPERTY() bool bS1 = false;
	UPROPERTY() bool bS2 = false;
	UPROPERTY() bool bS3 = false;

	UPROPERTY() FItemInstance Slot1;
	UPROPERTY() FItemInstance Slot2;
	UPROPERTY() FItemInstance Slot3;
};