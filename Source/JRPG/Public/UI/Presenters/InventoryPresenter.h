#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryPresenter.generated.h"

class UInventoryViewModel;
class UUserWidget;

UCLASS(BlueprintType)
class JRPG_API UInventoryPresenter : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* World, TSubclassOf<UUserWidget> MenuWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "UI|Presenter")
	void OpenInventory(AActor* DefaultCharacter);

	UFUNCTION(BlueprintCallable, Category = "UI|Presenter")
	void CloseInventory();

	UFUNCTION(BlueprintPure, Category = "UI|Presenter")
	bool IsMenuOpen() const { return bIsOpen; }

private:
	UPROPERTY() TObjectPtr<UUserWidget> MenuWidgetInstance;
	UPROPERTY() TObjectPtr<UInventoryViewModel> InventoryViewModel;
	TWeakObjectPtr<UWorld> CachedWorld;

	bool bIsOpen = false;
};