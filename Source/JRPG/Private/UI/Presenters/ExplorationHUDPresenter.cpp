#include "UI/Presenters/ExplorationHUDPresenter.h"
#include "UI/Exploration/ExplorationUIWidget.h"
#include "UI/ViewModels/ExplorationViewModel.h"
#include "UI/ViewModels/CombatViewModels.h"
#include "UI/Combat/CombatPartyRosterWidget.h"
#include "UI/Combat/CombatPartySlotWidget.h"
#include "Game/Companion/JRPGCompanionPawn.h"
#include "Game/HubSubsystem.h"
#include "Combat/Battle/BattleSessionSubsystem.h"
#include "Combat/Characters/PartyActorSpawnSubsystem.h"
#include "Combat/Characters/PartySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/Characters/CharacterRuntimeSubsystem.h"

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

	// 임시 통합 테스트용 퀘스트 데이터 로드
	ViewModel->LoadTempQuestData();

	if (UHubSubsystem* HubSub = InWorld->GetSubsystem<UHubSubsystem>())
	{
		HubSub->OnInteractableTargetChanged.AddUObject(this, &UExplorationHUDPresenter::OnInteractableTargetChanged);
	}

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
	//UE_LOG(LogTemp, Warning, TEXT("UExplorationHUDPresenter::StartPostCombatRegenUI"));
	if (bIsTabInfoOpen)
	{
		return;
	}


	bIsPostCombatRegenActive = true;
	//UE_LOG(LogTemp, Warning, TEXT("UExplorationHUDPresenter::StartPostCombatRegenUI : Update Party Status UI"));
	UpdatePartyStatusUI();

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

	RosterPanel->ClearRoster();
	for (auto& VM : PartyViewModels) { if (VM) VM->Unbind(); }
	PartyViewModels.Empty();

	TArray<FName> PartyIDs;
	if (UPartySubsystem* PartySys = GetWorld()->GetGameInstance()->GetSubsystem<UPartySubsystem>())
	{
		PartyIDs = PartySys->GetPartyIds();
	}


	for (FName CharID : PartyIDs)
	{
		//if (UCharacterRuntimeSubsystem* CRSys = GetWorld()->GetGameInstance()->GetSubsystem<UCharacterRuntimeSubsystem>())
		//{
		//	if (CRSys->HasSnapshot(CharID))
		//	{
		//		const FCharacterResourceSnapshot* Snapshot = CRSys->GetSnapshot(CharID);
		//		UE_LOG(LogTemp, Warning, TEXT("[SnapShot] %s, HP : %f, AP : %d, SP : %d"), *CharID.ToString(), Snapshot->HP, Snapshot->AP, Snapshot->SP);
		//	}
		//	else
		//	{
		//		UE_LOG(LogTemp, Warning, TEXT("UExplorationHUDPresenter : %s Snapshot is Invaild"), *CharID.ToString());
		//	}
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("UExplorationHUDPresenter : UCharacterRuntimeSubsystem is Invaild"));
		//}
		//UE_LOG(LogTemp, Warning, TEXT("PartyID : %s"), *CharID.ToString());
		if (CharID.IsNone() || !RosterPanel->PartySlotClass) continue;

		UCombatPartySlotWidget* SlotWidget = CreateWidget<UCombatPartySlotWidget>(GetWorld(), RosterPanel->PartySlotClass);

		UCombatPartySlotViewModel* SlotVM = NewObject<UCombatPartySlotViewModel>(this);

		SlotVM->OnNameUpdated.AddUObject(this, &UExplorationHUDPresenter::OnPartySlotNameUpdated, SlotWidget);
		SlotVM->OnHPUIUpdated.AddUObject(this, &UExplorationHUDPresenter::OnPartySlotHPUpdated, SlotWidget);
		SlotVM->OnAPUIUpdated.AddUObject(this, &UExplorationHUDPresenter::OnPartySlotAPUpdated, SlotWidget);

		SlotVM->BindToCharacter(CharID);

		SlotVM->Refresh();

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
	//UE_LOG(LogTemp, Warning, TEXT("UExplorationHUDPresenter::OnBattleEnded"));
	//RefreshPartyStatusData();
	//ShowExplorationUI();
	//StartPostCombatRegenUI();
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
		UE_LOG(LogTemp, Warning, TEXT("UExplorationHUDPresenter::UpdatePartyStatusUI : 전투 종료 후 파티 스탯 표시"));
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
	//UE_LOG(LogTemp, Error, TEXT("UExplorationHUDPresenter::OnPartySlotHPUpdated"));
	if (SlotWidget) SlotWidget->UpdateHP(Percent, Text);
}
void UExplorationHUDPresenter::OnPartySlotAPUpdated(float Percent, UCombatPartySlotWidget* SlotWidget)
{
	if (SlotWidget) SlotWidget->UpdateAP(Percent);
}

void UExplorationHUDPresenter::OnInteractableTargetChanged(bool bIsVisible, const FString& Text)
{
	if (ExplorationWidget)
	{
		ExplorationWidget->ShowInteraction(bIsVisible, Text);
	}
}
