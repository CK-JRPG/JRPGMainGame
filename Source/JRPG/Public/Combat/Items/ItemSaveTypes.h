// Source/JRPGCombat/Public/Combat/Items/ItemSaveTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/ItemTypes.h"
#include "ItemSaveTypes.generated.h"

USTRUCT()
struct FInventorySaveData
{
	GENERATED_BODY()

	UPROPERTY() TArray<FItemInstance> Instances;
	UPROPERTY() int32 Gold = 0;
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