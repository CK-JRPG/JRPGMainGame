#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatTagSwapSlotWidget.generated.h"

class UCombatPartySlotViewModel;
class UImage;
class UTextBlock;

UCLASS()
class JRPG_API UCombatTagSwapSlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void BindSwapData(UCombatPartySlotViewModel* InVM, const FString& KeyString);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Portrait;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KeyBadge;
};
