#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "ExplorationUIWidget.generated.h"

class UImage;
class UTextBlock;
class UOverlay;
class UTexture2D;
class UVerticalBox;
class UPartyChatBubbleWidget;
class UUserWidget;
class UCombatPartyRosterWidget;
class UCombatPartySlotViewModel;

UCLASS()
class JRPG_API UExplorationUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 뷰모델(Presenter)에서 호출해 줄 퀘스트 갱신 함수
	void UpdateQuestInfo(UTexture2D* QuestIcon, const FString& ObjectiveText);

	// 미니맵에 이정표(마커) 위젯을 동적으로 추가할 때 사용할 컨테이너 노출
	UOverlay* GetMinimapOverlay() const { return Overlay_MinimapMarkers; }

	// --- 파티 상태 & 지역명 갱신 ---
	// Mode: 0 = 숨김, 1 = 회복 모드(N초), 2 = 전체 정보 모드(Tab)
	void SetPartyStatusMode(int32 Mode);

	// --- 상호작용 & 대화 ---
	void ShowInteraction(bool bShow, const FString& Text);
	void ShowDialogue(bool bShow, const FString& Speaker, const FString& Text);

	void ShowRegionName(const FString& RegionName);
	void AddPartyChat(const FPartyChatMsg& Msg);

	UCombatPartyRosterWidget* GetPartyRoster() const { return Widget_PartyStatus; }
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

	// --- 지역명 UI ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RegionName;

	// 블루프린트에서 UMG 애니메이션(페이드 인 -> 대기 -> 페이드 아웃)을 재생하도록 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Animation")
	void PlayRegionNameAnimation();

	// --- 파티 대화 UI ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_PartyChat;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UPartyChatBubbleWidget> ChatBubbleClass;

	// 관리용 리스트
	UPROPERTY()
	TArray<UPartyChatBubbleWidget*> ActiveChatBubbles;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatPartyRosterWidget> Widget_PartyStatus;

	// --- 상호작용 UI ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Interaction;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_InteractionAction;

	// --- 대화창 UI ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Dialogue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DialogueSpeaker;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DialogueContent;

	// --- 블루프린트 애니메이션 이벤트 ---
	UFUNCTION(BlueprintImplementableEvent)
	void PlayPartyStatusAnim(bool bIsTabMode);
};