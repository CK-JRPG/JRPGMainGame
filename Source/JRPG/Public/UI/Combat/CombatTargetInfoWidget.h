#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatTargetInfoWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class JRPG_API UCombatTargetInfoWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void UpdateTargetName(const FString& Name);
    void UpdateTargetHP(float Percent);
    //void UpdateGroggyState(bool bGroggy);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_TargetName;
    UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_TargetHP;
    UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_CatchUpBar;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Animation")
    float UpdateInterval = 0.016f;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Animation")
    float CatchUpSpeed = 5.0f;

    //UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_GroggyGauge;

private:
    FTimerHandle CatchUpTimerHandle;

    float TargetHPPercent = 1.0f;
    float CurrentCatchUpPercent = 1.0f;

    void OnCatchUpTimerTick();

};