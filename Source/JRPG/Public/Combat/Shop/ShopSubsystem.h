// Source/JRPGCombat/Public/Combat/Shop/ShopSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Shop/ShopTypes.h"
#include "ShopSubsystem.generated.h"

class UShopVendorDataAsset;
class UItemDatabaseAsset;
class UInventorySubsystem;
class UEconomySubsystem;
class UItemDataAsset;
class UAugmentEquipComponent;
class ICombatLevelProvider;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopOpened, FName/*VendorId*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopClosed, FName/*VendorId*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShopTransactionCompleted, const FShopTransactionSnapshot&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnShopTransactionRejected, FName/*VendorId*/, EShopTxType/*Type*/, FName/*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnShopStockChanged, FName/*VendorId*/, FName/*ItemId*/, int32/*Remaining*/);

UCLASS()
class JRPGCOMBAT_API UShopSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) TObjectPtr<UItemDatabaseAsset> ItemDB = nullptr;

	// Vendors registry(프로젝트에서는 DataAsset 목록을 GameInstance에 넣거나, AssetManager로 조회)
	UPROPERTY(EditAnywhere) TArray<TObjectPtr<UShopVendorDataAsset>> Vendors;

	// Events
	FOnShopOpened OnShopOpened;
	FOnShopClosed OnShopClosed;
	FOnShopTransactionCompleted OnShopTransactionCompleted;
	FOnShopTransactionRejected OnShopTransactionRejected;
	FOnShopStockChanged OnShopStockChanged;

	// Open/Close
	FItemOp OpenShop(FName VendorId);
	void CloseShop();

	// Queries
	FItemOp QueryCatalog(FName VendorId,/*out*/TArray<FShopCatalogEntry>& OutEntries,/*out*/TArray<FShopStockState>& OutStocks) const;

	FShopQuote GetBuyQuote(FName VendorId, FName ItemId, int32 Qty, int32 PartyLevel) const;
	FItemOp TryBuy(FName VendorId, FName ItemId, int32 Qty, int32 PartyLevel,/*out*/FShopTransactionSnapshot& OutSnap);

	FShopQuote GetSellQuote(const UInventorySubsystem& Inv, FGuid InstanceId, int32 Qty, float VendorSellRate) const;
	FItemOp TrySell(FName VendorId, UInventorySubsystem& Inv, UEconomySubsystem& Eco, FGuid InstanceId, int32 Qty,/*out*/FShopTransactionSnapshot& OutSnap);

private:
	UPROPERTY() FName OpenVendorId = NAME_None;

	// stock runtime state: VendorId+ItemId -> state
	UPROPERTY() TMap<uint64, FShopStockState> StockMap;

	const UShopVendorDataAsset* FindVendor(FName VendorId) const;
	const UItemDataAsset* FindItem(FName ItemId) const;

	uint64 StockKey(FName VendorId, FName ItemId) const;
	FShopStockState& GetOrInitStock(const UShopVendorDataAsset& Vendor, const FShopCatalogEntry& E);

	static double NowReal(const UWorld* World);

	// dependencies
	UInventorySubsystem* GetInv() const;
	UEconomySubsystem* GetEco() const;
};