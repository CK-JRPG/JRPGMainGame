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
    UPROPERTY(meta = (BindWidget)) class UProgressBar* PB_HPBar;
};