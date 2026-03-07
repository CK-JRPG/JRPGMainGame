// Source/JRPGCombat/Private/Combat/Shop/ShopSubsystem.cpp
#include "Combat/Shop/ShopSubsystem.h"
#include "Combat/Shop/ShopVendorDataAsset.h"

#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/EconomySubsystem.h"
#include "Combat/Items/ItemDatabaseAsset.h"
#include "Combat/Items/ItemDataAsset.h"

#include "Engine/World.h"

static uint64 Hash2(const FName& A, const FName& B)
{
	return HashCombineFast(GetTypeHash(A), GetTypeHash(B));
}

UInventorySubsystem* UShopSubsystem::GetInv() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UInventorySubsystem>() : nullptr;
}
UEconomySubsystem* UShopSubsystem::GetEco() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UEconomySubsystem>() : nullptr;
}

double UShopSubsystem::NowReal(const UWorld* World)
{
	return World ? (double)World->GetRealTimeSeconds() : 0.0;
}

uint64 UShopSubsystem::StockKey(FName VendorId, FName ItemId) const
{
	return Hash2(VendorId, ItemId);
}

const UShopVendorDataAsset* UShopSubsystem::FindVendor(FName VendorId) const
{
	for (const UShopVendorDataAsset* V : Vendors)
	{
		if (V && V->VendorId == VendorId) return V;
	}
	return nullptr;
}

const UItemDataAsset* UShopSubsystem::FindItem(FName ItemId) const
{
	return ItemDB ? ItemDB->FindItem(ItemId) : nullptr;
}

FShopStockState& UShopSubsystem::GetOrInitStock(const UShopVendorDataAsset& Vendor, const FShopCatalogEntry& E)
{
	const uint64 K = StockKey(Vendor.VendorId, E.ItemId);
	if (FShopStockState* Found = StockMap.Find(K))
		return *Found;

	FShopStockState S;
	S.VendorId = Vendor.VendorId;
	S.ItemId = E.ItemId;
	S.RemainingStock = (E.StockType == EShopStockType::LimitedStock) ? E.InitialStock : 999999999;
	StockMap.Add(K, S);
	return StockMap.FindChecked(K);
}

FItemOp UShopSubsystem::OpenShop(FName VendorId)
{
	const UShopVendorDataAsset* Vendor = FindVendor(VendorId);
	if (!Vendor) return FItemOp::Fail("Reject.InvalidVendor");

	OpenVendorId = VendorId;
	OnShopOpened.Broadcast(VendorId);
	return FItemOp::Ok();
}

void UShopSubsystem::CloseShop()
{
	if (!OpenVendorId.IsNone())
	{
		OnShopClosed.Broadcast(OpenVendorId);
		OpenVendorId = NAME_None;
	}
}

FItemOp UShopSubsystem::QueryCatalog(FName VendorId, TArray<FShopCatalogEntry>& OutEntries, TArray<FShopStockState>& OutStocks) const
{
	const UShopVendorDataAsset* Vendor = FindVendor(VendorId);
	if (!Vendor) return FItemOp::Fail("Reject.InvalidVendor");

	OutEntries = Vendor->CatalogEntries;

	OutStocks.Reset();
	for (const FShopCatalogEntry& E : Vendor->CatalogEntries)
	{
		const uint64 K = StockKey(VendorId, E.ItemId);
		if (const FShopStockState* S = StockMap.Find(K))
		{
			OutStocks.Add(*S);
		}
		else
		{
			// 아직 초기화 안 된 경우, UI query에서도 초기화해서 “남은 재고”를 보여줄 수 있음
			// (const 함수라 여기선 그대로 비워둠. 실제 프로젝트에선 mutable 처리 또는 InitStock API 권장)
		}
	}

	return FItemOp::Ok();
}

