#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatActionPaletteWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class JRPG_API UCombatActionPaletteWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void UpdateSPUI(float Percent, const FString& Text);
    void UpdateHP(float Percent, const FString& Text);
    void UpdateAP(float Percent);

protected:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_SPBar;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_SP;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> PB_HPBar;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Text_HP;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar>PB_APBar;
};