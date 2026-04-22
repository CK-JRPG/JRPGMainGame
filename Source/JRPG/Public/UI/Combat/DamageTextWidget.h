#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageTextWidget.generated.h"

UENUM(BlueprintType)
enum class EDamageTextType : uint8
{
    EnemyDamage,    // 적 피해(흰색)
    PlayerDamage,   // 아군 피해 (빨간색)
    Heal            // 회복 (초록색)
};

DECLARE_DELEGATE_OneParam(FOnDamageTextFinished, class UDamageTextWidget*);

UCLASS()
class JRPG_API UDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    void InitializeDamage(AActor* InTarget, float DamageAmount, bool bIsCritical, EDamageTextType TextType);

    FOnDamageTextFinished OnDamageTextFinished;

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
