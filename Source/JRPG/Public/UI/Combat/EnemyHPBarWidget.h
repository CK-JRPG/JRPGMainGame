#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHPBarWidget.generated.h"

UCLASS()
class JRPG_API UEnemyHPBarWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void UpdateHP(float Percent);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* PB_HPBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* PB_CatchUpBar;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Animation")
    float UpdateInterval = 0.016f;

    UPROPERTY(EditDefaultsOnly, Category = "UI|Animation")
    float CatchUpSpeed = 5.0f;

private:
    FTimerHandle CatchUpTimerHandle;

    float TargetHPPercent = 1.0f;
    float CurrentCatchUpPercent = 1.0f;

    void OnCatchUpTimerTick();
};