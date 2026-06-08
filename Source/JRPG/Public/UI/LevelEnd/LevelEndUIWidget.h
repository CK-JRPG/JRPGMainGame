#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelEndUIWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UWidget;

UCLASS()
class JRPG_API ULevelEndUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "JRPG|LevelEnd")
	void ShowEndScreen();

	UFUNCTION(BlueprintCallable, Category = "JRPG|LevelEnd")
	void RestartCurrentLevel();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Restart;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Fade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JRPG|LevelEnd", meta = (ClampMin = "0.0"))
	float FadeDuration = 1.25f;

private:
	void ApplyFadeOpacity(float Alpha);

	float FadeElapsed = 0.f;
	bool bFadeActive = false;
};
