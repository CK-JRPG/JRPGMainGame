// Source/JRPGCombat/Public/Combat/Items/InventorySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Items/ItemTypes.h"
#include "InventorySubsystem.generated.h"

class UItemDatabaseAsset;
class UItemDataAsset;

// 인벤 인스턴스: GUID
USTRUCT()
struct FItemInstance
{
	GENERATED_BODY()

	UPROPERTY() FGuid InstanceId;
	UPROPERTY() FName ItemId = NAME_None;
	UPROPERTY() int32 Quantity = 1;

	UPROPERTY() bool bLocked = false;

	// 확장 대비
	UPROPERTY() int32 EnhanceLevel = 0;
	UPROPERTY() int32 CustomSeed = 0;

	static FItemInstance New(FName InItemId, int32 Qty)
		{
			FItemInstance I;
			I.InstanceId = FGuid::NewGuid();
			I.ItemId = InItemId;
			I.Quantity = Qty;
			return I;
		}
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemAdded, FName /*ItemId*/, int32 /*Qty*/, FName /*SourceTag*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemRemoved, FName /*ItemId*/, int32 /*Qty*/, FName /*ReasonTag*/);

UCLASS()
class JRPGCOMBAT_API UInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) TObjectPtr<UItemDatabaseAsset> ItemDB = nullptr;

	// 인벤 수용 규칙(최소): 슬롯 개수
	UPROPERTY(EditAnywhere) int32 MaxSlots = 120;

	// Events (SSOT)
	FOnItemAdded OnItemAdded;
	FOnItemRemoved OnItemRemoved;

	// --- Core API ---
	FItemOp AddItem(FName ItemId, int32 Quantity, FName SourceTag,/*out*/TArray<FGuid>* OutTouchedInstances = nullptr);
	FItemOp RemoveItemByInstance(FGuid InstanceId, int32 Quantity, FName ReasonTag);

	FItemOp SetLocked(FGuid InstanceId, bool bLocked);

	bool HasItem(FName ItemId, int32 RequiredQty) const;
	int32 CountItem(FName ItemId) const;

	bool TryGetInstance(FGuid InstanceId, FItemInstance& Out) const;

	void GetAllInstances(TArray<FItemInstance>& Out) const;

	// Slot/Capacity
	int32 GetUsedSlots() const { return Instances.Num(); }
	bool CanAcceptItem(FName ItemId, int32 Quantity, int32* OutNeededNewSlots = nullptr) const;

	// Equipped tracking (상점 판매 제한/장착 중 판매 불가)
	void NotifyEquipped(FGuid InstanceId);
	void NotifyUnequipped(FGuid InstanceId);
	bool IsEquipped(FGuid InstanceId) const { return EquippedInstances.Contains(InstanceId); }

	// Helpers
	const UItemDataAsset* FindDef(FName ItemId) const;

private:
	// 인벤은 "인스턴스 단위" 관리(스택은 Quantity)
	UPROPERTY() TMap<FGuid, FItemInstance> Instances;

	UPROPERTY() TSet<FGuid> EquippedInstances;

	// 내부 유틸
	TArray<FGuid> FindStackableInstances(FName ItemId) const;
};