#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Items/ItemTypes.h"
#include "Combat/Items/InventoryTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "Combat/Stats/CombatStatsComponent.h"
#include "InventoryViewModel.generated.h"

class UInventorySubsystem;
class UInventoryPresentationSubsystem;
class UAugmentEquipComponent;
class UWeaponEquipComponent;

UENUM(BlueprintType)
enum class EInventoryTab : uint8
{
	Equipment = 0,
	Consumable = 1,
	Material = 2
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryListUpdated, const TArray<FItemInstance>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsBreakdownUpdated, const FStatsBreakdownSnapshot&, StatsBreakdown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipPreviewUpdated, const FStatsPreviewDelta&, PreviewDelta);

UCLASS(BlueprintType)
class JRPG_API UInventoryViewModel : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(UWorld* World);
	void Deinitialize();

	UPROPERTY(BlueprintAssignable, Category = "InventoryUI|Events")
	FOnInventoryListUpdated OnInventoryListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "InventoryUI|Events")
	FOnStatsBreakdownUpdated OnStatsBreakdownUpdated;

	UPROPERTY(BlueprintAssignable, Category = "InventoryUI|Events")
	FOnEquipPreviewUpdated OnEquipPreviewUpdated;

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void SelectCharacter(AActor* CharacterActor);

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void FilterItems(EInventoryTab TabType);

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void SearchItems(const FString& Keyword);

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void HoverItemForPreview(const FItemInstance& ItemInfo);

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void ClearPreview();

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void RequestEquipAugment(FGuid InstanceId, EAugmentEquipSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void RequestUnequipAugment(EAugmentEquipSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "InventoryUI|Commands")
	void RequestEquipWeapon(FGuid InstanceId);

	UFUNCTION(BlueprintPure, Category = "InventoryUI|Data")
	const class UItemDataAsset* GetItemDefinition(FName ItemId) const;

private:
	TWeakObjectPtr<UWorld> CachedWorld;
	TWeakObjectPtr<UInventorySubsystem> InvSubsystem;
	TWeakObjectPtr<UInventoryPresentationSubsystem> PresentationSubsystem;

	TWeakObjectPtr<AActor> CurrentCharacter;
	TWeakObjectPtr<UAugmentEquipComponent> CurrentAugmentComp;
	TWeakObjectPtr<UWeaponEquipComponent> CurrentWeaponComp;

	EInventoryTab CurrentTab = EInventoryTab::Equipment;
	FString CurrentKeyword = TEXT("");

	void RefreshItemList();
	void RefreshStatsBreakdown();

	// --- 버킷 정렬 ---
	void SortEquipmentBucket(TArray<FItemInstance>& InOutItems);
	int32 GetRoleScore(const UItemDataAsset* ItemDef, AActor* TargetChar) const;

	UFUNCTION() void HandleInventoryChanged(EInventoryChangeType ChangeType, FGuid InstanceId, int32 Delta, FName ReasonTag);
	UFUNCTION() void HandleAugmentEquipped(FName CharId, EAugmentEquipSlot Slot, FName Old, FName New, FName Reason);
	UFUNCTION() void HandleWeaponEquipped(FName CharId, EEquipmentSlotType Slot, FGuid OldInstanceId, FGuid NewInstanceId, FName ReasonTag);
};