FShopQuote UShopSubsystem::GetBuyQuote(FName VendorId, FName ItemId, int32 Qty, int32 PartyLevel) const
{
	FShopQuote Q;
	if (Qty <= 0) { Q.ReasonTag = "Reject.InvalidQuantity"; return Q; }

	const UShopVendorDataAsset* Vendor = FindVendor(VendorId);
	if (!Vendor) { Q.ReasonTag = "Reject.InvalidVendor"; return Q; }

	// vendor unlock
	if (!Vendor->VendorUnlockCondition.IsMet(PartyLevel))
	{
		Q.ReasonTag = "Reject.VendorLocked";
		return Q;
	}

	// find entry
	const FShopCatalogEntry* Entry = nullptr;
	for (const FShopCatalogEntry& E : Vendor->CatalogEntries)
	{
		if (E.ItemId == ItemId) { Entry = &E; break; }
	}
	if (!Entry) { Q.ReasonTag = "Reject.EntryNotFound"; return Q; }

	if (!Entry->EntryUnlockCondition.IsMet(PartyLevel))
	{
		Q.ReasonTag = "Reject.EntryLocked";
		return Q;
	}

	const UItemDataAsset* Def = FindItem(ItemId);
	if (!Def) { Q.ReasonTag = "Reject.ItemNotFound"; return Q; }

	// ShopOnly expected check(경고 성격): 여기서는 reject하지 않음
	if (Entry->bShopOnlyExpected && Def->SourcePolicy != EItemSourcePolicy::ShopOnly)
	{
		// 개발 로그로만 처리 권장
	}

	// 구매 제한 상점(옵션): 레벨 미달이면 구매도 막음
	if (Vendor->bBlockPurchaseIfLevelTooLow && Def->RequiredLevel > 0 && PartyLevel < Def->RequiredLevel)
	{
		Q.ReasonTag = "Reject.LevelTooLowForPurchase";
		return Q;
	}

	// price
	const float Discount = FMath::Clamp(Entry->DiscountRate, 0.f, 1.f);
	const float Unit = (float)Entry->BaseBuyPrice * Vendor->VendorPriceModifier * (1.f - Discount);

	Q.UnitPrice = FMath::Max(0, (int32)FMath::FloorToInt(Unit));
	Q.Quantity = Qty;
	Q.TotalPrice = Q.UnitPrice * Qty;
	Q.bOk = true;
	return Q;
}

FItemOp UShopSubsystem::TryBuy(FName VendorId, FName ItemId, int32 Qty, int32 PartyLevel, FShopTransactionSnapshot& OutSnap)
{
	OutSnap = FShopTransactionSnapshot();

	UInventorySubsystem* Inv = GetInv();
	UEconomySubsystem* Eco = GetEco();
	if (!Inv || !Eco) return FItemOp::Fail("Reject.MissingServices");

	const UShopVendorDataAsset* Vendor = FindVendor(VendorId);
	if (!Vendor) return FItemOp::Fail("Reject.InvalidVendor");

	// quote/guards
	const FShopQuote Q = GetBuyQuote(VendorId, ItemId, Qty, PartyLevel);
	if (!Q.bOk)
	{
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Buy, Q.ReasonTag);
		return FItemOp::Fail(Q.ReasonTag);
	}

	// stock
	const FShopCatalogEntry* Entry = nullptr;
	for (const FShopCatalogEntry& E : Vendor->CatalogEntries)
	{
		if (E.ItemId == ItemId) { Entry = &E; break; }
	}
	if (!Entry) return FItemOp::Fail("Reject.EntryNotFound");

	FShopStockState& Stock = const_cast<UShopSubsystem*>(this)->GetOrInitStock(*Vendor, *Entry);
	const int32 BeforeStock = Stock.RemainingStock;

	if (Entry->StockType == EShopStockType::LimitedStock && Stock.RemainingStock < Qty)
	{
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Buy, "Reject.OutOfStock");
		return FItemOp::Fail("Reject.OutOfStock");
	}

	// inventory capacity
	if (!Inv->CanAcceptItem(ItemId, Qty, nullptr))
	{
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Buy, "Reject.InventoryFull");
		return FItemOp::Fail("Reject.InventoryFull");
	}

	// currency
	const int32 GoldBefore = Eco->GetGold();
	const FItemOp Spend = Eco->TrySpendGold(Q.TotalPrice, "Shop.Buy");
	if (!Spend.bOk)
	{
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Buy, Spend.ReasonTag);
		return Spend;
	}

	// add item
	const FItemOp Add = Inv->AddItem(ItemId, Qty, "Shop.Buy", nullptr);
	if (!Add.bOk)
	{
		// 롤백(간단): 골드 되돌리기
		Eco->AddGold(Q.TotalPrice, "Shop.Buy.Rollback");
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Buy, Add.ReasonTag);
		return Add;
	}

	// reduce stock
	if (Entry->StockType == EShopStockType::LimitedStock)
	{
		Stock.RemainingStock -= Qty;
		Stock.RemainingStock = FMath::Max(0, Stock.RemainingStock);
		OnShopStockChanged.Broadcast(VendorId, ItemId, Stock.RemainingStock);
	}

	// snapshot
	OutSnap.VendorId = VendorId;
	OutSnap.TransactionType = EShopTxType::Buy;
	OutSnap.ItemId = ItemId;
	OutSnap.Quantity = Qty;
	OutSnap.UnitPrice = Q.UnitPrice;
	OutSnap.TotalPrice = Q.TotalPrice;
	OutSnap.CurrencyBefore = GoldBefore;
	OutSnap.CurrencyAfter = Eco->GetGold();
	OutSnap.StockBefore = BeforeStock;
	OutSnap.StockAfter = Stock.RemainingStock;
	OutSnap.TimestampReal = NowReal(GetWorld());

	OnShopTransactionCompleted.Broadcast(OutSnap);
	return FItemOp::Ok();
}

