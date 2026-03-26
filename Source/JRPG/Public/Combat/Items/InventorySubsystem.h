// Source/JRPGCombat/Public/Combat/Items/InventorySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Items/ItemTypes.h"
#include "InventorySubsystem.generated.h"

class UItemDatabaseAsset;
class UItemDataAsset;

USTRUCT()
struct FItemInstance
{
	GENERATED_BODY()

	UPROPERTY() FGuid InstanceId;
	UPROPERTY() FName ItemId = NAME_None;
	UPROPERTY() int32 Quantity = 1;
	UPROPERTY() bool bLocked = false;
	UPROPERTY() bool bFavorite = false;
	UPROPERTY() FDateTime AcquiredAt;

// 강화/랜덤옵션 확장 대비(현재 미사용)
	UPROPERTY() int32 EnhanceLevel = 0;
	UPROPERTY() int32 CustomSeed = 0;

	static FItemInstance New(FName InItemId,int32 Qty)
	{
		FItemInstance I;
		I.InstanceId = FGuid::NewGuid();
		I.ItemId = InItemId;
		I.Quantity = Qty;
		I.AcquiredAt = FDateTime::UtcNow();
		return I;
	}
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemAdded, FName /*ItemId*/, int32 /*Qty*/, FName /*SourceTag*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemRemoved, FName /*ItemId*/, int32 /*Qty*/, FName /*ReasonTag*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnInventoryChanged, EInventoryChangeType /*ChangeType*/, FGuid /*InstanceId*/, int32 /*Delta*/, FName /*ReasonTag*/);

UCLASS()
class JRPG_API UInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) TObjectPtr<UItemDatabaseAsset> ItemDB = nullptr;
	UPROPERTY(EditAnywhere) int32 MaxSlots = 120;

	FOnItemAdded OnItemAdded;
	FOnItemRemoved OnItemRemoved;
	FOnInventoryChanged OnInventoryChanged;

	FItemOp AddItem(FName ItemId, int32 Quantity, FName SourceTag, /*out*/ TArray<FGuid>* OutTouchedInstances = nullptr);
	FItemOp RemoveItemByInstance(FGuid InstanceId, int32 Quantity, FName ReasonTag);

	// 인스턴스 그대로 복원(강화/옵션 보존 핵심)
	FItemOp RestoreInstance(const FItemInstance& Instance, FName SourceTag);
	
	FItemOp SetLocked(FGuid InstanceId, bool bLocked);
	FItemOp setFavorite(FGuid InstanceId, bool bFavorite);

	bool HasItem(FName ItemId, int32 RequiredQty) const;
	int32 CountItem(FName ItemId) const;

	bool TryGetInstance(FGuid InstanceId, FItemInstance& Out) const;
	void GetAllInstances(TArray<FItemInstance>& Out) const;

	int32 GetUsedSlots() const { return Instances.Num(); }
	bool CanAcceptItem(FName ItemId, int32 Quantity, int32* OutNeededNewSlots = nullptr) const;

	void NotifyEquipped(FGuid InstanceId);
	void NotifyUnequipped(FGuid InstanceId);
	bool IsEquipped(FGuid InstanceId) const { return EquippedInstances.Contains(InstanceId); }

	const UItemDataAsset* FindDef(FName ItemId) const;

	FItemOp SortByRarity(bool bDescending);
	void ClearInventory();

	// Save/Load
	void ExportSaveData(struct FInventorySaveData& Out) const;
	void ImportSaveData(const struct FInventorySaveData& In);

private:
	UPROPERTY() TMap<FGuid, FItemInstance> Instances;
	UPROPERTY() TSet<FGuid> EquippedInstances;

	TArray<FGuid> FindStackableInstances(FName ItemId) const;

	bool CanRestoreInstance(const FItemInstance& Instance, FName& OutReason) const;
};