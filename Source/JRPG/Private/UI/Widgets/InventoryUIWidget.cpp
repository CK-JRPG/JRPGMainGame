#include "UI/Widgets/InventoryUIWidget.h"
#include "UI/ViewModels/InventoryViewModel.h"
#include "Components/ListView.h"
#include "UI/Widgets/ItemWrapperObject.h"

void UInventoryUIWidget::SetViewModel(UInventoryViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->OnInventoryListUpdated.RemoveDynamic(this, &UInventoryUIWidget::OnInventoryListUpdated);
	}

	ViewModel = InViewModel;

	if (ViewModel)
	{
		ViewModel->OnInventoryListUpdated.AddDynamic(this, &UInventoryUIWidget::OnInventoryListUpdated);
	}
}

void UInventoryUIWidget::OnInventoryListUpdated(const TArray<FItemInstance>& Items)
{
	if (!ListView_Inventory) return;

	ListView_Inventory->ClearListItems();

	for (const FItemInstance& Item : Items)
	{
		UItemWrapperObject* Wrapper = NewObject<UItemWrapperObject>(this);
		Wrapper->ItemData = Item;
		if (ViewModel)
		{
			Wrapper->ItemDef = ViewModel->GetItemDefinition(Item.ItemId);
		}
		ListView_Inventory->AddItem(Wrapper);
	}
}

void UInventoryUIWidget::OnTabButtonClicked(EInventoryTab TabType)
{
	if (ViewModel)
	{
		ViewModel->FilterItems(TabType);
	}
}