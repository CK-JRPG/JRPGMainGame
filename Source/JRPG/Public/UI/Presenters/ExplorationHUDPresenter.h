#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/Battle/BattleSessionTypes.h"
#include "ExplorationHUDPresenter.generated.h"

class UExplorationUIWidget;
class UExplorationViewModel;

UCLASS()
class JRPG_API UExplorationHUDPresenter : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* InWorld, TSubclassOf<UExplorationUIWidget> WidgetClass);
	void Shutdown();

	// 전투 진입/종료 시 JRPGHUD가 호출해 줄 함수
	void ShowExplorationUI();
	void HideExplorationUI();

private:
	UPROPERTY() TObjectPtr<UExplorationUIWidget> ExplorationWidget;
	UPROPERTY() TObjectPtr<UExplorationViewModel> ViewModel;

	// 브릿지 콜백 (ViewModel -> View)
	void OnViewModelQuestUpdated(UTexture2D* QuestIcon, const FString& Objective);

	void OnBattleStarted(const FBattleSessionSnapshot& Snapshot);
	void OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason);
};