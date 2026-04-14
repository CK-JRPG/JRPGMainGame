#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemSlotViewModel.generated.h"

class UInventorySubsystem;
class UItemDataAsset;

UCLASS()
class JRPG_API UItemSlotViewModel : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(FGuid InInstanceId, UInventorySubsystem* InInvSubsystem);

	UFUNCTION(BlueprintPure)
	int32 GetQuantity() const;

	UFUNCTION(BlueprintPure)
	bool IsEquipped() const;

	UFUNCTION(BlueprintPure)
	const UItemDataAsset* GetItemDef() const;

	UFUNCTION(BlueprintPure)
	FGuid GetInstanceId() const { return InstanceId; }

private:
	FGuid InstanceId;
	TWeakObjectPtr<UInventorySubsystem> WeakInvSubsystem;

};
