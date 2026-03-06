// Source/JRPGCombat/Public/Combat/Shop/ShopVendorDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Shop/ShopTypes.h"
#include "ShopVendorDataAsset.generated.h"

UCLASS()
class JRPGCOMBAT_API UShopVendorDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) FName VendorId = NAME_None;
	UPROPERTY(EditAnywhere) FText DisplayName;

	// SellRate는 항상 1.0 미만
	UPROPERTY(EditAnywhere) float SellRate = 0.4f;

	UPROPERTY(EditAnywhere) float VendorPriceModifier = 1.0f;

	UPROPERTY(EditAnywhere) bool bBlockPurchaseIfLevelTooLow = false;

	UPROPERTY(EditAnywhere) FShopUnlockCondition VendorUnlockCondition;

	UPROPERTY(EditAnywhere) TArray<FShopCatalogEntry> CatalogEntries;
};