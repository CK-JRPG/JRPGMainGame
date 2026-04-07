#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h" // FPartyChatMsg 구조체용
#include "PartyChatBubbleWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class JRPG_API UPartyChatBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitChatMessage(const FPartyChatMsg& Msg);

	// 최대 5개가 넘어갔을 때 즉시 퇴장시키기 위해 C++에서 호출하는 함수
	UFUNCTION(BlueprintCallable)
	void ForceDismiss();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Profile;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	// 블루프린트에서 구현할 퇴장 애니메이션 이벤트 (텍스트 사라짐 -> 이미지 슬라이드 -> RemoveFromParent)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Animation")
	void PlayOutroAnimation();

private:
	bool bIsDismissing = false; // 중복 애니메이션 실행 방지
};