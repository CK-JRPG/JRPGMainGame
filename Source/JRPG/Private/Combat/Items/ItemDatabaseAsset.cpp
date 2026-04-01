// Source/JRPGCombat/Private/Combat/Items/ItemDatabaseAsset.cpp
#include "Combat/Items/ItemDatabaseAsset.h"

void UItemDatabaseAsset::PostInitProperties()
{
	Super::PostInitProperties();
	Map.Reset();
	for (UItemDataAsset* I : Items)
	{
		if (!I || I->ItemId.IsNone()) continue;
		Map.Add(I->ItemId, I);
	}
}

void UItemDatabaseAsset::PostLoad()
{
	Super::PostLoad();
	Map.Reset();
	for (UItemDataAsset* I : Items)
	{
		if (!I || I->ItemId.IsNone()) continue;
		Map.Add(I->ItemId, I);
	}
}

const UItemDataAsset* UItemDatabaseAsset::FindItem(FName ItemId) const
{
	if (const TObjectPtr<UItemDataAsset>* Found = Map.Find(ItemId))
		return Found->Get();

	UE_LOG(LogTemp, Warning, TEXT("ItemDatabaseAsset: Item Not Found"));
	return nullptr;
}