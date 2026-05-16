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
    //void UpdateTargetName(const FString& Name);
    void UpdateTargetHP(float Percent);
    //void UpdateGroggyState(bool bGroggy);

protected:
    //UPROPERTY(meta = (BindWidget)) class UTextBlock* Text_TargetName;
    UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_TargetHP;
    //UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_GroggyGauge;
};