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

	// 전체 탭이 사라졌으므로, 1개의 ListView만 사용하여 갈아끼웁니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> ListView_Inventory;

	UFUNCTION()
	void OnInventoryListUpdated(const TArray<FItemInstance>& Items);

	// 블루프린트 버튼(장비, 소비, 재료)의 OnClicked에서 이 함수를 호출합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|Events")
	void OnTabButtonClicked(EInventoryTab TabType);
};