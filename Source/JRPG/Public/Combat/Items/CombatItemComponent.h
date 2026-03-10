// Source/JRPGCombat/Public/Combat/Items/CombatItemComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatItemComponent.generated.h"

class UCombatUsableItemDataAsset;
class UCombatCharacterComponent;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class JRPG_API UCombatItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatItemComponent();

	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<UCombatUsableItemDataAsset>> RegisteredItemDefs;

	// 런타임 수량. 저장/로드/상점/탐험 보상과 연결 가능.
	UPROPERTY(EditAnywhere)
	TMap<FName, int32> ItemCounts;

	bool HasItem(FName ItemId, int32 Count = 1) const;
	int32 GetItemCount(FName ItemId) const;

	bool ConsumeItem(FName ItemId, int32 Count, FName ReasonTag);
	void RestoreItem(FName ItemId, int32 Count, FName ReasonTag);

	void AddItem(UCombatUsableItemDataAsset* ItemDef, int32 Count);
	void AddItemById(FName ItemId, int32 Count);

	UCombatUsableItemDataAsset* FindItemDef(FName ItemId) const;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UCombatCharacterComponent> CharacterComp;
	void BootstrapFromCharacterDef();
};
