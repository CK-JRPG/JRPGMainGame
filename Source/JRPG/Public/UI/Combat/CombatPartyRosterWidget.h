#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatPartyRosterWidget.generated.h"

UCLASS()
class JRPG_API UCombatPartyRosterWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
    TSubclassOf<class UCombatPartySlotWidget> PartySlotClass;

    void ClearRoster();
    void AddPartySlot(UUserWidget* SlotWidget);

protected:
    UPROPERTY(meta = (BindWidget)) class UPanelWidget* PartyListContainer;
};