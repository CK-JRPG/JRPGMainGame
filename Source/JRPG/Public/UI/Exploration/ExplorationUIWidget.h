#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "UI/Exploration/EnemyIndicatorWidget.h"
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
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void UpdateQuestInfo(UTexture2D* QuestIcon, const FString& ObjectiveText);

	UOverlay* GetMinimapOverlay() const { return Overlay_MinimapMarkers; }

	// --- 파티 상태 & 지역명 갱신 ---
	// Mode: 0 = 숨김, 1 = 회복 모드(N초), 2 = 전체 정보 모드(Tab)
	void SetPartyStatusMode(int32 Mode);

	void ShowInteraction(bool bShow, const FString& Text);
	void ShowDialogue(bool bShow, const FString& Speaker, const FString& Text);

	void ShowRegionName(const FString& RegionName);
	void AddPartyChat(const FPartyChatMsg& Msg);

	UCombatPartyRosterWidget* GetPartyRoster() const { return Widget_PartyStatus; }
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_QuestIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_QuestObjective;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_MinimapRender;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_MinimapMarkers;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RegionName;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Animation")
	void PlayRegionNameAnimation();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_PartyChat;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UPartyChatBubbleWidget> ChatBubbleClass;

	UPROPERTY()
	TArray<UPartyChatBubbleWidget*> ActiveChatBubbles;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCombatPartyRosterWidget> Widget_PartyStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Interaction;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_InteractionAction;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Dialogue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DialogueSpeaker;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DialogueContent;

	UFUNCTION(BlueprintImplementableEvent)
	void PlayPartyStatusAnim(bool bIsTabMode);

	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* Canvas_Indicators;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Classes")
	TSubclassOf<UEnemyIndicatorWidget> IndicatorClass;

private:
	UPROPERTY()
	TArray<UEnemyIndicatorWidget*> CachedIndicatorWidgets;

	const float DetectionRadius = 3000.0f;
};