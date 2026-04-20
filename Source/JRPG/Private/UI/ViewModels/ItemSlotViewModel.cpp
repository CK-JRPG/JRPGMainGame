#include "UI/ViewModels/ItemSlotViewModel.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Items/InventoryTypes.h"

void UItemSlotViewModel::Initialize(FGuid InInstanceId, UInventorySubsystem* InInvSubsystem)
{
	InstanceId = InInstanceId;
	WeakInvSubsystem = InInvSubsystem;
}

int32 UItemSlotViewModel::GetQuantity() const
{
	if (UInventorySubsystem* InvSub = WeakInvSubsystem.Get())
	{
		FItemInstance Inst;
		if (InvSub->TryGetInstance(InstanceId, Inst))
		{
			return Inst.Quantity;
		}
	}
	return 0;
}

bool UItemSlotViewModel::IsEquipped() const
{
	if (UInventorySubsystem* InvSub = WeakInvSubsystem.Get())
	{
		return InvSub->IsEquipped(InstanceId);
	}
	return false;
}

const UItemDataAsset* UItemSlotViewModel::GetItemDef() const
{
	if (UInventorySubsystem* InvSub = WeakInvSubsystem.Get())
	{
		FItemInstance Inst;
		if (InvSub->TryGetInstance(InstanceId, Inst))
		{
			return InvSub->FindDef(Inst.ItemId);
		}
	}
	return nullptr;
}
