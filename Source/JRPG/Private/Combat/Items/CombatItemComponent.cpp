// Source/JRPGCombat/Private/Combat/Items/CombatItemComponent.cpp
#include "Combat/Items/CombatItemComponent.h"

#include "Combat/Items/CombatUsableItemDataAsset.h"
#include "Combat/Characters/CombatCharacterComponent.h"
#include "Combat/Characters/CombatCharacterDataAsset.h"

UCombatItemComponent::UCombatItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatItemComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterComp = GetOwner() ? GetOwner()->FindComponentByClass<UCombatCharacterComponent>() : nullptr;
	BootstrapFromCharacterDef();

	for (UCombatUsableItemDataAsset* Def : RegisteredItemDefs)
	{
		if (!Def || !Def->IsValidItem()) continue;
		ItemCounts.FindOrAdd(Def->ItemId, ItemCounts.FindRef(Def->ItemId));
	}
}

void UCombatItemComponent::BootstrapFromCharacterDef()
{
	if (!CharacterComp.IsValid() || !CharacterComp->CharacterDef) return;

	for (const TPair<FName, int32>& KV : CharacterComp->CharacterDef->StartingItems)
	{
		if (KV.Key.IsNone() || KV.Value <= 0) continue;
		ItemCounts.FindOrAdd(KV.Key) += KV.Value;
	}
}

bool UCombatItemComponent::HasItem(FName ItemId, int32 Count) const
{
	return GetItemCount(ItemId) >= Count;
}

int32 UCombatItemComponent::GetItemCount(FName ItemId) const
{
	const int32* Found = ItemCounts.Find(ItemId);
	return Found ? *Found : 0;
}

bool UCombatItemComponent::ConsumeItem(FName ItemId, int32 Count, FName)
{
	if (ItemId.IsNone() || Count <= 0) return false;

	int32* Found = ItemCounts.Find(ItemId);
	if (!Found || *Found < Count) return false;

	*Found -= Count;
	if (*Found <= 0)
	{
		ItemCounts.Remove(ItemId);
	}
	return true;
}

void UCombatItemComponent::RestoreItem(FName ItemId, int32 Count, FName)
{
	if (ItemId.IsNone() || Count <= 0) return;
	ItemCounts.FindOrAdd(ItemId) += Count;
}

void UCombatItemComponent::AddItem(UCombatUsableItemDataAsset* ItemDef, int32 Count)
{
	if (!ItemDef || !ItemDef->IsValidItem() || Count <= 0) return;

	if (!RegisteredItemDefs.Contains(ItemDef))
	{
		RegisteredItemDefs.Add(ItemDef);
	}
	ItemCounts.FindOrAdd(ItemDef->ItemId) += Count;
}

void UCombatItemComponent::AddItemById(FName ItemId, int32 Count)
{
	if (ItemId.IsNone() || Count <= 0) return;
	ItemCounts.FindOrAdd(ItemId) += Count;
}

UCombatUsableItemDataAsset* UCombatItemComponent::FindItemDef(FName ItemId) const
{
	for (UCombatUsableItemDataAsset* Def : RegisteredItemDefs)
	{
		if (Def && Def->ItemId == ItemId)
			return Def;
	}
	return nullptr;
}
