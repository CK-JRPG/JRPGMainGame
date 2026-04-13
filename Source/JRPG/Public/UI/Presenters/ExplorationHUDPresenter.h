#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "ExplorationHUDPresenter.generated.h"

class UExplorationUIWidget;
class UExplorationViewModel;
class UCombatPartySlotViewModel;
class UCombatPartySlotWidget;

UCLASS()
class JRPG_API UExplorationHUDPresenter : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* InWorld, TSubclassOf<UExplorationUIWidget> WidgetClass);
	void Shutdown();

	// 컨트롤러/HUD에서 Tab 키 입력 시 호출
	void TogglePartyInfo();

	// 전투 서브시스템 종료 이벤트에서 호출 (N초 세팅)
	void StartPostCombatRegenUI(float Duration = 5.0f);

	// 전투 진입/종료 시 JRPGHUD가 호출해 줄 함수
	void ShowExplorationUI();
	void HideExplorationUI();

	// 파티 데이터를 갱신하고 UI에 쏴주는 함수
	void RefreshPartyStatusData();

	// [콘솔 명령어용 함수]
	void TestRegionName(const FString& RegionName);
	void TestPartyChat(const FString& Message);

private:
	UPROPERTY() TObjectPtr<UExplorationUIWidget> ExplorationWidget;
	UPROPERTY() TObjectPtr<UExplorationViewModel> ViewModel;
	UPROPERTY()	TArray<UCombatPartySlotViewModel*> PartyViewModels;

	// 브릿지 콜백 (ViewModel -> View)
	void OnViewModelQuestUpdated(UTexture2D* QuestIcon, const FString& Objective);
	void OnPartyChatReceived(const FPartyChatMsg& Msg);
	void OnRegionChanged(const FString& RegionName);

	void OnBattleStarted(const FBattleSessionSnapshot& Snapshot);
	void OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);

	bool bIsTabInfoOpen = false;
	bool bIsPostCombatRegenActive = false;

	FTimerHandle PostCombatTimerHandle;

	void UpdatePartyStatusUI();
	void OnPostCombatTimerExpired();

	void OnPartySlotNameUpdated(const FString& Name, UCombatPartySlotWidget* SlotWidget);
	void OnPartySlotHPUpdated(float Percent, const FString& Text, UCombatPartySlotWidget* SlotWidget);
	void OnPartySlotAPUpdated(float Percent, UCombatPartySlotWidget* SlotWidget);
	void OnInteractableTargetChanged(bool bIsVisible, const FString& Text);
};