#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExplorationUIWidget.generated.h"

class UImage;
class UTextBlock;
class UOverlay;
class UTexture2D;

UCLASS()
class JRPG_API UExplorationUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 뷰모델(Presenter)에서 호출해 줄 퀘스트 갱신 함수
	void UpdateQuestInfo(UTexture2D* QuestIcon, const FString& ObjectiveText);

	// 미니맵에 이정표(마커) 위젯을 동적으로 추가할 때 사용할 컨테이너 노출
	UOverlay* GetMinimapOverlay() const { return Overlay_MinimapMarkers; }

protected:
	// --- 퀘스트 관련 바인딩 ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_QuestIcon; // 퀘스트 유형 아이콘

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_QuestObjective; // 퀘스트 목표

	// --- 미니맵 관련 바인딩 ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_MinimapRender; // 미니맵 실제 렌더 타겟 이미지

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_MinimapMarkers; // 이정표(마커)들이 배치될 오버레이 패널
};