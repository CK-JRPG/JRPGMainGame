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

	// FItemInstance 구조체를 UItemWrapperObject로 감싸서 ListView에 전달
	for (const FItemInstance& Item : Items)
	{
		UItemWrapperObject* Wrapper = NewObject<UItemWrapperObject>(this);
		Wrapper->ItemData = Item;
		ListView_Inventory->AddItem(Wrapper);
	}
}

void UInventoryUIWidget::OnTabButtonClicked(EInventoryTab TabType)
{
	if (ViewModel)
	{
		// 뷰는 판단하지 않고, 뷰모델에게 필터 변경 명령만 내립니다.
		ViewModel->FilterItems(TabType);
	}
}