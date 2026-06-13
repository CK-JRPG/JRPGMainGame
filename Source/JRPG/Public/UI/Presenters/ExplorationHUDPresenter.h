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

	void TogglePartyInfo();

	void StartPostCombatRegenUI(float Duration = 5.0f);

	void ShowExplorationUI();
	void HideExplorationUI();

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