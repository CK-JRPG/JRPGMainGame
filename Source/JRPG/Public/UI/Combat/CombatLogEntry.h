#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatLogEntry.generated.h"

UCLASS()
class JRPG_API UCombatLogEntry : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "CombatLog")
    void SetupLog(const FString& Message, class UTexture2D* Icon);

protected:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_LogMessage;

    UPROPERTY(meta = (BindWidgetOptional))
        class UImage* Img_SkillIcon;

private:
    FTimerHandle AutoDestroyTimerHandle;
    void RemoveLogEntry();
};
