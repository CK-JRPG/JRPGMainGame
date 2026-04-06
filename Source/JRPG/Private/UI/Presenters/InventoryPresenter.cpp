#include "UI/Presenters/InventoryPresenter.h"
#include "UI/ViewModels/InventoryViewModel.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void UInventoryPresenter::Initialize(UWorld* World, TSubclassOf<UUserWidget> MenuWidgetClass)
{
	CachedWorld = World;
	if (!World || !MenuWidgetClass) return;

	InventoryViewModel = NewObject<UInventoryViewModel>(this);
	InventoryViewModel->Initialize(World);

	MenuWidgetInstance = CreateWidget<UUserWidget>(World, MenuWidgetClass);
	if (MenuWidgetInstance)
	{
		UFunction* Func = MenuWidgetInstance->FindFunction(FName("SetViewModel"));
		if (Func)
		{
			struct { UInventoryViewModel* VM; } Params;
			Params.VM = InventoryViewModel;
			MenuWidgetInstance->ProcessEvent(Func, &Params);
		}

		MenuWidgetInstance->AddToViewport(50);
		MenuWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UInventoryPresenter::OpenInventory(AActor* DefaultCharacter)
{
	if (!MenuWidgetInstance || !InventoryViewModel || bIsOpen) return;

	InventoryViewModel->SelectCharacter(DefaultCharacter);

	// 맨 처음 인벤토리를 열었을 때 무조건 장비 탭(0번)으로 초기화 & 정렬
	InventoryViewModel->FilterItems(EInventoryTab::Equipment);

	MenuWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bIsOpen = true;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(CachedWorld.Get(), 0))
	{
		PC->SetInputMode(FInputModeGameAndUI());
		PC->bShowMouseCursor = true;
	}
}

void UInventoryPresenter::CloseInventory()
{
	if (!MenuWidgetInstance || !InventoryViewModel || !bIsOpen) return;

	MenuWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	InventoryViewModel->ClearPreview();
	bIsOpen = false;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(CachedWorld.Get(), 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}