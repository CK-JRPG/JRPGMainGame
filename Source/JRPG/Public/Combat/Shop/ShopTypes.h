// Source/JRPGCombat/Public/Combat/Shop/ShopTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Combat/Items/ItemTypes.h"
#include "ShopTypes.generated.h"

UENUM()
enum class EShopStockType : uint8
{
	InfiniteStock,
	LimitedStock,
	RotatingStock
};

UENUM()
enum class EShopTxType : uint8
{
	Buy,
	Sell
};

USTRUCT()
struct FShopUnlockCondition
{
	GENERATED_BODY()

	// 최소 구현: 레벨 기반(확장: 챕터/플래그)
	UPROPERTY(EditAnywhere) int32 MinPartyLevel = 0;

	bool IsMet(int32 PartyLevel) const
	{
		return PartyLevel >= MinPartyLevel;
	}
};

USTRUCT()
struct FShopCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName ItemId = NAME_None;
	UPROPERTY(EditAnywhere) EShopStockType StockType = EShopStockType::InfiniteStock;

	UPROPERTY(EditAnywhere) int32 InitialStock = 0;// Limited일 때
	UPROPERTY(EditAnywhere) int32 BaseBuyPrice = 0;
	UPROPERTY(EditAnywhere) float DiscountRate = 0.f;// 0~1

	UPROPERTY(EditAnywhere) FName SortCategory = "Utility";

	UPROPERTY(EditAnywhere) bool bShopOnlyExpected = true;

	UPROPERTY(EditAnywhere) FShopUnlockCondition EntryUnlockCondition;
};

USTRUCT()
struct FShopStockState
{
	GENERATED_BODY()

	UPROPERTY() FName VendorId = NAME_None;
	UPROPERTY() FName ItemId = NAME_None;
	UPROPERTY() int32 RemainingStock = 0;

	// RestockKey(확장): 챕터/플래그 스냅샷
	UPROPERTY() int32 LastRestockKey = 0;
};

USTRUCT()
struct FShopQuote
{
	GENERATED_BODY()

	UPROPERTY() bool bOk = false;
	UPROPERTY() FName ReasonTag = NAME_None;

	UPROPERTY() int32 UnitPrice = 0;
	UPROPERTY() int32 Quantity = 0;
	UPROPERTY() int32 TotalPrice = 0;
};

USTRUCT()
struct FShopTransactionSnapshot
{
	GENERATED_BODY()

	UPROPERTY() FName VendorId = NAME_None;
	UPROPERTY() EShopTxType TransactionType = EShopTxType::Buy;

	UPROPERTY() FName ItemId = NAME_None;
	UPROPERTY() int32 Quantity = 0;

	UPROPERTY() int32 UnitPrice = 0;
	UPROPERTY() int32 TotalPrice = 0;

	UPROPERTY() int32 CurrencyBefore = 0;
	UPROPERTY() int32 CurrencyAfter = 0;

	UPROPERTY() int32 StockBefore = 0;
	UPROPERTY() int32 StockAfter = 0;

	UPROPERTY() double TimestampReal = 0.0;
};