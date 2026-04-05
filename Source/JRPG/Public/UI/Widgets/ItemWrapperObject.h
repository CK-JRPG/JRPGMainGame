#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/ItemDataAsset.h"
#include "ItemWrapperObject.generated.h"

class UItemDataAsset;

UCLASS(BlueprintType)
class JRPG_API UItemWrapperObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FItemInstance ItemData;

	UPROPERTY(BlueprintReadOnly, Category = "Item") 
	const UItemDataAsset* ItemDef;

	UFUNCTION(BlueprintPure, Category = "Item|StaticData")
	FText GetDisplayName() const
	{
		return ItemDef ? ItemDef->DisplayName : FText::FromString(TEXT("Unknown"));
	}

	UFUNCTION(BlueprintPure, Category = "Item|StaticData")
	FText GetDescription() const
	{
		return ItemDef ? ItemDef->Description : FText::GetEmpty();
	}

	UFUNCTION(BlueprintPure, Category = "Item|StaticData")
	EItemRarity GetRarity() const
	{
		return ItemDef ? ItemDef->Rarity : EItemRarity::Common;
	}
};