#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "UI/ViewModels/CombatViewModels.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "Game/Companion/JRPGCompanionPawn.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Kismet/GameplayStatics.h"

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
	ViewModel->OnPartyChatReceived.AddUObject(this, &UExplorationHUDPresenter::OnPartyChatReceived);
	ViewModel->OnRegionChanged.AddUObject(this, &UExplorationHUDPresenter::OnRegionChanged);

	if (UBattleSessionSubsystem* BattleSub = InWorld->GetSubsystem<UBattleSessionSubsystem>())
	{
		BattleSub->OnBattleStarted.AddUObject(this, &UExplorationHUDPresenter::OnBattleStarted);
		BattleSub->OnBattleEnded.AddUObject(this, &UExplorationHUDPresenter::OnBattleEnded);
	}

	// 3. 임시 통합 테스트용 퀘스트 데이터 로드 지시
	ViewModel->LoadTempQuestData();

	RefreshPartyStatusData();
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

void UExplorationHUDPresenter::TogglePartyInfo()
{
	bIsTabInfoOpen = !bIsTabInfoOpen;

	// Tab을 눌러서 열면, 기존에 돌고 있던 회복 UI 타이머를 무시/취소합니다.
	if (bIsTabInfoOpen && bIsPostCombatRegenActive)
	{
		if (UWorld* World = ExplorationWidget ? ExplorationWidget->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(PostCombatTimerHandle);
			bIsPostCombatRegenActive = false;
		}
	}

	UpdatePartyStatusUI();
}

void UExplorationHUDPresenter::StartPostCombatRegenUI(float Duration)
{
	// Tab 창이 켜져있다면 회복 UI 연출을 띄울 필요가 없습니다.
	if (bIsTabInfoOpen) return;

	bIsPostCombatRegenActive = true;
	UpdatePartyStatusUI();

	// N초 뒤에 UI를 끄는 타이머 설정
	if (UWorld* World = ExplorationWidget ? ExplorationWidget->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(
			PostCombatTimerHandle,
			this,
			&UExplorationHUDPresenter::OnPostCombatTimerExpired,
			Duration,
			false
		);
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

void UExplorationHUDPresenter::RefreshPartyStatusData()
{
	if (!ExplorationWidget || !ExplorationWidget->GetPartyRoster()) return;

	UCombatPartyRosterWidget* RosterPanel = ExplorationWidget->GetPartyRoster();

	// 1. 초기화
	RosterPanel->ClearRoster();
	for (auto& VM : PartyViewModels) { if (VM) VM->Unbind(); }
	PartyViewModels.Empty();

	// 2. 필드 액터 찾기 (서브시스템에서 가져오기)
	TArray<AActor*> FieldActors;

	if (UWorld* World = GetWorld())
	{
		// 2-1. 조종 중인 플레이어 (리더) 먼저 추가
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				FieldActors.Add(PlayerPawn);
			}
		}

		// 2-2. 필드에 스폰되어 있는 동료(AJRPGCompanionPawn)들 찾아서 추가
		TArray<AActor*> FoundCompanions;
		UGameplayStatics::GetAllActorsOfClass(World, AJRPGCompanionPawn::StaticClass(), FoundCompanions);

		for (AActor* Companion : FoundCompanions)
		{
			if (Companion)
			{
				FieldActors.Add(Companion);
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[탐험 UI] 서브시스템에서 찾은 필드 파티원 수: %d 명"), FieldActors.Num());

	// 3. 뷰모델 및 위젯 생성 & 바인딩
	for (AActor* Actor : FieldActors)
	{
		if (!Actor || !RosterPanel->PartySlotClass) continue;

		UCombatPartySlotWidget* SlotWidget = CreateWidget<UCombatPartySlotWidget>(GetWorld(), RosterPanel->PartySlotClass);
		UCombatPartySlotViewModel* SlotVM = NewObject<UCombatPartySlotViewModel>(this);

		SlotVM->OnNameUpdated.AddUObject(this, &UExplorationHUDPresenter::OnPartySlotNameUpdated, SlotWidget);
		SlotVM->OnHPUIUpdated.AddUObject(this, &UExplorationHUDPresenter::OnPartySlotHPUpdated, SlotWidget);
		SlotVM->OnAPUIUpdated.AddUObject(this, &UExplorationHUDPresenter::OnPartySlotAPUpdated, SlotWidget);

		SlotVM->BindToActor(Actor);

		PartyViewModels.Add(SlotVM);
		RosterPanel->AddPartySlot(SlotWidget);
	}
}
void UExplorationHUDPresenter::TestRegionName(const FString& RegionName)
{
	if (ViewModel) ViewModel->PushTestRegionName(RegionName);
}

void UExplorationHUDPresenter::TestPartyChat(const FString& Message)
{
	if(ViewModel) ViewModel->PushTestPartyChat(nullptr, Message);
}

void UExplorationHUDPresenter::OnViewModelQuestUpdated(UTexture2D* QuestIcon, const FString& Objective)
{
	if (ExplorationWidget)
	{
		ExplorationWidget->UpdateQuestInfo(QuestIcon, Objective);
	}
}

void UExplorationHUDPresenter::OnPartyChatReceived(const FPartyChatMsg& Msg)
{
	if (ExplorationWidget) ExplorationWidget->AddPartyChat(Msg);
}

void UExplorationHUDPresenter::OnRegionChanged(const FString& RegionName)
{
	if (ExplorationWidget) ExplorationWidget->ShowRegionName(RegionName);
}

void UExplorationHUDPresenter::OnBattleStarted(const FBattleSessionSnapshot& Snapshot)
{
	HideExplorationUI();
}

void UExplorationHUDPresenter::OnBattleEnded(const FBattleSessionSnapshot& Snapshot, EBattleEndReason Reason)
{
	ShowExplorationUI();
}

void UExplorationHUDPresenter::UpdatePartyStatusUI()
{
	if (!ExplorationWidget) return;

	if (bIsTabInfoOpen)
	{
		ExplorationWidget->SetPartyStatusMode(2); // Tab 모드 (프로필, 지역명 모두 표시)
	}
	else if (bIsPostCombatRegenActive)
	{
		ExplorationWidget->SetPartyStatusMode(1); // 회복 모드 (체력 위주 표시)
	}
	else
	{
		ExplorationWidget->SetPartyStatusMode(0); // 평상시 숨김
	}
}

void UExplorationHUDPresenter::OnPostCombatTimerExpired()
{
	bIsPostCombatRegenActive = false;
	UpdatePartyStatusUI();
}

void UExplorationHUDPresenter::OnPartySlotNameUpdated(const FString& Name, UCombatPartySlotWidget* SlotWidget)
{
	if (SlotWidget) SlotWidget->UpdateName(Name);
}
void UExplorationHUDPresenter::OnPartySlotHPUpdated(float Percent, const FString& Text, UCombatPartySlotWidget* SlotWidget)
{
	if (SlotWidget) SlotWidget->UpdateHP(Percent, Text);
}
void UExplorationHUDPresenter::OnPartySlotAPUpdated(float Percent, UCombatPartySlotWidget* SlotWidget)
{
	if (SlotWidget) SlotWidget->UpdateAP(Percent);
}
