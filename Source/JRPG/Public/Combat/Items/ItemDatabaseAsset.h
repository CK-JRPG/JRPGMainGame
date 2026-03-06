// Source/JRPGCombat/Public/Combat/Items/ItemDatabaseAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/Items/ItemDataAsset.h"
#include "ItemDatabaseAsset.generated.h"

// 정적 정의 SSOT: ItemDatabase(DataAsset/DT)
UCLASS()
class JRPGCOMBAT_API UItemDatabaseAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere) TArray<TObjectPtr<UItemDataAsset>> Items;

	// 런타임 lookup 캐시
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UItemDataAsset>> Map;

	virtual void PostLoad() override;
	virtual void PostInitProperties() override;

	const UItemDataAsset* FindItem(FName ItemId) const;
};