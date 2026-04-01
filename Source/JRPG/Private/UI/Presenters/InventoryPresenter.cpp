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
	InventoryViewModel->FilterItems(EItemType::Weapon);

	MenuWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bIsOpen = true;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(CachedWorld.Get(), 0))
	{
		// 게임을 일시정지 할지 선택
		//PC->SetPause(true);
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
		// 게임을 일시정지 할지 선택
		//PC->SetPause(false);
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}