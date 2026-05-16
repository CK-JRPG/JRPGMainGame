#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatActionPaletteWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UCombatPartySlotWidget;

UCLASS()
class JRPG_API UCombatActionPaletteWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    //void UpdateSPUI(float Percent, const FString& Text);
    //void UpdateHP(float Percent, const FString& Text);
    void UpdateAP(float Percent);
    void UpdateSkillList(const TArray<FString>& SkillNames);
    UCombatPartySlotWidget* GetPartySlot(int32 Index);
    void ClearAllPartySlots();

    UFUNCTION(BlueprintImplementableEvent, Category = "PartyWheel")
    void SetWheelModeActive(bool bActive);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PartyWheel")
    FName GetSelectedPartyMemberID() const;
    virtual FName GetSelectedPartyMemberID_Implementation() const;

protected:
    //UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_SPBar;
    //UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SP;
    //
    //UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_HPBar;
    //UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_HP;
    //
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar>PB_APBar;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Skill1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Skill2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Skill3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_Skill4;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_Key1;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_Key2;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_Key3;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> Img_Key4;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCombatPartySlotWidget> Slot_Party1;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCombatPartySlotWidget> Slot_Party2;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCombatPartySlotWidget> Slot_Party3;
};