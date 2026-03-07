// Source/JRPGCombat/Public/Combat/Items/ItemDatabaseAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Items/ItemDataAsset.h"
#include "ItemDatabaseAsset.generated.h"

UCLASS()
class JRPG_API UItemDatabaseAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TArray<TObjectPtr<UItemDataAsset>> Items;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UItemDataAsset>> Map;

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

	const UItemDataAsset* FindItem(FName ItemId) const;
};