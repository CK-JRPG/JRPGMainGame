#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatTagSwapWidget.generated.h"

class UCombatTagSwapSlotWidget;
class UCombatPartySlotViewModel;

UCLASS()
class JRPG_API UCombatTagSwapWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void UpdateSwapUI(UCombatPartySlotViewModel* LeftVM, UCombatPartySlotViewModel* RightVM);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatTagSwapSlotWidget> Slot_Q;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatTagSwapSlotWidget> Slot_E;
};