FShopQuote UShopSubsystem::GetSellQuote(const UInventorySubsystem& Inv, FGuid InstanceId, int32 Qty, float VendorSellRate) const
{
	FShopQuote Q;

	if (!InstanceId.IsValid() || Qty <= 0) { Q.ReasonTag = "Reject.InvalidQuantity"; return Q; }

	FItemInstance Inst;
	if (!Inv.TryGetInstance(InstanceId, Inst)) { Q.ReasonTag = "Reject.InstanceNotFound"; return Q; }
	if (Inst.bLocked) { Q.ReasonTag = "Reject.ItemLocked"; return Q; }
	if (Inv.IsEquipped(InstanceId)) { Q.ReasonTag = "Reject.ItemEquipped"; return Q; }
	if (Inst.Quantity < Qty) { Q.ReasonTag = "Reject.InvalidQuantity"; return Q; }

	const UItemDataAsset* Def = FindItem(Inst.ItemId);
	if (!Def) { Q.ReasonTag = "Reject.ItemNotFound"; return Q; }

	// 판매 제한: KeyItem/QuestItem 판매 불가, bSellable==true만
	if (Def->IsKeyItem()) { Q.ReasonTag = "Reject.KeyOrQuestItem"; return Q; }
	if (!Def->bSellable) { Q.ReasonTag = "Reject.NotSellable"; return Q; }

	const float SellRate = FMath::Clamp(VendorSellRate, 0.f, 0.999f);
	const int32 UnitSell = FMath::Max(0, (int32)FMath::FloorToInt((float)Def->BaseValue * SellRate));

	Q.UnitPrice = UnitSell;
	Q.Quantity = Qty;
	Q.TotalPrice = UnitSell * Qty;
	Q.bOk = true;
	return Q;
}

FItemOp UShopSubsystem::TrySell(FName VendorId, UInventorySubsystem& Inv, UEconomySubsystem& Eco, FGuid InstanceId, int32 Qty, FShopTransactionSnapshot& OutSnap)
{
	OutSnap = FShopTransactionSnapshot();

	const UShopVendorDataAsset* Vendor = FindVendor(VendorId);
	if (!Vendor) return FItemOp::Fail("Reject.InvalidVendor");

	const FShopQuote Q = GetSellQuote(Inv, InstanceId, Qty, Vendor->SellRate);
	if (!Q.bOk)
	{
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Sell, Q.ReasonTag);
		return FItemOp::Fail(Q.ReasonTag);
	}

	FItemInstance Inst;
	Inv.TryGetInstance(InstanceId, Inst);

	const int32 GoldBefore = Eco.GetGold();
	const int32 StockBefore = 0;

	// remove
	const FItemOp Rem = Inv.RemoveItemByInstance(InstanceId, Qty, "Shop.Sell");
	if (!Rem.bOk)
	{
		OnShopTransactionRejected.Broadcast(VendorId, EShopTxType::Sell, Rem.ReasonTag);
		return Rem;
	}

	// add gold
	Eco.AddGold(Q.TotalPrice, "Shop.Sell");

	OutSnap.VendorId = VendorId;
	OutSnap.TransactionType = EShopTxType::Sell;
	OutSnap.ItemId = Inst.ItemId;
	OutSnap.Quantity = Qty;
	OutSnap.UnitPrice = Q.UnitPrice;
	OutSnap.TotalPrice = Q.TotalPrice;
	OutSnap.CurrencyBefore = GoldBefore;
	OutSnap.CurrencyAfter = Eco.GetGold();
	OutSnap.StockBefore = StockBefore;
	OutSnap.StockAfter = 0;
	OutSnap.TimestampReal = NowReal(GetWorld());

	OnShopTransactionCompleted.Broadcast(OutSnap);
	return FItemOp::Ok();
}