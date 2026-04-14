#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageTextWidget.generated.h"

UCLASS()
class JRPG_API UDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    void InitializeDamage(AActor* InTarget, float DamageAmount, bool bIsCritical);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<class UTextBlock> Text_Damage;

    UPROPERTY(Transient, meta = (BindWidgetAnim))
    TObjectPtr<class UWidgetAnimation> Anim_FloatAndFade;

private:
    UFUNCTION() void OnFloatAnimFinished();
    TWeakObjectPtr<AActor> TargetActor;
    FVector WorldOffset;
};
