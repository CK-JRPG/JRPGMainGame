#include "UI/ViewModels/InventoryViewModel.h"
#include "Combat/Items/InventoryPresentationSubsystem.h"
#include "Combat/Items/AugmentEquipComponent.h"
#include "Combat/Items/WeaponEquipComponent.h"
#include "DeveloperSettings/JRPGItemSettings.h"

void UInventoryViewModel::Initialize(UWorld* World)
{
	CachedWorld = World;
	if (!World) return;

	InvSubsystem = World->GetGameInstance()->GetSubsystem<UInventorySubsystem>();
	PresentationSubsystem = World->GetGameInstance()->GetSubsystem<UInventoryPresentationSubsystem>();

	if (InvSubsystem.IsValid())
	{
		InvSubsystem->OnItemAdded.AddUObject(this, &UInventoryViewModel::HandleInventoryChanged);
		InvSubsystem->OnItemRemoved.AddUObject(this, &UInventoryViewModel::HandleInventoryChanged);
	}
}

void UInventoryViewModel::Deinitialize()
{
	if (InvSubsystem.IsValid())
	{
		InvSubsystem->OnItemAdded.RemoveAll(this);
		InvSubsystem->OnItemRemoved.RemoveAll(this);
	}
}

void UInventoryViewModel::SelectCharacter(AActor* CharacterActor)
{
	if (CurrentAugmentComp.IsValid()) CurrentAugmentComp->OnAugmentEquipped.RemoveAll(this);
	if (CurrentWeaponComp.IsValid()) CurrentWeaponComp->OnEquipmentChanged.RemoveAll(this);

	CurrentCharacter = CharacterActor;
	if (CharacterActor)
	{
		CurrentAugmentComp = CharacterActor->FindComponentByClass<UAugmentEquipComponent>();
		CurrentWeaponComp = CharacterActor->FindComponentByClass<UWeaponEquipComponent>();

		if (CurrentAugmentComp.IsValid())
		{
			CurrentAugmentComp->OnAugmentEquipped.AddUObject(this, &UInventoryViewModel::HandleAugmentEquipped);
		}
		if (CurrentWeaponComp.IsValid())
		{
			CurrentWeaponComp->OnEquipmentChanged.AddUObject(this, &UInventoryViewModel::HandleWeaponEquipped);
		}
	}

	RefreshStatsBreakdown();
}

void UInventoryViewModel::FilterItems(EItemType ItemType)
{
	CurrentFilter = ItemType;
	RefreshItemList();
}

void UInventoryViewModel::SearchItems(const FString& Keyword)
{
	CurrentKeyword = Keyword;
	RefreshItemList();
}

void UInventoryViewModel::RefreshItemList()
{
	if (!InvSubsystem.IsValid() || !PresentationSubsystem.IsValid()) return;

	TArray<FItemInstance> FilteredItems = PresentationSubsystem->FilterByType(InvSubsystem.Get(), CurrentFilter);

	if (!CurrentKeyword.IsEmpty())
	{
		FilteredItems = PresentationSubsystem->SearchByName(InvSubsystem.Get(), CurrentKeyword);
	}

	UE_LOG(LogTemp, Warning, TEXT("[ViewModel] 현재 필터(%d)를 통과한 아이템 개수: %d개"), (int32)CurrentFilter, FilteredItems.Num());

	OnInventoryListUpdated.Broadcast(FilteredItems);
}

void UInventoryViewModel::HoverItemForPreview(const FItemInstance& ItemInfo)
{
	if (!PresentationSubsystem.IsValid() || !CurrentCharacter.IsValid() || !InvSubsystem.IsValid()) return;

	// 실제 스탯 컴포넌트와 아이템 DB를 가져와서 프리뷰 계산
	/*
	UCombatStatsComponent* StatsComp = CurrentCharacter->FindComponentByClass<UCombatStatsComponent>();
	const UItemDataAsset* ItemDef = InvSubsystem->FindDef(ItemInfo.ItemId);
	if (StatsComp && ItemDef && ItemDef->IsAugment())
	{
		FAugmentModifierSet CandidateMods;
		FStatsPreviewDelta Delta = PresentationSubsystem->PreviewAugmentDelta(StatsComp, CandidateMods);
		OnEquipPreviewUpdated.Broadcast(Delta);
	}
	*/
}

void UInventoryViewModel::ClearPreview()
{
	OnEquipPreviewUpdated.Broadcast(FStatsPreviewDelta());
}

void UInventoryViewModel::RequestEquipAugment(FGuid InstanceId, EAugmentEquipSlot Slot)
{
	if (CurrentAugmentComp.IsValid() && InvSubsystem.IsValid())
		CurrentAugmentComp->TryEquipFromInventory(InvSubsystem.Get(), InstanceId, Slot);
}

void UInventoryViewModel::RequestUnequipAugment(EAugmentEquipSlot Slot)
{
	if (CurrentAugmentComp.IsValid() && InvSubsystem.IsValid())
		CurrentAugmentComp->TryUnequipToInventory(InvSubsystem.Get(), Slot);
}

void UInventoryViewModel::RequestEquipWeapon(FGuid InstanceId)
{
	if (CurrentWeaponComp.IsValid() && InvSubsystem.IsValid())
		CurrentWeaponComp->EquipWeapon(InvSubsystem.Get(), InstanceId);
}

void UInventoryViewModel::RefreshStatsBreakdown()
{
	if (!CurrentCharacter.IsValid()) return;
	// 캐릭터에서 FStatsBreakdownSnapshot 추출 로직 연동
	//if (UCombatStatsComponent* StatsComp = CurrentCharacter->FindComponentByClass<UCombatStatsComponent>())
	//{
	//	FStatsBreakdownSnapshot Snapshot = StatsComp->GetStatsBreakdownSnapshot();
	//	OnStatsBreakdownUpdated.Broadcast(Snapshot);
	//}
}

void UInventoryViewModel::HandleInventoryChanged(FName ItemId, int32 Qty, FName Tag)
{
	RefreshItemList();
}

void UInventoryViewModel::HandleAugmentEquipped(FName CharId, EAugmentEquipSlot Slot, FName Old, FName New, FName Reason)
{
	RefreshStatsBreakdown();
}

void UInventoryViewModel::HandleWeaponEquipped(FName CharId, EEquipmentSlotType Slot, FGuid OldInstanceId, FGuid NewInstanceId, FName ReasonTag)
{
	RefreshStatsBreakdown();
}

UItemDataAsset* UInventoryViewModel::GetItemDefinition(FName ItemId) const
{
	if (UItemDatabaseAsset* DB = UJRPGItemSettings::GetItemDB())
	{
		return const_cast<UItemDataAsset*>(DB->FindItem(ItemId));
	}
	return nullptr;
}