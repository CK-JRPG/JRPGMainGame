#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "Combat/Battle/BattleSessionSubsystem.h"

void UExplorationHUDPresenter::Initialize(UWorld* InWorld, TSubclassOf<UExplorationUIWidget> WidgetClass)
{
	if (!InWorld || !WidgetClass) return;

	ExplorationWidget = CreateWidget<UExplorationUIWidget>(InWorld, WidgetClass);
	if (ExplorationWidget)
	{
		ExplorationWidget->AddToViewport();
		ExplorationWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	ViewModel = NewObject<UExplorationViewModel>(this);
	ViewModel->Initialize(InWorld);
	ViewModel->OnQuestUpdated.AddUObject(this, &UExplorationHUDPresenter::OnViewModelQuestUpdated);

	if (UBattleSessionSubsystem* BattleSub = InWorld->GetSubsystem<UBattleSessionSubsystem>())
	{
		BattleSub->OnBattleStarted.AddUObject(this, &UExplorationHUDPresenter::OnBattleStarted);
		BattleSub->OnBattleEnded.AddUObject(this, &UExplorationHUDPresenter::OnBattleEnded);
	}

	// 3. 임시 통합 테스트용 퀘스트 데이터 로드 지시
	ViewModel->LoadTempQuestData();
}

void UExplorationHUDPresenter::Shutdown()
{
	if (ViewModel) ViewModel->Deinitialize();

	if (ExplorationWidget)
	{
		ExplorationWidget->RemoveFromParent();
		ExplorationWidget = nullptr;
	}

	if (UWorld* World = ExplorationWidget ? ExplorationWidget->GetWorld() : nullptr)
	{
		if (UBattleSessionSubsystem* BattleSub = World->GetSubsystem<UBattleSessionSubsystem>())
		{
			BattleSub->OnBattleStarted.RemoveAll(this);
			BattleSub->OnBattleEnded.RemoveAll(this);
		}
	}
}

void UExplorationHUDPresenter::ShowExplorationUI()
{
	if (ExplorationWidget) ExplorationWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UExplorationHUDPresenter::HideExplorationUI()
{
	if (ExplorationWidget) ExplorationWidget->SetVisibility(ESlateVisibility::Hidden);
}

void UExplorationHUDPresenter::OnViewModelQuestUpdated(UTexture2D* QuestIcon, const FString& Objective)
{
	if (ExplorationWidget)
	{
		ExplorationWidget->UpdateQuestInfo(QuestIcon, Objective);
	}
}

void UExplorationHUDPresenter::OnBattleStarted(const FBattleSessionSnapshot& Snapshot)
{
	HideExplorationUI();
}

void UExplorationHUDPresenter::OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
	ShowExplorationUI();
}