// Source/JRPGCombat/Private/Combat/Items/InventorySubsystem.cpp
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/ItemDatabaseAsset.h"
#include "Combat/Items/ItemDataAsset.h"
#include "Combat/Items/ItemSaveTypes.h"

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
	for (const auto& KV : Instances) Out.Add(KV.Value);
}

int32 UInventorySubsystem::CountItem(FName ItemId) const
{
	int32 Sum = 0;
	for (const auto& KV : Instances)
		if (KV.Value.ItemId == ItemId) Sum += KV.Value.Quantity;
	return Sum;
}

bool UInventorySubsystem::HasItem(FName ItemId, int32 RequiredQty) const
{
	return CountItem(ItemId) >= RequiredQty;
}

TArray<FGuid> UInventorySubsystem::FindStackableInstances(FName ItemId) const
{
	TArray<FGuid> Out;
	for (const auto& KV : Instances)
		if (KV.Value.ItemId == ItemId) Out.Add(KV.Key);
	return Out;
}

bool UInventorySubsystem::CanAcceptItem(FName ItemId, int32 Quantity, int32* OutNeededNewSlots) const
{
	const UItemDataAsset* Def = FindDef(ItemId);
	if (!Def) return false;

	const int32 MaxStack = FMath::Max(1, Def->MaxStack);
	int32 Remaining = FMath::Max(0, Quantity);

	if (MaxStack > 1)
	{
		for (const FGuid& Id : FindStackableInstances(ItemId))
		{
			const FItemInstance* I = Instances.Find(Id);
			if (!I) continue;
			const int32 Space = MaxStack - I->Quantity;
			if (Space <= 0) continue;

			const int32 Take = FMath::Min(Space, Remaining);
			Remaining -= Take;
			if (Remaining <= 0) break;
		}
	}

	int32 NeedSlots = 0;
	if (Remaining > 0) NeedSlots = (Remaining + MaxStack - 1) / MaxStack;
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

	// 기존 스택 채우기
	if (MaxStack > 1)
	{
		for (const FGuid& Id : FindStackableInstances(ItemId))
		{
			FItemInstance* I = Instances.Find(Id);
			if (!I) continue;
			if (I->bLocked) continue;

			const int32 Space = MaxStack - I->Quantity;
			if (Space <= 0) continue;

			const int32 Take = FMath::Min(Space, Remaining);
			I->Quantity += Take;
			Remaining -= Take;

			if (OutTouchedInstances) OutTouchedInstances->Add(Id);
			if (Remaining <= 0) break;
		}
	}

	// 새 인스턴스 생성
	while (Remaining > 0)
	{
		const int32 Give = FMath::Min(MaxStack, Remaining);
		FItemInstance NewInst = FItemInstance::New(ItemId, Give);
		Instances.Add(NewInst.InstanceId, NewInst);
		if (OutTouchedInstances) OutTouchedInstances->Add(NewInst.InstanceId);
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
	if (!I) return FItemOp::Fail("Reject.InstanceNotFound");
	if (I->bLocked) return FItemOp::Fail("Reject.ItemLocked");
	if (I->Quantity < Quantity) return FItemOp::Fail("Reject.InvalidQuantity");

	I->Quantity -= Quantity;
	const FName ItemId = I->ItemId;

	if (I->Quantity <= 0)
		Instances.Remove(InstanceId);

	OnItemRemoved.Broadcast(ItemId, Quantity, ReasonTag);
	return FItemOp::Ok();
}

bool UInventorySubsystem::CanRestoreInstance(const FItemInstance& Instance, FName& OutReason) const
{
	if (!Instance.InstanceId.IsValid() || Instance.ItemId.IsNone() || Instance.Quantity <= 0)
	{
		OutReason = "Reject.InvalidInstance";
		return false;
	}
	if (Instances.Contains(Instance.InstanceId))
	{
		OutReason = "Reject.DuplicateInstanceId";
		return false;
	}

	const UItemDataAsset* Def = FindDef(Instance.ItemId);
	if (!Def)
	{
		OutReason = "Reject.ItemNotFound";
		return false;
	}

	// Restore는 “인스턴스 보존”이 목적이라 스택 합치지 않고 “슬롯 1개”로 복원한다.
	if ((GetUsedSlots() + 1) > MaxSlots)
	{
		OutReason = "Reject.InventoryFull";
		return false;
	}

	const int32 MaxStack = FMath::Max(1, Def->MaxStack);
	if (Instance.Quantity > MaxStack)
	{
		OutReason = "Reject.QuantityExceedsMaxStack";
		return false;
	}

	return true;
}

FItemOp UInventorySubsystem::RestoreInstance(const FItemInstance& Instance, FName SourceTag)
{
	FName Reason;
	if (!CanRestoreInstance(Instance, Reason))
		return FItemOp::Fail(Reason);

	Instances.Add(Instance.InstanceId, Instance);
	OnItemAdded.Broadcast(Instance.ItemId, Instance.Quantity, SourceTag);
	return FItemOp::Ok();
}

FItemOp UInventorySubsystem::SetLocked(FGuid InstanceId, bool bLocked)
{
	FItemInstance* I = Instances.Find(InstanceId);
	if (!I) return FItemOp::Fail("Reject.InstanceNotFound");

	const UItemDataAsset* Def = FindDef(I->ItemId);
	if (!Def || !Def->bLockable)
		return FItemOp::Fail("Reject.NotLockable");

	I->bLocked = bLocked;
	return FItemOp::Ok();
}

void UInventorySubsystem::NotifyEquipped(FGuid InstanceId)
{
	if (InstanceId.IsValid()) EquippedInstances.Add(InstanceId);
}

void UInventorySubsystem::NotifyUnequipped(FGuid InstanceId)
{
	if (InstanceId.IsValid()) EquippedInstances.Remove(InstanceId);
}

void UInventorySubsystem::ExportSaveData(FInventorySaveData& Out) const
{
	Out = FInventorySaveData();
	Out.Instances.Reserve(Instances.Num());
	for (const auto& KV : Instances) Out.Instances.Add(KV.Value);
	// Gold는 EconomySubsystem이 권위지만, SaveData 구조상 한 곳에 묶어두면 편함(실제 저장 시 Shop/Eco에서 채우기)
}

void UInventorySubsystem::ImportSaveData(const FInventorySaveData& In)
{
	Instances.Reset();
	EquippedInstances.Reset();

	for (const FItemInstance& Inst : In.Instances)
	{
		FName Reason;
		if (!CanRestoreInstance(Inst, Reason))
		{
			// 손상된 세이브 데이터는 스킵(안전)
			continue;
		}
		Instances.Add(Inst.InstanceId, Inst);
	}
}
