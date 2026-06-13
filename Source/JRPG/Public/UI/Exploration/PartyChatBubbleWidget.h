#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "PartyChatBubbleWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class JRPG_API UPartyChatBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitChatMessage(const FPartyChatMsg& Msg);

	UFUNCTION(BlueprintCallable)
	void ForceDismiss();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Profile;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Animation")
	void PlayOutroAnimation();

private:
	bool bIsDismissing = false;
};