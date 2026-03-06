// Source/JRPGCombat/Private/Combat/Items/InventorySubsystem.cpp
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/ItemDatabaseAsset.h"
#include "Combat/Items/ItemDataAsset.h"

const UItemDataAsset* UInventorySubsystem::FindDef(FName ItemId) const
{
	return ItemDB ? ItemDB->FindItem(ItemId) : nullptr;
}

bool UInventorySubsystem::TryGetInstance(FGuid InstanceId, FItemInstance& Out) const
{
	if (const FItemInstance* I = Instances.Find(InstanceId))
	{
		Out = *I;
		return true;
	}
	return false;
}

void UInventorySubsystem::GetAllInstances(TArray<FItemInstance>& Out) const
{
	Out.Reset();
	Out.Reserve(Instances.Num());
	for (const auto& KV : Instances)
	{
		Out.Add(KV.Value);
	}
}

int32 UInventorySubsystem::CountItem(FName ItemId) const
{
	int32 Sum = 0;
	for (const auto& KV : Instances)
	{
		if (KV.Value.ItemId == ItemId)
			Sum += KV.Value.Quantity;
	}
	return Sum;
}

bool UInventorySubsystem::HasItem(FName ItemId, int32 RequiredQty) const
{
	return CountItem(ItemId) >= RequiredQty;
}

TArray<FGuid> UInventorySubsystem::FindStackableInstances(FName ItemId) const
{
	TArray<FGuid>Out;
	for (const auto& KV : Instances)
	{
		const FItemInstance& I = KV.Value;
		if (I.ItemId == ItemId)
		{
			Out.Add(KV.Key);
		}
	}
	return Out;
}

bool UInventorySubsystem::CanAcceptItem(FName ItemId, int32 Quantity, int32* OutNeededNewSlots) const
{
	const UItemDataAsset* Def = FindDef(ItemId);
	if (!Def) return false;

	const int32 MaxStack = FMath::Max(1, Def->MaxStack);
	int32 Remaining = FMath::Max(0, Quantity);

	// 기존 스택 채우기
	if (MaxStack > 1)
	{
		for (const FGuid& Id : FindStackableInstances(ItemId))
		{
			const FItemInstance* I = Instances.Find(Id);
			if (!I)continue;
			const int32 Space = MaxStack - I->Quantity;
			if (Space <= 0)continue;

			const int32 Take = FMath::Min(Space, Remaining);
			Remaining -= Take;
			if (Remaining <= 0) break;
		}
	}

	// Remaining을 담으려면 새 슬롯이 몇 개 필요한지
	int32 NeedSlots = 0;
	if (Remaining > 0)
	{
		NeedSlots = (Remaining + MaxStack - 1) / MaxStack;
	}

	if (OutNeededNewSlots) *OutNeededNewSlots = NeedSlots;

	return (GetUsedSlots() + NeedSlots) <= MaxSlots;
}

FItemOp UInventorySubsystem::AddItem(FName ItemId, int32 Quantity, FName SourceTag, TArray<FGuid>* OutTouchedInstances)
{
	if (ItemId.IsNone() || Quantity <= 0)
		return FItemOp::Fail("Reject.InvalidItemOrQty");

	const UItemDataAsset* Def = FindDef(ItemId);
	if (!Def)
		return FItemOp::Fail("Reject.ItemNotFound");

	int32 NeedSlots = 0;
	if (!CanAcceptItem(ItemId, Quantity, &NeedSlots))
		return FItemOp::Fail("Reject.InventoryFull");

	const int32 MaxStack = FMath::Max(1, Def->MaxStack);
	int32 Remaining = Quantity;

	// 1) 기존 스택 채우기
	if (MaxStack > 1)
	{
		for (const FGuid& Id : FindStackableInstances(ItemId))
		{
			FItemInstance* I = Instances.Find(Id);
			if (!I)continue;
			if (I->bLocked)continue;// 잠긴 스택은 안 섞는 정책(안전)

			const int32 Space = MaxStack - I->Quantity;
			if (Space <= 0)continue;

			const int32 Take = FMath::Min(Space, Remaining);
			I->Quantity += Take;
			Remaining -= Take;

			if (OutTouchedInstances)OutTouchedInstances->Add(Id);
			if (Remaining <= 0)break;
		}
	}

	// 2) 새 인스턴스 생성
	while (Remaining > 0)
	{
		const int32 Give = FMath::Min(MaxStack, Remaining);
		FItemInstance NewInst = FItemInstance::New(ItemId, Give);
		Instances.Add(NewInst.InstanceId, NewInst);
		if (OutTouchedInstances)OutTouchedInstances->Add(NewInst.InstanceId);
		Remaining -= Give;
	}

	OnItemAdded.Broadcast(ItemId, Quantity, SourceTag);
	return FItemOp::Ok();
}

FItemOp UInventorySubsystem::RemoveItemByInstance(FGuid InstanceId, int32 Quantity, FName ReasonTag)
{
	if (!InstanceId.IsValid() || Quantity <= 0)
		return FItemOp::Fail("Reject.InvalidInstanceOrQty");

	FItemInstance* I = Instances.Find(InstanceId);
	if (!I)return FItemOp::Fail("Reject.InstanceNotFound");

	if (I->bLocked)return FItemOp::Fail("Reject.ItemLocked");

	if (I->Quantity < Quantity)return FItemOp::Fail("Reject.InvalidQuantity");

	// 장착 중 판매/삭제 방지 정책에서 사용(상점/폐기 공통)
	// 여기서 “Remove”는 장착 이동에도 쓰이므로, Equip이 remove할 땐 먼저 NotifyEquipped 하기 전에 실행하도록 사용자가 호출 순서를 지켜야 함.
	const int32 OldQty = I->Quantity;
	I->Quantity -= Quantity;

	if (I->Quantity <= 0)
	{
		Instances.Remove(InstanceId);
	}

	OnItemRemoved.Broadcast(I->ItemId, Quantity, ReasonTag);
	return FItemOp::Ok();
}

FItemOp UInventorySubsystem::SetLocked(FGuid InstanceId, bool bLocked)
{
	FItemInstance* I = Instances.Find(InstanceId);
	if (!I) return FItemOp::Fail("Reject.InstanceNotFound");

	if (!FindDef(I->ItemId) || !FindDef(I->ItemId)->bLockable)
		return FItemOp::Fail("Reject.NotLockable");

	I->bLocked = bLocked;
	return FItemOp::Ok();
}

void UInventorySubsystem::NotifyEquipped(FGuid InstanceId)
{
	if (InstanceId.IsValid())
	{
		EquippedInstances.Add(InstanceId);
	}
}

void UInventorySubsystem::NotifyUnequipped(FGuid InstanceId)
{
	if (InstanceId.IsValid())
	{
		EquippedInstances.Remove(InstanceId);
	}
}