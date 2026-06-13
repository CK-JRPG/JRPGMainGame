#include "Combat/Items/InventoryPresentationSubsystem.h"

#include "Combat/Items/ItemDataAsset.h"
#include "Combat/Stats/CombatStatsComponent.h"

FStatsPreviewDelta UInventoryPresentationSubsystem::PreviewAugmentDelta(UCombatStatsComponent* StatsComponent,
	const FAugmentModifierSet& CandidateMods) const
{
	if (!StatsComponent)
	{
		return FStatsPreviewDelta();
	}

	return StatsComponent->BuildPreviewDelta(CandidateMods);
}

TArray<FItemInstance> UInventoryPresentationSubsystem::FilterByType(const UInventorySubsystem* Inventory, EItemType ItemType) const
{
	TArray<FItemInstance> Out;
	if (!Inventory)
	{
		return Out;
	}

	TArray<FItemInstance> All;
	Inventory->GetAllInstances(All);
	for (const FItemInstance& Item : All)
	{
		const UItemDataAsset* Def = Inventory->FindDef(Item.ItemId);
		if (Def && Def->ItemType == ItemType)
		{
			Out.Add(Item);
		}
	}
	return Out;
}

TArray<FItemInstance> UInventoryPresentationSubsystem::SearchByName(const UInventorySubsystem* Inventory, const FString& Keyword) const
{
	TArray<FItemInstance> Out;
	if (!Inventory)
	{
		return Out;
	}

	const FString TrimmedKeyword = Keyword.TrimStartAndEnd();
	if (TrimmedKeyword.IsEmpty())
	{
		Inventory->GetAllInstances(Out);
		return Out;
	}

	TArray<FItemInstance> All;
	Inventory->GetAllInstances(All);
	for (const FItemInstance& Item : All)
	{
		const UItemDataAsset* Def = Inventory->FindDef(Item.ItemId);
		if (!Def)
		{
			continue;
		}

		if (Def->DisplayName.ToString().Contains(TrimmedKeyword))
		{
			Out.Add(Item);
		}
	}

	return Out;
}