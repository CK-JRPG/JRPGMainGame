#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatPartySlotWidget.generated.h"

UCLASS()
class JRPG_API UCombatPartySlotWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void UpdateName(const FString& Name);
    void UpdateHP(float Percent, const FString& Text);
    void UpdateAP(float Percent);
    void SetCharacterID(FName InID) { BoundID = InID; }
    FName GetCharacterID() const { return BoundID; }
    void SetIsActiveCharacter(bool bIsActive);

protected:
    //UPROPERTY(meta = (BindWidget)) class UImage* Image_Portrait;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_Name;
    UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_HPBar;
    UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_HP;
    UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_APBar;

private:
    FName BoundID;
};