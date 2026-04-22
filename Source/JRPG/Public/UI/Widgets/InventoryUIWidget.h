#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/Items/InventoryTypes.h"
#include "Combat/Items/InventorySubsystem.h"
#include "InventoryUIWidget.generated.h"

class UListView;
class UInventoryViewModel;
enum class EInventoryTab : uint8;

UCLASS()
class JRPG_API UInventoryUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI|MVVM")
	void SetViewModel(UInventoryViewModel* InViewModel);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "UI|MVVM")
	TObjectPtr<UInventoryViewModel> ViewModel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> ListView_Inventory;

	UFUNCTION()
	void OnInventoryListUpdated(const TArray<FItemInstance>& Items);

	UFUNCTION(BlueprintCallable, Category = "UI|Events")
	void OnTabButtonClicked(EInventoryTab TabType);
};