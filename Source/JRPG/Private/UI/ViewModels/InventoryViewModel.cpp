#include "UI/ViewModels/InventoryViewModel.h"
#include "Combat/Items/ItemDataAsset.h"
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
		InvSubsystem->OnInventoryChanged.AddUObject(this, &UInventoryViewModel::HandleInventoryChanged);
	}
}

void UInventoryViewModel::Deinitialize()
{
	if (InvSubsystem.IsValid())
	{
		InvSubsystem->OnInventoryChanged.RemoveAll(this);
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

		if (CurrentAugmentComp.IsValid()) CurrentAugmentComp->OnAugmentEquipped.AddUObject(this, &UInventoryViewModel::HandleAugmentEquipped);
		if (CurrentWeaponComp.IsValid()) CurrentWeaponComp->OnEquipmentChanged.AddUObject(this, &UInventoryViewModel::HandleWeaponEquipped);
	}
	RefreshStatsBreakdown();
}

void UInventoryViewModel::FilterItems(EInventoryTab TabType)
{
	CurrentTab = TabType;
	RefreshItemList();
}

void UInventoryViewModel::SearchItems(const FString& Keyword)
{
	CurrentKeyword = Keyword;
	RefreshItemList();
}

void UInventoryViewModel::RefreshItemList()
{
	if (!InvSubsystem.IsValid()) return;

	TArray<FItemInstance> AllInstances;
	InvSubsystem->GetAllInstances(AllInstances);

	TArray<FItemInstance> FilteredItems;
	FilteredItems.Reserve(AllInstances.Num());

	// 1. 탭 분류 필터링
	for (const FItemInstance& Item : AllInstances)
	{
		const UItemDataAsset* ItemDef = InvSubsystem->FindDef(Item.ItemId);
		if (!ItemDef) continue;

		bool bMatch = false;
		switch (CurrentTab)
		{
		case EInventoryTab::Equipment:
			bMatch = (ItemDef->IsWeapon() || ItemDef->IsAugment());
			break;
		case EInventoryTab::Consumable:
			bMatch = ItemDef->IsConsumable();
			break;
		case EInventoryTab::Material:
			bMatch = (ItemDef->ItemType == EItemType::Material || ItemDef->IsKeyItem());
			break;
		}

		// 이름 검색 로직 유지
		if (bMatch && !CurrentKeyword.IsEmpty())
		{
			bMatch = ItemDef->DisplayName.ToString().Contains(CurrentKeyword);
		}

		if (bMatch) FilteredItems.Add(Item);
	}

	// 2. 장비 탭인 경우 O(N) 버킷 정렬 수행
	if (CurrentTab == EInventoryTab::Equipment && CurrentCharacter.IsValid())
	{
		SortEquipmentBucket(FilteredItems);
	}

	OnInventoryListUpdated.Broadcast(FilteredItems);
}

int32 UInventoryViewModel::GetRoleScore(const UItemDataAsset* ItemDef, AActor* TargetChar) const
{
	if (!ItemDef) return 0;
	if (ItemDef->RoleRestrictionMask == 0) return 1; // 공용 아이템

	// TODO: 캐릭터 직업(Role) 검사 연동 부분 (TargetChar->GetRole() 사용)
	// 예시: if (ItemDef->IsRoleAllowed(CharRole)) return 2; 

	return 0; // 타 직업 아이템
}

void UInventoryViewModel::SortEquipmentBucket(TArray<FItemInstance>& InOutItems)
{
	if (InOutItems.IsEmpty()) return;

	const int32 MAX_SCORE_BUCKETS = 25;
	TArray<TArray<FItemInstance>> Buckets;
	Buckets.SetNum(MAX_SCORE_BUCKETS);

	for (const FItemInstance& Item : InOutItems)
	{
		const UItemDataAsset* ItemDef = InvSubsystem->FindDef(Item.ItemId);
		if (!ItemDef) continue;

		int32 RoleScore = GetRoleScore(ItemDef, CurrentCharacter.Get());
		int32 RarityScore = static_cast<int32>(ItemDef->Rarity); // 0 ~ 4

		int32 FinalScore = (RoleScore * 10) + RarityScore;
		if (FinalScore >= 0 && FinalScore < MAX_SCORE_BUCKETS)
		{
			Buckets[FinalScore].Add(Item);
		}
	}

	InOutItems.Empty();
	for (int32 i = MAX_SCORE_BUCKETS - 1; i >= 0; --i)
	{
		for (const FItemInstance& Item : Buckets[i]) InOutItems.Add(Item);
	}
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
void UInventoryViewModel::ClearPreview() { OnEquipPreviewUpdated.Broadcast(FStatsPreviewDelta()); }
void UInventoryViewModel::RequestEquipAugment(FGuid InstanceId, EAugmentEquipSlot Slot) { if (CurrentAugmentComp.IsValid() && InvSubsystem.IsValid()) CurrentAugmentComp->TryEquipFromInventory(InvSubsystem.Get(), InstanceId, Slot); }
void UInventoryViewModel::RequestUnequipAugment(EAugmentEquipSlot Slot) { if (CurrentAugmentComp.IsValid() && InvSubsystem.IsValid()) CurrentAugmentComp->TryUnequipToInventory(InvSubsystem.Get(), Slot); }
void UInventoryViewModel::RequestEquipWeapon(FGuid InstanceId) { if (CurrentWeaponComp.IsValid() && InvSubsystem.IsValid()) CurrentWeaponComp->EquipWeapon(InvSubsystem.Get(), InstanceId); }
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

void UInventoryViewModel::HandleInventoryChanged(EInventoryChangeType ChangeType, FGuid InstanceId, int32 Delta, FName ReasonTag) { RefreshItemList(); }
void UInventoryViewModel::HandleAugmentEquipped(FName CharId, EAugmentEquipSlot Slot, FName Old, FName New, FName Reason) { RefreshStatsBreakdown(); }
void UInventoryViewModel::HandleWeaponEquipped(FName CharId, EEquipmentSlotType Slot, FGuid OldInstanceId, FGuid NewInstanceId, FName ReasonTag) { RefreshStatsBreakdown(); }

const UItemDataAsset* UInventoryViewModel::GetItemDefinition(FName ItemId) const
{
	if (InvSubsystem.IsValid()) return InvSubsystem->FindDef(ItemId);
	return nullptr;
}